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
        @file           ov_turn_attr_even_port.h
        @author         Markus Töpfer

        @date           2022-01-21

         0
         0 1 2 3 4 5 6 7
        +-+-+-+-+-+-+-+-+
        |R|    RFFU     |
        +-+-+-+-+-+-+-+-+

        ------------------------------------------------------------------------
*/
#ifndef ov_turn_attr_even_port_h
#define ov_turn_attr_even_port_h

#define TURN_EVEN_PORT 0x0018

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
bool ov_turn_attr_is_even_port(const uint8_t *buffer, size_t length);

/*
 *      ------------------------------------------------------------------------
 *
 *                        SERIALIZATION / DESERIALIZATION
 *
 *      ------------------------------------------------------------------------
 */

size_t ov_turn_attr_even_port_encoding_length();

/*----------------------------------------------------------------------------*/

/**
        Encode an number to an buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     (optional) pointer to next (after write)
        @param number   number to encode
        @returns true   if content of address is written to buffer
*/
bool ov_turn_attr_even_port_encode(uint8_t *buffer, size_t length,
                                   uint8_t **next, bool set);

/*----------------------------------------------------------------------------*/

/**
        Decode an number from buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param number   pointer to store decoded number
*/
bool ov_turn_attr_even_port_decode(const uint8_t *buffer, size_t length,
                                   bool *set);

#endif /* ov_turn_attr_even_port_h */
