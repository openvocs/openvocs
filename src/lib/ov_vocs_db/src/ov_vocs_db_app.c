/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_vocs_db_app.c
        @author         Markus Töpfer

        @date           2023-11-16


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_db_app.h"

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_error_codes.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_string.h>

#include <ov_core/ov_event_api.h>
#include <ov_core/ov_event_async.h>
#include <ov_core/ov_event_engine.h>
#include <ov_core/ov_event_io.h>
#include <ov_core/ov_event_session.h>
#include <ov_core/ov_broadcast_registry.h>

/*----------------------------------------------------------------------------*/

#define OV_VOCS_DB_APP_MAGIC_BYTES 0xdbaa

/*----------------------------------------------------------------------------*/

#define IMPL_DEFAULT_LDAP_TIMEOUT_USEC 5000 * 1000

/*----------------------------------------------------------------------------*/

typedef struct Connection {

    int socket;
    ov_json_value *data;

} Connection;

/*----------------------------------------------------------------------------*/

struct ov_vocs_db_app {

    uint16_t magic_bytes;
    ov_vocs_db_app_config config;

    ov_event_async_store *async;
    ov_event_engine *engine;
    ov_event_session *user_sessions;

    ov_dict *connections;

    ov_ldap *ldap;

    ov_broadcast_registry *broadcasts;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      CONNECTION
 *
 *      ------------------------------------------------------------------------
 */

static void *connection_free(void *connection) {

    Connection *c = (Connection *)connection;
    if (!c) goto done;

    c->data = ov_json_value_free(c->data);
    c = ov_data_pointer_free(c);
done:
    return c;
}

/*----------------------------------------------------------------------------*/

static bool drop_connection(ov_vocs_db_app *self, int socket) {

    if (!self || !socket) goto error;

    intptr_t key = socket;
    ov_dict_del(self->connections, (void *)key);

    if (self->config.env.close)
        self->config.env.close(self->config.env.userdata, socket);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static Connection *connection_create(ov_vocs_db_app *self,
                                     int socket,
                                     const char *user,
                                     const char *client) {

    if (!self || !socket || !user) goto error;

    Connection *c = calloc(1, sizeof(Connection));
    if (!c) goto error;

    c->data = ov_json_object();
    ov_json_value *u = ov_json_string(user);
    ov_json_object_set(c->data, OV_KEY_USER, u);

    ov_json_value *co = ov_json_string(client);
    ov_json_object_set(c->data, OV_KEY_CLIENT, co);

    intptr_t key = socket;
    ov_dict_set(self->connections, (void *)key, c, NULL);
    return c;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static Connection *get_connection(ov_vocs_db_app *self, int socket) {

    if (!self || !socket) goto error;

    intptr_t key = socket;
    return ov_dict_get(self->connections, (void *)key);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static const char *get_connection_user(Connection *c) {

    if (!c) goto error;
    return ov_json_string_get(ov_json_get(c->data, "/" OV_KEY_USER));
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static const char *get_connection_client(Connection *c) {

    if (!c) goto error;
    return ov_json_string_get(ov_json_get(c->data, "/" OV_KEY_CLIENT));
error:
    return NULL;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      AUTHORIZATION
 *
 *      ------------------------------------------------------------------------
 */

static bool authorize_connection_for_id(ov_vocs_db_app *app,
                                        int socket,
                                        ov_vocs_db_entity entity,
                                        const char *id) {

    ov_vocs_db_parent parent = {0};

    if (!app || !socket || !id) goto error;

    Connection *conn = get_connection(app, socket);
    const char *user = get_connection_user(conn);

    if (!user) goto error;

    parent = ov_vocs_db_get_parent(app->config.db, entity, id);

    if (NULL == parent.id) goto error;

    switch (parent.scope) {

        case OV_VOCS_DB_SCOPE_DOMAIN:

            if (!ov_vocs_db_authorize_domain_admin(
                    app->config.db, user, parent.id)) {
                goto error;
            }

            break;

        case OV_VOCS_DB_SCOPE_PROJECT:

            if (!ov_vocs_db_authorize_project_admin(
                    app->config.db, user, parent.id)) {
                goto error;
            }

            break;
    }

    ov_vocs_db_parent_clear(&parent);
    return true;
error:
    ov_vocs_db_parent_clear(&parent);
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      APP EVENTS
 *
 *      ------------------------------------------------------------------------
 */

static void async_timedout(void *userdata, ov_event_async_data data) {

    ov_vocs_db_app *self = ov_vocs_db_app_cast(userdata);
    if (!self) goto error;

    char *str = ov_json_value_to_string(data.value);
    ov_log_error("Async timeout - dropping %i | %s", data.socket, str);
    str = ov_data_pointer_free(str);

    ov_json_value *out = ov_event_api_create_error_response(
        data.value, OV_ERROR_TIMEOUT, OV_ERROR_DESC_TIMEOUT);
    ov_event_io_send(&data.params, data.socket, out);
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
    ov_event_async_data data = {0};

    ov_vocs_db_app *self = ov_vocs_db_app_cast(userdata);
    OV_ASSERT(self);
    if (!self) goto error;

    data = ov_event_async_unset(self->async, uuid);
    if (!data.value) goto error;

    const char *session_id = NULL;

    const char *user = ov_json_string_get(
        ov_json_get(data.value, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    const char *client =
        ov_json_string_get(ov_json_get(data.value, "/" OV_KEY_CLIENT));

    switch (result) {

        case OV_LDAP_AUTH_REJECTED:

            ov_log_error(
                "LDAP AUTHENTICATE failed at %i | %s", data.socket, user);

            out = ov_event_api_create_error_response(
                data.value, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

            ov_event_io_send(&data.params, data.socket, out);
            out = ov_json_value_free(out);

            drop_connection(self, data.socket);
            goto error;
            break;

        case OV_LDAP_AUTH_GRANTED:
            break;
    }

    /* successful authenticated */

    Connection *conn = connection_create(self, data.socket, user, client);
    if (!conn) goto error;

    session_id = ov_event_session_init(self->user_sessions, client, user);

    out = ov_event_api_create_success_response(data.value);
    ov_json_value *res = ov_event_api_get_response(out);
    if (!ov_vocs_json_set_id(res, user)) goto error;
    if (!ov_vocs_json_set_session_id(res, session_id)) goto error;

    ov_event_io_send(&data.params, data.socket, out);
    out = ov_json_value_free(out);

    ov_log_info("authenticated %s", user);

    if (!ov_broadcast_registry_set(self->broadcasts,
                                   OV_BROADCAST_KEY_SYSTEM_BROADCAST,
                                   data.socket,
                                   OV_SYSTEM_BROADCAST)) {
        goto error;
    }

error:
    ov_json_value_free(out);
    ov_event_async_data_clear(&data);
    return;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_login(void *userdata,
                           const int socket,
                           const ov_event_parameter *params,
                           ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    const char *session_id = NULL;

    const char *uuid = ov_event_api_get_uuid(input);
    const char *user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));
    const char *pass = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD));
    const char *client =
        ov_json_string_get(ov_json_get(input, "/" OV_KEY_CLIENT));

    const char *session_user =
        ov_event_session_get_user(app->user_sessions, client);

    if (session_user && user && pass) {

        if (!ov_event_session_verify(app->user_sessions, client, user, pass)) {

            out = ov_event_api_create_error_response(
                input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

            goto response;
        }

        goto login_session_user;
    }

    if (!user || !pass || !client) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (app->config.ldap.enable) {

        if (!ov_event_async_set(
                app->async,
                uuid,
                (ov_event_async_data){.socket = socket,
                                      .value = input,
                                      .params = *params,
                                      .timedout.userdata = app,
                                      .timedout.callback = async_timedout},
                app->config.ldap.timeout_usec)) {

            goto error;
        }

        return ov_ldap_authenticate_password(
            app->ldap,
            user,
            pass,
            uuid,
            (ov_ldap_auth_callback){
                .userdata = app, .callback = cb_ldap_authentication});
    }

    /* authenticate user in db */

    if (!ov_vocs_db_authenticate(app->config.db, user, pass)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

login_session_user:

    session_id = ov_event_session_init(app->user_sessions, client, user);

    Connection *c = connection_create(app, socket, user, client);
    if (!c) goto error;

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    if (!ov_vocs_json_set_id(res, user)) goto error;
    if (!ov_vocs_json_set_session_id(res, session_id)) goto error;

    if (!ov_broadcast_registry_set(app->broadcasts,
                                   OV_BROADCAST_KEY_SYSTEM_BROADCAST,
                                   socket,
                                   OV_SYSTEM_BROADCAST)) {
        goto error;
    }

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

static bool cb_update_client_login(void *userdata,
                                   const int socket,
                                   const ov_event_parameter *params,
                                   ov_json_value *input) {

    bool result = false;
    ov_json_value *out = NULL;

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);

    if (!app || !input || socket < 0) goto error;

    const char *client_id =
        ov_json_string_get(ov_json_object_get(input, OV_KEY_CLIENT));

    const char *session_id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION));

    const char *user_id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!client_id || !session_id || !user_id) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!ov_event_session_update(
            app->user_sessions, client_id, user_id, session_id)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    if (!ov_vocs_json_set_id(res, user_id)) goto error;
    if (!ov_vocs_json_set_session_id(res, session_id)) goto error;

response:
    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);

    return true;

    /* close socket if authentication messaging failed */
error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    return result;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_logout(void *userdata,
                            const int socket,
                            const ov_event_parameter *params,
                            ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    out = ov_event_api_create_success_response(input);
    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);

    Connection *conn = get_connection(app, socket);
    if (!conn) goto error;

    ov_event_session_delete(app->user_sessions, get_connection_client(conn));

    drop_connection(app, socket);
    input = ov_json_value_free(input);
    return true;
error:
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_update_password(void *userdata,
                                     const int socket,
                                     const ov_event_parameter *params,
                                     ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    if (!conn) goto error;

    const char *conn_user = get_connection_user(conn);

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

    if (app->config.ldap.enable) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH_LDAP_USED, OV_ERROR_DESC_AUTH_LDAP_USED);

        goto response;
    }

    if (0 == ov_string_compare(user, conn_user)) {

        /* each user is allowed to update it's own password */

        if (!ov_vocs_db_set_password(app->config.db, user, pass)) {

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
            ov_vocs_db_get_parent(app->config.db, OV_VOCS_DB_USER, user);

        switch (parent.scope) {

            case OV_VOCS_DB_SCOPE_DOMAIN:

                if (!ov_vocs_db_authorize_domain_admin(
                        app->config.db, conn_user, parent.id)) {

                    out = ov_event_api_create_error_response(
                        input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

                    ov_vocs_db_parent_clear(&parent);

                    goto response;
                }

                break;

            case OV_VOCS_DB_SCOPE_PROJECT:

                if (!ov_vocs_db_authorize_project_admin(
                        app->config.db, conn_user, parent.id)) {

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

        if (!ov_vocs_db_set_password(app->config.db, user, pass)) {

            out = ov_event_api_create_error_response(
                input,
                OV_ERROR_CODE_PROCESSING_ERROR,
                OV_ERROR_DESC_PROCESSING_ERROR);

            goto response;
        }

        out = ov_event_api_create_success_response(input);
        goto response;
    }

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

static bool cb_event_get_admin_domains(void *userdata,
                                       const int socket,
                                       const ov_event_parameter *params,
                                       ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    if (!conn) goto error;
    const char *conn_user = get_connection_user(conn);

    const char *user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!user) user = conn_user;

    /* all users are allowed to send the request */

    val = ov_vocs_db_get_admin_domains(app->config.db, user);
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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    if (!conn) goto error;
    const char *conn_user = get_connection_user(conn);

    const char *user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!user) user = conn_user;

    /* all users are allowed to send the request */

    val = ov_vocs_db_get_admin_projects(app->config.db, user);
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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    if (!conn) goto error;

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

        if (ov_vocs_db_check_entity_id_exists(app->config.db, entity, id)) {
            val = ov_json_true();
        } else {
            val = ov_json_false();
        }

    } else {

        /* check for unique ID */

        if (ov_vocs_db_check_id_exists(app->config.db, id)) {
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

static bool cb_event_get(void *userdata,
                         const int socket,
                         const ov_event_parameter *params,
                         ov_json_value *input) {

    /* checking input:

       {
           "event" : "get",
           "parameter" :
           {
               "type"     : "<domain | project | user | role | loop >",
               "id" : "<id to get>",
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    if (!conn) goto error;

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

    val = ov_vocs_db_get_entity(app->config.db, entity, id);
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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    if (!conn) goto error;

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

    if (!authorize_connection_for_id(app, socket, entity, id)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_delete_entity(app->config.db, entity, id)) {

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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    const char *user = get_connection_user(conn);
    if (!conn || !user) goto error;

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
                app->config.db, user, scope_id)) {

            out = ov_event_api_create_error_response(
                input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

            goto response;
        }

    } else if (!ov_vocs_db_authorize_project_admin(
                   app->config.db, user, scope_id)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_create_entity(
            app->config.db, entity, id, db_scope, scope_id)) {

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
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    if (!conn) goto error;

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

    val = ov_vocs_db_get_entity_key(app->config.db, entity, id, key);
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
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    if (!conn) goto error;

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

    if (!authorize_connection_for_id(app, socket, entity, id)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_update_entity_key(app->config.db, entity, id, key, data)) {

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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    if (!conn) goto error;

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

    if (!authorize_connection_for_id(app, socket, entity, id)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_delete_entity_key(app->config.db, entity, id, key)) {

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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    if (!conn) goto error;

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

    if (!authorize_connection_for_id(app, socket, entity, id)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_verify_entity_item(
            app->config.db, entity, id, data, &errors)) {

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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    if (!conn) goto error;
    const char *user = get_connection_user(conn);

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

        if (!ov_vocs_db_authorize_domain_admin(app->config.db, user, id)) {

            out = ov_event_api_create_error_response(
                input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

            goto response;
        }

    } else if (0 == strcmp(type, OV_KEY_PROJECT)) {

        if (!ov_vocs_db_authorize_project_admin(app->config.db, user, id)) {

            out = ov_event_api_create_error_response(
                input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

            goto response;
        }

    } else {

        if (!authorize_connection_for_id(app, socket, entity, id)) {

            out = ov_event_api_create_error_response(
                input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

            goto response;
        }
    }

    if (!ov_vocs_db_update_entity_item(
            app->config.db, entity, id, data, &errors)) {

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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    const char *user = get_connection_user(conn);
    if (!conn || !user) goto error;

    /* check if the user is some domain admin */
    val = ov_vocs_db_get_admin_domains(app->config.db, user);
    if (!val || (0 == ov_json_array_count(val))) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        val = ov_json_value_free(val);

        goto response;
    }
    val = ov_json_value_free(val);

    if (!ov_vocs_db_persistance_load(app->config.persistance)) {

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
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    const char *user = get_connection_user(conn);
    if (!conn || !user) goto error;

    /* check if the user is some domain admin */
    val = ov_vocs_db_get_admin_domains(app->config.db, user);
    if (!val || (0 == ov_json_array_count(val))) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        val = ov_json_value_free(val);

        goto response;
    }
    val = ov_json_value_free(val);

    if (!ov_vocs_db_persistance_save(app->config.persistance)) {

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
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    const char *user = get_connection_user(conn);
    if (!conn || !user) goto error;

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

    if (!authorize_connection_for_id(app, socket, OV_VOCS_DB_ROLE, id)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_set_layout(app->config.db, id, data)) {

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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    const char *user = get_connection_user(conn);
    if (!conn || !user) goto error;

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ROLE));

    if (!id) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    /* any user can get the layout of some role */

    val = ov_vocs_db_get_layout(app->config.db, id);
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
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    const char *user = get_connection_user(conn);
    if (!conn || !user) goto error;

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
    if (!ov_vocs_db_authorize_domain_admin(app->config.db, user, domain)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_add_domain_admin(app->config.db, domain, admin)) {

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
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    const char *user = get_connection_user(conn);
    if (!conn || !user) goto error;

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
    if (!ov_vocs_db_authorize_project_admin(app->config.db, user, project)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_add_project_admin(app->config.db, project, admin)) {

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
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    const char *user = get_connection_user(conn);
    if (!conn || !user) goto error;

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
    if (!ov_vocs_db_authorize_domain_admin(app->config.db, user, domain)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_persistance_ldap_import(app->config.persistance,
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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    const char *user = get_connection_user(conn);
    if (!conn || !user) goto error;

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

    if (!ov_vocs_db_authorize_domain_admin(app->config.db, user, domain)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_set_keyset_layout(app->config.db, domain, name, layout)) {

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
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    if (!conn) goto error;

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

    val = ov_vocs_db_get_keyset_layout(app->config.db, domain, layout);
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
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    if (!conn) goto error;
    const char *user = get_connection_user(conn);

    if (!user) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    ov_json_value *data = ov_event_api_get_parameter(input);
    if (!data) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!ov_vocs_db_set_user_data(app->config.db, user, data)) {

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
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
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

    ov_vocs_db_app *app = ov_vocs_db_app_cast(userdata);
    if (!app || !socket || !params || !input) goto error;

    Connection *conn = get_connection(app, socket);
    if (!conn) goto error;
    const char *user = get_connection_user(conn);

    if (!user) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    ov_json_value *data = ov_vocs_db_get_user_data(app->config.db, user);

    if (!data) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_DATA, data);

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

bool register_event_callbacks(ov_vocs_db_app *self) {

    if (!self) goto error;

    if (!ov_event_engine_register(
            self->engine, OV_EVENT_API_LOGIN, cb_event_login))
        goto error;

    if (!ov_event_engine_register(
            self->engine, OV_EVENT_API_UPDATE_LOGIN, cb_update_client_login))
        goto error;

    if (!ov_event_engine_register(
            self->engine, OV_EVENT_API_AUTHENTICATE, cb_event_login))
        goto error;

    if (!ov_event_engine_register(
            self->engine, OV_EVENT_API_LOGOUT, cb_event_logout))
        goto error;

    if (!ov_event_engine_register(
            self->engine, OV_VOCS_DB_UPDATE_PASSWORD, cb_event_update_password))
        goto error;

    if (!ov_event_engine_register(
            self->engine, OV_VOCS_DB_ADMIN_DOMAINS, cb_event_get_admin_domains))
        goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_ADMIN_PROJECTS,
                                  cb_event_get_admin_projects))
        goto error;

    if (!ov_event_engine_register(
            self->engine, OV_VOCS_DB_ID_EXISTS, cb_event_check_id_exists))
        goto error;

    if (!ov_event_engine_register(self->engine, OV_VOCS_DB_GET, cb_event_get))
        goto error;

    if (!ov_event_engine_register(
            self->engine, OV_VOCS_DB_DELETE, cb_event_delete))
        goto error;

    if (!ov_event_engine_register(
            self->engine, OV_VOCS_DB_CREATE, cb_event_create))
        goto error;

    if (!ov_event_engine_register(
            self->engine, OV_VOCS_DB_GET_KEY, cb_event_get_key))
        goto error;

    if (!ov_event_engine_register(
            self->engine, OV_VOCS_DB_UPDATE_KEY, cb_event_update_key))
        goto error;

    if (!ov_event_engine_register(
            self->engine, OV_VOCS_DB_DELETE_KEY, cb_event_delete_key))
        goto error;

    if (!ov_event_engine_register(
            self->engine, OV_VOCS_DB_VERIFY, cb_event_verify))
        goto error;

    if (!ov_event_engine_register(
            self->engine, OV_VOCS_DB_UPDATE, cb_event_update))
        goto error;

    if (!ov_event_engine_register(self->engine, OV_VOCS_DB_LOAD, cb_event_load))
        goto error;

    if (!ov_event_engine_register(self->engine, OV_VOCS_DB_SAVE, cb_event_save))
        goto error;

    if (!ov_event_engine_register(
            self->engine, OV_VOCS_DB_SET_LAYOUT, cb_event_set_layout))
        goto error;

    if (!ov_event_engine_register(
            self->engine, OV_VOCS_DB_GET_LAYOUT, cb_event_get_layout))
        goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_ADD_DOMAIN_ADMIN,
                                  cb_event_add_domain_admin))
        goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_ADD_PROJECT_ADMIN,
                                  cb_event_add_project_admin))
        goto error;

    if (!ov_event_engine_register(
            self->engine, OV_VOCS_DB_LDAP_IMPORT, cb_event_ldap_import))
        goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_SET_KEYSET_LAYOUT,
                                  cb_event_set_keyset_layout))
        goto error;

    if (!ov_event_engine_register(self->engine,
                                  OV_VOCS_DB_GET_KEYSET_LAYOUT,
                                  cb_event_get_keyset_layout))
        goto error;

    if (!ov_event_engine_register(
            self->engine, OV_VOCS_DB_SET_USER_DATA, cb_event_set_user_data))
        goto error;

    if (!ov_event_engine_register(
            self->engine, OV_VOCS_DB_GET_USER_DATA, cb_event_get_user_data))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool module_load_ldap(ov_vocs_db_app *self) {

    OV_ASSERT(self);

    self->config.ldap.config.loop = self->config.loop;
    self->ldap = ov_ldap_create(self->config.ldap.config);
    if (!self->ldap) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool check_config(ov_vocs_db_app_config *config) {

    if (!config->loop) goto error;
    if (!config->db) goto error;
    if (!config->persistance) goto error;

    if (0 == config->ldap.timeout_usec)
        config->ldap.timeout_usec = IMPL_DEFAULT_LDAP_TIMEOUT_USEC;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_vocs_db_app *ov_vocs_db_app_create(ov_vocs_db_app_config config) {

    ov_vocs_db_app *self = NULL;
    if (!check_config(&config)) goto error;

    self = calloc(1, sizeof(ov_vocs_db_app));
    if (!self) goto error;

    self->magic_bytes = OV_VOCS_DB_APP_MAGIC_BYTES;
    self->config = config;

    self->engine = ov_event_engine_create();
    if (!self->engine) goto error;

    ov_dict_config d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = connection_free;

    self->connections = ov_dict_create(d_config);

    self->async = ov_event_async_store_create(
        (ov_event_async_store_config){.loop = config.loop});

    if (!self->async || !self->connections) goto error;

    if (self->config.ldap.enable && !module_load_ldap(self)) goto error;

    if (!register_event_callbacks(self)) goto error;

    self->user_sessions =
        ov_event_session_create((ov_event_session_config){.loop = config.loop});
    if (!self->user_sessions) goto error;

    self->broadcasts = ov_broadcast_registry_create(
        (ov_event_broadcast_config){0});

    if (!self->broadcasts) goto error;

    return self;
error:
    ov_vocs_db_app_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_db_app *ov_vocs_db_app_free(ov_vocs_db_app *app) {

    if (!ov_vocs_db_app_cast(app)) goto error;

    app->async = ov_event_async_store_free(app->async);
    app->connections = ov_dict_free(app->connections);
    app->engine = ov_event_engine_free(app->engine);
    app->user_sessions = ov_event_session_free(app->user_sessions);
    app->ldap = ov_ldap_free(app->ldap);
    app->broadcasts = ov_broadcast_registry_free(app->broadcasts);

    app = ov_data_pointer_free(app);
error:
    return app;
}

/*----------------------------------------------------------------------------*/

ov_vocs_db_app *ov_vocs_db_app_cast(const void *data) {

    if (!data) return NULL;

    if (*(uint16_t *)data != OV_VOCS_DB_APP_MAGIC_BYTES) return NULL;

    return (ov_vocs_db_app *)data;
}

/*----------------------------------------------------------------------------*/

ov_vocs_db_app_config ov_vocs_db_app_config_from_json(
    const ov_json_value *val) {

    ov_vocs_db_app_config out = {0};

    if (!val) goto error;
    /*
        const ov_json_value *config = ov_json_object_get(val, OV_KEY_VOCS);
        if (!config) config = val;
    */
    out.ldap.config = ov_ldap_config_from_json(val);
    return out;
error:
    return (ov_vocs_db_app_config){0};
}
/*
 *      ------------------------------------------------------------------------
 *
 *      WEBIO EVENTS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_socket_close(void *userdata, int socket) {

    ov_vocs_db_app *self = ov_vocs_db_app_cast(userdata);
    if (!self || socket < 0) goto error;

    ov_log_info("client socket close at %i", socket);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool cb_client_process(void *userdata,
                              const int socket,
                              const ov_event_parameter *params,
                              ov_json_value *input) {

    ov_vocs_db_app *self = ov_vocs_db_app_cast(userdata);
    if (!self || socket < 0 || !params || !input) goto error;

    ov_event_engine_push(self->engine, self, socket, *params, input);
    return true;

error:
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_event_io_config ov_vocs_db_app_io_uri_config(ov_vocs_db_app *self) {

    return (ov_event_io_config){.name = "db",
                                .userdata = self,
                                .callback.close = cb_socket_close,
                                .callback.process = cb_client_process};
}

/*----------------------------------------------------------------------------*/

static bool env_send(ov_vocs_db_app *app, int socket, const ov_json_value *input) {

    OV_ASSERT(app);
    OV_ASSERT(app->config.env.send);
    OV_ASSERT(app->config.env.userdata);

    bool result =
        app->config.env.send(app->config.env.userdata, socket, input);

    return result;
}

/*----------------------------------------------------------------------------*/

static bool send_socket(void *userdata, int socket, const ov_json_value *input) {

    ov_vocs_db_app *self = ov_vocs_db_app_cast(userdata);
    if (!self || !input) return false;

    return env_send(self, socket, input);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_app_send_broadcast(
        ov_vocs_db_app *self,
        const ov_json_value *broadcast){

    if (!self || !broadcast) goto error;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            self->broadcasts, OV_BROADCAST_KEY_SYSTEM_BROADCAST, &parameter, broadcast, 
            OV_SYSTEM_BROADCAST)) goto error;

    return true;
error:
    return false;
}