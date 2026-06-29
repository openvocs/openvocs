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
        @file           ov_mc_mixer.c
        @author         Markus Töpfer

        @date           2023-01-21


        ------------------------------------------------------------------------
*/

#include <ov_base/ov_config.h>
#include <ov_base/ov_config_log.h>
#include <ov_base/ov_convert.h>
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_json.h>
#include <ov_os/ov_os_event_loop.h>

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>

#include <ov_vocs/ov_mc_mixer_app.h>

#define CONFIG_PATH                                                            \
    OPENVOCS_ROOT                                                              \
    "/src/service/ov_mc_mixer/config/default_config.json"

/*---------------------------------------------------------------------------*/

static bool read_user_input(int argc, char **argv, char *host, uint32_t *port, char **path){

    int c;
    int option_index = 0;

    while (1) {

        static struct option long_options[] = {

            /* These options don’t set a flag.
               We distinguish them by their indices. */
            {"port", required_argument, 0, 'p'},
            {"host", required_argument, 0, 'h'},
            {"version", optional_argument, 0, 'v'},
            {"config", optional_argument, 0, 'c'},
            {0, 0, 0, 0}};

        /* getopt_long stores the option index here. */

        c = getopt_long(argc, argv, "p:h:v?:c?", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {

        case 0:
            *path = optarg;
            break;

        case 'h':
            snprintf(host, OV_HOST_NAME_MAX, "%s", optarg);
            break;

        case 'v':
            OV_VERSION_PRINT(stderr);
            goto error;
            break;

        case 'c':
            *path = optarg;
            break;

        case 'p':
            ov_convert_string_to_uint32(optarg, strlen(optarg), port);
            break;

        default:
            goto error;
        }
    }

    if (optind < argc) {
        *path = argv[optind++];
    }

    return true;
error:  
    return false;
}

/*---------------------------------------------------------------------------*/

const char *default_config = "{"
    "\"log\" : {"
        "\"systemd\" : false,"
        "\"file\" : \"stdout\","
        "\"level\" : \"error\""
    "},"
    "\"app\" :"
    "{"
        "\"resource_manager\":"
        "{"
            "\"host\" : \"127.0.0.1\","
            "\"port\" : 12346,"
            "\"type\" : \"TCP\""
        "}"
    "}"
"}";

/*---------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    int retval = EXIT_FAILURE;

    ov_mc_mixer_app *app = NULL;
    ov_io *io = NULL;
    ov_event_loop *loop = NULL;
    ov_json_value *json_config = NULL;

    ov_event_loop_config loop_config = (ov_event_loop_config){
        .max.sockets = ov_socket_get_max_supported_runtime_sockets(0),
        .max.timers = ov_socket_get_max_supported_runtime_sockets(0)};

    char *path = NULL;
    char host[OV_HOST_NAME_MAX] = {0};
    uint32_t port = 0;

    if (!read_user_input(argc, argv, host, &port, &path))
        goto error;

    if (path){

        json_config = ov_config_load(path);

        if (!json_config) {
            ov_log_error("Failed to load config from %s", path);
            goto error;
        } else {
            ov_log_debug("Config load from PATH %s", path);
        }

    } else {

        json_config = ov_json_decode(default_config);
        ov_json_value *res_mgr = (ov_json_value*) ov_json_get(json_config, "/app/resource_manager");
        ov_json_object_set(res_mgr, "host", ov_json_string(host));
        ov_json_object_set(res_mgr, "port", ov_json_number(port));

    }

    char *str = ov_json_value_to_string(json_config);
    ov_log_debug("%s", str);
    str = ov_data_pointer_free(str);

    if (!ov_config_log_from_json(json_config))
        goto error;

    loop = ov_os_event_loop(loop_config);

    if (!loop) {
        ov_log_error("Failed to create eventloop");
        goto error;
    }

    if (!ov_event_loop_setup_signals(loop))
        goto error;

    ov_io_config io_config = ov_io_config_from_json(json_config);
    io_config.loop = loop;

    io = ov_io_create(io_config);
    if (!io) {
        ov_log_error("Failed to create io");
        goto error;
    }

    ov_mc_mixer_app_config app_config =
        ov_mc_mixer_app_config_from_json(json_config);
    app_config.loop = loop;
    app_config.io = io;

    app = ov_mc_mixer_app_create(app_config);
    if (!app) {
        ov_log_error("Failed to create APP");
        goto error;
    }

    /*  Run event loop */
    loop->run(loop, OV_RUN_MAX);

    retval = EXIT_SUCCESS;

error:

    json_config = ov_json_value_free(json_config);
    app = ov_mc_mixer_app_free(app);
    io = ov_io_free(io);
    loop = ov_event_loop_free(loop);

    return retval;
}
