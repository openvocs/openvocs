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
        @file           ov_json_test.c
        @author         Markus Toepfer

        @date           2018-12-02

        @ingroup        ov_json

        @brief          Unit tests of a JSON implementation combining a
                        a customizable json for value generation,
                        as well as a customizable parser for item parsing.


        ------------------------------------------------------------------------
*/

#ifndef OV_TEST_RESOURCE_DIR
#error "Must provide -D OV_TEST_RESOURCE_DIR=value while compiling this file."
#endif

#include "ov_json.c"
#include <ov_test/testrun.h>

/*----------------------------------------------------------------------------*/

int test_ov_json_read() {

    ov_json_value *value = NULL;

    char *buffer = "{}";

    testrun(!ov_json_read(NULL, 0));
    testrun(!ov_json_read(buffer, 0));
    testrun(!ov_json_read(NULL, strlen(buffer)));

    value = ov_json_read(buffer, strlen(buffer));
    testrun(value);
    testrun(ov_json_object_is_empty(value));
    value = ov_json_value_free(value);

    buffer = "[]";
    value = ov_json_read(buffer, strlen(buffer));
    testrun(value);
    testrun(ov_json_array_is_empty(value));
    value = ov_json_value_free(value);

    buffer = "\"string\"";
    value = ov_json_read(buffer, strlen(buffer));
    testrun(value);
    testrun(0 == strncmp("string", ov_json_string_get(value), strlen(buffer)));
    value = ov_json_value_free(value);

    buffer = "1 ";
    value = ov_json_read(buffer, strlen(buffer));
    testrun(value);
    testrun(1 == ov_json_number_get(value));
    value = ov_json_value_free(value);

    buffer = "true";
    value = ov_json_read(buffer, strlen(buffer));
    testrun(value);
    testrun(ov_json_is_true(value));
    value = ov_json_value_free(value);

    buffer = "{ not json }";
    testrun(!ov_json_read(buffer, strlen(buffer)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_write() {

    size_t size = 100;
    char buffer[size];
    memset(buffer, 0, size);

    ov_json_value *value = ov_json_object();

    char *next = NULL;

    testrun(!ov_json_write(NULL, buffer, size, &next));
    testrun(!ov_json_write(value, NULL, size, &next));
    testrun(!ov_json_write(value, buffer, 0, &next));

    testrun(ov_json_write(value, buffer, size, NULL));
    testrun(0 == strncmp(buffer, "{}", 2));
    testrun(ov_json_object_set(value, "1", ov_json_number(1)));
    testrun(ov_json_write(value, buffer, size, &next));
    char *expect = "{\n\t\"1\":1\n}";
    testrun(0 == strncmp(buffer, expect, strlen(expect)));
    testrun(next == buffer + strlen(expect));

    testrun(ov_json_write(value, buffer, strlen(expect), &next));
    testrun(!ov_json_write(value, buffer, strlen(expect) - 1, &next));

    ov_json_value_free(value);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_read_file() {

    char *path = OV_TEST_RESOURCE_DIR "/JSON/json.ok1";
    ov_json_value *value = NULL;

    testrun(!ov_json_read_file(NULL));
    value = ov_json_read_file(path);
    value = ov_json_value_free(value);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

    testrun_init();

    testrun_test(test_ov_json_read);
    testrun_test(test_ov_json_write);
    testrun_test(test_ov_json_read_file);

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
