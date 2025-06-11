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
        @file           ov_json_object.c
        @author         Markus Toepfer

        @date           2018-12-02

        @ingroup        ov_json_object

        @brief          Implementation of a JSON object interface.

        ------------------------------------------------------------------------
*/

#include "../../include/ov_json_object.h"

/*----------------------------------------------------------------------------*/

typedef struct json_object json_object;

/*----------------------------------------------------------------------------*/

struct json_object {

    ov_json_value head;
    size_t type;

    /*--------------------------------------------------------------------*/

    bool (*set)(json_object *self, const char *key, ov_json_value *value);

    bool (*del)(json_object *self, const char *key);

    ov_json_value *(*get)(json_object *self, const char *key);

    ov_json_value *(*remove)(json_object *self, const char *key);

    /*--------------------------------------------------------------------*/

    size_t (*count)(const json_object *self);

    bool (*is_empty)(const json_object *self);

    bool (*for_each)(json_object *self,
                     void *data,
                     bool (*function)(const void *key,
                                      void *value,
                                      void *data));

    bool (*remove_child)(json_object *self, ov_json_value *child);
};

#define AS_JSON_OBJECT(x)                                                      \
    (((ov_json_value_cast(x) != 0) &&                                          \
      (OV_JSON_OBJECT == ((ov_json_value *)x)->type))                          \
         ? (json_object *)(x)                                                  \
         : 0)

/*----------------------------------------------------------------------------*/

bool ov_json_is_object(const ov_json_value *value) {

    if (AS_JSON_OBJECT(value)) return true;

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

static json_object *set_head(json_object *self) {

    if (!self) return NULL;

    self->head.magic_byte = OV_JSON_VALUE_MAGIC_BYTE;
    self->head.type = OV_JSON_OBJECT;
    self->head.parent = NULL;

    return self;
}

/*----------------------------------------------------------------------------*/

bool ov_json_object_clear(void *data) {

    json_object *object = AS_JSON_OBJECT(data);
    if (!object || !object->head.clear) return false;

    return object->head.clear(object);
}

/*----------------------------------------------------------------------------*/

void *ov_json_object_free(void *data) {

    json_object *object = AS_JSON_OBJECT(data);
    if (!object || !object->head.free) return data;

    return object->head.free(object);
}

/*----------------------------------------------------------------------------*/

static bool copy_object_items(const void *key, void *val, void *data) {

    if (!key) return true;

    ov_json_value *value = ov_json_value_cast(val);

    if (!value || !data) return false;

    json_object *target = AS_JSON_OBJECT(data);
    ov_json_value *content = NULL;

    if (!target || !target->set) goto error;

    if (!ov_json_value_copy((void **)&content, value)) goto error;

    if (!content) goto error;

    if (target->set(target, (char *)key, content)) return true;

    content = ov_json_value_free(content);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

void *ov_json_object_copy(void **dest, const void *data) {

    bool created = false;

    json_object *copy = NULL;
    json_object *orig = AS_JSON_OBJECT(data);
    if (!dest || !orig) return NULL;

    if (*dest) {
        copy = AS_JSON_OBJECT(*dest);
        if (!copy) goto error;

    } else {

        *dest = ov_json_object();
        if (!*dest) goto error;

        created = true;
    }

    copy = AS_JSON_OBJECT(*dest);
    if (!ov_json_object_clear(copy)) goto error;

    if (!orig->for_each(orig, copy, copy_object_items)) goto error;

    return copy;
error:
    if (created) *dest = ov_json_object_free(*dest);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool dump_object_items(const void *key, void *val, void *data) {

    if (!key) return true;

    ov_json_value *value = ov_json_value_cast(val);

    if (!value || !data) return false;

    FILE *stream = (FILE *)data;

    if (!fprintf(stream, " \"%s\" :", (char *)key)) goto error;

    if (!ov_json_value_dump(stream, value)) goto error;

    if (!fprintf(stream, "\n")) goto error;

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_json_object_dump(FILE *stream, const void *data) {

    json_object *object = AS_JSON_OBJECT(data);
    if (!stream || !object) return false;

    if (!stream || !object) return false;

    if (!fprintf(stream, "\n{\n")) goto error;

    if (!object->for_each(object, stream, dump_object_items)) goto error;

    if (!fprintf(stream, "}\n")) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_data_function ov_json_object_data_functions() {

    ov_data_function f = {

        .clear = ov_json_object_clear,
        .free = ov_json_object_free,
        .dump = ov_json_object_dump,
        .copy = ov_json_object_copy};

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

#define OV_JSON_OBJECT_DICT 0x0d1d
#include "ov_dict.h"

typedef struct {

    json_object public;
    ov_dict *data;

} JsonDict;

/*----------------------------------------------------------------------------*/

#define DEFAULT_DICT_KEYS 100

#define AS_JSON_DICT(x)                                                        \
    (((ov_json_value_cast(x) != 0) &&                                          \
      (OV_JSON_OBJECT == ((ov_json_value *)x)->type) &&                        \
      (OV_JSON_OBJECT_DICT == ((json_object *)x)->type))                       \
         ? (JsonDict *)(x)                                                     \
         : 0)

/*
 *      ------------------------------------------------------------------------
 *
 *      INTERFACE IMPLEMENTATIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool impl_json_object_clear(void *self);
static void *impl_json_object_free(void *self);

/*----------------------------------------------------------------------------*/

static bool impl_json_object_set(json_object *self,
                                 const char *key,
                                 ov_json_value *value);

static bool impl_json_object_del(json_object *self, const char *key);

static ov_json_value *impl_json_object_get(json_object *self, const char *key);

static ov_json_value *impl_json_object_remove(json_object *self,
                                              const char *key);

static bool impl_json_object_for_each(json_object *self,
                                      void *data,
                                      bool (*function)(const void *key,
                                                       void *value,
                                                       void *data));

static size_t impl_json_object_count(const json_object *self);

static bool impl_json_object_remove_child(json_object *self,
                                          ov_json_value *child);

static bool impl_json_object_is_empty(const json_object *self);

/*----------------------------------------------------------------------------*/

ov_dict_config json_dict_config() {

    ov_dict_config config = ov_dict_string_key_config(DEFAULT_DICT_KEYS);
    config.value.data_function = ov_json_value_data_functions();
    return config;
}

/*----------------------------------------------------------------------------*/

bool json_dict_init(JsonDict *dict) {

    if (!dict) goto error;

    if (!set_head((json_object *)dict)) goto error;

    dict->public.type = OV_JSON_OBJECT_DICT;

    if (dict->data) dict->data = ov_dict_free(dict->data);
    dict->data = ov_dict_create(json_dict_config());

    // set interface functions

    dict->public.head.clear = impl_json_object_clear;
    dict->public.head.free = impl_json_object_free;

    dict->public.set = impl_json_object_set;
    dict->public.del = impl_json_object_del;
    dict->public.get = impl_json_object_get;
    dict->public.remove = impl_json_object_remove;
    dict->public.count = impl_json_object_count;

    dict->public.is_empty = impl_json_object_is_empty;
    dict->public.for_each = impl_json_object_for_each;
    dict->public.remove_child = impl_json_object_remove_child;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_json_object() {

    JsonDict *dict = calloc(1, sizeof(JsonDict));
    if (!dict) goto error;

    if (json_dict_init(dict)) return (ov_json_value *)dict;

    free(dict);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool impl_json_object_clear(void *self) {

    JsonDict *dict = AS_JSON_DICT(self);

    if (!dict) goto error;

    // clear content
    return ov_dict_clear(dict->data);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

void *impl_json_object_free(void *self) {

    JsonDict *dict = AS_JSON_DICT(self);
    if (!dict) goto error;

    // in case of parent loop over ov_json_value_free
    if (dict->public.head.parent) return ov_json_value_free(self);

    ov_dict_free(dict->data);
    free(dict);
    return NULL;

error:
    return self;
}

/*----------------------------------------------------------------------------*/

bool impl_json_object_set(json_object *self,
                          const char *key,
                          ov_json_value *value) {

    char *new_key = NULL;

    JsonDict *dict = AS_JSON_DICT(self);
    if (!dict || !key || !value) goto error;

    if (!ov_json_value_validate(value)) goto error;

    if (value == (ov_json_value *)self) goto error;

    new_key = strdup(key);
    if (!new_key) goto error;

    // SET PARENT WITH CHECKS
    if (!ov_json_value_set_parent(value, (ov_json_value *)self)) goto error;

    /* HERE out MUST be used to prevent
     * parent clearance override in case
     * of item replacement. (autocleanup)*/

    ov_json_value *out = NULL;
    if (ov_dict_set(dict->data, new_key, value, (void **)&out)) {

        if (out) {
            out->parent = NULL;
            out = ov_json_value_free(out);
        }

        return true;
    }

    // UNSET PARENT
    value->parent = NULL;

error:
    if (new_key) free(new_key);
    return false;
}

/*----------------------------------------------------------------------------*/

bool impl_json_object_del(json_object *self, const char *key) {

    JsonDict *dict = AS_JSON_DICT(self);
    if (!dict || !key || !dict->data) goto error;

    return ov_dict_del(dict->data, key);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *impl_json_object_get(json_object *self, const char *key) {

    JsonDict *dict = AS_JSON_DICT(self);
    if (!dict || !key || !dict->data) goto error;

    return ov_json_value_cast(ov_dict_get(dict->data, key));

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *impl_json_object_remove(json_object *self, const char *key) {

    JsonDict *dict = AS_JSON_DICT(self);
    if (!dict || !key || !dict->data) goto error;

    ov_json_value *value = ov_dict_remove(dict->data, key);
    if (value) value->parent = NULL;

    return value;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool impl_json_object_for_each(json_object *self,
                               void *data,
                               bool (*function)(const void *key,
                                                void *value,
                                                void *data)) {

    JsonDict *dict = AS_JSON_DICT(self);
    if (!dict || !function || !dict->data) goto error;

    return ov_dict_for_each(dict->data, data, function);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

size_t impl_json_object_count(const json_object *self) {

    JsonDict *dict = AS_JSON_DICT(self);
    if (!dict || !dict->data) goto error;

    return ov_dict_count(dict->data);

error:
    return 0;
}

/*----------------------------------------------------------------------------*/

bool impl_json_object_remove_child(json_object *self, ov_json_value *child) {

    if (!self) goto error;

    if (!child) return true;

    JsonDict *dict = AS_JSON_DICT(self);
    if (!dict) goto error;

    if (child->parent)
        if (child->parent != (ov_json_value *)self) goto error;

    ov_json_value *val = NULL;

    ov_list *keys = dict->data->get_keys(dict->data, child);
    if (!keys) goto error;

    if (keys->is_empty(keys)) goto done;

    void *key = keys->pop(keys);
    while (key) {

        val = dict->data->remove(dict->data, key);

        if (val) val->parent = NULL;

        key = keys->pop(keys);
    }

done:
    ov_list_free(keys);
    child->parent = NULL;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool impl_json_object_is_empty(const json_object *self) {

    JsonDict *dict = AS_JSON_DICT(self);
    if (!dict) goto error;

    return ov_dict_is_empty(dict->data);
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

bool ov_json_object_set(ov_json_value *object,
                        const char *key,
                        ov_json_value *value) {

    json_object *obj = AS_JSON_OBJECT(object);
    if (!obj || !obj->set || !key || !value) return false;

    return obj->set(obj, key, value);
}

/*----------------------------------------------------------------------------*/

bool ov_json_object_del(ov_json_value *object, const char *key) {

    json_object *obj = AS_JSON_OBJECT(object);
    if (!obj || !obj->del || !key) return false;

    return obj->del(obj, key);
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_json_object_get(const ov_json_value *object,
                                  const char *key) {

    json_object *obj = AS_JSON_OBJECT(object);
    if (!obj || !obj->get || !key) return NULL;

    return obj->get(obj, key);
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_json_object_remove(ov_json_value *object, const char *key) {

    json_object *obj = AS_JSON_OBJECT(object);
    if (!obj || !obj->remove || !key) return NULL;

    return obj->remove(obj, key);
}

/*----------------------------------------------------------------------------*/

size_t ov_json_object_count(const ov_json_value *object) {

    json_object *obj = AS_JSON_OBJECT(object);
    if (!obj || !obj->count) return 0;

    return obj->count(obj);
}

/*----------------------------------------------------------------------------*/

bool ov_json_object_is_empty(const ov_json_value *object) {

    json_object *obj = AS_JSON_OBJECT(object);
    if (!obj || !obj->is_empty) return 0;

    return obj->is_empty(obj);
}

/*----------------------------------------------------------------------------*/

bool ov_json_object_for_each(ov_json_value *object,
                             void *data,
                             bool (*function)(const void *key,
                                              void *value,
                                              void *data)) {

    json_object *obj = AS_JSON_OBJECT(object);
    if (!obj || !obj->for_each || !function) return 0;

    return obj->for_each(obj, data, function);
}

/*----------------------------------------------------------------------------*/

bool ov_json_object_remove_child(ov_json_value *object, ov_json_value *child) {

    json_object *obj = AS_JSON_OBJECT(object);
    if (!obj || !obj->remove_child || !child) return 0;

    return obj->remove_child(obj, child);
}

/*
 *      ------------------------------------------------------------------------
 *
 *      INLINE INTERFACE TEST DEFINITION
 *
 *      ------------------------------------------------------------------------
 */

ov_json_value *(*object_creator)();

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_impl_json_object_clear() {

    ov_json_value *value = object_creator();
    json_object *object = AS_JSON_OBJECT(value);

    testrun(object);
    testrun(object->head.clear);

    testrun(0 == ov_json_object_count(value));

    testrun(!object->head.clear(NULL));
    testrun(object->head.clear(object));
    testrun(0 == ov_json_object_count(value));

    testrun(ov_json_object_set(value, "v1", ov_json_true()));
    testrun(ov_json_object_set(value, "v2", ov_json_false()));
    testrun(ov_json_object_set(value, "v3", ov_json_null()));
    testrun(ov_json_object_set(value, "v4", ov_json_string("test")));
    testrun(ov_json_object_set(value, "v5", ov_json_number(5)));
    testrun(ov_json_object_set(value, "v6", ov_json_array()));
    testrun(ov_json_object_set(value, "v7", ov_json_object()));
    testrun(ov_json_object_set(value, "v8", object_creator()));

    testrun(8 == ov_json_object_count(value));
    testrun(object->head.clear(object));
    testrun(0 == ov_json_object_count(value));

    ov_json_object_free(object);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_object_free() {

    ov_json_value *value = object_creator();
    json_object *object = AS_JSON_OBJECT(value);

    testrun(object);
    testrun(object->head.free);

    testrun(0 == ov_json_object_count(value));

    testrun(NULL == object->head.free(NULL));
    testrun(NULL == object->head.free(object));

    value = object_creator();
    object = AS_JSON_OBJECT(value);

    testrun(ov_json_object_set(value, "v1", ov_json_true()));
    testrun(ov_json_object_set(value, "v2", ov_json_false()));
    testrun(ov_json_object_set(value, "v3", ov_json_null()));
    testrun(ov_json_object_set(value, "v4", ov_json_string("test")));
    testrun(ov_json_object_set(value, "v5", ov_json_number(5)));
    testrun(ov_json_object_set(value, "v6", ov_json_array()));
    testrun(ov_json_object_set(value, "v7", ov_json_object()));
    testrun(ov_json_object_set(value, "v8", object_creator()));

    testrun(8 == ov_json_object_count(value));
    testrun(NULL == object->head.free(object));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_object_set() {

    ov_json_value *value = object_creator();
    json_object *object = AS_JSON_OBJECT(value);

    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_number(1);
    ov_json_value *val3 = ov_json_string("test");

    testrun(object);
    testrun(object->set);

    testrun(!object->set(NULL, NULL, NULL));
    testrun(!object->set(object, "key", NULL));
    testrun(!object->set(object, NULL, val1));
    testrun(!object->set(NULL, "key", val1));

    testrun(0 == ov_json_object_count(value));
    testrun(object->set(object, "key", val1));
    testrun(1 == ov_json_object_count(value));
    testrun(val1 == ov_json_object_get(value, "key"));

    // override
    testrun(object->set(object, "key", val2));
    testrun(1 == ov_json_object_count(value));
    testrun(val2 == ov_json_object_get(value, "key"));

    val1 = ov_json_true();

    // TYPE WRONG
    val1->type = NOT_JSON;
    testrun(!object->set(object, "key", val1));
    testrun(1 == ov_json_object_count(value));
    testrun(val2 == ov_json_object_get(value, "key"));
    val1->type = OV_JSON_TRUE;

    // MAGIC BYTE WRONG
    val1->magic_byte = 0;
    testrun(!object->set(object, "key", val1));
    testrun(1 == ov_json_object_count(value));
    testrun(val2 == ov_json_object_get(value, "key"));
    val1->magic_byte = OV_JSON_VALUE_MAGIC_BYTE;
    testrun(object->set(object, "key", val1));
    testrun(1 == ov_json_object_count(value));
    testrun(val1 == ov_json_object_get(value, "key"));

    // try to set self
    testrun(!object->set(object, "self", value));
    testrun(1 == ov_json_object_count(value));
    testrun(val1 == ov_json_object_get(value, "key"));

    // try to set with parent
    val2 = ov_json_true();
    val2->parent = val3;
    testrun(!object->set(object, "val2", val2));
    testrun(1 == ov_json_object_count(value));
    testrun(val1 == ov_json_object_get(value, "key"));
    val2->parent = NULL;
    testrun(object->set(object, "val2", val2));
    testrun(2 == ov_json_object_count(value));
    testrun(val1 == ov_json_object_get(value, "key"));
    testrun(val2 == ov_json_object_get(value, "val2"));
    testrun(object->set(object, "val3", val3));
    testrun(3 == ov_json_object_count(value));
    testrun(val1 == ov_json_object_get(value, "key"));
    testrun(val2 == ov_json_object_get(value, "val2"));
    testrun(val3 == ov_json_object_get(value, "val3"));

    // Same key
    testrun(object->set(object, "key", ov_json_true()));
    testrun(object->set(object, "key", ov_json_true()));
    // Case sensitive
    testrun(object->set(object, "Key", ov_json_true()));
    testrun(4 == ov_json_object_count(value));

    // key is whitespace sensitive
    testrun(object->set(object, "key ", ov_json_true()));
    testrun(object->set(object, " Key", ov_json_true()));
    testrun(6 == ov_json_object_count(value));

    ov_json_object_free(object);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_object_del() {

    ov_json_value *value = object_creator();
    json_object *object = AS_JSON_OBJECT(value);

    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_number(1);
    ov_json_value *val3 = ov_json_string("test");

    testrun(object);
    testrun(object->del);

    testrun(!object->del(NULL, NULL));
    testrun(!object->del(object, NULL));
    testrun(!object->del(NULL, "key"));

    // not set
    testrun(0 == ov_json_object_count(value));
    testrun(object->del(object, "key"));

    // set
    testrun(object->set(object, "key", val1));
    testrun(1 == ov_json_object_count(value));
    testrun(value == val1->parent);
    testrun(object->del(object, "key"));
    testrun(0 == ov_json_object_count(value));

    testrun(object->set(object, "key", val2));
    testrun(object->set(object, "Key", val3));
    testrun(value == val2->parent);
    testrun(value == val3->parent);
    testrun(2 == ov_json_object_count(value));
    testrun(ov_json_object_get(value, "Key"));
    testrun(ov_json_object_get(value, "key"));
    testrun(object->del(object, "key"));
    testrun(1 == ov_json_object_count(value));
    testrun(ov_json_object_get(value, "Key"));
    testrun(!ov_json_object_get(value, "key"));

    ov_json_object_free(object);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_object_get() {

    ov_json_value *value = object_creator();
    json_object *object = AS_JSON_OBJECT(value);

    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_number(1);
    ov_json_value *val3 = ov_json_string("test");

    testrun(object);
    testrun(object->get);

    testrun(!object->get(NULL, NULL));
    testrun(!object->get(object, NULL));
    testrun(!object->get(NULL, "key"));

    // not set
    testrun(0 == ov_json_object_count(value));
    testrun(!object->get(object, "key"));

    // set
    testrun(object->set(object, "key", val1));
    testrun(1 == ov_json_object_count(value));
    testrun(val1 == object->get(object, "key"));
    testrun(object->set(object, "key", val2));
    testrun(object->set(object, "Key", val3));
    testrun(2 == ov_json_object_count(value));
    testrun(val2 == object->get(object, "key"));
    testrun(val3 == object->get(object, "Key"));

    // whitespace sensitive
    testrun(NULL == object->get(object, " key"));
    testrun(NULL == object->get(object, "Key "));

    ov_json_object_free(object);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_object_remove() {

    ov_json_value *value = object_creator();
    json_object *object = AS_JSON_OBJECT(value);

    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_number(1);
    ov_json_value *val3 = ov_json_string("test");

    testrun(object);
    testrun(object->remove);

    testrun(!object->remove(NULL, NULL));
    testrun(!object->remove(object, NULL));
    testrun(!object->remove(NULL, "key"));

    // not set
    testrun(0 == ov_json_object_count(value));
    testrun(!object->remove(object, "key"));

    // set
    testrun(object->set(object, "key", val1));
    testrun(value == val1->parent);
    testrun(1 == ov_json_object_count(value));
    testrun(val1 == object->remove(object, "key"));
    testrun(0 == ov_json_object_count(value));
    testrun(NULL == val1->parent);

    testrun(object->set(object, "key", val2));
    testrun(object->set(object, "Key", val3));
    testrun(value == val2->parent);
    testrun(value == val3->parent);
    testrun(2 == ov_json_object_count(value));
    testrun(val2 == object->remove(object, "key"));
    testrun(val3 == object->remove(object, "Key"));
    testrun(NULL == val2->parent);
    testrun(NULL == val3->parent);

    ov_json_object_free(object);
    ov_json_value_free(val1);
    ov_json_value_free(val2);
    ov_json_value_free(val3);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_object_count() {

    ov_json_value *value = object_creator();
    json_object *object = AS_JSON_OBJECT(value);

    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_number(1);
    ov_json_value *val3 = ov_json_string("test");
    ov_json_value *val4 = object_creator();
    ov_json_value *val5 = ov_json_array();

    testrun(object);
    testrun(object->count);

    testrun(0 == object->count(NULL));
    testrun(0 == object->count(object));

    testrun(ov_json_object_set(value, "1", val1));
    testrun(1 == object->count(object));
    testrun(ov_json_object_set(value, "2", val2));
    testrun(2 == object->count(object));
    testrun(ov_json_object_set(value, "3", val3));
    testrun(3 == object->count(object));
    testrun(ov_json_object_set(value, "4", val4));
    testrun(4 == object->count(object));
    testrun(ov_json_object_set(value, "5", val5));
    testrun(5 == object->count(object));
    testrun(ov_json_object_set(value, "6", ov_json_true()));
    testrun(6 == object->count(object));

    // only count same level
    testrun(6 == object->count(object));
    testrun(ov_json_object_set(val4, "1", ov_json_true()));
    testrun(ov_json_object_set(val4, "2", ov_json_true()));
    testrun(ov_json_object_set(val4, "3", ov_json_true()));
    testrun(ov_json_object_set(val4, "4", ov_json_true()));
    testrun(6 == object->count(object));
    testrun(4 == object->count(AS_JSON_OBJECT(val4)));
    testrun(ov_json_array_push(val5, ov_json_true()));
    testrun(ov_json_array_push(val5, ov_json_true()));
    testrun(ov_json_array_push(val5, ov_json_true()));
    testrun(6 == object->count(object));
    testrun(3 == ov_json_array_count(val5));

    testrun(ov_json_object_del(value, "1"));
    testrun(5 == object->count(object));
    testrun(ov_json_object_del(value, "2"));
    testrun(4 == object->count(object));
    testrun(ov_json_object_del(value, "3"));
    testrun(3 == object->count(object));
    testrun(ov_json_object_del(value, "4"));
    testrun(2 == object->count(object));
    testrun(ov_json_object_del(value, "5"));
    testrun(1 == object->count(object));
    testrun(ov_json_object_del(value, "6"));
    testrun(0 == object->count(object));

    ov_json_object_free(object);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_object_is_empty() {

    ov_json_value *value = object_creator();
    json_object *object = AS_JSON_OBJECT(value);

    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_number(1);
    ov_json_value *val3 = ov_json_string("test");
    ov_json_value *val4 = object_creator();
    ov_json_value *val5 = ov_json_array();

    testrun(object);
    testrun(object->is_empty);

    testrun(!object->is_empty(NULL));
    testrun(object->is_empty(object));

    testrun(ov_json_object_set(value, "1", val1));
    testrun(!object->is_empty(object));
    testrun(ov_json_object_set(value, "2", val2));
    testrun(!object->is_empty(object));
    testrun(ov_json_object_set(value, "3", val3));
    testrun(!object->is_empty(object));
    testrun(ov_json_object_set(value, "4", val4));
    testrun(!object->is_empty(object));
    testrun(ov_json_object_set(value, "5", val5));
    testrun(!object->is_empty(object));
    testrun(ov_json_object_set(value, "6", ov_json_true()));
    testrun(!object->is_empty(object));

    testrun(object->is_empty(AS_JSON_OBJECT(val4)));

    testrun(ov_json_object_del(value, "1"));
    testrun(!object->is_empty(object));
    testrun(ov_json_object_del(value, "2"));
    testrun(!object->is_empty(object));
    testrun(ov_json_object_del(value, "3"));
    testrun(!object->is_empty(object));
    testrun(ov_json_object_del(value, "4"));
    testrun(!object->is_empty(object));
    testrun(ov_json_object_del(value, "5"));
    testrun(!object->is_empty(object));
    testrun(ov_json_object_del(value, "6"));
    testrun(object->is_empty(object));

    ov_json_object_free(object);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool dummy_run(const void *key, void *value, void *data) {

    if (!key) return true;

    if (!value || !data) return true;

    return true;
}

/*----------------------------------------------------------------------------*/

static bool dummy_count_numbers(const void *key, void *value, void *data) {

    if (!key) return true;

    if (!value || !data) return false;

    if (ov_json_is_number(value)) ov_list_push(data, value);

    return true;
}

/*----------------------------------------------------------------------------*/

int test_impl_json_object_for_each() {

    ov_list *list = ov_list_create((ov_list_config){0});

    ov_json_value *value = object_creator();
    json_object *object = AS_JSON_OBJECT(value);

    testrun(object);
    testrun(object->for_each);

    testrun(!object->for_each(NULL, NULL, NULL));
    testrun(!object->for_each(object, object, NULL));
    testrun(!object->for_each(NULL, object, dummy_run));
    testrun(!object->for_each(object, object, NULL));

    // no data no content
    testrun(object->for_each(object, NULL, dummy_run));

    ov_json_object_set(value, "1", ov_json_true());
    ov_json_object_set(value, "2", ov_json_string("test"));
    ov_json_object_set(value, "3", ov_json_array());
    ov_json_object_set(value, "4", object_creator());
    ov_json_object_set(value, "5", ov_json_number(1));
    testrun(object->for_each(object, NULL, dummy_run));

    testrun(!object->for_each(object, NULL, dummy_count_numbers));
    testrun(0 == ov_list_count(list));
    testrun(object->for_each(object, list, dummy_count_numbers));
    testrun(1 == ov_list_count(list));
    testrun(object->for_each(object, list, dummy_count_numbers));
    testrun(2 == ov_list_count(list));
    testrun(object->for_each(object, list, dummy_count_numbers));
    testrun(3 == ov_list_count(list));
    ov_list_clear(list);

    ov_json_object_set(value, "6", ov_json_number(2));
    ov_json_object_set(value, "7", ov_json_number(3));
    ov_json_object_set(value, "8", ov_json_number(4));
    ov_json_object_set(value, "9", ov_json_number(5));
    testrun(object->for_each(object, list, dummy_count_numbers));
    testrun(5 == ov_list_count(list));

    ov_list_free(list);
    ov_json_object_free(object);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_json_object_remove_child() {

    ov_json_value *value = object_creator();
    json_object *object = AS_JSON_OBJECT(value);

    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_number(1);

    testrun(object);
    testrun(object->remove_child);

    testrun(!object->remove_child(NULL, NULL));
    testrun(!object->remove_child(NULL, val1));

    // not a child
    testrun(object->remove_child(object, NULL));
    testrun(object->remove_child(object, val1));

    ov_json_object_set(value, "1", val1);
    ov_json_object_set(value, "2", val2);
    testrun(2 == ov_json_object_count(value));
    testrun(val1->parent == value);
    testrun(val2->parent == value);

    testrun(object->remove_child(object, val1));
    testrun(1 == ov_json_object_count(value));
    testrun(val1->parent == NULL);
    testrun(object->remove_child(object, val2));
    testrun(0 == ov_json_object_count(value));
    testrun(val2->parent == NULL);

    // child with different parent
    ov_json_object_set(value, "1", val1);
    ov_json_object_set(value, "2", val2);
    testrun(2 == ov_json_object_count(value));
    testrun(val1->parent == value);
    testrun(val2->parent == value);
    val2->parent = val1;
    testrun(object->remove_child(object, val1));
    testrun(1 == ov_json_object_count(value));
    testrun(val1->parent == NULL);
    testrun(!object->remove_child(object, val2));
    testrun(1 == ov_json_object_count(value));
    testrun(val2->parent == val1);

    // no parent relationship but included
    val2->parent = NULL;
    testrun(1 == ov_json_object_count(value));
    testrun(object->remove_child(object, val2));
    testrun(0 == ov_json_object_count(value));
    testrun(val2->parent == NULL);

    // not included but parent relationship
    val2->parent = value;
    testrun(object->remove_child(object, val2));
    testrun(0 == ov_json_object_count(value));
    testrun(val2->parent == NULL);

    ov_json_object_free(object);
    ov_json_value_free(val1);
    ov_json_value_free(val2);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/
