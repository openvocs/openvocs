/***
        ------------------------------------------------------------------------

        Copyright (c) 2025 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the openvocs project. https://openvocs.org

        ------------------------------------------------------------------------
*//**
        @file           ov_vocs_app.c
        @author         Töpfer, Markus

        @date           2025-11-07


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_app.h"
#include "../include/ov_vocs_core.h"
#include "../include/ov_vocs_loop.h"
#include "../include/ov_mc_sip_msg.h"

#include <ov_base/ov_utils.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_event_keys.h>
#include <ov_base/ov_json_value.h>
#include <ov_base/ov_error_codes.h>
#include <ov_base/ov_result.h>
#include <ov_base/ov_id.h>

#include <ov_core/ov_response_state.h>
#include <ov_core/ov_broadcast_registry.h>
#include <ov_core/ov_event_api.h>
#include <ov_core/ov_event_async.h>
#include <ov_core/ov_event_engine.h>
#include <ov_core/ov_event_session.h>
#include <ov_core/ov_socket_json.h>
#include <ov_core/ov_media_definitions.h>

#include <ov_vocs_db/ov_vocs_db_app.h>

#define OV_VOCS_APP_MAGIC_BYTES 0xab1f

/*----------------------------------------------------------------------------*/

struct ov_vocs_app {

    uint16_t magic_byte;
    ov_vocs_app_config config;

    ov_event_engine *engine;
    
    ov_event_session *user_sessions;
    ov_socket_json *connections;

    ov_event_async_store *async;

    ov_dict *loops;

    ov_broadcast_registry *broadcasts;

};

/*----------------------------------------------------------------------------*/

static bool env_send(ov_vocs_app *self, int socket, const ov_json_value *input) {

    OV_ASSERT(self);
    OV_ASSERT(self->config.env.send);
    OV_ASSERT(self->config.env.userdata);

    bool result =
        self->config.env.send(self->config.env.userdata, socket, input);

    return result;
}

/*----------------------------------------------------------------------------*/

static bool send_socket(void *userdata, int socket, const ov_json_value *input) {

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !input) return false;

    return env_send(self, socket, input);
}

/*----------------------------------------------------------------------------*/

static bool send_error_response(ov_vocs_app *self,
                                const ov_json_value *input,
                                int socket,
                                uint64_t code,
                                const char *desc) {

    bool result = false;

    if (!self || !input) goto error;

    if (!desc) desc = OV_ERROR_DESC;

    ov_json_value *out = ov_event_api_create_error_response(input, code, desc);
    ov_event_api_set_type(out, OV_KEY_UNICAST);

    result = env_send(self, socket, out);

    out = ov_json_value_free(out);
    return result;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool send_success_response(ov_vocs_app *self,
                                  const ov_json_value *input,
                                  int socket,
                                  ov_json_value **response) {

    bool result = false;

    if (!self || !input) goto error;

    ov_json_value *out = ov_event_api_create_success_response(input);
    ov_event_api_set_type(out, OV_KEY_UNICAST);

    if (response && *response) {

        if (ov_json_object_set(out, OV_KEY_RESPONSE, *response))
            *response = NULL;
    }

    result = env_send(self, socket, out);

    out = ov_json_value_free(out);
    return result;

error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #BROADCAST FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool send_switch_loop_broadcast(ov_vocs_app *self,
                                       int socket,
                                       const char *loop,
                                       ov_vocs_permission current) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;
    ov_json_value *data = NULL;

    if (!self || !loop) goto error;

    data = ov_socket_json_get(self->connections, socket);
    const char *state = ov_vocs_permission_to_string(current);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));
    const char *client =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_CLIENT));

    ov_vocs_loop *l = ov_dict_get(self->loops, loop);
    ov_json_value *participants = ov_vocs_loop_get_participants(l);

    out = ov_event_api_message_create(OV_EVENT_API_SWITCH_LOOP_STATE, NULL, 0);

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_LOOP_BROADCAST))
        goto error;

    par = ov_event_api_set_parameter(out);
    if (!ov_json_object_set(par, OV_KEY_PARTICIPANTS, participants)) goto error;

    val = ov_json_string(loop);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val)) goto error;

    if (user) {

        val = ov_json_string(user);
        if (!ov_json_object_set(par, OV_KEY_USER, val)) goto error;
    }

    if (role) {

        val = ov_json_string(role);
        if (!ov_json_object_set(par, OV_KEY_ROLE, val)) goto error;
    }

    if (state) {

        val = ov_json_string(state);
        if (!ov_json_object_set(par, OV_KEY_STATE, val)) goto error;
    }

    if (client) {

        val = ov_json_string(client);
        if (!ov_json_object_set(par, OV_KEY_CLIENT, val)) goto error;
    }

    val = NULL;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            self->broadcasts, loop, &parameter, out, OV_LOOP_BROADCAST))
        goto error;

    data = ov_json_value_free(data);
    out = ov_json_value_free(out);

    return true;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool send_switch_loop_user_broadcast(ov_vocs_app *self,
                                            int socket,
                                            const char *loop,
                                            ov_vocs_permission current) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;
    ov_json_value *data = NULL;

    if (!self || !loop) goto error;

    data = ov_socket_json_get(self->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));
    const char *client =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_CLIENT));
    const char *state = ov_vocs_permission_to_string(current);

    ov_vocs_loop *l = ov_dict_get(self->loops, loop);
    ov_json_value *participants = ov_vocs_loop_get_participants(l);

    out = ov_event_api_message_create(OV_EVENT_API_SWITCH_LOOP_STATE, NULL, 0);

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_USER_BROADCAST))
        goto error;

    par = ov_event_api_set_parameter(out);
    if (!ov_json_object_set(par, OV_KEY_PARTICIPANTS, participants)) goto error;

    val = ov_json_string(loop);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val)) goto error;

    if (user) {

        val = ov_json_string(user);
        if (!ov_json_object_set(par, OV_KEY_USER, val)) goto error;
    }

    if (role) {

        val = ov_json_string(role);
        if (!ov_json_object_set(par, OV_KEY_ROLE, val)) goto error;
    }

    if (state) {

        val = ov_json_string(state);
        if (!ov_json_object_set(par, OV_KEY_STATE, val)) goto error;
    }

    if (client) {

        val = ov_json_string(client);
        if (!ov_json_object_set(par, OV_KEY_CLIENT, val)) goto error;
    }

    val = NULL;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            self->broadcasts, user, &parameter, out, OV_USER_BROADCAST))
        goto error;

    out = ov_json_value_free(out);
    data = ov_json_value_free(data);

    return true;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool send_switch_volume_user_broadcast(ov_vocs_app *self,
                                              int socket,
                                              const char *loop,
                                              uint8_t volume) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;
    ov_json_value *data = NULL;

    if (!self || !loop) goto error;

    data = ov_socket_json_get(self->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));
    const char *client =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_CLIENT));

    if (!user) goto error;

    out = ov_event_api_message_create(OV_EVENT_API_SWITCH_LOOP_VOLUME, NULL, 0);

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_USER_BROADCAST))
        goto error;

    par = ov_event_api_set_parameter(out);

    val = ov_json_string(loop);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val)) goto error;

    val = ov_json_number(volume);
    if (!ov_json_object_set(par, OV_KEY_VOLUME, val)) goto error;

    if (user) {

        val = ov_json_string(user);
        if (!ov_json_object_set(par, OV_KEY_USER, val)) goto error;
    }

    if (role) {

        val = ov_json_string(role);
        if (!ov_json_object_set(par, OV_KEY_ROLE, val)) goto error;
    }

    if (client) {

        val = ov_json_string(client);
        if (!ov_json_object_set(par, OV_KEY_CLIENT, val)) goto error;
    }

    val = NULL;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            self->broadcasts, user, &parameter, out, OV_USER_BROADCAST))
        goto error;

    out = ov_json_value_free(out);
    data = ov_json_value_free(data);
    return true;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool send_talking_loop_broadcast(ov_vocs_app *self,
                                        int socket,
                                        const ov_event_parameter *params,
                                        const char *loop,
                                        const ov_json_value *state,
                                        const char *client) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;
    ov_json_value *data = NULL;

    UNUSED(params);

    if (!self || !loop) goto error;

    data = ov_socket_json_get(self->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    out = ov_event_api_message_create(OV_EVENT_API_TALKING, NULL, 0);

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_LOOP_BROADCAST))
        goto error;

    par = ov_event_api_set_parameter(out);

    val = ov_json_string(loop);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val)) goto error;

    if (user) {

        val = ov_json_string(user);
        if (!ov_json_object_set(par, OV_KEY_USER, val)) goto error;
    }

    if (role) {

        val = ov_json_string(role);
        if (!ov_json_object_set(par, OV_KEY_ROLE, val)) goto error;
    }

    if (state) {

        val = NULL;
        if (!ov_json_value_copy((void **)&val, state)) goto error;

        if (!ov_json_object_set(par, OV_KEY_STATE, val)) goto error;
    }

    if (client) {

        val = ov_json_string(client);
        if (!ov_json_object_set(par, OV_KEY_CLIENT, val)) goto error;
    }

    val = NULL;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            self->broadcasts, loop, &parameter, out, OV_LOOP_BROADCAST)) {

        goto error;
    }

    out = ov_json_value_free(out);
    data = ov_json_value_free(data);
    return true;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(data);
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #DROP EVENTS
 *
 *      ------------------------------------------------------------------------
 */

struct container_drop {

    ov_vocs_app *vocs;
    int socket;
    ov_json_value *data;
};

/*----------------------------------------------------------------------------*/

static bool close_participation(const void *key, void *val, void *data) {

    if (!key) return true;

    struct container_drop *c = (struct container_drop *)data;
    ov_vocs_loop *l = ov_vocs_loop_cast(val);

    const ov_json_value *loops = ov_json_get(data, "/" OV_KEY_LOOPS);

    ov_vocs_permission current = ov_vocs_permission_from_string(
        ov_json_string_get(ov_json_object_get(loops, (char *)key)));

    ov_vocs_loop_drop_participant(l, c->socket);

    switch (current) {

        case OV_VOCS_NONE:
            return true;

        default:
            if (!send_switch_loop_broadcast(
                    c->vocs, c->socket, key, OV_VOCS_NONE)) {
                ov_log_error("failed to send switch loop broadcast %s", key);
            }
    }

    return true;
}


/*----------------------------------------------------------------------------*/

static bool drop_connection(ov_vocs_app *self, int socket){

    ov_json_value *data = NULL;

    if (!self || socket < 0) goto error;

    ov_log_debug("dropping connection %i", socket);

    data = ov_socket_json_get(self->connections, socket);
    const char *session =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));

    ov_vocs_core_connection_drop(self->config.core, socket, session);

    ov_event_async_drop(self->async, socket);
    ov_broadcast_registry_unset(self->broadcasts, socket);

    struct container_drop container = {
        .vocs = self, .socket = socket, .data = data};

    ov_dict_for_each(self->loops, &container, close_participation);

    ov_socket_json_drop(self->connections, socket);

    self->config.env.close(self->config.env.userdata, socket);
    data = ov_json_value_free(data);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_drop_connection(ov_vocs_app *self, int socket){

    return drop_connection(self, socket);
}

/*----------------------------------------------------------------------------*/

static void async_timedout(void *userdata, ov_event_async_data data) {

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self) goto error;

    char *str = ov_json_value_to_string(data.value);
    ov_log_error("Async timeout - dropping %i | %s", data.socket, str);
    str = ov_data_pointer_free(str);

    ov_json_value *out = ov_event_api_create_error_response(
        data.value, OV_ERROR_TIMEOUT, OV_ERROR_DESC_TIMEOUT);
    env_send(self, data.socket, out);
    out = ov_json_value_free(out);

    drop_connection(self, data.socket);

error:
    ov_event_async_data_clear(&data);
    return;
}



/*
 *      ------------------------------------------------------------------------
 *
 *      #API #AUTH FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_ldap_authentication(void *userdata,
                                   const char *uuid,
                                   ov_ldap_auth_result result) {

    ov_json_value *out = NULL;
    ov_json_value *data = NULL;

    ov_event_async_data adata = {0};

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    OV_ASSERT(self);
    if (!self) goto error;

    adata = ov_event_async_unset(self->async, uuid);
    if (!adata.value) goto error;

    data = ov_socket_json_get(self->connections, adata.socket);

    const char *client_id =
        ov_json_string_get(ov_json_object_get(adata.value, OV_KEY_CLIENT));

    const char *user = ov_json_string_get(
        ov_json_get(adata.value, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    const char *session_id = NULL;

    switch (result) {

        case OV_LDAP_AUTH_REJECTED:

            ov_log_error(
                "LDAP AUTHENTICATE failed at %i | %s", adata.socket, user);

            send_error_response(self,
                                adata.value,
                                adata.socket,
                                OV_ERROR_CODE_AUTH,
                                OV_ERROR_DESC_AUTH);

            drop_connection(self, adata.socket);
            goto error;
            break;

        case OV_LDAP_AUTH_GRANTED:
            ov_log_info(
                "LDAP AUTHENTICATE granted at %i | %s", adata.socket, user);
            break;
    }

    session_id = ov_event_session_init(self->user_sessions, client_id, user);

    /* successful authenticated */

    if (!ov_broadcast_registry_set(
            self->broadcasts, user, adata.socket, OV_USER_BROADCAST)) {
        goto error;
    }

    if (!ov_broadcast_registry_set(self->broadcasts,
                                   OV_BROADCAST_KEY_SYSTEM_BROADCAST,
                                   adata.socket,
                                   OV_SYSTEM_BROADCAST)) {
        goto error;
    }

    ov_json_object_set(data, OV_KEY_CLIENT, ov_json_string(client_id));
    ov_json_object_set(data, OV_KEY_USER, ov_json_string(user));

    ov_socket_json_set(self->connections, adata.socket, &data);

    out = ov_json_object();
    if (!ov_vocs_json_set_id(out, user)) goto error;
    if (!ov_vocs_json_set_session_id(out, session_id)) goto error;

    result = send_success_response(self, adata.value, adata.socket, &out);
    if (result) {
        ov_log_info("VOCS AUTHENTICATE LDAP at %i | %s", adata.socket, user);
    } else {
        ov_log_error(
            "VOCS AUTHENTICATE LDAP failed at %i | %s", adata.socket, user);
    }

error:
    ov_json_value_free(out);
    ov_event_async_data_clear(&adata);
    ov_json_value_free(data);
    return;
}

/*----------------------------------------------------------------------------*/

static bool client_login(void *userdata,
                    const int socket,
                    const ov_event_parameter *parameter,
                    ov_json_value *input){

    ov_json_value *data = NULL;
    ov_json_value *out = NULL;

    bool result = false;

    UNUSED(parameter);

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (user) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_ALREADY_AUTHENTICATED,
                            OV_ERROR_DESC_ALREADY_AUTHENTICATED);

        goto error;
    }

    const char *session_id = NULL;
    const char *pass = NULL;

    const char *uuid = ov_event_api_get_uuid(input);
    if (!uuid) goto error;

    const char *client_id =
        ov_json_string_get(ov_json_object_get(input, OV_KEY_CLIENT));

    const char *session_user =
        ov_event_session_get_user(self->user_sessions, client_id);

    user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));
    pass = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD));

    if (session_user && user && pass) {

        if (ov_event_session_verify(
                self->user_sessions, client_id, user, pass)) {

            ov_log_debug("relogin session for client %s", client_id);

            goto login_session_user;
        }
    }

    if (!user || !pass) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (self->config.ldap) {

        ov_log_debug("Requesting LDAP authentication for user %s", user);

        if (!ov_event_async_set(
                self->async,
                uuid,
                (ov_event_async_data){.socket = socket,
                                      .value = input,
                                      .timedout.userdata = self,
                                      .timedout.callback = async_timedout},
                self->config.limits.response_usec)) {

            goto error;
        }

        return ov_ldap_authenticate_password(
            self->config.ldap,
            user,
            pass,
            uuid,
            (ov_ldap_auth_callback){
                .userdata = self, .callback = cb_ldap_authentication});
    }

    if (!ov_vocs_db_authenticate(self->config.db, user, pass)) {

        send_error_response(
            self, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        ov_log_error("VOCS AUTHENTICATE local failed at %i | %s", socket, user);
        goto error;
    }

login_session_user:

    /* successful authenticated */

    if (!ov_broadcast_registry_set(
            self->broadcasts, user, socket, OV_USER_BROADCAST)) {
        goto error;
    }

    if (!ov_broadcast_registry_set(self->broadcasts,
                                   OV_BROADCAST_KEY_SYSTEM_BROADCAST,
                                   socket,
                                   OV_SYSTEM_BROADCAST)) {
        goto error;
    }

    session_id = ov_event_session_init(self->user_sessions, client_id, user);

    /* successful authenticated */

    ov_json_object_set(data, OV_KEY_CLIENT, ov_json_string(client_id));
    ov_json_object_set(data, OV_KEY_USER, ov_json_string(user));

    ov_socket_json_set(self->connections, socket, &data);

    out = ov_json_object();
    if (!ov_vocs_json_set_id(out, user)) goto error;
    if (!ov_vocs_json_set_session_id(out, session_id)) goto error;

    result = send_success_response(self, input, socket, &out);
    if (result) {
        ov_log_info("VOCS AUTHENTICATE local at %i | %s", socket, user);
    } else {
        ov_log_error("VOCS AUTHENTICATE local failed at %i | %s", socket, user);
    }

    /* close socket if authentication messaging failed */
error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return result;
}

/*----------------------------------------------------------------------------*/

static bool client_logout(void *userdata,
                    const int socket,
                    const ov_event_parameter *parameter,
                    ov_json_value *input){

    ov_json_value *data = NULL;

    bool result = false;
    UNUSED(parameter);

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);
    const char *session =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *client =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_CLIENT));

    send_success_response(self, input, socket, NULL);

    ov_log_info("client logout %i|%s user %s", socket, session, user);

    ov_event_session_delete(self->user_sessions, client);

    /* Close connection socket if drop is not successfull */
    result = drop_connection(self, socket);
    data = ov_json_value_free(data);

error:
    ov_json_value_free(input);
    ov_json_value_free(data);
    return result;
}

/*----------------------------------------------------------------------------*/

static bool update_client_login(void *userdata,
                                int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    bool result = false;
    ov_json_value *out = NULL;

    UNUSED(params);
    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    const char *client_id =
        ov_json_string_get(ov_json_object_get(input, OV_KEY_CLIENT));

    const char *session_id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION));

    const char *user_id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!client_id || !session_id || !user_id) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (!ov_event_session_update(
            self->user_sessions, client_id, user_id, session_id)) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    out = ov_json_object();
    if (!ov_vocs_json_set_id(
            out, ov_event_session_get_user(self->user_sessions, client_id)))
        goto error;

    if (!ov_vocs_json_set_session_id(out, session_id)) goto error;

    result = send_success_response(self, input, socket, &out);
    return true;

    /* close socket if authentication messaging failed */
error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    return result;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #API #CLIENT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

static bool client_media(void *userdata,
                                int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input)  {

    UNUSED(params);
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    
    const char *session =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));

    if (!user) goto error;

    const char *uuid = ov_event_api_get_uuid(input);

    const char *sdp = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_SDP));

    ov_media_type type = ov_media_type_from_string(ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE)));

    if (!uuid) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    switch (type) {

        case OV_MEDIA_REQUEST:

            if (!ov_vocs_core_session_create(
                self->config.core,
                uuid,
                OV_VOCS_DEFAULT_SDP)){

                send_error_response(self,
                                    input,
                                    socket,
                                    OV_ERROR_CODE_PROCESSING_ERROR,
                                    OV_ERROR_DESC_PROCESSING_ERROR);

                goto error;
            }

            break;

        case OV_MEDIA_OFFER:
        case OV_MEDIA_ANSWER:

            if (!sdp) {

                send_error_response(self,
                                    input,
                                    socket,
                                    OV_ERROR_CODE_PARAMETER_ERROR,
                                    OV_ERROR_DESC_PARAMETER_ERROR);

                goto error;
            }

            if (!session) {

                send_error_response(self,
                                    input,
                                    socket,
                                    OV_ERROR_CODE_SESSION_UNKNOWN,
                                    OV_ERROR_DESC_SESSION_UNKNOWN);

                goto error;
            }



            if (!ov_vocs_core_session_update(
                    self->config.core, uuid, session, type, sdp)) {

                send_error_response(self,
                                    input,
                                    socket,
                                    OV_ERROR_CODE_PROCESSING_ERROR,
                                    OV_ERROR_DESC_PROCESSING_ERROR);

                goto error;
            }

            break;

        default:

            send_error_response(self,
                                input,
                                socket,
                                OV_ERROR_CODE_PARAMETER_ERROR,
                                OV_ERROR_DESC_PARAMETER_ERROR);

            goto error;
    }

    if (!ov_event_async_set(
            self->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = self,
                                  .timedout.callback = async_timedout},
            self->config.limits.response_usec)) {

        char *str = ov_json_value_to_string(input);
        ov_log_error("failed to set async msg - closing %i | %s", socket, str);
        str = ov_data_pointer_free(str);

        goto error;
    }

    input = NULL;
    data = ov_json_value_free(data);
    return true;
error:
    /* let socket close in case of media setup errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_candidate(void *userdata,
                                int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input)  {

    UNUSED(params);
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *session =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));

    if (!user) goto error;

    const char *uuid = ov_event_api_get_uuid(input);
    ov_json_value *par = ov_event_api_get_parameter(input);
    ov_ice_candidate_info info = ov_ice_candidate_info_from_json(par);

    if (!uuid || !info.candidate || !info.ufrag) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (NULL == session) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_SESSION_UNKNOWN,
                            OV_ERROR_DESC_SESSION_UNKNOWN);

        goto error;
    }

    if (!ov_vocs_core_candidate(self->config.core, uuid, session, &info)) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    if (!ov_event_async_set(
            self->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = self,
                                  .timedout.callback = async_timedout},
            self->config.limits.response_usec)) {

        char *str = ov_json_value_to_string(input);
        ov_log_error("failed to set async msg - closing %i | %s", socket, str);
        str = ov_data_pointer_free(str);

        goto error;
    }

    data = ov_json_value_free(data);
    return true;

error:
    /* let socket close in case of media setup errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_end_of_candidates(void *userdata,
                                int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    UNUSED(params);
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *session =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));

    if (!user) goto error;

    const char *uuid = ov_event_api_get_uuid(input);

    if (!uuid) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (NULL == session) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_SESSION_UNKNOWN,
                            OV_ERROR_DESC_SESSION_UNKNOWN);

        goto error;
    }

    if (!ov_vocs_core_end_of_candidates(self->config.core, uuid, session)) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    if (!ov_event_async_set(
            self->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = self,
                                  .timedout.callback = async_timedout},
            self->config.limits.response_usec)) {

        char *str = ov_json_value_to_string(input);
        ov_log_error("failed to set async msg - closing %i | %s", socket, str);
        str = ov_data_pointer_free(str);

        goto error;
    }

    data = ov_json_value_free(data);
    return true;

error:
    /* let socket close in case of media setup errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_authorize(void *userdata,
                             int socket,
                             const ov_event_parameter *params,
                             ov_json_value *input) {

    ov_json_value *data = NULL;
    ov_json_value *out = NULL;
    bool result = false;

    UNUSED(params);

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user) goto error;

    if (role) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_NOT_IMPLEMENTED,
                            "current status: "
                            "changing a role MUST be done using logout/login.");

        goto error;
    }

    role = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ROLE));

    if (!role) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    /* process authorization request */

    if (!ov_vocs_db_authorize(self->config.db, user, role)) {

        send_error_response(
            self, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    ov_json_object_set(data, OV_KEY_ROLE, ov_json_string(role));

    if (!ov_broadcast_registry_set(
            self->broadcasts, role, socket, OV_ROLE_BROADCAST)) {
        goto error;
    }

    out = ov_json_object();
    if (!ov_vocs_json_set_id(out, role)) goto error;

    result = send_success_response(self, input, socket, &out);
    if (result) {
        ov_log_info("VOCS AUTHORIZE at %i | %s | %s", socket, user, role);
    } else {
        ov_log_error("VOCS AUTHORIZE failed at %i | %s", socket, user);
    }

    ov_socket_json_set(self->connections, socket, &data);

    /* close socket if authorization messaging failed */
error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return result;
}

/*----------------------------------------------------------------------------*/

static bool client_get(void *userdata,
                       int socket,
                       const ov_event_parameter *params,
                       ov_json_value *input) {

    bool result = false;
    ov_json_value *val = NULL;
    ov_json_value *out = NULL;
    ov_json_value *data = NULL;

    UNUSED(params);

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) {

        send_error_response(
            self, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    ov_json_value *parameter = ov_event_api_get_parameter(input);
    const char *type = ov_event_api_get_type(parameter);

    if (!parameter || !type) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (0 == strcmp(type, OV_KEY_USER)) {

        val = ov_vocs_db_get_entity(self->config.db, OV_VOCS_DB_USER, user);
        out = ov_json_object();
        if (!ov_json_object_set(out, OV_KEY_RESULT, val)) goto error;

        val = ov_json_string(type);
        if (!ov_json_object_set(out, OV_KEY_TYPE, val)) goto error;

        val = ov_vocs_db_get_entity_domain(
            self->config.db, OV_VOCS_DB_USER, user);
        if (val) ov_json_object_set(out, OV_KEY_DOMAIN, val);

        val = NULL;
        result = send_success_response(self, input, socket, &out);

    } else {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_NOT_IMPLEMENTED,
                            "only GET user implemented yet.");

        TODO("... to be implemented");
    }

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return result;
}

/*----------------------------------------------------------------------------*/

static bool client_user_roles(void *userdata,
                              int socket,
                              const ov_event_parameter *params,
                              ov_json_value *input) {

    bool result = false;
    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_json_value *data = NULL;

    UNUSED(params);

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) goto error;

    val = ov_vocs_db_get_user_roles(self->config.db, user);

    if (!val) {

        send_error_response(
            self, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    out = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_ROLES, val)) goto error;

    val = NULL;

    result = send_success_response(self, input, socket, &out);

error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return result;
}

/*----------------------------------------------------------------------------*/

struct container_loop_broadcasts {

    ov_vocs_app *vocs;
    int socket;

    const char *project;
    const char *user;
    const char *role;
};

/*----------------------------------------------------------------------------*/

static bool add_loop_broadcast_and_state(const void *key,
                                         void *val,
                                         void *data) {

    if (!key) return true;

    UNUSED(val);

    char *name = (char *)key;
    struct container_loop_broadcasts *c =
        (struct container_loop_broadcasts *)data;

    return ov_broadcast_registry_set(
        c->vocs->broadcasts, name, c->socket, OV_LOOP_BROADCAST);
}

/*----------------------------------------------------------------------------*/

static bool client_role_loops(void *userdata,
                              int socket,
                              const ov_event_parameter *params,
                              ov_json_value *input) {

    bool result = false;
    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    UNUSED(params);

    ov_json_value *data = NULL;

    UNUSED(params);

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            self, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    val = ov_vocs_db_get_user_role_loops(self->config.db, user, role);

    if (!val) {

        send_error_response(
            self, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    struct container_loop_broadcasts c = (struct container_loop_broadcasts){
        .vocs = self, .socket = socket, .user = user, .role = role};

    if (!ov_json_object_for_each(val, &c, add_loop_broadcast_and_state))
        goto error;

    out = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_LOOPS, val)) goto error;

    val = NULL;

    result = send_success_response(self, input, socket, &out);

error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return result;
}

/*----------------------------------------------------------------------------*/

static bool client_switch_loop_state(void *userdata,
                                     int socket,
                                     const ov_event_parameter *params,
                                     ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_json_value *data = NULL;

    UNUSED(params);

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *sess =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            self, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    if (NULL == sess) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_SESSION_UNKNOWN,
                            OV_ERROR_DESC_SESSION_UNKNOWN);

        goto error;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    const char *state = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE));

    if (!uuid || !loop || !state) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (!(ov_json_is_true(ov_json_get(data, "/" OV_KEY_ICE)) &&
          (ov_json_is_true(ov_json_get(data, "/" OV_KEY_MEDIA_READY)))))
        goto error;

    ov_vocs_permission requested = ov_vocs_permission_from_string(state);
    ov_vocs_permission permission =
        ov_vocs_db_get_permission(self->config.db, role, loop);

    if (!ov_vocs_permission_granted(permission, requested)) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_AUTH_PERMISSION,
                            OV_ERROR_DESC_AUTH_PERMISSION);

        goto error;
    }

    const ov_json_value *loops = ov_json_get(data, "/" OV_KEY_LOOPS);

    ov_vocs_permission current = ov_vocs_permission_from_string(
        ov_json_string_get(ov_json_object_get(loops, loop)));

    if (current == requested) {

        ov_vocs_loop *l = ov_dict_get(self->loops, loop);

        if (!l) {

            l = ov_vocs_loop_create(loop);
            char *name = ov_string_dup(loop);

            if (!ov_dict_set(self->loops, name, l, NULL)) {
                l = ov_vocs_loop_free(l);
                name = ov_data_pointer_free(name);
                goto error;
            }
        }

        ov_json_value *participants = ov_vocs_loop_get_participants(l);

        out = ov_json_object();

        if (!ov_json_object_set(out, OV_KEY_PARTICIPANTS, participants))
            goto error;

        val = ov_json_string(ov_vocs_permission_to_string(current));
        if (!ov_json_object_set(out, OV_KEY_STATE, val)) goto error;

        val = ov_json_string(loop);
        if (!ov_json_object_set(out, OV_KEY_LOOP, val)) goto error;

        val = NULL;

        send_success_response(self, input, socket, &out);

        out = ov_json_value_free(out);
        input = ov_json_value_free(input);

        goto done;
    }

    if (!ov_vocs_core_perform_switch_loop_request(
            self->config.core, uuid, sess, user, role, loop, current, requested)) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    /* we requested either some loop aquisition or switch loop */

    if (!ov_event_async_set(
            self->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = self,
                                  .timedout.callback = async_timedout},
            self->config.limits.response_usec)) {

        goto error;
    }

done:
    data = ov_json_value_free(data);
    return true;

error:
    /* let socket close in case of switch media errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_switch_loop_volume(void *userdata,
                                      int socket,
                                      const ov_event_parameter *params,
                                      ov_json_value *input) {

    UNUSED(params);
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));
    const char *sess =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));

    if (!user || !role) {

        send_error_response(
            self, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    uint64_t percent = ov_json_number_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_VOLUME));

    if (!uuid || !loop || (percent > 100)) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (NULL == sess) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_SESSION_UNKNOWN,
                            OV_ERROR_DESC_SESSION_UNKNOWN);

        goto error;
    }

    if (!ov_vocs_core_set_loop_volume(self->config.core,
                                       uuid,
                                       sess,
                                       loop,
                                       percent)) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    if (!ov_event_async_set(
            self->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = self,
                                  .timedout.callback = async_timedout},
            self->config.limits.response_usec)) {

        goto error;
    }

    send_switch_volume_user_broadcast(self, socket, loop, percent);

    ov_json_value_free(data);
    return true;
error:
    /* let socket close in case of switch media errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_talking(void *userdata,
                           int socket,
                           const ov_event_parameter *params,
                           ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    UNUSED(params);

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            self, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    ov_json_value const *state =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE);

    bool off = ov_json_is_false(state);

    const char *client_id =
        ov_json_string_get(ov_json_get(input, "/" OV_KEY_CLIENT));

    ov_vocs_core_ptt(self->config.core, user, role, loop, off);

    if (!uuid || !loop || !state) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    ov_vocs_permission permission =
        ov_vocs_db_get_permission(self->config.db, role, loop);

    if (permission != OV_VOCS_SEND) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_AUTH_PERMISSION,
                            OV_ERROR_DESC_AUTH_PERMISSION);

        goto error;
    }

    out = ov_json_object();
    val = ov_json_string(user);
    if (!ov_json_object_set(out, OV_KEY_USER, val)) goto error;

    val = ov_json_string(role);
    if (!ov_json_object_set(out, OV_KEY_ROLE, val)) goto error;

    val = ov_json_string(loop);
    if (!ov_json_object_set(out, OV_KEY_LOOP, val)) goto error;

    val = NULL;
    if (!ov_json_value_copy((void **)&val, state)) goto error;

    if (!ov_json_object_set(out, OV_KEY_STATE, val)) goto error;

    val = NULL;

    send_success_response(self, input, socket, &out);

    send_talking_loop_broadcast(
            self, socket, params, loop, state, client_id);

error:
    /* Do not close socket in case of broadcasting errors */
    ov_json_value_free(val);
    ov_json_value_free(out);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_event_sip_status(void *userdata,
                                    int socket,
                                    const ov_event_parameter *params,
                                    ov_json_value *input) {

    ov_json_value *val = NULL;
    ov_json_value *out = NULL;
    ov_json_value *data = NULL;

    UNUSED(params);

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) {

        send_error_response(
            self, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    bool status = ov_vocs_core_get_sip_status(self->config.core);

    out = ov_json_object();

    if (status) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }

    ov_json_object_set(out, OV_KEY_CONNECTED, val);

    send_success_response(self, input, socket, &out);

error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_calling(void *userdata,
                           int socket,
                           const ov_event_parameter *params,
                           ov_json_value *input) {

    UNUSED(params);

    ov_json_value *data = NULL;

    char *uuid_request = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            self, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    const char *loop = ov_json_string_get(
        (ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP)));
    const char *dest = ov_json_string_get(
        (ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DESTINATION)));
    const char *from = ov_json_string_get(
        (ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_FROM)));

    if (!uuid || !loop || !dest) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (!ov_vocs_db_sip_allow_callout(self->config.db, loop, role)) {

        send_error_response(
            self, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    uuid_request = ov_vocs_core_sip_create_call(self->config.core,
        loop, dest, from);

    if (!uuid_request) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);
        goto error;
    }

    if (!ov_event_async_set(
            self->async,
            uuid_request,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = self,
                                  .timedout.callback = async_timedout},
            self->config.limits.response_usec)) {
        goto error;
    }

    input = NULL;

error:
    /* Do not close socket in case of call errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    uuid_request = ov_data_pointer_free(uuid_request);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_hangup(void *userdata,
                          int socket,
                          const ov_event_parameter *params,
                          ov_json_value *input) {

    UNUSED(params);

    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            self, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    const char *call_id = ov_json_string_get(
        (ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_CALL)));
    const char *loop = ov_json_string_get(
        (ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP)));

    if (!uuid || !call_id) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (!ov_vocs_db_sip_allow_callend(self->config.db, loop, role)) {

        send_error_response(
            self, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    if (ov_vocs_core_sip_terminate_call(self->config.core, call_id)){

        send_success_response(self, input, socket, NULL);

    } else {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);
    }

error:
    /* Do not close socket in case of call errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_permit_call(void *userdata,
                               int socket,
                               const ov_event_parameter *params,
                               ov_json_value *input) {

    UNUSED(params);

    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            self, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    bool ok = true;

    const char *uuid = ov_event_api_get_uuid(input);
    ov_sip_permission permission = ov_sip_permission_from_json(
        ov_json_get(input, "/" OV_KEY_PARAMETER), &ok);

    if (!uuid || !ok) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (ov_vocs_core_sip_create_permission(self->config.core, permission)) {

        send_success_response(self, input, socket, NULL);

    } else {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);
    }

error:
    /* Do not close socket in case of call errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_revoke_call(void *userdata,
                               int socket,
                               const ov_event_parameter *params,
                               ov_json_value *input) {

    UNUSED(params);

    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            self, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    bool ok = true;

    const char *uuid = ov_event_api_get_uuid(input);
    ov_sip_permission permission = ov_sip_permission_from_json(
        ov_json_get(input, "/" OV_KEY_PARAMETER), &ok);

    if (!uuid || !ok) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (ov_vocs_core_sip_terminate_permission(self->config.core, permission)) {

        send_success_response(self, input, socket, NULL);

    } else {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);
    }

error:
    /* Do not close socket in case of call errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_call_function(ov_vocs_app *self,
                                 int socket,
                                 const ov_event_parameter *params,
                                 ov_json_value *input,
                                 bool (*function)(ov_vocs_core *core,
                                                  const char *uuid)) {

    UNUSED(params);

    ov_json_value *data = NULL;

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));
    const char *uuid = ov_event_api_get_uuid(input);

    if (!user || !role || !uuid) {

        send_error_response(
            self, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    if (!ov_event_async_set(
            self->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = self,
                                  .timedout.callback = async_timedout},
            self->config.limits.response_usec)) {

        goto error;
    }

    input = NULL;

    if (!function(self->config.core, uuid)) {

        ov_event_async_data adata = ov_event_async_unset(self->async, uuid);

        send_error_response(self,
                            adata.value,
                            adata.socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        adata.value = ov_json_value_free(adata.value);

        goto error;
    }

error:
    /* Do not close socket in case of call errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_list_calls(void *userdata,
                              int socket,
                              const ov_event_parameter *params,
                              ov_json_value *input) {

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    return client_call_function(
        self, socket, params, input, ov_vocs_core_sip_list_calls);
}

/*----------------------------------------------------------------------------*/

static bool client_list_call_permissions(void *userdata,
                                         int socket,
                                         const ov_event_parameter *params,
                                         ov_json_value *input) {
    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    return client_call_function(
        self, socket, params, input, ov_vocs_core_sip_list_call_permissions);
}

/*----------------------------------------------------------------------------*/

static bool client_list_sip_status(void *userdata,
                                   int socket,
                                   const ov_event_parameter *params,
                                   ov_json_value *input) {

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    return client_call_function(
        self, socket, params, input, ov_vocs_core_sip_list_sip_status);
}

/*----------------------------------------------------------------------------*/

static bool client_event_set_keyset_layout(void *userdata,
                                           int socket,
                                           const ov_event_parameter *params,
                                           ov_json_value *input) {

    /* checking input:

       {
           "event" : "set_keyset_layout",
           "parameter" :
           {
                "domain" : "<domainname>",
                "name"   : "<name>",
                "layout"   : {}
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) goto error;

    const char *domain = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DOMAIN));

    const char *name = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_NAME));

    const ov_json_value *layout =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LAYOUT);

    if (!domain || !name || !layout) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!ov_vocs_db_authorize_domain_admin(self->config.db, user, domain)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_set_keyset_layout(self->config.db, domain, name, layout)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_event_get_keyset_layout(void *userdata,
                                           int socket,
                                           const ov_event_parameter *params,
                                           ov_json_value *input) {

    /* checking input:

       {
           "event" : "get_keyset_layout",
           "parameter" :
           {
                "domain"   : "<domain_id>",
                "layout".  : "<layout_name>"
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) goto error;

    const char *domain = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DOMAIN));

    const char *layout = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LAYOUT));

    if (!domain || !layout) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    /* anyone can get the layout of some keyset */

    val = ov_vocs_db_get_keyset_layout(self->config.db, domain, layout);
    if (!val) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_LAYOUT, val);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    data = ov_json_value_free(data);
    input = ov_json_value_free(input);
    return false;
}


/*----------------------------------------------------------------------------*/

static bool client_event_set_user_data(void *userdata,
                                       int socket,
                                       const ov_event_parameter *params,
                                       ov_json_value *input) {

    /* checking input:

       {
           "event" : "set_user_data",
           "parameter" :
           {
                // data to set
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    ov_json_value *fdata = ov_event_api_get_parameter(input);
    if (!fdata) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!ov_vocs_db_set_user_data(self->config.db, user, fdata)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    val = NULL;
    ov_json_value_copy((void **)&val, fdata);
    ov_json_object_set(res, OV_KEY_DATA, val);

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            self->broadcasts, user, &parameter, out, OV_USER_BROADCAST))
        goto error;

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    data = ov_json_value_free(data);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_event_get_user_data(void *userdata,
                                       int socket,
                                       const ov_event_parameter *params,
                                       ov_json_value *input) {

    /* checking input:

       {
           "event" : "get_user_data",
           "parameter" :
           {
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    ov_json_value *fdata = ov_vocs_db_get_user_data(self->config.db, user);

    if (!fdata) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_DATA, fdata);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    data = ov_json_value_free(data);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_event_get_recording(void *userdata,
                                       int socket,
                                       const ov_event_parameter *params,
                                       ov_json_value *input) {

    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    UNUSED(params);

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            self, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    ov_db_recordings_get_params db_params = {0};

    db_params.loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    db_params.user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));
    db_params.from_epoch_secs = ov_json_number_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_FROM));
    db_params.until_epoch_secs = ov_json_number_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TO));

    val = ov_vocs_core_get_recording(self->config.core, db_params);

    if (!val) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

    } else if (OV_DB_RECORDINGS_RESULT_TOO_BIG == val) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            "Search returned too many results - please confine "
                            "your search parameters");

    } else {
        send_success_response(self, input, socket, &val);
    }

error:
    ov_json_value_free(val);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_event_register(void *userdata,
                                  int socket,
                                  const ov_event_parameter *params,
                                  ov_json_value *input) {

    /* checking input:

       {
           "event" : "register",
           "parameter" :
           {
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !socket || !params || !input) goto error;

    if (!ov_broadcast_registry_set(self->broadcasts,
                                   OV_BROADCAST_KEY_SYSTEM_BROADCAST,
                                   socket,
                                   OV_SYSTEM_BROADCAST)) {
        goto error;
    }

    out = ov_event_api_create_success_response(input);

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #API #MANAGEMENT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool get_mixer_count(void *userdata,
                                int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *res = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!user){

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

    } else {

        ov_mc_backend_registry_count count = ov_vocs_core_count_mixers(self->config.core);

        out = ov_event_api_create_success_response(input);
        res = ov_event_api_get_response(out);
    
        val = ov_json_number(count.mixers);
        if (!ov_json_object_set(res, OV_KEY_MIXER, val)) goto error;
    
        val = ov_json_number(count.used);
        if (!ov_json_object_set(res, OV_KEY_USED, val)) goto error;
    }

    

    env_send(self, socket, out);

    ov_json_value_free(out);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

struct mixer_data_mgmt {

    ov_vocs_app *vocs;
    const char *user;
    const char *uuid;

};

/*----------------------------------------------------------------------------*/

static bool call_mixer_state_mgmt(const void *key, void *val, void *data){

    if (!key) return true;

    struct mixer_data_mgmt *container = (struct mixer_data_mgmt*) data;

    const char *user = ov_json_string_get(ov_json_get(
        ov_json_value_cast(val), "/"OV_KEY_USER));

    if (!user) return true;

    if (container->user){

        if (0 != ov_string_compare(user, container->user))
            return true;

    }

    const char *session = ov_json_string_get(ov_json_get(
        ov_json_value_cast(val), "/"OV_KEY_SESSION));

    if (!session) return true;

    if (!ov_vocs_core_get_mixer_state(
        container->vocs->config.core,
        container->uuid,
        session)) goto error;

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool get_mixer_state(void *userdata,
                                int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    ov_json_value *data = NULL;
    ov_json_value *out = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!user){

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        env_send(self, socket, out);
        goto error;

    } 

    const char *uuid = ov_event_api_get_uuid(input);

    user = ov_json_string_get(
        ov_json_get(input, "/"OV_KEY_PARAMETER"/"OV_KEY_USER));

    struct mixer_data_mgmt container = (struct mixer_data_mgmt){
        .vocs = self,
        .user = user,
        .uuid = uuid
    };

    bool result = ov_socket_json_for_each(self->connections, &container, 
        call_mixer_state_mgmt);

    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    out = ov_json_value_free(out);
    return result;
error:
    ov_json_value_free(input);
    ov_json_value_free(data);
    out = ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool check_if_admin_is_logged_in(ov_vocs_app *self, int socket){

    ov_json_value *data = NULL;

    data = ov_socket_json_get(self->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!user) goto error;

    ov_json_value *domains = ov_vocs_db_get_admin_domains(self->config.db, user);
    if (!domains || ov_json_object_is_empty(domains)) goto error;

    data = ov_json_value_free(data);
    domains = ov_json_value_free(domains);
    return true;
error:
    data = ov_json_value_free(data);
    domains = ov_json_value_free(domains);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool get_connection_state(void *userdata,
                                int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *res = NULL;
    
    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !params || !input) goto error;

    if (!check_if_admin_is_logged_in(self, socket)){

        out = ov_event_api_create_error_response(input, 
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    val = ov_json_object();

    if (!ov_socket_json_for_each_set_data(self->connections, val)) goto error;

    out = ov_event_api_create_success_response(input);
    res = ov_event_api_get_response(out);

    if (!ov_json_object_set(res, OV_KEY_CONNECTIONS, val)) goto error;

response:
    env_send(self, socket, out);

    ov_json_value_free(out);
    ov_json_value_free(input);
    return true;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static void cb_start_recording(void *userdata, int socket, const char *uuid, const char *loop,
    ov_result result){

    ov_json_value *out = NULL;
    ov_json_value *res = NULL;
    ov_json_value *val = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !loop || !uuid) goto error;

    out = ov_event_api_message_create("start_recording", uuid, 0);
    res = ov_json_object();
    ov_json_object_set(out, OV_KEY_RESPONSE, res);
    val = ov_json_string(loop);
    ov_json_object_set(res, OV_KEY_LOOP, val);

    switch (result.error_code){

        case OV_ERROR_NOERROR:
            break;

        default:
            ov_event_api_add_error(out, result.error_code, result.message);
            break;
    }

    env_send(self, socket, out);

error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static bool start_recording(void *userdata,
                                int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    UNUSED(params);

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input) goto error;

    if (!check_if_admin_is_logged_in(self, socket)){

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_AUTH,
                            OV_ERROR_DESC_AUTH);

        input = ov_json_value_free(input);
        goto done;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    if (!uuid) goto error;

    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));

    if (!uuid || !loop) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        input = ov_json_value_free(input);
        goto done;
    }

    ov_vocs_core_start_recording(self->config.core, 
        uuid, loop,
        self, socket, cb_start_recording);
done:
    input = ov_json_value_free(input);
    return true;

error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static void cb_stop_recording(void *userdata, int socket, const char *uuid, const char *loop,
    ov_result result){

    ov_json_value *out = NULL;
    ov_json_value *res = NULL;
    ov_json_value *val = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !loop || !uuid) goto error;

    out = ov_event_api_message_create("stop_recording", uuid, 0);
    res = ov_json_object();
    ov_json_object_set(out, OV_KEY_RESPONSE, res);
    val = ov_json_string(loop);
    ov_json_object_set(res, OV_KEY_LOOP, val);

    switch (result.error_code){

        case OV_ERROR_NOERROR:
            break;

        default:
            ov_event_api_add_error(out, result.error_code, result.message);
            break;
    }

    env_send(self, socket, out);

error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static bool stop_recording(void *userdata,
                                int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    UNUSED(params);

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input) goto error;

    if (!check_if_admin_is_logged_in(self, socket)){

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_AUTH,
                            OV_ERROR_DESC_AUTH);

        input = ov_json_value_free(input);
        goto done;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    if (!uuid) goto error;

    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));

    if (!uuid || !loop) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        input = ov_json_value_free(input);
        goto done;
    }

    ov_vocs_core_stop_recording(self->config.core, 
        uuid, loop,
        self, socket, cb_stop_recording);
done:
    input = ov_json_value_free(input);
    return true;

error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool get_recorded_loops(void *userdata,
                                int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    UNUSED(params);

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input) goto error;

    if (!check_if_admin_is_logged_in(self, socket)){

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_AUTH,
                            OV_ERROR_DESC_AUTH);

        input = ov_json_value_free(input);
        goto done;
    }

    val = ov_vocs_core_get_recorded_loops(self->config.core);
    if (!val) val = ov_json_object();

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    if (!ov_json_object_set(res, OV_KEY_RESULT, val)) goto error;

    val = NULL;

    env_send(self, socket, out);

done:
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return true;

error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool get_all_loops(void *userdata,
                                int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    UNUSED(params);

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input) goto error;

    if (!check_if_admin_is_logged_in(self, socket)){

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_AUTH,
                            OV_ERROR_DESC_AUTH);

        input = ov_json_value_free(input);
        goto done;
    }

    val = ov_vocs_db_get_all_loops_incl_domain(self->config.db);
    if (!val){
        val = ov_json_object();
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    if (!ov_json_object_set(res, OV_KEY_RESULT, val)) goto error;

    val = NULL;

    env_send(self, socket, out);

done:
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool broadcast(void *userdata,
                                int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    UNUSED(params);

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    if (!self || !input) goto error;

    if (!check_if_admin_is_logged_in(self, socket)){

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_AUTH,
                            OV_ERROR_DESC_AUTH);

        input = ov_json_value_free(input);
        goto done;
    }

    if (!ov_event_api_set_type(input, OV_BROADCAST_KEY_SYSTEM_BROADCAST))
        goto error;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            self->broadcasts, OV_BROADCAST_KEY_SYSTEM_BROADCAST, &parameter, input, 
            OV_SYSTEM_BROADCAST)) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);
    } else {

        send_success_response(self, input, socket, NULL);
        
    }

done:
    input = ov_json_value_free(input);
    return true;
error:
    input = ov_json_value_free(input);
    return false;
}


/*----------------------------------------------------------------------------*/

static bool cb_event_update_password(void *userdata,
                                int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);
    if (!data) goto error;

    const char *conn_user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    const char *user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));
    const char *pass = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD));

    if (!user || !pass) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (self->config.ldap) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH_LDAP_USED, OV_ERROR_DESC_AUTH_LDAP_USED);

        goto response;
    }

    if (0 == ov_string_compare(user, conn_user)) {

        /* each user is allowed to update it's own password */

        if (!ov_vocs_db_set_password(self->config.db, user, pass)) {

            out = ov_event_api_create_error_response(
                input,
                OV_ERROR_CODE_PROCESSING_ERROR,
                OV_ERROR_DESC_PROCESSING_ERROR);

            goto response;
        }

        out = ov_event_api_create_success_response(input);
        goto response;

    } else {

        /* password reset from admin */

        ov_vocs_db_parent parent =
            ov_vocs_db_get_parent(self->config.db, OV_VOCS_DB_USER, user);

        switch (parent.scope) {

            case OV_VOCS_DB_SCOPE_DOMAIN:

                if (!ov_vocs_db_authorize_domain_admin(
                        self->config.db, conn_user, parent.id)) {

                    out = ov_event_api_create_error_response(
                        input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

                    ov_vocs_db_parent_clear(&parent);

                    goto response;
                }

                break;

            case OV_VOCS_DB_SCOPE_PROJECT:

                if (!ov_vocs_db_authorize_project_admin(
                        self->config.db, conn_user, parent.id)) {

                    out = ov_event_api_create_error_response(
                        input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

                    ov_vocs_db_parent_clear(&parent);

                    goto response;
                }

                break;
        }

        ov_vocs_db_parent_clear(&parent);

        /* admin of the project or the domain of the user,
         * update allowed */

        if (!ov_vocs_db_set_password(self->config.db, user, pass)) {

            out = ov_event_api_create_error_response(
                input,
                OV_ERROR_CODE_PROCESSING_ERROR,
                OV_ERROR_DESC_PROCESSING_ERROR);

            goto response;
        }

        out = ov_event_api_create_success_response(input);
        goto response;
    }

    ov_vocs_db_persistance_broadcast(self->config.persistance, input);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return false;
}


/*----------------------------------------------------------------------------*/

static bool cb_event_get_admin_domains(void *userdata,
                                       const int socket,
                                       const ov_event_parameter *params,
                                       ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    if (!check_if_admin_is_logged_in(self, socket)){

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_AUTH,
                            OV_ERROR_DESC_AUTH);

        input = ov_json_value_free(input);
        goto done;
    }

    const char *user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    /* all users are allowed to send the request */

    val = ov_vocs_db_get_admin_domains(self->config.db, user);
    if (!val) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_DOMAINS, val);

response:

    ov_event_io_send(params, socket, out);
done:
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return true;
error:
    input = ov_json_value_free(input);
    out = ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_get_admin_projects(void *userdata,
                                        const int socket,
                                        const ov_event_parameter *params,
                                        ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    if (!check_if_admin_is_logged_in(self, socket)){

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_AUTH,
                            OV_ERROR_DESC_AUTH);

        input = ov_json_value_free(input);
        goto done;
    }

    const char *user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    /* all users are allowed to send the request */

    val = ov_vocs_db_get_admin_projects(self->config.db, user);
    if (!val) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_PROJECTS, val);

response:

    ov_event_io_send(params, socket, out);

done:
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return true;
error:
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return false;
}


/*----------------------------------------------------------------------------*/

static bool cb_event_check_id_exists(void *userdata,
                                     const int socket,
                                     const ov_event_parameter *params,
                                     ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *scope = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_SCOPE));

    if (!id) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    /* all users are allowed to send the request */

    if (scope) {

        /* check for unique entity id */

        ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(scope);

        if (ov_vocs_db_check_entity_id_exists(self->config.db, entity, id)) {
            val = ov_json_true();
        } else {
            val = ov_json_false();
        }

    } else {

        /* check for unique ID */

        if (ov_vocs_db_check_id_exists(self->config.db, id)) {
            val = ov_json_true();
        } else {
            val = ov_json_false();
        }
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_RESULT, val);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return true;
error:
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool authorize_connection_for_id(ov_vocs_app *self,
                                        int socket,
                                        ov_vocs_db_entity entity,
                                        const char *id) {

    ov_vocs_db_parent parent = {0};
    ov_json_value *data = NULL;

    if (!self || !socket || !id) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) goto error;

    parent = ov_vocs_db_get_parent(self->config.db, entity, id);

    if (NULL == parent.id) goto error;

    switch (parent.scope) {

        case OV_VOCS_DB_SCOPE_DOMAIN:

            if (!ov_vocs_db_authorize_domain_admin(
                    self->config.db, user, parent.id)) {
                goto error;
            }

            break;

        case OV_VOCS_DB_SCOPE_PROJECT:

            if (!ov_vocs_db_authorize_project_admin(
                    self->config.db, user, parent.id)) {
                goto error;
            }

            break;
    }

    ov_vocs_db_parent_clear(&parent);
    data = ov_json_value_free(data);
    return true;
error:
    ov_vocs_db_parent_clear(&parent);
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_delete(void *userdata,
                            const int socket,
                            const ov_event_parameter *params,
                            ov_json_value *input) {

    /* checking input:

       {
           "event" : "delete",
           "parameter" :
           {
               "type" : "<domain | project | user | role | loop >",
               "id" : "<id to get>",
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    if (!id || !type) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!authorize_connection_for_id(self, socket, entity, id)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_delete_entity(self->config.db, entity, id)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_vocs_db_persistance_broadcast(self->config.persistance, input);
response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_create(void *userdata,
                            const int socket,
                            const ov_event_parameter *params,
                            ov_json_value *input) {

    /* checking input:

       {
           "event" : "create",
           "parameter" :
           {
               "type" : "<domain | project | user | role | loop >",
               "id" : "<id to get>",
               "scope" :
               {
                   "type": "<domain | project>",
                   "id" : "<parent scope id>"
               }
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    const char *scope_id = ov_json_string_get(ov_json_get(
        input, "/" OV_KEY_PARAMETER "/" OV_KEY_SCOPE "/" OV_KEY_ID));

    const char *scope_type = ov_json_string_get(ov_json_get(
        input, "/" OV_KEY_PARAMETER "/" OV_KEY_SCOPE "/" OV_KEY_TYPE));

    if (!id || !type || !scope_id || !scope_type) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_vocs_db_scope db_scope = OV_VOCS_DB_SCOPE_PROJECT;

    if (0 == strcmp(scope_type, OV_KEY_DOMAIN)) {

        db_scope = OV_VOCS_DB_SCOPE_DOMAIN;

        if (!ov_vocs_db_authorize_domain_admin(
                self->config.db, user, scope_id)) {

            out = ov_event_api_create_error_response(
                input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

            goto response;
        }

    } else if (!ov_vocs_db_authorize_project_admin(
                   self->config.db, user, scope_id)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_create_entity(
            self->config.db, entity, id, db_scope, scope_id)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    ov_vocs_db_persistance_broadcast(self->config.persistance, input);
    out = ov_event_api_create_success_response(input);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_get_key(void *userdata,
                             const int socket,
                             const ov_event_parameter *params,
                             ov_json_value *input) {

    /* checking input:

       {
           "event" : "get_key",
           "parameter" :
           {
               "type"     : "<domain | project | user | role | loop >",
               "id" : "<id to get>",
               "key" : "<key to get>"
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;

    }

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *key = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_KEY));

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    if (!id || !type || !key) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    /* all users are allowed to get anything */

    val = ov_vocs_db_get_entity_key(self->config.db, entity, id, key);
    if (!val) {
        val = ov_json_null();
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    if (!ov_json_object_set(res, OV_KEY_RESULT, val)) goto error;

    val = ov_json_string(type);
    if (!ov_json_object_set(res, OV_KEY_TYPE, val)) goto error;

    val = ov_json_string(id);
    if (!ov_json_object_set(res, OV_KEY_ID, val)) goto error;

    val = ov_json_string(key);
    if (!ov_json_object_set(res, OV_KEY_KEY, val)) goto error;

    val = NULL;

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return false;
}


/*----------------------------------------------------------------------------*/

static bool cb_event_update_key(void *userdata,
                                const int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    /* checking input:

       {
           "event" : "update_key",
           "parameter" :
           {
               "type"     : "<domain | project | user | role | loop >",
               "id" : "<id to get>",
               "key" : "<key to get>"
               "data"  : { <data to update> }
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *key = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_KEY));

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    const ov_json_value *data =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DATA);

    if (!id || !type || !key || !data) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!authorize_connection_for_id(self, socket, entity, id)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_update_entity_key(self->config.db, entity, id, key, data)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    ov_vocs_db_persistance_broadcast(self->config.persistance, input);
    out = ov_event_api_create_success_response(input);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return false;
}


/*----------------------------------------------------------------------------*/

static bool cb_event_delete_key(void *userdata,
                                const int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    /* checking input:

       {
           "event" : "delete_key",
           "parameter" :
           {
               "type"     : "<domain | project | user | role | loop >",
               "id" : "<id to get>",
               "key" : "<key to get>"
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *key = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_KEY));

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    if (!id || !type || !key) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!authorize_connection_for_id(self, socket, entity, id)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_delete_entity_key(self->config.db, entity, id, key)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    ov_vocs_db_persistance_broadcast(self->config.persistance, input);
    out = ov_event_api_create_success_response(input);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_verify(void *userdata,
                            const int socket,
                            const ov_event_parameter *params,
                            ov_json_value *input) {

    /* checking input:

       {
           "event" : "verify",
           "parameter" :
           {
               "type"     : "<domain | project | user | role | loop >",
               "id" : "<id to get>",
               "data" : "<object to verify>"
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *errors = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const ov_json_value *data =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DATA);

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    if (!id || !type || !data) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!authorize_connection_for_id(self, socket, entity, id)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_verify_entity_item(
            self->config.db, entity, id, data, &errors)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        if (errors) {

            ov_json_value *res = ov_event_api_get_response(out);
            ov_json_object_set(res, OV_KEY_ERRORS, errors);
        }

        goto response;
    }

    OV_ASSERT(errors == NULL);
    ov_vocs_db_persistance_broadcast(self->config.persistance, input);
    out = ov_event_api_create_success_response(input);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return false;
}


/*----------------------------------------------------------------------------*/

static bool cb_event_update(void *userdata,
                            const int socket,
                            const ov_event_parameter *params,
                            ov_json_value *input) {

    /* checking input:

       {
           "event" : "update",
           "parameter" :
           {
               "type" : "<domain | project | user | role | loop >",
               "id"   : "<id to get>",
               "data" : "<data to update>"
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *errors = NULL;
    ov_json_value *udata = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    udata = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(udata, "/"OV_KEY_USER));

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const ov_json_value *data =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DATA);

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    if (!id || !type || !data) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (0 == strcmp(type, OV_KEY_DOMAIN)) {

        if (!ov_vocs_db_authorize_domain_admin(self->config.db, user, id)) {

            out = ov_event_api_create_error_response(
                input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

            goto response;
        }

    } else if (0 == strcmp(type, OV_KEY_PROJECT)) {

        if (!ov_vocs_db_authorize_project_admin(self->config.db, user, id)) {

            out = ov_event_api_create_error_response(
                input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

            goto response;
        }

    } else {

        if (!authorize_connection_for_id(self, socket, entity, id)) {

            out = ov_event_api_create_error_response(
                input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

            goto response;
        }
    }

    if (!ov_vocs_db_update_entity_item(
            self->config.db, entity, id, data, &errors)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        if (errors) {

            ov_json_value *res = ov_event_api_get_response(out);
            ov_json_object_set(res, OV_KEY_ERRORS, errors);
        }

        goto response;
    }

    OV_ASSERT(errors == NULL);
    ov_vocs_db_persistance_broadcast(self->config.persistance, input);
    out = ov_event_api_create_success_response(input);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    udata = ov_json_value_free(udata);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    udata = ov_json_value_free(udata);
    return false;
}


/*----------------------------------------------------------------------------*/

static bool cb_event_load(void *userdata,
                          const int socket,
                          const ov_event_parameter *params,
                          ov_json_value *input) {

    /* checking input:

       {
           "event" : "load",
           "parameter" : {}
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    /* check if the user is some domain admin */
    val = ov_vocs_db_get_admin_domains(self->config.db, user);
    if (!val || (0 == ov_json_array_count(val))) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        val = ov_json_value_free(val);

        goto response;
    }
    val = ov_json_value_free(val);

    if (!ov_vocs_db_persistance_load(self->config.persistance)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    ov_vocs_db_persistance_broadcast(self->config.persistance, input);
    out = ov_event_api_create_success_response(input);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_save(void *userdata,
                          const int socket,
                          const ov_event_parameter *params,
                          ov_json_value *input) {

    /* checking input:

       {
           "event" : "save",
           "parameter" : {}
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    /* check if the user is some domain admin */
    val = ov_vocs_db_get_admin_domains(self->config.db, user);
    if (!val || (0 == ov_json_array_count(val))) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        val = ov_json_value_free(val);

        goto response;
    }
    val = ov_json_value_free(val);

    if (!ov_vocs_db_persistance_save(self->config.persistance)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    ov_vocs_db_persistance_broadcast(self->config.persistance, input);
    out = ov_event_api_create_success_response(input);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return false;
}


/*----------------------------------------------------------------------------*/

static bool cb_event_set_layout(void *userdata,
                                const int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    /* checking input:

       {
           "event" : "set_layout",
           "parameter" :
           {
                "role"   : "<role_id>"
                "layout" : <layout to set>
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;


    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ROLE));

    const ov_json_value *data =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LAYOUT);

    if (!id || !data) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!authorize_connection_for_id(self, socket, OV_VOCS_DB_ROLE, id)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_set_layout(self->config.db, id, data)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    ov_vocs_db_persistance_broadcast(self->config.persistance, input);
    out = ov_event_api_create_success_response(input);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_get_layout(void *userdata,
                                const int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    /* checking input:

       {
           "event" : "get_layout",
           "parameter" :
           {
                "role"   : "<role_id>"
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!user){

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ROLE));

    if (!id) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    /* any user can get the layout of some role */

    val = ov_vocs_db_get_layout(self->config.db, id);
    if (!val) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_LAYOUT, val);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return false;
}


/*----------------------------------------------------------------------------*/

static bool cb_event_add_domain_admin(void *userdata,
                                      const int socket,
                                      const ov_event_parameter *params,
                                      ov_json_value *input) {

    /* checking input:

       {
           "event" : "add_domain_admin",
           "parameter" :
           {
                "domain" : "<domain_id>"
                "user"   : <user id to add>
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!user){

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *domain = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DOMAIN));

    const char *admin = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!domain || !admin) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    /* MUST be an admin of the domain */
    if (!ov_vocs_db_authorize_domain_admin(self->config.db, user, domain)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_add_domain_admin(self->config.db, domain, admin)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    ov_vocs_db_persistance_broadcast(self->config.persistance, input);
    out = ov_event_api_create_success_response(input);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_add_project_admin(void *userdata,
                                       const int socket,
                                       const ov_event_parameter *params,
                                       ov_json_value *input) {

    /* checking input:

       {
           "event" : "add_project_admin",
           "parameter" :
           {
                "project" : "<project_id>"
                "user"    : <user id to add>
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!user){

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *project = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_PROJECT));

    const char *admin = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!project || !admin) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    /* MUST be an admin of the project */
    if (!ov_vocs_db_authorize_project_admin(self->config.db, user, project)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_add_project_admin(self->config.db, project, admin)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    ov_vocs_db_persistance_broadcast(self->config.persistance, input);
    out = ov_event_api_create_success_response(input);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return false;
}


/*----------------------------------------------------------------------------*/

static bool cb_event_ldap_import(void *userdata,
                                 const int socket,
                                 const ov_event_parameter *params,
                                 ov_json_value *input) {

    /* checking input:

       {
           "event" : "ldap_import",
           "parameter" :
           {
                "host"     : "<ldap_host>",
                "base"     : <ldap_base_dn>,
                "domain"   : "<domain id>",
                "user"     : <ldap_user>,
                "password" : <ldap_password>
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!user){

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *host = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_HOST));

    const char *base = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_BASE));

    const char *domain = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DOMAIN));

    const char *ldap_user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    const char *ldap_pass = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD));

    if (!host || !base || !domain || !ldap_user || !ldap_pass) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    /* MUST be an admin of the domain */
    if (!ov_vocs_db_authorize_domain_admin(self->config.db, user, domain)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_persistance_ldap_import(self->config.persistance,
                                            host,
                                            base,
                                            ldap_user,
                                            ldap_pass,
                                            domain)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    ov_vocs_db_persistance_broadcast(self->config.persistance, input);
    out = ov_event_api_create_success_response(input);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return false;
}


/*----------------------------------------------------------------------------*/

static bool cb_event_set_keyset_layout(void *userdata,
                                       const int socket,
                                       const ov_event_parameter *params,
                                       ov_json_value *input) {

    /* checking input:

       {
           "event" : "set_keyset_layout",
           "parameter" :
           {
                "domain" : "<domainname>",
                "name"   : "<name>",
                "layout"   : {}
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!user){

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *domain = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DOMAIN));

    const char *name = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_NAME));

    const ov_json_value *layout =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LAYOUT);

    if (!domain || !name || !layout) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!ov_vocs_db_authorize_domain_admin(self->config.db, user, domain)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_set_keyset_layout(self->config.db, domain, name, layout)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    ov_vocs_db_persistance_broadcast(self->config.persistance, input);
    out = ov_event_api_create_success_response(input);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_get_keyset_layout(void *userdata,
                                       const int socket,
                                       const ov_event_parameter *params,
                                       ov_json_value *input) {

    /* checking input:

       {
           "event" : "get_keyset_layout",
           "parameter" :
           {
                "domain"   : "<domain_id>",
                "layout".  : "<layout_name>"
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!user){

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *domain = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DOMAIN));

    const char *layout = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LAYOUT));

    if (!domain || !layout) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    /* anyone can get the layout of some keyset */

    val = ov_vocs_db_get_keyset_layout(self->config.db, domain, layout);
    if (!val) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_LAYOUT, val);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return false;
}


/*----------------------------------------------------------------------------*/

static bool cb_event_set_user_data(void *userdata,
                                   const int socket,
                                   const ov_event_parameter *params,
                                   ov_json_value *input) {

    /* checking input:

       {
           "event" : "set_user_data",
           "parameter" :
           {
                // data to set
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!user){

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    ov_json_value *udata = ov_event_api_get_parameter(input);
    if (!udata) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!ov_vocs_db_set_user_data(self->config.db, user, udata)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_LAYOUT, val);

    ov_vocs_db_persistance_broadcast(self->config.persistance, input);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_get_user_data(void *userdata,
                                   const int socket,
                                   const ov_event_parameter *params,
                                   ov_json_value *input) {

    /* checking input:

       {
           "event" : "get_user_data",
           "parameter" :
           {
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!user){

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    ov_json_value *udata = ov_vocs_db_get_user_data(self->config.db, user);

    if (!udata) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_DATA, udata);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_get_highest_port(void *userdata,
                                   const int socket,
                                   const ov_event_parameter *params,
                                   ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!user){

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    uint32_t port = ov_vocs_db_get_highest_port(self->config.db);

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_PORT, ov_json_number(port));

response:
    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool init_config(ov_vocs_app_config *config){

    if (!config) goto error;

    if (!config->loop){

        ov_log_error("No eventloop given - abort.");
        goto error;

    }

    if (!config->core){

        ov_log_error("No CORE given - abort.");
        goto error;

    }

    if (!config->db || !config->persistance){

        ov_log_error("No DB given - abort.");
        goto error;
        
    }

    if (0 == config->sessions.path[0])
        strncpy(config->sessions.path, "/tmp/openvocs/sessions", PATH_MAX);

    if (0 == config->limits.response_usec)
        config->limits.response_usec = 5000000;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool register_events(ov_vocs_app *self){

    if (!self) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_KEY_LOGIN,
                                  client_login)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_EVENT_API_AUTHENTICATE,
                                  client_login)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_KEY_LOGOUT,
                                  client_logout)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_EVENT_API_UPDATE_LOGIN,
                                  update_client_login)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_EVENT_API_MEDIA,
                                  client_media)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_ICE_STRING_CANDIDATE,
                                  client_candidate)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_ICE_STRING_END_OF_CANDIDATES,
                                  client_end_of_candidates)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_EVENT_API_AUTHORISE,
                                  client_authorize)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_EVENT_API_AUTHORIZE,
                                  client_authorize)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_EVENT_API_GET,
                                  client_get)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_EVENT_API_USER_ROLES,
                                  client_user_roles)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_EVENT_API_ROLE_LOOPS,
                                  client_role_loops)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_EVENT_API_SWITCH_LOOP_STATE,
                                  client_switch_loop_state)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_EVENT_API_SWITCH_LOOP_VOLUME,
                                  client_switch_loop_volume)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_EVENT_API_TALKING,
                                  client_talking)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_KEY_CALL,
                                  client_calling)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_KEY_HANGUP,
                                  client_hangup)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_KEY_PERMIT_CALL,
                                  client_permit_call)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_KEY_REVOKE_CALL,
                                  client_revoke_call)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_KEY_LIST_CALLS,
                                  client_list_calls)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_KEY_LIST_CALL_PERMISSIONS,
                                  client_list_call_permissions)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_KEY_LIST_SIP_STATUS,
                                  client_list_sip_status)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_SET_KEYSET_LAYOUT,
                                  client_event_set_keyset_layout)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_GET_KEYSET_LAYOUT,
                                  client_event_get_keyset_layout)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_SET_USER_DATA,
                                  client_event_set_user_data)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_GET_USER_DATA,
                                  client_event_get_user_data)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_KEY_GET_RECORDING,
                                  client_event_get_recording)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_KEY_SIP,
                                  client_event_sip_status)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_KEY_REGISTER,
                                  client_event_register)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_EVENT_API_STATE_MIXER,
                                  get_mixer_count)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  "get_mixer_state",
                                  get_mixer_state)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_EVENT_API_STATE_CONNECTIONS,
                                  get_connection_state)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_EVENT_START_RECORD,
                                  start_recording)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_EVENT_STOP_RECORD,
                                  stop_recording)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  "get_recorded_loops",
                                  get_recorded_loops)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  "get_all_loops",
                                  get_all_loops)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_KEY_BROADCAST,
                                  broadcast)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_UPDATE_PASSWORD,
                                  cb_event_update_password)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_ADMIN_DOMAINS,
                                  cb_event_get_admin_domains)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_ADMIN_PROJECTS,
                                  cb_event_get_admin_projects)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_ID_EXISTS,
                                  cb_event_check_id_exists)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_DELETE,
                                  cb_event_delete)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_CREATE,
                                  cb_event_create)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_GET_KEY,
                                  cb_event_get_key)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_UPDATE_KEY,
                                  cb_event_update_key)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_DELETE_KEY,
                                  cb_event_delete_key)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_VERIFY,
                                  cb_event_verify)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_UPDATE,
                                  cb_event_update)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_LOAD,
                                  cb_event_load)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_SAVE,
                                  cb_event_save)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_GET_LAYOUT,
                                  cb_event_get_layout)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_SET_LAYOUT,
                                  cb_event_set_layout)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_LOAD,
                                  cb_event_load)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_ADD_DOMAIN_ADMIN,
                                  cb_event_add_domain_admin)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_ADD_PROJECT_ADMIN,
                                  cb_event_add_project_admin)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_LDAP_IMPORT,
                                  cb_event_ldap_import)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_SET_KEYSET_LAYOUT,
                                  cb_event_set_keyset_layout)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_GET_KEYSET_LAYOUT,
                                  cb_event_get_keyset_layout)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_SET_USER_DATA,
                                  cb_event_set_user_data)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_GET_USER_DATA,
                                  cb_event_get_user_data)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  "get_highest_port",
                                  cb_event_get_highest_port)) goto error;


    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_vocs_app *ov_vocs_app_create(ov_vocs_app_config config){

    ov_vocs_app *self = NULL;

    if (!init_config(&config)) goto error;

    self = calloc(1, sizeof(ov_vocs_app));
    if (!self) goto error;

    self->magic_byte = OV_VOCS_APP_MAGIC_BYTES;
    self->config = config;

    self->engine = ov_event_engine_create();    
    if (!register_events(self)) goto error;

    self->connections =
        ov_socket_json_create((ov_socket_json_config){.loop = config.loop});
    if (!self->connections) goto error;

    self->async = ov_event_async_store_create(
        (ov_event_async_store_config){.loop = config.loop});
    if (!self->async) goto error;

    ov_event_session_config event_config =
        (ov_event_session_config){.loop = config.loop};

    strncpy(event_config.path, config.sessions.path, PATH_MAX);

    self->user_sessions = ov_event_session_create(event_config);
    if (!self->user_sessions) goto error;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = ov_vocs_loop_free_void;
    self->loops = ov_dict_create(d_config);
    if (!self->loops) goto error;

    self->broadcasts = ov_broadcast_registry_create((ov_event_broadcast_config){
        .max_sockets = ov_socket_get_max_supported_runtime_sockets(0)});

    if (!self->broadcasts) goto error;

    if (!register_events(self)) goto error;

    return self;

error:
    ov_vocs_app_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_app *ov_vocs_app_free(ov_vocs_app *self){

    if (!ov_vocs_app_cast(self)) return self;

    self->engine = ov_event_engine_free(self->engine);
    self->user_sessions = ov_event_session_free(self->user_sessions);
    self->connections = ov_socket_json_free(self->connections);
    self->async = ov_event_async_store_free(self->async);
    self->loops = ov_dict_free(self->loops);
    self->broadcasts = ov_broadcast_registry_free(self->broadcasts);

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_app *ov_vocs_app_cast(const void *self){

    if (!self) goto error;

    if (*(uint16_t *)self == OV_VOCS_APP_MAGIC_BYTES) 
        return (ov_vocs_app *)self;
error:
    return NULL;

}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_api_process(ov_vocs_app *self, 
                             int socket,
                             const ov_event_parameter *params, 
                             ov_json_value *input){

    return ov_event_engine_push(self->engine, self, socket, *params, input);

}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_send_media_ready(ov_vocs_app *self, 
                                  const char *uuid,
                                  int socket, 
                                  const char *session_id){

    ov_json_value *data = NULL;

    if (!self || !uuid || !session_id) goto error;

    data = ov_socket_json_get(self->connections, socket);
    ov_json_object_set(data, OV_KEY_MEDIA_READY, ov_json_true());

    if (ov_json_is_true(ov_json_get(data, "/" OV_KEY_ICE))) {

        ov_json_value *out =
            ov_event_api_message_create(OV_KEY_MEDIA_READY, NULL, 0);

        ov_event_api_set_type(out, OV_KEY_UNICAST);

        if (!ov_event_api_set_parameter(out)) goto error;

        env_send(self, socket, out);
        out = ov_json_value_free(out);
    }

    ov_socket_json_set(self->connections, socket, &data);

    data = ov_json_value_free(data);
    return true;
error:
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool set_loop_state_in_data(ov_json_value *data,
                                   const char *loop,
                                   ov_vocs_permission permission) {

    ov_json_value *loops = ov_json_object_get(data, OV_KEY_LOOPS);
    if (!loops) {
        loops = ov_json_object();
        ov_json_object_set(data, OV_KEY_LOOPS, loops);
    }

    ov_json_object_set(
        loops, loop, ov_json_string(ov_vocs_permission_to_string(permission)));
    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_join_loop(ov_vocs_app *self, 
                           const char *uuid,
                           const char *session_id,
                           const char *loopname,
                           uint64_t error_code,
                           const char *error_desc){

    ov_json_value *data = NULL;
    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!self || !loopname || !uuid || !session_id) goto error;

    ov_event_async_data adata = ov_event_async_unset(self->async, uuid);
    if (!adata.value) goto error;

    data = ov_socket_json_get(self->connections, adata.socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));
    const char *client =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_CLIENT));

    switch (error_code) {

        case OV_ERROR_NOERROR:
            break;

        default:

            send_error_response(self, adata.value, adata.socket, error_code, error_desc);

            goto error;
            break;
    }

    const char *state = ov_json_string_get(
        ov_json_get(adata.value, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE));

    ov_vocs_permission request = ov_vocs_permission_from_string(state);
    ov_vocs_permission current = OV_VOCS_RECV;

    if (!set_loop_state_in_data(data, loopname, current)) goto error;
    if (!ov_vocs_db_set_state(self->config.db, user, role, loopname, current))
        goto error;

    ov_vocs_loop *loop = ov_dict_get(self->loops, loopname);
    if (!loop) {

        loop = ov_vocs_loop_create(loopname);
        char *name = ov_string_dup(loopname);

        if (!ov_dict_set(self->loops, name, loop, NULL)) {
            loop = ov_vocs_loop_free(loop);
            name = ov_data_pointer_free(name);
            goto error;
        }
    }

    if (!ov_vocs_loop_add_participant(loop, adata.socket, client, user, role))
        goto error;

    // ensure loop broadcast is registered
    if (!ov_broadcast_registry_set(
            self->broadcasts, loopname, adata.socket, OV_LOOP_BROADCAST))
        goto error;

    send_switch_loop_broadcast(self, adata.socket, loopname, current);

    if (current != request) {

        OV_ASSERT(request == OV_VOCS_SEND);

        if (!ov_event_async_set(
                self->async,
                uuid,
                (ov_event_async_data){.socket = adata.socket,
                                      .value = adata.value,
                                      .timedout.userdata = self,
                                      .timedout.callback = async_timedout},
                self->config.limits.response_usec)) {

            ov_log_error("failed to reset async");
            OV_ASSERT(1 == 0);
            goto error;
        }

        adata.value = NULL;

        if (!ov_vocs_core_perform_switch_loop_request(self->config.core,
                                         uuid,
                                         session_id,
                                         user,
                                         role,
                                         loopname,
                                         current,
                                         request)) {

            ov_log_error("Failed to perform talk switch %s session %s|%s|%s",
                         loopname,
                         session_id,
                         user,
                         role);

            goto error;
        }

    } else {

        out = ov_json_object();

        ov_json_value *participants = ov_vocs_loop_get_participants(loop);
        if (!ov_json_object_set(out, OV_KEY_PARTICIPANTS, participants))
            goto error;

        val = ov_json_string(ov_vocs_permission_to_string(current));
        if (!ov_json_object_set(out, OV_KEY_STATE, val)) goto error;

        val = ov_json_string(loopname);
        if (!ov_json_object_set(out, OV_KEY_LOOP, val)) goto error;

        val = NULL;

        send_switch_loop_user_broadcast(self, adata.socket, loopname, current);

        if (!send_success_response(self, adata.value, adata.socket, &out)) goto error;
    }

    ov_socket_json_set(self->connections, adata.socket, &data);
    adata.value = ov_json_value_free(adata.value);
    out = ov_json_value_free(out);
    data = ov_json_value_free(data);

    return true;
error:
    adata.value = ov_json_value_free(adata.value);
    data = ov_json_value_free(data);
    out = ov_json_value_free(out);
    return false;

}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_leave_loop(ov_vocs_app *self, 
                           const char *uuid,
                           const char *session_id,
                           const char *loopname,
                           uint64_t error_code,
                           const char *error_desc){

    ov_json_value *data = NULL;
    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!self || !loopname || !uuid || !session_id) goto error;

    ov_event_async_data adata = ov_event_async_unset(self->async, uuid);
    if (!adata.value) goto error;

    data = ov_socket_json_get(self->connections, adata.socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    switch (error_code) {

        case OV_ERROR_NOERROR:
            break;

        default:

            send_error_response(self, adata.value, adata.socket, error_code, error_desc);

            goto error;
            break;
    }

    if (!set_loop_state_in_data(data, loopname, OV_VOCS_NONE)) goto error;

    if (!ov_vocs_db_set_state(
            self->config.db, user, role, loopname, OV_VOCS_NONE))
        goto error;

    if (!ov_broadcast_registry_set(
            self->broadcasts, loopname, adata.socket, OV_BROADCAST_UNSET))
        goto error;

    ov_vocs_loop *loop = ov_dict_get(self->loops, loopname);
    OV_ASSERT(loop);

    ov_vocs_loop_drop_participant(loop, adata.socket);

    send_switch_loop_broadcast(self, adata.socket, loopname, OV_VOCS_NONE);

    out = ov_json_object();

    ov_json_value *participants = ov_vocs_loop_get_participants(loop);
    if (!ov_json_object_set(out, OV_KEY_PARTICIPANTS, participants)) goto error;

    val = ov_json_string(ov_vocs_permission_to_string(OV_VOCS_NONE));
    if (!ov_json_object_set(out, OV_KEY_STATE, val)) goto error;

    val = ov_json_string(loopname);
    if (!ov_json_object_set(out, OV_KEY_LOOP, val)) goto error;

    val = NULL;

    send_switch_loop_user_broadcast(self, adata.socket, loopname, OV_VOCS_NONE);

    if (!send_success_response(self, adata.value, adata.socket, &out)) goto error;

    if (!ov_socket_json_set(self->connections, adata.socket, &data)) goto error;

    ov_json_value_free(out);
    data = ov_json_value_free(data);
    adata.value = ov_json_value_free(adata.value);
    return true;

error:
    ov_json_value_free(out);
    data = ov_json_value_free(data);
    adata.value = ov_json_value_free(adata.value);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_loop_volume(ov_vocs_app *self, 
                           const char *uuid,
                           const char *session_id,
                           const char *loopname,
                           uint8_t volume,
                           uint64_t error_code,
                           const char *error_desc){

    ov_json_value *data = NULL;
    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!self || !loopname || !uuid || !session_id) goto error;

    ov_event_async_data adata = ov_event_async_unset(self->async, uuid);
    if (!adata.value) goto error;

    data = ov_socket_json_get(self->connections, adata.socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    volume = ov_convert_to_vol_percent(volume, 3);

    switch (error_code) {

        case OV_ERROR_NOERROR:
            break;

        default:

            send_error_response(self, adata.value, adata.socket, error_code, error_desc);

            goto error;
            break;
    }

    ov_vocs_db_set_volume(self->config.db, user, role, loopname, volume);

    out = ov_json_object();
    val = ov_json_number(volume);

    if (!ov_json_object_set(out, OV_KEY_VOLUME, val)) goto error;

    val = ov_json_string(loopname);
    if (!ov_json_object_set(out, OV_KEY_LOOP, val)) goto error;

    send_success_response(self, adata.value, adata.socket, &out);

    out = ov_json_value_free(out);
    data = ov_json_value_free(data);
    adata.value = ov_json_value_free(adata.value);
    return true;
error:
    out = ov_json_value_free(out);
    data = ov_json_value_free(data);
    adata.value = ov_json_value_free(adata.value);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_mixer_state(ov_vocs_app *self, 
                             const char *uuid,
                             const ov_json_value *state){

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *orig = NULL;

    if (!self || !uuid || !state) goto error;

    ov_event_async_data adata = ov_event_async_unset(self->async, uuid);
    if (!adata.value) goto error;

    if (!ov_json_value_copy((void **)&val, state)) goto error;

    if (!ov_json_object_set(
            ov_event_api_get_response(adata.value), OV_KEY_BACKEND, val))
        goto error;

    env_send(self, adata.socket, adata.value);

    return true;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    orig = ov_json_value_free(orig);
    return false;
}   

/*
 *      ------------------------------------------------------------------------
 *
 *      #ICE FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_vocs_app_ice_session_create(ov_vocs_app *self,
                                    const ov_response_state event,
                                    const char *session_id,
                                    const char *type,
                                    const char *sdp){

    ov_json_value *data = NULL;
    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_event_async_data adata = (ov_event_async_data){0};

    if (!self || !session_id || !type || !sdp) goto error;

    adata = ov_event_async_unset(self->async, event.id);
    if (!adata.value) goto error;

    data = ov_socket_json_get(self->connections, adata.socket);

    if (!ov_vocs_core_set_session(self->config.core, adata.socket, session_id))
        goto error;

    switch (event.result.error_code) {

        case OV_ERROR_NOERROR:
            break;

        default:

            send_error_response(self,
                                adata.value,
                                adata.socket,
                                event.result.error_code,
                                event.result.message);

            /* let client react to error */
            goto error;
            break;
    }

     if (!ov_json_object_set(data, OV_KEY_SESSION, ov_json_string(session_id)))
        goto error;

    if (!ov_socket_json_set(self->connections, adata.socket, &data))
        goto error;

    out = ov_json_object();

    val = ov_json_string(type);
    if (!ov_json_object_set(out, OV_KEY_TYPE, val)) goto error;

    val = ov_json_string(sdp);
    if (!ov_json_object_set(out, OV_KEY_SDP, val)) goto error;

    if (!send_success_response(self, adata.value, adata.socket, &out)) {

        out = ov_json_value_free(out);
        goto error;
    }
 
    out = ov_json_value_free(out);
    adata.value = ov_json_value_free(adata.value);

    return true;
error:
    ov_vocs_app_drop_connection(self, adata.socket);
    out = ov_json_value_free(out);
    adata.value = ov_json_value_free(adata.value);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_ice_session_complete(ov_vocs_app *self,
                                      int socket,
                                      const char *session_id,
                                      bool success){

    ov_json_value *data = NULL;

    if (!self || !session_id) goto error;

    data = ov_socket_json_get(self->connections, socket);

    if (success && ov_json_is_true(ov_json_get(data, "/" OV_KEY_ICE))) {

        data = ov_json_value_free(data);
        return true;
    }

    ov_json_object_set(data, OV_KEY_ICE, ov_json_true());

    if (!success) {

        ov_log_error("ICE session failed %s", session_id);
        goto error;

    } else if (ov_json_is_true(ov_json_get(data, "/" OV_KEY_MEDIA_READY))) {

        ov_json_value *out =
            ov_event_api_message_create(OV_KEY_MEDIA_READY, NULL, 0);

        ov_event_api_set_type(out, OV_KEY_UNICAST);

        if (!ov_event_api_set_parameter(out)) goto error;

        env_send(self, socket, out);
        out = ov_json_value_free(out);
    }

    ov_socket_json_set(self->connections, socket, &data);
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_ice_session_update(ov_vocs_app *self,
                                    const ov_response_state event,
                                    const char *session_id){

    ov_json_value *out = NULL;
    ov_event_async_data adata = (ov_event_async_data){0};

    if (!self || !session_id) goto error;

    adata = ov_event_async_unset(self->async, event.id);

    switch (event.result.error_code) {

        case OV_ERROR_NOERROR:

            if (adata.value) {

                out = ov_event_api_create_success_response(adata.value);

            } else {

                out = ov_event_api_message_create(OV_KEY_MEDIA, NULL, 0);
            }

            break;

        case OV_ERROR_CODE_NOT_A_RESPONSE:

            out = ov_event_api_message_create(OV_KEY_MEDIA, NULL, 0);
            break;

        default:

            if (adata.value) {

                out = ov_event_api_create_error_response(
                    adata.value, event.result.error_code, event.result.message);

            } else {

                /* We should only receive some error reponse for requests,
                 * which may be timed out already - ignore */

                goto error;
            }

            break;
    }

    OV_ASSERT(out);
    ov_event_api_set_type(out, OV_KEY_UNICAST);
    env_send(self, adata.socket, out);

    out = ov_json_value_free(out);
    adata.value = ov_json_value_free(adata.value);

    return true;
error:
    out = ov_json_value_free(out);
    adata.value = ov_json_value_free(adata.value);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_ice_session_state(ov_vocs_app *self,
                                   const ov_response_state event,
                                   const char *session_id,
                                   const ov_json_value *state){

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_event_async_data adata = (ov_event_async_data){0};

    if (!self || !session_id || !state) goto error;

    adata = ov_event_async_unset(self->async, event.id);
    if (!adata.value) goto error;

    const char *uuid = ov_event_api_get_uuid(adata.value);
    if (!uuid || !session_id) goto error;

    switch (event.result.error_code) {

        case OV_ERROR_NOERROR:

            break;

        default:

            out = ov_event_api_create_error_response(
                adata.value, event.result.error_code, event.result.message);

            env_send(self, adata.socket, out);
            goto error;
            break;
    }

    if (!ov_json_value_copy((void **)&val, state)) goto error;

    out = ov_event_api_create_success_response(adata.value);

    if (!ov_event_api_set_uuid(out, uuid)) goto error;

    if (!ov_json_object_set(
            ov_event_api_get_response(out), OV_KEY_FRONTEND, val))
        goto error;

    if (!ov_event_async_set(
            self->async,
            uuid,
            (ov_event_async_data){.socket = adata.socket,
                                  .value = out,
                                  .timedout.userdata = self,
                                  .timedout.callback = async_timedout},
            self->config.limits.response_usec)) {

        char *str = ov_json_value_to_string(out);
        ov_log_error("failed to set async msg - closing %i | %s", socket, str);
        str = ov_data_pointer_free(str);

        goto error;
    }

    out = NULL;

    out = ov_json_value_free(out);
    adata.value = ov_json_value_free(adata.value);
    return true;
error:
    out = ov_json_value_free(out);
    adata.value = ov_json_value_free(adata.value);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_ice_candidate(ov_vocs_app *self,
                               const ov_response_state event,
                               int socket,
                               const ov_ice_candidate_info *info){

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_event_async_data adata = (ov_event_async_data){0};

    if (!self || !info) goto error;

    adata = ov_event_async_unset(self->async, event.id);

    val = ov_ice_candidate_info_to_json(*info);
    if (!val) goto error;

    switch (event.result.error_code) {

        case OV_ERROR_NOERROR:

            if (adata.value) {

                out = ov_event_api_create_success_response(adata.value);
                if (!ov_json_object_set(out, OV_KEY_RESPONSE, val)) goto error;

                val = NULL;

            } else {

                out = ov_event_api_message_create(
                    OV_ICE_STRING_CANDIDATE, NULL, 0);
                if (!ov_json_object_set(out, OV_KEY_PARAMETER, val)) goto error;

                val = NULL;
            }

            break;

        case OV_ERROR_CODE_NOT_A_RESPONSE:

            out = ov_event_api_message_create(OV_ICE_STRING_CANDIDATE, NULL, 0);
            if (!ov_json_object_set(out, OV_KEY_PARAMETER, val)) goto error;

            val = NULL;
            break;

        default:

            if (adata.value) {

                out = ov_event_api_create_error_response(
                    adata.value, event.result.error_code, event.result.message);

            } else {

                /* We should only receive some error reponse for requests,
                 * which may be timed out already - ignore */

                goto error;
            }

            break;
    }

    OV_ASSERT(out);
    ov_event_api_set_type(out, OV_KEY_UNICAST);
    env_send(self, socket, out);

    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    adata.value = ov_json_value_free(adata.value);
    return true;

error:
    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    adata.value = ov_json_value_free(adata.value);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_ice_end_of_candidates(ov_vocs_app *self,
                               const ov_response_state event,
                               int socket,
                               const ov_ice_candidate_info *info){

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_event_async_data adata = (ov_event_async_data){0};

    if (!self || !info) goto error;

    adata = ov_event_async_unset(self->async, event.id);

    val = ov_ice_candidate_info_to_json(*info);
    if (!val) goto error;

    switch (event.result.error_code) {

        case OV_ERROR_NOERROR:

            if (adata.value) {

                out = ov_event_api_create_success_response(adata.value);
                if (!ov_json_object_set(out, OV_KEY_RESPONSE, val)) goto error;

                val = NULL;

            } else {

                out = ov_event_api_message_create(
                    OV_ICE_STRING_END_OF_CANDIDATES, NULL, 0);
                if (!ov_json_object_set(out, OV_KEY_PARAMETER, val)) goto error;

                val = NULL;
            }

            break;

        case OV_ERROR_CODE_NOT_A_RESPONSE:

            out = ov_event_api_message_create(OV_ICE_STRING_END_OF_CANDIDATES, NULL, 0);
            if (!ov_json_object_set(out, OV_KEY_PARAMETER, val)) goto error;

            val = NULL;
            break;

        default:

            if (adata.value) {

                out = ov_event_api_create_error_response(
                    adata.value, event.result.error_code, event.result.message);

            } else {

                /* We should only receive some error reponse for requests,
                 * which may be timed out already - ignore */

                goto error;
            }

            break;
    }

    OV_ASSERT(out);
    ov_event_api_set_type(out, OV_KEY_UNICAST);
    env_send(self, socket, out);

    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    adata.value = ov_json_value_free(adata.value);
    return true;

error:
    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    adata.value = ov_json_value_free(adata.value);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_ice_talk(ov_vocs_app *self,
                            const ov_response_state event,
                            const char *session_id,
                            int socket,
                            const ov_mc_loop_data data,
                            bool on){

    ov_json_value *sdata = NULL;
    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_event_async_data adata = (ov_event_async_data){0};

    if (!self) goto error;

    adata = ov_event_async_unset(self->async, event.id);
    if (!adata.value) goto error;

    const char *loop = data.name;
    const char *state = ov_json_string_get(
        ov_json_get(adata.value, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE));

    ov_vocs_permission current = OV_VOCS_SEND;
    ov_vocs_permission requested = ov_vocs_permission_from_string(state);

    switch (event.result.error_code) {

        case OV_ERROR_NOERROR:

            break;

        default:

            send_error_response(self,
                                adata.value,
                                adata.socket,
                                event.result.error_code,
                                event.result.message);

            goto error;
    }

    if (on) {

        current = OV_VOCS_SEND;

    } else {

        current = OV_VOCS_RECV;
    }

    sdata = ov_socket_json_get(self->connections, adata.socket);
    set_loop_state_in_data(sdata, loop, current);
    const char *user = ov_json_string_get(ov_json_get(sdata, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(sdata, "/" OV_KEY_ROLE));

    if (!ov_vocs_db_set_state(self->config.db, user, role, loop, current))
        goto error;

    ov_socket_json_set(self->connections, adata.socket, &sdata);

    switch (requested) {

        case OV_VOCS_SEND:
        case OV_VOCS_RECV:
            break;

        case OV_VOCS_NONE:
            goto switch_off_loop;
            break;
    }

    // we are at the final state for the switch command

    ov_vocs_loop *l = ov_dict_get(self->loops, loop);
    OV_ASSERT(l);

    ov_json_value *participants = ov_vocs_loop_get_participants(l);

    out = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_PARTICIPANTS, participants)) goto error;

    val = ov_json_string(ov_vocs_permission_to_string(current));
    if (!ov_json_object_set(out, OV_KEY_STATE, val)) goto error;

    val = ov_json_string(loop);
    if (!ov_json_object_set(out, OV_KEY_LOOP, val)) goto error;

    val = NULL;

    if (!send_switch_loop_user_broadcast(self, adata.socket, loop, current)) {

        ov_log_error(
            "failed to send switch loop user "
            "broadcast %s",
            loop);
    }

    if (!send_success_response(self, adata.value, adata.socket, &out)) {
        out = ov_json_value_free(out);
        goto error;
    }

    adata.value = ov_json_value_free(adata.value);
    ov_json_value_free(out);

    sdata = ov_json_value_free(sdata);

    return true;

switch_off_loop:

    // reset async event

    if (!ov_event_async_set(
            self->async,
            event.id,
            (ov_event_async_data){.socket = adata.socket,
                                  .value = adata.value,
                                  .timedout.userdata = self,
                                  .timedout.callback = async_timedout},
            self->config.limits.response_usec)) {

        goto error;
    }

    adata.value = NULL;

    if (!ov_vocs_core_leave_loop(
        self->config.core,
        event.id,
        session_id,
        loop)) goto error;

    sdata = ov_json_value_free(sdata);
    return true;


error:
    ov_vocs_app_drop_connection(self, socket);
    sdata = ov_json_value_free(sdata);
    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    adata.value = ov_json_value_free(adata.value);
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      SIP FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_vocs_app_sip_call_init(ov_vocs_app *self,
                         const char *uuid,
                         const char *loopname,
                         const char *call_id,
                         const char *caller,
                         const char *callee,
                         uint8_t error_code,
                         const char *error_desc){

    ov_event_async_data adata = {0};

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!self || !uuid) goto error;

    adata = ov_event_async_unset(self->async, uuid);
    if (!adata.value) goto response_loop;

    switch (error_code) {

        case OV_ERROR_NOERROR:

            out = ov_event_api_create_success_response(adata.value);
            par = ov_event_api_get_response(out);
            break;

        default:

            out = ov_event_api_create_error_response(
                adata.value, error_code, error_desc);

            par = ov_event_api_get_response(out);
    }

    if (loopname) {

        val = ov_json_string(loopname);
        if (!ov_json_object_set(par, OV_KEY_LOOP, val)) goto error;
    }

    val = NULL;

    if (call_id) {

        val = ov_json_string(call_id);
        if (!ov_json_object_set(par, OV_KEY_CALL_ID, val)) goto error;
    }

    val = NULL;

    if (caller) {

        val = ov_json_string(caller);
        if (!ov_json_object_set(par, OV_KEY_CALLER, val)) goto error;
    }

    val = NULL;

    if (callee) {

        val = ov_json_string(callee);
        if (!ov_json_object_set(par, OV_KEY_CALLEE, val)) goto error;
    }

    val = NULL;

    env_send(self, adata.socket, out);

    adata.value = ov_json_value_free(adata.value);
    ov_json_value_free(out);
    return true;

response_loop:

    out = ov_mc_sip_msg_create_call(loopname, callee, caller);
    par = ov_event_api_get_parameter(out);

    switch (error_code) {

        case OV_ERROR_NOERROR:
            break;

        default:

            ov_event_api_add_error(out, error_code, error_desc);
    }

    if (call_id) {

        val = ov_json_string(call_id);
        if (!ov_json_object_set(par, OV_KEY_CALL_ID, val)) goto error;
    }

    val = NULL;

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_LOOP_BROADCAST))
        goto error;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            self->broadcasts, loopname, &parameter, out, OV_LOOP_BROADCAST)) {

        goto error;
    }

    ov_json_value_free(out);
    return true;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_sip_new(ov_vocs_app *self, 
                         const char *loopname,
                         const char *call_id,
                         const char *peer){

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!self || !loopname) goto error;

    out = ov_event_api_message_create(OV_KEY_CALL, NULL, 0);

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_LOOP_BROADCAST))
        goto error;

    par = ov_event_api_set_parameter(out);

    val = ov_json_string(loopname);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val)) goto error;

    val = ov_json_string(call_id);
    if (!ov_json_object_set(par, OV_KEY_CALL_ID, val)) goto error;

    val = ov_json_string(peer);
    if (!ov_json_object_set(par, OV_KEY_PEER, val)) goto error;

    val = NULL;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            self->broadcasts, loopname, &parameter, out, OV_LOOP_BROADCAST)) {

        goto error;
    }


    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return true;

error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_sip_terminated(ov_vocs_app *self,
                                const char *call_id,
                                const char *loopname){

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!self || !call_id || !loopname) goto error;

    out = ov_event_api_message_create(OV_KEY_HANGUP, NULL, 0);

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_LOOP_BROADCAST))
        goto error;

    par = ov_event_api_set_parameter(out);

    val = ov_json_string(loopname);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val)) goto error;

    val = ov_json_string(call_id);
    if (!ov_json_object_set(par, OV_KEY_CALL_ID, val)) goto error;

    val = NULL;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            self->broadcasts, loopname, &parameter, out, OV_LOOP_BROADCAST)) {

        goto error;
    }

    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return true;

error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_sip_permit(ov_vocs_app *self,
                            const ov_sip_permission permission,
                            uint64_t error_code,
                            const char *error_desc){

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!self) goto error;

    if (NULL == permission.loop) goto error;

    out = ov_event_api_message_create(OV_KEY_PERMIT, NULL, 0);

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_LOOP_BROADCAST))
        goto error;

    val = ov_sip_permission_to_json(permission);
    if (!ov_json_object_set(out, OV_KEY_PARAMETER, val)) goto error;

    val = NULL;

    if (0 != error_code) {

        val = ov_event_api_create_error_code(error_code, error_desc);
        if (!ov_json_object_set(out, OV_KEY_ERROR, val)) goto error;

        ov_vocs_db_remove_permission(self->config.db, permission);
        val = NULL;
    }

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(self->broadcasts,
                                    permission.loop,
                                    &parameter,
                                    out,
                                    OV_LOOP_BROADCAST)) {

        goto error;
    }

    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_sip_revoke(ov_vocs_app *self,
                            const ov_sip_permission permission,
                            uint64_t error_code,
                            const char *error_desc){

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!self) goto error;

    if (NULL == permission.loop) goto error;

    out = ov_event_api_message_create(OV_KEY_REVOKE, NULL, 0);

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_LOOP_BROADCAST))
        goto error;

    val = ov_sip_permission_to_json(permission);
    if (!ov_json_object_set(out, OV_KEY_PARAMETER, val)) goto error;

    val = NULL;

    if (0 != error_code) {

        val = ov_event_api_create_error_code(error_code, error_desc);
        if (!ov_json_object_set(out, OV_KEY_ERROR, val)) goto error;

        val = NULL;
    }

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(self->broadcasts,
                                    permission.loop,
                                    &parameter,
                                    out,
                                    OV_LOOP_BROADCAST)) {

        goto error;
    }

    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return true;

error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_sip_list_calls(ov_vocs_app *self,
                                const char *uuid,
                                const ov_json_value *calls,
                                uint64_t error_code,
                                const char *error_desc){

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    ov_event_async_data adata = {0};

    if (!self) goto error;

    adata = ov_event_async_unset(self->async, uuid);
    if (!adata.value) goto error;

    switch (error_code) {

        case OV_ERROR_NOERROR:

            out = ov_event_api_create_success_response(adata.value);
            par = ov_event_api_get_response(out);
            val = NULL;
            ov_json_value_copy((void **)&val, calls);
            if (!ov_json_object_set(par, OV_KEY_CALLS, val)) goto error;

            val = NULL;

            break;

        default:

            out = ov_event_api_create_error_response(
                adata.value, error_code, error_desc);
    }

    env_send(self, adata.socket, out);

    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    adata.value = ov_json_value_free(adata.value);
    return true;

error:
    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    adata.value = ov_json_value_free(adata.value);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_sip_list_permissions(ov_vocs_app *self,
                                const char *uuid,
                                const ov_json_value *permissions,
                                uint64_t error_code,
                                const char *error_desc){

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    ov_event_async_data adata = {0};

    if (!self) goto error;
    
    adata = ov_event_async_unset(self->async, uuid);
    if (!adata.value) goto error;

    switch (error_code) {

        case OV_ERROR_NOERROR:

            out = ov_event_api_create_success_response(adata.value);
            par = ov_event_api_get_response(out);
            val = NULL;
            ov_json_value_copy((void **)&val, permissions);
            if (!ov_json_object_set(par, OV_KEY_PERMISSIONS, val)) goto error;

            val = NULL;

            break;

        default:

            out = ov_event_api_create_error_response(
                adata.value, error_code, error_desc);
    }

    env_send(self, adata.socket, out);

    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    adata.value = ov_json_value_free(adata.value);
    return true;

error:
    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    adata.value = ov_json_value_free(adata.value);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_sip_get_status(ov_vocs_app *self,
                                const char *uuid,
                                const ov_json_value *status,
                                uint64_t error_code,
                                const char *error_desc){

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    ov_event_async_data adata = {0};

    if (!self) goto error;

    adata = ov_event_async_unset(self->async, uuid);
    if (!adata.value) goto error;

    switch (error_code) {

        case OV_ERROR_NOERROR:

            out = ov_event_api_create_success_response(adata.value);
            par = ov_event_api_get_response(out);
            val = NULL;
            ov_json_value_copy((void **)&val, status);
            if (!ov_json_object_set(par, OV_KEY_STATUS, val)) goto error;

            val = NULL;

            break;

        default:

            out = ov_event_api_create_error_response(
                adata.value, error_code, error_desc);
    }

    env_send(self, adata.socket, out);

    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    adata.value = ov_json_value_free(adata.value);
    return true;

error:
    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    adata.value = ov_json_value_free(adata.value);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_sip_connected(ov_vocs_app *self,
                                bool status){

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;

    if (!self) goto error;

    out = ov_event_api_message_create(OV_KEY_SIP, NULL, 0);
    par = ov_event_api_set_parameter(out);

    if (status) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }

    ov_json_object_set(par, OV_KEY_CONNECTED, val);

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(self->broadcasts,
                                    OV_BROADCAST_KEY_SYSTEM_BROADCAST,
                                    &parameter,
                                    out,
                                    OV_SYSTEM_BROADCAST)) {

        goto error;
    }

    ov_json_value_free(out);
    return true;

error:
    ov_json_value_free(out);
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      RECORDER FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_vocs_app_record_start(ov_vocs_app *self,
     const char *uuid, ov_result error){

    if (!self) goto error;

    ov_event_async_data adata = ov_event_async_unset(self->async, uuid);

    if (!adata.value) goto error;

    switch (error.error_code) {

        case OV_ERROR_NOERROR:

            send_success_response(self, adata.value, adata.socket, NULL);
            break;

        default:
            send_error_response(self,
                            adata.value, adata.socket,
                            error.error_code,
                            error.message);
    }

    adata.value = ov_json_value_free(adata.value);

    return true;
error:
    adata.value = ov_json_value_free(adata.value);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_record_stop(ov_vocs_app *self,
     const char *uuid, ov_result error){

        if (!self) goto error;

    ov_event_async_data adata = ov_event_async_unset(self->async, uuid);

    if (!adata.value) goto error;

    switch (error.error_code) {

        case OV_ERROR_NOERROR:

            send_success_response(self, adata.value, adata.socket, NULL);
            break;

        default:
            send_error_response(self,
                            adata.value, adata.socket,
                            error.error_code,
                            error.message);
    }

    adata.value = ov_json_value_free(adata.value);

    return true;
error:
    adata.value = ov_json_value_free(adata.value);
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      VAD FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_vocs_app_vad(ov_vocs_app *self,
                     const char *loop,
                     bool on){
    
    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!self) goto error;

    out = ov_event_api_message_create(OV_KEY_VAD, NULL, 0);

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_LOOP_BROADCAST))
        goto error;

    par = ov_event_api_set_parameter(out);

    if (!ov_json_object_set(par, OV_KEY_LOOP, ov_json_string(loop))) goto error;

    if (on) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }

    if (!ov_json_object_set(par, OV_KEY_ON, val)) goto error;

    val = NULL;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            self->broadcasts, loop, &parameter, out, OV_LOOP_BROADCAST))
        goto error;

    out = ov_json_value_free(out);
    return true;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return false;
}