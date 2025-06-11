/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_mc_socket.c
        @author         Markus Töpfer

        @date           2022-11-09


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_socket.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int ov_mc_socket(ov_socket_configuration config) {

    int socket = -1;

    if (!config.host[0]) goto error;
    config.type = UDP;

    int loop = 1;
    static struct ip_mreq command = {0};

    socket = ov_socket_create(config, false, NULL);
    if (-1 == socket) goto error;

    if (!ov_socket_ensure_nonblocking(socket)) goto error;
    if (!ov_socket_set_reuseaddress(socket)) goto error;

    /* allow broadcast on the machine */

    if (setsockopt(socket, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) <
        0) {
        goto error;
    }

    /* join the broadcast group */

    command.imr_multiaddr.s_addr = inet_addr(config.host);
    command.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(
            socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &command, sizeof(command)) <
        0) {
        goto error;
    }

    return socket;

error:
    if (-1 != socket) close(socket);
    return -1;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_socket_drop_membership(int socket) {

    ov_socket_data data = {0};
    if (!ov_socket_get_data(socket, &data, NULL)) goto error;

    static struct ip_mreq command = {0};
    command.imr_multiaddr.s_addr = inet_addr(data.host);
    command.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(
            socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, &command, sizeof(command)) <
        0) {
        goto error;
    }

    return true;
error:
    return false;
}