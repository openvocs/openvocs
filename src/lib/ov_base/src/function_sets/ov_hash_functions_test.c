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

        This file is part of the openvocs project. http://openvocs.org

        ------------------------------------------------------------------------
*//**
        @file           ov_hash_functions_test.c
        @author         Michael Beer
        @author         Markus Toepfer

        @date           2018-05-07

        @ingroup        ov_data_structures

        @brief


        ------------------------------------------------------------------------
*/
#include "ov_hash_functions.c"
#include <ov_test/testrun.h>

#include "../../include/ov_utils.h"
#include "stdbool.h"

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST HELPER                                                     #HELPER
 *
 *      ------------------------------------------------------------------------
 */

#define VARIANCE(v, e, x) ((v) + ((x) - (e)) * ((x) - (e)))

bool utils_fill_buffer_random_string(uint8_t **buffer,
                                     size_t length,
                                     const char *alphabet_in) {

    static const char *alphabet_default = "1234567890AbCdEfGhJkMnPqRsTwXz";

    const char *alphabet = alphabet_in;
    if (!alphabet) alphabet = alphabet_default;

    size_t alphabet_max_index = strlen(alphabet);

    if (0 == alphabet_max_index) goto error;

    --alphabet_max_index;

    size_t index = 0;
    uint8_t *pointer = NULL;
    int64_t number = 0;

    if (!length) goto error;

    pointer = *buffer;

    if (!pointer) {
        pointer = calloc(length, sizeof(uint8_t));
        *buffer = pointer;
    }

    for (index = 0; index + 1 < length; index++) {
        number = random();
        pointer[index] = alphabet[(number * alphabet_max_index) / RAND_MAX];
    }

    pointer[length - 1] = 0;

    return true;

error:
    return false;
}

void create_random_c_strings(char **keys, size_t num_keys) {

    for (size_t i = 0; i < num_keys; ++i) {

        utils_fill_buffer_random_string((uint8_t **)&keys[i], 5 + i % 250, 0);
    }
}

char *keys[1 << 20] = {};

/*---------------------------------------------------------------------------*/

void helper_hash_function_c_string(uint64_t (*f)(const void *),
                                   double expected_avg) {

    OV_ASSERT(0 == f(0));

    uint64_t hash_value = 0;
    double average = 0;
    double variance = 0;

    const size_t num_keys = sizeof(keys) / sizeof(keys[0]);

    for (size_t i = 0; i < num_keys; ++i) {

        hash_value = f(keys[i]);
        average += hash_value;
        variance = VARIANCE(variance, expected_avg, hash_value);
    }

    average /= num_keys;
    variance /= num_keys;

    fprintf(stderr,
            "AVERAGE EXPECTED: %f    REAL:  %f    VARIANCE %f\n",
            expected_avg,
            average,
            variance);
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_hash_simple_c_string() {

    helper_hash_function_c_string(ov_hash_simple_c_string, UINT8_MAX / 2);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_ov_hash_pearson_c_string() {

    helper_hash_function_c_string(ov_hash_pearson_c_string, UINT8_MAX / 2);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_hash_intptr() {

    for (intptr_t i = 0; i < 0xffff; i++) {

        testrun((uint64_t)i == ov_hash_intptr((void *)i));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_hash_uint64() {

    uint64_t i = 0;

    for (i = 0; i < 0xffff; i++) {

        testrun((uint64_t)i == ov_hash_uint64(&i));
    }

    // check max
    i = 0xFFFFFFFFFFFFFFFF;
    testrun(i == ov_hash_uint64(&i));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_hash_int64() {

    int64_t i = 0;

    for (i = -0xffff; i < 0xffff; i++) {

        testrun((uint64_t)i == ov_hash_int64(&i));
    }

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
    testrun_test(test_ov_hash_simple_c_string);
    testrun_test(test_ov_hash_pearson_c_string);
    testrun_test(test_ov_hash_intptr);
    testrun_test(test_ov_hash_uint64);
    testrun_test(test_ov_hash_int64);

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
