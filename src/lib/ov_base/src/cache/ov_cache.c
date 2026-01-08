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
        @file           ov_cache.c
        @author         Michael Beer
        @author         Markus Toepfer

        @date           2018-12-11

        ------------------------------------------------------------------------
*/

#include "../../include/ov_cache.h"
#include "../../include/ov_utils.h"
/*----------------------------------------------------------------------------*/

#ifdef __STDC_NO_ATOMICS__
#define OV_DISABLE_CACHING
#endif

/******************************************************************************
 *                              CACHING ENABLED
 ******************************************************************************/

#ifndef OV_DISABLE_CACHING

#include <stdatomic.h>
#include <stdlib.h>

#include "../../include/ov_constants.h"

/*----------------------------------------------------------------------------*/

struct ov_cache_struct {

    size_t capacity;
    size_t next_free;
    void **elements;
    atomic_flag in_use;

    bool (*element_checker)(void *);
};

static ov_cache *cache_create(size_t capacity) {

    ov_cache *cache = 0;

    cache = calloc(1, sizeof(ov_cache));
    cache->elements = calloc(capacity, sizeof(void *));
    cache->capacity = capacity;
    cache->next_free = 0;

    atomic_flag_clear(&cache->in_use);

    return cache;
}

/*----------------------------------------------------------------------------*/

ov_cache *ov_cache_extend(ov_cache *cache, size_t capacity) {

    if (0 == capacity) {

        capacity = OV_DEFAULT_CACHE_SIZE;
    }

    if (0 == cache) {

        return cache_create(capacity);
    }

    capacity += cache->capacity;

    cache->elements = realloc(cache->elements, sizeof(void *) * capacity);

    cache->capacity = capacity;

    return cache;
}

/*----------------------------------------------------------------------------*/

ov_cache *ov_cache_free(ov_cache *restrict self, uint64_t timeout_usec,
                        void *(*item_free)(void *)) {

    if (0 == self)
        goto error;

    uint64_t start = ov_time_get_current_time_usecs();
    uint64_t current = 0;

    while (atomic_flag_test_and_set(&self->in_use)) {

        current = ov_time_get_current_time_usecs();

        if (0 == timeout_usec)
            continue;

        if (current > timeout_usec + start)
            goto error;
    }

    if ((0 != self->elements) && (item_free != 0)) {

        for (size_t i = 0; i < self->next_free; i++) {
            item_free(self->elements[i]);
        }
    }

    if (0 != self->elements) {

        free(self->elements);
        self->elements = 0;
    }

    free(self);
    self = 0;

error:

    return self;
}

/*---------------------------------------------------------------------------*/

void *ov_cache_get(ov_cache *restrict self) {

    if (0 == self)
        goto error;

    if (atomic_flag_test_and_set(&self->in_use)) {

        /* Cache is in use - cannot get object */
        goto error;
    }

    if (0 == self->elements)
        goto error;

    void *object = 0;

    if (0 != self->next_free) {
        --self->next_free;
        object = self->elements[self->next_free];
    }

    atomic_flag_clear(&self->in_use);

    return object;

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

static bool element_type_correct_nocheck(ov_cache *restrict self,
                                         void *object) {

    if (0 == self->element_checker) {

        return true;
    }

    return self->element_checker(object);
}

/*----------------------------------------------------------------------------*/

void *ov_cache_put(ov_cache *restrict self, void *object) {

    if (0 == self)
        goto error;

    if (!element_type_correct_nocheck(self, object)) {

        goto error;
    }

    if (atomic_flag_test_and_set(&self->in_use)) {

        /* Cache is in use - cannot get object */
        goto error;
    }

    if (0 == self->elements)
        goto error;

    if (self->capacity == self->next_free) {
        goto set_unused_and_return;
    }

    self->elements[self->next_free] = object;
    ++self->next_free;

    object = 0;

set_unused_and_return:

    atomic_flag_clear(&self->in_use);

error:

    return object;
}

/*----------------------------------------------------------------------------*/

void ov_cache_set_element_checker(ov_cache *self,
                                  bool (*element_checker)(void *)) {

    if (0 == self) {

        return;
    }

    self->element_checker = element_checker;
}

/******************************************************************************
 *                              CACHING DISABLED
 ******************************************************************************/

#else /* OV_DISABLE_CACHING */

struct ov_cache_struct {
    bool dummy : 1;
};

ov_cache *ov_cache_extend(ov_cache *cache, size_t capacity) {

    UNUSED(cache);
    UNUSED(capacity);

    return 0;
}

/*----------------------------------------------------------------------------*/

ov_cache *ov_cache_free(ov_cache *restrict self, uint64_t timeout_usec,
                        void *(*item_free)(void *)) {

    UNUSED(self);
    UNUSED(timeout_usec);
    UNUSED(item_free);
    return 0;
}

/*----------------------------------------------------------------------------*/

void *ov_cache_get(ov_cache *restrict self) {

    UNUSED(self);
    return 0;
}

/*----------------------------------------------------------------------------*/

void *ov_cache_put(ov_cache *restrict self, void *object) {

    UNUSED(self);
    UNUSED(object);
    return 0;
}

/*----------------------------------------------------------------------------*/

void ov_cache_set_element_checker(ov_cache *self,
                                  bool (*element_checker)(void *)) {

    UNUSED(self);
    UNUSED(element_checker);
}

/*----------------------------------------------------------------------------*/

#endif
