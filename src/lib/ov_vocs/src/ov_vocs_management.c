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
        @file           ov_vocs_management.c
        @author         Töpfer, Markus

        @date           2025-09-17


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_management.h"

#include "../include/ov_mc_backend.h"

#include <ov_base/ov_event_keys.h>
#include <ov_base/ov_error_codes.h>

#include <ov_core/ov_event_api.h>
#include <ov_core/ov_event_engine.h>
#include <ov_core/ov_socket_json.h>
#include <ov_core/ov_event_async.h>
#include <ov_core/ov_event_session.h>

#define OV_VOCS_MANAGEMENT_MAGIC_BYTES 0xef32

/*----------------------------------------------------------------------------*/

struct ov_vocs_management {

    uint16_t magic_bytes;
    ov_vocs_management_config config;

    ov_event_engine *engine;
    ov_event_session *user_sessions;
    ov_event_async_store *async;

    ov_socket_json *connections;

};

/*----------------------------------------------------------------------------*/

static bool env_send(ov_vocs_management *vocs, int socket, const ov_json_value *input) {

    OV_ASSERT(vocs);
    OV_ASSERT(vocs->config.env.send);
    OV_ASSERT(vocs->config.env.userdata);

    bool result =
        vocs->config.env.send(vocs->config.env.userdata, socket, input);

    return result;
}

/*----------------------------------------------------------------------------*/

static bool send_error_response(ov_vocs_management *vocs,
                                const ov_json_value *input,
                                int socket,
                                uint64_t code,
                                const char *desc) {

    bool result = false;

    if (!vocs || !input) goto error;

    if (!desc) desc = OV_ERROR_DESC;

    ov_json_value *out = ov_event_api_create_error_response(input, code, desc);
    ov_event_api_set_type(out, OV_KEY_UNICAST);

    result = env_send(vocs, socket, out);

    out = ov_json_value_free(out);
    return result;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool send_success_response(ov_vocs_management *vocs,
                                  const ov_json_value *input,
                                  int socket,
                                  ov_json_value **response) {

    bool result = false;

    if (!vocs || !input) goto error;

    ov_json_value *out = ov_event_api_create_success_response(input);
    ov_event_api_set_type(out, OV_KEY_UNICAST);

    if (response && *response) {

        if (ov_json_object_set(out, OV_KEY_RESPONSE, *response))
            *response = NULL;
    }

    result = env_send(vocs, socket, out);

    out = ov_json_value_free(out);
    return result;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool drop_connection(ov_vocs_management *self, int socket){

    if (!self || socket < 0) goto error;

    ov_log_debug("dropping connection %i", socket);

    ov_event_async_drop(self->async, socket);
    ov_socket_json_drop(self->connections, socket);
    self->config.env.close(self->config.env.userdata, socket);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void async_timedout(void *userdata, ov_event_async_data data) {

    ov_vocs_management *self = ov_vocs_management_cast(userdata);
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

/*----------------------------------------------------------------------------*/

static void cb_ldap_authentication(void *userdata,
                                   const char *uuid,
                                   ov_ldap_auth_result result) {

    ov_json_value *out = NULL;
    ov_json_value *data = NULL;

    ov_event_async_data adata = {0};

    ov_vocs_management *self = ov_vocs_management_cast(userdata);
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

    ov_vocs_management *self = ov_vocs_management_cast(userdata);

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
                self->config.timeout.response_usec)) {

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

    ov_vocs_management *self = ov_vocs_management_cast(userdata);

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
    ov_vocs_management *self = ov_vocs_management_cast(userdata);

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

/*----------------------------------------------------------------------------*/

static bool get_mixer_count(void *userdata,
                                int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *res = NULL;

    ov_vocs_management *self = ov_vocs_management_cast(userdata);
    if (!self || !params || !input) goto error;

    ov_mc_backend_registry_count count = ov_vocs_count_mixers(self->config.vocs);

    out = ov_event_api_create_success_response(input);
    res = ov_event_api_get_response(out);

    val = ov_json_number(count.mixers);
    if (!ov_json_object_set(res, OV_KEY_MIXER, val)) goto error;

    val = ov_json_number(count.used);
    if (!ov_json_object_set(res, OV_KEY_USED, val)) goto error;

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

static void cb_get_mixer_state(void *userdata, int socket, ov_json_value *input){

    ov_vocs_management *self = ov_vocs_management_cast(userdata);
    if (!self || !input) goto error;

    env_send(self, socket, input);
    input = ov_json_value_free(input);
error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool get_mixer_state(void *userdata,
                                int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    ov_vocs_management *self = ov_vocs_management_cast(userdata);
    if (!self || !params || !input) goto error;

    const char *uuid = ov_event_api_get_uuid(input);

    const char *user = ov_json_string_get(
        ov_json_get(input, "/"OV_KEY_PARAMETER"/"OV_KEY_USER));

    bool result = ov_vocs_get_mixer_state(
        self->config.vocs,
        uuid,
        user,
        self,
        socket,
        cb_get_mixer_state);

    input = ov_json_value_free(input);
    
    return result;
error:
    ov_json_value_free(input);
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

    ov_vocs_management *self = ov_vocs_management_cast(userdata);
    if (!self || !params || !input) goto error;

    val = ov_vocs_get_connection_state(self->config.vocs);

    out = ov_event_api_create_success_response(input);
    res = ov_event_api_get_response(out);

    if (!ov_json_object_set(res, OV_KEY_CONNECTIONS, val)) goto error;

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

static bool get(void *userdata,
                                int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    UNUSED(params);

    ov_vocs_management *self = ov_vocs_management_cast(userdata);

    if (!self || !input) goto error;

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

    /* all users are allowed to get anything */

    val = ov_vocs_db_get_entity(self->config.db, entity, id);
    if (!val) {
        val = ov_json_null();
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    if (!ov_json_object_set(res, OV_KEY_RESULT, val)) goto error;

    val = ov_json_string(type);
    if (!ov_json_object_set(res, OV_KEY_TYPE, val)) goto error;

    val = NULL;

response:

    env_send(self, socket, out);
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

static void cb_start_recording(void *userdata, int socket, const char *uuid, const char *loop,
    ov_result result){

    ov_json_value *out = NULL;
    ov_json_value *res = NULL;
    ov_json_value *val = NULL;

    ov_vocs_management *self = ov_vocs_management_cast(userdata);
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

    ov_vocs_management *self = ov_vocs_management_cast(userdata);

    if (!self || !input) goto error;

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

    ov_vocs_start_recording(self->config.vocs, 
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

    ov_vocs_management *self = ov_vocs_management_cast(userdata);
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

    ov_vocs_management *self = ov_vocs_management_cast(userdata);

    if (!self || !input) goto error;

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

    ov_vocs_stop_recording(self->config.vocs, 
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

    ov_vocs_management *self = ov_vocs_management_cast(userdata);

    if (!self || !input) goto error;

    val = ov_vocs_get_recorded_loops(self->config.vocs);
    if (!val) val = ov_json_object();

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    if (!ov_json_object_set(res, OV_KEY_RESULT, val)) goto error;

    val = NULL;

    env_send(self, socket, out);

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

    ov_vocs_management *self = ov_vocs_management_cast(userdata);

    if (!self || !input) goto error;

    val = ov_vocs_db_get_all_loops_incl_domain(self->config.db);
    if (!val){
        val = ov_json_object();
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    if (!ov_json_object_set(res, OV_KEY_RESULT, val)) goto error;

    val = NULL;

    env_send(self, socket, out);
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

    ov_vocs_management *self = ov_vocs_management_cast(userdata);

    if (!self || !input) goto error;

    ov_vocs_send_broadcast(self->config.vocs, socket, input);

    input = ov_json_value_free(input);
    return true;
error:
    input = ov_json_value_free(input);
    return false;
}


/*----------------------------------------------------------------------------*/

static bool register_events(ov_vocs_management *self){

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
                                  OV_EVENT_API_STATE_MIXER,
                                  get_mixer_count)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  "get_mixer_state",
                                  get_mixer_state)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_EVENT_API_STATE_CONNECTIONS,
                                  get_connection_state)) goto error;

    if (!ov_event_engine_register(self->engine,
                                  "get",
                                  get)) goto error;

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

    



    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_vocs_management *ov_vocs_management_create(ov_vocs_management_config config){

    ov_vocs_management *self = NULL;

    if (!config.vocs || !config.db || !config.loop) goto error;

    self = calloc(1, sizeof(ov_vocs_management));
    if (!self) goto error;

    self->magic_bytes = OV_VOCS_MANAGEMENT_MAGIC_BYTES;
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


    return self;
error:
    ov_vocs_management_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_management *ov_vocs_management_free(ov_vocs_management *self){

    if (!ov_vocs_management_cast(self)) return self;

    self->engine = ov_event_engine_free(self->engine);
    self->user_sessions = ov_event_session_free(self->user_sessions);
    self->connections = ov_socket_json_free(self->connections);
    self->async = ov_event_async_store_free(self->async);

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_management *ov_vocs_management_cast(const void *self){

    if (!self) goto error;

    if (*(uint16_t *)self == OV_VOCS_MANAGEMENT_MAGIC_BYTES) 
        return (ov_vocs_management *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static void cb_socket_close(void *userdata, int socket) {

    ov_vocs_management *self = ov_vocs_management_cast(userdata);
    if (!self || socket < 0) goto error;

    ov_log_debug("Client socket close at %i", socket);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool cb_client_process(void *userdata,
                              const int socket,
                              const ov_event_parameter *params,
                              ov_json_value *input) {

    ov_vocs_management *self = ov_vocs_management_cast(userdata);
    if (!self || !params) goto error;

    return ov_event_engine_push(
        self->engine,
        self,
        socket,
        *params,
        input);

    /* Close connection socket with false as return value */

error:
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_event_io_config ov_vocs_management_io_uri_config(ov_vocs_management *self){

    return (ov_event_io_config){.name = "management",
                                .userdata = self,
                                .callback.close = cb_socket_close,
                                .callback.process = cb_client_process};
}