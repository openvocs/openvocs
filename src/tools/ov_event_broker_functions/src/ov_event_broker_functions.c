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
        @file           ov_event_broker_functions.c
        @author         Töpfer, Markus

        @date           2026-04-20


        ------------------------------------------------------------------------
*/
#include <ov_core/ov_event_api.h>
#include <ov_core/ov_io.h>
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_convert.h>
#include <ov_base/ov_socket.h>
#include <ov_os/ov_os_event_loop.h>

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>

struct user_input {

    ov_socket_configuration socket;
    char password[PATH_MAX];

};

/*----------------------------------------------------------------------------*/

struct userdata {

    ov_event_loop *loop;
    ov_io *io;

    int socket;

    struct user_input user_input;
};

static void print_usage() {

    fprintf(stdout, "\n");
    fprintf(stdout, "Get functions of ov_event_broker.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "USAGE              [OPTIONS]... [PASSWORD]\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "               -i,     --interface host to connect to\n");
    fprintf(stdout, "               -p,     --port      port to connect to\n");
    fprintf(stdout, "               -x,     --password  password to use\n");
    fprintf(stdout, "               -h,     --help      print this help\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "\n");

    return;
}

/*----------------------------------------------------------------------------*/

bool read_user_input(int argc, char *argv[], struct user_input *config) {

    int c = 0;
    int option_index = 0;

    while (1) {

        static struct option long_options[] = {

            /* These options don’t set a flag.
               We distinguish them by their indices. */
            {"interface", required_argument, 0, 'i'},
            {"port", required_argument, 0, 'p'},
            {"password", required_argument, 0, 'y'},
            {"help", optional_argument, 0, 'h'},
            {0, 0, 0, 0}};

        /* getopt_long stores the option index here. */

        c = getopt_long(argc, argv, "p:i:x:h", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {

        case 0:

            printf("option %s", long_options[option_index].name);
            if (optarg)
                printf(" with arg %s", optarg);
            printf("\n");
            break;

        case 'h':
            print_usage();
            goto error;
            break;

        case 'p':
            ov_convert_string_to_uint16(optarg, strlen(optarg), &config->socket.port);
            break;

        case 'i':
            strncpy(config->socket.host, optarg, OV_HOST_NAME_MAX);
            break;

        case 'x':
            strncpy(config->password, optarg, PATH_MAX);
            break;

        default:
            print_usage();
            goto error;
        }
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void cb_connected(void *userdata, int socket){

    struct userdata *self = (struct userdata*) userdata;

    ov_json_value *msg = ov_event_api_message_create("broker_login", NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(msg);
    ov_json_object_set(par, "password", ov_json_string(self->user_input.password));

    char *str = ov_json_value_to_string(msg);
    
    ov_io_send(self->io, socket, (ov_memory_pointer){
        .start = (uint8_t*) str,
        .length = strlen(str)
    });

    ov_log_debug("send login");

    str = ov_data_pointer_free(str);
    msg = ov_json_value_free(msg);
    return;
}

/*----------------------------------------------------------------------------*/

static bool cb_io(void *userdata, int socket, const char *domain, const ov_memory_pointer data){

    struct userdata *self = (struct userdata*) userdata;
    UNUSED(domain);

    ov_json_value *msg = ov_json_value_from_string((char*)data.start, data.length);
    if (!msg) goto error;

    if (ov_event_api_event_is(msg, "broker_login")){

        ov_json_value *out = ov_event_api_message_create("functions", NULL, 0);

        char *str = ov_json_value_to_string(out);
    
        ov_io_send(self->io, socket, (ov_memory_pointer){
            .start = (uint8_t*) str,
            .length = strlen(str)
        });

        ov_log_debug("send %s", str);

        str = ov_data_pointer_free(str);
        out = ov_json_value_free(out);

    } else {

        ov_log_debug("%.*s", data.length, (char*) data.start);
    }

    msg = ov_json_value_free(msg);
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char **argv){

    int retval = EXIT_FAILURE;

    struct userdata self = {0};

    ov_event_loop_config loop_config = (ov_event_loop_config){
        .max.sockets = ov_socket_get_max_supported_runtime_sockets(0),
        .max.timers = ov_socket_get_max_supported_runtime_sockets(0)};

    self.loop = ov_os_event_loop(loop_config);

    if (!self.loop) {
        ov_log_error("Failed to create eventloop");
        goto error;
    }

    self.io = ov_io_create((ov_io_config){ .loop = self.loop });
    if (!self.io) {
        ov_log_error("Failed to create io.");
    }

    if (!read_user_input(argc, argv, &self.user_input))
        goto error;

    self.user_input.socket.type = TCP;

    fprintf(stdout, "connecting to host:port %s:%i\n", 
        self.user_input.socket.host,
        self.user_input.socket.port);

    self.socket = ov_io_open_connection(self.io, (ov_io_socket_config){
        .auto_reconnect = true,
        .socket = self.user_input.socket,
        .callbacks.userdata = &self,
        .callbacks.connected = cb_connected,
        .callbacks.io = cb_io
    });

    self.loop->run(self.loop, OV_RUN_MAX);

    retval = EXIT_SUCCESS;
error:
    ov_io_free(self.io);
    ov_event_loop_free(self.loop);
    return retval;
}