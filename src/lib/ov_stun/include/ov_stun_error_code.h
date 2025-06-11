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
        @file           ov_stun_error_code.h
        @author         Markus Toepfer

        @date           2018-10-22

        @ingroup        ov_stun

        @brief          Definition of RFC 5389 attribute "error code"


         0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |           Reserved, should be 0         |Class|     Number    |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |      Reason Phrase (variable)                                ..
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

        ------------------------------------------------------------------------
*/
#ifndef ov_stun_error_code_h
#define ov_stun_error_code_h

#define STUN_ERROR_CODE 0x0009

#include "ov_stun_attribute.h"
#include "ov_stun_frame.h"
#include "ov_stun_grammar.h"

#include <ov_base/ov_utf8.h>

/*      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Check is an attribute buffer may contain an error code

        @param buffer           start of the attribute buffer
        @param length           length of the attribute buffer (at least)
*/
bool ov_stun_attribute_frame_is_error_code(const uint8_t *buffer,
                                           size_t length);

/*
 *      ------------------------------------------------------------------------
 *
 *                        SERIALIZATION / DESERIALIZATION
 *
 *      ------------------------------------------------------------------------
 */

/**
        Calculates the attribute length, based on the phrase_length used.
*/
size_t ov_stun_error_code_encoding_length(const uint8_t *phrase,
                                          size_t phrase_length);

/*----------------------------------------------------------------------------*/

/**
        Get the error code number (300 - 699) from an attribute buffer.
*/
uint16_t ov_stun_error_code_decode_code(const uint8_t *buffer, size_t length);

/*----------------------------------------------------------------------------*/

/**
        Get pointer and size of the phrase. Used in attribute buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param phrase   pointer to encoded phrase
        @param size     pointer for size of the phrase
*/
bool ov_stun_error_code_decode_phrase(const uint8_t *buffer,
                                      size_t length,
                                      uint8_t **phrase,
                                      size_t *size);

/*----------------------------------------------------------------------------*/

/**
        Encode a code number and phrase to an attribute buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     pointer to next after write
        @param code     code number (3 digit) to be used
        @param phrase   phrase to encode (MUST not be NULL)
        @param plength  length of the phrase to encode
        @returns true   if code and phrase are written as attribute
*/
bool ov_stun_error_code_encode(uint8_t *buffer,
                               size_t length,
                               uint8_t **next,
                               uint16_t code,
                               const uint8_t *phrase,
                               size_t plength);

/*----------------------------------------------------------------------------*/

/**
        Write error code 300 "try alternate" to buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     pointer to next after write
*/
bool ov_stun_error_code_set_try_alternate(uint8_t *buffer,
                                          size_t length,
                                          uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
        Write error code 400 "bad request" to buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     pointer to next after write
*/
bool ov_stun_error_code_set_bad_request(uint8_t *buffer,
                                        size_t length,
                                        uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
        Write error code 401 "unauthorized" to buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     pointer to next after write
*/
bool ov_stun_error_code_set_unauthorized(uint8_t *buffer,
                                         size_t length,
                                         uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
        Write error code 420 "unknown attribute" to buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     pointer to next after write
*/
bool ov_stun_error_code_set_unknown_attribute(uint8_t *buffer,
                                              size_t length,
                                              uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
        Write error code 438 "stale nonce" to buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     pointer to next after write
*/
bool ov_stun_error_code_set_stale_nonce(uint8_t *buffer,
                                        size_t length,
                                        uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
        Write error code 500 "server error" to buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     pointer to next after write
*/
bool ov_stun_error_code_set_server_error(uint8_t *buffer,
                                         size_t length,
                                         uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
        Write error code 437 "allocation mismatch" to buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     pointer to next after write
*/
bool ov_stun_error_code_set_allocation_mismatch(uint8_t *buffer,
                                                size_t length,
                                                uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
        Write error code 443 "allocation mismatch" to buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     pointer to next after write
*/
bool ov_stun_error_code_set_family_mismatch(uint8_t *buffer,
                                            size_t length,
                                            uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
        Write error code 442 "Unsupported transport" to buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     pointer to next after write
*/
bool ov_stun_error_code_set_unsupported_transport(uint8_t *buffer,
                                                  size_t length,
                                                  uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
        Write error code 440 "Unsupported address family" to buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     pointer to next after write
*/
bool ov_stun_error_code_set_address_family_not_supported(uint8_t *buffer,
                                                         size_t length,
                                                         uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
        Write error code 403 "Forbidden" to buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     pointer to next after write
*/
bool ov_stun_error_code_set_forbidden(uint8_t *buffer,
                                      size_t length,
                                      uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
        Generate a error response to an incoming frame.

        This function is using functions,
        which generate the error code attribute.

        @param frame            frame to respond to
        @param frame_size       size of frame
        @param response         buffer for response
        @param response_size    size of response buffer
        @param function         function to add an error code attribute frame.
*/
size_t ov_stun_error_code_generate_response(
    const uint8_t *frame,
    size_t frame_size,
    uint8_t *response,
    size_t response_size,
    bool (*function)(uint8_t *attribute_frame,
                     size_t max_size,
                     uint8_t **next));

#endif /* ov_stun_error_code_h */
