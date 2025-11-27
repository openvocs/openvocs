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
        @file           ov_mc_vocs.c
        @author         Markus Töpfer

        @date           2023-01-26


        ------------------------------------------------------------------------
*/

#include <ov_base/ov_config.h>
#include <ov_base/ov_config_log.h>
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_json.h>
#include <ov_os/ov_os_event_loop.h>

#include <ov_core/ov_event_trigger.h>
#include <ov_core/ov_io.h>
#include <ov_core/ov_webserver_minimal.h>

#include <ov_vocs_db/ov_vocs_db.h>
#include <ov_vocs_db/ov_vocs_db_app.h>
#include <ov_vocs_db/ov_vocs_db_persistance.h>

#include <ov_base/ov_plugin_system.h>
#include <ov_vocs/ov_vocs.h>

#define CONFIG_PATH                                                            \
    OPENVOCS_ROOT                                                              \
    "/src/service/ov_mc_vocs/config/default_config.json"

/*----------------------------------------------------------------------------*/

static bool env_close_socket(void *userdata, int socket) {

    ov_webserver_minimal *srv = ov_webserver_minimal_cast(userdata);
    return ov_webserver_minimal_close(srv, socket);
}

/*----------------------------------------------------------------------------*/

static bool env_send_socket(void *userdata,
                            int socket,
                            const ov_json_value *msg) {

    ov_webserver_minimal *srv = ov_webserver_minimal_cast(userdata);
    return ov_webserver_minimal_send_json(srv, socket, msg);
}

/*---------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    int retval = EXIT_FAILURE;

    ov_event_loop *loop = NULL;
    ov_webserver_minimal *server = NULL;
    ov_json_value *json_config = NULL;
    ov_vocs_db *db = NULL;
    ov_vocs_db_persistance *db_persistance = NULL;
    ov_vocs *vocs = NULL;
    ov_event_trigger *trigger = NULL;
    ov_io *io = NULL;

    ov_database_export_symbols_for_plugins();
    ov_plugin_system_load_dir(0);

    ov_event_loop_config loop_config = (ov_event_loop_config){
        .max.sockets = ov_socket_get_max_supported_runtime_sockets(0),
        .max.timers = ov_socket_get_max_supported_runtime_sockets(0)};

    const char *path = ov_config_path_from_command_line(argc, argv);
    if (!path) path = CONFIG_PATH;

    if (path == VERSION_REQUEST_ONLY) goto error;

    json_config = ov_config_load(path);
    if (!json_config) {
        ov_log_error("Failed to load config from %s", path);
        goto error;
    } else {
        ov_log_debug("Config load from PATH %s", path);
    }

    if (!ov_config_log_from_json(json_config)) goto error;

    loop = ov_os_event_loop(loop_config);

    if (!loop) {
        ov_log_error("Failed to create eventloop");
        goto error;
    }

    if (!ov_event_loop_setup_signals(loop)) goto error;

    /* Create webserver instance */

    ov_webserver_minimal_config webserver_config = {0};
    webserver_config = ov_webserver_minimal_config_from_json(json_config);
    webserver_config.base.loop = loop;

    server = ov_webserver_minimal_create(webserver_config);
    if (!server) {
        ov_log_error("Failed to create webserver");
        goto error;
    }

    const char *domain = ov_json_string_get(ov_json_object_get(
        (ov_json_object_get(json_config, "vocs")), OV_KEY_DOMAIN));

    if (!domain) {
        ov_log_error("No domain input to enable vocs module");
        goto error;
    }

    io = ov_io_create((ov_io_config){.loop = loop});

    if (!io) goto error;

    /*  Create DB relevant items
     *
     *  (1) DB itself
     *  (2) DB persistance layer
     *  (3) DB service layer
     */
    trigger = ov_event_trigger_create((ov_event_trigger_config){0});
    if (!trigger) goto error;

    ov_vocs_db_config db_config = ov_vocs_db_config_from_json(json_config);
    db_config.trigger = trigger;

    db = ov_vocs_db_create(db_config);
    if (!db) {
        ov_log_error("Failed to create db");
        goto error;
    }

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
    
    if (!ov_vocs_db_set_persistance(db, db_persistance)) goto error;

    
    /* Create the vocs core */

    ov_vocs_config core_config = ov_vocs_config_from_json(json_config);
    core_config.loop = loop;
    core_config.db = db;
    core_config.persistance = db_persistance;
    core_config.io = io;
    core_config.env.userdata = server;
    core_config.env.close = env_close_socket;
    core_config.env.send = env_send_socket;
    core_config.trigger = trigger;

    vocs = ov_vocs_create(core_config);
    if (!vocs) goto error;

    /* Enable uri domain/api for VOCS operation */

    if (!ov_webserver_minimal_configure_uri_event_io(
            server,
            (ov_memory_pointer){
                .start = (uint8_t *)domain, .length = strlen(domain)

            },
            ov_vocs_event_io_uri_config(vocs))) {

        ov_log_error(
            "Failed to enable vocs URI callback "
            "at domain %s - check config to include same domain in "
            "webserver and vocs module.",
            domain);

        goto error;
    }

    /*  Run event loop */
    loop->run(loop, OV_RUN_MAX);

    retval = EXIT_SUCCESS;

error:

    json_config = ov_json_value_free(json_config);
    vocs = ov_vocs_free(vocs);
    db_persistance = ov_vocs_db_persistance_free(db_persistance);
    db = ov_vocs_db_free(db);
    server = ov_webserver_minimal_free(server);
    loop = ov_event_loop_free(loop);
    trigger = ov_event_trigger_free(trigger);
    io = ov_io_free(io);
    return retval;
}
