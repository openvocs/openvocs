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
        @file           ov_json_array_test.c
        @author         Markus Toepfer

        @date           2018-12-02

        @ingroup        ov_json_array

        @brief          Unit tests of json array.


        ------------------------------------------------------------------------
*/

#include "ov_json_array.c"
#include <ov_test/testrun.h>

/*----------------------------------------------------------------------------*/

int test_ov_json_array_data_functions() {

    ov_data_function f = ov_json_array_data_functions();

    testrun(f.clear == ov_json_array_clear);
    testrun(f.free == ov_json_array_free);
    testrun(f.copy == ov_json_array_copy);
    testrun(f.dump == ov_json_array_dump);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_array() {

    ov_json_value *array = ov_json_array();
    testrun(array);
    testrun(AS_JSON_ARRAY(array));
    testrun(AS_JSON_LIST(array));

    ov_json_array_free(array);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_is_array() {

    ov_json_value value = {0};

    for (uint16_t i = 0; i < 0xffff; i++) {

        value.magic_byte = i;
        value.type = OV_JSON_ARRAY;

        if (i == OV_JSON_VALUE_MAGIC_BYTE) {

            testrun(ov_json_is_array(&value));

            value.type = OV_JSON_OBJECT;
            testrun(!ov_json_is_array(&value));
            value.type = NOT_JSON;
            testrun(!ov_json_is_array(&value));

        } else {
            testrun(!ov_json_is_array(&value));
        }
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_array_set_head() {

    ov_json_value *value = ov_json_array();

    json_array *array = AS_JSON_ARRAY(value);

    testrun(array);
    testrun(AS_JSON_LIST(value));

    array->head.magic_byte = 0;
    array->head.type = 0;
    array->head.parent = (ov_json_value *)array;

    testrun(ov_json_array_set_head(array));
    testrun(array->head.magic_byte == OV_JSON_VALUE_MAGIC_BYTE);
    testrun(array->head.type == OV_JSON_ARRAY);
    testrun(array->head.parent == NULL);

    testrun(NULL == ov_json_array_set_head(NULL));

    ov_json_array_free(array);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool dummy_clear(void *data) {

    if (!data)
        return false;
    return true;
}

/*----------------------------------------------------------------------------*/

int test_ov_json_array_clear() {

    ov_json_value *val = ov_json_array();
    testrun(0 != val);

    testrun(!ov_json_array_clear(NULL));
    testrun(ov_json_array_clear(val));

    json_array *arr = AS_JSON_ARRAY(val);

    // wrong type
    arr->head.magic_byte = 0;
    testrun(!ov_json_array_clear(val));
    arr->head.magic_byte = OV_JSON_VALUE_MAGIC_BYTE;
    testrun(ov_json_array_clear(val));

    arr->head.type = NOT_JSON;
    testrun(!ov_json_array_clear(val));
    arr->head.type = OV_JSON_OBJECT;
    testrun(!ov_json_array_clear(val));
    arr->head.type = OV_JSON_ARRAY;
    testrun(ov_json_array_clear(val));

    // no self.clear
    arr->head.clear = NULL;
    testrun(!ov_json_array_clear(val));
    arr->head.clear = dummy_clear;
    testrun(ov_json_array_clear(val));

    // reset clear for termination
    arr->head.clear = impl_json_array_clear;
    ov_json_array_free(val);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static void *dummy_free(void *data) {

    if (!data)
        return NULL;
    return NULL;
}

/*----------------------------------------------------------------------------*/

int test_ov_json_array_free() {

    ov_json_value *val = ov_json_array();

    testrun(NULL == ov_json_array_free(NULL));

    // wrong type
    json_array *arr = AS_JSON_ARRAY(val);

    arr = ov_json_array_cast(ov_json_array());
    arr->head.magic_byte = 0;
    testrun(arr == ov_json_array_free(arr));
    arr->head.magic_byte = OV_JSON_VALUE_MAGIC_BYTE;
    testrun(NULL == ov_json_array_free(arr));

    arr = ov_json_array_cast(ov_json_array());
    arr->head.type = NOT_JSON;
    testrun(arr == ov_json_array_free(arr));
    arr->head.type = OV_JSON_OBJECT;
    testrun(arr == ov_json_array_free(arr));
    arr->head.type = OV_JSON_ARRAY;
    testrun(NULL == ov_json_array_free(arr));

    // no self.free
    arr = ov_json_array_cast(ov_json_array());
    arr->head.free = NULL;
    testrun(arr == ov_json_array_free(arr));
    arr->head.free = dummy_free;
    testrun(NULL == ov_json_array_free(arr));

    // reset clear for termination
    arr->head.free = impl_json_array_free;
    testrun(NULL == ov_json_array_free(arr));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_array_copy() {

    ov_json_value *val = ov_json_array();
    ov_json_value *copy = 0;

    testrun(!ov_json_array_copy(NULL, NULL));
    testrun(!ov_json_array_copy(NULL, val));
    testrun(!ov_json_array_copy((void **)&copy, NULL));

    testrun(ov_json_array_copy((void **)&copy, val));
    testrun(copy);
    testrun(AS_JSON_LIST(copy));

    json_array *arr = AS_JSON_ARRAY(val);
    // wrong type
    arr->head.magic_byte = 0;
    testrun(!ov_json_array_copy((void **)&copy, arr));
    arr->head.magic_byte = OV_JSON_VALUE_MAGIC_BYTE;
    testrun(ov_json_array_copy((void **)&copy, arr));

    // with content
    testrun(arr->push(arr, ov_json_true()));
    testrun(arr->push(arr, ov_json_false()));
    testrun(arr->count(arr) == 2);

    testrun(ov_json_array_copy((void **)&copy, arr));
    testrun(copy);
    testrun(ov_json_array_count(copy) == 2);
    testrun(ov_json_is_true(ov_json_array_get(copy, 1)));
    testrun(ov_json_is_false(ov_json_array_get(copy, 2)));

    ov_json_value_free(arr);
    ov_json_value_free(copy);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_array_dump() {

    ov_json_value *val = ov_json_array();

    testrun(!ov_json_array_dump(NULL, NULL));
    testrun(!ov_json_array_dump(NULL, val));
    testrun(!ov_json_array_dump(stdout, NULL));

    testrun(ov_json_array_dump(stdout, val));

    // wrong type
    json_array *arr = AS_JSON_ARRAY(val);

    arr->head.magic_byte = 0;
    testrun(!ov_json_array_dump(stdout, arr));
    arr->head.magic_byte = OV_JSON_VALUE_MAGIC_BYTE;
    testrun(ov_json_array_dump(stdout, arr));

    // add some content

    testrun(arr->push(arr, ov_json_number(123)));
    testrun(arr->push(arr, ov_json_number(456)));
    testrun(arr->push(arr, ov_json_string("test")));

    testrun(ov_json_array_dump(stdout, arr));

    ov_json_array_free(arr);
    return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      VALUE BASED INTERFACE FUNCTION CALLS
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_json_array_push() {

    ov_json_value *array = ov_json_array();
    ov_json_value *value = ov_json_true();

    testrun(!ov_json_array_push(NULL, NULL));
    testrun(!ov_json_array_push(array, NULL));
    testrun(!ov_json_array_push(NULL, value));

    // wrong type
    array->type = OV_JSON_OBJECT;
    testrun(!ov_json_array_push(array, value));
    array->type = OV_JSON_ARRAY;
    testrun(0 == ov_json_array_count(array));
    testrun(ov_json_array_push(array, value));
    testrun(1 == ov_json_array_count(array));

    // function not available
    json_array *arr = AS_JSON_ARRAY(array);

    value = ov_json_true();
    arr->push = NULL;
    testrun(!ov_json_array_push(array, value));
    arr->push = impl_json_array_push;
    testrun(1 == ov_json_array_count(array));
    testrun(ov_json_array_push(array, value));
    testrun(2 == ov_json_array_count(array));

    ov_json_value_free(array);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_array_pop() {

    ov_json_value *array = ov_json_array();
    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_true();
    ov_json_value *val3 = ov_json_true();

    testrun(ov_json_array_push(array, val1));
    testrun(ov_json_array_push(array, val2));
    testrun(ov_json_array_push(array, val3));

    testrun(ov_json_array_pop(array));

    // wrong type

    array->type = OV_JSON_OBJECT;
    testrun(!ov_json_array_pop(array));
    array->type = OV_JSON_ARRAY;
    testrun(2 == ov_json_array_count(array));
    testrun(ov_json_array_pop(array));
    testrun(1 == ov_json_array_count(array));

    // function not available
    json_array *arr = AS_JSON_ARRAY(array);

    arr->pop = NULL;
    testrun(!ov_json_array_pop(array));
    arr->pop = impl_json_array_pop;
    testrun(1 == ov_json_array_count(array));
    testrun(ov_json_array_pop(array));
    testrun(0 == ov_json_array_count(array));

    // nothing to pop
    testrun(!ov_json_array_pop(array));

    ov_json_value_free(array);
    ov_json_value_free(val1);
    ov_json_value_free(val2);
    ov_json_value_free(val3);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_array_find() {

    ov_json_value *array = ov_json_array();
    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_true();
    ov_json_value *val3 = ov_json_true();

    testrun(ov_json_array_push(array, val1));
    testrun(ov_json_array_push(array, val2));

    testrun(0 == ov_json_array_find(NULL, NULL));
    testrun(0 == ov_json_array_find(array, NULL));
    testrun(0 == ov_json_array_find(NULL, val1));

    testrun(1 == ov_json_array_find(array, val1));
    testrun(2 == ov_json_array_find(array, val2));
    testrun(0 == ov_json_array_find(array, val3));

    // wrong type
    array->type = OV_JSON_OBJECT;
    testrun(0 == ov_json_array_find(array, val1));
    testrun(0 == ov_json_array_find(array, val2));
    testrun(0 == ov_json_array_find(array, val3));
    array->type = OV_JSON_ARRAY;
    testrun(1 == ov_json_array_find(array, val1));
    testrun(2 == ov_json_array_find(array, val2));
    testrun(0 == ov_json_array_find(array, val3));

    // function not available
    json_array *arr = AS_JSON_ARRAY(array);

    arr->find = NULL;
    testrun(0 == ov_json_array_find(array, val1));
    testrun(0 == ov_json_array_find(array, val2));
    testrun(0 == ov_json_array_find(array, val3));
    arr->find = impl_json_array_find;
    testrun(1 == ov_json_array_find(array, val1));
    testrun(2 == ov_json_array_find(array, val2));
    testrun(0 == ov_json_array_find(array, val3));

    ov_json_array_free(array);
    ov_json_value_free(val3);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_array_del() {

    ov_json_value *array = ov_json_array();
    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_true();
    ov_json_value *val3 = ov_json_true();

    testrun(ov_json_array_push(array, val1));
    testrun(ov_json_array_push(array, val2));
    testrun(ov_json_array_push(array, val3));

    testrun(!ov_json_array_del(NULL, 0));
    testrun(!ov_json_array_del(array, 0));
    testrun(!ov_json_array_del(NULL, 1));

    testrun(1 == ov_json_array_find(array, val1));
    testrun(2 == ov_json_array_find(array, val2));
    testrun(3 == ov_json_array_find(array, val3));

    // wrong type
    array->type = OV_JSON_OBJECT;
    testrun(!ov_json_array_del(array, 1));
    array->type = OV_JSON_ARRAY;
    testrun(ov_json_array_del(array, 1));
    testrun(0 == ov_json_array_find(array, val1));
    testrun(1 == ov_json_array_find(array, val2));
    testrun(2 == ov_json_array_find(array, val3));

    // function not available
    json_array *arr = AS_JSON_ARRAY(array);

    arr->del = NULL;
    testrun(!ov_json_array_del(array, 1));
    arr->del = impl_json_array_del;
    testrun(ov_json_array_del(array, 1));
    testrun(0 == ov_json_array_find(array, val1));
    testrun(0 == ov_json_array_find(array, val2));
    testrun(1 == ov_json_array_find(array, val3));

    ov_json_array_free(array);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_array_get() {

    ov_json_value *array = ov_json_array();
    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_true();
    ov_json_value *val3 = ov_json_true();

    testrun(ov_json_array_push(array, val1));
    testrun(ov_json_array_push(array, val2));

    testrun(!ov_json_array_get(NULL, 0));
    testrun(!ov_json_array_get(array, 0));
    testrun(!ov_json_array_get(NULL, 1));

    testrun(val1 == ov_json_array_get(array, 1));
    testrun(val2 == ov_json_array_get(array, 2));
    testrun(NULL == ov_json_array_get(array, 3));

    // wrong type
    array->type = OV_JSON_OBJECT;
    testrun(NULL == ov_json_array_get(array, 1));
    testrun(NULL == ov_json_array_get(array, 2));
    testrun(NULL == ov_json_array_get(array, 3));
    array->type = OV_JSON_ARRAY;
    testrun(val1 == ov_json_array_get(array, 1));
    testrun(val2 == ov_json_array_get(array, 2));
    testrun(NULL == ov_json_array_get(array, 3));

    // function not available
    json_array *arr = AS_JSON_ARRAY(array);

    arr->get = NULL;
    testrun(NULL == ov_json_array_get(array, 1));
    testrun(NULL == ov_json_array_get(array, 2));
    testrun(NULL == ov_json_array_get(array, 3));

    arr->get = impl_json_array_get;
    testrun(val1 == ov_json_array_get(array, 1));
    testrun(val2 == ov_json_array_get(array, 2));
    testrun(NULL == ov_json_array_get(array, 3));

    ov_json_array_free(array);
    ov_json_value_free(val3);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_array_insert() {

    ov_json_value *array = ov_json_array();
    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_true();
    ov_json_value *val3 = ov_json_true();

    testrun(!ov_json_array_insert(NULL, 1, val1));
    testrun(!ov_json_array_insert(array, 0, val2));
    testrun(!ov_json_array_insert(array, 1, NULL));
    testrun(!ov_json_array_insert(array, 100, val2));

    testrun(ov_json_array_insert(array, 1, val1));

    testrun(val1 == ov_json_array_get(array, 1));
    testrun(NULL == ov_json_array_get(array, 2));
    testrun(NULL == ov_json_array_get(array, 3));

    // wrong type
    array->type = OV_JSON_OBJECT;
    testrun(!ov_json_array_insert(array, 1, val2));
    array->type = OV_JSON_ARRAY;
    testrun(ov_json_array_insert(array, 1, val2));
    testrun(val1 == ov_json_array_get(array, 2));
    testrun(val2 == ov_json_array_get(array, 1));
    testrun(NULL == ov_json_array_get(array, 3));

    json_array *arr = AS_JSON_ARRAY(array);

    // function not available
    arr->insert = NULL;
    testrun(!ov_json_array_insert(array, 1, val3));
    arr->insert = impl_json_array_insert;
    testrun(ov_json_array_insert(array, 1, val3));
    testrun(val3 == ov_json_array_get(array, 1));
    testrun(val2 == ov_json_array_get(array, 2));
    testrun(val1 == ov_json_array_get(array, 3));

    ov_json_array_free(array);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_array_remove() {

    ov_json_value *array = ov_json_array();
    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_true();
    ov_json_value *val3 = ov_json_true();

    testrun(ov_json_array_push(array, val1));
    testrun(ov_json_array_push(array, val2));
    testrun(ov_json_array_push(array, val3));

    testrun(!ov_json_array_remove(NULL, 0));
    testrun(!ov_json_array_remove(array, 0));
    testrun(!ov_json_array_remove(NULL, 1));

    testrun(val1->parent == array);
    testrun(3 == ov_json_array_count(array));
    testrun(val1 == ov_json_array_remove(array, 1));
    testrun(val1->parent == NULL);
    testrun(2 == ov_json_array_count(array));

    // wrong type
    array->type = OV_JSON_OBJECT;
    testrun(NULL == ov_json_array_remove(array, 1));
    array->type = OV_JSON_ARRAY;
    testrun(val2 == ov_json_array_remove(array, 1));

    json_array *arr = AS_JSON_ARRAY(array);

    // function not available
    arr->remove = NULL;
    testrun(NULL == ov_json_array_remove(array, 1));
    arr->remove = impl_json_array_remove;
    testrun(val3 == ov_json_array_remove(array, 1));

    ov_json_array_free(array);
    ov_json_value_free(val1);
    ov_json_value_free(val2);
    ov_json_value_free(val3);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_array_count() {

    ov_json_value *array = ov_json_array();
    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_true();
    ov_json_value *val3 = ov_json_true();

    testrun(0 == ov_json_array_count(NULL));
    testrun(0 == ov_json_array_count(array));

    testrun(ov_json_array_push(array, val1));
    testrun(ov_json_array_push(array, val2));
    testrun(ov_json_array_push(array, val3));

    testrun(3 == ov_json_array_count(array));

    // wrong type
    array->type = OV_JSON_OBJECT;
    testrun(0 == ov_json_array_count(array));
    array->type = OV_JSON_ARRAY;
    testrun(3 == ov_json_array_count(array));

    json_array *arr = AS_JSON_ARRAY(array);

    // function not available
    arr->count = NULL;
    testrun(0 == ov_json_array_count(array));
    arr->count = impl_json_array_count;
    testrun(3 == ov_json_array_count(array));

    ov_json_array_free(array);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_array_is_empty() {

    ov_json_value *array = ov_json_array();
    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_true();
    ov_json_value *val3 = ov_json_true();

    testrun(!ov_json_array_is_empty(NULL));
    testrun(ov_json_array_is_empty(array));

    testrun(ov_json_array_push(array, val1));
    testrun(ov_json_array_push(array, val2));
    testrun(ov_json_array_push(array, val3));

    testrun(!ov_json_array_is_empty(array));

    testrun(ov_json_array_pop(array));
    testrun(ov_json_array_pop(array));
    testrun(ov_json_array_pop(array));

    testrun(ov_json_array_is_empty(array));

    // wrong type
    array->type = OV_JSON_OBJECT;
    testrun(!ov_json_array_is_empty(array));
    array->type = OV_JSON_ARRAY;
    testrun(ov_json_array_is_empty(array));

    // function not available
    json_array *arr = AS_JSON_ARRAY(array);

    arr->is_empty = NULL;
    testrun(!ov_json_array_is_empty(array));
    arr->is_empty = impl_json_array_is_empty;
    testrun(ov_json_array_is_empty(array));

    ov_json_array_free(array);
    ov_json_value_free(val1);
    ov_json_value_free(val2);
    ov_json_value_free(val3);

    return testrun_log_success();
}
/*----------------------------------------------------------------------------*/

static bool dummy_for_each(void *item, void *data) {

    if (item || data)
        return true;
    return true;
}

/*----------------------------------------------------------------------------*/

int test_ov_json_array_for_each() {

    ov_json_value *array = ov_json_array();
    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_true();
    ov_json_value *val3 = ov_json_true();

    testrun(!ov_json_array_for_each(NULL, NULL, NULL));
    testrun(!ov_json_array_for_each(array, NULL, NULL));
    testrun(!ov_json_array_for_each(NULL, NULL, dummy_for_each));

    testrun(ov_json_array_for_each(array, NULL, dummy_for_each));

    testrun(ov_json_array_push(array, val1));
    testrun(ov_json_array_push(array, val2));
    testrun(ov_json_array_push(array, val3));

    testrun(ov_json_array_for_each(array, NULL, dummy_for_each));

    // wrong type
    array->type = OV_JSON_OBJECT;
    testrun(!ov_json_array_for_each(array, NULL, dummy_for_each));
    array->type = OV_JSON_ARRAY;
    testrun(ov_json_array_for_each(array, NULL, dummy_for_each));

    json_array *arr = AS_JSON_ARRAY(array);
    // function not available
    arr->for_each = NULL;
    testrun(!ov_json_array_for_each(array, NULL, dummy_for_each));
    arr->for_each = impl_json_array_for_each;
    testrun(ov_json_array_for_each(array, NULL, dummy_for_each));

    ov_json_array_free(array);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_array_remove_child() {

    ov_json_value *array2 = ov_json_array();
    ov_json_value *array = ov_json_array();
    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_true();
    ov_json_value *val3 = ov_json_true();

    testrun(ov_json_array_push(array, val1));
    testrun(ov_json_array_push(array, val2));
    testrun(ov_json_array_push(array, val3));

    testrun(!ov_json_array_remove_child(NULL, NULL));
    testrun(!ov_json_array_remove_child(array, NULL));
    testrun(!ov_json_array_remove_child(NULL, val1));

    testrun(val1->parent == array);
    testrun(3 == ov_json_array_count(array));
    testrun(ov_json_array_remove_child(array, val1));
    testrun(val1->parent == NULL);
    testrun(2 == ov_json_array_count(array));

    // wrong type
    array->type = OV_JSON_OBJECT;
    testrun(!ov_json_array_remove_child(array, val2));
    array->type = OV_JSON_ARRAY;
    testrun(val2->parent == array);
    testrun(2 == ov_json_array_count(array));
    testrun(ov_json_array_remove_child(array, val2));
    testrun(val2->parent == NULL);
    testrun(1 == ov_json_array_count(array));

    json_array *arr = AS_JSON_ARRAY(array);
    // function not available
    arr->remove_child = NULL;
    testrun(!ov_json_array_remove_child(array, val3));
    arr->remove_child = impl_json_array_remove_child;
    testrun(ov_json_array_remove_child(array, val3));
    testrun(val3->parent == NULL);
    testrun(0 == ov_json_array_count(array));

    testrun(val1->parent == NULL);
    testrun(val2->parent == NULL);
    testrun(val3->parent == NULL);

    // remove from wrong array
    testrun(ov_json_array_push(array2, val1));
    testrun(ov_json_array_push(array2, val2));
    testrun(ov_json_array_push(array, val3));

    testrun(!ov_json_array_remove_child(array, val1));
    testrun(!ov_json_array_remove_child(array, val2));
    testrun(!ov_json_array_remove_child(array2, val3));

    testrun(2 == ov_json_array_count(array2));
    testrun(1 == ov_json_array_count(array));
    testrun(val1->parent == array2);
    testrun(val2->parent == array2);
    testrun(val3->parent == array);
    testrun(ov_json_array_get(array, 1) == val3);
    testrun(ov_json_array_get(array2, 1) == val1);
    testrun(ov_json_array_get(array2, 2) == val2);

    ov_json_array_free(array);
    ov_json_array_free(array2);

    return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

    testrun_init();

    // generic functions

    testrun_test(test_ov_json_array_data_functions);
    testrun_test(test_ov_json_is_array);
    testrun_test(test_ov_json_array);
    testrun_test(test_ov_json_array_set_head);

    testrun_test(test_ov_json_array_clear);
    testrun_test(test_ov_json_array_free);
    testrun_test(test_ov_json_array_copy);
    testrun_test(test_ov_json_array_dump);

    // interface tests

    testrun_test(test_ov_json_array_push);
    testrun_test(test_ov_json_array_pop);

    testrun_test(test_ov_json_array_find);
    testrun_test(test_ov_json_array_del);
    testrun_test(test_ov_json_array_get);
    testrun_test(test_ov_json_array_insert);
    testrun_test(test_ov_json_array_remove);

    testrun_test(test_ov_json_array_count);
    testrun_test(test_ov_json_array_is_empty);
    testrun_test(test_ov_json_array_for_each);
    testrun_test(test_ov_json_array_remove_child);

    OV_JSON_ARRAY_PERFORM_INTERFACE_TESTS(ov_json_array);
    return testrun_counter;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST EXECUTION                                                  #EXEC
 *
 *      ------------------------------------------------------------------------
 */

testrun_run(all_tests);
