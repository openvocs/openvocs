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
        @file           ov_cache.h
        @author         Michael Beer
        @author         Markus Toepfer

        @date           2018-12-11

        @ingroup        ov_cache

        @brief          Definition of a standard cache infrastructure


        ------------------------------------------------------------------------
*/
#ifndef ov_cache_h
#define ov_cache_h

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "ov_time.h"
#include "ov_utils.h"

/*----------------------------------------------------------------------------*/

typedef struct ov_cache_struct ov_cache;

typedef struct {
    size_t capacity;
} ov_cache_config;

/*----------------------------------------------------------------------------*/

/**
 * Creates new or extends the capacity of an existing cache, possibly by
 * creating a new cache.
 * The new cache has a capacity of capacity(old_cache) + capacity .
 *
 * @return new / extended cache
 *
 * BEWARE: No content is lost - if a new cache must be created, the elements in
 * the old cache are copied into the new cache.
 */
ov_cache *ov_cache_extend(ov_cache *cache, size_t capacity);

/*----------------------------------------------------------------------------*/

/**
        Free (terminate) a cache.
        @param timeout_usec if 0, will try locking forever
*/
ov_cache *ov_cache_free(ov_cache *restrict self,
                        uint64_t timeout_usec,
                        void *(*item_free)(void *));

/*----------------------------------------------------------------------------*/

/**
        Get an object out of the cache.
*/
void *ov_cache_get(ov_cache *restrict self);

/*----------------------------------------------------------------------------*/

/**
        Put an object out to the cache.

        @returns object in case of error
        @returns NULL in case of success
*/
void *ov_cache_put(ov_cache *restrict self, void *object);

/*----------------------------------------------------------------------------*/

/**
 * Set a method to check added pointers for valid type.
 * Required since we must use void pointers and cannot use the compiler to check
 * for us
 */
void ov_cache_set_element_checker(ov_cache *cache,
                                  bool (*element_checker)(void *));

/*----------------------------------------------------------------------------*/

#endif /* ov_cache_h */
