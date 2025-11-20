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
        @file           ov_vocs_api_client.c
        @author         Töpfer, Markus

        @date           2025-11-13


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_api_client.h"
#include "../include/ov_vocs_loop.h"

#include <ov_base/ov_event_keys.h>

#include <ov_vocs_db/ov_vocs_db_app.h>

/*----------------------------------------------------------------------------*/

#define OV_VOCS_API_CLIENT_MAGIC_BYTES 0xa002

/*----------------------------------------------------------------------------*/

struct ov_vocs_api_client {

    uint16_t magic_bytes;
    ov_vocs_api_client_config config;

};

/*----------------------------------------------------------------------------*/

static void async_timedout(void *userdata, ov_event_async_data data) {

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);
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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);
    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

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

        ov_vocs_app_send_error_response(self->config.app,
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

                ov_vocs_app_send_error_response(self->config.app,
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

                ov_vocs_app_send_error_response(self->config.app,
                                    input,
                                    socket,
                                    OV_ERROR_CODE_PARAMETER_ERROR,
                                    OV_ERROR_DESC_PARAMETER_ERROR);

                goto error;
            }

            if (!session) {

                ov_vocs_app_send_error_response(self->config.app,
                                    input,
                                    socket,
                                    OV_ERROR_CODE_SESSION_UNKNOWN,
                                    OV_ERROR_DESC_SESSION_UNKNOWN);

                goto error;
            }



            if (!ov_vocs_core_session_update(
                    self->config.core, uuid, session, type, sdp)) {

                ov_vocs_app_send_error_response(self->config.app,
                                    input,
                                    socket,
                                    OV_ERROR_CODE_PROCESSING_ERROR,
                                    OV_ERROR_DESC_PROCESSING_ERROR);

                goto error;
            }

            break;

        default:

            ov_vocs_app_send_error_response(self->config.app,
                                input,
                                socket,
                                OV_ERROR_CODE_PARAMETER_ERROR,
                                OV_ERROR_DESC_PARAMETER_ERROR);

            goto error;
    }

    if (!ov_event_async_set(
            self->config.async,
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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);
    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *session =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));

    if (!user) goto error;

    const char *uuid = ov_event_api_get_uuid(input);
    ov_json_value *par = ov_event_api_get_parameter(input);
    ov_ice_candidate_info info = ov_ice_candidate_info_from_json(par);

    if (!uuid || !info.candidate || !info.ufrag) {

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (NULL == session) {

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_SESSION_UNKNOWN,
                            OV_ERROR_DESC_SESSION_UNKNOWN);

        goto error;
    }

    if (!ov_vocs_core_candidate(self->config.core, uuid, session, &info)) {

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    if (!ov_event_async_set(
            self->config.async,
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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);
    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *session =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));

    if (!user) goto error;

    const char *uuid = ov_event_api_get_uuid(input);

    if (!uuid) {

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (NULL == session) {

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_SESSION_UNKNOWN,
                            OV_ERROR_DESC_SESSION_UNKNOWN);

        goto error;
    }

    if (!ov_vocs_core_end_of_candidates(self->config.core, uuid, session)) {

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    if (!ov_event_async_set(
            self->config.async,
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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user) goto error;

    if (role) {

        ov_vocs_app_send_error_response(self->config.app,
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

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    /* process authorization request */

    if (!ov_vocs_db_authorize(self->config.db, user, role)) {

        ov_vocs_app_send_error_response(
            self->config.app, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    ov_json_object_set(data, OV_KEY_ROLE, ov_json_string(role));

    if (!ov_broadcast_registry_set(
            self->config.broadcasts, role, socket, OV_ROLE_BROADCAST)) {
        goto error;
    }

    out = ov_json_object();
    if (!ov_vocs_json_set_id(out, role)) goto error;

    result = ov_vocs_app_send_success_response(self->config.app, input, socket, &out);
    if (result) {
        ov_log_info("VOCS AUTHORIZE at %i | %s | %s", socket, user, role);
    } else {
        ov_log_error("VOCS AUTHORIZE failed at %i | %s", socket, user);
    }

    ov_socket_json_set(self->config.connections, socket, &data);

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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);
    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) {

        ov_vocs_app_send_error_response(
            self->config.app, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    ov_json_value *parameter = ov_event_api_get_parameter(input);
    const char *type = ov_event_api_get_type(parameter);

    if (!parameter || !type) {

        ov_vocs_app_send_error_response(self->config.app,
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
        result = ov_vocs_app_send_success_response(self->config.app, input, socket, &out);

    } else {

        ov_vocs_app_send_error_response(self->config.app,
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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) goto error;

    val = ov_vocs_db_get_user_roles(self->config.db, user);

    if (!val) {

        ov_vocs_app_send_error_response(
            self->config.app, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    out = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_ROLES, val)) goto error;

    val = NULL;

    result = ov_vocs_app_send_success_response(self->config.app, input, socket, &out);

error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return result;
}

/*----------------------------------------------------------------------------*/

struct container_loop_broadcasts {

    ov_vocs_api_client *vocs;
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
        c->vocs->config.broadcasts, name, c->socket, OV_LOOP_BROADCAST);
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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        ov_vocs_app_send_error_response(
            self->config.app, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    val = ov_vocs_db_get_user_role_loops(self->config.db, user, role);

    if (!val) {

        ov_vocs_app_send_error_response(
            self->config.app, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    struct container_loop_broadcasts c = (struct container_loop_broadcasts){
        .vocs = self, .socket = socket, .user = user, .role = role};

    if (!ov_json_object_for_each(val, &c, add_loop_broadcast_and_state))
        goto error;

    out = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_LOOPS, val)) goto error;

    val = NULL;

    result = ov_vocs_app_send_success_response(self->config.app, input, socket, &out);

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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *sess =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        ov_vocs_app_send_error_response(
            self->config.app, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    if (NULL == sess) {

        ov_vocs_app_send_error_response(self->config.app,
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

        ov_vocs_app_send_error_response(self->config.app,
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

        ov_vocs_app_send_error_response(self->config.app,
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

        ov_vocs_loop *l = ov_dict_get(self->config.loops, loop);

        if (!l) {

            l = ov_vocs_loop_create(loop);
            char *name = ov_string_dup(loop);

            if (!ov_dict_set(self->config.loops, name, l, NULL)) {
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

        ov_vocs_app_send_success_response(self->config.app, input, socket, &out);

        out = ov_json_value_free(out);
        input = ov_json_value_free(input);

        goto done;
    }

    if (!ov_vocs_core_perform_switch_loop_request(
            self->config.core, uuid, sess, user, role, loop, current, requested)) {

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    /* we requested either some loop aquisition or switch loop */

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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));
    const char *sess =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));

    if (!user || !role) {

        ov_vocs_app_send_error_response(
            self->config.app, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    uint64_t percent = ov_json_number_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_VOLUME));

    if (!uuid || !loop || (percent > 100)) {

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (NULL == sess) {

        ov_vocs_app_send_error_response(self->config.app,
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

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

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

    ov_vocs_app_send_switch_volume_user_broadcast(self->config.app, socket, loop, percent);

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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        ov_vocs_app_send_error_response(
            self->config.app, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

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

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    ov_vocs_permission permission =
        ov_vocs_db_get_permission(self->config.db, role, loop);

    if (permission != OV_VOCS_SEND) {

        ov_vocs_app_send_error_response(self->config.app,
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

    ov_vocs_app_send_success_response(self->config.app, input, socket, &out);

    ov_vocs_app_send_talking_loop_broadcast(
            self->config.app, socket, params, loop, state, client_id);

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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) {

        ov_vocs_app_send_error_response(
            self->config.app, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

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

    ov_vocs_app_send_success_response(self->config.app, input, socket, &out);

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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        ov_vocs_app_send_error_response(
            self->config.app, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

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

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (!ov_vocs_db_sip_allow_callout(self->config.db, loop, role)) {

        ov_vocs_app_send_error_response(
            self->config.app, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    uuid_request = ov_vocs_core_sip_create_call(self->config.core,
        loop, dest, from);

    if (!uuid_request) {

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);
        goto error;
    }

    if (!ov_event_async_set(
            self->config.async,
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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        ov_vocs_app_send_error_response(
            self->config.app, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    const char *call_id = ov_json_string_get(
        (ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_CALL)));
    const char *loop = ov_json_string_get(
        (ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP)));

    if (!uuid || !call_id) {

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (!ov_vocs_db_sip_allow_callend(self->config.db, loop, role)) {

        ov_vocs_app_send_error_response(
            self->config.app, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    if (ov_vocs_core_sip_terminate_call(self->config.core, call_id)){

        ov_vocs_app_send_success_response(self->config.app, input, socket, NULL);

    } else {

        ov_vocs_app_send_error_response(self->config.app,
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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        ov_vocs_app_send_error_response(
            self->config.app, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    bool ok = true;

    const char *uuid = ov_event_api_get_uuid(input);
    ov_sip_permission permission = ov_sip_permission_from_json(
        ov_json_get(input, "/" OV_KEY_PARAMETER), &ok);

    if (!uuid || !ok) {

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (ov_vocs_core_sip_create_permission(self->config.core, permission)) {

        ov_vocs_app_send_success_response(self->config.app, input, socket, NULL);

    } else {

        ov_vocs_app_send_error_response(self->config.app,
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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        ov_vocs_app_send_error_response(
            self->config.app, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    bool ok = true;

    const char *uuid = ov_event_api_get_uuid(input);
    ov_sip_permission permission = ov_sip_permission_from_json(
        ov_json_get(input, "/" OV_KEY_PARAMETER), &ok);

    if (!uuid || !ok) {

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (ov_vocs_core_sip_terminate_permission(self->config.core, permission)) {

        ov_vocs_app_send_success_response(self->config.app, input, socket, NULL);

    } else {

        ov_vocs_app_send_error_response(self->config.app,
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

static bool client_call_function(ov_vocs_api_client *self,
                                 int socket,
                                 const ov_event_parameter *params,
                                 ov_json_value *input,
                                 bool (*function)(ov_vocs_core *core,
                                                  const char *uuid)) {

    UNUSED(params);

    ov_json_value *data = NULL;

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));
    const char *uuid = ov_event_api_get_uuid(input);

    if (!user || !role || !uuid) {

        ov_vocs_app_send_error_response(
            self->config.app, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

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

    input = NULL;

    if (!function(self->config.core, uuid)) {

        ov_event_async_data adata = ov_event_async_unset(self->config.async, uuid);

        ov_vocs_app_send_error_response(self->config.app,
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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    return client_call_function(
        self, socket, params, input, ov_vocs_core_sip_list_calls);
}

/*----------------------------------------------------------------------------*/

static bool client_list_call_permissions(void *userdata,
                                         int socket,
                                         const ov_event_parameter *params,
                                         ov_json_value *input) {
    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    return client_call_function(
        self, socket, params, input, ov_vocs_core_sip_list_call_permissions);
}

/*----------------------------------------------------------------------------*/

static bool client_list_sip_status(void *userdata,
                                   int socket,
                                   const ov_event_parameter *params,
                                   ov_json_value *input) {

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);
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
        (ov_event_parameter){.send.instance = self, .send.send = ov_vocs_app_send};

    if (!ov_broadcast_registry_send(
            self->config.broadcasts, user, &parameter, out, OV_USER_BROADCAST))
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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    if (!self || !input || socket < 0) goto error;

    data = ov_socket_json_get(self->config.connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user) {

        ov_vocs_app_send_error_response(
            self->config.app, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

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

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

    } else if (OV_DB_RECORDINGS_RESULT_TOO_BIG == val) {

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            "Search returned too many results - please confine "
                            "your search parameters");

    } else {
        ov_vocs_app_send_success_response(self->config.app, input, socket, &val);
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

    ov_vocs_api_client *self = ov_vocs_api_client_cast(userdata);

    if (!self || !socket || !params || !input) goto error;

    if (!ov_broadcast_registry_set(self->config.broadcasts,
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


/*----------------------------------------------------------------------------*/

static bool register_events(ov_vocs_api_client *self){

    if (!ov_event_engine_register(self->config.engine,
                                  OV_EVENT_API_MEDIA,
                                  self,
                                  client_media)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_ICE_STRING_CANDIDATE,
                                  self,
                                  client_candidate)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_ICE_STRING_END_OF_CANDIDATES,
                                  self,
                                  client_end_of_candidates)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_EVENT_API_AUTHORISE,
                                  self,
                                  client_authorize)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_EVENT_API_AUTHORIZE,
                                  self,
                                  client_authorize)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_EVENT_API_GET,
                                  self,
                                  client_get)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_EVENT_API_USER_ROLES,
                                  self,
                                  client_user_roles)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_EVENT_API_ROLE_LOOPS,
                                  self,
                                  client_role_loops)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_EVENT_API_SWITCH_LOOP_STATE,
                                  self,
                                  client_switch_loop_state)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_EVENT_API_SWITCH_LOOP_VOLUME,
                                  self,
                                  client_switch_loop_volume)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_EVENT_API_TALKING,
                                  self,
                                  client_talking)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_KEY_CALL,
                                  self,
                                  client_calling)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_KEY_HANGUP,
                                  self,
                                  client_hangup)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_KEY_PERMIT_CALL,
                                  self,
                                  client_permit_call)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_KEY_REVOKE_CALL,
                                  self,
                                  client_revoke_call)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_KEY_LIST_CALLS,
                                  self,
                                  client_list_calls)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_KEY_LIST_CALL_PERMISSIONS,
                                  self,
                                  client_list_call_permissions)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_KEY_LIST_SIP_STATUS,
                                  self,
                                  client_list_sip_status)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_SET_KEYSET_LAYOUT,
                                  self,
                                  client_event_set_keyset_layout)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_GET_KEYSET_LAYOUT,
                                  self,
                                  client_event_get_keyset_layout)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_SET_USER_DATA,
                                  self,
                                  client_event_set_user_data)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_GET_USER_DATA,
                                  self,
                                  client_event_get_user_data)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_KEY_GET_RECORDING,
                                  self,
                                  client_event_get_recording)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_KEY_SIP,
                                  self,
                                  client_event_sip_status)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_KEY_REGISTER,
                                  self,
                                  client_event_register)) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool init_config(ov_vocs_api_client_config *config){

    if (!config) goto error;
    if (!config->loop) goto error;
    if (!config->app) goto error;
    if (!config->core) goto error;
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

ov_vocs_api_client *ov_vocs_api_client_create(ov_vocs_api_client_config config){

    ov_vocs_api_client *self = NULL;
    
    if (!init_config(&config)) goto error;

    self = calloc(1, sizeof(ov_vocs_api_client));
    if (!self) goto error;

    self->magic_bytes = OV_VOCS_API_CLIENT_MAGIC_BYTES;
    self->config = config;

    if (!register_events(self)) goto error;

    return self;
error:
    ov_vocs_api_client_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_api_client *ov_vocs_api_client_free(ov_vocs_api_client *self){

    if (!ov_vocs_api_client_cast(self)) return self;

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_api_client *ov_vocs_api_client_cast(const void *self){

    if (!self) goto error;

    if (*(uint16_t *)self == OV_VOCS_API_CLIENT_MAGIC_BYTES) 
        return (ov_vocs_api_client *)self;
error:
    return NULL;
}