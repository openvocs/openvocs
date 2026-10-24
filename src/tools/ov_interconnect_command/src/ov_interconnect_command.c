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
        @file           ov_interconnect_command.c
        @author         Töpfer, Markus

        @date           2026-10-23


        ------------------------------------------------------------------------
*/

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_string.h>
#include <ov_core/ov_io.h>
#include <ov_core/ov_event_app.h>
#include <ov_core/ov_event_api.h>
#include <ov_os/ov_os_event_loop.h>

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct userdata {

    char *password;
    char *loop1;
    char *loop2;
    bool on;
    int socket;

    ov_event_app *app;
    ov_event_loop *loop;
    ov_io *io;

} userdata;

/*----------------------------------------------------------------------------*/

static void print_usage() {

    fprintf(stdout, "\n");
    fprintf(stdout, "Command an interconnect server.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "USAGE              [OPTIONS]\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "               -i,     --ip        ip to use\n");
    fprintf(stdout, "               -p,     --port      port to use\n");
    fprintf(stdout, "               -x,     --pass      password to use\n");
    fprintf(stdout, "               -a,     --loopA     loop to bridge\n");
    fprintf(stdout, "               -b,     --loopB     loop to bridge\n");
    fprintf(stdout, "               -o,     --ob        [on|off]\n");

    return;
}

/*----------------------------------------------------------------------------*/

bool read_user_input(int argc, char *argv[], char **host, uint16_t *port, 
        char **password, char **loop1, char **loop2, char **on) {

    int c = 0;
    int option_index = 0;

    while (1) {

        static struct option long_options[] = {

            /* These options don’t set a flag.
               We distinguish them by their indices. */
            {"ip", required_argument, 0, 'i'},
            {"port", required_argument, 0, 'p'},
            {"pass", required_argument, 0, 'x'},
            {"loopA", required_argument, 0, 'a'},
            {"loopB", required_argument, 0, 'b'},
            {"on", required_argument, 0, 'o'},
            {"help", optional_argument, 0, 'h'},
            {0, 0, 0, 0}};

        /* getopt_long stores the option index here. */

        c = getopt_long(argc, argv, "i:p:x:a:b:o:?h", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {

        case 'h':
            print_usage();
            goto error;
            break;

        case '?':
            print_usage();
            goto error;
            break;

        case 'p':
            ov_convert_string_to_uint16(optarg, strlen(optarg), port);
            break;

        case 'i':
            *host = ov_string_dup(optarg);
            break;

        case 'a':
            *loop1 = ov_string_dup(optarg);
            break;

        case 'b':
            *loop2 = ov_string_dup(optarg);
            break;

        case 'x':
            *password = ov_string_dup(optarg);
            break;

        case 'o':
            *on = ov_string_dup(optarg);
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

static void cb_connected(void *userdata, int connection){

    struct userdata *self = (struct userdata*) userdata;

    self->socket = connection;

    ov_json_value *msg = ov_event_api_message_create("register", NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(msg);
    ov_json_object_set(par, "password", ov_json_string(self->password));

    ov_event_app_send(self->app, self->socket, msg);
    msg = ov_json_value_free(msg);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_register(void *userdata, const char *event_name, int socket,
                             ov_json_value *input){

    UNUSED(event_name);

    struct userdata *self = (struct userdata*) userdata;

    ov_json_value *msg = ov_event_api_message_create("loop", NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(msg);
    ov_json_object_set(par, "loop1", ov_json_string(self->loop1));
    ov_json_object_set(par, "loop2", ov_json_string(self->loop2));
    if (self->on){
        ov_json_object_set(par, "on", ov_json_true());
    } else {
        ov_json_object_set(par, "on", ov_json_false());
    }

    ov_event_app_send(self->app, socket, msg);
    msg = ov_json_value_free(msg);
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_loop(void *userdata, const char *event_name, int socket,
                             ov_json_value *input){

    UNUSED(event_name);
    UNUSED(socket);

    struct userdata *self = (struct userdata*) userdata;

    char *str = ov_json_value_to_string(input);
    ov_log_debug("%s", str);
    str = ov_data_pointer_free(str);

    input = ov_json_value_free(input);
    
    ov_event_loop_stop(self->loop);    
    return;
}

/*----------------------------------------------------------------------------*/

static bool register_app_events(struct userdata *userdata){

    if (!userdata || !userdata->app) goto error;

    ov_event_app_register(userdata->app,
        "register",
        userdata,
        cb_register);

    ov_event_app_register(userdata->app,
        "loop",
        userdata,
        cb_loop);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {

    struct userdata userdata = {0};

    int retval = EXIT_FAILURE;

    char *on = NULL;
    char *host = NULL;
    uint16_t port = 0;

    if (!read_user_input(argc, argv, &host, &port, &userdata.password, &userdata.loop1, &userdata.loop2, &on))
        goto error;

    if (!on || !host || !userdata.password || !userdata.loop1 || !userdata.loop2) {
        print_usage();
        goto error;
    }

    if (0 == ov_string_compare(on, "on"))
        userdata.on = true;

    ov_event_loop_config loop_config = (ov_event_loop_config){
        .max.sockets = ov_socket_get_max_supported_runtime_sockets(0),
        .max.timers = ov_socket_get_max_supported_runtime_sockets(0)};

    userdata.loop = ov_os_event_loop(loop_config);

    if (!userdata.loop) {
        ov_log_error("Failed to create eventloop");
        goto error;
    }

    if (!ov_event_loop_setup_signals(userdata.loop))
        goto error;

    ov_socket_configuration socket_config =
        (ov_socket_configuration){.port = port, .type = TCP, .host = {0}};

    strncpy(socket_config.host, host, OV_HOST_NAME_MAX);

    userdata.io = ov_io_create((ov_io_config){
        .loop = userdata.loop
    });

    if (!userdata.io) goto error;

    userdata.app = ov_event_app_create((ov_event_app_config){
        .io = userdata.io,
        .callbacks.userdata = &userdata,
        .callbacks.connected = cb_connected
    });

    if (!userdata.app) goto error;

    if (!register_app_events(&userdata)) goto error;

    ov_event_app_open_connection(
        userdata.app,
        (ov_io_socket_config){
            .socket = socket_config
        });

    /*  Run event loop */
    userdata.loop->run(userdata.loop, OV_RUN_MAX);

    retval = EXIT_SUCCESS;

error:
    host = ov_data_pointer_free(host);
    userdata.loop1 = ov_data_pointer_free(userdata.loop1);
    userdata.loop2 = ov_data_pointer_free(userdata.loop2);
    userdata.password = ov_data_pointer_free(userdata.password);
    userdata.io = ov_io_free(userdata.io);
    userdata.app = ov_event_app_free(userdata.app);
    userdata.loop = ov_event_loop_free(userdata.loop);
    return retval;
}
