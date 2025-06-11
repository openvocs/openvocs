/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_event_listener_cmdline.c
        @author         Markus Töpfer

        @date           2024-01-25


        ------------------------------------------------------------------------
*/

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_convert.h>
#include <ov_base/ov_id.h>
#include <ov_base/ov_socket.h>

#include <ov_core/ov_event_api.h>
#include <ov_core/ov_io_base.h>

#include <ov_os/ov_os_event_loop.h>

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>

static void print_usage() {

    fprintf(stdout, "\n");
    fprintf(stdout, "Listen to events of VOCS per commandline.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "USAGE              [OPTIONS]... [PASSWORD]\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "               -i,     --interface host to connect to\n");
    fprintf(stdout, "               -p,     --port      port to connect to\n");

    fprintf(stdout, "               -h,     --help      print this help\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "\n");
    fprintf(stdout,
            "This tool connects to a VOCS instance and prints the events "
            "happening.\n");

    return;
}

/*----------------------------------------------------------------------------*/

bool read_user_input(int argc, char *argv[], ov_socket_configuration *config) {

    int c = 0;
    int option_index = 0;

    while (1) {

        static struct option long_options[] = {

            /* These options don’t set a flag.
               We distinguish them by their indices. */
            {"interface", required_argument, 0, 'i'},
            {"port", required_argument, 0, 'p'},
            {"help", optional_argument, 0, 'h'},
            {0, 0, 0, 0}};

        /* getopt_long stores the option index here. */

        c = getopt_long(argc, argv, "p:i:h", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1) break;

        switch (c) {

            case 0:

                printf("option %s", long_options[option_index].name);
                if (optarg) printf(" with arg %s", optarg);
                printf("\n");
                break;

            case 'h':
                print_usage();
                goto error;
                break;

            case 'p':
                ov_convert_string_to_uint16(
                    optarg, strlen(optarg), &config->port);
                break;

            case 'i':
                strncpy(config->host, optarg, OV_HOST_NAME_MAX);
                break;

            default:
                print_usage();
                goto error;
        }
    }

    if (optind < argc) {
        strncpy(config->host, argv[optind++], OV_HOST_NAME_MAX);
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

struct userdata {

    ov_event_loop *loop;
    ov_io_base *base;

    int socket;
    ov_id uuid;
};

/*----------------------------------------------------------------------------*/

static void cb_connected(void *userdata, int socket, bool result) {

    struct userdata *self = (struct userdata *)userdata;

    if (!result) {

        ov_log_error("failed to connect socket");

    } else {

        ov_log_debug("connected socket");

        ov_json_value *out =
            ov_event_api_message_create(OV_KEY_REGISTER, NULL, 0);
        ov_json_value *par = ov_event_api_set_parameter(out);
        ov_json_object_set(par, OV_KEY_UUID, ov_json_string(self->uuid));
        ov_json_object_set(
            par, OV_KEY_TYPE, ov_json_string("commandline logger"));

        char *str = ov_json_value_to_string(out);
        out = ov_json_value_free(out);

        ov_io_base_send(self->base,
                        socket,
                        (ov_memory_pointer){
                            .start = (uint8_t *)str, .length = strlen(str)});

        str = ov_data_pointer_free(str);
    }

    self->socket = socket;

    return;
}

/*----------------------------------------------------------------------------*/

static void cb_close(void *userdata, int connection) {

    struct userdata *self = (struct userdata *)userdata;
    UNUSED(self);

    ov_log_error("closing connection socket %i", connection);

    return;
}

/*----------------------------------------------------------------------------*/

static bool cb_io(void *userdata,
                  int connection,
                  const ov_memory_pointer buffer) {

    struct userdata *self = (struct userdata *)userdata;
    UNUSED(self);
    UNUSED(connection);

    fprintf(stdout, "EVENT <- %s\n", (char *)buffer.start);
    return true;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {

    int retval = EXIT_FAILURE;

    struct userdata self = {0};

    ov_socket_configuration config = {.type = TCP};
    ov_event_loop *loop = NULL;
    ov_io_base *base = NULL;

    ov_event_loop_config loop_config = (ov_event_loop_config){
        .max.sockets = ov_socket_get_max_supported_runtime_sockets(0),
        .max.timers = ov_socket_get_max_supported_runtime_sockets(0)};

    loop = ov_os_event_loop(loop_config);

    if (!loop) {
        ov_log_error("Failed to create eventloop");
        goto error;
    }

    base = ov_io_base_create(
        (ov_io_base_config){.loop = loop, .name = "ov_event_listener_cmdline"});

    if (!base) {
        ov_log_error("Failed to create IO base.");
        goto error;
    }

    self.base = base;
    self.loop = loop;
    ov_id_fill_with_uuid(self.uuid);

    if (!read_user_input(argc, argv, &config)) goto error;

    if (0 == config.port) config.port = 44444;

    if (0 == config.host[0]) strcat(config.host, "127.0.0.1");

    fprintf(
        stdout, "connecting to host:port %s:%i\n", config.host, config.port);

    self.socket = ov_io_base_create_connection(
        base,
        (ov_io_base_connection_config){.connection.socket = config,
                                       .connection.callback.userdata = &self,
                                       .connection.callback.io = cb_io,
                                       .connection.callback.close = cb_close,
                                       .client_connect_trigger_usec = 1,
                                       .auto_reconnect = true,
                                       .connected = cb_connected});

    if (-1 == self.socket) {
        ov_log_error("failed to connect to %s:%i\n", config.host, config.port);
    }

    loop->run(loop, OV_RUN_MAX);

    retval = EXIT_SUCCESS;
error:
    ov_event_loop_free(loop);
    return retval;
}
