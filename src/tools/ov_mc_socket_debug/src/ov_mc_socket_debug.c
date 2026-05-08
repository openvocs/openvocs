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
        @file           ov_mc_socket_debug.c
        @author         Markus Töpfer

        @date           2025-01-24


        ------------------------------------------------------------------------
*/

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_mc_socket.h>
#include <ov_base/ov_rtp_frame.h>
#include <ov_base/ov_string.h>

#include <ov_base/ov_event_loop.h>
#include <ov_os/ov_os_event_loop.h>

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>

static void print_usage() {

    fprintf(stdout, "\n");
    fprintf(stdout, "Debug a multicast socket and print RTP Frame data.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "USAGE              [OPTIONS]\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "               -i,     --ip        multicast ip to use\n");
    fprintf(stdout, "               -p,     --port      port to use\n");

    return;
}

/*----------------------------------------------------------------------------*/

bool read_user_input(int argc, char *argv[], uint16_t *port, char **host) {

    int c = 0;
    int option_index = 0;

    while (1) {

        static struct option long_options[] = {

            /* These options don’t set a flag.
               We distinguish them by their indices. */
            {"ip", required_argument, 0, 'i'},
            {"port", required_argument, 0, 'p'},
            {"help", optional_argument, 0, 'h'},
            {0, 0, 0, 0}};

        /* getopt_long stores the option index here. */

        c = getopt_long(argc, argv, "i:p:?h", long_options, &option_index);

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

static bool cb_socket_io(int socket, uint8_t events, void *userdata) {

    UNUSED(userdata);

    if ((events & OV_EVENT_IO_ERR) || (events & OV_EVENT_IO_CLOSE)) {

        ov_log_error("Socket closed.");
        goto error;
    }

    uint8_t buffer[OV_UDP_PAYLOAD_OCTETS] = {0};
    ov_socket_data in = {0};
    socklen_t in_len = 0;

    ssize_t bytes = recvfrom(socket, buffer, OV_UDP_PAYLOAD_OCTETS, 0,
                             (struct sockaddr *)&in.sa, &in_len);

    in = ov_socket_data_from_sockaddr_storage(&in.sa);

    switch (bytes) {

    case 0:
        goto error;

    case -1:
        goto done;

    default:
        break;
    }

    ov_rtp_frame *frame = ov_rtp_frame_decode(buffer, bytes);
    if (!frame)
        goto done;

    ov_log_debug("IO from SSRC %i, seq %i bytes %i from %s:%i",
                 frame->expanded.ssrc, frame->expanded.sequence_number,
                 frame->expanded.payload.length, in.host, in.port);

    frame = ov_rtp_frame_free(frame);

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {

    int retval = EXIT_FAILURE;

    ov_event_loop *loop = NULL;

    uint16_t port = 0;
    char *host = NULL;

    if (!read_user_input(argc, argv, &port, &host))
        goto error;

    if (!host || !port) {
        print_usage();
        goto error;
    }

    ov_log_debug("starting Multicast reading on %s:%i", host, port);

    ov_event_loop_config loop_config = (ov_event_loop_config){
        .max.sockets = ov_socket_get_max_supported_runtime_sockets(0),
        .max.timers = ov_socket_get_max_supported_runtime_sockets(0)};

    loop = ov_os_event_loop(loop_config);

    if (!loop) {
        ov_log_error("Failed to create eventloop");
        goto error;
    }

    if (!ov_event_loop_setup_signals(loop))
        goto error;

    ov_socket_configuration socket_config =
        (ov_socket_configuration){.port = port, .type = UDP, .host = {0}};

    strncpy(socket_config.host, host, OV_HOST_NAME_MAX);

    int socket = ov_mc_socket(socket_config);
    if (-1 == socket) {
        ov_log_error("Failed to open socket.");
        goto error;
    }

    ov_socket_ensure_nonblocking(socket);

    if (!ov_event_loop_set(loop, socket,
                           OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
                           NULL, cb_socket_io)) {

        ov_log_debug("Failed to set IO callback.");
        goto error;
    }

    /*  Run event loop */
    loop->run(loop, OV_RUN_MAX);

    retval = EXIT_SUCCESS;
error:
    host = ov_data_pointer_free(host);
    return retval;
}
