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

        ------------------------------------------------------------------------
*/

#include "../../include/ov_registered_cache.h"
#include "../../include/ov_utils.h"
/*----------------------------------------------------------------------------*/

#ifdef __STDC_NO_ATOMICS__
#define OV_DISABLE_CACHING
#endif

#include "../../include/ov_config_keys.h"

/******************************************************************************
 *                              CACHING ENABLED
 ******************************************************************************/

#ifndef OV_DISABLE_CACHING

#include "../../include/ov_constants.h"
#include <stdatomic.h>
#include <stdlib.h>

#include "../../include/ov_hashtable.h"
#include "../../include/ov_thread_lock.h"

#include "../../include/ov_buffer.h"
#include "../../include/ov_linked_list.h"
#include "../../include/ov_rtp_frame.h"
#include "../../include/ov_string.h"
#include "../../include/ov_teardown.h"

/*----------------------------------------------------------------------------*/

struct ov_registered_cache_struct {

    size_t capacity;
    size_t next_free;
    void **elements;
    atomic_flag in_use;

    bool (*element_checker)(void *);

    void *(*item_free)(void *);
    uint64_t timeout_usec;

    struct {

        size_t elements_put;
        size_t elements_got;

        size_t get_called;
        size_t put_called;

    } stats;
};

static ov_hashtable *g_registry = 0;
static ov_thread_lock g_lock;

/*----------------------------------------------------------------------------*/

static ov_registered_cache *cache_create(size_t capacity) {

    ov_registered_cache *cache = 0;

    cache = calloc(1, sizeof(ov_registered_cache));
    cache->elements = calloc(capacity, sizeof(void *));
    cache->capacity = capacity;
    cache->next_free = 0;

    atomic_flag_clear(&cache->in_use);

    // Let's ensure that ov_teardown() also frees all caches
    ov_teardown_register(ov_registered_cache_free_all, "Caches");

    return cache;
}

/*----------------------------------------------------------------------------*/

static ov_registered_cache *cache_extend(ov_registered_cache *cache,
                                         size_t capacity) {

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

ov_registered_cache *
ov_registered_cache_extend(char const *cache_name,
                           ov_registered_cache_config cfg) {

    bool registry_locked = false;

    if (0 == cache_name) {

        ov_log_error("No cache name given");
        goto error;
    }

    ov_log_debug("Enabling caching for %s with %zu buckets", cache_name,
                 cfg.capacity);

    if (0 == g_registry) {

        g_registry = ov_hashtable_create_c_string(20);
        ov_thread_lock_init(&g_lock, 1000 * 1000);
    }

    if (!ov_thread_lock_try_lock(&g_lock)) {

        ov_log_error("Could not lock cache registry");
        goto error;
    }

    registry_locked = true;

    ov_registered_cache *cache = ov_hashtable_get(g_registry, cache_name);

    if ((0 != cache) && (cfg.item_free != cache->item_free)) {

        ov_log_error("Wont manipulate entry for cache %s = item_free differs",
                     cache_name);
        goto error;
    }

    cache = cache_extend(cache, cfg.capacity);

    OV_ASSERT(0 != cache);

    cache->item_free = cfg.item_free;
    cache->timeout_usec = cfg.timeout_usec;

    ov_hashtable_set(g_registry, cache_name, cache);

    ov_thread_lock_unlock(&g_lock);

    return cache;

error:

    if (registry_locked) {

        ov_thread_lock_unlock(&g_lock);
    }

    return 0;
}

/*----------------------------------------------------------------------------*/

static ov_registered_cache *cache_free(ov_registered_cache *restrict self) {

    if (0 == self)
        goto error;

    uint64_t start = ov_time_get_current_time_usecs();
    uint64_t current = 0;

    while (atomic_flag_test_and_set(&self->in_use)) {

        current = ov_time_get_current_time_usecs();

        if (0 == self->timeout_usec)
            continue;

        if (current > self->timeout_usec + start)
            goto error;
    }

    size_t no_elements_freed = 0;

    if ((0 != self->elements) && (self->item_free != 0)) {

        for (size_t i = 0; i < self->next_free; i++) {
            self->item_free(self->elements[i]);

            if (0 != self->elements[i]) {
                ++no_elements_freed;
            }
        }
    }

    ov_log_debug(
        "Cache Calls: Put: %zu (Put elements: %zu)   Get: %zu(Got elements: "
        "%zu)   Next free at exit: %zu  Freed: %zu",
        self->stats.put_called, self->stats.elements_put,
        self->stats.get_called, self->stats.elements_got, self->next_free,
        no_elements_freed);

    if (0 != self->elements) {

        free(self->elements);
        self->elements = 0;
    }

    free(self);
    self = 0;

error:

    return self;
}

/*----------------------------------------------------------------------------*/

static bool free_hashtable_entry(void const *key, void const *value,
                                 void *arg) {

    UNUSED(key);
    UNUSED(arg);

    ov_log_debug("Freeing cache %s", ov_string_sanitize(key));
    cache_free((ov_registered_cache *)value);

    return true;
}

/*----------------------------------------------------------------------------*/

void ov_registered_cache_free_all() {

    if (0 == g_registry)
        return;

    if (!ov_thread_lock_try_lock(&g_lock)) {

        ov_log_warning("Could not lock cache registry");
        OV_ASSERT(!"MUST NEVER HAPPEN");
    }

    size_t caches_freed =
        ov_hashtable_for_each(g_registry, free_hashtable_entry, 0);

    ov_log_debug("Freed %zu caches", caches_freed);

    ov_hashtable_free(g_registry);

    g_registry = 0;

    ov_thread_lock_unlock(&g_lock);
    ov_thread_lock_clear(&g_lock);
}

/*---------------------------------------------------------------------------*/

static bool element_type_correct(ov_registered_cache *restrict self,
                                 void *object) {

    if (0 == self) {
        return false;
    } else if (0 == self->element_checker) {
        return true;
    } else {
        return self->element_checker(object);
    }
}

/*---------------------------------------------------------------------------*/

static void *locked_cache_get(ov_registered_cache *restrict self) {

    if ((0 == self) || (0 == self->elements) || (0 == self->next_free)) {

        return 0;

    } else {

        --self->next_free;
        void *object = self->elements[self->next_free];

        ++self->stats.elements_got;

        OV_ASSERT(element_type_correct(self, object));

        return object;
    }
}

/*----------------------------------------------------------------------------*/

void *ov_registered_cache_get(ov_registered_cache *restrict self) {

    if (0 == self) {

        return 0;

    } else {

        ++self->stats.get_called;

        void *object = 0;

        if (!atomic_flag_test_and_set(&self->in_use)) {

            object = locked_cache_get(self);
            atomic_flag_clear(&self->in_use);
        }

        return object;
    }
}

/*----------------------------------------------------------------------------*/

static void *locked_cache_put(ov_registered_cache *restrict self,
                              void *object) {

    if (0 == object) {

        return 0;

    } else if ((0 == self->elements) || (self->capacity == self->next_free)) {

        return object;

    } else {

        self->elements[self->next_free] = object;
        ++self->next_free;

        ++self->stats.elements_put;

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

void *ov_registered_cache_put(ov_registered_cache *restrict self,
                              void *object) {

    if (0 == self) {

        return object;

    } else {

        ++self->stats.put_called;

        if (!atomic_flag_test_and_set(&self->in_use)) {

            OV_ASSERT(element_type_correct(self, object));

            object = locked_cache_put(self, object);
            atomic_flag_clear(&self->in_use);
        }

        return object;
    }
}

/*----------------------------------------------------------------------------*/

void ov_registered_cache_set_element_checker(ov_registered_cache *self,
                                             bool (*element_checker)(void *)) {

    if (0 == self) {

        return;
    }

    self->element_checker = element_checker;
}

/******************************************************************************
 *                                JSON REPORT
 ******************************************************************************/

struct cache_to_json_arg {
    ov_json_value *target;
};

static bool cache_to_json(void const *key, void const *value, void *void_arg) {

    struct cache_to_json_arg *arg = void_arg;

    if ((0 == arg) || (0 == arg->target))
        goto error;

    if (0 == value)
        goto finish;

    ov_registered_cache *cache = (ov_registered_cache *)value;

    ov_json_value *jcache = ov_json_object();

    if (0 == jcache) {

        ov_log_error("Could not create json object for cache");
        goto error;
    }

    ov_json_object_set(jcache, OV_KEY_CAPACITY,
                       ov_json_number(cache->capacity));

    ov_json_object_set(jcache, OV_KEY_IN_USE, ov_json_number(cache->next_free));

    ov_json_object_set(arg->target, key, jcache);

finish:

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_registered_cache_report(ov_json_value *target) {

    ov_json_value *jval = 0;

    if (0 == g_registry) {

        goto finish;
    }

    if (!ov_thread_lock_try_lock(&g_lock)) {

        ov_log_error("Could not lock cache registry");
        goto error;
    }

    struct cache_to_json_arg arg = {

        .target = target,

    };

    if (0 == target) {

        jval = ov_json_object();
        arg.target = jval;
    }

    size_t num_caches = ov_hashtable_for_each(g_registry, cache_to_json, &arg);

    ov_thread_lock_unlock(&g_lock);

    ov_log_debug("Generated report for %zu caches", num_caches);

finish:

    return arg.target;

error:

    jval = ov_json_value_free(jval);

    return 0;
}

/*----------------------------------------------------------------------------*/

static bool enable_caching_for(ov_registered_cache_sizes *cfg,
                               void (*enable_caching)(size_t),
                               char const *cache_name) {

    if (0 == enable_caching)
        goto error;
    if (0 == cache_name)
        goto error;

    size_t size = ov_registered_cache_size_for(cfg, cache_name);

    if (0 == size) {
        return true;
    }

    enable_caching(size);

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_registered_cache_sizes_configure(ov_registered_cache_sizes *cfg) {

    bool ok = enable_caching_for(cfg, ov_buffer_enable_caching, OV_KEY_BUFFERS);

    ok = ok &
         enable_caching_for(cfg, ov_linked_list_enable_caching, OV_KEY_LISTS);

    ok = ok & enable_caching_for(cfg, ov_rtp_frame_enable_caching,
                                 OV_KEY_RTP_FRAMES);

    return ok;
}

/******************************************************************************
 *                              CACHING DISABLED
 ******************************************************************************/

#else /* OV_DISABLE_CACHING */

struct ov_registered_cache_struct {
    bool dummy : 1;
};

void *ov_registered_cache_get(ov_registered_cache *restrict self) {

    UNUSED(self);
    return 0;
}

/*----------------------------------------------------------------------------*/

void *ov_registered_cache_put(ov_registered_cache *restrict self,
                              void *object) {

    UNUSED(self);
    return object;
}

/*----------------------------------------------------------------------------*/

void ov_registered_cache_set_element_checker(ov_registered_cache *self,
                                             bool (*element_checker)(void *)) {

    UNUSED(self);
    UNUSED(element_checker);
}
/*----------------------------------------------------------------------------*/

ov_registered_cache *
ov_registered_cache_extend(char const *cache_name,
                           ov_registered_cache_config cfg) {

    UNUSED(cache_name);
    UNUSED(cfg);
    return 0;
}

/*----------------------------------------------------------------------------*/

void ov_registered_cache_free_all() { return; }

/*----------------------------------------------------------------------------*/

ov_json_value *ov_registered_cache_report(ov_json *json,
                                          ov_json_value *target) {

    UNUSED(json);
    UNUSED(target);

    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_registered_cache_sizes_configure(ov_registered_cache_sizes *sizes) {
    UNUSED(sizes);
    return true;
}

/*----------------------------------------------------------------------------*/
#endif

/*****************************************************************************
                                 CONFIGURATION
 ****************************************************************************/

struct ov_registered_cache_sizes {

    bool enabled : 1; // Caching enabled ?
    ov_hashtable *sizes;
};

/*----------------------------------------------------------------------------*/

struct add_cache_size_arg {
    bool ok;
    ov_hashtable *tbl;
};

static bool add_cache_size(void const *vkey, void *value, void *varg) {

    struct add_cache_size_arg *arg = varg;
    char const *key = vkey;

    OV_ASSERT(0 != arg);
    OV_ASSERT(0 != key);

    ov_hashtable *sizes = arg->tbl;

    OV_ASSERT(0 != sizes);

    if (!ov_json_is_number(value)) {

        char *cval = ov_json_value_to_string(value);
        ov_log_error("Malformed config: Expected number for key %s, got %s",
                     key, cval == 0 ? "0" : cval);
        free(cval);

        goto error;
    }

    double dvalue = ov_json_number_get(value);

    if (0 > dvalue) {
        ov_log_error("Malformed config: Size for %s negative", key);
        goto error;
    }

    size_t size = dvalue;

    if (0 != ov_hashtable_set(sizes, key, (void *)size)) {
        ov_log_warning("Overwriting old value for %s", key);
    }

    return true;

error:

    arg->ok = false;

    return false;
}

/*----------------------------------------------------------------------------*/

ov_registered_cache_sizes *
ov_registered_cache_sizes_free(ov_registered_cache_sizes *self) {

    if (0 == self)
        goto error;

    self->sizes = ov_hashtable_free(self->sizes);
    OV_ASSERT(0 == self->sizes);

    free(self);
    self = 0;

error:

    return self;
}

/*----------------------------------------------------------------------------*/

ov_registered_cache_sizes *
ov_registered_cache_sizes_from_json(ov_json_value const *jval) {

    ov_registered_cache_sizes *cfg = 0;

    ov_hashtable *sizes = 0;
    bool caching_enabled = true;

    if (0 == jval) {
        ov_log_error("Called with 0 pointer");
        goto error;
    }

    ov_json_value const *caching = ov_json_get(jval, "/" OV_KEY_CACHE_ENABLED);

    if ((0 != caching) && (!ov_json_is_true(caching))) {
        ov_log_info("Caching disabled");
        caching_enabled = false;
        goto finish;
    }

    sizes = ov_hashtable_create_c_string(25);

    jval = ov_json_get(jval, "/" OV_KEY_CACHE_SIZES);

    if (0 == jval) {
        goto finish;
    }

    OV_ASSERT(0 != jval);

    struct add_cache_size_arg arg = {
        .ok = true,
        .tbl = sizes,
    };

    ov_json_object_for_each((ov_json_value *)jval, &arg, add_cache_size);
    arg.tbl = 0;

    if (!arg.ok) {
        goto error;
    }

finish:

    cfg = calloc(1, sizeof(ov_registered_cache_sizes));
    cfg->enabled = caching_enabled;
    cfg->sizes = sizes;
    sizes = 0;

    return cfg;

error:

    sizes = ov_hashtable_free(sizes);
    OV_ASSERT(0 == sizes);

    return 0;
}

/*----------------------------------------------------------------------------*/

size_t ov_registered_cache_size_for(ov_registered_cache_sizes *cfg,
                                    char const *cache_name) {

    size_t size = 0;

    if (0 == cfg) {
        goto error;
    }

    if (0 == cache_name) {
        goto error;
    }

    if (!cfg->enabled) {
        return 0;
    }

    size = OV_DEFAULT_CACHE_SIZE;

    if (ov_hashtable_contains(cfg->sizes, cache_name)) {
        size = (size_t)ov_hashtable_get(cfg->sizes, cache_name);
    }

error:

    return size;
}
