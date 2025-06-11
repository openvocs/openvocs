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
        @file           ov_json_array.c
        @author         Markus Toepfer

        @date           2018-12-02

        @ingroup        ov_json_array

        @brief          Implementation of a standard interface for JSON ARRAY.

        ------------------------------------------------------------------------
*/

#include "../../include/ov_json_array.h"

/*----------------------------------------------------------------------------*/

typedef struct json_array json_array;

/*----------------------------------------------------------------------------*/

struct json_array {

    ov_json_value head;
    size_t type;

    /*--------------------------------------------------------------------*/

    bool (*push)(json_array *self, ov_json_value *value);

    ov_json_value *(*pop)(json_array *self);

    /*--------------------------------------------------------------------*/

    /* ERROR == 0 SUCCESS = 1 ... */
    size_t (*find)(json_array *self, const ov_json_value *child);

    bool (*del)(json_array *self, size_t position);

    ov_json_value *(*get)(json_array *self, size_t position);

    ov_json_value *(*remove)(json_array *self, size_t position);

    bool (*insert)(json_array *self, size_t position, ov_json_value *value);

    /*--------------------------------------------------------------------*/

    size_t (*count)(const json_array *self);

    bool (*is_empty)(const json_array *self);

    /* NOTE: for_each SHOULD not be used to free any array values
     *       you MAY search the value using for_each
     *       but FREE it after the for_each run.
     *       (external, don't delete during internal iteration) */
    bool (*for_each)(json_array *self,
                     void *data,
                     bool (*function)(void *value, void *data));

    bool (*remove_child)(json_array *self, ov_json_value *child);
};

#define AS_JSON_ARRAY(x)                                                       \
    (((ov_json_value_cast(x) != 0) &&                                          \
      (OV_JSON_ARRAY == ((ov_json_value *)x)->type))                           \
         ? (json_array *)(x)                                                   \
         : 0)

/*----------------------------------------------------------------------------*/

static ov_json_value *ov_json_list();

ov_json_value *ov_json_array() { return ov_json_list(); }

/*----------------------------------------------------------------------------*/

bool ov_json_is_array(const ov_json_value *value) {

    if (AS_JSON_ARRAY(value)) return true;

    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

json_array *ov_json_array_cast(const void *data) { return AS_JSON_ARRAY(data); }

/*----------------------------------------------------------------------------*/

json_array *ov_json_array_set_head(json_array *self) {

    if (!self) return NULL;

    self->head.magic_byte = OV_JSON_VALUE_MAGIC_BYTE;
    self->head.type = OV_JSON_ARRAY;
    self->head.parent = NULL;

    return self;
}

/*----------------------------------------------------------------------------*/

bool ov_json_array_clear(void *data) {

    json_array *array = AS_JSON_ARRAY(data);
    if (!array || !array->head.clear) return false;

    return array->head.clear(array);
}

/*----------------------------------------------------------------------------*/

void *ov_json_array_free(void *data) {

    json_array *array = AS_JSON_ARRAY(data);
    if (!array || !array->head.free) return data;

    return array->head.free(array);
}

/*----------------------------------------------------------------------------*/

static bool copy_items(void *value, void *data) {

    ov_json_value *val = ov_json_value_cast(value);
    if (!val || !data) return false;

    ov_json_value *content = NULL;
    json_array *target = ov_json_array_cast(data);
    if (!target || !target->push) goto error;

    if (!ov_json_value_copy((void **)&content, val)) goto error;

    if (!content) goto error;

    if (target->push(target, content)) return true;

    content = ov_json_value_free(content);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

void *ov_json_array_copy(void **dest, const void *data) {

    bool created = false;
    json_array *copy = NULL;
    json_array *orig = AS_JSON_ARRAY(data);
    if (!dest || !orig) return NULL;

    if (!*dest) {

        *dest = ov_json_array();
        if (!*dest) goto error;

        created = true;
    }

    copy = ov_json_array_cast(*dest);
    if (!ov_json_array_clear(copy)) goto error;

    if (!orig->for_each(orig, copy, copy_items)) goto error;

    return copy;
error:
    if (created) *dest = ov_json_array_free(*dest);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool dump_items(void *value, void *data) {

    ov_json_value *val = ov_json_value_cast(value);
    if (!val || !data) return false;

    if (!ov_json_value_dump((FILE *)data, val)) return false;

    if (!fprintf((FILE *)data, "\n")) return false;

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_json_array_dump(FILE *stream, const void *data) {

    json_array *array = AS_JSON_ARRAY(data);
    if (!stream || !array) return false;

    if (!fprintf(stream, "\n[\n")) goto error;

    if (!array->for_each(array, stream, dump_items)) goto error;

    if (!fprintf(stream, "]\n")) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_data_function ov_json_array_data_functions() {

    ov_data_function f = {

        .clear = ov_json_array_clear,
        .free = ov_json_array_free,
        .dump = ov_json_array_dump,
        .copy = ov_json_array_copy};

    return f;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      DEFAULT IMPLEMENTATION
 *
 *      ... included default implementation
 *
 *      Everything below MAY be moved to external file
 *      but is included to have a cleaner & leaner JSON interface
 *      (with implementation)
 *
 *      ------------------------------------------------------------------------
 */

#include "ov_list.h"
#define OV_JSON_ARRAY_LIST 0xabcd

typedef struct {

    json_array public;
    ov_list *data;

} JsonList;

/*----------------------------------------------------------------------------*/

#define AS_JSON_LIST(x)                                                        \
    (((ov_json_value_cast(x) != 0) &&                                          \
      (OV_JSON_ARRAY == ((ov_json_value *)(x))->type) &&                       \
      (OV_JSON_ARRAY_LIST == ((json_array *)(x))->type))                       \
         ? (JsonList *)(x)                                                     \
         : 0)

/*
 *      ------------------------------------------------------------------------
 *
 *      INTERFACE IMPLEMENTATIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool impl_json_array_clear(void *self);
static void *impl_json_array_free(void *self);

/*----------------------------------------------------------------------------*/

static bool impl_json_array_push(json_array *self, ov_json_value *value);
static ov_json_value *impl_json_array_pop(json_array *self);

/*----------------------------------------------------------------------------*/

static size_t impl_json_array_find(json_array *self,
                                   const ov_json_value *value);

static bool impl_json_array_del(json_array *self, size_t position);

static ov_json_value *impl_json_array_get(json_array *self, size_t position);

static ov_json_value *impl_json_array_remove(json_array *self, size_t position);

static bool impl_json_array_insert(json_array *self,
                                   size_t position,
                                   ov_json_value *value);

/*----------------------------------------------------------------------------*/

static size_t impl_json_array_count(const json_array *self);
static bool impl_json_array_is_empty(const json_array *self);

static bool impl_json_array_for_each(json_array *self,
                                     void *data,
                                     bool (*function)(void *value, void *data));

static bool impl_json_array_remove_child(json_array *self,
                                         ov_json_value *value);

/*----------------------------------------------------------------------------*/

ov_list_config json_list_config() {

    ov_list_config config = (ov_list_config){
        .item = ov_json_value_data_functions(),
    };
    return config;
}

/*----------------------------------------------------------------------------*/

bool json_list_init(JsonList *list) {

    if (!list) goto error;

    // set head
    if (!ov_json_array_set_head((json_array *)list)) goto error;

    list->public.type = OV_JSON_ARRAY_LIST;

    if (list->data) list->data = ov_list_free(list->data);
    list->data = ov_list_create(json_list_config());

    // set interface functions

    list->public.head.clear = impl_json_array_clear;
    list->public.head.free = impl_json_array_free;

    list->public.push = impl_json_array_push;
    list->public.pop = impl_json_array_pop;

    list->public.find = impl_json_array_find;
    list->public.del = impl_json_array_del;
    list->public.get = impl_json_array_get;
    list->public.remove = impl_json_array_remove;
    list->public.insert = impl_json_array_insert;

    list->public.count = impl_json_array_count;
    list->public.is_empty = impl_json_array_is_empty;
    list->public.for_each = impl_json_array_for_each;
    list->public.remove_child = impl_json_array_remove_child;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *ov_json_list() {

    JsonList *list = calloc(1, sizeof(JsonList));
    if (!list) goto error;

    if (json_list_init(list)) return (ov_json_value *)list;

    free(list);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool impl_json_array_clear(void *self) {

    JsonList *list = AS_JSON_LIST(self);
    if (!list || !list->data) goto error;

    // pop empty
    ov_json_value *value = ov_list_pop(list->data);
    while (value) {

        value->parent = NULL;
        value = ov_json_value_free(value);
        value = ov_list_pop(list->data);
    }

    return ov_list_clear(list->data);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

void *impl_json_array_free(void *self) {

    JsonList *list = AS_JSON_LIST(self);
    if (!list) return self;

    // in case of parent loop over ov_json_value_free
    if (list->public.head.parent) return ov_json_value_free(self);

    if (!impl_json_array_clear(self)) return self;

    ov_list_free(list->data);
    free(list);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool impl_json_array_push(json_array *self, ov_json_value *value) {

    JsonList *list = AS_JSON_LIST(self);
    if (!list || !list->data || !value) goto error;

    if (value == (ov_json_value *)self) goto error;

    if (!ov_json_value_validate(value)) goto error;

    if (!ov_json_value_set_parent(value, (ov_json_value *)self)) goto error;

    if (ov_list_push(list->data, value)) return true;

    // unset on push failure
    value->parent = NULL;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *impl_json_array_pop(json_array *self) {

    JsonList *list = AS_JSON_LIST(self);
    if (!list || !list->data) goto error;

    ov_json_value *value = ov_list_pop(list->data);
    if (value) value->parent = NULL;

    return value;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

size_t impl_json_array_find(json_array *self, const ov_json_value *value) {

    JsonList *list = AS_JSON_LIST(self);
    if (!list || !list->data) goto error;

    return ov_list_get_pos(list->data, (void *)value);
error:
    return 0;
}

/*----------------------------------------------------------------------------*/

bool impl_json_array_del(json_array *self, size_t position) {

    JsonList *list = AS_JSON_LIST(self);
    if (!list || !list->data) goto error;

    ov_json_value *value = impl_json_array_remove(self, position);
    if (!value) goto error;

    ov_json_value_free(value);
    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *impl_json_array_get(json_array *self, size_t position) {

    JsonList *list = AS_JSON_LIST(self);
    if (!list || !list->data) goto error;

    return (ov_json_value *)ov_list_get(list->data, position);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *impl_json_array_remove(json_array *self, size_t position) {

    JsonList *list = AS_JSON_LIST(self);
    if (!list || !list->data) goto error;

    ov_json_value *value = ov_list_remove(list->data, position);
    if (!value) goto error;

    value->parent = NULL;
    return value;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool impl_json_array_insert(json_array *self,
                            size_t pos,
                            ov_json_value *value) {

    JsonList *list = AS_JSON_LIST(self);
    if (!list || !list->data) goto error;

    if (!ov_json_value_validate(value)) goto error;

    if (value->parent) goto error;

    if (value == (ov_json_value *)self) goto error;

    if (pos > (ov_list_count(list->data) + 1)) goto error;

    if (!ov_json_value_set_parent(value, (ov_json_value *)self)) goto error;

    if (ov_list_insert(list->data, pos, value)) return true;

    // unset on insert failure
    value->parent = NULL;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

size_t impl_json_array_count(const json_array *self) {

    JsonList *list = AS_JSON_LIST(self);
    if (!list || !list->data) goto error;

    return ov_list_count(list->data);
error:
    return 0;
}

/*----------------------------------------------------------------------------*/

bool impl_json_array_is_empty(const json_array *self) {

    JsonList *list = AS_JSON_LIST(self);
    if (!list || !list->data) goto error;

    return ov_list_is_empty(list->data);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool impl_json_array_for_each(json_array *self,
                              void *data,
                              bool (*function)(void *value, void *data)) {

    JsonList *list = AS_JSON_LIST(self);
    if (!list || !list->data) goto error;

    return ov_list_for_each(list->data, data, function);

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool impl_json_array_remove_child(json_array *self, ov_json_value *child) {

    JsonList *list = AS_JSON_LIST(self);
    if (!list || !list->data) goto error;

    if (!child) return true;

    if (child->parent)
        if (child->parent != (ov_json_value *)self) goto error;

    if (!ov_list_remove_if_included(list->data, child)) goto error;

    child->parent = NULL;
    return true;
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      VALUE BASED INTERFACE FUNCTION CALLS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_json_array_push(ov_json_value *array, ov_json_value *value) {

    json_array *arr = ov_json_array_cast(array);
    if (!arr || !arr->push || !value) return false;

    return arr->push(arr, value);
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_json_array_pop(ov_json_value *array) {

    json_array *arr = ov_json_array_cast(array);
    if (!arr || !arr->pop) return NULL;

    return arr->pop(arr);
}

/*----------------------------------------------------------------------------*/

size_t ov_json_array_find(ov_json_value *array, const ov_json_value *child) {

    json_array *arr = ov_json_array_cast(array);
    if (!arr || !arr->find || !child) return 0;

    return arr->find(arr, child);
}

/*----------------------------------------------------------------------------*/

bool ov_json_array_del(ov_json_value *array, size_t position) {

    json_array *arr = ov_json_array_cast(array);
    if (!arr || !arr->del) return false;

    return arr->del(arr, position);
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_json_array_get(ov_json_value *array, size_t position) {

    json_array *arr = ov_json_array_cast(array);
    if (!arr || !arr->get) return NULL;

    return arr->get(arr, position);
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_json_array_remove(ov_json_value *array, size_t position) {

    json_array *arr = ov_json_array_cast(array);
    if (!arr || !arr->remove) return NULL;

    return arr->remove(arr, position);
}

/*----------------------------------------------------------------------------*/

bool ov_json_array_insert(ov_json_value *array,
                          size_t position,
                          ov_json_value *value) {

    json_array *arr = ov_json_array_cast(array);
    if (!arr || !arr->insert || !value) return false;

    return arr->insert(arr, position, value);
}

/*----------------------------------------------------------------------------*/

size_t ov_json_array_count(const ov_json_value *array) {

    json_array *arr = ov_json_array_cast(array);
    if (!arr || !arr->count) return 0;

    return arr->count(arr);
}

/*----------------------------------------------------------------------------*/

bool ov_json_array_is_empty(const ov_json_value *array) {

    json_array *arr = ov_json_array_cast(array);
    if (!arr || !arr->is_empty) return false;

    return arr->is_empty(arr);
}

/*----------------------------------------------------------------------------*/

bool ov_json_array_for_each(ov_json_value *array,
                            void *data,
                            bool (*function)(void *value, void *data)) {

    json_array *arr = ov_json_array_cast(array);
    if (!arr || !arr->for_each || !function) return false;

    return arr->for_each(arr, data, function);
}

/*----------------------------------------------------------------------------*/

bool ov_json_array_remove_child(ov_json_value *array, ov_json_value *child) {

    json_array *arr = ov_json_array_cast(array);
    if (!arr || !arr->remove_child || !child) return false;

    return arr->remove_child(arr, child);
}

/*----------------------------------------------------------------------------*/

ov_json_value *(*array_creator)();

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_impl_json_array_clear() {

    ov_json_value *value = array_creator();
    json_array *array = ov_json_array_cast(value);

    testrun(array);
    testrun(array->head.clear);

    testrun(0 == ov_json_array_count(value));

    testrun(!array->head.clear(NULL));
    testrun(array->head.clear(array));
    testrun(0 == ov_json_array_count(value));

    testrun(ov_json_array_push(value, ov_json_true()));
    testrun(ov_json_array_push(value, ov_json_number(1)));
    testrun(ov_json_array_push(value, ov_json_string("test")));
    // custom creator
    testrun(ov_json_array_push(value, array_creator()));
    // default creator
    testrun(ov_json_array_push(value, ov_json_array()));
    testrun(5 == ov_json_array_count(value));
    testrun(array->head.clear(array));
    testrun(0 == ov_json_array_count(value));

    ov_json_array_free(array);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_array_free() {

    ov_json_value *value = array_creator();
    json_array *array = ov_json_array_cast(value);
    ov_json_value *val1 = ov_json_true();

    testrun(array);
    testrun(array->head.free);

    testrun(NULL == array->head.free(NULL));
    testrun(val1 == array->head.free((json_array *)val1));
    testrun(NULL == array->head.free(array));

    value = array_creator();
    array = ov_json_array_cast(value);
    testrun(ov_json_array_push(value, ov_json_true()));
    testrun(ov_json_array_push(value, ov_json_number(1)));
    testrun(ov_json_array_push(value, ov_json_string("test")));
    // custom creator
    testrun(ov_json_array_push(value, array_creator()));
    // default creator
    testrun(ov_json_array_push(value, ov_json_array()));
    testrun(5 == ov_json_array_count(value));
    testrun(NULL == array->head.free(array));

    ov_json_value_free(val1);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_array_push() {

    ov_json_value *value = array_creator();
    json_array *array = ov_json_array_cast(value);
    ov_json_value *val1 = ov_json_true();

    testrun(array);
    testrun(array->push);

    testrun(!array->push(NULL, NULL));
    testrun(!array->push(array, NULL));
    testrun(!array->push(NULL, val1));

    testrun(0 == ov_json_array_count(value));
    testrun(array->push(array, val1));
    testrun(1 == ov_json_array_count(value));

    val1 = ov_json_string("test");
    val1->type = NOT_JSON;
    testrun(!array->push(array, val1));
    testrun(1 == ov_json_array_count(value));
    val1->type = OV_JSON_STRING;
    testrun(array->push(array, val1));
    testrun(2 == ov_json_array_count(value));

    // push self
    testrun(!array->push(array, value));
    testrun(2 == ov_json_array_count(value));
    testrun(value->parent == NULL);

    ov_json_array_free(array);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_array_pop() {

    ov_json_value *value = array_creator();
    json_array *array = ov_json_array_cast(value);

    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_number(1);
    ov_json_value *val3 = ov_json_string("test");

    testrun(array);
    testrun(array->pop);

    testrun(!array->pop(NULL));
    testrun(!array->pop(array));

    testrun(array->push(array, val1));
    testrun(array->push(array, val2));
    testrun(array->push(array, val3));

    testrun(val3 == array->pop(array));
    testrun(val2 == array->pop(array));
    testrun(val1 == array->pop(array));
    testrun(NULL == array->pop(array));

    ov_json_array_free(array);
    ov_json_value_free(val1);
    ov_json_value_free(val2);
    ov_json_value_free(val3);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_array_find() {

    ov_json_value *value = array_creator();
    json_array *array = ov_json_array_cast(value);

    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_number(1);
    ov_json_value *val3 = ov_json_string("test");

    testrun(array);
    testrun(array->find);

    testrun(array->push(array, val1));
    testrun(array->push(array, val2));
    testrun(array->push(array, val3));

    testrun(!array->find(NULL, NULL));
    testrun(!array->find(NULL, val2));
    testrun(!array->find(array, NULL));

    testrun(1 == array->find(array, val1));
    testrun(2 == array->find(array, val2));
    testrun(3 == array->find(array, val3));

    // not contained
    testrun(val3 == array->pop(array));
    testrun(0 == array->find(array, val3));

    // find self
    testrun(0 == array->find(array, value));

    ov_json_array_free(array);
    ov_json_value_free(val3);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_array_del() {

    ov_json_value *value = array_creator();
    json_array *array = ov_json_array_cast(value);

    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_number(1);
    ov_json_value *val3 = ov_json_string("test");

    testrun(array);
    testrun(array->del);

    testrun(array->push(array, val1));
    testrun(array->push(array, val2));
    testrun(array->push(array, val3));

    testrun(!array->del(NULL, 0));
    testrun(!array->del(NULL, 0));
    testrun(!array->del(array, 0));

    testrun(1 == array->find(array, val1));
    testrun(2 == array->find(array, val2));
    testrun(3 == array->find(array, val3));

    testrun(!array->del(array, 4));
    testrun(array->del(array, 2));

    testrun(1 == array->find(array, val1));
    testrun(0 == array->find(array, val2));
    testrun(2 == array->find(array, val3));

    testrun(array->del(array, 2));

    testrun(1 == array->find(array, val1));
    testrun(0 == array->find(array, val2));
    testrun(0 == array->find(array, val3));

    testrun(array->del(array, 1));

    testrun(0 == array->find(array, val1));
    testrun(0 == array->find(array, val2));
    testrun(0 == array->find(array, val3));

    ov_json_array_free(array);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_array_get() {

    ov_json_value *value = array_creator();
    json_array *array = ov_json_array_cast(value);

    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_number(1);
    ov_json_value *val3 = ov_json_string("test");

    testrun(array);
    testrun(array->get);

    testrun(array->push(array, val1));
    testrun(array->push(array, val2));
    testrun(array->push(array, val3));

    testrun(!array->get(NULL, 0));
    testrun(!array->get(NULL, 0));
    testrun(!array->get(array, 0));

    testrun(1 == array->find(array, val1));
    testrun(2 == array->find(array, val2));
    testrun(3 == array->find(array, val3));

    testrun(val1 == array->get(array, 1));
    testrun(val2 == array->get(array, 2));
    testrun(val3 == array->get(array, 3));
    testrun(NULL == array->get(array, 4));
    testrun(NULL == array->get(array, 5));
    testrun(NULL == array->get(array, 6));

    testrun(array->del(array, 2));

    testrun(val1 == array->get(array, 1));
    testrun(val3 == array->get(array, 2));
    testrun(NULL == array->get(array, 3));
    testrun(NULL == array->get(array, 4));
    testrun(NULL == array->get(array, 5));
    testrun(NULL == array->get(array, 6));

    ov_json_array_free(array);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_array_remove() {

    ov_json_value *value = array_creator();
    json_array *array = ov_json_array_cast(value);

    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_number(1);
    ov_json_value *val3 = ov_json_string("test");

    testrun(array);
    testrun(array->remove);

    testrun(array->push(array, val1));
    testrun(array->push(array, val2));
    testrun(array->push(array, val3));

    testrun(!array->remove(NULL, 0));
    testrun(!array->remove(NULL, 0));
    testrun(!array->remove(array, 0));

    testrun(1 == array->find(array, val1));
    testrun(2 == array->find(array, val2));
    testrun(3 == array->find(array, val3));
    testrun(3 == ov_json_array_count(value));

    testrun(val2 == array->remove(array, 2));
    testrun(2 == ov_json_array_count(value));
    testrun(val3 == array->remove(array, 2));
    testrun(1 == ov_json_array_count(value));
    testrun(NULL == array->remove(array, 2));
    testrun(NULL == array->remove(array, 4));
    testrun(NULL == array->remove(array, 5));
    testrun(NULL == array->remove(array, 6));
    testrun(val1 == array->remove(array, 1));
    testrun(0 == ov_json_array_count(value));

    testrun(val1->parent == NULL);
    testrun(val2->parent == NULL);
    testrun(val3->parent == NULL);

    ov_json_array_free(array);
    ov_json_value_free(val1);
    ov_json_value_free(val2);
    ov_json_value_free(val3);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_array_insert() {

    ov_json_value *value = array_creator();
    json_array *array = ov_json_array_cast(value);

    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_number(1);
    ov_json_value *val3 = ov_json_string("test");
    ov_json_value *val4 = array_creator();

    testrun(array);
    testrun(array->remove);

    testrun(!array->insert(NULL, 0, NULL));
    testrun(!array->insert(NULL, 0, val1));
    testrun(!array->insert(array, 0, val1));
    testrun(!array->insert(array, 1, NULL));

    testrun(0 == ov_json_array_count(value));
    testrun(array->insert(array, 1, val1));
    testrun(1 == ov_json_array_count(value));
    testrun(value == val1->parent);

    // insert with parent, even if parent is self
    testrun(!array->insert(array, 1, val1));
    testrun(1 == ov_json_array_count(value));
    testrun(value == val1->parent);
    testrun(1 == array->find(array, val1));
    testrun(0 == array->find(array, val2));
    testrun(0 == array->find(array, val3));

    // insert with other parent
    val2->parent = val3;
    testrun(!array->insert(array, 1, val2));
    testrun(1 == ov_json_array_count(value));
    testrun(1 == array->find(array, val1));
    testrun(0 == array->find(array, val2));
    testrun(0 == array->find(array, val3));

    val2->parent = NULL;

    // insert > last + 1
    testrun(!array->insert(array, 3, val2));
    testrun(1 == ov_json_array_count(value));
    testrun(1 == array->find(array, val1));
    testrun(0 == array->find(array, val2));
    testrun(0 == array->find(array, val3));
    testrun(val2->parent == NULL);

    // insert behind last
    testrun(array->insert(array, 2, val2));
    testrun(2 == ov_json_array_count(value));
    testrun(1 == array->find(array, val1));
    testrun(2 == array->find(array, val2));
    testrun(0 == array->find(array, val3));
    testrun(val2->parent == value);

    // insert as first
    testrun(array->insert(array, 1, val3));
    testrun(3 == ov_json_array_count(value));
    testrun(2 == array->find(array, val1));
    testrun(3 == array->find(array, val2));
    testrun(1 == array->find(array, val3));
    testrun(val3->parent == value);

    // insert middle
    testrun(array->insert(array, 3, val4));
    testrun(4 == ov_json_array_count(value));
    testrun(2 == array->find(array, val1));
    testrun(3 == array->find(array, val4));
    testrun(4 == array->find(array, val2));
    testrun(1 == array->find(array, val3));
    testrun(val4->parent == value);

    // insert self
    testrun(!array->insert(array, 3, value));
    testrun(value->parent == NULL);

    ov_json_array_free(array);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_array_count() {

    ov_json_value *value = array_creator();
    json_array *array = ov_json_array_cast(value);

    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_number(1);
    ov_json_value *val3 = ov_json_string("test");
    ov_json_value *val4 = array_creator();

    testrun(array);
    testrun(array->count);

    testrun(0 == array->count(NULL));
    testrun(0 == array->count(array));

    testrun(array->push(array, val1));
    testrun(1 == array->count(array));
    testrun(array->push(array, val2));
    testrun(2 == array->count(array));
    testrun(array->push(array, val3));
    testrun(3 == array->count(array));
    testrun(array->push(array, val4));
    testrun(4 == array->count(array));

    // count after content destroy
    ov_json_value_free(ov_json_array_pop(value));
    testrun(3 == array->count(array));
    testrun(ov_json_array_del(value, 2));
    testrun(2 == array->count(array));
    ov_json_value_free(ov_json_array_remove(value, 1));
    testrun(1 == array->count(array));

    ov_json_array_free(array);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_array_is_empty() {

    ov_json_value *value = array_creator();
    json_array *array = ov_json_array_cast(value);

    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_number(1);
    ov_json_value *val3 = ov_json_string("test");

    testrun(array);
    testrun(array->is_empty);

    testrun(!array->is_empty(NULL));
    testrun(array->is_empty(array));

    testrun(array->push(array, val1));
    testrun(array->push(array, val2));
    testrun(array->push(array, val3));
    testrun(!array->is_empty(array));

    ov_json_value_free(ov_json_array_pop(value));
    testrun(!array->is_empty(array));
    ov_json_value_free(ov_json_array_remove(value, 1));
    testrun(!array->is_empty(array));
    ov_json_value_free(ov_json_array_pop(value));
    testrun(array->is_empty(array));

    ov_json_array_free(array);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool dummy_true(void *value, void *data) {

    if (value || data) return true;

    return true;
}

/*----------------------------------------------------------------------------*/

static bool dummy_search_numbers(void *value, void *data) {

    if (!value || !data) return false;

    if (ov_json_is_number(value)) ov_list_push(data, (void *)value);

    return true;
}

/*----------------------------------------------------------------------------*/

int test_impl_json_array_for_each() {

    ov_list *list = ov_list_create((ov_list_config){0});

    ov_json_value *value = array_creator();
    json_array *array = ov_json_array_cast(value);

    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_number(1);
    ov_json_value *val3 = ov_json_string("test");

    testrun(array);
    testrun(array->for_each);

    testrun(!array->for_each(NULL, NULL, NULL));
    testrun(!array->for_each(array, NULL, NULL));
    testrun(!array->for_each(NULL, NULL, dummy_true));

    // empty with / without data
    testrun(array->for_each(array, NULL, dummy_true));
    testrun(array->for_each(array, array, dummy_true));

    testrun(array->push(array, val1));
    testrun(array->push(array, val2));
    testrun(array->push(array, val3));

    // not with / without data
    testrun(array->for_each(array, NULL, dummy_true));
    testrun(array->for_each(array, array, dummy_true));

    // delete all number values of the list
    testrun(3 == ov_json_array_count(value));
    testrun(!array->for_each(array, NULL, dummy_search_numbers));
    testrun(array->for_each(array, list, dummy_search_numbers));
    testrun(3 == ov_json_array_count(value));
    testrun(1 == ov_list_count(list));
    testrun(ov_list_clear(list));

    // multiple numbers
    testrun(array->push(array, ov_json_number(1)));
    testrun(array->push(array, ov_json_number(2)));
    testrun(array->push(array, ov_json_number(3)));
    testrun(6 == ov_json_array_count(value));
    testrun(array->for_each(array, list, dummy_search_numbers));
    testrun(6 == ov_json_array_count(value));
    testrun(4 == ov_list_count(list));
    testrun(ov_list_clear(list));

    ov_list_free(list);
    ov_json_array_free(array);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_array_remove_child() {

    ov_json_value *value = array_creator();
    json_array *array = ov_json_array_cast(value);

    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_number(1);
    ov_json_value *val3 = ov_json_string("test");

    testrun(array);
    testrun(array->remove_child);

    testrun(!array->remove_child(NULL, NULL));
    testrun(!array->remove_child(NULL, val1));
    testrun(array->remove_child(array, NULL));

    // not a parent
    testrun(0 == ov_json_array_count(value));
    testrun(val1->parent == NULL);
    testrun(array->remove_child(array, val1));
    testrun(val1->parent == NULL);
    testrun(0 == ov_json_array_count(value));

    testrun(array->push(array, val1));
    testrun(array->push(array, val2));
    testrun(array->push(array, val3));

    // parent of child
    testrun(3 == ov_json_array_count(value));
    testrun(val1->parent == value);
    testrun(array->remove_child(array, val1));
    testrun(val1->parent == NULL);
    testrun(2 == ov_json_array_count(value));

    // different parent
    val1->parent = val2;
    testrun(!array->remove_child(array, val1));
    testrun(val1->parent == val2);
    testrun(2 == ov_json_array_count(value));

    // parent set but not included
    val1->parent = value;
    testrun(2 == ov_json_array_count(value));
    testrun(array->remove_child(array, val1));
    testrun(val1->parent == NULL);
    testrun(2 == ov_json_array_count(value));

    // included but no parent set
    testrun(ov_json_array_push(value, val1));
    testrun(3 == ov_json_array_count(value));
    testrun(val1->parent == value);
    val1->parent = NULL;
    testrun(3 == ov_json_array_find(value, val1));
    testrun(array->remove_child(array, val1));
    testrun(val1->parent == NULL);
    testrun(2 == ov_json_array_count(value));
    testrun(!ov_json_array_find(value, val1));

    ov_json_value_free(val1);
    ov_json_array_free(array);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/
