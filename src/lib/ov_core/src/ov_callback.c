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
        @file           ov_callback.c
        @author         Markus Töpfer

        @date           2023-03-29


        ------------------------------------------------------------------------
*/
#include "../include/ov_callback.h"

#define OV_CALLBACK_REGISTRY_MAGIC_BYTES 0x734d

#include <ov_base/ov_data_function.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_registered_cache.h>
#include <ov_base/ov_time.h>

ov_registered_cache *g_ov_callback_data_cache = 0;

/*----------------------------------------------------------------------------*/

struct ov_callback_registry {

    uint16_t magic_bytes;
    ov_callback_registry_config config;

    ov_dict *data;

    uint32_t timer_id;
};

/*----------------------------------------------------------------------------*/

typedef struct ov_callback_data {

    uint64_t created_usec;
    uint64_t timeout_usec;

    ov_callback cb;

} ov_callback_data;

/*----------------------------------------------------------------------------*/

void *ov_callback_data_cache(void *self) {

    if (!self) return NULL;

    ov_callback_data *data = (ov_callback_data *)self;
    data->created_usec = 0;
    data->timeout_usec = 0;
    data->cb.userdata = NULL;
    data->cb.function = NULL;

    self = ov_registered_cache_put(g_ov_callback_data_cache, self);
    return ov_data_pointer_free(self);
}

/*----------------------------------------------------------------------------*/

struct container {

    ov_list *list;
    uint64_t now;
};

/*----------------------------------------------------------------------------*/

static bool search_timedout(const void *key, void *val, void *data) {

    if (!key) return true;
    ov_callback_data *d = (ov_callback_data *)val;
    struct container *c = (struct container *)data;

    if (c->now > d->created_usec + d->timeout_usec)
        ov_list_push(c->list, (void *)key);

    return true;
}

/*----------------------------------------------------------------------------*/

static bool delete_timedout(void *val, void *data) {

    ov_dict *dict = ov_dict_cast(data);
    return ov_dict_del(dict, val);
}

/*----------------------------------------------------------------------------*/

static bool cb_check_timeout(uint32_t id, void *data) {

    UNUSED(id);
    ov_callback_registry *self = ov_callback_registry_cast(data);
    if (!self) goto error;

    self->timer_id = self->config.loop->timer.set(
        self->config.loop, self->config.timeout_usec, self, cb_check_timeout);

    ov_list *timedout = ov_linked_list_create((ov_list_config){0});
    if (!timedout) goto error;

    struct container c = (struct container){

        .list = timedout, .now = ov_time_get_current_time_usecs()

    };

    ov_dict_for_each(self->data, &c, search_timedout);
    ov_list_for_each(timedout, self->data, delete_timedout);

    timedout = ov_list_free(timedout);
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_callback_registry *ov_callback_registry_create(
    ov_callback_registry_config config) {

    ov_callback_registry *registry = NULL;

    if (!config.loop) goto error;
    if (0 == config.timeout_usec)
        config.timeout_usec = OV_CALLBACK_TIMEOUT_DEFAULT_USEC;

    if (0 == config.cache_size)
        config.cache_size = OV_CALLBACK_DEFAULT_CACHE_SIZE;

    registry = calloc(1, sizeof(ov_callback_registry));
    if (!registry) goto error;

    registry->magic_bytes = OV_CALLBACK_REGISTRY_MAGIC_BYTES;
    registry->config = config;

    registry->timer_id = config.loop->timer.set(
        config.loop, config.timeout_usec, registry, cb_check_timeout);

    if (OV_TIMER_INVALID == registry->timer_id) goto error;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = ov_callback_data_cache;

    registry->data = ov_dict_create(d_config);
    if (!registry->data) goto error;

    ov_registered_cache_config cfg = {

        .capacity = config.cache_size,
        .item_free = ov_data_pointer_free,

    };

    g_ov_callback_data_cache =
        ov_registered_cache_extend("ov_callback_data_registry", cfg);

    return registry;
error:
    ov_callback_registry_free(registry);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_callback_registry *ov_callback_registry_free(ov_callback_registry *self) {

    if (!ov_callback_registry_cast(self)) return self;

    self->data = ov_dict_free(self->data);
    if (OV_TIMER_INVALID != self->timer_id) {

        self->config.loop->timer.unset(self->config.loop, self->timer_id, NULL);

        self->timer_id = OV_TIMER_INVALID;
    }

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_callback_registry *ov_callback_registry_cast(const void *self) {

    if (!self) goto error;

    if (*(uint16_t *)self == OV_CALLBACK_REGISTRY_MAGIC_BYTES)
        return (ov_callback_registry *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_callback_registry_register(ov_callback_registry *self,
                                   const char *key,
                                   const ov_callback callback,
                                   uint64_t timeout) {

    if (!self || !key || (0 == timeout)) goto error;

    ov_callback_data *data = ov_registered_cache_get(g_ov_callback_data_cache);

    if (!data) data = calloc(1, sizeof(ov_callback_data));

    if (!data) goto error;

    data->created_usec = ov_time_get_current_time_usecs();
    data->timeout_usec = timeout;
    data->cb = callback;

    char *k = strdup(key);

    if (!ov_dict_set(self->data, k, data, NULL)) {
        k = ov_data_pointer_free(k);
        data = ov_data_pointer_free(data);
        goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_callback ov_callback_registry_unregister(ov_callback_registry *self,
                                            const char *key) {

    ov_callback cb = {0};

    if (!self || !key) goto error;

    ov_callback_data *data = ov_dict_remove(self->data, key);
    if (!data) goto error;

    cb = data->cb;
    data = ov_callback_data_cache(data);

error:
    return cb;
}

/*----------------------------------------------------------------------------*/

ov_callback ov_callback_registry_get(ov_callback_registry *self,
                                     const char *key) {

    ov_callback cb = {0};

    if (!self || !key) goto error;

    ov_callback_data *data = ov_dict_get(self->data, key);
    if (!data) goto error;

    return data->cb;

error:
    return cb;
}
