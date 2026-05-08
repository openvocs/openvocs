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
        @file           ov_turn_attr_icmp.c
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#include "../include/ov_turn_attr_icmp.h"

bool ov_turn_attr_is_icmp(const uint8_t *buffer, size_t length) {

    if (!buffer || length < 12)
        goto error;

    uint16_t type = ov_stun_attribute_get_type(buffer, length);
    int64_t size = ov_stun_attribute_get_length(buffer, length);

    if (type != TURN_ICMP)
        goto error;

    if (size != 8)
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

size_t ov_turn_attr_icmp_encoding_length() { return 12; }

/*----------------------------------------------------------------------------*/

bool ov_turn_attr_icmp_encode(uint8_t *buffer, size_t length, uint8_t **next,
                              uint8_t type, uint8_t code, uint32_t error) {

    if (!buffer)
        goto error;

    size_t len = ov_turn_attr_icmp_encoding_length();

    if (length < len)
        goto error;

    uint8_t buf[12] = {0};
    buf[2] = type;
    buf[3] = code;
    buf[4] = error >> 24;
    buf[5] = error >> 16;
    buf[6] = error >> 8;
    buf[7] = error;

    return ov_stun_attribute_encode(buffer, length, next, TURN_ICMP, buf, 8);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_turn_attr_icmp_decode(const uint8_t *buffer, size_t length,
                              uint8_t *type, uint8_t *code, uint32_t *error) {

    if (!buffer || length < 12 || !type || !code || !error)
        goto error;

    if (!ov_turn_attr_is_icmp(buffer, length))
        goto error;

    *type = buffer[6];
    *code = buffer[7];

    uint32_t err = buffer[11];
    err += buffer[10] << 8;
    err += buffer[9] << 16;
    err += buffer[8] << 24;

    *error = err;
    return true;
error:
    return false;
}