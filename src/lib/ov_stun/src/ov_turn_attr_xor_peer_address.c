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
        @file           ov_turn_attr_xor_peer_address.c
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#include "../include/ov_turn_attr_xor_peer_address.h"

/*
 *      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

bool ov_turn_attr_frame_is_xor_peer_address(const uint8_t *buffer,
                                            size_t length) {

    if (!buffer || length < 12) goto error;

    uint16_t type = ov_stun_attribute_get_type(buffer, length);
    int64_t size = ov_stun_attribute_get_length(buffer, length);

    if (type != TURN_XOR_PEER_ADDRESS) goto error;

    // check family + size
    switch (*(uint8_t *)(buffer + 5)) {

        case 0x01:

            if (size != 8) goto error;
            break;

        case 0x02:

            if (size != 20) goto error;
            break;

        default:
            goto error;
    }

    return true;

error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *                        SERIALIZATION / DESERIALIZATION
 *
 *      ------------------------------------------------------------------------
 */

size_t ov_turn_attr_xor_peer_address_encoding_length(
    const struct sockaddr_storage *sa) {

    if (!sa) goto error;

    if (sa->ss_family == AF_INET) return 12;

    if (sa->ss_family == AF_INET6) return 24;

error:
    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_turn_attr_xor_peer_address_encode(uint8_t *buffer,
                                          size_t length,
                                          const uint8_t *head,
                                          uint8_t **next,
                                          const struct sockaddr_storage *sa) {

    if (!buffer || !sa || !head || length < 12) goto error;

    struct sockaddr_in *sock4 = NULL;
    struct sockaddr_in6 *sock6 = NULL;

    if (!ov_stun_attribute_set_type(buffer, length, TURN_XOR_PEER_ADDRESS))
        goto error;

    uint16_t port = 0;
    uint32_t ip = 0;
    uint8_t *content = buffer + 4;

    switch (sa->ss_family) {

        case AF_INET:

            if (!ov_stun_attribute_set_length(buffer, length, 8)) goto error;

            sock4 = (struct sockaddr_in *)sa;

            content[0] = 0;
            content[1] = 1;

            port = ntohs(sock4->sin_port);
            port = port ^ 0x2112;
            port = htons(port);

            if (!memcpy(content + 2, &port, 2)) goto error;

            ip = ntohl(sock4->sin_addr.s_addr);
            ip = ip ^ 0x2112A442;
            ip = htonl(ip);

            if (!memcpy(content + 4, &ip, sizeof(struct in_addr))) goto error;

            if (next) *next = buffer + 4 + 8;

            break;

        case AF_INET6:

            if (length < 24) goto error;

            if (!ov_stun_attribute_set_length(buffer, length, 20)) goto error;

            sock6 = (struct sockaddr_in6 *)sa;

            content[0] = 0;
            content[1] = 2;

            port = ntohs(sock6->sin6_port);
            port = port ^ 0x2112;
            port = htons(port);

            if (!memcpy(content + 2, &port, 2)) goto error;

            if (!memcpy(
                    content + 4, &sock6->sin6_addr, sizeof(struct in6_addr)))
                goto error;

            for (int i = 0; i < 16; i++) {

                // XOR with MAGIC COOKIE and TRANSACTION ID
                content[4 + i] = content[4 + i] ^ head[4 + i];
            }

            if (next) *next = buffer + 4 + 20;

            break;

        default:
            goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_turn_attr_xor_peer_address_decode(const uint8_t *buffer,
                                          size_t length,
                                          const uint8_t *head,
                                          struct sockaddr_storage **address) {

    bool created = false;
    if (!buffer || !head || length < 12 || !address) goto error;

    if (!ov_turn_attr_frame_is_xor_peer_address(buffer, length)) goto error;

    if (!*address) {

        *address = calloc(1, sizeof(struct sockaddr_storage));
        if (!*address) goto error;

        created = true;
    }

    struct sockaddr_storage *sa = *address;
    struct sockaddr_in *sock4 = NULL;
    struct sockaddr_in6 *sock6 = NULL;

    uint16_t port = 0;
    uint32_t ip = 0;
    uint8_t *content = (uint8_t *)buffer + 4;

    memset(sa, 0, sizeof(struct sockaddr_storage));

    switch (content[1]) {

        case 0x01:

            // IPv4
            sock4 = (struct sockaddr_in *)sa;
            sock4->sin_family = AF_INET;

            port = ntohs(*(uint16_t *)(content + 2));
            sock4->sin_port = htons(port ^ 0x2112);

            ip = ntohl(*(uint32_t *)(content + 4));
            sock4->sin_addr.s_addr = htonl(ip ^ 0x2112A442);

            break;

        case 0x02:

            // IPv6
            sock6 = (struct sockaddr_in6 *)sa;
            sock6->sin6_family = AF_INET6;

            port = ntohs(*(uint16_t *)(content + 2));
            sock6->sin6_port = htons(port ^ 0x2112);

            if (!memcpy(&sock6->sin6_addr.s6_addr, (uint8_t *)content + 4, 16))
                goto error;

            /*
             *      TBD TBV - verify (test) the following is Ok for
             *      IPv6 on little endian platforms,
             *      as well as non linux platforms.
             *
             *      This function applies the concatinated
             *      XOR to the 128 bit s6_addr.
             *
             *      RFC5389 - turn:
             *
             *              Take 128 bit address in host byte order
             *              XOR concat of cookie and transaction_id
             *              and transcode back to network byte order
             *
             *      RFC2553 - Basic Socket Interface Extensions for IPv6:
             *      /RFC3493
             *
             *              IPv6 shall always be stored always in
             *              network byte order.
             *
             *              That means: there is no host byte order
             *              at all for IPv6 addresses.
             *
             */

            for (int i = 0; i < 16; i++) {

                // XOR with MAGIC COOKIE and TRANSACTION ID

                sock6->sin6_addr.s6_addr[i] =
                    sock6->sin6_addr.s6_addr[i] ^ head[4 + i];
            }

            break;
        default:
            goto error;
    }

    return true;

error:
    if (address) {
        if (created) {
            free(*address);
            *address = NULL;
        }
    }
    return false;
}