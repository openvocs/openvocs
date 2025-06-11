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
        @file           ov_stun_attr_password-algorithm.h
        @author         Markus Töpfer

        @date           2022-01-22


      0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |          Algorithm           |  Algorithm Parameters Length   |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                    Algorithm Parameters (variable)
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

        ------------------------------------------------------------------------
*/
#ifndef ov_stun_attr_password_algorithm_h
#define ov_stun_attr_password_algorithm_h

#define STUN_ATTR_PASSWORD_ALGORITHM 0x001D

#include "ov_stun_attribute.h"

/*
 *      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Check is an attribute buffer may contain a realm

        @param buffer           start of the attribute buffer
        @param length           length of the attribute buffer (at least)
*/
bool ov_stun_attr_is_password_algorithm(const uint8_t *buffer, size_t length);

/*
 *      ------------------------------------------------------------------------
 *
 *                        SERIALIZATION / DESERIALIZATION
 *
 *      ------------------------------------------------------------------------
 */

size_t ov_stun_attr_password_algorithm_encoding_length(
    size_t password_algorithm);

/*----------------------------------------------------------------------------*/

/**
        Encode an number to an buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     (optional) pointer to next (after write)
        @param algorithm algorithm to use
        @param len      length of parameter
        @param params   parameter
        @returns true   if content of address is written to buffer
*/
bool ov_stun_attr_password_algorithm_encode(uint8_t *buffer,
                                            size_t length,
                                            uint8_t **next,
                                            uint16_t algorithm,
                                            uint16_t len,
                                            const uint8_t *params);

/*----------------------------------------------------------------------------*/

/**
        Decode an number from buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param password_algorithm     pointer to password_algorithm within
   buffer
        @param len      pointer to be set to length of password_algorithm
*/
bool ov_stun_attr_password_algorithm_decode(const uint8_t *buffer,
                                            size_t length,
                                            uint16_t *algorithm,
                                            uint16_t *len,
                                            const uint8_t **params);

#endif /* ov_stun_attr_password_algorithm_h */
