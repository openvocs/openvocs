/***
        ------------------------------------------------------------------------

        Copyright 2017 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_utils_tests.c
        @author         Michael Beer

        @date           2017-12-10

        @ingroup        ov_utils

        @brief


        ------------------------------------------------------------------------
*/

#include "../../include/ov_utils.h"

#include "ov_utils.c"
#include <ov_test/ov_test.h>

/******************************************************************************
 *                               pointer arrays
 ******************************************************************************/

static void set_random_value(void *varray, size_t capacity) {

    char **array = varray;

    OV_ASSERT(0 < capacity);

    /* 1st step: try to find an 'empty' index randomly */

    double rindex_d = random();
    rindex_d /= RAND_MAX;
    rindex_d *= capacity;

    size_t i = (size_t)rindex_d;

    size_t tries = 0;

    while ((tries < capacity) && (array[i] != 0)) {

        rindex_d = random();
        rindex_d /= RAND_MAX * capacity;

        i = (size_t)rindex_d;

        ++tries;
    }

    if (0 == array[i]) {
        goto finish;
    }

    /* unsuccessful.
     * 2nd step: try to find the first 'empty' index */

    for (i = 0; i < capacity - 1; ++i) {
        if (array[i] == 0) break;
    }

    if (0 != array[i]) {
        fprintf(stderr, "WARNING: No more free slots in array\n");
    }

finish:

    rindex_d += 1; /* Prevent setting a '0' */
    intptr_t content = (intptr_t)rindex_d;

    array[i] = (char *)content;
}

/*----------------------------------------------------------------------------*/

static int test_ov_free() {

    testrun(0 == ov_free(0));

    char *c = calloc(1, 1);
    testrun(0 != c);

    c = ov_free(c);

    testrun(0 == c);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_utils_add_to_array() {

    srandom(time(0));

    uintptr_t array1[1] = {0};

    uintptr_t user_dummy = 1;

    testrun(!ov_utils_add_to_array(0, 0, 0));
    testrun(!ov_utils_add_to_array(0, 0, (void *)user_dummy));
    testrun(!ov_utils_add_to_array(0, 1, 0));
    testrun(!ov_utils_add_to_array(0, 1, (void *)user_dummy));
    testrun(!ov_utils_add_to_array(array1, 0, 0));
    testrun(!ov_utils_add_to_array(array1, 0, (void *)user_dummy));

    /* Hmm, setting a zero pointer in an array where zero denotes empty
     * should ... ? */
    testrun(ov_utils_add_to_array(array1, 1, 0));

    testrun(ov_utils_add_to_array(array1, 1, (void *)user_dummy));
    user_dummy += 1;

    /* array full */
    testrun(!ov_utils_add_to_array(array1, 1, (void *)user_dummy));

    uintptr_t array237[237] = {0};

    for (uintptr_t i = 0; i < 237; ++i) {
        testrun(ov_utils_add_to_array(array237, 237, (void *)(i + 1)));
    }

    testrun(!ov_utils_add_to_array(array237, 237, (void *)238));

    memset(array237, 0, sizeof(array237));

    size_t nums = 0;
    for (; nums < 118; ++nums) {

        set_random_value(array237, 237);
    }

    size_t nonzeros_found = 0;
    for (uintptr_t i = 0; i < 237; ++i) {

        if (0 != array237[i]) ++nonzeros_found;
    }

    testrun(nonzeros_found == nums);

    for (uintptr_t i = 0; i < 237 - nums; ++i) {
        testrun(ov_utils_add_to_array(array237, 237, (void *)(i + 1)));
    }

    for (uintptr_t i = 237; i < 237 + 10; ++i) {
        testrun(!ov_utils_add_to_array(array237, 237, (void *)(i + 1)));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_utils_del_from_array() {

    srandom(time(0));

    intptr_t array1[1] = {0};

    intptr_t user_dummy = 1;

    testrun(!ov_utils_del_from_array(0, 0, 0));
    testrun(!ov_utils_del_from_array(0, 0, (void *)user_dummy));
    testrun(!ov_utils_del_from_array(0, 1, 0));
    testrun(!ov_utils_del_from_array(0, 1, (void *)user_dummy));

    testrun(!ov_utils_del_from_array(array1, 0, 0));
    testrun(!ov_utils_del_from_array(array1, 0, (void *)user_dummy));

    /* Well, removing 0 from an array of zeros should ... ? */
    testrun(ov_utils_del_from_array(array1, 1, 0));

    testrun(!ov_utils_del_from_array(array1, 1, (void *)user_dummy));

    testrun(ov_utils_add_to_array(array1, 1, (void *)user_dummy));

    testrun(!ov_utils_del_from_array(0, 0, 0));
    testrun(!ov_utils_del_from_array(0, 0, (void *)user_dummy));
    testrun(!ov_utils_del_from_array(0, 1, 0));
    testrun(!ov_utils_del_from_array(0, 1, (void *)user_dummy));

    testrun(!ov_utils_del_from_array(array1, 0, 0));
    testrun(!ov_utils_del_from_array(array1, 0, (void *)user_dummy));

    /* No zero entries left ... */
    testrun(!ov_utils_del_from_array(array1, 1, 0));

    /* Should succeed since user_dummy has been added */
    testrun(ov_utils_del_from_array(array1, 1, (void *)user_dummy));

    intptr_t array237[237] = {0};

    for (uintptr_t i = 0; i < 237; ++i) {
        testrun(!ov_utils_del_from_array(array237, 237, (void *)(i + 1)));
    }

    for (uintptr_t i = 0; i < 237; ++i) {
        testrun(ov_utils_add_to_array(array237, 237, (void *)(i + 1)));
    }

    /* Prevent removing in the same order as adding */
    for (uintptr_t i = 237; i > 0; --i) {
        testrun(ov_utils_del_from_array(array237, 237, (void *)i));
    }

    for (uintptr_t i = 0; i < 237; ++i) {
        testrun(!ov_utils_del_from_array(array237, 237, (void *)(i + 1)));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_utils_is_in_array() {

    testrun(!ov_utils_is_in_array(0, 0, 0));

    char *ptrs[1] = {0};
    const size_t ptrs_len = 1;

    testrun(!ov_utils_is_in_array(ptrs, 0, 0));
    testrun(!ov_utils_is_in_array(0, ptrs_len, 0));
    testrun(ov_utils_is_in_array(ptrs, ptrs_len, 0));
    testrun(!ov_utils_is_in_array(ptrs, ptrs_len, "hagen"));

    char const *AORIAN = "aorian";
    char const *BORIAN = "borian";
    char const *CORIAN = "corian";
    char const *DORIAN = "dorian";

    const size_t ptrs_2_len = 4;
    char const *ptrs_2[] = {AORIAN, BORIAN, CORIAN, DORIAN};

    testrun(!ov_utils_is_in_array(ptrs_2, ptrs_2_len, "hagen"));
    testrun(ov_utils_is_in_array(ptrs_2, ptrs_2_len, AORIAN));
    testrun(ov_utils_is_in_array(ptrs_2, ptrs_2_len, BORIAN));
    testrun(ov_utils_is_in_array(ptrs_2, ptrs_2_len, CORIAN));
    testrun(ov_utils_is_in_array(ptrs_2, ptrs_2_len, DORIAN));
    testrun(!ov_utils_is_in_array(ptrs_2, ptrs_2_len, "eleusis"));
    testrun(!ov_utils_is_in_array(ptrs_2, ptrs_2_len, 0));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_utils",
            test_ov_free,
            test_ov_utils_add_to_array,
            test_ov_utils_del_from_array,
            test_ov_utils_is_in_array);
