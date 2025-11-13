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
        @file           ov_vocs_api_admin.c
        @author         Töpfer, Markus

        @date           2025-11-13


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_api_admin.h"

#include <ov_base/ov_event_keys.h>

#include <ov_vocs_db/ov_vocs_db_app.h>

/*----------------------------------------------------------------------------*/

#define OV_VOCS_API_ADMIN_MAGIC_BYTES 0xa003

/*----------------------------------------------------------------------------*/

struct ov_vocs_api_admin {

    uint16_t magic_bytes;
    ov_vocs_api_admin_config config;

};


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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);
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

    ov_vocs_app_send(self->config.app, socket, out);

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

    ov_vocs_api_admin *vocs;
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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!user){

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        ov_vocs_app_send(self->config.app, socket, out);
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

    bool result = ov_socket_json_for_each(self->config.connections, &container, 
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

static bool check_if_admin_is_logged_in(ov_vocs_api_admin *self, int socket){

    ov_json_value *data = NULL;

    data = ov_socket_json_get(self->config.connections, socket);
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
    
    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !params || !input) goto error;

    if (!check_if_admin_is_logged_in(self, socket)){

        out = ov_event_api_create_error_response(input, 
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    val = ov_json_object();

    if (!ov_socket_json_for_each_set_data(self->config.connections, val)) goto error;

    out = ov_event_api_create_success_response(input);
    res = ov_event_api_get_response(out);

    if (!ov_json_object_set(res, OV_KEY_CONNECTIONS, val)) goto error;

response:
    ov_vocs_app_send(self->config.app, socket, out);

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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
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

    ov_vocs_app_send(self->config.app, socket, out);

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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);

    if (!self || !input) goto error;

    if (!check_if_admin_is_logged_in(self, socket)){

        ov_vocs_app_send_error_response(self->config.app,
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

        ov_vocs_app_send_error_response(self->config.app,
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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
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

    ov_vocs_app_send(self->config.app, socket, out);

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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);

    if (!self || !input) goto error;

    if (!check_if_admin_is_logged_in(self, socket)){

        ov_vocs_app_send_error_response(self->config.app,
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

        ov_vocs_app_send_error_response(self->config.app,
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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);

    if (!self || !input) goto error;

    if (!check_if_admin_is_logged_in(self, socket)){

        ov_vocs_app_send_error_response(self->config.app,
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

    ov_vocs_app_send(self->config.app, socket, out);

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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);

    if (!self || !input) goto error;

    if (!check_if_admin_is_logged_in(self, socket)){

        ov_vocs_app_send_error_response(self->config.app,
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

    ov_vocs_app_send(self->config.app, socket, out);

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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);

    if (!self || !input) goto error;

    if (!check_if_admin_is_logged_in(self, socket)){

        ov_vocs_app_send_error_response(self->config.app,
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
        (ov_event_parameter){.send.instance = self, .send.send = ov_vocs_app_send};

    if (!ov_broadcast_registry_send(
            self->config.broadcasts, OV_BROADCAST_KEY_SYSTEM_BROADCAST, &parameter, input, 
            OV_SYSTEM_BROADCAST)) {

        ov_vocs_app_send_error_response(self->config.app,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);
    } else {

        ov_vocs_app_send_success_response(self->config.app, input, socket, NULL);
        
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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);
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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    if (!check_if_admin_is_logged_in(self, socket)){

        ov_vocs_app_send_error_response(self->config.app,
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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    if (!check_if_admin_is_logged_in(self, socket)){

        ov_vocs_app_send_error_response(self->config.app,
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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
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

static bool authorize_connection_for_id(ov_vocs_api_admin *self,
                                        int socket,
                                        ov_vocs_db_entity entity,
                                        const char *id) {

    ov_vocs_db_parent parent = {0};
    ov_json_value *data = NULL;

    if (!self || !socket || !id) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    udata = ov_socket_json_get(self->config.connections, socket);

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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

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


    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

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

    ov_vocs_api_admin *self = ov_vocs_api_admin_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    data = ov_socket_json_get(self->config.connections, socket);

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

/*----------------------------------------------------------------------------*/

static bool register_events(ov_vocs_api_admin *self){

    if (!self) goto error;
    
    if (!ov_event_engine_register(self->config.engine,
                                  OV_EVENT_API_STATE_MIXER,
                                  self,
                                  get_mixer_count)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  "get_mixer_state",
                                  self,
                                  get_mixer_state)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_EVENT_API_STATE_CONNECTIONS,
                                  self,
                                  get_connection_state)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_EVENT_START_RECORD,
                                  self,
                                  start_recording)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_EVENT_STOP_RECORD,
                                  self,
                                  stop_recording)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  "get_recorded_loops",
                                  self,
                                  get_recorded_loops)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  "get_all_loops",
                                  self,
                                  get_all_loops)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_KEY_BROADCAST,
                                  self,
                                  broadcast)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_UPDATE_PASSWORD,
                                  self,
                                  cb_event_update_password)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_ADMIN_DOMAINS,
                                  self,
                                  cb_event_get_admin_domains)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_ADMIN_PROJECTS,
                                  self,
                                  cb_event_get_admin_projects)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_ID_EXISTS,
                                  self,
                                  cb_event_check_id_exists)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_DELETE,
                                  self,
                                  cb_event_delete)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_CREATE,
                                  self,
                                  cb_event_create)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_GET_KEY,
                                  self,
                                  cb_event_get_key)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_UPDATE_KEY,
                                  self,
                                  cb_event_update_key)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_DELETE_KEY,
                                  self,
                                  cb_event_delete_key)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_VERIFY,
                                  self,
                                  cb_event_verify)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_UPDATE,
                                  self,
                                  cb_event_update)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_LOAD,
                                  self,
                                  cb_event_load)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_SAVE,
                                  self,
                                  cb_event_save)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_GET_LAYOUT,
                                  self,
                                  cb_event_get_layout)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_SET_LAYOUT,
                                  self,
                                  cb_event_set_layout)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_LOAD,
                                  self,
                                  cb_event_load)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_ADD_DOMAIN_ADMIN,
                                  self,
                                  cb_event_add_domain_admin)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_ADD_PROJECT_ADMIN,
                                  self,
                                  cb_event_add_project_admin)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_LDAP_IMPORT,
                                  self,
                                  cb_event_ldap_import)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_SET_KEYSET_LAYOUT,
                                  self,
                                  cb_event_set_keyset_layout)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_GET_KEYSET_LAYOUT,
                                  self,
                                  cb_event_get_keyset_layout)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_SET_USER_DATA,
                                  self,
                                  cb_event_set_user_data)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  OV_VOCS_DB_GET_USER_DATA,
                                  self,
                                  cb_event_get_user_data)) goto error;

    if (!ov_event_engine_register(self->config.engine,
                                  "get_highest_port",
                                  self,
                                  cb_event_get_highest_port)) goto error;


    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool init_config(ov_vocs_api_admin_config *config){

    if (!config) goto error;
    if (!config->loop) goto error;
    if (!config->app) goto error;
    if (!config->core) goto error;
    if (!config->db) goto error;
    if (!config->persistance) goto error;
    if (!config->loops) goto error;
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

ov_vocs_api_admin *ov_vocs_api_admin_create(ov_vocs_api_admin_config config){

    ov_vocs_api_admin *self = NULL;
    
    if (!init_config(&config)) goto error;

    self = calloc(1, sizeof(ov_vocs_api_admin));
    if (!self) goto error;

    self->magic_bytes = OV_VOCS_API_ADMIN_MAGIC_BYTES;
    self->config = config;

    if (!register_events(self)) goto error;

    return self;
error:
    ov_vocs_api_admin_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_api_admin *ov_vocs_api_admin_free(ov_vocs_api_admin *self){

    if (!ov_vocs_api_admin_cast(self)) return self;

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_api_admin *ov_vocs_api_admin_cast(const void *self){

    if (!self) goto error;

    if (*(uint16_t *)self == OV_VOCS_API_ADMIN_MAGIC_BYTES) 
        return (ov_vocs_api_admin *)self;
error:
    return NULL;
}