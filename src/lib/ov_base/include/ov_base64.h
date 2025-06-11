/***
        ------------------------------------------------------------------------

        Copyright 2018 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the openvocs project. http://openvocs.org

        ------------------------------------------------------------------------
*//**
        @file           ov_base64.h
        @author         Markus Toepfer

        @date           2018-04-12

        @ingroup        ov_base

        @brief          Definition of base64 Encoding of RFC4648.


        ------------------------------------------------------------------------
*/
#ifndef ov_base64_h
#define ov_base64_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

/*
 *      ------------------------------------------------------------------------
 *
 *                             ENCODING
 *
 *      ------------------------------------------------------------------------
 *
 */

/**
        Encode a source buffer of length to a destination buffer.

        MODE 1  - allocate destination
        If *destination == NULL the buffer will be allocated and the
        length of the allocated data will be written to dest_length

        MODE 2  - fill destination
        If *destination != NULL the buffer will be filled with the
        encoded string, if the dest_length is big enough.

        @param source           source buffer to be encoded
        @param src_length       length of the soruce buffer
        @param destination      pointer to destination buffer
        @param dest_length      pointer to length of the destination buffer

        @returns true if the source was encoded and written to destination.
*/
bool ov_base64_encode(const uint8_t *source,
                      size_t src_length,
                      uint8_t **destination,
                      size_t *dest_length);

/*----------------------------------------------------------------------------*/

/**
        URL encode a source buffer of length to a destination buffer.
        For @mode and @param @see ov_base64_encode
*/
bool ov_base64_url_encode(const uint8_t *buffer,
                          size_t length,
                          uint8_t **result,
                          size_t *result_length);

/*
 *      ------------------------------------------------------------------------
 *
 *                             DECODING
 *
 *      ------------------------------------------------------------------------
 *
 */

/**
        Decode a source buffer of length to a destination buffer.
        For @mode and @param @see ov_base64_encode
*/
bool ov_base64_decode(const uint8_t *buffer,
                      size_t length,
                      uint8_t **result,
                      size_t *result_length);

/*----------------------------------------------------------------------------*/

/**
        URL decode a source buffer of length to a destination buffer.
        For @mode and @param @see ov_base64_encode
*/
bool ov_base64_url_decode(const uint8_t *buffer,
                          size_t length,
                          uint8_t **result,
                          size_t *result_length);

#endif /* ov_base64_h */
