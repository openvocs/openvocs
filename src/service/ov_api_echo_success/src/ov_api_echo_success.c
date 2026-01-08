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
        @file           ov_api_echo_success.c
        @author         Töpfer, Markus

        @date           2025-11-24


        ------------------------------------------------------------------------
*/

#include <ov_base/ov_config.h>
#include <ov_base/ov_config_log.h>
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_socket.h>

#include <ov_core/ov_event_api.h>
#include <ov_core/ov_io.h>

#include <ov_os/ov_os_event_loop.h>

/*----------------------------------------------------------------------------*/

#define CONFIG_PATH                                                            \
    OPENVOCS_ROOT                                                              \
    "/src/service/ov_api_echo_success/config/config.json"

/*----------------------------------------------------------------------------*/

static bool cb_io(void *userdata, int socket, const char *domain,
                  const ov_memory_pointer data) {

    bool retval = false;

    UNUSED(domain);

    ov_io *io = ov_io_cast(userdata);
    if (!io)
        goto error;

    ov_json_value *content = ov_json_read((char *)data.start, data.length);
    if (!content) {
        ov_log_error("Received non JSON content - ignoring");
        goto done;
    }

    ov_json_value *out = ov_event_api_create_success_response(content);
    char *str = ov_json_value_to_string(out);
    if (!str) {
        ov_log_error("Failed to parse response - ignoring");
        goto done;
    }

    ov_log_debug("OUT %s", str);

    ov_io_send(
        io, socket,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)});

done:
    out = ov_json_value_free(out);
    str = ov_data_pointer_free(str);
    content = ov_json_value_free(content);
    retval = true;
error:
    return retval;
}

/*----------------------------------------------------------------------------*/

static void cb_connected(void *userdata, int socket) {

    ov_io *io = ov_io_cast(userdata);

    ov_json_value *msg = ov_event_api_message_create("register", NULL, 0);
    char *str = ov_json_value_to_string(msg);
    ov_io_send(
        io, socket,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)});
    msg = ov_json_value_free(msg);
    str = ov_data_pointer_free(str);

    ov_log_debug("opened connection and registered.");
    return;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    int retval = EXIT_FAILURE;

    ov_event_loop *loop = NULL;
    ov_io *io = NULL;

    ov_json_value *json_config = NULL;

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

    io = ov_io_create((ov_io_config){.loop = loop});

    if (!io)
        goto error;

    int socket = -1;

    ov_socket_configuration socket_config = ov_socket_configuration_from_json(
        ov_json_get(json_config, "/echo/socket"), (ov_socket_configuration){0});

    if (ov_json_is_true(ov_json_get(json_config, "/echo/client"))) {

        ov_log_debug("Trying to open connection to %s:%i", socket_config.host,
                     socket_config.port);

        socket = ov_io_open_connection(
            io, (ov_io_socket_config){.auto_reconnect = true,
                                      .socket = socket_config,
                                      .callbacks.userdata = io,
                                      .callbacks.io = cb_io,
                                      .callbacks.connected = cb_connected

                });

        if (-1 != socket) {

            cb_connected(io, socket);
        }

    } else {

        ov_log_debug("Trying to open %s:%i", socket_config.host,
                     socket_config.port);

        socket = ov_io_open_listener(
            io, (ov_io_socket_config){.socket = socket_config,
                                      .callbacks.userdata = io,
                                      .callbacks.io = cb_io

                });

        if (-1 == socket) {

            ov_log_error("Failed to open socket");
            goto error;
        }
    }

    /*  Run event loop */
    loop->run(loop, OV_RUN_MAX);

    retval = EXIT_SUCCESS;

error:

    json_config = ov_json_value_free(json_config);
    io = ov_io_free(io);
    loop = ov_event_loop_free(loop);
    return retval;
}
