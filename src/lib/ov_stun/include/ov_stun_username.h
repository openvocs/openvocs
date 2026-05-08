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
        @file           ov_stun_username.h
        @author         Markus Toepfer

        @date           2018-10-19

        @ingroup        ov_stun

        @brief          Definition of RFC 5389 attribute "username"

         0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                                                               |
        |                 Variable Length UTF8 sequence                 |
        |                                                               |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


        ------------------------------------------------------------------------
*/
#ifndef ov_stun_username_h
#define ov_stun_username_h

#define STUN_USERNAME 0x0006

#include <ov_base/ov_utf8.h>
#include <string.h>

#include "ov_stun_attribute.h"

/*      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Check is an attribute buffer may contain a username

        @param buffer           start of the attribute buffer
        @param length           length of the attribute buffer (at least)
*/
bool ov_stun_attribute_frame_is_username(const uint8_t *buffer, size_t length);

/*      ------------------------------------------------------------------------
 *
 *                              VALIDATION
 *
 *      ------------------------------------------------------------------------
 */

/**
        Validate the content of the buffer up to length is valid input
        content for a STUN username.
*/
bool ov_stun_username_validate(const uint8_t *buffer, size_t length);

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
size_t ov_stun_username_encoding_length(const uint8_t *username, size_t length);

/*----------------------------------------------------------------------------*/

/**
        Encode an username to an buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     (optional) pointer to next (after write)
        @param username username to encode
        @param size     size of the username to encode
        @returns true   if content of address is written to buffer
*/
bool ov_stun_username_encode(uint8_t *buffer, size_t length, uint8_t **next,
                             const uint8_t *username, size_t size);

/*----------------------------------------------------------------------------*/

/**
        Decode an username from buffer. (pointer only)

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param username pointer to encoded username
        @param size     size of the username to encode
*/
bool ov_stun_username_decode(const uint8_t *buffer, size_t length,
                             uint8_t **username, size_t *size);

#endif /* ov_stun_username_h */
