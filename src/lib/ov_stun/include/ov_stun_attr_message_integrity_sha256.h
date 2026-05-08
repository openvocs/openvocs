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
        @file           ov_stun_attr_message_integrity_sha256.h
        @author         Markus Töpfer

        @date           2022-01-22


        ------------------------------------------------------------------------
*/
#ifndef ov_stun_attr_message_integrity_sha256_h
#define ov_stun_attr_message_integrity_sha256_h

#define STUN_ATTR_MESSAGE_INTEGRITY_SHA256 0x001C

#include <ov_encryption/ov_hmac.h>

#include "ov_stun_attribute.h"
#include "ov_stun_fingerprint.h"
#include "ov_stun_frame.h"

/*      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Check is an attribute buffer may contain a message integrity.

        @param buffer           start of the attribute buffer
        @param length           length of the attribute buffer (at least)
*/
bool ov_stun_attribute_frame_is_message_integrity_sha256(const uint8_t *buffer,
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
size_t ov_stun_message_integrity_sha256_encoding_length();

/*----------------------------------------------------------------------------*/

/**
        Add a message integrity to a buffer. This function will write a message
        integrity at position "start" within the buffer head:length.

        @param head     start of the message buffer
        @param length   length of the message buffer
        @param start    start of the message integrity attribute within head
        @param next     (optional) pointer to next (after write)
        @param key      key to use to generate the message integrity
        @param keylen   length of the key
        @returns true   if integrity was written to position
*/
bool ov_stun_add_message_integrity_sha256(uint8_t *head, size_t length,
                                          uint8_t *start, uint8_t **next,
                                          const uint8_t *key, size_t keylen);

/*----------------------------------------------------------------------------*/

/**
        Checks if a message integrity is set and if the integrity is valid.
        The input list of attributes will delete all attributes after integrity,
        except for the fingerprint attribute, which may be present after.

        @param head             start of the message buffer
        @param length           length of the message buffer
        @param attr             array of attribute pointers within head
        @param attr_size        size of the attribute array
        @param key              key to use to generate the message integrity
        @param keylen           length of the key
        @param must_be_set      if true the integrity MUST be set and valid
                                if false the integrity MAY be set, and
                                when set it MUST be valid
        @returns true           if integrity is found in the attributes list
                                and if the integrity is valid
                                @NOTE all invalid attribute pointers
                                will be removed from the list
*/
bool ov_stun_check_message_integrity_sha256(uint8_t *head, size_t length,
                                            uint8_t *attr[], size_t attr_size,
                                            uint8_t *key, size_t keylen,
                                            bool must_be_set);

#endif /* ov_stun_attr_message_integrity_sha256_h */
