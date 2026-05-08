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
        @file           ov_turn_attr_address_error_code.c
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#include "../include/ov_turn_attr_address_error_code.h"

bool ov_turn_attr_is_address_error_code(const uint8_t *buffer, size_t length) {

    if (!buffer || length < 12)
        goto error;

    uint16_t type = ov_stun_attribute_get_type(buffer, length);
    // int64_t size = ov_stun_attribute_get_length(buffer, length);

    if (type != TURN_ADDRESS_ERROR_CODE)
        goto error;

    switch (buffer[4]) {

    case 0x01:
    case 0x02:
        break;
    default:
        goto error;
    }

    // check class is set
    if (0 == buffer[7])
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

size_t ov_turn_attr_address_error_code_encoding_length(size_t phrase) {
    return 8 + phrase;
}

/*----------------------------------------------------------------------------*/

bool ov_turn_attr_address_error_code_encode(uint8_t *buffer, size_t length,
                                            uint8_t **next, uint8_t family,
                                            uint16_t code,
                                            const uint8_t *phrase,
                                            size_t phrase_len) {

    uint8_t buf[100 + OV_TURN_PHRASE_MAX] = {0};

    if (!buffer || !phrase || (phrase_len > OV_TURN_PHRASE_MAX))
        goto error;

    size_t len = ov_turn_attr_address_error_code_encoding_length(phrase_len);

    if (length < len)
        goto error;

    switch (family) {

    case 0x01:
    case 0x02:
        break;
    default:
        goto error;
    }

    if ((code < 99) || (code > 999))
        goto error;

    buf[0] = family;
    buf[2] = code / 100;
    buf[3] = code % 100;
    memcpy(buf + 4, phrase, phrase_len);

    return ov_stun_attribute_encode(
        buffer, length, next, TURN_ADDRESS_ERROR_CODE, buf, 4 + phrase_len);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_turn_attr_address_error_code_decode(const uint8_t *buffer,
                                            size_t length, uint8_t *family,
                                            uint16_t *number,
                                            const uint8_t **phrase,
                                            size_t *phrase_len) {

    if (!buffer || length < 8 || !number || !phrase || !phrase_len || !family)
        goto error;

    if (!ov_turn_attr_is_address_error_code(buffer, length))
        goto error;

    int64_t len = ov_stun_attribute_get_length(buffer, length);
    if (len <= 0)
        goto error;

    *phrase_len = (size_t)len - 4;
    *phrase = (uint8_t *)buffer + 8;

    *family = buffer[4];

    uint16_t num = 0;
    num = buffer[6];
    num *= 100;
    num += buffer[7];
    *number = num;

    return true;
error:
    return false;
}
