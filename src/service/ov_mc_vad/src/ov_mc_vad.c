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
        @file           ov_mc_vad.c
        @author         Markus Töpfer

        @date           2025-01-10


        ------------------------------------------------------------------------
*/

#include <ov_base/ov_config.h>
#include <ov_base/ov_config_log.h>
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_json.h>
#include <ov_os/ov_os_event_loop.h>

#include <ov_vad/ov_vad_app.h>

#define CONFIG_PATH                                                            \
    OPENVOCS_ROOT                                                              \
    "/src/service/ov_mc_vad/config/default_config.json"

/*---------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    int retval = EXIT_FAILURE;

    ov_vad_app *app = NULL;
    ov_io *io = NULL;
    ov_event_loop *loop = NULL;
    ov_json_value *json_config = NULL;

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

    ov_io_config io_config = ov_io_config_from_json(json_config);
    io_config.loop = loop;

    io = ov_io_create(io_config);
    if (!io) {
        ov_log_error("Failed to create io");
        goto error;
    }

    ov_vad_app_config app_config = ov_vad_app_config_from_json(json_config);
    app_config.loop = loop;
    app_config.io = io;

    app = ov_vad_app_create(app_config);
    if (!app) {
        ov_log_error("Failed to create app");
        goto error;
    }

    /*  Run event loop */
    loop->run(loop, OV_RUN_MAX);

    retval = EXIT_SUCCESS;

error:

    json_config = ov_json_value_free(json_config);
    app = ov_vad_app_free(app);
    io = ov_io_free(io);
    loop = ov_event_loop_free(loop);
    return retval;
}
