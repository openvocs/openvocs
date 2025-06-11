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
        @file           ov_vector_list.c
        @author         Markus Toepfer

        @date           2018-08-09

        @ingroup        ov_basics

        @brief          Implementation of ov_vector_list.


        ------------------------------------------------------------------------
*/
#include "../../include/ov_vector_list.h"

/******************************************************************************
 *                          INTERNAL DATA STRUCTURES
 ******************************************************************************/

const uint16_t IMPL_VECTOR_LIST_TYPE = 0x4c4d;

typedef struct {

    ov_list public;

    struct config {

        bool shrink;
        size_t rate;

    } config;

    size_t last;
    size_t size;

    void **items;

} VectorList;

#define VECTOR_DEFAULT_SIZE 100

#define AS_VECTOR_LIST(x)                                                      \
    (((ov_list_cast(x) != 0) &&                                                \
      (IMPL_VECTOR_LIST_TYPE == ((ov_list *)x)->type))                         \
         ? (VectorList *)(x)                                                   \
         : 0)

/******************************************************************************
 *                             INTERNAL FUNCTIONS
 ******************************************************************************/

static bool impl_vector_list_is_empty(const ov_list *self);
static bool impl_vector_list_clear(ov_list *self);
static ov_list *impl_vector_list_free(ov_list *self);
static size_t impl_vector_list_get_pos(const ov_list *self, const void *item);
static void *impl_vector_list_get(ov_list *self, size_t pos);
static bool impl_vector_list_set(ov_list *self,
                                 size_t pos,
                                 void *item,
                                 void **old);
static bool impl_vector_list_insert(ov_list *self, size_t pos, void *item);
static void *impl_vector_list_remove(ov_list *self, size_t pos);
static bool impl_vector_list_push(ov_list *self, void *item);
static void *impl_vector_list_pop(ov_list *self);
static size_t impl_vector_list_count(const ov_list *self);
static bool impl_vector_list_for_each(ov_list *self,
                                      void *data,
                                      bool (*function)(void *item, void *data));

static void *impl_vector_list_iter(ov_list *self);
static void *impl_vector_list_next(ov_list *self, void *iter, void **element);

/******************************************************************************
 *                              PUBLIC FUNCTIONS
 ******************************************************************************/

ov_list *ov_vector_list_create(ov_list_config config) {

    ov_list_default_implementations default_implementations =
        ov_list_get_default_implementations();

    VectorList *list = calloc(1, sizeof(VectorList));

    *list = (VectorList){
        .public =
            (ov_list){

                .type = IMPL_VECTOR_LIST_TYPE,
                .config = config,
                .is_empty = impl_vector_list_is_empty,
                .clear = impl_vector_list_clear,
                .free = impl_vector_list_free,
                .copy = default_implementations.copy,
                .get_pos = impl_vector_list_get_pos,
                .get = impl_vector_list_get,
                .set = impl_vector_list_set,
                .insert = impl_vector_list_insert,
                .remove = impl_vector_list_remove,
                .push = impl_vector_list_push,
                .pop = impl_vector_list_pop,
                .count = impl_vector_list_count,
                .for_each = impl_vector_list_for_each,
                .iter = impl_vector_list_iter,
                .next = impl_vector_list_next,
            },

        .config.shrink = false,
        .config.rate = VECTOR_DEFAULT_SIZE,
        .last = 0,
        .size = VECTOR_DEFAULT_SIZE,

        .items = calloc(VECTOR_DEFAULT_SIZE, sizeof(void *)),

    };

    return ov_list_set_magic_bytes((ov_list *)list);
}

/*----------------------------------------------------------------------------*/

bool ov_vector_list_set_shrink(ov_list *self, bool shrink) {

    VectorList *list = AS_VECTOR_LIST(self);
    if (!list) goto error;

    list->config.shrink = shrink;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vector_list_set_rate(ov_list *self, size_t rate) {

    VectorList *list = AS_VECTOR_LIST(self);
    if (!list || rate < 1) goto error;

    list->config.rate = rate;
    return true;
error:
    return false;
}

/******************************************************************************
 *                              PRIVATE FUNCTIONS
 ******************************************************************************/

static bool vector_resize(VectorList *vector, size_t slots) {

    if (!vector || !vector->items) goto error;

    if (slots == 0 || vector->last >= slots - 1) goto error;

    if (vector->size == slots - 1) return true;

    size_t old_max = vector->size;
    void *newPtr = realloc(vector->items, slots * sizeof(void *));
    if (!newPtr) goto error;

    vector->items = newPtr;
    vector->size = slots - 1;

    // initialize new slots to 0
    for (size_t i = old_max; i <= vector->size; i++) {
        vector->items[i] = NULL;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool vector_auto_adjust(VectorList *self) {

    if (!self || !self->items) return false;

    if (self->last >= self->size) return false;

    /* grow if the vector is used by 100 % */
    if (self->last == self->size - 1)
        return vector_resize(self, (self->size + self->config.rate + 1));

    if (self->config.shrink == false) return true;

    /* shrink if the use of the vector is smaller than the rate */
    if ((self->size - self->last) > self->config.rate)
        return vector_resize(self, (self->last + self->config.rate + 1));

    return true;
}

/*----------------------------------------------------------------------------*/

static bool vector_list_is_empty(VectorList *list) {

    if (!list || !list->items) return false;

    if (list->last == 0) return true;

    return false;
}

/******************************************************************************
 *                         IMPLEMENTATION FUNCTIONS
 ******************************************************************************/

static bool impl_vector_list_is_empty(const ov_list *self) {

    VectorList *list = AS_VECTOR_LIST(self);
    if (!list) goto error;

    if (list->last == 0) return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool impl_vector_list_clear(ov_list *self) {

    VectorList *list = AS_VECTOR_LIST(self);
    if (!list) goto error;

    for (size_t i = 0; i < list->size; i++) {

        /* Use configured item free by default. */

        if (self->config.item.free)
            list->items[i] = self->config.item.free(list->items[i]);

        /* Unset the pointer. */
        list->items[i] = NULL;
    }

    list->last = 0;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static ov_list *impl_vector_list_free(ov_list *self) {

    VectorList *list = AS_VECTOR_LIST(self);
    if (!list) goto error;

    if (!self->clear(self)) goto error;

    free(list->items);
    free(list);

    return NULL;
error:
    return self;
}

/*----------------------------------------------------------------------------*/

static size_t impl_vector_list_get_pos(const ov_list *self, const void *item) {

    VectorList *list = AS_VECTOR_LIST(self);
    if (!list || (!item)) goto error;

    for (size_t i = 0; i <= list->last; i++) {

        if (list->items[i] == item) return i;
    }

error:
    return 0;
}

/*----------------------------------------------------------------------------*/

static void *impl_vector_list_get(ov_list *self, size_t pos) {

    VectorList *list = AS_VECTOR_LIST(self);
    if (!list || (pos == 0)) goto error;

    if (pos > list->last) goto error;

    return list->items[pos];

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool impl_vector_list_set(ov_list *self,
                                 size_t pos,
                                 void *item,
                                 void **old) {

    VectorList *list = AS_VECTOR_LIST(self);
    if (!list || (pos == 0)) goto error;

    if (pos >= list->size)
        if (!vector_resize(list, pos + 2)) goto error;

    if (old) *old = list->items[pos];

    list->items[pos] = item;

    if (pos > list->last) list->last = pos;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool impl_vector_list_insert(ov_list *self, size_t pos, void *item) {

    VectorList *list = AS_VECTOR_LIST(self);
    if (!list || (pos == 0)) goto error;

    if (pos > list->last) return self->set(self, pos, item, NULL);

    if (list->last == list->size - 1)
        if (!vector_resize(list, list->size + list->config.rate)) goto error;

    list->last = list->last + 1;
    for (size_t i = list->last + 1; i > pos; i--) {

        list->items[i] = list->items[i - 1];
    }

    list->items[pos] = item;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void *impl_vector_list_remove(ov_list *self, size_t pos) {

    VectorList *list = AS_VECTOR_LIST(self);
    if (!list || (pos == 0)) goto error;

    if (pos > list->last) goto error;

    void *item = list->items[pos];

    for (size_t i = pos; i <= list->last; i++) {

        list->items[i] = list->items[i + 1];
    }
    list->items[list->last] = NULL;
    list->last = list->last - 1;

    vector_auto_adjust(list);
    return item;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool impl_vector_list_push(ov_list *self, void *item) {

    VectorList *list = AS_VECTOR_LIST(self);
    if (!list) goto error;

    if (list->last == list->size - 1)
        if (!vector_resize(list, list->size + list->config.rate)) goto error;

    list->last = list->last + 1;
    list->items[list->last] = item;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void *impl_vector_list_pop(ov_list *self) {

    VectorList *list = AS_VECTOR_LIST(self);
    if (!list || !list->items) goto error;

    if (list->last == 0) return NULL;

    void *item = list->items[list->last];
    list->items[list->last] = NULL;

    list->last -= 1;

    vector_auto_adjust(list);
    return item;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static size_t impl_vector_list_count(const ov_list *self) {

    VectorList *list = AS_VECTOR_LIST(self);
    if (!list) goto error;

    if (self->is_empty(self)) return 0;

    return list->last;

error:
    return 0;
}

/*----------------------------------------------------------------------------*/

static bool impl_vector_list_for_each(
    ov_list *self, void *data, bool (*function)(void *item, void *data)) {

    VectorList *list = AS_VECTOR_LIST(self);
    if (!list || !function) goto error;

    if (vector_list_is_empty(list)) return true;

    for (size_t i = 1; i <= list->last; i++) {

        if (!function(list->items[i], data)) goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

void *impl_vector_list_iter(ov_list *self) {

    VectorList *list = AS_VECTOR_LIST(self);
    if (!list || !list->items) goto error;

    // empty?
    if (impl_vector_list_is_empty(self)) return NULL;

    return *list->items + 1;
error:

    return 0;
}

/*---------------------------------------------------------------------------*/

void *impl_vector_list_next(ov_list *self, void *iter, void **element) {

    VectorList *list = AS_VECTOR_LIST(self);
    if (!list || !list->items) goto error;

    if (0 == iter) goto error;

    // check if iter is part of the list
    if (iter - *list->items > (int64_t)list->last) goto error;

    size_t pos = (iter - *list->items);

    // walk list->items, which is a pointer array of pointers
    if (element) *element = list->items[pos];

    if (pos == list->last) return NULL;

    return *list->items + pos + 1;

error:

    if (0 != element) {
        *element = 0;
    }
    return 0;
}