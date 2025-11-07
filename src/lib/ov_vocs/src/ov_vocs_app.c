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

#define OV_VOCS_APP_MAGIC_BYTES 0xab1f

/*----------------------------------------------------------------------------*/

struct ov_vocs_app {

    uint16_t magic_bytes;
    ov_vocs_app_config config;

    ov_event_engine *engine;
    
    ov_event_session *user_sessions;
    ov_socket_json *connections;

    ov_event_async_store *async;

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

/*----------------------------------------------------------------------------*/

static bool drop_connection(ov_vocs_app *self, int socket){

    if (!self || socket < 0) goto error;

    ov_log_debug("dropping connection %i", socket);

    ov_event_async_drop(self->async, socket);
    ov_socket_json_drop(self->connections, socket);

    ov_vocs_core_drop_connection(self->config.core, socket);

    return true;
error:
    return false;
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

static void cb_core_create_session(void *userdata, 
    const ov_response_state event,
    const char *session_id,
    const char *type,
    const char *sdp){

    ov_json_value *data = NULL;
    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_id new_uuid = {0};

    ov_vocs_app *self = ov_vocs_app_cast(userdata);

    ov_event_async_data adata = ov_event_async_unset(vocs->async, event.id);
    
    if (!adata.value){

        if (OV_ERROR_NOERROR == event.result.error_code) goto drop_ice_session;

    }

    /* orig request available */

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

    data = ov_socket_json_get(self->connections, socket);

    char *key = strdup(session_id);
    if (!key) goto error;

    if (!ov_dict_set(self->sessions, key, (void *)(intptr_t)socket, NULL)) {
        key = ov_data_pointer_free(key);
        goto drop_ice_session;
    }

    if (!ov_json_object_set(data, OV_KEY_SESSION, ov_json_string(session_id)))
        goto drop_ice_session;

    if (!ov_socket_json_set(self->connections, socket, &data))
        goto drop_ice_session;
    
    out = ov_json_object();

    val = ov_json_string(type);
    if (!ov_json_object_set(out, OV_KEY_TYPE, val)) goto drop_session;

    val = ov_json_string(sdp);
    if (!ov_json_object_set(out, OV_KEY_SDP, val)) goto drop_session;

    if (!send_success_response(self, adata.value, socket, &out)) {

        out = ov_json_value_free(out);
        goto drop_session;
    }

    adata.value = ov_json_value_free(adata.value);
    data = ov_json_value_free(data);
    return;

drop_session:

    out = ov_json_value_free(out);
    val = ov_json_value_free(val);

    // fall into drop_ice_session

drop_ice_session:

    ov_id_fill_with_uuid(new_uuid);
    ov_vocs_core_drop_session(self->config.core, new_uuid, session_id);

    // fall into error

error:
    data = ov_json_value_free(data);
    adata.value = ov_json_value_free(adata.value);
    return;
}


/*----------------------------------------------------------------------------*/

static bool client_media(void *userdata,
                                int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input)  {

    UNUSED(params);
    ov_json_value *data = NULL;

    ov_vocs_app *self = ov_vocs_app_cast(userdata);
    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

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

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    switch (type) {

        case OV_MEDIA_REQUEST:

            if (!ov_vocs_core_create_session(
                self->config.core,
                uuid,
                OV_VOCS_DEFAULT_SDP)){

                send_error_response(vocs,
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

                send_error_response(vocs,
                                    input,
                                    socket,
                                    OV_ERROR_CODE_PARAMETER_ERROR,
                                    OV_ERROR_DESC_PARAMETER_ERROR);

                goto error;
            }

            if (!session) {

                send_error_response(vocs,
                                    input,
                                    socket,
                                    OV_ERROR_CODE_SESSION_UNKNOWN,
                                    OV_ERROR_DESC_SESSION_UNKNOWN);

                goto error;
            }



            if (!ov_mc_frontend_update_session(
                    vocs->frontend, uuid, session, type, sdp)) {

                send_error_response(vocs,
                                    input,
                                    socket,
                                    OV_ERROR_CODE_PROCESSING_ERROR,
                                    OV_ERROR_DESC_PROCESSING_ERROR);

                goto error;
            }

            break;

        default:

            send_error_response(vocs,
                                input,
                                socket,
                                OV_ERROR_CODE_PARAMETER_ERROR,
                                OV_ERROR_DESC_PARAMETER_ERROR);

            goto error;
    }

    if (!ov_event_async_set(
            vocs->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = vocs,
                                  .timedout.callback = async_timedout},
            vocs->config.timeout.response_usec)) {

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

/*
 *      ------------------------------------------------------------------------
 *
 *      #API #MANAGEMENT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */


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

    if (0 == config.sessions.path[0])
        strncpy(config.session.path, "/tmp/openvocs/sessions", PATH_MAX);

    if (0 == config.limits.response_usec)
        config.limits.response_usec = 5000000

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