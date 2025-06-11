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
        @file           ov_stun_attr_password-algorithm.c
        @author         Markus Töpfer

        @date           2022-01-22


        ------------------------------------------------------------------------
*/
#include "../include/ov_stun_attr_password_algorithm.h"

bool ov_stun_attr_is_password_algorithm(const uint8_t *buffer, size_t length) {

    if (!buffer || length < 8) goto error;

    uint16_t type = ov_stun_attribute_get_type(buffer, length);
    // int64_t size = ov_stun_attribute_get_length(buffer, length);

    if (type != STUN_ATTR_PASSWORD_ALGORITHM) goto error;

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

size_t ov_stun_attr_password_algorithm_encoding_length(size_t password) {

    size_t pad = 0;
    pad = password % 4;
    if (pad != 0) pad = 4 - pad;

    return password + 8 + pad;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_attr_password_algorithm_encode(uint8_t *buffer,
                                            size_t length,
                                            uint8_t **next,
                                            uint16_t algorithm,
                                            uint16_t parameter_length,
                                            const uint8_t *parameter) {

    size_t size = parameter_length + 4;
    uint8_t buf[size];
    memset(buf, 0, size);

    if (!buffer || size < 1 || !parameter) goto error;

    size_t len =
        ov_stun_attr_password_algorithm_encoding_length(parameter_length);

    if (length < len) goto error;

    buf[0] = algorithm >> 8;
    buf[1] = algorithm;
    buf[2] = parameter_length >> 8;
    buf[3] = parameter_length;

    memcpy(buf + 4, parameter, size - 4);

    return ov_stun_attribute_encode(
        buffer, length, next, STUN_ATTR_PASSWORD_ALGORITHM, buf, size);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_attr_password_algorithm_decode(const uint8_t *buffer,
                                            size_t length,
                                            uint16_t *algorithm,
                                            uint16_t *size,
                                            const uint8_t **params) {

    if (!buffer || length < 8 || !algorithm || !size || !params) goto error;

    if (!ov_stun_attr_is_password_algorithm(buffer, length)) goto error;

    int64_t len = ov_stun_attribute_get_length(buffer, length);
    if (len <= 0) goto error;

    uint16_t num = buffer[4] << 8;
    num += buffer[5];
    *algorithm = num;

    num = buffer[6] << 8;
    num += buffer[7];
    *size = num;
    *params = buffer + 8;

    return true;
error:
    return false;
}
