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
        @file           ov_stun_mapped_address.h
        @author         Markus Toepfer

        @date           2018-10-19

        @ingroup        ov_stun

        @brief          Definition of RFC 5389 attribute "mapped address"

        
         0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |0 0 0 0 0 0 0 0|    Family     |           Port                |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                                                               |
        |                 Address (32 bits or 128 bits)                 |
        |                                                               |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


        ------------------------------------------------------------------------
*/
#ifndef ov_stun_mapped_address_h
#define ov_stun_mapped_address_h

#define STUN_MAPPED_ADDRESS 0x0001

#include <inttypes.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <string.h>

#include "ov_stun_attribute.h"

/*      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Check is an attribute buffer may contain a mapped address.

        @param buffer           start of the attribute buffer
        @param length           length of the attribute buffer (at least)
*/
bool ov_stun_attribute_frame_is_mapped_address(const uint8_t *buffer,
                                               size_t length);

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
size_t ov_stun_mapped_address_encoding_length(
    const struct sockaddr_storage *address);

/*----------------------------------------------------------------------------*/

/**
        Encode a sockaddr_storage as STUN attribute.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param next     (optional) pointer to next (after write)
        @param address  sockaddr_storage to encode.
        @returns true   if content of address is written to buffer
*/
bool ov_stun_mapped_address_encode(uint8_t *buffer,
                                   size_t length,
                                   uint8_t **next,
                                   const struct sockaddr_storage *address);

/*----------------------------------------------------------------------------*/

/**
        Decode a sockaddr_storage from a STUN attribute.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param address  pointer to sockaddr_storage to fill / create
*/
bool ov_stun_mapped_address_decode(const uint8_t *buffer,
                                   size_t length,
                                   struct sockaddr_storage **address);

#endif /* ov_stun_mapped_address_h */
