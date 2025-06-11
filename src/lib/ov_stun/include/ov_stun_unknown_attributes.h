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
        @file           ov_stun_unknown_attributes.h
        @author         Markus Toepfer

        @date           2018-10-23

        @ingroup        ov_stun

        @brief          Definition of RFC 5389 attribute "unknown attributes"


         0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |      Attribute 1 Type           |     Attribute 2 Type        |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |      Attribute 3 Type           |     Attribute 4 Type    ...
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


        ------------------------------------------------------------------------
*/
#ifndef ov_stun_unknown_attributes_h
#define ov_stun_unknown_attributes_h

#define STUN_UNKNOWN_ATTRIBUTES 0x000A

#include "ov_stun_attribute.h"
#include "ov_stun_grammar.h"

/*      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Check is an attribute buffer may contains unknown attributes

        @param buffer           start of the attribute buffer
        @param length           length of the attribute buffer (at least)
*/
bool ov_stun_attribute_frame_is_unknown_attributes(const uint8_t *buffer,
                                                   size_t length);

/*
 *      ------------------------------------------------------------------------
 *
 *                        SERIALIZATION / DESERIALIZATION
 *
 *      ------------------------------------------------------------------------
 */

/**
        Decode unknown attributes pointers to a list

        @param buffer   start of the attribute frame
        @param length   length of the buffer
*/
ov_list *ov_stun_unknown_attributes_decode(const uint8_t *buffer,
                                           size_t length);

/*----------------------------------------------------------------------------*/

/**
        Decode a pointer from network byte order to uint16_t
*/
uint16_t ov_stun_unknown_attributes_decode_pointer(const uint8_t *pointer);

/*----------------------------------------------------------------------------*/

/**
        Encode unknown attributes pointers to a list

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     (optional) pointer to next (after write)
        @param unknown  array with unknown uint16_t values
        @param items    amount of items in unknown to set
        @returns true   if content of address is written to buffer
*/
bool ov_stun_unknown_attributes_encode(uint8_t *buffer,
                                       size_t length,
                                       uint8_t **next,
                                       uint16_t unknown[],
                                       size_t items);

#endif /* ov_stun_unknown_attributes_h */
