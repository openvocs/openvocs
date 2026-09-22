/***
        ------------------------------------------------------------------------

        Copyright (c) 2026 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_mfa.c
        @author         Töpfer, Markus

        @date           2026-09-17


        ------------------------------------------------------------------------
*/

#define CONFIG_PATH                                                            \
    OPENVOCS_ROOT                                                              \
    "/src/service/ov_mfa/config/default_config.json"

#include <ov_base/ov_id.h>
#include <ov_base/ov_config.h>
#include <ov_base/ov_config_log.h>
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_json.h>
#include <ov_os/ov_os_event_loop.h>

#include <ov_core/ov_io.h>
#include <ov_core/ov_event_api.h>

#include <ov_vocs_db/ov_vocs_db.h>
#include <ov_vocs_db/ov_vocs_db_persistance.h>

#include <ov_webauthn/ov_webauthn.h>


/*---------------------------------------------------------------------------*/

typedef struct dummy_user_data {

    ov_io *io;
    ov_vocs_db *db;
    ov_webauthn *auth;

} dummy_user_data;

/*---------------------------------------------------------------------------*/

static void register_mfa(dummy_user_data *data, int socket, const ov_json_value *msg){

    ov_json_value *out = NULL;

    if (!data || !socket || !msg) goto error;

    const char *user = ov_json_string_get(ov_json_get(msg, "/parameter/user"));
    const char *domain = ov_io_get_socket_domain(data->io, socket);
    
    if (!user || !domain) {

        out = ov_event_api_create_error_response(msg, 1, "parameter error");
        goto response;
    
    }

    ov_log_debug("MFA - REGISTER for user %s", user);

    ov_json_value *val = ov_webauthn_create_registration_challenge(data->auth, user, domain);

    if (!val){

        out = ov_event_api_create_error_response(msg, 3, "processing error.");
    
    } else {

        out = ov_event_api_create_success_response(msg);
        ov_json_value *par = ov_event_api_get_response(out);
        ov_json_object_set(par, "challenge", val);
    }

response:

    char *str = ov_json_value_to_string(out);
    ov_io_send(data->io, socket, (ov_memory_pointer){
        .start = (uint8_t*) str,
        .length = strlen(str)
    });
    str = ov_data_pointer_free(str);

error:
    out = ov_json_value_free(out);
    return;
}

/*---------------------------------------------------------------------------*/

static void register_mfa_second_factor(dummy_user_data *data, int socket, const ov_json_value *msg){

    ov_json_value *out = NULL;

    if (!data || !socket || !msg) goto error;

    if (ov_webauthn_process_registration_challenge(data->auth, ov_event_api_get_parameter(msg))){

        out = ov_event_api_create_success_response(msg);

    } else {

        out = ov_event_api_create_error_response(msg, 5, "MFA registration failed.");
    
    }

    char *str = ov_json_value_to_string(out);
    ov_io_send(data->io, socket, (ov_memory_pointer){
        .start = (uint8_t*) str,
        .length = strlen(str)
    });
    str = ov_data_pointer_free(str);

error:
    out = ov_json_value_free(out);
    return;
}

/*---------------------------------------------------------------------------*/

static void login_password(dummy_user_data *data, int socket, const ov_json_value *msg){

    ov_json_value *out = NULL;

    if (!data || !socket || !msg) goto error;

    const char *user = ov_json_string_get(ov_json_get(msg, "/parameter/user"));
    const char *pass = ov_json_string_get(ov_json_get(msg, "/parameter/password"));
    const char *domain = ov_io_get_socket_domain(data->io, socket);

    if (!user || !pass) {

        out = ov_event_api_create_error_response(msg, 1, "parameter error");
        goto response;
    
    }

    ov_log_debug("MFA - AUTH 1 - PASSWORD for user %s", user);

    if (!ov_vocs_db_authenticate(data->db, user, pass)){

        out = ov_event_api_create_error_response(msg, 2, "wrong password or user");
        goto response;
    }

    ov_json_value *val = ov_webauthn_create_login_challenge(data->auth,user, domain);

    if (!val){

        out = ov_event_api_create_error_response(msg, 6, "MFA CHALLENGE FAILED.");
        goto response;

    }

    out = ov_event_api_create_success_response(msg);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, "mfa", val);

response:

    char *str = ov_json_value_to_string(out);
    ov_io_send(data->io, socket, (ov_memory_pointer){
        .start = (uint8_t*) str,
        .length = strlen(str)
    });
    str = ov_data_pointer_free(str);

error:
    out = ov_json_value_free(out);
    return;
}

/*---------------------------------------------------------------------------*/

static void login_second_factor(dummy_user_data *data, int socket, const ov_json_value *msg){

    ov_json_value *out = NULL;

    if (!data || !socket || !msg) goto error;

    ov_log_debug("MFA - AUTH 2 - WEBAUTHN");

    if (!ov_webauthn_process_login_challenge(data->auth,
        ov_event_api_get_parameter(msg))){

        out = ov_event_api_create_error_response(msg, 7, "second factor error");
        goto response;

    }

    out = ov_event_api_create_success_response(msg);

response:

    char *str = ov_json_value_to_string(out);
    ov_io_send(data->io, socket, (ov_memory_pointer){
        .start = (uint8_t*) str,
        .length = strlen(str)
    });
    str = ov_data_pointer_free(str);

error:
    out = ov_json_value_free(out);
    return;
}

/*---------------------------------------------------------------------------*/

static void callback_event(void *userdata, int socket, ov_json_value *msg){

    dummy_user_data *data = (dummy_user_data*) userdata;

    UNUSED(socket);
    if (!data || !msg) goto error;

    if (ov_event_api_event_is(msg, "register")){

        register_mfa(data, socket, msg);

    } else if (ov_event_api_event_is(msg, "login")){

        login_password(data, socket, msg);

    } else if (ov_event_api_event_is(msg, "mfa_register")){

        register_mfa_second_factor(data, socket, msg);

    } else if (ov_event_api_event_is(msg, "mfa_login")){

        login_second_factor(data, socket, msg);
    }

error:
    ov_json_value_free(msg);
    return;
}

/*---------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    int retval = EXIT_FAILURE;

    dummy_user_data userdata = {0};

    ov_event_loop *loop = NULL;
    ov_json_value *json_config = NULL;
    ov_vocs_db *db = NULL;
    ov_vocs_db_persistance *db_persistance = NULL;
    ov_io *io = NULL;

    ov_event_loop_config loop_config = (ov_event_loop_config){
        .max.sockets = ov_socket_get_max_supported_runtime_sockets(0),
        .max.timers = ov_socket_get_max_supported_runtime_sockets(0)};

    const char *path = ov_config_path_from_command_line(argc, argv);
    if (!path)
        path = CONFIG_PATH;

    if (path == VERSION_REQUEST_ONLY)
        goto error;

    json_config = ov_config_load(path);
    if (!json_config) {
        ov_log_error("Failed to load config from %s", path);
        goto error;
    } else {
        ov_log_debug("Config load from PATH %s", path);
    }

    if (!ov_config_log_from_json(json_config))
        goto error;

    loop = ov_os_event_loop(loop_config);

    if (!loop) {
        ov_log_error("Failed to create eventloop");
        goto error;
    }

    if (!ov_event_loop_setup_signals(loop))
        goto error;

    /* Create webserver instance */

    const char *domain = ov_json_string_get(ov_json_object_get(
        (ov_json_object_get(json_config, "mfa")), OV_KEY_DOMAIN));

    if (!domain) {
        ov_log_error("No domain input to enable mfa module");
        goto error;
    }

    ov_io_config io_config = ov_io_config_from_json(json_config);
    io_config.loop = loop;

    io = ov_io_create(io_config);

    if (!io)
        goto error;

    /* Create DB instance */

    ov_vocs_db_config db_config = ov_vocs_db_config_from_json(json_config);

    db = ov_vocs_db_create(db_config);
    if (!db) {
        ov_log_error("Failed to create db");
        goto error;
    }

    userdata.db = db;
    userdata.io = io;

    ov_vocs_db_persistance_config db_persistance_config =
        ov_vocs_db_persistance_config_from_json(json_config);

    db_persistance_config.db = db;
    db_persistance_config.loop = loop;
    db_persistance_config.io = io;
    db_persistance = ov_vocs_db_persistance_create(db_persistance_config);
    if (!db_persistance) {
        ov_log_error("Failed to create db_persistance");
        goto error;
    }

    if (!ov_vocs_db_persistance_load(db_persistance)) {
        ov_log_error("Failed to load db_persistance.");
    }

    if (!ov_vocs_db_set_persistance(db, db_persistance))
        goto error;

    /* Create webauthn instance */

    ov_webauthn_config auth_config = ov_webauthn_config_from_json(json_config);
    auth_config.loop = loop;
    auth_config.db = db;

    userdata.auth = ov_webauthn_create(auth_config);

    /* Create webserver environment */

    ov_io_https_config https_config = ov_io_https_config_from_json(json_config);

    int webserver = ov_io_open_https(io, https_config);
    if (-1 == webserver){
        ov_log_error("Failed to create Webserver.");
        goto error;
    }

    if (!ov_io_enable_websocket_events(io,
        domain,
        "/",
        &userdata,
        callback_event)) goto error;

    /*  Run event loop */
    loop->run(loop, OV_RUN_MAX);

    retval = EXIT_SUCCESS;

error:
    
    userdata.auth = ov_webauthn_free(userdata.auth);

    io = ov_io_free(io);
    json_config = ov_json_value_free(json_config);
    db_persistance = ov_vocs_db_persistance_free(db_persistance);
    db = ov_vocs_db_free(db);
    loop = ov_event_loop_free(loop);
  
    return retval;
}
