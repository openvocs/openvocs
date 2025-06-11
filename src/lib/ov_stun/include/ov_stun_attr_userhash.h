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
        @file           ov_stun_attr_userhash.h
        @author         Markus Töpfer

        @date           2022-01-22


        ------------------------------------------------------------------------
*/
#ifndef ov_stun_attr_userhash_h
#define ov_stun_attr_userhash_h

#define STUN_ATTR_USERHASH 0x001E

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
bool ov_stun_attr_is_userhash(const uint8_t *buffer, size_t length);

/*
 *      ------------------------------------------------------------------------
 *
 *                        SERIALIZATION / DESERIALIZATION
 *
 *      ------------------------------------------------------------------------
 */

size_t ov_stun_attr_userhash_encoding_length(size_t data);

/*----------------------------------------------------------------------------*/

/**
        Encode an number to an buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     (optional) pointer to next (after write)
        @param data     data to encode
        @param len      length of data
        @returns true   if content of address is written to buffer
*/
bool ov_stun_attr_userhash_encode(uint8_t *buffer,
                                  size_t length,
                                  uint8_t **next,
                                  const uint8_t *data,
                                  size_t len);

/*----------------------------------------------------------------------------*/

/**
        Decode an number from buffer.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param data     pointer to data within buffer
        @param len      pointer to be set to length of data
*/
bool ov_stun_attr_userhash_decode(const uint8_t *buffer,
                                  size_t length,
                                  uint8_t **data,
                                  size_t *len);

#endif /* ov_stun_attr_userhash_h */
