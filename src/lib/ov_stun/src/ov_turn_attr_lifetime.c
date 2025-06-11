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
        @file           ov_turn_attr_lifetime.c
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#include "../include/ov_turn_attr_lifetime.h"

bool ov_turn_attr_is_lifetime(const uint8_t *buffer, size_t length) {

    if (!buffer || length < 8) goto error;

    uint16_t type = ov_stun_attribute_get_type(buffer, length);
    int64_t size = ov_stun_attribute_get_length(buffer, length);

    if (type != TURN_LIFETIME) goto error;

    if (size != 4) goto error;

    if (length < (size_t)size + 4) goto error;

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

size_t ov_turn_attr_lifetime_encoding_length() { return 8; }

/*----------------------------------------------------------------------------*/

bool ov_turn_attr_lifetime_encode(uint8_t *buffer,
                                  size_t length,
                                  uint8_t **next,
                                  uint32_t number) {

    if (!buffer) goto error;

    size_t len = ov_turn_attr_lifetime_encoding_length();

    if (length < len) goto error;

    uint8_t buf[4] = {0};
    buf[0] = number >> 24;
    buf[1] = number >> 16;
    buf[2] = number >> 8;
    buf[3] = number;

    return ov_stun_attribute_encode(
        buffer, length, next, TURN_LIFETIME, buf, 4);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_turn_attr_lifetime_decode(const uint8_t *buffer,
                                  size_t length,
                                  uint32_t *number) {

    if (!buffer || length < 8 || !number) goto error;

    if (!ov_turn_attr_is_lifetime(buffer, length)) goto error;

    uint32_t num = 0;

    num = buffer[7];
    num += (buffer[6] << 8);
    num += (buffer[5] << 16);
    num += (buffer[4] << 24);

    *number = num;
    return true;
error:
    return false;
}