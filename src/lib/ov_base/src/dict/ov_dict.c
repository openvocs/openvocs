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
        @file           ov_dict.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-08-15

        @ingroup        ov_basics

        @brief          Implementation of a standard KEY/VALUE store.

        The dict consists of a hash table with SLOTS entries.
        Each slot contains a linked list with key,value pairs.
        The linked list ALWAYS starts off with a dummy pair (0, 0).
        This pair must be ignored when doing anything with the dict.


        ------------------------------------------------------------------------
*/
#include "../../include/ov_dict.h"
#include "../../include/ov_utils.h"

/*
 *      ------------------------------------------------------------------------
 *
 *                        INTERNAL DATA STRUCTURES
 *
 *      ------------------------------------------------------------------------
 */

const uint16_t IMPL_DEFAULT_DICT_TYPE = 0xaffe;
const uint64_t IMPL_DEFAULT_SLOTS = 100;

typedef struct dict_pair dict_pair;

struct dict_pair {

    void *key;
    void *value;
    dict_pair *next;
};

/*---------------------------------------------------------------------------*/

typedef struct {

    ov_dict public;
    dict_pair *items;

} DefaultDict;

#define AS_DEFAULT_DICT(x)                                                     \
    (((ov_dict_cast(x) != 0) &&                                                \
      (IMPL_DEFAULT_DICT_TYPE == ((ov_dict *)x)->type))                        \
         ? (DefaultDict *)(x)                                                  \
         : 0)

/*
 *      ------------------------------------------------------------------------
 *
 *                        PROTOTYPE DEFINITION
 *
 *      ------------------------------------------------------------------------
 */

static bool impl_dict_is_empty(const ov_dict *self);
static bool impl_dict_clear(ov_dict *self);
static ov_dict *impl_dict_free(ov_dict *self);

static ov_list *impl_dict_get_keys(const ov_dict *self, const void *val);
static void *impl_dict_get(const ov_dict *self, const void *key);
static bool impl_dict_set(ov_dict *self,
                          void *key,
                          void *value,
                          void **replaced);
static bool impl_dict_del(ov_dict *self, const void *key);
static void *impl_dict_remove(ov_dict *self, const void *key);

static bool impl_dict_for_each(ov_dict *self,
                               void *data,
                               bool (*function)(const void *key,
                                                void *value,
                                                void *data));

/*---------------------------------------------------------------------------*/

static bool dict_calculate_slot(const ov_dict *self,
                                const void *key,
                                size_t *slot) {

    if (!self || !slot) return false;

    if (!ov_dict_config_is_valid(&self->config)) return false;

    *slot = (self->config.key.hash(key) % self->config.slots);
    return true;
}

/*---------------------------------------------------------------------------*/

static dict_pair *get_start_pair_at_slot(const ov_dict *self, size_t slot) {

    if (!self) goto error;

    DefaultDict *d = AS_DEFAULT_DICT(self);
    if (!d || !d->items) goto error;

    dict_pair *pair = (dict_pair *)&d->items[slot];
    return pair;

error:
    return NULL;
}

/*
 *      ------------------------------------------------------------------------
 *
 *                        DEFAULT IMPLEMENTATION
 *
 *      ------------------------------------------------------------------------
 */

ov_dict *ov_dict_cast(const void *data) {

    if (!data) return NULL;

    if (*(uint16_t *)data != OV_DICT_MAGIC_BYTE) return NULL;

    return (ov_dict *)data;
}

/*---------------------------------------------------------------------------*/

ov_dict *ov_dict_set_magic_bytes(ov_dict *dict) {

    if (!dict) return NULL;

    dict->magic_byte = OV_DICT_MAGIC_BYTE;
    return dict;
}

/*---------------------------------------------------------------------------*/

bool ov_dict_config_is_valid(const ov_dict_config *config) {

    if (!config || (config->slots < 1) || !config->key.hash ||
        !config->key.match)
        return false;

    return true;
}

/*---------------------------------------------------------------------------*/

bool ov_dict_is_valid(const ov_dict *dict) {

    if (!ov_dict_cast(dict)) return false;

    if (!ov_dict_config_is_valid(&dict->config)) return false;

    if (!dict->is_empty || !dict->create || !dict->clear || !dict->free ||
        !dict->get || !dict->set || !dict->del || !dict->remove ||
        !dict->for_each)
        return false;

    return true;
}

/*---------------------------------------------------------------------------*/

ov_dict *ov_dict_create(ov_dict_config config) {

    ov_dict *dict = NULL;

    if (!ov_dict_config_is_valid(&config)) goto error;

    dict = calloc(1, sizeof(DefaultDict));
    if (!dict) goto error;

    if (!ov_dict_set_magic_bytes(dict)) goto error;

    // set default data
    dict->type = IMPL_DEFAULT_DICT_TYPE;
    dict->config = config;

    // set default functions
    dict->is_empty = impl_dict_is_empty;
    dict->create = ov_dict_create;
    dict->clear = impl_dict_clear;
    dict->free = impl_dict_free;
    dict->get_keys = impl_dict_get_keys;
    dict->get = impl_dict_get;
    dict->set = impl_dict_set;
    dict->del = impl_dict_del;
    dict->remove = impl_dict_remove;
    dict->for_each = impl_dict_for_each;

    DefaultDict *d = AS_DEFAULT_DICT(dict);
    if (!d) goto error;

    // set custom data
    d->items = calloc(dict->config.slots + 1, sizeof(dict_pair));
    if (!d->items) goto error;

    return dict;
error:
    return impl_dict_free(dict);
}

/*---------------------------------------------------------------------------*/

bool ov_dict_is_set(const ov_dict *self, const void *key) {

    if (!self) goto error;

    size_t slot = 0;

    if (!dict_calculate_slot(self, key, &slot)) goto error;

    dict_pair *pair = get_start_pair_at_slot(self, slot);

    OV_ASSERT(0 != pair);

    while (pair->next) {

        pair = pair->next;

        if (self->config.key.match(key, pair->key)) return true;
    }
error:
    return false;
}

/*---------------------------------------------------------------------------*/

static bool count_keys(const void *key, void *value, void *data) {

    if (!key) return true;

    if (!data) goto error;

    intptr_t *ptr = (intptr_t *)data;
    if (*ptr < 0) goto error;

    *ptr = *ptr + 1;
    return true;

error:
    if (value) { /* unused */
    };
    return false;
}

/*---------------------------------------------------------------------------*/

int64_t ov_dict_count(const ov_dict *self) {

    if (!self) goto error;

    intptr_t counter = 0;

    if (!self->for_each((ov_dict *)self, &counter, count_keys)) goto error;

    return counter;

error:
    return -1;
}

/*---------------------------------------------------------------------------*/

static bool key_is_empty(const void *key, void *value, void *data) {

    if (!key) return true;

    if (value || data) { /* unused */
    };
    return false;
}

/*---------------------------------------------------------------------------*/

static bool impl_dict_is_empty(const ov_dict *self) {

    if (!self) goto error;

    if (!self->for_each((ov_dict *)self, NULL, key_is_empty)) goto error;

    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

static bool impl_dict_clear(ov_dict *self) {

    if (!self) goto error;

    DefaultDict *d = AS_DEFAULT_DICT(self);
    if (!d || !d->items) goto error;

    // walk all slots
    for (size_t i = 0; i < self->config.slots; i++) {

        dict_pair *start = (dict_pair *)&d->items[i];
        dict_pair *pair = NULL;
        dict_pair *next = start;

        // walk all overflow lists
        while (next) {

            pair = next;
            next = pair->next;

            if (self->config.key.data_function.free)
                self->config.key.data_function.free(pair->key);

            if (self->config.value.data_function.free)
                self->config.value.data_function.free(pair->value);

            if (pair == start) {

                pair->key = NULL;
                pair->value = NULL;
                pair->next = NULL;

            } else {

                free(pair);
            }
        }
    }

    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

static ov_dict *impl_dict_free(ov_dict *self) {

    if (!self) goto error;

    if (!self->clear(self)) return self;

    DefaultDict *d = AS_DEFAULT_DICT(self);
    if (!d) return self;

    free(d->items);
    free(self);
    return NULL;
error:
    return self;
}

/*---------------------------------------------------------------------------*/

struct data_container {

    const void *value;
    ov_list *list;
};

/*---------------------------------------------------------------------------*/

static bool check_value_at_key(const void *key, void *value, void *data) {

    if (!key || !value) return true;

    if (!data) return false;

    struct data_container *container = data;
    if (!ov_list_cast(container->list)) return false;

    if (value == container->value)
        if (!container->list->push(container->list, (void *)key)) return false;

    return true;
}

/*----------------------------------------------------------------------------*/

static bool key_collector(const void *key, void *value, void *data) {

    if (!key || !value) return true;

    if (!data) return false;

    struct data_container *container = data;
    if (!ov_list_cast(container->list)) return false;

    if (!container->list->push(container->list, (void *)key)) return false;

    return true;
}

/*---------------------------------------------------------------------------*/

static ov_list *impl_dict_get_keys(const ov_dict *self, const void *val) {

    ov_list *keys = NULL;

    if (!self) goto error;

    keys = ov_list_create((ov_list_config){0});

    struct data_container container = {
        .value = val,
        .list = keys,

    };

    bool (*collector)(const void *key, void *value, void *data) =
        check_value_at_key;

    if (0 == val) {

        collector = key_collector;
    }

    if (!self->for_each((ov_dict *)self, &container, collector)) goto error;

    return keys;
error:
    if (keys) ov_list_free(keys);
    return NULL;
}

/*---------------------------------------------------------------------------*/

static void *impl_dict_get(const ov_dict *self, const void *key) {

    if (!self) goto error;

    size_t slot = 0;

    if (!dict_calculate_slot(self, key, &slot)) goto error;

    dict_pair *pair = get_start_pair_at_slot(self, slot);

    OV_ASSERT(0 != pair);

    while (pair->next) {

        pair = pair->next;

        if (self->config.key.match(key, pair->key)) return pair->value;
    }
error:
    return NULL;
}

/*---------------------------------------------------------------------------*/

static bool dict_pair_replace_value(ov_dict *self,
                                    dict_pair *pair,
                                    void *value,
                                    void **replaced) {

    if (!self || !pair) return false;

    if (replaced) {

        *replaced = pair->value;

    } else {

        if (self->config.value.data_function.free)
            self->config.value.data_function.free(pair->value);
    }

    pair->value = value;
    return true;
}

/*---------------------------------------------------------------------------*/

static bool dict_pair_push_value(dict_pair *pair, void *key, void *value) {

    if (!pair) return false;

    // ensure to be at the end of the list
    while (pair->next) {

        pair = pair->next;
    }

    pair->next = calloc(1, sizeof(dict_pair));
    pair->next->key = key;
    pair->next->value = value;

    return true;
}

/*---------------------------------------------------------------------------*/

static bool impl_dict_set(ov_dict *self,
                          void *key,
                          void *value,
                          void **replaced) {

    if (!self) goto error;

    size_t slot = 0;

    if (self->config.key.validate_input)
        if (!self->config.key.validate_input(key)) return false;

    if (self->config.value.validate_input)
        if (!self->config.value.validate_input(value)) return false;

    if (!dict_calculate_slot(self, key, &slot)) goto error;

    dict_pair *pair = get_start_pair_at_slot(self, slot);

    OV_ASSERT(0 != pair);

    while (pair->next) {

        pair = pair->next;

        if (self->config.key.match(key, pair->key)) {

            // delete old key value
            if (self->config.key.data_function.free)
                self->config.key.data_function.free(pair->key);

            pair->key = key;

            return dict_pair_replace_value(self, pair, value, replaced);
        }
    }

    return dict_pair_push_value(pair, key, value);

error:
    return false;
}

/*---------------------------------------------------------------------------*/

static bool impl_dict_del(ov_dict *self, const void *key) {

    if (!self) goto error;

    void *value = impl_dict_remove(self, key);
    if (value)
        if (self->config.value.data_function.free)
            self->config.value.data_function.free(value);

    return true;

error:
    return false;
}

/*---------------------------------------------------------------------------*/

static void *impl_dict_remove(ov_dict *self, const void *key) {

    if (!self) goto error;

    size_t slot = 0;

    if (!dict_calculate_slot(self, key, &slot)) goto error;

    void *value = NULL;

    dict_pair *pair = get_start_pair_at_slot(self, slot);
    dict_pair *drop = NULL;

    OV_ASSERT(0 != pair);

    // item somewhere in overflow list ?

    while (pair->next) {

        if (self->config.key.match(key, pair->next->key)) {

            drop = pair->next;
            value = drop->value;

            drop->value = NULL;
            if (self->config.key.data_function.free)
                self->config.key.data_function.free(drop->key);

            pair->next = drop->next;
            free(drop);
            break;
        }

        pair = pair->next;
    }

    return value;
error:
    return NULL;
}

/*---------------------------------------------------------------------------*/

static bool impl_dict_for_each(ov_dict *self,
                               void *data,
                               bool (*function)(const void *key,
                                                void *value,
                                                void *data)) {

    if (!self || !function) goto error;

    DefaultDict *d = AS_DEFAULT_DICT(self);
    if (!d || !d->items) goto error;

    // walk all slots
    for (size_t i = 0; i < self->config.slots; i++) {

        dict_pair *pair = (dict_pair *)&d->items[i];

        OV_ASSERT(0 != pair);

        // walk all overflow lists
        while (pair->next) {

            pair = pair->next;

            if (NULL == pair) continue;

            if (!function(pair->key, pair->value, data)) goto error;
        }
    }

    return true;

error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *                        DEFAULT CONFIGURATIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_dict_config ov_dict_string_key_config(size_t slots) {

    if (slots == 0) slots = IMPL_DEFAULT_SLOTS;

    return (ov_dict_config){

        .slots = slots,
        .key.data_function = ov_data_string_data_functions(),
        .key.hash = ov_hash_pearson_c_string,
        .key.match = ov_match_c_string_strict,

    };
}

/*----------------------------------------------------------------------------*/

static bool intptr_dump(FILE *stream, const void *pointer) {

    if (!stream) return false;

    fprintf(stream, "%" PRIiPTR, (intptr_t)pointer);
    return true;
}

/*----------------------------------------------------------------------------*/

ov_dict_config ov_dict_intptr_key_config(size_t slots) {

    if (slots == 0) slots = IMPL_DEFAULT_SLOTS;

    return (ov_dict_config){

        .slots = slots,
        .key.data_function.copy = NULL,
        .key.data_function.dump = intptr_dump,
        .key.hash = ov_hash_intptr,
        .key.match = ov_match_intptr,

    };
}

/*---------------------------------------------------------------------------*/

ov_dict_config ov_dict_uint64_key_config(size_t slots) {

    if (slots == 0) slots = IMPL_DEFAULT_SLOTS;

    return (ov_dict_config){

        .slots = slots,
        .key.data_function = ov_data_uint64_data_functions(),
        .key.hash = ov_hash_uint64,
        .key.match = ov_match_uint64,

    };
}

/*
 *      ------------------------------------------------------------------------
 *
 *                        DATA FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_dict_clear(void *data) {

    ov_dict *dict = ov_dict_cast(data);
    if (!dict) return false;

    return dict->clear(dict);
}

/*---------------------------------------------------------------------------*/

void *ov_dict_free(void *data) {

    ov_dict *dict = ov_dict_cast(data);
    if (!dict) return dict;

    return dict->free(dict);
}

/*---------------------------------------------------------------------------*/

static bool copy_dict_content(const void *key, void *value, void *data) {

    ov_dict *dict = ov_dict_cast(data);
    if (!dict) goto error;

    // skip empty
    if (!key) return true;

    if (!dict->config.key.data_function.dump) goto error;

    if (!dict->config.value.data_function.copy) goto error;

    void *k = NULL;
    void *v = NULL;

    if (!dict->config.key.data_function.copy(&k, key)) goto error;

    if (value)
        if (!dict->config.value.data_function.copy(&v, value)) goto error;

    if (!dict->set(dict, k, v, NULL)) goto error;

    return true;

error:
    // ensure memory cleanup if possible
    if (dict) {

        if (k)
            if (dict->config.key.data_function.free)
                dict->config.key.data_function.free(k);

        if (v)
            if (dict->config.value.data_function.free)
                dict->config.value.data_function.free(v);
    }

    return false;
}

/*---------------------------------------------------------------------------*/

void *ov_dict_copy(void **destination, const void *data) {

    bool created = false;
    ov_dict *copy = NULL;
    ov_dict *dict = ov_dict_cast(data);
    if (!dict || !destination) goto error;

    if (!ov_dict_is_valid(dict)) return false;

    if (!dict->config.key.data_function.copy) goto error;

    if (!dict->config.value.data_function.copy) goto error;

    if (!*destination) {

        *destination = dict->create(dict->config);
        if (!*destination) goto error;
        created = true;
    }

    copy = ov_dict_cast(*destination);
    if (!copy) goto error;

    if (!copy->clear(copy)) goto error;

    // set key and data config
    copy->config.key = dict->config.key;
    copy->config.value = dict->config.value;

    // @NOTE slots MAY differ on purpose (if *dest was preset with size)

    if (!dict->for_each(dict, copy, copy_dict_content)) goto error;

    return copy;

error:
    if (created) {
        ov_dict_free(*destination);
        *destination = NULL;
    }

    return NULL;
}

/*---------------------------------------------------------------------------*/

struct dump_data {

    FILE *stream;
    ov_dict *dict;
};

/*---------------------------------------------------------------------------*/

static bool dump_dict_content(const void *key, void *value, void *data) {

    // skip empty
    if (!key) return true;

    if (!data) goto error;

    struct dump_data *d = data;

    if (!d->stream || !d->dict) goto error;

    if (!d->dict->config.key.data_function.dump) goto error;

    if (!fprintf(d->stream, "\nkey:\t")) goto error;

    if (!d->dict->config.key.data_function.dump(d->stream, key)) goto error;

    if (!fprintf(d->stream, "val:\t")) goto error;

    if (!value) {

        if (!fprintf(d->stream, "(null)\n")) goto error;

    } else if (d->dict->config.value.data_function.dump) {

        if (!d->dict->config.value.data_function.dump(d->stream, value))
            goto error;

        if (!fprintf(d->stream, "\n")) goto error;

    } else if (!fprintf(d->stream, "%p\n", value)) {

        goto error;
    }

    return true;

error:
    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_dict_dump(FILE *stream, const void *data) {

    if (!stream || !data) return false;

    ov_dict *dict = ov_dict_cast(data);
    if (!dict) return false;

    if (!ov_dict_is_valid(dict)) return false;

    size_t count = ov_dict_count(dict);

    if (!fprintf(stream, "\nDICT %p with %zu entries\n", dict, count))
        goto error;

    struct dump_data d = {.stream = stream, .dict = dict};

    if (dict->config.key.data_function.dump)
        return dict->for_each(dict, &d, dump_dict_content);

    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

ov_data_function ov_dict_data_functions() {

    ov_data_function func = {

        .clear = ov_dict_clear,
        .free = ov_dict_free,
        .copy = ov_dict_copy,
        .dump = ov_dict_dump};

    return func;
}

/*
 *      ------------------------------------------------------------------------
 *
 *                        FUNCTIONS TO INTERNAL POINTERS
 *
 *
 *      ... following functions check if the dict has the respective function
 * and execute the linked function.
 *      ------------------------------------------------------------------------
 */

bool ov_dict_is_empty(const ov_dict *dict) {

    if (!dict || !dict->is_empty) return false;

    return dict->is_empty(dict);
}

/*---------------------------------------------------------------------------*/

ov_list *ov_dict_get_keys(const ov_dict *dict, const void *value) {

    if (!dict || !dict->get_keys) return NULL;

    return dict->get_keys(dict, value);
}

/*---------------------------------------------------------------------------*/

void *ov_dict_get(const ov_dict *dict, const void *key) {

    if (!dict || !dict->get) return NULL;

    return dict->get(dict, key);
}

/*---------------------------------------------------------------------------*/

bool ov_dict_set(ov_dict *dict, void *key, void *value, void **replaced) {

    if (!dict || !dict->set) return false;

    return dict->set(dict, key, value, replaced);
}

/*---------------------------------------------------------------------------*/

bool ov_dict_del(ov_dict *dict, const void *key) {

    if (!dict || !dict->del) return false;

    return dict->del(dict, key);
}

/*---------------------------------------------------------------------------*/

void *ov_dict_remove(ov_dict *dict, const void *key) {

    if (!dict || !dict->remove) return NULL;

    return dict->remove(dict, key);
}

/*---------------------------------------------------------------------------*/

bool ov_dict_for_each(ov_dict *dict,
                      void *data,
                      bool (*function)(const void *key,
                                       void *value,
                                       void *data)) {

    if (!dict || !dict->for_each || !function) return false;

    return dict->for_each(dict, data, function);
}

/*---------------------------------------------------------------------------*/

static ov_list *dict_load_factor(const ov_dict *dict) {

    DefaultDict *d = AS_DEFAULT_DICT(dict);
    if (!d || !d->items) goto error;

    ov_list *list = ov_list_create((ov_list_config){0});

    // walk all slots
    for (size_t i = 0; i < dict->config.slots; i++) {

        dict_pair *start = (dict_pair *)&d->items[i];
        dict_pair *pair = NULL;
        dict_pair *next = start;

        intptr_t counter = 0;

        // walk all overflow lists
        while (next) {

            pair = next;
            next = pair->next;

            if (pair->key) counter++;
        }

        ov_list_push(list, (void *)counter);
    }

    return list;
error:
    return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_dict_dump_load_factor(FILE *stream, const ov_dict *dict) {

    if (!stream || !dict) goto error;

    ov_list *list = dict_load_factor(dict);
    if (!list) goto error;

    uint64_t counter = ov_list_count(list);
    fprintf(stream, "DICT with %" PRIu64 " items LOAD ---> \n", counter);

    uint64_t items_count = 0;
    for (uint64_t i = 0; i < counter; i++) {

        intptr_t items = (intptr_t)ov_list_get(list, i + 1);
        items_count += items;
        if (0 == items) continue;

        fprintf(stream, "%" PRIu64 " %" PRIiPTR "\n", i, items);
    }
    list = ov_list_free(list);

    fprintf(stream,
            "\n----------------------\n"
            "items %" PRIu64 " average per slot %" PRIu64 "\n",
            items_count,
            items_count / counter);

    return true;
error:
    return false;
}
