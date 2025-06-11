/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

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
        @author         Michael Beer


        ALWAYS RECALL:

        NON-PERFORMANT CACHING DOES NOT MAKE SENSE!


        This is a cache implementation with all caches being registered
        centrally. While the registry is centralized, all cache instances
        of ov_registered_cache MUST be static global within the implementation
        using this type of cache. (see below)

        This allows for reporting on cache usage and:
        It allows to manage cache sizes locally.

        Use when implementing a data structure that should be cached
        `ov_cached_datastructure`:

        static ov_registered_cache *g_cache;

        void ov_data_structure_enable_caching(size_t capacity) {

            ov_registered_cache_config cfg = {
                .item_free = data_structure_free,
                .capacity = capacity,
            };

            ov_registered_cache_extend(g_cache, cfg);

        }

        ov_data_structure *ov_data_structure_create() {

            ov_data_structure *ds = ov_cache_get(g_cache);

            if(0 == ds) {

                ds == calloc(1, sizeof(ov_data_structure));

            }

            return ds;

        }

        ov_data_structure_free(ov_data_structure *self) {

            if(0 == self) goto finish;

            self = ov_registry_cache_put(g_cache, self);

            if(0 != self) {

                free(self);
                self = 0;

            }

        finish:

            return self;

        }

        If `ov_data_structure` contains other data structures internally,
        it should extend their cache accordingly:

        typedef struct {

            // Implemented via `ov_linked_list`
            ov_list *elements;

        } ov_data_structure;

        ov_data_structure_enable_caching(size_t capacity) {

            ov_registered_cache_config cfg = {
                .item_free = data_structure_free,
                .capacity = capacity,
            };

            ov_registered_cache_extend(g_cache, cfg);

            // Adapt linked list cache accordingly:
            // Each ov_data_structure requires one linked list,
            // thus the cache for linked_list should be extended by
            // the amount of ov_data_structures to cache
            ov_linked_list_enable_caching(capacity);

        }

        // Remainder of functions stays the same

        This way, the cache for ov_linked_list is kept at a appropriate size.

        The caches are freed when `ov_registered_cache_free_all()` is called.

        This should be done once *AND ONLY ONCE* immediately before terminating
        the process - e.g. before leaving the main method.

        The registered cache must not have functionality to disable created
        caches.
        We must be able to cache the pointer to the cache
        (e.g. ov_buffer needs to be able to retain its static g_cache pointer).

        Otherwise we cannot cache the cache pointer and always have to retrieve
        the pointer when trying to get / put on the cache like

        // Inflicts heavy performance hit due to hashtable lookup &
        // necessity to lock the cache registry
        my_cache = ov_registered_cache_get_cache("my_struct");
        ov_registered_cache_put(my_cache, my_struct);
        ...
        // compressing into a single call does NOT solve the root problem,
        // still a hashtable lookup & and lock is required is needed
        ov_registered_cache_put("my_struct", my_struct);

        wich causes a hashtable look up.
        Moreover, in order to be thread-safe, for conducting the hashtable
        lookup, the hashtable has to be looked every single time an object is
        put/retrieved .

        This contradicts the very idea to gain performance by caching
        if the caching itself becomes so costly.

        ------------------------------------------------------------------------
*/
#ifndef ov_registered_cache_h
#define ov_registered_cache_h

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "ov_time.h"
#include "ov_utils.h"

#include "ov_hashtable.h"
#include "ov_json.h"
#include "ov_json_value.h"

/*----------------------------------------------------------------------------*/

typedef struct ov_registered_cache_struct ov_registered_cache;

typedef struct {

    uint64_t timeout_usec;
    void *(*item_free)(void *);
    size_t capacity;

} ov_registered_cache_config;

/*----------------------------------------------------------------------------*/

/**
 * Create / extend a registered cache.
 * Registered caches can be reported on via `ov_cache_report` and are freed
 * by `ov_cache_free_all`
 *
 * BEWARE:
 * Call this function *ONLY ONCE* and *ONLY AFTER YOU ARE SURE NOBODY USES
 * ANY OF THE REGISTERED CACHES ANY LONGER!*
 */
ov_registered_cache *ov_registered_cache_extend(char const *cache_name,
                                                ov_registered_cache_config cfg);

/*----------------------------------------------------------------------------*/

void ov_registered_cache_free_all();

/*----------------------------------------------------------------------------*/

/**
        Get an object out of the cache.
*/
void *ov_registered_cache_get(ov_registered_cache *restrict self);

/*----------------------------------------------------------------------------*/

/**
        Put an object out to the cache.

        @returns object in case of error
        @returns NULL in case of success
*/
void *ov_registered_cache_put(ov_registered_cache *restrict self, void *object);

/*----------------------------------------------------------------------------*/

/**
 * Set a method to check added pointers for valid type.
 * Required since we must use void pointers and cannot use the compiler to check
 * for us
 */
void ov_registered_cache_set_element_checker(ov_registered_cache *cache,
                                             bool (*element_checker)(void *));

/*----------------------------------------------------------------------------*/

/**
 * Creates a report of all registered caches.
 * The report is a json object containig at least capacity & number of
 * cache entries in use for each cache.
 */
ov_json_value *ov_registered_cache_report(ov_json_value *target);

/*****************************************************************************
                                 CONFIGURATION
 ****************************************************************************/

typedef struct ov_registered_cache_sizes ov_registered_cache_sizes;

/*----------------------------------------------------------------------------*/

ov_registered_cache_sizes *ov_registered_cache_sizes_free(
    ov_registered_cache_sizes *self);

/*----------------------------------------------------------------------------*/

/**
 * Parse in cache config from json.
 *
 * JSON looks like
 *
 * {
 * "enable_caching" : true,
 * "cache_sizes" : {
 *    "buffers" : 10
 *    }
 * }
 */
ov_registered_cache_sizes *ov_registered_cache_sizes_from_json(
    ov_json_value const *jval);

/*----------------------------------------------------------------------------*/

/**
 * Get configured cache size for cache `cache_name` out of `sizes`.
 * Returns default cache size if appropriate or 0 if disabled
 */
size_t ov_registered_cache_size_for(ov_registered_cache_sizes *sizes,
                                    char const *cache_name);

/*----------------------------------------------------------------------------*/

bool ov_registered_cache_sizes_configure(ov_registered_cache_sizes *sizes);

/*----------------------------------------------------------------------------*/

#endif /* ov_registered_cache_h */
