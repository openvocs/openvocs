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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

        Chunk data into blocks of fixed size.

        Do like:

        ov_buffer *data = get_data();

        // Collect data
        while(0 != data) {
           ov_chunker_add(us, data);
           ov_buffer_free(data);
           data = get_data();
        };

        // Get it back as 20 byte chunks
        data = ov_chunker_next_chunk(us, 20);

        while(data) {
           consume(data);
           data = ov_chunker_next_chunk(us, 20);
        }

        // get final incomplete chunk
        data = ov_chunker_remainder(us);
        consume(data);

        Beware:

        add and next operations can be intermingled:

        ov_buffer *data = get_data();

        while(0 != data) {

           ov_chunker_add(us, data);
           ov_buffer_free(data);

           data = ov_chunker_next_chunk(us, 20);

           if(0 != data) {
              consume(data);
           }

           data = get_data();

        }

        // get final incomplete chunk
        data = ov_chunker_remainder(us);
        consume(data);

        ------------------------------------------------------------------------
*/
#ifndef OV_CHUNKER_H
#define OV_CHUNKER_H
/*----------------------------------------------------------------------------*/

#include "ov_buffer.h"

/*----------------------------------------------------------------------------*/

struct ov_chunker_struct;
typedef struct ov_chunker_struct ov_chunker;

ov_chunker *ov_chunker_create();

ov_chunker *ov_chunker_free(ov_chunker *self);

size_t ov_chunker_available_octets(ov_chunker *self);

bool ov_chunker_add(ov_chunker *self, ov_buffer const *data);

/**
 * Peek into the currently available data.
 * If not at least num_octets are available, 0 is returned.
 * If at least num_octets are available, a pointer to an array of
 * num_octets length is returned. This data MUST NOT be altered, the pointer
 * MUST NOT be freed.
 */
uint8_t const *ov_chunker_next_chunk_preview(ov_chunker *self,
                                             size_t num_octets);

bool ov_chunker_next_chunk_raw(ov_chunker *self,
                               size_t num_octets,
                               uint8_t *dest);

ov_buffer *ov_chunker_next_chunk(ov_chunker *self, size_t num_octets);

/**
 * Returns whatever is left in the internal buffers.
 */
ov_buffer *ov_chunker_remainder(ov_chunker *self);

void ov_chunker_enable_caching();

/*----------------------------------------------------------------------------*/

#endif
