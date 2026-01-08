/***
        ------------------------------------------------------------------------

        Copyright (c) 2018 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_stun_software.c
        @author         Markus Toepfer

        @date           2018-10-23

        @ingroup        ov_stun

        @brief          Implementation of RFC 5389 attribute "software"


        ------------------------------------------------------------------------
*/
#include "../include/ov_stun_software.h"

/*
 *      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_attribute_frame_is_software(const uint8_t *buffer, size_t length) {

    if (!buffer || length < 8)
        goto error;

    uint16_t type = ov_stun_attribute_get_type(buffer, length);
    int64_t size = ov_stun_attribute_get_length(buffer, length);

    if (type != STUN_SOFTWARE)
        goto error;

    if (size < 1 || size > 763)
        goto error;

    if (length < (size_t)size + 4)
        goto error;

    return true;

error:
    return false;
}

/*      ------------------------------------------------------------------------
 *
 *                              VALIDATION
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_software_validate(const uint8_t *buffer, size_t length) {

    if (!buffer || length < 1 || length > 763)
        return false;

    return ov_utf8_validate_sequence(buffer, length);
}

/*
 *      ------------------------------------------------------------------------
 *
 *                        SERIALIZATION / DESERIALIZATION
 *
 *      ------------------------------------------------------------------------
 */

size_t ov_stun_software_encoding_length(const uint8_t *software,
                                        size_t length) {

    if (!software || length == 0 || length > 763)
        goto error;

    size_t pad = 0;
    pad = length % 4;
    if (pad != 0)
        pad = 4 - pad;

    return length + 4 + pad;
error:
    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_software_encode(uint8_t *buffer, size_t length, uint8_t **next,
                             const uint8_t *software, size_t size) {

    if (!buffer || !software || size > 763 || size == 0)
        goto error;

    size_t len = ov_stun_software_encoding_length(software, size);

    if (length < len)
        goto error;

    if (!ov_stun_software_validate(software, size))
        goto error;

    return ov_stun_attribute_encode(buffer, length, next, STUN_SOFTWARE,
                                    software, size);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_software_decode(const uint8_t *buffer, size_t length,
                             uint8_t **software, size_t *size) {

    if (!buffer || length < 5 || !software || !size)
        goto error;

    if (!ov_stun_attribute_frame_is_software(buffer, length))
        goto error;

    *size = ov_stun_attribute_get_length(buffer, length);
    *software = (uint8_t *)buffer + 4;

    if (*size == 0)
        goto error;

    if (!ov_stun_software_validate(*software, *size))
        goto error;

    return true;

error:
    if (software)
        *software = NULL;
    if (size)
        *size = 0;
    return false;
}