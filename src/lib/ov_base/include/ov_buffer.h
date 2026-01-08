/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_buffer.h
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2019-01-25

        @ingroup        ov_buffer

        @brief          Definition of a standard buffer.

                        @NOTE @CONVENTION

                        if capacity == 0 start has not been allocated by us
                        if capacity  > 0 start is allocated and
                                         length is current used size

                        if capacity != 0 ov_buffer_free will free the pointer
                        at start

        ------------------------------------------------------------------------
*/
#ifndef ov_buffer_h
#define ov_buffer_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "ov_data_function.h"
#include "ov_utils.h"

/*----------------------------------------------------------------------------*/

#define OV_BUFFER_MAGIC_BYTE 0xBBBB

/*----------------------------------------------------------------------------*/

typedef struct {

  uint16_t magic_byte;

  uint8_t *start;  // start of data
  size_t length;   // length of data
  size_t capacity; // allocated capacity (if 0 start is pointer)

} ov_buffer;

ssize_t ov_buffer_len(ov_buffer const *self);
uint8_t const *ov_buffer_data(ov_buffer const *self);
uint8_t *ov_buffer_data_mutable(ov_buffer *self);

/*
 *      ------------------------------------------------------------------------
 *
 *      PUBLIC FUNCTION IMPLEMENTATION
 *
 *      ------------------------------------------------------------------------
 */

/**
        Create a buffer with given size.
        @param size     defines size allocated for data
*/
ov_buffer *ov_buffer_create(size_t size);

/*----------------------------------------------------------------------------*/

/**
        Create a buffer and copy data with length size to buffer.
*/
ov_buffer *ov_buffer_from_string(const char *string);

/*----------------------------------------------------------------------------*/

/**
 * Appends b2 at the end of b1
 * The result is placed within b1 - no new buffer is created
 */
ov_buffer *ov_buffer_concat(ov_buffer *b1, ov_buffer const *b2);

/*----------------------------------------------------------------------------*/

/**
 * Creates a zero-terminated buffer containing a concatenation of all strings
 */
#define ov_buffer_from_strlist(...)                                            \
  ov_buffer_from_strlist_internal((char const *[]){__VA_ARGS__, 0})

/*----------------------------------------------------------------------------*/
/**
        Cast to ov_buffer
*/
ov_buffer *ov_buffer_cast(const void *data);

/*----------------------------------------------------------------------------*/

/**
        Copy some data to buffer start.
        This function will take into account current buffer capacity and
        adjust the capacity to the new content length, if required.

        @param buffer   buffer to use
        @param data     start of data to copy
        @param length   length of data to copy
*/
bool ov_buffer_set(ov_buffer *buffer, const void *data, size_t length);

/*----------------------------------------------------------------------------*/

/**
        PUSH some data to the end of the buffer.

        @param buffer   buffer to use
        @param data     data to write behind buffer->length
        @param size     required size
*/
bool ov_buffer_push(ov_buffer *buffer, void *data, size_t size);

/*----------------------------------------------------------------------------*/

/**
        Shift buffer start to next.

        This function will delete some memory area at the start of the
        buffer.

        @param buffer   buffer to use
        @param next     pointer to next byte in buffer
*/
bool ov_buffer_shift(ov_buffer *buffer, uint8_t *next);

/*----------------------------------------------------------------------------*/

bool ov_buffer_equals(ov_buffer const *self, char const *refstr);

/*----------------------------------------------------------------------------*/

/**
        Shift buffer start by length

        This function will delete some memory area at the start of the
        buffer.

        @param buffer   buffer to use
        @param length   length of the buffer to cut at start
*/
bool ov_buffer_shift_length(ov_buffer *buffer, size_t length);

/*----------------------------------------------------------------------------*/

/**
        Increases the buffer->capacity by additional_bytes

        @param buffer       buffer to use
        @param add_bytes    additional bytes to add to capacity
*/
bool ov_buffer_extend(ov_buffer *buffer, size_t add_bytes);

/*
 *      ------------------------------------------------------------------------
 *
 *      DEFAULT DATA FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_buffer_clear(void *self);
void *ov_buffer_free(void *self);
void *ov_buffer_copy(void **destination, const void *self);
bool ov_buffer_dump(FILE *stream, const void *self);

void *ov_buffer_free_uncached(void *self);

ov_data_function ov_buffer_data_functions();

/******************************************************************************
 *                                  CACHING
 ******************************************************************************/

/**
 * Enables caching.
 * BEWARE: Call ov_registered_cache_free_all() before exiting your process to
 * avoid memleaks!
 */
void ov_buffer_enable_caching(size_t capacity);

/*----------------------------------------------------------------------------*/

/*****************************************************************************
                                    INTERNAL
 ****************************************************************************/

ov_buffer *ov_buffer_from_strlist_internal(char const **strlist);

#endif /* ov_buffer_h */
