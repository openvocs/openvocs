/***
        ------------------------------------------------------------------------

        Copyright 2018 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_list.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-07-02

        @ingroup        ov_basics

        @brief


    ------------------------------------------------------------------------
    */

// #include "../../include/ov_list.h"

#define MAGIC_BYTE OV_LIST_MAGIC_BYTE

#define DEFAULT_LIST_ITEM_RATE 100
#define DEFAULT_LIST_SHRINK false

/* include default implementation */
#include "../../include/ov_vector_list.h"

/*
 *      ------------------------------------------------------------------------
 *
 *                        PROTOTYPE DEFINITION
 *
 *      ------------------------------------------------------------------------
 */

static bool list_copy_each(void *item, void *destination_list);
static ov_list *list_copy(const ov_list *self, ov_list *destination);
static bool list_dump(const ov_list *self, FILE *stream);

/*
 *      ------------------------------------------------------------------------
 *
 *                        Default IMPLEMENTATION
 *
 *      ------------------------------------------------------------------------
 */

ov_list *ov_list_cast(const void *data) {

    if (!data) return NULL;

    if (*(uint16_t *)data != MAGIC_BYTE) return NULL;

    return (ov_list *)data;
}

/*----------------------------------------------------------------------------*/

ov_list *ov_list_create(ov_list_config config) {

    ov_list *list = ov_vector_list_create(config);
    if (!list) goto error;

    if (!ov_vector_list_set_shrink(list, DEFAULT_LIST_SHRINK)) goto error;

    if (!ov_vector_list_set_rate(list, DEFAULT_LIST_ITEM_RATE)) goto error;

    return list;

error:
    if (list) list->free(list);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_list_clear(void *data) {

    ov_list *list = ov_list_cast(data);

    if (!list) return false;

    if (list->clear) return list->clear(list);

    return false;
}

/*---------------------------------------------------------------------------*/

void *ov_list_free(void *data) {

    ov_list *list = ov_list_cast(data);

    if (!list) return data;

    if (!list->clear || !list->free) return data;

    if (!list->clear(list)) return data;

    return list->free(list);
}

/*----------------------------------------------------------------------------*/

void *ov_list_copy(void **destination, const void *source) {

    if (!destination || !source) return NULL;

    ov_list *orig = ov_list_cast(source);
    ov_list *copy = NULL;

    if (!orig || !orig->copy) return NULL;

    if (*destination) {

        copy = ov_list_cast(*destination);
        if (!copy) return NULL;

    } else {

        *destination = ov_list_create(orig->config);
        copy = *destination;
    }

    if (!orig->copy(orig, copy)) return NULL;

    return copy;
}

/*----------------------------------------------------------------------------*/

bool ov_list_dump(FILE *stream, const void *data) {

    if (!stream || !data) return false;

    ov_list *list = ov_list_cast(data);

    if (!list) return false;

    if (list_dump(list, stream)) return true;

    if (!list->count) return false;

    if (!fprintf(stream, "LIST %p has %zu items\n", list, list->count(list)))
        return false;

    return true;
}

/*----------------------------------------------------------------------------*/

ov_data_function ov_list_data_functions() {

    ov_data_function function = {

        .clear = ov_list_clear,
        .free = ov_list_free,
        .copy = ov_list_copy,
        .dump = ov_list_dump

    };

    return function;
}

/*---------------------------------------------------------------------------*/

ov_list *ov_list_set_magic_bytes(ov_list *list) {

    if (0 == list) goto error;

    list->magic_byte = MAGIC_BYTE;

    return list;

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

ov_list_default_implementations ov_list_get_default_implementations() {

    return (ov_list_default_implementations){
        .copy = list_copy,
    };
}

/*----------------------------------------------------------------------------*/

static bool list_copy_each(void *item, void *destination_list) {

    ov_list *dest = ov_list_cast(destination_list);
    if (!dest) return false;

    void *new_item = NULL;

    if (item)
        if (!dest->config.item.copy(&new_item, item)) return false;

    return dest->push(dest, new_item);
}

/*----------------------------------------------------------------------------*/

static ov_list *list_copy(const ov_list *self, ov_list *destination) {

    if (!self || !destination) return NULL;

    if (NULL == self->config.item.copy) return NULL;

    destination->config = self->config;

    if (!self->for_each((ov_list *)self, destination, list_copy_each))
        return NULL;

    return destination;
};

/*----------------------------------------------------------------------------*/

struct ov_list_dump_data {

    FILE *stream;
    OV_DATA_DUMP dump;
};

/*----------------------------------------------------------------------------*/

static bool list_dump_each(void *item, void *list_dump_data) {

    if (!list_dump_data) return false;

    struct ov_list_dump_data *d = list_dump_data;
    return d->dump(d->stream, item);
}

/*----------------------------------------------------------------------------*/

static bool list_dump(const ov_list *list, FILE *stream) {

    if (!list || !stream) return false;

    if (NULL == list->config.item.dump) return false;

    struct ov_list_dump_data data = {

        .stream = stream, .dump = list->config.item.dump};

    return list->for_each((ov_list *)list, &data, list_dump_each);
}

/*
 *      ------------------------------------------------------------------------
 *
 *                        GENERIC FUNCTIONS
 *
 *       ... definition of common generic list functions
 *
 *      ------------------------------------------------------------------------
 */

bool ov_list_remove_if_included(ov_list *list, const void *item) {

    if (!list || !ov_list_cast(list)) goto error;

    if (!item || list->is_empty(list)) return true;

    size_t pos = list->get_pos(list, item);

    // was not included ?
    if (pos == 0) return true;

    // was removed ?
    if (list->remove(list, pos)) return true;

error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *                        FUNCTIONS TO INTERNAL POINTERS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_list_is_empty(const ov_list *list) {

    if (!list || !list->is_empty) return false;

    return list->is_empty(list);
}

/*---------------------------------------------------------------------------*/

size_t ov_list_get_pos(const ov_list *list, void *item) {

    if (!list || !list->get_pos || !item) return 0;

    return list->get_pos(list, item);
}

/*---------------------------------------------------------------------------*/

void *ov_list_get(const ov_list *list, size_t pos) {

    if (!list || !list->get) return NULL;

    return list->get((ov_list *)list, pos);
}

/*---------------------------------------------------------------------------*/

bool ov_list_set(ov_list *list, size_t pos, void *item, void **replaced) {

    if (!list || !list->set || !item) return false;

    return list->set(list, pos, item, replaced);
}

/*---------------------------------------------------------------------------*/

bool ov_list_insert(ov_list *list, size_t pos, void *item) {

    if (!list || !list->insert || !item) return false;

    return list->insert(list, pos, item);
}

/*---------------------------------------------------------------------------*/

void *ov_list_remove(ov_list *list, size_t pos) {

    if (!list || !list->remove) return NULL;

    return list->remove(list, pos);
}

/*---------------------------------------------------------------------------*/

bool ov_list_delete(ov_list *list, size_t pos) {

    if (!list || !list->remove || !list->config.item.free) return false;

    void *item = list->remove(list, pos);
    item = list->config.item.free(item);

    if (item) return false;
    return true;
}

/*---------------------------------------------------------------------------*/

bool ov_list_push(ov_list *list, void *item) {

    if (!list || !list->push) return false;

    return list->push(list, item);
}

/*---------------------------------------------------------------------------*/

void *ov_list_pop(ov_list *list) {

    if (!list || !list->pop) return NULL;

    return list->pop(list);
}

/*---------------------------------------------------------------------------*/

size_t ov_list_count(const ov_list *list) {

    if (!list || !list->count) return 0;

    return list->count(list);
}

/*---------------------------------------------------------------------------*/

bool ov_list_for_each(ov_list *list,
                      void *data,
                      bool (*function)(void *item, void *data)) {

    if (!list || !list->for_each || !function) return false;

    return list->for_each(list, data, function);
}

/*---------------------------------------------------------------------------*/

bool ov_list_queue_push(ov_list *list, void *item) {

    return ov_list_insert(list, 1, item);
}

/*---------------------------------------------------------------------------*/

void *ov_list_queue_pop(ov_list *list) { return ov_list_pop(list); }
