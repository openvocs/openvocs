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
        @file           ov_stun_frame.c
        @author         Markus Toepfer

        @date           2018-10-18

        @ingroup        ov_stun

        @brief          Implementation of RFC 5389 frame related functions.


        ------------------------------------------------------------------------
*/
#include "../include/ov_stun_frame.h"

bool ov_stun_frame_add_padding(const uint8_t *head, uint8_t **pos) {

    if (!head || !pos || !*pos) return false;

    size_t pad = 0;
    pad = (*pos - head) % 4;
    if (pad != 0) pad = 4 - pad;

    *pos = *pos + pad;
    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

bool ov_stun_frame_is_valid(const uint8_t *buffer, size_t length) {

    if (!buffer || length < 20) goto error;

    // MUST be 32 bit padded
    if (length % 4 != 0) return false;

    // check leading zeros
    if ((buffer[0] & 0x80) || (buffer[0] & 0x40)) return false;

    // check closing zeros
    if ((buffer[3] & 0x01) || (buffer[3] & 0x02)) return false;

    // check magic cookie
    if (buffer[4] != 0x21) return false;

    if (buffer[5] != 0x12) return false;

    if (buffer[6] != 0xA4) return false;

    if (buffer[7] != 0x42) return false;

    // frame length MUST be message length + header
    size_t len = 20 + ov_stun_frame_get_length(buffer, length);
    if (length != len) goto error;

    return true;

error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *                              FRAME SLICING
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_frame_slice(const uint8_t *buffer,
                         size_t length,
                         uint8_t *array[],
                         size_t array_size) {

    if (!buffer || length < 20) goto error;

    if (!array || array_size < 1) goto error;

    size_t len = 0;
    int64_t open = 0;
    uint8_t *ptr = 0;
    int pad = 0;

    open = ov_stun_frame_get_length(buffer, length);
    ptr = (uint8_t *)buffer + 20;

    if (open != (int64_t)length - 20) goto error;

    memset(array, 0, array_size);

    size_t item = 0;
    while (open > 0) {

        array[item] = ptr;
        item++;

        if (item == array_size) goto error;

        if (open < 4) goto error;

        len = ntohs(*(uint16_t *)(ptr + 2));

        // add padding to 32 bit
        pad = len % 4;
        if (pad != 0) pad = 4 - pad;

        len += pad;
        // log("len %jd pad %jd", len, pad);
        if (open < (int64_t)(4 + len)) goto error;

        ptr += (4 + len);
        open -= (4 + len);
    }

    return true;
error:
    if (array) memset(array, 0, array_size);
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *                               MESSAGE LENGTH
 *
 *      ------------------------------------------------------------------------
 */

int64_t ov_stun_frame_get_length(const uint8_t *buffer, size_t length) {

    if (!buffer || length < 4) return -1;

    // use of shift based calculation (network byte order)
    return ntohs(*(uint16_t *)(buffer + 2));
}

/*----------------------------------------------------------------------------*/

bool ov_stun_frame_set_length(uint8_t *head,
                              size_t header_length,
                              uint16_t length) {

    if (!head || header_length < 20) return false;

    *(uint16_t *)(head + 2) = htons(length);
    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *                               TRANSACTION ID
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_frame_generate_transaction_id(uint8_t *buffer) {

    /**

    VALGRIND ERROR when using RAND_bytes,
    most likely related to an BUG, where valdrind hasn't implementent a function
    used by openssl.

    to use the following snipped include openssl/rand.h

    unsigned char *buf = *buffer;

    if (!RAND_bytes(buf, 12))
            return false;

    */

    srandom(ov_time_get_current_time_usecs());
    uint64_t number = 0;

    for (int i = 0; i < 12; i++) {

        number = random();
        number = (number * 0xFF) / RAND_MAX;

        if (number == 0) {
            i--;
        } else {
            buffer[i] = number;
        }
    }

    return buffer;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_frame_set_transaction_id(uint8_t *head,
                                      size_t length,
                                      const uint8_t *id) {

    if (!head || !id || length < 20) return false;

    if (!memcpy(head + 8, id, 12)) return false;

    return true;
}

/*----------------------------------------------------------------------------*/

uint8_t *ov_stun_frame_get_transaction_id(const uint8_t *head, size_t length) {

    if (!head || length < 20) return false;

    return (uint8_t *)head + 8;
}

/*
 *      ------------------------------------------------------------------------
 *
 *                               MAGIC COOKIE
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_frame_has_magic_cookie(const uint8_t *head, size_t length) {

    if (!head || length < 20) return false;

    if (head[4] != 0x21) return false;

    if (head[5] != 0x12) return false;

    if (head[6] != 0xA4) return false;

    if (head[7] != 0x42) return false;

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_frame_set_magic_cookie(uint8_t *head, size_t length) {

    if (!head || length < 20) return false;

    head[4] = 0x21;
    head[5] = 0x12;
    head[6] = 0xA4;
    head[7] = 0x42;

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *                               MESSAGE TYPE
 *
 *      ------------------------------------------------------------------------
 */

uint16_t ov_stun_frame_get_method(const uint8_t *head, size_t length) {

    if (!head || length < 2) return false;

    uint16_t type = 0;

    type = head[1] & 0x0F;
    // shift C out of buffer an right shift back
    type = type + ((head[1] >> 5) << 4);
    // shift C out of buffer and right shift back
    type = type + (((head[0]) >> 1) << 7);

    return type;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_frame_set_method(uint8_t *head, size_t length, uint16_t type) {

    if (!head || length < 20 || type > 0xfff) return false;

    /*
     *  0                   1
     *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |0 0|     STUN Message Type     |
     * |0 0|M M M M M C M M M C M M M M|
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |0 0 0 0 0 0 0 X 0 0 0 X 0 0 0 0|
     * ---------------^-------^
     */

    // unset all method bits
    head[1] &= 0x10;
    head[0] &= 0x01;

    if (type <= 0x0f) {

        head[1] |= type;
        return true;
    }

    // set around C bits

    // set last 4 bit
    head[1] |= (type & 0x0F);

    // set next 3 bit
    head[1] |= ((type << 1) & 0xE0);

    // set remaining 5 bit
    head[0] |= (((type << 2) >> 8) & 0x3E);

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *                               MESSAGE CLASSE
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_frame_class_is_request(const uint8_t *head, size_t length) {

    if (!head || length < 2) return false;

    if ((head[0] & 0x01)) return false;

    if ((head[1] & 0x10)) return false;

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_frame_class_is_indication(const uint8_t *head, size_t length) {

    if (!head || length < 2) return false;

    if ((head[0] & 0x01)) return false;

    if (!(head[1] & 0x10)) return false;

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_frame_class_is_success_response(const uint8_t *head,
                                             size_t length) {

    if (!head || length < 2) return false;

    if (!(head[0] & 0x01)) return false;

    if ((head[1] & 0x10)) return false;

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_frame_class_is_error_response(const uint8_t *head, size_t length) {

    if (!head || length < 2) return false;

    if (!(head[0] & 0x01)) return false;

    if (!(head[1] & 0x10)) return false;

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_frame_set_success_response(uint8_t *head, size_t length) {

    if (!head || length < 20) return false;

    head[0] = head[0] | 0x01;
    head[1] = (head[1] & 0xEF);

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_frame_set_error_response(uint8_t *head, size_t length) {

    if (!head || length < 20) return false;

    head[0] = head[0] | 0x01;
    head[1] = head[1] | 0x10;

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_frame_set_request(uint8_t *head, size_t length) {

    if (!head || length < 20) return false;

    head[0] = head[0] & 0x3E;
    head[1] = head[1] & 0xEF;

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_frame_set_indication(uint8_t *head, size_t length) {

    if (!head || length < 20) return false;

    head[0] = head[0] & 0x3E;
    head[1] = head[1] | 0x10;

    return true;
}
