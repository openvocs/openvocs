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
        @file           ov_mc_test_common.c
        @author         Markus Töpfer

        @date           2022-11-11


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_test_common.h"
#include "../include/ov_socket.h"
#include "../include/ov_utils.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

static bool interface_permit_ipv4(const struct sockaddr_in *sock) {

    OV_ASSERT(sock);

    if (!sock)
        goto error;

    char host[OV_HOST_NAME_MAX + 1] = {0};

    if (!inet_ntop(AF_INET, &sock->sin_addr, host, OV_HOST_NAME_MAX))
        goto error;
    /*
     *      ignore local IP4 interfaces
     */

    if (0 == strncmp("127", host, 3))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool interface_permit_ipv6(const struct sockaddr_in6 *sock) {

    OV_ASSERT(sock);

    if (!sock)
        goto error;

    char host[OV_HOST_NAME_MAX + 1] = {0};

    if (!inet_ntop(AF_INET6, &sock->sin6_addr, host, OV_HOST_NAME_MAX))
        goto error;

    /*
     *      (1) ignore local IP6 interfaces
     */

    bool zero = true;

    if (0xFE & sock->sin6_addr.s6_addr[0]) {

        zero = false;
        if (0x80 == sock->sin6_addr.s6_addr[1]) {

            zero = true;
            for (size_t i = 2; i < 8; i++) {

                if (sock->sin6_addr.s6_addr[i] != 0x00) {
                    zero = false;
                    break;
                }
            }
        }

        // Link-Local IPv6 Unicast Addresses
        if (zero)
            goto error;

        // Site-Local IPv6 Unicast Addresses
        if (0xC0 & sock->sin6_addr.s6_addr[1])
            goto error;
    }

    zero = true;
    for (size_t i = 0; i < 10; i++) {

        if (sock->sin6_addr.s6_addr[i] != 0x00) {
            zero = false;
            break;
        }
    }

    if (zero) {

        // ignore IPv4 compatible
        if ((sock->sin6_addr.s6_addr[10] == 0x00) &&
            (sock->sin6_addr.s6_addr[11] == 0x00))
            goto error;

        // ignore IPv4 mapped
        if ((sock->sin6_addr.s6_addr[10] == 0xFF) &&
            (sock->sin6_addr.s6_addr[11] == 0xFF))
            goto error;

        for (size_t i = 10; i < 15; i++) {

            if (sock->sin6_addr.s6_addr[i] != 0x00) {
                zero = false;
                break;
            }
        }

        if (zero) {

            // loopback address ::1
            if (sock->sin6_addr.s6_addr[15] & 0x01)
                goto error;

            // any address ::0 | ::
            if (sock->sin6_addr.s6_addr[15] & 0x00)
                goto error;
        }
    }

    // done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

int ov_mc_test_common_open_socket(int net) {

    struct ifaddrs *ifaddr = NULL, *ifa = NULL;

    ov_socket_configuration socket_config = {0};
    int socket = -1, n = 0;

    if (getifaddrs(&ifaddr) == -1) {
        goto error;
    }

    for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {

        if (ifa->ifa_addr == NULL)
            continue;

        socket = -1;
        socket_config = (ov_socket_configuration){0};
        socket_config.type = UDP;

        bool ignore = true;

        switch (ifa->ifa_addr->sa_family) {

        case AF_INET:

            if (net == AF_INET6)
                break;

            if (interface_permit_ipv4((struct sockaddr_in *)ifa->ifa_addr)) {

                if (0 == getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                                     socket_config.host, OV_HOST_NAME_MAX, NULL,
                                     0, NI_NUMERICHOST)) {

                    ignore = false;
                }
            }

            break;

        case AF_INET6:

            if (net == AF_INET)
                break;

            if (interface_permit_ipv6((struct sockaddr_in6 *)ifa->ifa_addr)) {

                if (0 == getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6),
                                     socket_config.host, OV_HOST_NAME_MAX, NULL,
                                     0, NI_NUMERICHOST)) {

                    ignore = false;
                }
            }

            break;

        default:
            break;
        }

        if (ignore)
            continue;

        if (0 == socket_config.host[0])
            continue;

        socket = ov_socket_create(socket_config, false, NULL);

        if (-1 == socket) {

            continue;

        } else {

            break;
        }
    }

    freeifaddrs(ifaddr);
    ifaddr = NULL;

    return socket;
error:

    if (ifaddr)
        freeifaddrs(ifaddr);
    return -1;
}
