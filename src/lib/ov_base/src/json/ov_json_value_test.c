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
        @file           ov_json_value_test.c
        @author         Markus Toepfer

        @date           2018-12-02

        @ingroup        ov_json_value

        @brief          Unit tests of ov_json_value.


        ------------------------------------------------------------------------
*/
#include "ov_json_value.c"
#include <ov_base/ov_json.h>
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_json_value_cast() {

    ov_json_value value = {0};

    for (uint16_t i = 0; i < 0xffff; i++) {

        value.magic_byte = i;
        value.type = 0;

        if (i == OV_JSON_VALUE_MAGIC_BYTE) {

            testrun(ov_json_value_cast(&value));

            // check ignore value type
            value.type = OV_JSON_OBJECT;
            testrun(ov_json_value_cast(&value));
            value.type = OV_JSON_ARRAY;
            testrun(ov_json_value_cast(&value));
            value.type = OV_JSON_STRING;
            testrun(ov_json_value_cast(&value));
            value.type = OV_JSON_NUMBER;
            testrun(ov_json_value_cast(&value));
            value.type = OV_JSON_TRUE;
            testrun(ov_json_value_cast(&value));
            value.type = OV_JSON_FALSE;
            testrun(ov_json_value_cast(&value));
            value.type = OV_JSON_NULL;
            testrun(ov_json_value_cast(&value));
            value.type = NOT_JSON;
            testrun(ov_json_value_cast(&value));

        } else {
            testrun(!ov_json_value_cast(&value));
        }
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_value_validate() {

    ov_json_value value = {0};

    for (uint16_t i = 0; i < 0xffff; i++) {

        value.magic_byte = i;
        value.type = OV_JSON_OBJECT;

        if (i == OV_JSON_VALUE_MAGIC_BYTE) {

            testrun(ov_json_value_validate(&value));

            // check ignore value type
            value.type = OV_JSON_OBJECT;
            testrun(ov_json_value_validate(&value));
            value.type = OV_JSON_ARRAY;
            testrun(ov_json_value_validate(&value));
            value.type = OV_JSON_STRING;
            testrun(ov_json_value_validate(&value));
            value.type = OV_JSON_NUMBER;
            testrun(ov_json_value_validate(&value));
            value.type = OV_JSON_TRUE;
            testrun(ov_json_value_validate(&value));
            value.type = OV_JSON_FALSE;
            testrun(ov_json_value_validate(&value));
            value.type = OV_JSON_NULL;
            testrun(ov_json_value_validate(&value));

            value.type = NOT_JSON;
            testrun(!ov_json_value_validate(&value));
            value.type = i;
            testrun(!ov_json_value_validate(&value));
            value.type = 666;
            testrun(!ov_json_value_validate(&value));

        } else {
            testrun(!ov_json_value_cast(&value));
        }
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_value_clear() {

    ov_json_value *value = ov_json_true();

    testrun(ov_json_value_cast(value));
    testrun(!ov_json_value_clear(NULL));

    // check type not found
    value->type = NOT_JSON;
    testrun(ov_json_value_cast(value));
    testrun(!ov_json_value_clear(value));

    // check literal clear
    value->type = OV_JSON_TRUE;
    testrun(ov_json_value_clear(value));
    testrun(value->type == OV_JSON_NULL);
    value->type = OV_JSON_FALSE;
    testrun(ov_json_value_clear(value));
    testrun(value->type == OV_JSON_NULL);
    value->type = OV_JSON_NULL;
    testrun(ov_json_value_clear(value));
    testrun(value->type == OV_JSON_NULL);
    value = ov_json_value_free(value);

    // check string clear
    value = ov_json_string("TEST");
    testrun(4 == strlen(ov_json_string_get(value)));
    testrun(ov_json_value_clear(value));
    testrun(0 == strlen(ov_json_string_get(value)));
    value = ov_json_value_free(value);

    // check number clear
    value = ov_json_number(123);
    testrun(123 == ov_json_number_get(value));
    testrun(ov_json_value_clear(value));
    testrun(0 == ov_json_number_get(value));
    value = ov_json_value_free(value);

    // check array clear
    value = ov_json_array();
    ov_json_array_push(value, ov_json_true());
    ov_json_array_push(value, ov_json_true());
    ov_json_array_push(value, ov_json_true());
    testrun(3 == ov_json_array_count(value));
    testrun(ov_json_value_clear(value));
    testrun(0 == ov_json_array_count(value));
    value = ov_json_value_free(value);

    // check object clear
    value = ov_json_object();
    ov_json_object_set(value, "key1", ov_json_true());
    ov_json_object_set(value, "key2", ov_json_true());
    ov_json_object_set(value, "key3", ov_json_true());
    testrun(3 == ov_json_object_count(value));
    testrun(ov_json_value_clear(value));
    testrun(0 == ov_json_object_count(value));
    value = ov_json_value_free(value);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_value_free() {

    ov_json_value *value = ov_json_true();

    testrun(NULL == ov_json_value_free(value));
    testrun(NULL == ov_json_value_free(NULL));

    // check type not found
    value = ov_json_true();
    value->type = NOT_JSON;
    testrun(value == ov_json_value_free(value));

    // check literal free
    value->type = OV_JSON_TRUE;
    testrun(NULL == ov_json_value_free(value));
    value = ov_json_false();
    testrun(NULL == ov_json_value_free(value));
    value = ov_json_null();
    testrun(NULL == ov_json_value_free(value));

    // check string clear
    value = ov_json_string("TEST");
    testrun(NULL == ov_json_value_free(value));

    // check number clear
    value = ov_json_number(123);
    testrun(NULL == ov_json_value_free(value));

    // check array clear
    value = ov_json_array();
    ov_json_array_push(value, ov_json_true());
    ov_json_array_push(value, ov_json_true());
    ov_json_array_push(value, ov_json_true());
    testrun(3 == ov_json_array_count(value));
    testrun(NULL == ov_json_value_free(value));

    // check object clear
    value = ov_json_object();
    ov_json_object_set(value, "key1", ov_json_true());
    ov_json_object_set(value, "key2", ov_json_true());
    ov_json_object_set(value, "key3", ov_json_true());
    testrun(3 == ov_json_object_count(value));
    testrun(NULL == ov_json_value_free(value));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_value_copy() {

    ov_json_value *obj = ov_json_object();
    ov_json_value *orig = ov_json_true();
    ov_json_value *copy = NULL;

    testrun(!ov_json_value_copy(NULL, NULL));
    testrun(!ov_json_value_copy(NULL, orig));
    testrun(!ov_json_value_copy((void **)&copy, NULL));

    testrun(ov_json_value_copy((void **)&copy, orig));
    testrun(ov_json_is_true(copy));

    // copy literal
    // (1) to null
    copy = ov_json_value_free(copy);
    testrun(ov_json_value_copy((void **)&copy, orig));
    testrun(ov_json_is_true(copy));
    // (2) to existing literal
    testrun(ov_json_value_copy((void **)&copy, orig));
    testrun(ov_json_is_true(copy));
    orig->type = OV_JSON_FALSE;
    testrun(ov_json_value_copy((void **)&copy, orig));
    testrun(ov_json_is_false(copy));
    // (3) to non literal destination
    testrun(!ov_json_value_copy((void **)&obj, orig));
    testrun(ov_json_is_false(orig));
    testrun(ov_json_is_object(obj));
    copy = ov_json_value_free(copy);
    orig = ov_json_value_free(orig);

    // copy string
    // (1) to null
    orig = ov_json_string("test");
    copy = ov_json_value_free(copy);
    testrun(ov_json_value_copy((void **)&copy, orig));
    testrun(ov_json_is_string(copy));
    testrun(0 == strncmp("test", ov_json_string_get(copy), 4));
    // (2) to existing string
    testrun(ov_json_string_set(orig, "longer"));
    testrun(ov_json_value_copy((void **)&copy, orig));
    testrun(ov_json_is_string(copy));
    testrun(0 == strncmp("longer", ov_json_string_get(copy), 6));
    // (3) to non string destination
    testrun(!ov_json_value_copy((void **)&obj, orig));
    testrun(ov_json_is_string(orig));
    testrun(ov_json_is_object(obj));
    copy = ov_json_value_free(copy);
    orig = ov_json_value_free(orig);

    // copy number
    // (1) to null
    orig = ov_json_number(123);
    copy = ov_json_value_free(copy);
    testrun(ov_json_value_copy((void **)&copy, orig));
    testrun(ov_json_is_number(copy));
    testrun(123 == ov_json_number_get(copy));
    // (2) to existing number
    testrun(ov_json_number_set(orig, -45));
    testrun(ov_json_value_copy((void **)&copy, orig));
    testrun(ov_json_is_number(copy));
    testrun(-45 == ov_json_number_get(copy));
    // (3) to non number destination
    testrun(!ov_json_value_copy((void **)&obj, orig));
    testrun(ov_json_is_number(orig));
    testrun(ov_json_is_object(obj));
    copy = ov_json_value_free(copy);
    orig = ov_json_value_free(orig);

    // copy array
    // (1) to null
    orig = ov_json_array();
    copy = ov_json_value_free(copy);
    testrun(ov_json_array_push(orig, ov_json_true()));
    testrun(ov_json_array_push(orig, ov_json_true()));
    testrun(ov_json_array_push(orig, ov_json_true()));
    testrun(3 == ov_json_array_count(orig));
    testrun(ov_json_value_copy((void **)&copy, orig));
    testrun(ov_json_is_array(copy));
    testrun(3 == ov_json_array_count(copy));
    // (2) to existing array
    testrun(ov_json_array_push(orig, ov_json_true()));
    testrun(ov_json_array_push(orig, ov_json_true()));
    testrun(ov_json_value_copy((void **)&copy, orig));
    testrun(ov_json_is_array(copy));
    testrun(5 == ov_json_array_count(copy));
    // (3) to non array destination
    testrun(!ov_json_value_copy((void **)&obj, orig));
    testrun(ov_json_is_array(orig));
    testrun(ov_json_is_object(obj));
    copy = ov_json_value_free(copy);
    orig = ov_json_value_free(orig);

    // copy object
    // (1) to null
    orig = ov_json_object();
    copy = ov_json_value_free(copy);
    testrun(ov_json_object_set(orig, "1", ov_json_true()));
    testrun(ov_json_object_set(orig, "2", ov_json_false()));
    testrun(ov_json_object_set(orig, "3", ov_json_null()));
    testrun(3 == ov_json_object_count(orig));
    testrun(ov_json_value_copy((void **)&copy, orig));
    testrun(ov_json_is_object(copy));
    testrun(3 == ov_json_object_count(copy));
    testrun(ov_json_is_true(ov_json_object_get(copy, "1")));
    testrun(ov_json_is_false(ov_json_object_get(copy, "2")));
    testrun(ov_json_is_null(ov_json_object_get(copy, "3")));
    // (2) to existing array
    testrun(ov_json_object_set(orig, "2", ov_json_array()));
    testrun(ov_json_object_set(orig, "4", ov_json_array()));
    testrun(ov_json_value_copy((void **)&copy, orig));
    testrun(4 == ov_json_object_count(copy));
    testrun(ov_json_is_true(ov_json_object_get(copy, "1")));
    testrun(ov_json_is_array(ov_json_object_get(copy, "2")));
    testrun(ov_json_is_null(ov_json_object_get(copy, "3")));
    testrun(ov_json_is_array(ov_json_object_get(copy, "4")));
    // (3) to non object destination
    obj = ov_json_value_free(obj);
    obj = ov_json_array();
    testrun(!ov_json_value_copy((void **)&obj, orig));
    testrun(ov_json_is_array(obj));
    testrun(ov_json_is_object(orig));
    copy = ov_json_value_free(copy);
    orig = ov_json_value_free(orig);

    ov_json_value_free(obj);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_value_dump() {

    ov_json_value *orig = ov_json_true();

    testrun(!ov_json_value_dump(NULL, NULL));
    testrun(!ov_json_value_dump(stdout, NULL));
    testrun(!ov_json_value_dump(NULL, orig));

    testrun_log("TESTDUMPS START\n");

    testrun(ov_json_value_dump(stdout, orig));
    orig->type = OV_JSON_FALSE;
    testrun(ov_json_value_dump(stdout, orig));
    orig->type = OV_JSON_NULL;
    testrun(ov_json_value_dump(stdout, orig));
    orig = ov_json_value_free(orig);

    orig = ov_json_string("test");
    testrun(ov_json_value_dump(stdout, orig));
    orig = ov_json_value_free(orig);

    orig = ov_json_number(123);
    testrun(ov_json_value_dump(stdout, orig));
    orig = ov_json_value_free(orig);

    orig = ov_json_array();
    testrun(ov_json_value_dump(stdout, orig));
    orig = ov_json_value_free(orig);

    orig = ov_json_object();
    testrun(ov_json_value_dump(stdout, orig));
    orig = ov_json_value_free(orig);

    testrun_log("\n\tTESTDUMPS END\n");

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_value_data_functions() {

    ov_data_function f = ov_json_value_data_functions();

    testrun(f.clear == ov_json_value_clear);
    testrun(f.free == ov_json_value_free);
    testrun(f.copy == ov_json_value_copy);
    testrun(f.dump == ov_json_value_dump);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_value_set_parent() {

    ov_json_value *obj = ov_json_object();
    ov_json_value *arr = ov_json_array();
    ov_json_value *val = ov_json_true();
    ov_json_value *num = ov_json_number(2);

    testrun(!ov_json_value_set_parent(NULL, NULL));
    testrun(!ov_json_value_set_parent(val, NULL));
    testrun(!ov_json_value_set_parent(NULL, arr));

    // set to non collection parent
    testrun(!ov_json_value_set_parent(val, num));
    testrun(val->parent == NULL);

    // set to collection parent
    testrun(ov_json_value_set_parent(val, arr));
    testrun(val->parent == arr);

    // set with existing parent
    testrun(!ov_json_value_set_parent(val, obj));
    testrun(val->parent == arr);

    // set to same existing parent
    testrun(ov_json_value_set_parent(val, arr));
    testrun(val->parent == arr);

    // free with parent, but not added
    testrun(0 == ov_json_array_count(arr));
    testrun(NULL == ov_json_value_free(val));
    testrun(0 == ov_json_array_count(arr));

    // free added at parent
    testrun(ov_json_value_set_parent(num, arr));
    testrun(ov_json_array_push(arr, num));
    testrun(1 == ov_json_array_count(arr));
    testrun(NULL == ov_json_value_free(num));
    testrun(0 == ov_json_array_count(arr));

    ov_json_value_free(arr);
    ov_json_value_free(obj);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_value_unset_parent() {

    ov_json_value *obj = ov_json_object();
    ov_json_value *arr = ov_json_array();
    ov_json_value *val = ov_json_true();
    ov_json_value *num = ov_json_number(2);

    testrun(!ov_json_value_unset_parent(NULL));

    // value without parent
    testrun(val->parent == NULL);
    testrun(0 == ov_json_array_count(arr));
    testrun(0 == ov_json_object_count(obj));
    testrun(ov_json_value_unset_parent(val));
    testrun(val->parent == NULL);
    testrun(0 == ov_json_array_count(arr));
    testrun(0 == ov_json_object_count(obj));

    // value added to parent, but not in parent
    testrun(ov_json_value_set_parent(val, arr));
    testrun(val->parent == arr);
    testrun(0 == ov_json_array_count(arr));
    testrun(0 == ov_json_object_count(obj));
    testrun(ov_json_value_unset_parent(val));
    testrun(val->parent == NULL);
    testrun(0 == ov_json_array_count(arr));
    testrun(0 == ov_json_object_count(obj));

    // value added to parent, add in parent
    testrun(ov_json_value_set_parent(val, arr));
    testrun(ov_json_array_push(arr, val));
    testrun(val->parent == arr);
    testrun(1 == ov_json_array_count(arr));
    testrun(0 == ov_json_object_count(obj));
    testrun(ov_json_value_unset_parent(val));
    testrun(val->parent == NULL);
    testrun(0 == ov_json_array_count(arr));
    testrun(0 == ov_json_object_count(obj));

    // value moved to different parent
    testrun(ov_json_value_set_parent(val, obj));
    testrun(ov_json_object_set(obj, "1", val));
    testrun(val->parent == obj);
    testrun(0 == ov_json_array_count(arr));
    testrun(1 == ov_json_object_count(obj));
    testrun(ov_json_value_unset_parent(val));
    testrun(val->parent == NULL);
    testrun(0 == ov_json_array_count(arr));
    testrun(0 == ov_json_object_count(obj));

    // value parent not a collection
    val->parent = num;
    testrun(ov_json_value_unset_parent(val));
    testrun(val->parent == NULL);

    // free with non collection parent
    val->parent = num;
    testrun(NULL == ov_json_value_free(val));

    ov_json_value_free(arr);
    ov_json_value_free(obj);
    ov_json_value_free(num);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_value_to_string() {

    ov_json_value *value = ov_json_object();
    char *expect = "{}";
    char *result = NULL;

    testrun(NULL == ov_json_value_to_string(NULL));
    testrun(NULL == ov_json_value_to_string((ov_json_value *)expect));
    result = ov_json_value_to_string(value);
    testrun(result);
    testrun(0 == strncmp(result, expect, strlen(expect)));
    result = ov_data_pointer_free(result);

    testrun(ov_json_object_set(value, "1", ov_json_number(1)));
    expect = "{\n\t\"1\":1\n}";
    result = ov_json_value_to_string(value);
    testrun(result);
    testrun(0 == strncmp(result, expect, strlen(expect)));
    result = ov_data_pointer_free(result);

    testrun(ov_json_object_set(value, "two", ov_json_array()));
    expect = "{\n\t\"1\":1,\n\t\"two\":[]\n}";
    result = ov_json_value_to_string(value);
    testrun(result);
    testrun(0 == strncmp(result, expect, strlen(expect)));
    result = ov_data_pointer_free(result);

    testrun(NULL == ov_json_value_free(value));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_value_from_string() {

    char *string = "{}";
    testrun(!ov_json_value_from_string(NULL, 0));
    testrun(!ov_json_value_from_string(string, 1));

    ov_json_value *result = ov_json_value_from_string(string, strlen(string));
    testrun(result);
    testrun(ov_json_object_is_empty(result));
    result = ov_json_value_free(result);

    string = "{\"1\":1,\"two\":[true,1,{\"x\":false}]}";
    result = ov_json_value_from_string(string, strlen(string));
    testrun(result);
    testrun(ov_json_is_false(ov_json_get(result, "/two/2/x")));
    result = ov_json_value_free(result);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_parsing_escaped_double_quote() {

    char *string = "\"echo_cancellator\"";
    fprintf(stdout, "IN  |%s|\n", string);

    int len = strlen(string);
    ov_json_value *value = ov_json_value_from_string(string, len);
    testrun(value);

    char *out = ov_json_value_to_string(value);
    fprintf(stdout, "OUT |%s|\n", out);
    out = ov_data_pointer_free(out);
    value = ov_json_value_free(value);

    string = "{\"type\": \"\"echo_cancellator\"\"}";
    len = strlen(string);
    fprintf(stdout, "IN  |%s|\n", string);
    fprintf(stdout, "IN  |%.*s|%i|\n", len, string, len);
    value = ov_json_value_from_string(string, len);
    testrun(!value);

    fprintf(stdout, "\n--------------------------\n");

    string = "{\"type\":\"\\\"echo_cancellator\\\"\"}";
    len = strlen(string);
    fprintf(stdout, "IN  |%s|\n", string);
    fprintf(stdout, "IN  |%.*s|%i|\n", len, string, len);
    value = ov_json_value_from_string(string, len);
    testrun(value);
    out = ov_json_value_to_string(value);
    fprintf(stdout, "OUT |%s|\n", out);
    out = ov_data_pointer_free(out);
    value = ov_json_value_free(value);

    /*
     *      Use default formating for content check
     */

    string = "{\n\t\"type\":\"\\\"echo_cancellator\\\"\"\n}";
    len = strlen(string);
    fprintf(stdout, "IN  |%.*s|%i|\n", len, string, len);
    value = ov_json_value_from_string(string, len);
    testrun(value);
    out = ov_json_value_to_string(value);
    fprintf(stdout, "IN  |%s|\n", string);
    fprintf(stdout, "OUT |%s|\n", out);
    testrun(0 == strncmp(string, out, strlen(string)));
    out = ov_data_pointer_free(out);
    value = ov_json_value_free(value);

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

    testrun_test(test_ov_json_value_cast);
    testrun_test(test_ov_json_value_validate);
    testrun_test(test_ov_json_value_clear);
    testrun_test(test_ov_json_value_free);
    testrun_test(test_ov_json_value_copy);
    testrun_test(test_ov_json_value_dump);

    testrun_test(test_ov_json_value_data_functions);

    testrun_test(test_ov_json_value_set_parent);
    testrun_test(test_ov_json_value_unset_parent);

    testrun_test(test_ov_json_value_to_string);
    testrun_test(test_ov_json_value_from_string);

    testrun_test(check_parsing_escaped_double_quote);

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
