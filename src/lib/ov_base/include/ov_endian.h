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
        @file           ov_endian.h
        @author         Markus Toepfer

        @date           2018-11-21

        @ingroup        ov_base

        @brief          Definition of some endian write and read functions.

                        This implementation is independent of the endianess
                        used at the platform and POSIX compliant.
                        It is intent to be used for network protocols, to write
                        in big endian network byte order.

                        @NOTE There are most probably more efficient plattform
                        specific implementations.

        ------------------------------------------------------------------------
*/
#ifndef ov_endian_h
#define ov_endian_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      WRITE from whatever HOST endianess is used, to BIG ENDIAN
 *
 *      ... example usage (write and move ptr in some buffer of buffer_size)
 *
 *      if (!ov_endian_write_ ... (
 *              ptr,
 *              buffer_size - (ptr - buffer_start),
 *              &ptr,
 *              number))
 *              goto error;
 *
 *      ------------------------------------------------------------------------
 */

/**
        Write a uint16_t number to buffer in network byte order.

        @param buffer   start of the buffer to write to
        @param length   of the buffer to write to (MUST be at least 2)
        @param next     (optional) pointer to byte after write
        @param number   number to write in network byte order.
*/
bool ov_endian_write_uint16_be(uint8_t *buffer,
                               size_t length,
                               uint8_t **next,
                               uint16_t number);

/*----------------------------------------------------------------------------*/

/**
        Write a uint32_t number to buffer in network byte order.

        @param buffer   start of the buffer to write to
        @param length   of the buffer to write to (MUST be at least 4)
        @param next     (optional) pointer to byte after write
        @param number   number to write in network byte order.
*/
bool ov_endian_write_uint32_be(uint8_t *buffer,
                               size_t length,
                               uint8_t **next,
                               uint32_t number);

/*----------------------------------------------------------------------------*/

/**
        Write a uint64_t number to buffer in network byte order.

        @param buffer   start of the buffer to write to
        @param length   of the buffer to write to (MUST be at least 8)
        @param next     (optional) pointer to byte after write
        @param number   number to write in network byte order.
*/
bool ov_endian_write_uint64_be(uint8_t *buffer,
                               size_t length,
                               uint8_t **next,
                               uint64_t number);

/*
 *      ------------------------------------------------------------------------
 *
 *      READ to whatever HOST endianess is used, from BIG ENDIAN
 *
 *      ------------------------------------------------------------------------
 */

/**
        Read a uint16_t number from a buffer in network byte order.

        @param buffer   start of the buffer to read from
        @param length   of the buffer to read from (MUST be at least 2)
        @param number   pointer to number
*/
bool ov_endian_read_uint16_be(const uint8_t *buffer,
                              size_t length,
                              uint16_t *number);

/*----------------------------------------------------------------------------*/

/**
        Read a uint32_t number from a buffer in network byte order.

        @param buffer   start of the buffer to read from
        @param length   of the buffer to read from (MUST be at least 4)
        @param number   pointer to number
*/
bool ov_endian_read_uint32_be(const uint8_t *buffer,
                              size_t length,
                              uint32_t *number);

/*----------------------------------------------------------------------------*/

/**
        Read a uint64_t number from a buffer in network byte order.

        @param buffer   start of the buffer to read from
        @param length   of the buffer to read from (MUST be at least 8)
        @param number   pointer to number
*/
bool ov_endian_read_uint64_be(const uint8_t *buffer,
                              size_t length,
                              uint64_t *number);

#endif /* ov_endian_h */
