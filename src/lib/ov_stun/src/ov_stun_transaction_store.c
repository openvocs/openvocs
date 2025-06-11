/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_stun_transaction_store.c
        @author         Markus

        @date           2024-04-24


        ------------------------------------------------------------------------
*/
#include "../include/ov_stun_transaction_store.h"
#include "../include/ov_stun_frame.h"

#include <ov_base/ov_dict.h>
#include <ov_base/ov_list.h>
#include <ov_base/ov_time.h>

#define OV_STUN_TRANSACTION_STORE_MAGIC_BYTES 0xef82

/*----------------------------------------------------------------------------*/

struct TransactionData {

    uint64_t created;
    void *data;
};

/*----------------------------------------------------------------------------*/

struct ov_stun_transaction_store {

    uint16_t magic_bytes;
    ov_stun_transaction_store_config config;

    ov_dict *dict;
    uint32_t timer_invalidate;
};

/*----------------------------------------------------------------------------*/

struct container_timer {

    uint64_t now;
    uint64_t max;
    ov_list *list;
};

/*----------------------------------------------------------------------------*/

static bool create_timeout_key_list(const void *key, void *value, void *data) {

    if (!key) return true;

    if (!value || !data) goto error;

    struct container_timer *container = (struct container_timer *)data;
    struct TransactionData *tdata = (struct TransactionData *)value;

    uint64_t lifetime = tdata->created + container->max;

    if (container->now < lifetime) return true;

    // add to delete list
    return ov_list_push(container->list, (void *)key);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool del_key_list(void *key, void *data) {

    if (!key || !data) goto error;

    ov_dict *dict = ov_dict_cast(data);
    OV_ASSERT(dict);

    return ov_dict_del(dict, key);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool invalidate_transactions(uint32_t timer, void *data) {

    UNUSED(timer);

    ov_stun_transaction_store *self = ov_stun_transaction_store_cast(data);
    if (!self) goto error;

    ov_list *list = ov_list_create((ov_list_config){0});

    struct container_timer container = {

        .now = ov_time_get_current_time_usecs(),
        .max = self->config.timer.invalidation_usec,
        .list = list,
    };

    if (!ov_dict_for_each(self->dict, &container, create_timeout_key_list))
        goto error;

    if (!ov_list_for_each(list, self->dict, del_key_list)) goto error;

    list = ov_list_free(list);

    self->timer_invalidate =
        ov_event_loop_timer_set(self->config.loop,
                                self->config.timer.invalidation_usec,
                                self,
                                invalidate_transactions);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool check_config(ov_stun_transaction_store_config *config) {

    if (!config) goto error;
    if (!config->loop) goto error;

    if (0 == config->timer.invalidation_usec)
        config->timer.invalidation_usec =
            OV_STUN_TRANSACTION_STORE_DEFAULT_INVALIDATION_USEC;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_stun_transaction_store *ov_stun_transaction_store_create(
    ov_stun_transaction_store_config config) {

    ov_stun_transaction_store *self = NULL;
    if (!check_config(&config)) goto error;

    self = calloc(1, sizeof(ov_stun_transaction_store));
    if (!self) goto error;

    self->magic_bytes = OV_STUN_TRANSACTION_STORE_MAGIC_BYTES;
    self->config = config;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = ov_data_pointer_free;

    self->dict = ov_dict_create(d_config);
    if (!self->dict) goto error;

    self->timer_invalidate =
        ov_event_loop_timer_set(config.loop,
                                self->config.timer.invalidation_usec,
                                self,
                                invalidate_transactions);

    return self;
error:
    ov_stun_transaction_store_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_stun_transaction_store *ov_stun_transaction_store_free(
    ov_stun_transaction_store *self) {

    if (!ov_stun_transaction_store_cast(self)) goto error;

    if (OV_TIMER_INVALID != self->timer_invalidate) {

        ov_event_loop_timer_unset(
            self->config.loop, self->timer_invalidate, NULL);
        self->timer_invalidate = OV_TIMER_INVALID;
    }

    self->dict = ov_dict_free(self->dict);
    self = ov_data_pointer_free(self);
error:
    return self;
}

/*----------------------------------------------------------------------------*/

ov_stun_transaction_store *ov_stun_transaction_store_cast(const void *data) {

    if (!data) return NULL;

    if (*(uint16_t *)data != OV_STUN_TRANSACTION_STORE_MAGIC_BYTES) return NULL;

    return (ov_stun_transaction_store *)data;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TRANSACTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_transaction_store_create_transaction(
    ov_stun_transaction_store *self, uint8_t *ptr, void *input) {

    struct TransactionData *data = NULL;
    char *key = NULL;

    if (!self || !ptr) goto error;

    key = calloc(13, sizeof(char));
    if (!key) goto error;
    ov_stun_frame_generate_transaction_id((uint8_t *)key);

    data = calloc(1, sizeof(struct TransactionData));
    if (!data) goto error;

    data->created = ov_time_get_current_time_usecs();
    data->data = input;

    bool result = false;

    result = ov_dict_set(self->dict, key, data, NULL);
    if (!result) goto error;

    memcpy(ptr, key, 12);

    return true;
error:
    ov_data_pointer_free(key);
    ov_data_pointer_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

void *ov_stun_transaction_store_unset(ov_stun_transaction_store *self,
                                      const uint8_t *transaction_id) {

    void *data = NULL;
    char key[13] = {0};

    if (!self || !transaction_id) goto error;

    strncpy(key, (char *)transaction_id, 12);

    struct TransactionData *tdata = ov_dict_remove(self->dict, key);
    if (tdata) data = tdata->data;

    ov_data_pointer_free(tdata);

    return data;
error:
    return NULL;
}
