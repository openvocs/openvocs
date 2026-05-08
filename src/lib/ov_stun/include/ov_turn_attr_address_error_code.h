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
        @file           ov_turn_attr_address_error_code.h
        @author         Markus Töpfer

        @date           2022-01-21

          0                   1                   2                   3
          0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |  Family       |    Reserved             |Class|     Number    |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |      Reason Phrase (variable)                                ..
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        ------------------------------------------------------------------------
*/
#ifndef ov_turn_attr_address_error_code_h
#define ov_turn_attr_address_error_code_h

#define TURN_ADDRESS_ERROR_CODE 0x8001
#define OV_TURN_PHRASE_MAX 500

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
bool ov_turn_attr_is_address_error_code(const uint8_t *buffer, size_t length);

/*
 *      ------------------------------------------------------------------------
 *
 *                        SERIALIZATION / DESERIALIZATION
 *
 *      ------------------------------------------------------------------------
 */

size_t ov_turn_attr_address_error_code_encoding_length(size_t phrase_len);

/*----------------------------------------------------------------------------*/

/**
        Encode an number to an buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     (optional) pointer to next (after write)
        @param family   ip family
        @param code     3 digit error code to encode
        @param phrase   phrase to encode
        @param len      phrase len
        @returns true   if content of address is written to buffer
*/
bool ov_turn_attr_address_error_code_encode(uint8_t *buffer, size_t length,
                                            uint8_t **next, uint8_t family,
                                            uint16_t code,
                                            const uint8_t *phrase, size_t len);

/*----------------------------------------------------------------------------*/

/**
        Decode an number from buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param family   ip family
        @param code     pointer for 3 digit error code to decode
        @param phrase   pointer for phrase to decode
        @param len      pointer for phrase len
*/
bool ov_turn_attr_address_error_code_decode(const uint8_t *buffer,
                                            size_t length, uint8_t *family,
                                            uint16_t *code,
                                            const uint8_t **phrase,
                                            size_t *len);

#endif /* ov_turn_attr_address_error_code_h */
