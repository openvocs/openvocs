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
        @file           ov_buffer.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2019-01-25

        @ingroup        ov_buffer

        @brief          Implementation of a standard data buffer

        ------------------------------------------------------------------------
*/

#include <stdint.h>

#include "../../include/ov_constants.h"

#include "../../include/ov_buffer.h"
#include "../../include/ov_dump.h"
#include "../../include/ov_registered_cache.h"

/*----------------------------------------------------------------------------*/

static ov_registered_cache *g_cache = 0;

/*----------------------------------------------------------------------------*/

static ov_buffer const *as_buffer(const void *vptr) {

  if (0 == vptr)
    return 0;

  ov_buffer const *buffer = vptr;

  if (OV_BUFFER_MAGIC_BYTE != buffer->magic_byte)
    return 0;

  return buffer;
}

/*----------------------------------------------------------------------------*/

static bool adapt_size_nocheck(ov_buffer *buffer, size_t size) {

  if (size < 1) {
    goto error;
  }

  if (buffer->capacity >= size) {
    return true;
  }

  if (0 != buffer->capacity) {

    free(buffer->start);
    buffer->start = 0;
    buffer->capacity = 0;
  }

  OV_ASSERT(0 == buffer->capacity);

  if (size == SIZE_MAX)
    goto error;

  /* Why one excess byte ? */
  buffer->start = calloc(size + 1, sizeof(uint8_t));
  if (!buffer->start) {
    goto error;
  }

  buffer->length = 0;
  buffer->capacity = size;

  return true;

error:

  return false;
}

/*----------------------------------------------------------------------------*/

static ov_buffer *buffer_create(ov_registered_cache *cache,
                                const size_t min_size) {

  ov_buffer *buffer = 0;

  if (0 < min_size) {

    buffer = ov_registered_cache_get(cache);
  }

  if (0 == buffer) {

    buffer = calloc(1, sizeof(ov_buffer));
    buffer->magic_byte = OV_BUFFER_MAGIC_BYTE;
    buffer->start = 0;
    buffer->capacity = 0;
  }

  OV_ASSERT(0 != buffer);

  buffer->length = 0;

  if (0 == min_size) {

    OV_ASSERT(0 == buffer->capacity);
    OV_ASSERT(0 == buffer->start);
    return buffer;
  }

  if (!adapt_size_nocheck(buffer, min_size)) {

    goto error;
  }

  OV_ASSERT(min_size <= buffer->capacity);

  return buffer;

error:

  buffer = ov_buffer_free(buffer);
  return buffer;
}

/*----------------------------------------------------------------------------*/

ssize_t ov_buffer_len(ov_buffer const *self) {

  if (0 == self) {
    return -1;
  } else {
    return self->length;
  }
}

/*----------------------------------------------------------------------------*/

uint8_t const *ov_buffer_data(ov_buffer const *self) {

  if (0 == self) {
    return 0;
  } else {
    return self->start;
  }
}

/*----------------------------------------------------------------------------*/

uint8_t *ov_buffer_data_mutable(ov_buffer *self) {

  if (0 == self) {
    return 0;
  } else {
    return self->start;
  }
}

/*----------------------------------------------------------------------------*/

ov_buffer *ov_buffer_create(size_t size) {

  ov_buffer *buffer = buffer_create(g_cache, size);

  OV_ASSERT(0 != buffer);
  OV_ASSERT(size <= buffer->capacity);
  OV_ASSERT(0 == buffer->length);

  OV_ASSERT((0 != buffer) || (0 == size));
  OV_ASSERT((0 == size) || (0 != buffer));

  return buffer;
}

/*----------------------------------------------------------------------------*/

bool ov_buffer_extend(ov_buffer *buffer, size_t size) {

  if (!buffer || (0 == size))
    goto error;

  size_t new_size = buffer->capacity + size;

  void *new_start = realloc(buffer->start, new_size);
  if (!new_start)
    goto error;

  buffer->start = new_start;
  buffer->capacity = new_size;

  memset(buffer->start + buffer->length, 0, buffer->capacity - buffer->length);
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

ov_buffer *ov_buffer_from_string(const char *string) {

  if (0 == string) {

    ov_log_warning("Cannot convert string to buffer: No String");
    return 0;

  } else {

    size_t len = strlen(string);

    ov_buffer *buffer = ov_buffer_create(len + 1);

    if (ov_buffer_set(buffer, string, len)) {

      buffer->start[buffer->length] = 0;

    } else {

      buffer = ov_buffer_free(buffer);
    }

    return buffer;
  }
}

/*----------------------------------------------------------------------------*/

ov_buffer *ov_buffer_concat(ov_buffer *b1, ov_buffer const *b2) {

  if (0 == b1) {
    return ov_buffer_copy(0, b2);
  } else if (0 == b2) {
    return b1;
  } else {
    size_t len1 = b1->length;
    size_t len2 = b2->length;
    ov_buffer_extend(b1, len1 + len2);
    OV_ASSERT(b1->capacity >= len1 + len2);
    memcpy(b1->start + len1, b2->start, len2);
    b1->length = len1 + len2;
    return b1;
  }
}

/*----------------------------------------------------------------------------*/

ov_buffer *ov_buffer_from_strlist_internal(char const **strlist) {

  if (ov_ptr_valid(strlist, "Invalid string list")) {

    size_t len = 1; // terminal zero

    for (char const **ptr = strlist; *ptr != 0; ++ptr) {
      len += strlen(*ptr);
    }

    ov_buffer *b = ov_buffer_create(len);
    b->length = len;
    char *str = (char *)b->start;
    str[0] = 0;

    for (char const **ptr = strlist; *ptr != 0; ++ptr) {
      strcat(str, *ptr);
    }

    return b;

  } else {
    return 0;
  }
}

/*----------------------------------------------------------------------------*/

ov_buffer *ov_buffer_cast(const void *data) {

  if (!data)
    return NULL;

  if (*(uint16_t *)data == OV_BUFFER_MAGIC_BYTE)
    return (ov_buffer *)data;

  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_buffer_set(ov_buffer *buffer, const void *data, size_t length) {

  if ((0 == as_buffer(buffer)) || (0 == data) || (length < 1)) {
    goto error;
  }

  OV_ASSERT(0 != buffer);

  if (!adapt_size_nocheck(buffer, length)) {

    goto error;
  }

  OV_ASSERT(length <= buffer->capacity);
  OV_ASSERT(0 != buffer->start);

  if (!memcpy(buffer->start, data, length))
    goto error;

  buffer->length = length;
  return true;

error:

  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_buffer_clear(void *self) {

  ov_buffer *buffer = (ov_buffer *)as_buffer(self);

  if (!buffer)
    goto error;

  if (0 == buffer->capacity) {

    // pointer buffer, unset pointer data
    buffer->start = NULL;
    buffer->length = 0;
  }

  if (0 != buffer->start) {

    memset(buffer->start, 0, buffer->capacity);
    buffer->length = 0;
  }

  return true;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

void *ov_buffer_free_uncached(void *self) {

  ov_buffer *buffer = (ov_buffer *)as_buffer(self);

  if (0 != buffer) {

    if (0 == buffer->capacity) {
      buffer->start = 0;
    }

    ov_free(buffer->start);

    return ov_free(buffer);

  } else {

    return self;
  }
}

/*----------------------------------------------------------------------------*/

void *ov_buffer_free(void *self) {

  ov_buffer *buffer = (ov_buffer *)as_buffer(self);

  if (0 == buffer) {

    return buffer;

  } else if (0 != buffer->capacity) {

    memset(buffer->start, 0, buffer->capacity);
    buffer = ov_registered_cache_put(g_cache, buffer);
  }

  return ov_buffer_free_uncached(buffer);
}

/*----------------------------------------------------------------------------*/

void *ov_buffer_copy(void **destination, const void *self) {

  ov_buffer *buffer_to_free_on_error = 0;

  ov_buffer *orig = (ov_buffer *)as_buffer(self);

  if (!orig)
    goto error;

  ov_buffer *copy = 0;

  if (0 != destination) {

    copy = (ov_buffer *)as_buffer(*destination);
  }

  if (0 == copy) {

    copy = ov_buffer_create(orig->length);
    buffer_to_free_on_error = copy;
  }

  OV_ASSERT(0 != copy);

  if (0 == orig->length) {

    copy->length = 0;
    goto done;
  }

  if (!ov_buffer_set(copy, orig->start, orig->length)) {

    goto error;
  }

done:

  if (0 != destination) {

    *destination = copy;
  }

  return copy;

error:

  if (0 != buffer_to_free_on_error) {

    buffer_to_free_on_error = ov_buffer_free(buffer_to_free_on_error);
  }

  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_buffer_dump(FILE *stream, const void *self) {

  ov_buffer const *buffer = as_buffer((void *)self);

  if (!stream || !buffer)
    goto error;

  if (!fprintf(stream,
               "BUFFER DUMP \n"
               "    length   %zd\n"
               "    capacity %zd\n"
               "    content  %s\n"
               "\n",
               buffer->length, buffer->capacity, buffer->start))
    goto error;

  if (buffer->start) {

    fprintf(stream, "BUFFER HEX DUMP\n");

    ov_dump_binary_as_hex(stream, buffer->start, buffer->length);

    fprintf(stream, "\n\n");
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

ov_data_function ov_buffer_data_functions() {

  ov_data_function func = {

      .clear = ov_buffer_clear,
      .free = ov_buffer_free,
      .copy = ov_buffer_copy,
      .dump = ov_buffer_dump};

  return func;
}

/*----------------------------------------------------------------------------*/

bool ov_buffer_push(ov_buffer *buffer, void *data, size_t size) {

  if ((0 == as_buffer(buffer)) || (0 == data) || (1 > size)) {
    goto error;
  }

  const size_t new_length = buffer->length + size;

  /* If start was not allocated ? */
  if (0 == buffer->capacity) {
    uint8_t *old_data = buffer->start;
    /* Again, 1 byte excess ? */
    buffer->start = calloc(1, new_length + 1);
    buffer->capacity = new_length;
    memcpy(buffer->start, old_data, buffer->length);
  }

  if (buffer->capacity < new_length) {

    /* Again, 1 byte excess ? */
    buffer->start = realloc(buffer->start, new_length + 1);

    if (0 == buffer->start) {
      buffer->capacity = 0;
      goto error;
    }

    buffer->capacity = new_length;
  }

  if (!memcpy(buffer->start + buffer->length, data, size))
    goto error;

  buffer->length += size;
  return true;

error:
  return false;
}

/******************************************************************************
 *                                  CACHING
 ******************************************************************************/

static void *free_buffer(void *vptr) {

  return ov_buffer_free_uncached((ov_buffer *)as_buffer(vptr));
}

/*----------------------------------------------------------------------------*/

void ov_buffer_enable_caching(size_t capacity) {

  ov_registered_cache_config cfg = {

      .capacity = capacity,
      .item_free = free_buffer,

  };

  g_cache = ov_registered_cache_extend("buffer", cfg);
}

/*----------------------------------------------------------------------------*/

bool ov_buffer_shift(ov_buffer *buffer, uint8_t *next) {

  if (!buffer || !buffer->start || !next)
    goto error;

  int64_t length = next - buffer->start;
  if (length < 0)
    goto error;

  if (length > (int64_t)buffer->length)
    goto error;

  if (length == (int64_t)buffer->length)
    return ov_buffer_clear(buffer);

  return ov_buffer_shift_length(buffer, (size_t)length);

error:

  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_buffer_equals(ov_buffer const *self, char const *refstr) {

  if ((0 == refstr) || (0 == self)) {
    return (0 == refstr) && (0 == self);
  } else if (strlen(refstr) != self->length) {
    return false;
  } else {
    return 0 == strncmp(refstr, (char const *)self->start, self->length);
  }
}

/*----------------------------------------------------------------------------*/

bool ov_buffer_shift_length(ov_buffer *buffer, size_t length) {

  if (!buffer || !buffer->start)
    return false;

  if (length == 0)
    return true;

  if (length > buffer->length)
    return false;

  if (length == buffer->length)
    return ov_buffer_clear(buffer);

  size_t size = buffer->length - length;
  uint8_t temp[size];
  memset(temp, 0, size);

  if (!memcpy(temp, buffer->start + length, size))
    return false;

  if (!ov_buffer_clear(buffer))
    return false;

  return ov_buffer_set(buffer, temp, size);
}
