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
        @file           ov_json_object_test.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-12-02

        @ingroup        ov_json_object

        @brief          Unit tests of


        ------------------------------------------------------------------------
*/
#include "ov_json_object.c"
#include <ov_test/testrun.h>

/*----------------------------------------------------------------------------*/

int test_ov_json_object_data_functions() {

    ov_data_function f = ov_json_object_data_functions();

    testrun(f.clear == ov_json_object_clear);
    testrun(f.free == ov_json_object_free);
    testrun(f.copy == ov_json_object_copy);
    testrun(f.dump == ov_json_object_dump);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_is_object() {

    ov_json_value value = {0};

    for (uint16_t i = 0; i < 0xffff; i++) {

        value.magic_byte = i;
        value.type = OV_JSON_OBJECT;

        if (i == OV_JSON_VALUE_MAGIC_BYTE) {

            testrun(ov_json_is_object(&value));

            value.type = OV_JSON_ARRAY;
            testrun(!ov_json_is_object(&value));
            value.type = NOT_JSON;
            testrun(!ov_json_is_object(&value));

        } else {
            testrun(!ov_json_is_object(&value));
        }
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_object() {

    ov_json_value *object = ov_json_object();
    testrun(object);
    testrun(AS_JSON_OBJECT(object));
    testrun(AS_JSON_DICT(object));

    ov_json_value_free(object);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool dummy_clear(void *data) {

    if (!data) return false;
    return true;
}

/*----------------------------------------------------------------------------*/

int test_ov_json_object_clear() {

    ov_json_value *object = ov_json_object();

    testrun(!ov_json_object_clear(NULL));
    testrun(ov_json_object_clear(object));

    // wrong type

    json_object *obj = AS_JSON_OBJECT(object);

    obj->head.magic_byte = 0;
    testrun(!ov_json_object_clear(obj));
    obj->head.magic_byte = OV_JSON_VALUE_MAGIC_BYTE;
    testrun(ov_json_object_clear(obj));

    obj->head.type = NOT_JSON;
    testrun(!ov_json_object_clear(obj));
    obj->head.type = OV_JSON_ARRAY;
    testrun(!ov_json_object_clear(obj));
    obj->head.type = OV_JSON_OBJECT;
    testrun(ov_json_object_clear(obj));

    // no self.clear
    obj->head.clear = NULL;
    testrun(!ov_json_object_clear(object));
    obj->head.clear = dummy_clear;
    testrun(ov_json_object_clear(object));

    // reset clear for termination
    obj->head.clear = impl_json_object_clear;
    ov_json_value_free(object);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_object_free() {

    ov_json_value *object = ov_json_object();

    testrun(NULL == ov_json_object_free(NULL));
    testrun(NULL == ov_json_object_free(object));

    uint64_t not_a_json_object = 0;

    testrun(&not_a_json_object == ov_json_object_free(&not_a_json_object));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_object_copy() {

    ov_json_value *obj = ov_json_object();
    ov_json_value *copy = 0;

    testrun(!ov_json_object_copy(NULL, NULL));
    testrun(!ov_json_object_copy(NULL, obj));
    testrun(!ov_json_object_copy((void **)&copy, NULL));

    testrun(copy == NULL);
    testrun(ov_json_object_copy((void **)&copy, obj));
    testrun(copy);
    testrun(AS_JSON_DICT(copy));

    // wrong type
    obj->magic_byte = 0;
    testrun(!ov_json_object_copy((void **)&copy, obj));
    obj->magic_byte = OV_JSON_VALUE_MAGIC_BYTE;
    testrun(ov_json_object_copy((void **)&copy, obj));

    // with content
    testrun(ov_json_object_set(obj, "true", ov_json_true()));
    testrun(ov_json_object_set(obj, "false", ov_json_false()));
    testrun(ov_json_object_count(obj) == 2);

    testrun(ov_json_object_copy((void **)&copy, obj));
    testrun(copy);
    testrun(ov_json_object_count(copy) == 2);
    testrun(ov_json_is_true(ov_json_object_get(copy, "true")));
    testrun(ov_json_is_false(ov_json_object_get(copy, "false")));

    ov_json_object_free(obj);
    ov_json_object_free(copy);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_object_dump() {

    ov_json_value *obj = ov_json_object();

    testrun(!ov_json_object_dump(NULL, NULL));
    testrun(!ov_json_object_dump(NULL, obj));
    testrun(!ov_json_object_dump(stdout, NULL));

    testrun(ov_json_object_dump(stdout, obj));

    // wrong type
    obj->magic_byte = 0;
    testrun(!ov_json_object_dump(stdout, obj));
    obj->magic_byte = OV_JSON_VALUE_MAGIC_BYTE;
    testrun(ov_json_object_dump(stdout, obj));

    // add some content

    testrun(ov_json_object_set(obj, "key1", ov_json_true()));
    testrun(ov_json_object_set(obj, "key3", ov_json_true()));

    testrun(ov_json_object_dump(stdout, obj));

    ov_json_object_free(obj);
    return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      VALUE BASED INTERFACE FUNCTION CALLS
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_json_object_set() {

    ov_json_value *obj = ov_json_object();
    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_false();

    testrun(!ov_json_object_set(NULL, NULL, NULL));
    testrun(!ov_json_object_set(NULL, "v1", val1));
    testrun(!ov_json_object_set(obj, NULL, val1));
    testrun(!ov_json_object_set(obj, "v1", NULL));

    testrun(ov_json_object_set(obj, "v1", val1));
    testrun(1 == ov_json_object_count(obj));

    // wrong type
    obj->type = OV_JSON_ARRAY;
    testrun(!ov_json_object_set(obj, "v2", val2));
    obj->type = OV_JSON_OBJECT;
    testrun(ov_json_object_set(obj, "v2", val2));
    testrun(2 == ov_json_object_count(obj));

    ov_json_object_free(obj);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_object_del() {

    ov_json_value *object = ov_json_object();
    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_false();
    ov_json_value *val3 = ov_json_null();

    testrun(!ov_json_object_del(NULL, NULL));
    testrun(!ov_json_object_del(NULL, "v1"));
    testrun(!ov_json_object_del(object, NULL));

    // not contained
    testrun(ov_json_object_del(object, "v1"));

    testrun(ov_json_object_set(object, "v1", val1));
    testrun(ov_json_object_set(object, "v2", val2));
    testrun(ov_json_object_set(object, "v3", val3));
    testrun(3 == ov_json_object_count(object));

    // wrong type
    object->type = OV_JSON_ARRAY;
    testrun(!ov_json_object_del(object, "v1"));
    object->type = OV_JSON_OBJECT;
    testrun(ov_json_object_del(object, "v1"));
    testrun(2 == ov_json_object_count(object));

    ov_json_object_free(object);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_object_get() {

    ov_json_value *object = ov_json_object();
    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_false();
    ov_json_value *val3 = ov_json_null();

    testrun(!ov_json_object_get(NULL, NULL));
    testrun(!ov_json_object_get(NULL, "v1"));
    testrun(!ov_json_object_get(object, NULL));

    // not contained
    testrun(!ov_json_object_get(object, "v1"));

    testrun(ov_json_object_set(object, "v1", val1));
    testrun(ov_json_object_set(object, "v2", val2));
    testrun(ov_json_object_set(object, "v3", val3));
    testrun(3 == ov_json_object_count(object));

    testrun(val1 == ov_json_object_get(object, "v1"));

    // wrong type
    object->type = OV_JSON_ARRAY;
    testrun(!ov_json_object_get(object, "v1"));
    object->type = OV_JSON_OBJECT;
    testrun(val1 == ov_json_object_get(object, "v1"));

    ov_json_object_free(object);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_object_remove() {

    ov_json_value *object = ov_json_object();
    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_false();
    ov_json_value *val3 = ov_json_null();

    testrun(NULL == ov_json_object_remove(NULL, NULL));
    testrun(NULL == ov_json_object_remove(NULL, "v1"));
    testrun(NULL == ov_json_object_remove(object, NULL));

    // not contained
    testrun(NULL == ov_json_object_remove(object, "v1"));

    testrun(ov_json_object_set(object, "v1", val1));
    testrun(ov_json_object_set(object, "v2", val2));
    testrun(ov_json_object_set(object, "v3", val3));
    testrun(3 == ov_json_object_count(object));

    testrun(val1 == ov_json_object_remove(object, "v1"));

    // wrong type
    object->type = OV_JSON_ARRAY;
    testrun(NULL == ov_json_object_remove(object, "v2"));
    object->type = OV_JSON_OBJECT;
    testrun(val2 == ov_json_object_remove(object, "v2"));

    ov_json_object_free(object);
    ov_json_value_free(val1);
    ov_json_value_free(val2);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_object_count() {

    ov_json_value *object = ov_json_object();
    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_false();
    ov_json_value *val3 = ov_json_null();

    testrun(0 == ov_json_object_count(NULL));
    testrun(0 == ov_json_object_count(object));

    testrun(ov_json_object_set(object, "v1", val1));
    testrun(ov_json_object_set(object, "v2", val2));
    testrun(ov_json_object_set(object, "v3", val3));
    testrun(3 == ov_json_object_count(object));

    // wrong type
    object->type = OV_JSON_ARRAY;
    testrun(0 == ov_json_object_count(object));
    object->type = OV_JSON_OBJECT;
    testrun(3 == ov_json_object_count(object));

    ov_json_object_free(object);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_object_is_empty() {

    ov_json_value *object = ov_json_object();
    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_false();
    ov_json_value *val3 = ov_json_null();

    testrun(!ov_json_object_is_empty(NULL));
    testrun(ov_json_object_is_empty(object));

    testrun(ov_json_object_set(object, "v1", val1));
    testrun(ov_json_object_set(object, "v2", val2));
    testrun(ov_json_object_set(object, "v3", val3));
    testrun(3 == ov_json_object_count(object));
    testrun(!ov_json_object_is_empty(object));

    testrun(ov_json_object_del(object, "v1"));
    testrun(ov_json_object_del(object, "v2"));
    testrun(ov_json_object_del(object, "v3"));

    testrun(ov_json_object_is_empty(object));

    // wrong type
    object->type = OV_JSON_ARRAY;
    testrun(!ov_json_object_is_empty(object));
    object->type = OV_JSON_OBJECT;
    testrun(ov_json_object_is_empty(object));

    ov_json_object_free(object);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool dummy_for_each(const void *key, void *item, void *data) {

    if (key || item || data) return true;
    return true;
}

/*----------------------------------------------------------------------------*/

int test_ov_json_object_for_each() {

    ov_json_value *object = ov_json_object();
    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_false();
    ov_json_value *val3 = ov_json_null();

    testrun(!ov_json_object_for_each(NULL, NULL, NULL));
    testrun(!ov_json_object_for_each(object, NULL, NULL));
    testrun(!ov_json_object_for_each(NULL, NULL, dummy_for_each));

    testrun(ov_json_object_for_each(object, NULL, dummy_for_each));

    testrun(ov_json_object_set(object, "v1", val1));
    testrun(ov_json_object_set(object, "v2", val2));
    testrun(ov_json_object_set(object, "v3", val3));

    testrun(ov_json_object_for_each(object, NULL, dummy_for_each));

    // wrong type
    object->type = OV_JSON_ARRAY;
    testrun(!ov_json_object_for_each(object, NULL, dummy_for_each));
    object->type = OV_JSON_OBJECT;
    testrun(ov_json_object_for_each(object, NULL, dummy_for_each));

    ov_json_object_free(object);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_object_remove_child() {

    ov_json_value *object2 = ov_json_object();
    ov_json_value *object = ov_json_object();
    ov_json_value *val1 = ov_json_true();
    ov_json_value *val2 = ov_json_false();
    ov_json_value *val3 = ov_json_null();

    testrun(!ov_json_object_remove_child(NULL, NULL));
    testrun(!ov_json_object_remove_child(NULL, val1));
    testrun(!ov_json_object_remove_child(object, NULL));

    // not contained
    testrun(ov_json_object_remove_child(object, val1));

    testrun(ov_json_object_set(object, "v1", val1));
    testrun(ov_json_object_set(object, "v2", val2));
    testrun(ov_json_object_set(object, "v3", val3));
    testrun(3 == ov_json_object_count(object));

    testrun(val1->parent == object);
    testrun(3 == ov_json_object_count(object));
    testrun(ov_json_object_remove_child(object, val1));
    testrun(val1->parent == NULL);
    val1 = ov_json_value_free(val1);
    testrun(0 == val1);

    testrun(2 == ov_json_object_count(object));

    ov_json_object_free(object);
    ov_json_object_free(object2);

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

    testrun_test(test_ov_json_object);
    testrun_test(test_ov_json_is_object);

    testrun_test(test_ov_json_object_clear);
    testrun_test(test_ov_json_object_free);
    testrun_test(test_ov_json_object_copy);
    testrun_test(test_ov_json_object_dump);

    testrun_test(test_ov_json_object_data_functions);

    // interface tests

    testrun_test(test_ov_json_object_set);
    testrun_test(test_ov_json_object_del);
    testrun_test(test_ov_json_object_get);
    testrun_test(test_ov_json_object_remove);

    testrun_test(test_ov_json_object_count);
    testrun_test(test_ov_json_object_is_empty);
    testrun_test(test_ov_json_object_for_each);
    testrun_test(test_ov_json_object_remove_child);

    OV_JSON_OBJECT_PERFORM_INTERFACE_TESTS(ov_json_object);

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
