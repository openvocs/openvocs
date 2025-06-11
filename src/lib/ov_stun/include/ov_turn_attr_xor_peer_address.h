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
        @file           ov_turn_attr_xor_peer_address.h
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#ifndef ov_turn_attr_xor_peer_address_h
#define ov_turn_attr_xor_peer_address_h

#define TURN_XOR_PEER_ADDRESS 0x0012
#include "ov_stun_attribute.h"

/*
 *      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Check is an attribute buffer may contain a peer address.

        @param buffer           start of the attribute buffer
        @param length           length of the attribute buffer (at least)
*/
bool ov_turn_attr_frame_is_xor_peer_address(const uint8_t *buffer,
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
size_t ov_turn_attr_xor_peer_address_encoding_length(
    const struct sockaddr_storage *address);

/*----------------------------------------------------------------------------*/

/**
        Encode a sockaddr_storage as STUN attribute.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param head     pointer to header of the STUN frame
        @param next     (optional) pointer to next (after write)
        @param address  sockaddr_storage to encode.
        @returns true   if content of address is written to buffer
*/
bool ov_turn_attr_xor_peer_address_encode(
    uint8_t *buffer,
    size_t length,
    const uint8_t *head,
    uint8_t **next,
    const struct sockaddr_storage *address);

/*----------------------------------------------------------------------------*/

/**
        Decode a sockaddr_storage from a STUN attribute.

        @param buffer   start of the attribute frame
        @param length   length of the buffer
        @param head     pointer to header of the STUN frame
        @param address  pointer to sockaddr_storage to fill / create
*/
bool ov_turn_attr_xor_peer_address_decode(const uint8_t *buffer,
                                          size_t length,
                                          const uint8_t *head,
                                          struct sockaddr_storage **address);

#endif /* ov_turn_attr_xor_peer_address_h */
