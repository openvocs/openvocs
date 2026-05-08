/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_event_async.c
        @author         Markus Töpfer

        @date           2021-06-16


        ------------------------------------------------------------------------
*/
#include "../include/ov_event_async.h"

#include <ov_base/ov_utils.h>

#include <ov_base/ov_dict.h>
#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_registered_cache.h>
#include <ov_base/ov_thread_lock.h>
#include <ov_base/ov_time.h>

#define IMPL_THREADLOCK_TIMEOUT_USEC 100 * 1000    // 100ms
#define IMPL_INVALIDATE_TIMEOUT_USEC 1000 * 1000   // 1s
#define IMPL_INVALIDATE_TIMEOUT_MIN_USEC 10 * 1000 // 10ms

static ov_registered_cache *g_async_internal_cache = 0;

struct ov_event_async_store {

    ov_event_async_store_config config;

    ov_thread_lock lock;
    ov_dict *dict;

    uint32_t invalidate_timer;
};

/*----------------------------------------------------------------------------*/

typedef struct {

    uint64_t created_usec;
    uint64_t max_lifetime_usec;

    ov_event_async_data data;

} internal_session_data;

/*----------------------------------------------------------------------------*/

static void *free_internal_session_data(void *self) {

    if (!self)
        return NULL;

    internal_session_data *d = (internal_session_data *)self;
    d->data.value = ov_json_value_free(d->data.value);

    d = ov_registered_cache_put(g_async_internal_cache, d);
    d = ov_data_pointer_free(d);

    return NULL;
}

/*----------------------------------------------------------------------------*/

struct container {

    ov_list *list;
    uint64_t now;
};

/*----------------------------------------------------------------------------*/

static bool delete_keys_in_dict(void *item, void *data) {

    OV_ASSERT(item);
    OV_ASSERT(data);

    ov_dict *dict = ov_dict_cast(data);

    internal_session_data *entry =
        (internal_session_data *)ov_dict_remove(dict, item);

    if (entry && entry->data.timedout.callback) {

        entry->data.timedout.callback(entry->data.timedout.userdata,
                                      entry->data);

        entry->data.value = NULL;
    }

    entry = free_internal_session_data(entry);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool search_timed_out_keys(const void *key, void *item, void *data) {

    if (!key)
        return true;

    OV_ASSERT(item);
    OV_ASSERT(data);
    if (!item || !data)
        goto error;

    struct container *c = (struct container *)data;
    internal_session_data *d = (internal_session_data *)item;

    uint64_t lifetime = d->created_usec + d->max_lifetime_usec;

    if (lifetime < c->now)
        ov_list_push(c->list, (void *)key);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool invalidate_run(uint32_t id, void *data) {

    ov_event_async_store *store = data;
    OV_ASSERT(store);
    OV_ASSERT(id == store->invalidate_timer);

    store->invalidate_timer = OV_TIMER_INVALID;
    ov_event_loop *loop = store->config.loop;

    if (!ov_thread_lock_try_lock(&store->lock))
        goto timer_reenable;

    /* locked */

    struct container container = (struct container){

        .list = ov_linked_list_create((ov_list_config){0}),
        .now = ov_time_get_current_time_usecs()};

    if (!ov_dict_for_each(store->dict, &container, search_timed_out_keys)) {
        ov_log_error("Failed to search timedout items");
        OV_ASSERT(1 == 0);
    }

    if (!ov_list_for_each(container.list, store->dict, delete_keys_in_dict)) {
        ov_log_error("Failed to delete timedout ID items");
        OV_ASSERT(1 == 0);
    }

    container.list = ov_list_free(container.list);

    /* unlock */

    if (!ov_thread_lock_unlock(&store->lock)) {
        ov_log_error("Failed to unlock session store");
        OV_ASSERT(1 == 0);
    }

timer_reenable:

    /* Reenable invalidate run */

    store->invalidate_timer =
        loop->timer.set(loop, store->config.invalidate_check_interval_usec,
                        store, invalidate_run);

    if (OV_TIMER_INVALID == store->invalidate_timer)
        goto error;

    return true;
error:
    ov_log_error("Session store invalidate run failed");
    OV_ASSERT(1 == 0);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_event_async_store *
ov_event_async_store_create(ov_event_async_store_config config) {

    ov_event_async_store *store = NULL;

    if (!config.loop)
        goto error;

    if (0 == config.threadlock_timeout_usec)
        config.threadlock_timeout_usec = IMPL_THREADLOCK_TIMEOUT_USEC;

    if (0 == config.invalidate_check_interval_usec)
        config.invalidate_check_interval_usec = IMPL_INVALIDATE_TIMEOUT_USEC;

    if (IMPL_INVALIDATE_TIMEOUT_MIN_USEC >
        config.invalidate_check_interval_usec)
        config.invalidate_check_interval_usec =
            IMPL_INVALIDATE_TIMEOUT_MIN_USEC;

    store = calloc(1, sizeof(ov_event_async_store));
    if (!store)
        goto error;

    if (!ov_thread_lock_init(&store->lock, config.threadlock_timeout_usec))
        goto error;

    store->config = config;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = free_internal_session_data;

    store->dict = ov_dict_create(d_config);
    if (!store->dict)
        goto error;

    ov_event_loop *loop = store->config.loop;

    store->invalidate_timer =
        loop->timer.set(loop, store->config.invalidate_check_interval_usec,
                        store, invalidate_run);

    if (OV_TIMER_INVALID == store->invalidate_timer)
        goto error;

    if (0 != config.cache) {

        ov_registered_cache_config cfg = {

            .capacity = config.cache,
            .item_free = ov_data_pointer_free,

        };

        g_async_internal_cache =
            ov_registered_cache_extend("ov_event_async_internal", cfg);
    }

    return store;

error:
    ov_event_async_store_free(store);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_event_async_store *ov_event_async_store_free(ov_event_async_store *self) {

    if (!self)
        return NULL;

    int i = 0;
    int max = 100;

    for (i = 0; i < max; i++) {

        if (ov_thread_lock_try_lock(&self->lock))
            break;
    }

    if (i >= max) {
        ov_log_error("Failed to lock store for delete");
        return self;
    }

    OV_ASSERT(i < max);

    /* locked */

    if (OV_TIMER_INVALID != self->invalidate_timer) {

        ov_event_loop *loop = self->config.loop;
        loop->timer.unset(loop, self->invalidate_timer, NULL);
        self->invalidate_timer = OV_TIMER_INVALID;
    }

    self->dict = ov_dict_free(self->dict);

    if (!ov_thread_lock_unlock(&self->lock)) {
        ov_log_error("Failed to unlock for delete");
    } else {
        ov_thread_lock_clear(&self->lock);
    }

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_event_async_set(ov_event_async_store *store, const char *id,
                        ov_event_async_data data, uint64_t max_lifetime_usec) {

    internal_session_data *internal = NULL;
    char *key = NULL;
    bool result = false;

    if (!store || !id)
        goto error;

    key = strdup(id);

    internal = ov_registered_cache_get(g_async_internal_cache);
    if (!internal)
        internal = calloc(1, sizeof(internal_session_data));

    if (!internal)
        goto error;

    internal->created_usec = ov_time_get_current_time_usecs();
    internal->max_lifetime_usec = max_lifetime_usec;

    if (!ov_thread_lock_try_lock(&store->lock))
        goto error;

    /* locked */

    if (!ov_dict_set(store->dict, key, internal, NULL)) {
        internal = free_internal_session_data(internal);
        key = ov_data_pointer_free(key);
        goto unlock;
    }

    /* We put the internal data to the dict,
     * so everything is contained in the store,
     * but the data to store.
     * We finaly add the data here before unlock */

    internal->data = data;
    result = true;

unlock:
    if (!ov_thread_lock_unlock(&store->lock)) {
        ov_log_error("Failed to unlock");
        OV_ASSERT(1 == 0);
    }

    return result;
error:
    free_internal_session_data(internal);
    key = ov_data_pointer_free(key);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_event_async_data ov_event_async_unset(ov_event_async_store *store,
                                         const char *id) {

    ov_event_async_data data = (ov_event_async_data){0};

    if (!store || !id)
        goto error;

    if (!ov_thread_lock_try_lock(&store->lock))
        goto error;

    internal_session_data *internal =
        (internal_session_data *)ov_dict_remove(store->dict, id);

    if (internal) {

        data = internal->data;
        internal->data = (ov_event_async_data){0};
        internal = free_internal_session_data(internal);
    }

    if (!ov_thread_lock_unlock(&store->lock)) {
        ov_log_error("Failed to unlock");
        OV_ASSERT(1 == 0);
    }

    return data;
error:
    return (ov_event_async_data){0};
}

/*----------------------------------------------------------------------------*/

struct container_socket {

    int socket;
    ov_list *list;
};

/*----------------------------------------------------------------------------*/

static bool search_ids_of_socket(const void *key, void *val, void *data) {

    if (!key)
        return true;

    struct container_socket *c = (struct container_socket *)data;
    internal_session_data *d = (internal_session_data *)val;

    if (c->socket == d->data.socket)
        return ov_list_push(c->list, (void *)key);

    return true;
}

/*----------------------------------------------------------------------------*/

static bool drop_entry(void *val, void *data) {

    ov_event_async_store *store = (ov_event_async_store *)data;
    return ov_dict_del(store->dict, val);
}

/*----------------------------------------------------------------------------*/

bool ov_event_async_drop(ov_event_async_store *store, int socket) {

    ov_list *list = NULL;
    bool result = false;

    if (!store)
        goto error;

    list = ov_linked_list_create((ov_list_config){0});

    if (!ov_thread_lock_try_lock(&store->lock))
        goto error;

    struct container_socket c =
        (struct container_socket){.socket = socket, .list = list};

    result = ov_dict_for_each(store->dict, &c, search_ids_of_socket);

    if (!result)
        goto done;

    result &= ov_list_for_each(list, store, drop_entry);

done:
    if (!ov_thread_lock_unlock(&store->lock)) {
        ov_log_error("Failed to unlock");
        OV_ASSERT(1 == 0);
    }

    ov_list_free(list);
    return result;
error:
    ov_list_free(list);
    return false;
}

/*----------------------------------------------------------------------------*/

void ov_event_async_data_clear(ov_event_async_data *data) {

    if (!data)
        goto done;

    data->value = ov_json_value_free(data->value);
    *data = (ov_event_async_data){0};
done:
    return;
}
