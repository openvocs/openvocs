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
        @file           ov_turn_attr_additional_address_family.c
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#include "../include/ov_turn_attr_additional_address_family.h"

bool ov_turn_attr_is_additional_addr_family(const uint8_t *buffer,
                                            size_t length) {

    if (!buffer || length < 8) goto error;

    uint16_t type = ov_stun_attribute_get_type(buffer, length);
    int64_t size = ov_stun_attribute_get_length(buffer, length);

    if (type != TURN_ADDITIONAL_ADDRESS_FAMILY) goto error;

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

size_t ov_turn_attr_additional_addr_family_encoding_length() { return 8; }

/*----------------------------------------------------------------------------*/

bool ov_turn_attr_additional_addr_family_encode(uint8_t *buffer,
                                                size_t length,
                                                uint8_t **next,
                                                uint16_t number) {

    if (!buffer) goto error;

    size_t len = ov_turn_attr_additional_addr_family_encoding_length();

    if (length < len) goto error;

    switch (number) {

        case 0x01:
        case 0x02:
            break;
        default:
            goto error;
    }

    uint8_t buf[4] = {0};
    buf[0] = number;

    return ov_stun_attribute_encode(
        buffer, length, next, TURN_ADDITIONAL_ADDRESS_FAMILY, buf, 4);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_turn_attr_additional_addr_family_decode(const uint8_t *buffer,
                                                size_t length,
                                                uint16_t *number) {

    if (!buffer || length < 8 || !number) goto error;

    if (!ov_turn_attr_is_additional_addr_family(buffer, length)) goto error;

    uint16_t num = 0;
    num = buffer[4];

    *number = num;
    return true;
error:
    return false;
}
