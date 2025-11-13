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
#include "../include/ov_vocs_api_authentication.h"
#include "../include/ov_vocs_api_client.h"
#include "../include/ov_vocs_api_admin.h"

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

    struct  {

        ov_vocs_api_authentication *authentication;
        ov_vocs_api_client *client;
        ov_vocs_api_admin *admin;

    } api;


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

bool ov_vocs_app_send(void *userdata, int socket, const ov_json_value *input) {

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !input) return false;

    return env_send(self, socket, input);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_app_send_error_response(ov_vocs_app *self,
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

bool ov_vocs_app_send_success_response(ov_vocs_app *self,
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

bool ov_vocs_app_send_switch_loop_broadcast(ov_vocs_app *self,
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
        (ov_event_parameter){.send.instance = self, .send.send = ov_vocs_app_send};

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

bool ov_vocs_app_send_switch_loop_user_broadcast(ov_vocs_app *self,
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
        (ov_event_parameter){.send.instance = self, .send.send = ov_vocs_app_send};

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

bool ov_vocs_app_send_switch_volume_user_broadcast(ov_vocs_app *self,
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
        (ov_event_parameter){.send.instance = self, .send.send = ov_vocs_app_send};

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

bool ov_vocs_app_send_talking_loop_broadcast(ov_vocs_app *self,
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
        (ov_event_parameter){.send.instance = self, .send.send = ov_vocs_app_send};

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
            if (!ov_vocs_app_send_switch_loop_broadcast(
                    c->vocs, c->socket, key, OV_VOCS_NONE)) {
                ov_log_error("failed to send switch loop broadcast %s", key);
            }
    }

    return true;
}


/*----------------------------------------------------------------------------*/

bool ov_vocs_app_drop_connection(ov_vocs_app *self, int socket){

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

bool ov_vocs_app_ov_vocs_app_drop_connection(ov_vocs_app *self, int socket){

    return ov_vocs_app_drop_connection(self, socket);
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

    ov_vocs_app_drop_connection(self, data.socket);

error:
    ov_event_async_data_clear(&data);
    return;
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

ov_vocs_app *ov_vocs_app_create(ov_vocs_app_config config){

    ov_vocs_app *self = NULL;

    if (!init_config(&config)) goto error;

    self = calloc(1, sizeof(ov_vocs_app));
    if (!self) goto error;

    self->magic_byte = OV_VOCS_APP_MAGIC_BYTES;
    self->config = config;

    self->engine = ov_event_engine_create();    

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

    ov_vocs_api_authentication_config auth_config = (ov_vocs_api_authentication_config){
        .loop = self->config.loop,
        .app = self,
        .db = self->config.db,
        .persistance = self->config.persistance,
        .engine = self->engine,
        .ldap = self->config.ldap,
        .user_sessions = self->user_sessions,
        .connections = self->connections,
        .async = self->async,
        .broadcasts = self->broadcasts,
        .limits.response_usec = self->config.limits.response_usec
    };

    self->api.authentication = ov_vocs_api_authentication_create(auth_config);
    if (!self->api.authentication) goto error;

    ov_vocs_api_client_config c_config = (ov_vocs_api_client_config){
        .loop = self->config.loop,
        .app = self,
        .core = self->config.core,
        .db = self->config.db,
        .persistance = self->config.persistance,
        .engine = self->engine,
        .loops = self->loops,
        .user_sessions = self->user_sessions,
        .connections = self->connections,
        .async = self->async,
        .broadcasts = self->broadcasts,
        .limits.response_usec = self->config.limits.response_usec
    };

    self->api.client = ov_vocs_api_client_create(c_config);
    if (!self->api.client) goto error;

    ov_vocs_api_admin_config a_config = (ov_vocs_api_admin_config){
        .loop = self->config.loop,
        .app = self,
        .core = self->config.core,
        .db = self->config.db,
        .ldap = self->config.ldap,
        .persistance = self->config.persistance,
        .engine = self->engine,
        .loops = self->loops,
        .user_sessions = self->user_sessions,
        .connections = self->connections,
        .async = self->async,
        .broadcasts = self->broadcasts,
        .limits.response_usec = self->config.limits.response_usec
    };

    self->api.admin = ov_vocs_api_admin_create(a_config);
    if (!self->api.admin) goto error;

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

    self->api.authentication = ov_vocs_api_authentication_free(self->api.authentication);
    self->api.client = ov_vocs_api_client_free(self->api.client);
    self->api.admin = ov_vocs_api_admin_free(self->api.admin);

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

    return ov_event_engine_push(self->engine, socket, *params, input);

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

            ov_vocs_app_send_error_response(self, adata.value, adata.socket, error_code, error_desc);

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

    ov_vocs_app_send_switch_loop_broadcast(self, adata.socket, loopname, current);

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

        ov_vocs_app_send_switch_loop_user_broadcast(self, adata.socket, loopname, current);

        if (!ov_vocs_app_send_success_response(self, adata.value, adata.socket, &out)) goto error;
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

            ov_vocs_app_send_error_response(self, adata.value, adata.socket, error_code, error_desc);

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

    ov_vocs_app_send_switch_loop_broadcast(self, adata.socket, loopname, OV_VOCS_NONE);

    out = ov_json_object();

    ov_json_value *participants = ov_vocs_loop_get_participants(loop);
    if (!ov_json_object_set(out, OV_KEY_PARTICIPANTS, participants)) goto error;

    val = ov_json_string(ov_vocs_permission_to_string(OV_VOCS_NONE));
    if (!ov_json_object_set(out, OV_KEY_STATE, val)) goto error;

    val = ov_json_string(loopname);
    if (!ov_json_object_set(out, OV_KEY_LOOP, val)) goto error;

    val = NULL;

    ov_vocs_app_send_switch_loop_user_broadcast(self, adata.socket, loopname, OV_VOCS_NONE);

    if (!ov_vocs_app_send_success_response(self, adata.value, adata.socket, &out)) goto error;

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

            ov_vocs_app_send_error_response(self, adata.value, adata.socket, error_code, error_desc);

            goto error;
            break;
    }

    ov_vocs_db_set_volume(self->config.db, user, role, loopname, volume);

    out = ov_json_object();
    val = ov_json_number(volume);

    if (!ov_json_object_set(out, OV_KEY_VOLUME, val)) goto error;

    val = ov_json_string(loopname);
    if (!ov_json_object_set(out, OV_KEY_LOOP, val)) goto error;

    ov_vocs_app_send_success_response(self, adata.value, adata.socket, &out);

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

            ov_vocs_app_send_error_response(self,
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

    if (!ov_vocs_app_send_success_response(self, adata.value, adata.socket, &out)) {

        out = ov_json_value_free(out);
        goto error;
    }
 
    out = ov_json_value_free(out);
    adata.value = ov_json_value_free(adata.value);

    return true;
error:
    ov_vocs_app_ov_vocs_app_drop_connection(self, adata.socket);
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

            ov_vocs_app_send_error_response(self,
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

    if (!ov_vocs_app_send_switch_loop_user_broadcast(self, adata.socket, loop, current)) {

        ov_log_error(
            "failed to send switch loop user "
            "broadcast %s",
            loop);
    }

    if (!ov_vocs_app_send_success_response(self, adata.value, adata.socket, &out)) {
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
    ov_vocs_app_ov_vocs_app_drop_connection(self, socket);
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
        (ov_event_parameter){.send.instance = self, .send.send = ov_vocs_app_send};

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
        (ov_event_parameter){.send.instance = self, .send.send = ov_vocs_app_send};

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
        (ov_event_parameter){.send.instance = self, .send.send = ov_vocs_app_send};

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
        (ov_event_parameter){.send.instance = self, .send.send = ov_vocs_app_send};

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
        (ov_event_parameter){.send.instance = self, .send.send = ov_vocs_app_send};

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
        (ov_event_parameter){.send.instance = self, .send.send = ov_vocs_app_send};

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

            ov_vocs_app_send_success_response(self, adata.value, adata.socket, NULL);
            break;

        default:
            ov_vocs_app_send_error_response(self,
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

            ov_vocs_app_send_success_response(self, adata.value, adata.socket, NULL);
            break;

        default:
            ov_vocs_app_send_error_response(self,
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
        (ov_event_parameter){.send.instance = self, .send.send = ov_vocs_app_send};

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