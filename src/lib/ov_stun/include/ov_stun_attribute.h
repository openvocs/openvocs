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
        @file           ov_stun_attribute.h
        @author         Markus Toepfer

        @date           2018-10-18

        @ingroup        ov_stun

        @brief          Definition of RFC 5389 attribute related functions.


         0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |         Type                  |            Length             |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                         Value (variable)                ....
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


        ------------------------------------------------------------------------
*/
#ifndef ov_stun_attribute_h
#define ov_stun_attribute_h

#include <arpa/inet.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <ov_base/ov_list.h>

/*
 *      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Check if a buffer contains a valid STUN attribute frame.

        @param buffer           start of the attribute buffer
        @param length           length of the attribute buffer (at least)
*/
bool ov_stun_attribute_frame_is_valid(const uint8_t *buffer, size_t length);

/*----------------------------------------------------------------------------*/

/**
        Check if the frame is valid and set content start and length of the
        attribute.

        @NOTE content will point to content start within buffer
        @param buffer           start of the attribute buffer
        @param length           length of the attribute buffer (at least)
        @param content_start    pointer to be set to content start
        @param content_length   site_t to be set to content length
        @returns                true if the buffer contains a valid attribute
   frame
        @NOTE if content_length == 0 content_start will be set to NULL
*/
bool ov_stun_attribute_frame_content(const uint8_t *buffer, size_t length,
                                     uint8_t **content_start,
                                     size_t *content_length);

/*
 *      ------------------------------------------------------------------------
 *
 *                        SERIALIZATION / DESERIALIZATION
 *
 *      ... some generic encode and decode functions.
 *      For speficic de and encodings check the attribute implementations.
 *
 *      ------------------------------------------------------------------------
 */

/**
        Parse the type of an attribute (from network byte order)
*/
uint16_t ov_stun_attribute_get_type(const uint8_t *buffer, size_t length);

/*----------------------------------------------------------------------------*/

/**
        Set the type of an attribute (in network byte order)
*/
bool ov_stun_attribute_set_type(uint8_t *buffer, size_t length, uint16_t type);

/*----------------------------------------------------------------------------*/

/**
        Parse the length of an attribute (from network byte order)
*/
int64_t ov_stun_attribute_get_length(const uint8_t *buffer, size_t length);

/*----------------------------------------------------------------------------*/

/**
        Set the length of an attribute (in network byte order)
*/
bool ov_stun_attribute_set_length(uint8_t *buffer, size_t length,
                                  uint16_t length_to_set);

/*----------------------------------------------------------------------------*/

/**
        Encode some attribute content string as TLV (type length value)
        to a given buffer.

        This function will write TYPE LENGTH and CONTENT to buffer,
        if the buffer fits the content.

        @param buffer           buffer to write to
        @param length           open length of the buffer
        @param next             (optional) pointer to next byte after write
        @param attribute_type   type of the attribute to be set
        @param content_start    start of a content string
        @param content_length   length of a content string
        @returns true           if content was written to buffer
*/
bool ov_stun_attribute_encode(uint8_t *buffer, size_t length, uint8_t **next,
                              uint16_t attribute_type,
                              const uint8_t *content_start,
                              size_t content_length);

/*----------------------------------------------------------------------------*/

/**
        Create an independent string of the content of the attribute
        within buffer. (copy content)

        @param buffer           buffer to decode
        @param length           length of the buffer
        @param type             pointer to uint16_t to write the type
        @param content          pointer to pointer for result string
        @returns true           if attribute content was copied to *content
        @NOTE                   *content will be an allocated string
                                on success.
*/
bool ov_stun_attribute_decode(const uint8_t *buffer, size_t length,
                              uint16_t *type, uint8_t **content);

/*----------------------------------------------------------------------------*/

/**
        Get a attribute frame out of a list of attribute frames,
        based on the type of the attribute.

        @param list             list of pointers to attribute frames
        @param type             type to search
        @returns                pointer to attribute frame if included.
*/
uint8_t *ov_stun_attributes_get_type(uint8_t *attr[], size_t attr_size,
                                     uint16_t type);

/*----------------------------------------------------------------------------*/

/**
        Copy an attribute frame from one buffer to another,
        the length will be parsed form the orig frame.

        @param orig             pointer to attr_frame
        @param dest             pointer to dest attr frame
        @param dest_size        size of open bytes at dest
        @param next             pointer to next byte after write
        @returns                true if the frame was copied
*/
bool ov_stun_attribute_frame_copy(const uint8_t *orig, uint8_t *dest,
                                  size_t dest_size, uint8_t **next);

#endif /* ov_stun_attribute_h */
