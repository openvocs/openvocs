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
        @file           ov_stun_error_code.c
        @author         Markus Toepfer

        @date           2018-10-22

        @ingroup        ov_stun

        @brief          Implementation of RFC 5389 attribute "error code"


        ------------------------------------------------------------------------
*/
#include "../include/ov_stun_error_code.h"

/*
 *      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_attribute_frame_is_error_code(const uint8_t *buffer,
                                           size_t length) {

    if (!buffer || length < 8) goto error;

    uint16_t type = ov_stun_attribute_get_type(buffer, length);
    int64_t size = ov_stun_attribute_get_length(buffer, length);

    if (type != STUN_ERROR_CODE) goto error;

    if (size > 763 || size < 1) goto error;

    if (length < (size_t)size + 8) goto error;

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

size_t ov_stun_error_code_encoding_length(const uint8_t *phrase,
                                          size_t length) {

    if (!phrase || length < 1 || length > 763) return 0;

    size_t pad = 0;
    pad = length % 4;
    if (pad != 0) pad = 4 - pad;

    return length + 8 + pad;
}

/*----------------------------------------------------------------------------*/

uint16_t ov_stun_error_code_decode_code(const uint8_t *buffer, size_t length) {

    if (!buffer || length < 8) goto error;

    uint16_t code = (buffer[6] & 0x0f) * 100;
    code += buffer[7];

    return code;
error:
    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_error_code_decode_phrase(const uint8_t *buffer,
                                      size_t length,
                                      uint8_t **phrase,
                                      size_t *size) {

    if (!buffer || length < 9 || !phrase || !size) goto error;

    if (ov_stun_attribute_get_type(buffer, 4) != STUN_ERROR_CODE) goto error;

    *size = ov_stun_attribute_get_length(buffer, 4) - 4;
    *phrase = (uint8_t *)buffer + 8;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_error_code_encode(uint8_t *buffer,
                               size_t length,
                               uint8_t **next,
                               uint16_t code,
                               const uint8_t *phrase,
                               size_t phrase_length) {

    uint8_t content[phrase_length + 4];

    if (!buffer || length < 8 || !phrase || phrase_length < 1) goto error;

    if (code < 300 || code >= 700) goto error;

    if (phrase_length > 763) goto error;

    if (length < phrase_length + 8) goto error;

    if (!ov_utf8_validate_sequence(phrase, phrase_length)) goto error;

    content[0] = 0;
    content[1] = 0;
    content[2] = 0x0f & (code / 100);
    content[3] = code % 100;
    if (!memcpy(content + 4, phrase, phrase_length)) goto error;

    return ov_stun_attribute_encode(
        buffer, length, next, STUN_ERROR_CODE, content, phrase_length + 4);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_error_code_set_try_alternate(uint8_t *buffer,
                                          size_t length,
                                          uint8_t **next) {

    char *phrase = "try alternate";

    return ov_stun_error_code_encode(
        buffer, length, next, 300, (uint8_t *)phrase, strlen(phrase));
}

/*----------------------------------------------------------------------------*/

bool ov_stun_error_code_set_bad_request(uint8_t *buffer,
                                        size_t length,
                                        uint8_t **next) {

    char *phrase = "bad request";

    return ov_stun_error_code_encode(
        buffer, length, next, 400, (uint8_t *)phrase, strlen(phrase));
}

/*----------------------------------------------------------------------------*/

bool ov_stun_error_code_set_unauthorized(uint8_t *buffer,
                                         size_t length,
                                         uint8_t **next) {

    char *phrase = "unauthorized";

    return ov_stun_error_code_encode(
        buffer, length, next, 401, (uint8_t *)phrase, strlen(phrase));
}

/*----------------------------------------------------------------------------*/

bool ov_stun_error_code_set_unknown_attribute(uint8_t *buffer,
                                              size_t length,
                                              uint8_t **next) {

    char *phrase = "unknown attribute";

    return ov_stun_error_code_encode(
        buffer, length, next, 420, (uint8_t *)phrase, strlen(phrase));
}

/*----------------------------------------------------------------------------*/

bool ov_stun_error_code_set_stale_nonce(uint8_t *buffer,
                                        size_t length,
                                        uint8_t **next) {

    char *phrase = "stale nonce";

    return ov_stun_error_code_encode(
        buffer, length, next, 438, (uint8_t *)phrase, strlen(phrase));
}

/*----------------------------------------------------------------------------*/

bool ov_stun_error_code_set_server_error(uint8_t *buffer,
                                         size_t length,
                                         uint8_t **next) {

    char *phrase = "server error";

    return ov_stun_error_code_encode(
        buffer, length, next, 500, (uint8_t *)phrase, strlen(phrase));
}

/*----------------------------------------------------------------------------*/

bool ov_stun_error_code_set_allocation_mismatch(uint8_t *buffer,
                                                size_t length,
                                                uint8_t **next) {

    char *phrase = "allocation mismatch";

    return ov_stun_error_code_encode(
        buffer, length, next, 437, (uint8_t *)phrase, strlen(phrase));
}

/*----------------------------------------------------------------------------*/

bool ov_stun_error_code_set_family_mismatch(uint8_t *buffer,
                                            size_t length,
                                            uint8_t **next) {

    char *phrase = "family mismatch";

    return ov_stun_error_code_encode(
        buffer, length, next, 443, (uint8_t *)phrase, strlen(phrase));
}

/*----------------------------------------------------------------------------*/

bool ov_stun_error_code_set_unsupported_transport(uint8_t *buffer,
                                                  size_t length,
                                                  uint8_t **next) {

    char *phrase = "unsupported transport";

    return ov_stun_error_code_encode(
        buffer, length, next, 442, (uint8_t *)phrase, strlen(phrase));
}

/*----------------------------------------------------------------------------*/

bool ov_stun_error_code_set_address_family_not_supported(uint8_t *buffer,
                                                         size_t length,
                                                         uint8_t **next) {

    char *phrase = "unsupported address family";

    return ov_stun_error_code_encode(
        buffer, length, next, 440, (uint8_t *)phrase, strlen(phrase));
}

/*----------------------------------------------------------------------------*/

bool ov_stun_error_code_set_forbidden(uint8_t *buffer,
                                      size_t length,
                                      uint8_t **next) {

    char *phrase = "forbidden";

    return ov_stun_error_code_encode(
        buffer, length, next, 403, (uint8_t *)phrase, strlen(phrase));
}

/*----------------------------------------------------------------------------*/

size_t ov_stun_error_code_generate_response(
    const uint8_t *frame,
    size_t frame_size,
    uint8_t *response,
    size_t response_size,
    bool (*function)(uint8_t *attribute_frame,
                     size_t max_size,
                     uint8_t **next)) {

    if (!frame || !response || frame_size < 20 || response_size < 20 ||
        !function)
        goto error;

    uint8_t *next = NULL;

    // clean response frame
    memset(response, 0, response_size);

    if (!ov_stun_frame_set_error_response(response, response_size)) goto error;

    if (!ov_stun_frame_set_magic_cookie(response, response_size)) goto error;

    // copy method
    if (!ov_stun_frame_set_method(response,
                                  response_size,
                                  ov_stun_frame_get_method(frame, frame_size)))
        goto error;

    // copy transaction id
    if (!ov_stun_frame_set_transaction_id(
            response,
            response_size,
            ov_stun_frame_get_transaction_id(frame, frame_size)))
        goto error;

    // add error code attribute
    if (!function(response + 20, response_size - 20, &next)) goto error;

    if (!ov_stun_attribute_set_length(
            response, response_size, (next - response) - 20))
        goto error;

    return (next - response);

error:
    return 0;
}
