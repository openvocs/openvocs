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
        @file           ov_stun_nonce.c
        @author         Markus Toepfer

        @date           2018-10-22

        @ingroup        ov_stun

        @brief          Implementation of RFC 5389 attribute "nonce"


        ------------------------------------------------------------------------
*/
#include "../include/ov_stun_nonce.h"

#include <ov_base/ov_base64.h>

//                        1234567890
static const char *pre = "obMatJos2";
static size_t pre_len = 9;

/*
 *      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_attribute_frame_is_nonce(const uint8_t *buffer, size_t length) {

    if (!buffer || length < 8)
        goto error;

    uint16_t type = ov_stun_attribute_get_type(buffer, length);
    int64_t size = ov_stun_attribute_get_length(buffer, length);

    if (type != STUN_NONCE)
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

bool ov_stun_nonce_validate(const uint8_t *buffer, size_t length) {

    if (!buffer || length < 1 || length > 763)
        return false;

    // MUST be content of a RFC3629 quoted string
    return ov_stun_grammar_is_quoted_string_content(buffer, length);
}

/*      ------------------------------------------------------------------------
 *
 *                              RANDOM CREATION
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_nonce_fill(uint8_t *start, size_t length) {

    if (!start || length < 1 || length > 763)
        return false;

    uint8_t *ptr = start;
    uint64_t codepoint = 0;

    // seed random with time
    srandom(ov_time_get_current_time_usecs());

    uint64_t used = 0;
    uint64_t open = length;

    for (size_t i = 0; i < length; i++) {

        open = length - (ptr - start);

        codepoint = random();

        if (open == 1) {

            codepoint = (codepoint * 0xFF) / RAND_MAX;

        } else {
            // get a random value of the basic multilingual UTF8 plane
            codepoint = (codepoint * 0xFFFF) / RAND_MAX;
        }

        if (codepoint > 0x7F) {

            // UTF8
            if (!ov_utf8_encode_code_point(codepoint, &ptr, open, &used)) {

                // reset and try next random number
                i--;

            } else {

                ptr += used;
                i += used - 1;
            }

        } else {

            // ASCII
            ptr[0] = codepoint;

            if (!ov_stun_grammar_is_qdtext(ptr, 1)) {

                // reset and try next random number
                i--;
                continue;
            }

            ptr++;
        }
    }

    // verify valid result
    if (ov_stun_grammar_is_quoted_string_content(start, length))
        return true;

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_nonce_fill_rfc8489(uint8_t *start, size_t length, uint32_t sec) {

    if (!start || length < 14 || length > 763 || sec > 0xffffff)
        goto error;

    if (!snprintf((char *)start, length, "%s", pre))
        goto error;

    uint8_t buf[3] = {0};
    buf[0] = sec >> 16;
    buf[1] = sec >> 8;
    buf[2] = sec;

    size_t dest_len = length - pre_len;
    uint8_t *dest = start + pre_len;

    if (!ov_base64_encode(buf, 3, &dest, &dest_len))
        goto error;

    if (!ov_stun_nonce_fill(start + 13, length - 13))
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

size_t ov_stun_nonce_encoding_length(const uint8_t *nonce, size_t length) {

    if (!nonce || length == 0 || length > 763)
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

bool ov_stun_nonce_encode(uint8_t *buffer, size_t length, uint8_t **next,
                          const uint8_t *nonce, size_t size) {

    if (!buffer || !nonce || size > 763 || size == 0)
        goto error;

    size_t len = ov_stun_nonce_encoding_length(nonce, size);

    if (length < len)
        goto error;

    if (!ov_stun_nonce_validate(nonce, size))
        goto error;

    return ov_stun_attribute_encode(buffer, length, next, STUN_NONCE, nonce,
                                    size);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_nonce_decode(const uint8_t *buffer, size_t length, uint8_t **nonce,
                          size_t *size) {

    if (!buffer || length < 5 || !nonce || !size)
        goto error;

    if (!ov_stun_attribute_frame_is_nonce(buffer, length))
        goto error;

    *size = ov_stun_attribute_get_length(buffer, length);
    *nonce = (uint8_t *)buffer + 4;

    if (*size == 0)
        goto error;

    if (!ov_stun_nonce_validate(*nonce, *size))
        goto error;

    return true;

error:
    if (nonce)
        *nonce = NULL;
    if (size)
        *size = 0;
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_nonce_is_rfc8489(const uint8_t *buffer, size_t length) {

    if (!buffer || length < 5)
        goto error;

    if (!ov_stun_attribute_frame_is_nonce(buffer, length))
        goto error;

    size_t size = ov_stun_attribute_get_length(buffer, length);
    uint8_t *nonce = (uint8_t *)buffer + 4;

    if (!ov_stun_nonce_validate(nonce, size))
        goto error;

    if (size < pre_len)
        goto error;

    if (0 == memcmp(nonce, pre, pre_len))
        return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_nonce_decode_rfc8489_password_algorithm_set(const uint8_t *buffer,
                                                         size_t length) {

    if (!buffer || length < pre_len + 5)
        goto error;

    uint8_t buf[3] = {0};
    uint8_t *ptr = buf;
    size_t len = 3;

    if (0 != memcmp(buffer, pre, pre_len))
        goto error;

    if (!ov_base64_decode(buffer + pre_len, 4, &ptr, &len))
        goto error;

    if (len != 3)
        goto error;

    if (buf[2] & 0x01)
        return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_nonce_decode_rfc8489_username_anonymity_set(const uint8_t *buffer,
                                                         size_t length) {

    if (!buffer || length < pre_len + 5)
        goto error;

    uint8_t buf[3] = {0};
    uint8_t *ptr = buf;
    size_t len = 3;

    if (0 != memcmp(buffer, pre, pre_len))
        goto error;

    if (!ov_base64_decode(buffer + pre_len, 4, &ptr, &len))
        goto error;

    if (len != 3)
        goto error;

    if (buf[2] & 0x02)
        return true;

error:
    return false;
}