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
        @file           ov_stun_frame.h
        @author         Markus Toepfer

        @date           2018-10-18

        @ingroup        ov_stun

        @brief          Definition of RFC 5389 frame related functions.

         0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |0 0|     STUN Message Type     |         Message Length        |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                         Magic Cookie                          |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                                                               |
        |                     Transaction ID (96 bits)                  |
        |                                                               |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


         0                 1
         2  3  4 5 6 7 8 9 0 1 2 3 4 5
        +--+--+-+-+-+-+-+-+-+-+-+-+-+-+
        |M |M |M|M|M|C|M|M|M|C|M|M|M|M|
        |11|10|9|8|7|1|6|5|4|0|3|2|1|0|
        +--+--+-+-+-+-+-+-+-+-+-+-+-+-+

        ------------------------------------------------------------------------
*/
#ifndef ov_stun_frame_h
#define ov_stun_frame_h

#include <arpa/inet.h>
#include <ov_base/ov_list.h>
#include <ov_base/ov_time.h>

#define STUN_FRAME_ATTRIBUTE_ARRAY_SIZE 100

/*
 *      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Check if a frame contains a valid STUN message.
*/
bool ov_stun_frame_is_valid(const uint8_t *buffer, size_t length);

/*
 *      ------------------------------------------------------------------------
 *
 *                              FRAME SLICING
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

/**
        Slice a frame into it's attributes.

        This function creates a list of pointers to the start of each attribute
        within the buffer. All content of the list are pointers to the attribute
        start within the source buffer.

        @param buffer           start of the buffer to slice
        @param length           length of the buffer (at least 20 byte)
        @returns                ov_list on success, NULL on error

        @NOTE in case of NO ATTRIBUTES an empty list may be created
        @NOTE use ov_list_free(list) to delete the list of pointers
*/
bool ov_stun_frame_slice(const uint8_t *frame,
                         size_t frame_length,
                         uint8_t *array[],
                         size_t array_size);

/*
 *      ------------------------------------------------------------------------
 *
 *                               MESSAGE LENGTH
 *
 *      ------------------------------------------------------------------------
 */

/**
        Parse the length in host byte order out of a frame.
        @returns length on success
        @returns -1 on error;
*/
int64_t ov_stun_frame_get_length(const uint8_t *buffer, size_t length);

/*----------------------------------------------------------------------------*/

/**
        Set the length in network byte order to a frame buffer.
        @param head             start of the buffer
        @param length           length of the buffer (MUST be at least 20)
        @param length_to_set    length in host byte order
        @returns true           if lengths_to_set was written in network order
*/
bool ov_stun_frame_set_length(uint8_t *head,
                              size_t length,
                              uint16_t lengths_to_set);

/*
 *      ------------------------------------------------------------------------
 *
 *                               TRANSACTION ID
 *
 *      ------------------------------------------------------------------------
 */

/**
        Generate a random transaction id of 12 bytes.
        @param buffer MUST be 12 bytes long
        @returns true if buffer was filled with 12 random bytes

        @NOTE To set a random id you may use something like:
                ov_stun_frame_generate_transaction_id(head+8);
*/
bool ov_stun_frame_generate_transaction_id(uint8_t *buffer);

/*----------------------------------------------------------------------------*/

/**
        Set at 12 byte long transaction id to the header
        @param head             start of the buffer
        @param length           length of the buffer (MUST be at least 20)
        @param id               id to be copied to header (MUST be 12 byte long)
*/
bool ov_stun_frame_set_transaction_id(uint8_t *head,
                                      size_t length,
                                      const uint8_t *id);

/*----------------------------------------------------------------------------*/

/**
        Get the pointer of the transaction ID.
*/
uint8_t *ov_stun_frame_get_transaction_id(const uint8_t *head, size_t length);

/*
 *      ------------------------------------------------------------------------
 *
 *                               MAGIC COOKIE
 *
 *      ------------------------------------------------------------------------
 */

/**
        Check if the magic_cookie is set.
*/
bool ov_stun_frame_has_magic_cookie(const uint8_t *head, size_t length);

/*----------------------------------------------------------------------------*/

/**
        Set the magic_cookie to an buffer.
*/
bool ov_stun_frame_set_magic_cookie(uint8_t *head, size_t length);

/*
 *      ------------------------------------------------------------------------
 *
 *                               MESSAGE TYPE
 *
 *      ------------------------------------------------------------------------
 */

/**
        Get the type of the message out of the header.
*/
uint16_t ov_stun_frame_get_method(const uint8_t *head, size_t length);

/*----------------------------------------------------------------------------*/

/**
        Set the type to the header.
        @param head             start of the buffer
        @param length           length of the buffer (MUST be at least 20)
        @param type             message type to set.
*/
bool ov_stun_frame_set_method(uint8_t *head, size_t length, uint16_t type);

/*----------------------------------------------------------------------------*/

/**
        Add some padding to a pointer if required.

        Will MOVE *pos by required padding of multiple of 4 in reference to
   head. *pos will point to: head + (whatever * 4)
*/
bool ov_stun_frame_add_padding(const uint8_t *head, uint8_t **pos);

/*
 *      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_frame_class_is_request(const uint8_t *head, size_t length);
bool ov_stun_frame_class_is_indication(const uint8_t *head, size_t length);
bool ov_stun_frame_class_is_success_response(const uint8_t *head,
                                             size_t length);
bool ov_stun_frame_class_is_error_response(const uint8_t *head, size_t length);

bool ov_stun_frame_set_success_response(uint8_t *head, size_t length);
bool ov_stun_frame_set_error_response(uint8_t *head, size_t length);
bool ov_stun_frame_set_request(uint8_t *head, size_t length);
bool ov_stun_frame_set_indication(uint8_t *head, size_t length);

#endif /* ov_stun_frame_h */
