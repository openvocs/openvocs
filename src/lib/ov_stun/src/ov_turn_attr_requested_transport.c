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
        @file           ov_turn_attr_requested_transport.c
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#include "../include/ov_turn_attr_requested_transport.h"

bool ov_turn_attr_is_requested_transport(const uint8_t *buffer, size_t length) {

    if (!buffer || length < 8)
        goto error;

    uint16_t type = ov_stun_attribute_get_type(buffer, length);
    int64_t size = ov_stun_attribute_get_length(buffer, length);

    if (type != TURN_REQUESTED_TRANSPORT)
        goto error;

    if (size != 4)
        goto error;

    if (length < (size_t)size + 4)
        goto error;

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

size_t ov_turn_attr_requested_transport_encoding_length() { return 8; }

/*----------------------------------------------------------------------------*/

bool ov_turn_attr_requested_transport_encode(uint8_t *buffer, size_t length,
                                             uint8_t **next, uint8_t protocol) {

    if (!buffer)
        goto error;

    size_t len = ov_turn_attr_requested_transport_encoding_length();

    if (length < len)
        goto error;

    uint8_t buf[4] = {0};
    buf[0] = protocol;

    return ov_stun_attribute_encode(buffer, length, next,
                                    TURN_REQUESTED_TRANSPORT, buf, 4);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_turn_attr_requested_transport_decode(const uint8_t *buffer,
                                             size_t length, uint8_t *protocol) {

    if (!buffer || length < 8 || !protocol)
        goto error;

    if (!ov_turn_attr_is_requested_transport(buffer, length))
        goto error;

    *protocol = buffer[4];
    return true;
error:
    return false;
}