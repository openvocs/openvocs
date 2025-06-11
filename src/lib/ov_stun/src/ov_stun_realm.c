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
        @file           ov_stun_realm.c
        @author         Markus Toepfer

        @date           2018-10-19

        @ingroup        ov_stun

        @brief          Implementation of RFC 5389 attribute "realm"


        ------------------------------------------------------------------------
*/
#include "../include/ov_stun_realm.h"

/*
 *      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_attribute_frame_is_realm(const uint8_t *buffer, size_t length) {

    if (!buffer || length < 8) goto error;

    uint16_t type = ov_stun_attribute_get_type(buffer, length);
    int64_t size = ov_stun_attribute_get_length(buffer, length);

    if (type != STUN_REALM) goto error;

    if (size < 1 || size > 763) goto error;

    if (length < (size_t)size + 4) goto error;

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

bool ov_stun_realm_validate(const uint8_t *buffer, size_t length) {

    if (!buffer || length < 1 || length > 763) return false;

    // MUST be content of a RFC3629 quoted string
    return ov_stun_grammar_is_quoted_string_content(buffer, length);
}

/*
 *      ------------------------------------------------------------------------
 *
 *                        SERIALIZATION / DESERIALIZATION
 *
 *      ------------------------------------------------------------------------
 */

size_t ov_stun_realm_encoding_length(const uint8_t *realm, size_t length) {

    if (!realm || length == 0 || length > 763) goto error;

    size_t pad = 0;
    pad = length % 4;
    if (pad != 0) pad = 4 - pad;

    return length + 4 + pad;
error:
    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_realm_encode(uint8_t *buffer,
                          size_t length,
                          uint8_t **next,
                          const uint8_t *realm,
                          size_t size) {

    if (!buffer || !realm || size > 763 || size == 0) goto error;

    size_t len = ov_stun_realm_encoding_length(realm, size);

    if (length < len) goto error;

    if (!ov_stun_realm_validate(realm, size)) goto error;

    return ov_stun_attribute_encode(
        buffer, length, next, STUN_REALM, realm, size);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_realm_decode(const uint8_t *buffer,
                          size_t length,
                          uint8_t **realm,
                          size_t *size) {

    if (!buffer || length < 5 || !realm || !size) goto error;

    if (!ov_stun_attribute_frame_is_realm(buffer, length)) goto error;

    *size = ov_stun_attribute_get_length(buffer, length);
    *realm = (uint8_t *)buffer + 4;

    if (*size == 0) goto error;

    if (!ov_stun_realm_validate(*realm, *size)) goto error;

    return true;

error:
    if (realm) *realm = NULL;
    if (size) *size = 0;
    return false;
}