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
        @file           ov_stun_message_integrity.c
        @author         Markus Toepfer

        @date           2018-10-22

        @ingroup        ov_stun

        @brief          Implementation of RFC 5389 attribute "message integrity"


        ------------------------------------------------------------------------
*/
#include "../include/ov_stun_message_integrity.h"

/*      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_attribute_frame_is_message_integrity(const uint8_t *buffer,
                                                  size_t length) {

    if (!buffer || length < 24) goto error;

    uint16_t type = ov_stun_attribute_get_type(buffer, length);
    int64_t size = ov_stun_attribute_get_length(buffer, length);

    if (type != STUN_MESSAGE_INTEGRITY) goto error;

    if (size != 20) goto error;

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

size_t ov_stun_message_integrity_encoding_length() { return 24; }

/*----------------------------------------------------------------------------*/

bool ov_stun_add_message_integrity(uint8_t *head,
                                   size_t length,
                                   uint8_t *start,
                                   uint8_t **next,
                                   const uint8_t *key,
                                   size_t keylen) {

    if (!head || !start || !key || length < 44 || keylen < 1) goto error;

    uint8_t *content = start + 4;
    size_t len = (start - head) + 24;
    size_t hmac_len = 0;

    if (len > length) goto error;

    // not starting at multiple of 32 bit
    if (((start - head) % 4) != 0) goto error;

    if (!ov_stun_attribute_set_type(start, 24, STUN_MESSAGE_INTEGRITY))
        goto error;

    if (!ov_stun_attribute_set_length(start, 24, 20)) goto error;

    // set length including message integrity, excluding the header
    if (!ov_stun_frame_set_length(head, length, len - 20)) goto error;

    // write HMAC

    if (!ov_hmac(OV_HASH_SHA1, head, len - 24, key, keylen, content, &hmac_len))
        goto error;

    // check HMAC length
    if (hmac_len != 20) goto error;

    if (next) *next = start + 24;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_check_message_integrity(uint8_t *head,
                                     size_t length,
                                     uint8_t *attr[],
                                     size_t attr_size,
                                     uint8_t *key,
                                     size_t keylen,
                                     bool must_be_set) {

    if (!head || length < 44 || !attr || attr_size < 1) return false;

    uint8_t *integrity = NULL;
    size_t original_length = 0;
    size_t validate_length = 0;
    size_t len = 0;

    for (size_t i = 0; i < attr_size; i++) {

        // ignore all following attributes
        if (integrity) attr[i] = NULL;

        if (ov_stun_attribute_frame_is_message_integrity(
                attr[i], length - (attr[i] - head))) {

            integrity = attr[i];

            // allow fingerprint as follower
            if (i + 1 >= attr_size) break;

            if (ov_stun_attribute_frame_is_fingerprint(
                    attr[i + 1], length - (attr[i + 1] - head)))
                i++;
        }
    }

    if (!integrity) {

        if (must_be_set) goto error;

        // nothing to check
        return true;
    }

    if (!key || keylen < 1) goto error;

    uint8_t hmac_buffer[20] = {0};

    // save original length
    original_length = ov_stun_frame_get_length(head, length);
    validate_length = (integrity - head) + 4; // + 24 - 20

    if (!ov_stun_frame_set_length(head, length, validate_length)) goto error;

    if (!ov_hmac(OV_HASH_SHA1,
                 head,
                 (integrity - head),
                 key,
                 keylen,
                 hmac_buffer,
                 &len))
        goto error;

    // check HMAC
    if (0 != memcmp(hmac_buffer, integrity + 4, 20)) return false;

    // reset original length
    if (!ov_stun_frame_set_length(head, length, original_length)) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/
