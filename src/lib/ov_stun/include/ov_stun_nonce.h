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
        @file           ov_stun_nonce.h
        @author         Markus Toepfer

        @date           2018-10-22

        @ingroup        ov_stun

        @brief          Definition of RFC 5389 attribute "nonce"

         0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                                                               |
        |             Variable Length qdtext/qdpair sequence            |
        |                                                               |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


        ------------------------------------------------------------------------
*/
#ifndef ov_stun_nonce_h
#define ov_stun_nonce_h

#define STUN_NONCE 0x0015

#include <string.h>

#include "ov_stun_attribute.h"
#include "ov_stun_grammar.h"

#include <ov_base/ov_time.h>

/*      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Check is an attribute buffer may contain a nonce

        @param buffer           start of the attribute buffer
        @param length           length of the attribute buffer (at least)
*/
bool ov_stun_attribute_frame_is_nonce(const uint8_t *buffer, size_t length);

/*      ------------------------------------------------------------------------
 *
 *                              VALIDATION
 *
 *      ------------------------------------------------------------------------
 */

/**
        Validate the content of the buffer up to length is valid input
        content for a STUN nonce.
*/
bool ov_stun_nonce_validate(const uint8_t *buffer, size_t length);

/*      ------------------------------------------------------------------------
 *
 *                              RANDOM CREATION
 *
 *      ------------------------------------------------------------------------
 */

/**
        Fill the given buffer with a valid nonce.
        @NOTE impl uses the basic multilingual UTF8 plane 0x0000 - 0xFFFF
*/
bool ov_stun_nonce_fill(uint8_t *start, size_t length);

/*----------------------------------------------------------------------------*/

/**
        Fill the given buffer with a valid nonce.

        @params start   start of the buffer to fill
        @params length  length of the buffer to fill
        @params sec     Security feature
        @NOTE impl uses the basic multilingual UTF8 plane 0x0000 - 0xFFFF
*/
bool ov_stun_nonce_fill_rfc8489(uint8_t *start, size_t length, uint32_t sec);

/*
 *      ------------------------------------------------------------------------
 *
 *                        SERIALIZATION / DESERIALIZATION
 *
 *      ------------------------------------------------------------------------
 */

/**
        Calculate the byte length for the full attribute frame (incl header)
*/
size_t ov_stun_nonce_encoding_length(const uint8_t *nonce, size_t length);

/*----------------------------------------------------------------------------*/

/**
        Encode an nonce to an buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     (optional) pointer to next (after write)
        @param nonce    nonce to encode
        @param size     size of the nonce to encode
        @returns true   if content of address is written to buffer
*/
bool ov_stun_nonce_encode(uint8_t *buffer,
                          size_t length,
                          uint8_t **next,
                          const uint8_t *nonce,
                          size_t size);

/*----------------------------------------------------------------------------*/

/**
        Decode an nonce from buffer. (pointer only)

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param nonce    pointer to encoded nonce
        @param size     size of the nonce
*/
bool ov_stun_nonce_decode(const uint8_t *buffer,
                          size_t length,
                          uint8_t **nonce,
                          size_t *size);

/*----------------------------------------------------------------------------*/

/**
        Decode an nonce from buffer. (pointer only)

        @param buffer   start of the nonce
        @param length   length of the nonce
*/
bool ov_stun_nonce_decode_rfc8489_password_algorithm_set(const uint8_t *buffer,
                                                         size_t length);

bool ov_stun_nonce_decode_rfc8489_username_anonymity_set(const uint8_t *buffer,
                                                         size_t length);

bool ov_stun_nonce_is_rfc8489(const uint8_t *buffer, size_t length);

#endif /* ov_stun_nonce_h */
