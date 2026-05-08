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
        @file           ov_stun_fingerprint.h
        @author         Markus Toepfer

        @date           2018-10-22

        @ingroup        ov_stun

        @brief          Definition of RFC 5389 attribute "fingerprint"

         0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                                                               |
        |                         CRC 32 CHECKSUM                       |
        |                                                               |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+




        ------------------------------------------------------------------------
*/
#ifndef ov_stun_fingerprint_h
#define ov_stun_fingerprint_h

#define STUN_FINGERPRINT 0x8028

#include "ov_stun_attribute.h"
#include "ov_stun_frame.h"

/*      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Check is an attribute buffer may contain a fingerprint.

        @param buffer           start of the attribute buffer
        @param length           length of the attribute buffer (at least)
*/
bool ov_stun_attribute_frame_is_fingerprint(const uint8_t *buffer,
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
size_t ov_stun_fingerprint_encoding_length();

/*----------------------------------------------------------------------------*/

/**
        Add a fingerprint to a buffer. This function will write a fingerprint
        at position "start" within the buffer head:length.

        @param head             start of the message buffer
        @param length           length of the message buffer
        @param start            start of the attribute within head
        @param next             (optional) pointer to next (after write)
        @returns true           if attribute was written to position
*/
bool ov_stun_add_fingerprint(uint8_t *head, size_t length, uint8_t *start,
                             uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
        Checks if a fingerprint is set and valid.

        @param head             start of the message buffer
        @param length           length of the message buffer
        @param attr             list of attribute pointers within head
        @param must_be_set      if true the fingerprint MUST be set and valid
                                if false the fingerprint MAY be set, and
                                when set it MUST be valid
        @returns true           if fingerprint is found AND valid
                                @NOTE all following attribute pointers will be
                                removed from the list.
*/
bool ov_stun_check_fingerprint(uint8_t *head, size_t length, uint8_t *attr[],
                               size_t attr_size, bool must_be_set);

#endif /* ov_stun_fingerprint_h */
