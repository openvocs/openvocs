/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/
#include "../include/ov_chunker.h"
#include "ov_buffer.h"
#include <../include/ov_registered_cache.h>

/*----------------------------------------------------------------------------*/

#define MAGIC_BYTES 0x43ffff43

struct ov_chunker_struct {

    uint32_t magic_bytes;

    ov_buffer *buffer;
    uint8_t const *read_ptr;
};

/*----------------------------------------------------------------------------*/

static ov_chunker const *as_chunker(void const *vptr) {

    ov_chunker const *c = vptr;

    if (ov_ptr_valid(c, "Not a valid chunker: 0 pointer") &&
        ov_cond_valid(MAGIC_BYTES == c->magic_bytes,
                      "Not a valid chunker: Magic bytes do not match")) {
        return c;
    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ov_chunker *as_chunker_mutable(void *vptr) {
    return (ov_chunker *)as_chunker(vptr);
}

/*----------------------------------------------------------------------------*/

ov_chunker *ov_chunker_create() {

    ov_chunker *self = calloc(1, sizeof(ov_chunker));

    self->magic_bytes = MAGIC_BYTES;
    self->buffer = ov_buffer_create(1);
    OV_ASSERT(0 != self->buffer);
    self->read_ptr = self->buffer->start;

    return self;
}

/*----------------------------------------------------------------------------*/

ov_chunker *ov_chunker_free(ov_chunker *self) {

    self = as_chunker_mutable(self);

    if (0 != self) {
        self->buffer = ov_buffer_free(self->buffer);
    }

    return ov_free(self);
}

/*----------------------------------------------------------------------------*/

static bool append_to_buffer(ov_buffer *target, ov_buffer const *ext) {

    if ((0 == target) || (0 == ext)) {

        return false;

    } else if (target->capacity - target->length > ext->length) {

        if (0 < ext->length) {
            memcpy(target->start + target->length, ext->start, ext->length);
        }
        target->length += ext->length;

        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static ov_buffer *merge_data(uint8_t const *restrict b1, size_t b1len,
                             size_t b1capacity, uint8_t const *restrict b2,
                             size_t b2len) {

    size_t required_len = b1len + b2len;

    if ((0 == b1) || (0 == b2)) {
        return 0;
    } else {

        if (required_len > b1capacity) {
            required_len = b1len + 3 * b2len;
        }

        ov_buffer *merged = ov_buffer_create(required_len);
        OV_ASSERT(0 != merged);
        OV_ASSERT(0 != merged->start);

        merged->length = b1len + b2len;

        if (b1len > 0) {
            memcpy(merged->start, b1, b1len);
        }

        if (b2len > 0) {
            memcpy(merged->start + b1len, b2, b2len);
        }

        return merged;
    }
}

/*----------------------------------------------------------------------------*/

size_t ov_chunker_available_octets(ov_chunker *self) {

    if (ov_ptr_valid(as_chunker(self),
                     "Cannot get available octets - invalid chunker "
                     "instance")) {

        return self->buffer->length - (self->read_ptr - self->buffer->start);

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_chunker_add(ov_chunker *self, ov_buffer const *data) {

    if (ov_ptr_valid(as_chunker(self),
                     "Cannot add data - no valid chunker instance") &&
        ov_ptr_valid(data, "Cannot add data - no data (0 pointer)")) {

        if ((0 != data->length) && !append_to_buffer(self->buffer, data)) {

            // Trying to 'defrag' self->buffer does not yield anything,
            // because in order to copy round data within the buffer,
            // we would require a copy of the data...
            // Then we can just create the copy straight away and
            // work on it.
            ov_buffer *merged = merge_data(
                self->read_ptr,
                self->buffer->length - (self->read_ptr - self->buffer->start),
                self->buffer->capacity, data->start, data->length);

            if (ov_ptr_valid(merged, "Could not add data: Could not merge")) {

                ov_buffer_free(self->buffer);
                self->buffer = merged;
                self->read_ptr = merged->start;
                OV_ASSERT(self->buffer);
                OV_ASSERT(self->read_ptr);

            } else {

                return false;
            }
        }

        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

uint8_t const *ov_chunker_next_chunk_preview(ov_chunker *self,
                                             size_t num_octets) {

    if ((0 < num_octets) &&
        ov_ptr_valid(as_chunker(self), "Cannot get more data - invalid "
                                       "chunker "
                                       "instance")) {

        size_t available_octets =
            self->buffer->length - (self->read_ptr - self->buffer->start);

        if (ov_cond_valid_debug(num_octets <= available_octets,
                                "Cannot get more data - not enough data")) {

            return self->read_ptr;

        } else {

            return 0;
        }

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_chunker_next_chunk_raw(ov_chunker *self, size_t num_octets,
                               uint8_t *dest) {

    if ((0 < num_octets) &&
        ov_ptr_valid(as_chunker(self), "Cannot get more data - invalid chunker "
                                       "instance") &&
        ov_ptr_valid(dest, "Cannot get more data - 0 pointer")) {

        size_t available_octets =
            self->buffer->length - (self->read_ptr - self->buffer->start);

        if (ov_cond_valid_debug(num_octets <= available_octets,
                                "Cannot get more data - not enough data")) {

            memcpy(dest, self->read_ptr, num_octets);
            self->read_ptr += num_octets;

            return true;

        } else {

            return false;
        }

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

ov_buffer *ov_chunker_next_chunk(ov_chunker *self, size_t num_octets) {

    ov_buffer *data = ov_buffer_create(num_octets);

    if (ov_chunker_next_chunk_raw(self, num_octets, data->start)) {

        data->length = num_octets;

    } else {

        data = ov_buffer_free(data);
    }

    return data;
}

/*----------------------------------------------------------------------------*/

ov_buffer *ov_chunker_remainder(ov_chunker *self) {

    if (ov_ptr_valid(as_chunker(self), "Cannot get more data - invalid chunker "
                                       "instance")) {

        size_t available_octets =
            self->buffer->length - (self->read_ptr - self->buffer->start);

        return ov_chunker_next_chunk(self, available_octets);

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

void ov_chunker_enable_caching() {

    // We are going to use at most 1 buffers at the same time
    ov_buffer_enable_caching(1);
}

/*----------------------------------------------------------------------------*/
