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
        @file           ov_vocs_api_authentication.c
        @author         Töpfer, Markus

        @date           2025-11-12


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_api_authentication.h"

/*----------------------------------------------------------------------------*/

#define OV_VOCS_API_AUTHENTICATION_MAGIC_BYTES 0xa001

/*----------------------------------------------------------------------------*/

struct ov_vocs_api_authentication {

    uint16_t magic_byte;
    ov_vocs_api_authentication_config config;

};


/*----------------------------------------------------------------------------*/

static void async_timedout(void *userdata, ov_event_async_data data) {

    ov_vocs_api_authentication *self = ov_vocs_api_authentication_cast(userdata);
    if (!self) goto error;

    char *str = ov_json_value_to_string(data.value);
    ov_log_error("Async timeout - dropping %i | %s", data.socket, str);
    str = ov_data_pointer_free(str);

    ov_json_value *out = ov_event_api_create_error_response(
        data.value, OV_ERROR_TIMEOUT, OV_ERROR_DESC_TIMEOUT);
    ov_vocs_app_send(self->config.app, data.socket, out);
    out = ov_json_value_free(out);

    ov_vocs_app_drop_connection(self->config.app, data.socket);

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

    ov_vocs_api_authentication *self = ov_vocs_api_authentication_cast(userdata);
    OV_ASSERT(self);
    if (!self) goto error;

    adata = ov_event_async_unset(self->config.async, uuid);
    if (!adata.value) goto error;

    data = ov_socket_json_get(self->config.connections, adata.socket);

    const char *client_id =
        ov_json_string_get(ov_json_object_get(adata.value, OV_KEY_CLIENT));

    const char *user = ov_json_string_get(
        ov_json_get(adata.value, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    const char *session_id = NULL;

    switch (result) {

        case OV_LDAP_AUTH_REJECTED:

            ov_log_error(
                "LDAP AUTHENTICATE failed at %i | %s", adata.socket, user);

            ov_vocs_app_send_error_response(self->config.app,
                                adata.value,
                                adata.socket,
                                OV_ERROR_CODE_AUTH,
                                OV_ERROR_DESC_AUTH);

            ov_vocs_app_drop_connection(self->config.app, adata.socket);
            goto error;
            break;

        case OV_LDAP_AUTH_GRANTED:
            ov_log_info(
                "LDAP AUTHENTICATE granted at %i | %s", adata.socket, user);
            break;
    }

    session_id = ov_event_session_init(self->config.user_sessions, client_id, user);

    /* successful authenticated */

    if (!ov_broadcast_registry_set(
            self->config.broadcasts, user, adata.socket, OV_USER_BROADCAST)) {
        goto error;
    }

    if (!ov_broadcast_registry_set(self->config.broadcasts,
                                   OV_BROADCAST_KEY_SYSTEM_BROADCAST,
                                   adata.socket,
                                   OV_SYSTEM_BROADCAST)) {
        goto error;
    }

    ov_json_object_set(data, OV_KEY_CLIENT, ov_json_string(client_id));
    ov_json_object_set(data, OV_KEY_USER, ov_json_string(user));

    ov_socket_json_set(self->config.connections, adata.socket, &data);

    out = ov_json_object();
    if (!ov_vocs_json_set_id(out, user)) goto error;
    if (!ov_vocs_json_set_session_id(out, session_id)) goto error;

    result = ov_vocs_app_send_success_response(self->config.app, adata.value, adata.socket, &out);
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

    ov_vocs_api_authentication *self = ov_vocs_api_authentication_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (user) {

        ov_vocs_app_send_error_response(self->config.app,
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
        ov_event_session_get_user(self->config.user_sessions, client_id);

    user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));
    pass = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD));

    if (session_user && user && pass) {

        if (ov_event_session_verify(
                self->config.user_sessions, client_id, user, pass)) {

            ov_log_debug("relogin session for client %s", client_id);

            goto login_session_user;
        }
    }

    if (!user || !pass) {

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (self->config.ldap) {

        ov_log_debug("Requesting LDAP authentication for user %s", user);

        if (!ov_event_async_set(
                self->config.async,
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

        ov_vocs_app_send_error_response(self->config.app,
            input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        ov_log_error("VOCS AUTHENTICATE local failed at %i | %s", socket, user);
        goto error;
    }

login_session_user:

    /* successful authenticated */

    if (!ov_broadcast_registry_set(
            self->config.broadcasts, user, socket, OV_USER_BROADCAST)) {
        goto error;
    }

    if (!ov_broadcast_registry_set(self->config.broadcasts,
                                   OV_BROADCAST_KEY_SYSTEM_BROADCAST,
                                   socket,
                                   OV_SYSTEM_BROADCAST)) {
        goto error;
    }

    session_id = ov_event_session_init(self->config.user_sessions, client_id, user);

    /* successful authenticated */

    ov_json_object_set(data, OV_KEY_CLIENT, ov_json_string(client_id));
    ov_json_object_set(data, OV_KEY_USER, ov_json_string(user));

    ov_socket_json_set(self->config.connections, socket, &data);

    out = ov_json_object();
    if (!ov_vocs_json_set_id(out, user)) goto error;
    if (!ov_vocs_json_set_session_id(out, session_id)) goto error;

    result = ov_vocs_app_send_success_response(self->config.app, input, socket, &out);
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

    ov_vocs_api_authentication *self = ov_vocs_api_authentication_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);
    const char *session =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *client =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_CLIENT));

    ov_vocs_app_send_success_response(self->config.app, input, socket, NULL);

    ov_log_info("client logout %i|%s user %s", socket, session, user);

    ov_event_session_delete(self->config.user_sessions, client);

    /* Close connection socket if drop is not successfull */
    result = ov_vocs_app_drop_connection(self->config.app, socket);
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
    ov_vocs_api_authentication *self = ov_vocs_api_authentication_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    const char *client_id =
        ov_json_string_get(ov_json_object_get(input, OV_KEY_CLIENT));

    const char *session_id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION));

    const char *user_id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!client_id || !session_id || !user_id) {

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (!ov_event_session_update(
            self->config.user_sessions, client_id, user_id, session_id)) {

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    out = ov_json_object();
    if (!ov_vocs_json_set_id(
            out, ov_event_session_get_user(self->config.user_sessions, client_id)))
        goto error;

    if (!ov_vocs_json_set_session_id(out, session_id)) goto error;

    result = ov_vocs_app_send_success_response(self->config.app, input, socket, &out);
    return true;

    /* close socket if authentication messaging failed */
error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    return result;
}

/*----------------------------------------------------------------------------*/

static bool register_events(ov_vocs_api_authentication *self){

    if (!self) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_KEY_LOGIN,
                                  self,
                                  client_login)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_EVENT_API_AUTHENTICATE,
                                  self,
                                  client_login)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_KEY_LOGOUT,
                                  self,
                                  client_logout)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_EVENT_API_UPDATE_LOGIN,
                                  self,
                                  update_client_login)) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool init_config(ov_vocs_api_authentication_config *config){

    if (!config) goto error;
    if (!config->loop) goto error;
    if (!config->app) goto error;
    if (!config->engine) goto error;
    if (!config->user_sessions) goto error;
    if (!config->connections) goto error;
    if (!config->async) goto error;
    if (!config->broadcasts) goto error;

    return true;
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vocs_api_authentication *ov_vocs_api_authentication_create(ov_vocs_api_authentication_config config){

    ov_vocs_api_authentication *self = NULL;
    if (!init_config(&config))goto error;

    self = calloc(1, sizeof(ov_vocs_api_authentication));
    if (!self) goto error;

    self->magic_byte = OV_VOCS_API_AUTHENTICATION_MAGIC_BYTES;
    self->config = config;

    if (!register_events(self)) goto error;

    return self;
error:
    ov_vocs_api_authentication_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_api_authentication *ov_vocs_api_authentication_free(ov_vocs_api_authentication *self){

    if (!ov_vocs_api_authentication_cast(self)) return self;

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_api_authentication *ov_vocs_api_authentication_cast(const void *self){

    if (!self) goto error;

    if (*(uint16_t *)self == OV_VOCS_API_AUTHENTICATION_MAGIC_BYTES) 
        return (ov_vocs_api_authentication *)self;
error:
    return NULL;

}