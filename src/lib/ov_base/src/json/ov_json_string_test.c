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
        @file           ov_json_string_test.c
        @author         Markus Toepfer

        @date           2018-12-03

        @ingroup        ov_json_string

        @brief          Unit tests of 


        ------------------------------------------------------------------------
*/
#include "ov_json_string.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_json_string() {

    ov_json_value *value = NULL;
    JsonString *string = NULL;

    value = ov_json_string(NULL);
    testrun(ov_json_value_cast(value));
    testrun(ov_json_is_string(value));
    string = AS_JSON_STRING(value);
    testrun(string);
    testrun(NULL == string->buffer.start);
    testrun(0 == string->buffer.size);
    value = ov_json_string_free(value);

    value = ov_json_string("test");
    string = AS_JSON_STRING(value);
    testrun(string);
    testrun(NULL != string->buffer.start);
    testrun(4 == string->buffer.size);
    testrun(0 == strncmp("test", string->buffer.start, 4));
    value = ov_json_string_free(value);

    value = ov_json_string("test123");
    string = AS_JSON_STRING(value);
    testrun(string);
    testrun(NULL != string->buffer.start);
    testrun(7 == string->buffer.size);
    testrun(0 == strncmp("test123", string->buffer.start, 7));
    value = ov_json_string_free(value);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_is_string() {

    ov_json_value *value = NULL;

    value = ov_json_string(NULL);
    testrun(ov_json_is_string(value));
    value->type = OV_JSON_NULL;
    testrun(!ov_json_is_string(value));
    value->type = OV_JSON_FALSE;
    testrun(!ov_json_is_string(value));
    value->type = OV_JSON_TRUE;
    testrun(!ov_json_is_string(value));
    value->type = OV_JSON_NUMBER;
    testrun(!ov_json_is_string(value));
    value->type = OV_JSON_ARRAY;
    testrun(!ov_json_is_string(value));
    value->type = OV_JSON_OBJECT;
    testrun(!ov_json_is_string(value));
    value->type = NOT_JSON;
    testrun(!ov_json_is_string(value));
    value->type = 0;
    testrun(!ov_json_is_string(value));

    // reset for free
    value->type = OV_JSON_STRING;
    testrun(ov_json_is_string(value));
    value = ov_json_string_free(value);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_string_clear() {

    ov_json_value *value = NULL;
    JsonString *string = NULL;

    value = ov_json_string(NULL);
    string = AS_JSON_STRING(value);

    testrun(ov_json_string_clear(value));
    string = AS_JSON_STRING(value);
    testrun(NULL == string->buffer.start);
    testrun(0 == string->buffer.size);

    // with content
    testrun(ov_json_string_set(value, "xyz"));
    testrun(ov_json_string_clear(value));
    string = AS_JSON_STRING(value);
    testrun(NULL != string->buffer.start);
    testrun(3 == string->buffer.size);
    testrun(0 == strlen(string->buffer.start));

    // wrong type
    value->type = OV_JSON_FALSE;
    testrun(!ov_json_is_string(value));
    testrun(!ov_json_string_clear(value));

    value->type = OV_JSON_STRING;
    testrun(ov_json_string_clear(value));

    testrun(ov_json_is_string(value));
    value = ov_json_string_free(value);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_string_free() {

    ov_json_value *value = ov_json_string(NULL);

    testrun(NULL == ov_json_string_free(NULL));
    testrun(NULL == ov_json_string_free(value));

    // with content
    value = ov_json_string("1234567");
    testrun(NULL == ov_json_string_free(value));

    // wrong type
    value = ov_json_string(NULL);
    value->type = OV_JSON_TRUE;
    testrun(value == ov_json_string_free(value));

    value->type = OV_JSON_STRING;
    testrun(NULL == ov_json_string_free(value));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_string_copy() {

    ov_json_value *orig = ov_json_string(NULL);
    ov_json_value *copy = NULL;
    JsonString *string = NULL;

    testrun(!ov_json_string_copy(NULL, NULL));
    testrun(!ov_json_string_copy(NULL, orig));
    testrun(!ov_json_string_copy((void **)&copy, NULL));

    testrun(ov_json_string_copy((void **)&copy, orig));
    testrun(copy);
    testrun(copy->type == orig->type);
    string = AS_JSON_STRING(copy);
    testrun(NULL == string->buffer.start);
    testrun(0 == string->buffer.size);

    // all correct

    testrun(ov_json_string_set(orig, "test"));
    testrun(ov_json_string_copy((void **)&copy, orig));
    testrun(copy);
    testrun(copy->type == orig->type);
    string = AS_JSON_STRING(copy);
    testrun(NULL != string->buffer.start);
    testrun(4 == string->buffer.size);
    testrun(0 == strncmp("test", string->buffer.start, 4));

    testrun(ov_json_string_set(orig, "abc"));
    testrun(ov_json_string_copy((void **)&copy, orig));
    testrun(copy);
    testrun(copy->type == orig->type);
    string = AS_JSON_STRING(copy);
    testrun(NULL != string->buffer.start);
    testrun(4 == string->buffer.size);
    testrun(0 ==
            strncmp("abc", string->buffer.start, strlen(string->buffer.start)));

    // orig not a string

    testrun(ov_json_string_set(orig, "xyz1234"));

    orig->type = OV_JSON_FALSE;
    testrun(!ov_json_string_copy((void **)&copy, orig));
    // check copy unchanged
    testrun(copy);
    string = AS_JSON_STRING(copy);
    testrun(NULL != string->buffer.start);
    testrun(4 == string->buffer.size);
    testrun(0 ==
            strncmp("abc", string->buffer.start, strlen(string->buffer.start)));

    orig->type = OV_JSON_STRING;
    copy->type = OV_JSON_FALSE;
    testrun(!ov_json_string_copy((void **)&copy, orig));

    // both correct again
    copy->type = OV_JSON_STRING;
    testrun(ov_json_string_copy((void **)&copy, orig));
    testrun(copy);
    string = AS_JSON_STRING(copy);
    testrun(NULL != string->buffer.start);
    testrun(7 == string->buffer.size);
    testrun(0 == strncmp("xyz1234", string->buffer.start,
                         strlen(string->buffer.start)));

    testrun(NULL == ov_json_string_free(copy));
    testrun(NULL == ov_json_string_free(orig));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_string_dump() {

    ov_json_value *orig = ov_json_string(NULL);
    testrun(ov_json_is_string(orig));

    testrun(!ov_json_string_dump(NULL, NULL));
    testrun(!ov_json_string_dump(NULL, orig));
    testrun(!ov_json_string_dump(stdout, NULL));

    // all correct

    testrun_log("TESTDUMP follows");
    testrun(ov_json_string_dump(stdout, orig));

    // string with content
    testrun(ov_json_string_set(orig, "abcd"));
    testrun(ov_json_string_dump(stdout, orig));

    testrun_log("\n\tTESTDUMP done");

    // orig not a string

    orig->type = OV_JSON_FALSE;
    testrun(!ov_json_string_dump(stdout, orig));

    // reset for free
    orig->type = OV_JSON_STRING;
    testrun(NULL == ov_json_string_free(orig));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_string_set_length() {

    ov_json_value *value = NULL;
    JsonString *string = NULL;

    value = ov_json_string(NULL);
    string = AS_JSON_STRING(value);
    testrun(string);
    testrun(NULL == string->buffer.start);
    testrun(0 == string->buffer.size);

    testrun(!ov_json_string_set_length(NULL, NULL, 0));
    testrun(!ov_json_string_set_length(value, NULL, 4));
    testrun(!ov_json_string_set_length(NULL, "12345678", 4));
    testrun(!ov_json_string_set_length(value, "12345678", 0));

    testrun(ov_json_string_set_length(value, "12345678", 4));
    testrun(NULL != string->buffer.start);
    testrun(4 == string->buffer.size);
    testrun(0 == strncmp("1234", string->buffer.start, 4));

    // set with shorter string
    testrun(ov_json_string_set_length(value, "1234", 2));
    testrun(4 == string->buffer.size);
    testrun(0 ==
            strncmp("12", string->buffer.start, strlen(string->buffer.start)));

    // set with shorter string as initial alloc
    testrun(ov_json_string_set_length(value, "abcde", 3));
    testrun(4 == string->buffer.size);
    testrun(0 ==
            strncmp("abc", string->buffer.start, strlen(string->buffer.start)));

    // set with same string length as initial allocation
    testrun(ov_json_string_set_length(value, "abcde", 4));
    testrun(4 == string->buffer.size);
    testrun(0 == strncmp("abcd", string->buffer.start,
                         strlen(string->buffer.start)));

    // set with longer string length as initial allocation
    testrun(ov_json_string_set_length(value, "12345678", 8));
    testrun(8 == string->buffer.size);
    testrun(0 == strncmp("12345678", string->buffer.start,
                         strlen(string->buffer.start)));

    // not a string
    value->type = OV_JSON_NULL;
    testrun(!ov_json_string_set_length(value, "xyz", 3));
    // nothing changed
    testrun(8 == string->buffer.size);
    testrun(0 == strncmp("123456789", string->buffer.start,
                         strlen(string->buffer.start)));

    // reset for free
    value->type = OV_JSON_STRING;
    testrun(ov_json_is_string(value));
    value = ov_json_string_free(value);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_string_set() {

    ov_json_value *value = NULL;
    JsonString *string = NULL;

    value = ov_json_string(NULL);
    string = AS_JSON_STRING(value);
    testrun(string);
    testrun(NULL == string->buffer.start);
    testrun(0 == string->buffer.size);

    testrun(!ov_json_string_set(NULL, NULL));
    testrun(!ov_json_string_set(value, NULL));
    testrun(!ov_json_string_set(NULL, "12345678"));

    testrun(ov_json_string_set(value, "12345678"));
    testrun(NULL != string->buffer.start);
    testrun(8 == string->buffer.size);
    testrun(0 == strncmp("12345678", string->buffer.start, 8));

    // set with shorter string
    testrun(ov_json_string_set(value, "1234"));
    testrun(8 == string->buffer.size);
    testrun(0 == strncmp("1234", string->buffer.start,
                         strlen(string->buffer.start)));

    // set with shorter string as initial alloc
    testrun(ov_json_string_set(value, "abcde"));
    testrun(8 == string->buffer.size);
    testrun(0 == strncmp("abcde", string->buffer.start,
                         strlen(string->buffer.start)));

    // set with same string length as initial allocation
    testrun(ov_json_string_set(value, "abcdefg"));
    testrun(8 == string->buffer.size);
    testrun(0 == strncmp("abcdefg", string->buffer.start,
                         strlen(string->buffer.start)));

    // set with longer string length as initial allocation
    testrun(ov_json_string_set(value, "12345678"));
    testrun(8 == string->buffer.size);
    testrun(0 == strncmp("12345678", string->buffer.start,
                         strlen(string->buffer.start)));

    // not a string
    value->type = OV_JSON_NULL;
    testrun(!ov_json_string_set(value, "xyz"));
    // nothing changed
    testrun(8 == string->buffer.size);
    testrun(0 == strncmp("123456789", string->buffer.start,
                         strlen(string->buffer.start)));

    // reset for free
    value->type = OV_JSON_STRING;
    testrun(ov_json_is_string(value));
    value = ov_json_string_free(value);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_string_get() {

    ov_json_value *value = NULL;
    JsonString *string = NULL;

    value = ov_json_string(NULL);
    string = AS_JSON_STRING(value);
    testrun(string);
    testrun(NULL == string->buffer.start);
    testrun(0 == string->buffer.size);

    testrun(!ov_json_string_get(NULL));
    testrun(!ov_json_string_get(value));

    testrun(ov_json_string_set(value, "12345678"));
    const char *ptr = ov_json_string_get(value);
    testrun(ptr);
    testrun(ptr == string->buffer.start);
    testrun(8 == string->buffer.size);
    testrun(8 == strlen(ptr));

    // set with same string length as initial allocation
    testrun(ov_json_string_set(value, "abcde"));
    ptr = ov_json_string_get(value);
    testrun(ptr);
    testrun(ptr == string->buffer.start);
    testrun(8 == string->buffer.size);
    testrun(5 == strlen(ptr));

    // reset for free
    value->type = OV_JSON_STRING;
    testrun(ov_json_is_string(value));
    value = ov_json_string_free(value);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_string_is_valid() {

    ov_json_value *value = NULL;
    JsonString *string = NULL;

    value = ov_json_string(NULL);
    string = AS_JSON_STRING(value);
    testrun(string);
    testrun(NULL == string->buffer.start);
    testrun(0 == string->buffer.size);

    testrun(!ov_json_string_is_valid(NULL));

    // empty
    testrun(!ov_json_string_is_valid(value));

    // not empty
    testrun(ov_json_string_set(value, "12345678"));
    testrun(ov_json_string_is_valid(value));

    // not UTF8
    string->buffer.start[4] = 0xff;
    testrun(!ov_json_string_is_valid(value));

    string->buffer.start[4] = 0x23;
    testrun(ov_json_string_is_valid(value));

    // not a string
    value->type = OV_JSON_FALSE;
    testrun(!ov_json_string_is_valid(value));

    // reset for free
    value->type = OV_JSON_STRING;
    testrun(ov_json_is_string(value));
    value = ov_json_string_free(value);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_string_functions() {

    ov_data_function f = ov_json_string_functions();

    testrun(f.clear == ov_json_string_clear);
    testrun(f.free == ov_json_string_free);
    testrun(f.copy == ov_json_string_copy);
    testrun(f.dump == ov_json_string_dump);

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
    testrun_test(test_ov_json_string);
    testrun_test(test_ov_json_is_string);

    testrun_test(test_ov_json_string_clear);
    testrun_test(test_ov_json_string_free);
    testrun_test(test_ov_json_string_copy);
    testrun_test(test_ov_json_string_dump);

    testrun_test(test_ov_json_string_set_length);
    testrun_test(test_ov_json_string_set);
    testrun_test(test_ov_json_string_get);

    testrun_test(test_ov_json_string_is_valid);

    testrun_test(test_ov_json_string_functions);

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
