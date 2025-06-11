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
        @file           ov_ringbuffer.h
        @author         Michael Beer

        @date           2018-01-11

        @ingroup        ov_ringbuffer

        @brief          Definition of a ring buffer.

                        A ring buffer is a quite versatile structure,
                        allowing for easy implementation of
                        e.g. message queues and caches.

        ------------------------------------------------------------------------
*/
#ifndef ov_ringbuffer_h
#define ov_ringbuffer_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct ov_ringbuffer_struct ov_ringbuffer;
typedef struct ov_ringbuffer_statistics_struct ov_ringbuffer_statistics;

/*---------------------------------------------------------------------------*/

struct ov_ringbuffer_struct {

    uint32_t magic_bytes;

    /**
     * Returns the max number of elements this buffer may hold before
     * starting to overwrite elements.
     */
    size_t (*capacity)(const ov_ringbuffer *self);
    /**
     * Retrieves and erases an element from the ring buffer.
     * Prefer directly calling ov_ringbufer_pop() ...
     */
    void *(*pop)(ov_ringbuffer *self);

    /**
     * Inserts an element into the ring buffer.
     * '0' as element is prohibited.
     * Prefer ov_ringbuffer_insert() ...
     */
    bool (*insert)(ov_ringbuffer *self, void *element);
    bool (*clear)(ov_ringbuffer *self);
    ov_ringbuffer *(*free)(ov_ringbuffer *self);
    ov_ringbuffer_statistics (*get_statistics)(const ov_ringbuffer *self);
};

/*---------------------------------------------------------------------------*/

struct ov_ringbuffer_statistics_struct {

    uint64_t elements_inserted;
    uint64_t elements_dropped;
};

/*---------------------------------------------------------------------------*/

/**
        Creates a new ringbuffer.

        The dispose_element function is required in case the buffer fills up.
        It will then start to overwrite elements, and those need to be freed.
        'dispose_element' takes two arguments: The second one being the element
        to be freed.
        The first one is 'additional_arg'.

        @param capacity                 Max. number of elements to be contained
        @param free_element        Function to free an element placed in
                                        the buffer
 */
ov_ringbuffer *ov_ringbuffer_create(size_t capacity,
                                    void (*free_element)(void *additional_arg,
                                                         void *element_to_free),
                                    void *additional_arg);

/*----------------------------------------------------------------------------*/

ov_ringbuffer *ov_ringbuffer_free(ov_ringbuffer *self);

/*----------------------------------------------------------------------------*/

bool ov_ringbuffer_insert(ov_ringbuffer *self, void *element);

/*----------------------------------------------------------------------------*/

void *ov_ringbuffer_pop(ov_ringbuffer *self);

/*----------------------------------------------------------------------------*/

void ov_ringbuffer_enable_caching(size_t capacity);

/*----------------------------------------------------------------------------*/

#endif /* ov_ringbuffer_h */
