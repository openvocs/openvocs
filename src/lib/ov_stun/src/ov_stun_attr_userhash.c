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
        @file           ov_stun_attr_userhash.c
        @author         Markus Töpfer

        @date           2022-01-22


        ------------------------------------------------------------------------
*/
#include "../include/ov_stun_attr_userhash.h"

bool ov_stun_attr_is_userhash(const uint8_t *buffer, size_t length) {

    if (!buffer || length < 8) goto error;

    uint16_t type = ov_stun_attribute_get_type(buffer, length);
    // int64_t size = ov_stun_attribute_get_length(buffer, length);

    if (type != STUN_ATTR_USERHASH) goto error;

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

size_t ov_stun_attr_userhash_encoding_length(size_t data) {

    size_t pad = 0;
    pad = data % 4;
    if (pad != 0) pad = 4 - pad;

    return data + 4 + pad;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_attr_userhash_encode(uint8_t *buffer,
                                  size_t length,
                                  uint8_t **next,
                                  const uint8_t *data,
                                  size_t size) {

    if (!buffer || !data || size < 1) goto error;

    size_t len = ov_stun_attr_userhash_encoding_length(size);

    if (length < len) goto error;

    return ov_stun_attribute_encode(
        buffer, length, next, STUN_ATTR_USERHASH, data, size);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_attr_userhash_decode(const uint8_t *buffer,
                                  size_t length,
                                  uint8_t **data,
                                  size_t *size) {

    if (!buffer || length < 8 || !data || !size) goto error;

    if (!ov_stun_attr_is_userhash(buffer, length)) goto error;

    int64_t len = ov_stun_attribute_get_length(buffer, length);
    if (len <= 0) goto error;

    *size = (size_t)len;
    *data = (uint8_t *)buffer + 4;

    return true;
error:
    return false;
}
