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
        @file           ov_named_broadcast.c
        @author         Markus Töpfer

        @date           2021-02-24


        ------------------------------------------------------------------------
*/
#include "../include/ov_broadcast_registry.h"

#include <ov_base/ov_utils.h>

#include "../include/ov_event_broadcast.h"
#include <ov_base/ov_dict.h>
#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_thread_lock.h>

#define IMPL_THREAD_LOCK_TIMEOUT_USEC 500 * 1000 // 500 micro seconds

struct ov_broadcast_registry {

    ov_event_broadcast_config config;

    ov_dict *dict;
    uint64_t lock_timeout_usec;

    ov_thread_lock lock;
};

/*----------------------------------------------------------------------------*/

static void *void_free_broadcast(void *in) {

    return (void *)ov_event_broadcast_free((ov_event_broadcast *)in);
}

/*----------------------------------------------------------------------------*/

ov_broadcast_registry *
ov_broadcast_registry_create(ov_event_broadcast_config config) {

    if (0 == config.lock_timeout_usec)
        config.lock_timeout_usec = IMPL_THREAD_LOCK_TIMEOUT_USEC;

    if (0 == config.max_sockets)
        config.max_sockets = ov_socket_get_max_supported_runtime_sockets(0);

    ov_broadcast_registry *reg = calloc(1, sizeof(ov_broadcast_registry));
    if (!reg)
        goto error;

    reg->config = config;

    if (!ov_thread_lock_init(&reg->lock, config.lock_timeout_usec))
        goto error;

    ov_dict_config dconfig = ov_dict_string_key_config(255);
    dconfig.value.data_function.free = void_free_broadcast;

    reg->dict = ov_dict_create(dconfig);
    if (!reg->dict)
        goto error;

    return reg;

error:
    ov_broadcast_registry_free(reg);
    return NULL;
}

/*----------------------------------------------------------------------------*/
ov_broadcast_registry *ov_broadcast_registry_free(ov_broadcast_registry *self) {

    if (!self)
        return NULL;

    int i = 0;
    int max = 100;

    for (i = 0; i < max; i++) {

        if (ov_thread_lock_try_lock(&self->lock))
            break;
    }

    if (i == max) {
        OV_ASSERT(1 == 0);
        return self;
    }

    self->dict = ov_dict_free(self->dict);

    /* unlock */
    if (!ov_thread_lock_unlock(&self->lock)) {
        OV_ASSERT(1 == 0);
        return self;
    }

    /* clear lock */
    if (!ov_thread_lock_clear(&self->lock)) {
        OV_ASSERT(1 == 0);
        return self;
    }

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_broadcast_registry_set(ov_broadcast_registry *reg, const char *name,
                               int socket, uint8_t type) {

    bool result = false;

    if (!reg || !name)
        goto error;

    if (!ov_thread_lock_try_lock(&reg->lock))
        goto error;

    ov_event_broadcast *bcast = ov_dict_get(reg->dict, name);
    if (!bcast) {

        bcast = ov_event_broadcast_create(reg->config);
        if (!bcast)
            goto unlock;

        char *key = strdup(name);

        if (!ov_dict_set(reg->dict, key, bcast, NULL)) {
            key = ov_data_pointer_free(key);
            bcast = ov_event_broadcast_free(bcast);
            goto unlock;
        }
    }

    result = ov_event_broadcast_set(bcast, socket, type);

    if (!result || ov_event_broadcast_is_empty(bcast)) {
        ov_dict_del(reg->dict, name);
    }

unlock:

    if (!ov_thread_lock_unlock(&reg->lock)) {
        OV_ASSERT(1 == 0);
        goto error;
    }

    return result;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_broadcast_registry_del(ov_broadcast_registry *reg, const char *name) {

    bool result = false;

    if (!reg || !name)
        goto error;

    if (!ov_thread_lock_try_lock(&reg->lock))
        goto error;

    result = ov_dict_del(reg->dict, name);

    if (!ov_thread_lock_unlock(&reg->lock)) {
        OV_ASSERT(1 == 0);
        goto error;
    }

    return result;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_broadcast_registry_get(ov_broadcast_registry *reg, const char *name,
                               int socket, uint8_t *out) {

    uint8_t result = OV_BROADCAST_UNSET;

    if (!reg || !name || !out)
        goto error;

    if (!ov_thread_lock_try_lock(&reg->lock))
        goto error;

    ov_event_broadcast *bcast = ov_dict_get(reg->dict, name);
    if (bcast) {
        result = ov_event_broadcast_get(bcast, socket);
    }

    if (!ov_thread_lock_unlock(&reg->lock)) {
        OV_ASSERT(1 == 0);
        goto error;
    }

    *out = result;

    return true;
error:
    if (out)
        *out = result;
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_broadcast_registry_send(ov_broadcast_registry *reg, const char *name,
                                const ov_event_parameter *parameter,
                                const ov_json_value *input, uint8_t type) {

    bool result = false;

    if (!reg || !name)
        goto error;

    if (!ov_thread_lock_try_lock(&reg->lock))
        goto error;

    ov_event_broadcast *bcast = ov_dict_get(reg->dict, name);

    if (!bcast)
        goto unlock;

    result = ov_event_broadcast_send_params(bcast, parameter, input, type);

unlock:

    if (!ov_thread_lock_unlock(&reg->lock)) {
        OV_ASSERT(1 == 0);
        goto error;
    }

    return result;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_list *ov_broadcast_registry_get_sockets(ov_broadcast_registry *reg,
                                           const char *name, uint8_t type) {

    if (!reg || !name)
        goto error;

    ov_list *list = NULL;

    if (!ov_thread_lock_try_lock(&reg->lock))
        goto error;

    ov_event_broadcast *bcast = ov_dict_get(reg->dict, name);

    if (!bcast)
        goto unlock;

    list = ov_event_broadcast_get_sockets(bcast, type);

unlock:

    if (!ov_thread_lock_unlock(&reg->lock)) {
        OV_ASSERT(1 == 0);
        goto error;
    }

    return list;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

struct container {

    ov_broadcast_registry *reg;
    int socket;
    ov_list *list;
};

/*----------------------------------------------------------------------------*/

static bool unset_socket(const void *key, void *item, void *data) {

    if (!key)
        return true;

    struct container *container = (struct container *)data;
    ov_event_broadcast *bcast = (ov_event_broadcast *)item;

    if (!ov_event_broadcast_set(bcast, container->socket, OV_BROADCAST_UNSET))
        goto error;

    if (ov_event_broadcast_is_empty(bcast))
        ov_list_push(container->list, (char *)key);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool delete_key(void *item, void *data) {

    if (!item || !data)
        return false;

    ov_broadcast_registry *reg = (ov_broadcast_registry *)data;
    return ov_dict_del(reg->dict, item);
}

/*----------------------------------------------------------------------------*/

bool ov_broadcast_registry_unset(ov_broadcast_registry *reg, int socket) {

    bool result = false;

    if (!reg || socket < 0)
        goto error;

    if (!ov_thread_lock_try_lock(&reg->lock))
        goto error;

    struct container c = {.reg = reg,
                          .socket = socket,
                          .list = ov_linked_list_create((ov_list_config){0})};

    /* Unset the broadcast for the socket */
    result = ov_dict_for_each(reg->dict, &c, unset_socket);

    /* Delete unused (empty) named broadcasts */
    ov_list_for_each(c.list, reg, delete_key);

    c.list = ov_list_free(c.list);

    if (!ov_thread_lock_unlock(&reg->lock)) {
        OV_ASSERT(1 == 0);
        goto error;
    }

error:
    return result;
}

/*----------------------------------------------------------------------------*/

int64_t ov_broadcast_registry_count(ov_broadcast_registry *reg,
                                    const char *name, uint8_t type) {

    int64_t result = -1;

    if (!reg || !name)
        goto error;

    if (!ov_thread_lock_try_lock(&reg->lock))
        goto error;

    ov_event_broadcast *bcast = ov_dict_get(reg->dict, name);
    if (bcast) {
        result = ov_event_broadcast_count(bcast, type);
    } else {
        result = 0;
    }

    if (!ov_thread_lock_unlock(&reg->lock)) {
        OV_ASSERT(1 == 0);
        goto error;
    }

error:
    return result;
}

/*----------------------------------------------------------------------------*/

static bool add_broadcast_state(const void *key, void *item, void *data) {

    if (!key)
        return true;

    ov_json_value *out = ov_json_value_cast(data);
    ov_event_broadcast *bcast = (ov_event_broadcast *)item;

    ov_json_value *val = ov_event_broadcast_state(bcast);
    if (!val)
        goto error;

    if (!ov_json_object_set(out, key, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_broadcast_registry_state(ov_broadcast_registry *reg) {

    ov_json_value *out = NULL;
    bool result = false;

    if (!reg)
        goto error;

    if (!ov_thread_lock_try_lock(&reg->lock))
        goto error;

    out = ov_json_object();
    result = ov_dict_for_each(reg->dict, out, add_broadcast_state);

    if (!ov_thread_lock_unlock(&reg->lock)) {
        OV_ASSERT(1 == 0);
        goto error;
    }

    if (!result)
        goto error;

    return out;

error:
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_broadcast_registry_named_state(ov_broadcast_registry *reg,
                                                 const char *name) {

    ov_json_value *out = NULL;
    bool result = false;

    if (!reg || !name)
        goto error;

    if (!ov_thread_lock_try_lock(&reg->lock))
        goto error;

    out = ov_json_object();

    ov_event_broadcast *bcast = ov_dict_get(reg->dict, name);
    if (!bcast)
        goto unlock;

    ov_json_value *val = ov_event_broadcast_state(bcast);
    if (!val)
        goto unlock;

    if (!ov_json_object_set(out, name, val)) {
        val = ov_json_value_free(val);
        goto unlock;
    }

    result = true;

unlock:

    if (!ov_thread_lock_unlock(&reg->lock)) {
        OV_ASSERT(1 == 0);
        goto error;
    }

    if (!result)
        goto error;

    return out;

error:
    ov_json_value_free(out);
    return NULL;
}
