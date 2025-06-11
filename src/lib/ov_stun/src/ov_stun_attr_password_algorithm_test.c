/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_stun_attr_password-algorithm_test.c
        @author         Markus Töpfer

        @date           2022-01-22


        ------------------------------------------------------------------------
*/
#include "ov_stun_attr_password_algorithm.c"
#include <ov_test/testrun.h>

int test_ov_stun_attr_is_password_algorithm() {

    size_t size = 1000;
    uint8_t buf[size];
    uint8_t *buffer = buf;

    memset(buf, 'a', size);

    // prepare valid frame
    testrun(
        ov_stun_attribute_set_type(buffer, size, STUN_ATTR_PASSWORD_ALGORITHM));
    testrun(ov_stun_attribute_set_length(buffer, size, 1));

    testrun(ov_stun_attr_is_password_algorithm(buffer, size));

    testrun(!ov_stun_attr_is_password_algorithm(NULL, size));
    testrun(!ov_stun_attr_is_password_algorithm(buffer, 0));
    testrun(!ov_stun_attr_is_password_algorithm(buffer, 4));

    // min buffer min valid
    testrun(ov_stun_attr_is_password_algorithm(buffer, 8));

    // length < size
    testrun(ov_stun_attribute_set_length(buffer, size, 2));
    testrun(!ov_stun_attr_is_password_algorithm(buffer, 7));
    testrun(ov_stun_attr_is_password_algorithm(buffer, 8));

    // type not nonce
    testrun(ov_stun_attribute_set_type(buffer, size, 0));
    testrun(!ov_stun_attr_is_password_algorithm(buffer, size));
    testrun(
        ov_stun_attribute_set_type(buffer, size, STUN_ATTR_PASSWORD_ALGORITHM));
    testrun(ov_stun_attr_is_password_algorithm(buffer, size));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_attr_password_algorithm_encoding_length() {

    testrun(8 == ov_stun_attr_password_algorithm_encoding_length(0));
    testrun(8 + 10 + 2 == ov_stun_attr_password_algorithm_encoding_length(10));

    size_t pad = 0;

    for (size_t i = 1; i < 1000; i++) {

        pad = i % 4;
        if (pad != 0) pad = 4 - pad;

        testrun(8 + i + pad ==
                ov_stun_attr_password_algorithm_encoding_length(i));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_attr_password_algorithm_encode() {

    size_t size = 100;
    uint8_t buf[size];
    uint8_t *buffer = buf;
    uint8_t *next = NULL;

    uint8_t *nonce = (uint8_t *)"test1234";

    testrun(!ov_stun_attr_password_algorithm_encode(NULL, 0, NULL, 0, 0, NULL));
    testrun(
        !ov_stun_attr_password_algorithm_encode(NULL, size, NULL, 1, 8, nonce));
    testrun(
        !ov_stun_attr_password_algorithm_encode(buffer, 0, NULL, 1, 8, nonce));
    testrun(!ov_stun_attr_password_algorithm_encode(
        buffer, size, NULL, 1, 8, NULL));

    memset(buf, 0, size);
    testrun(ov_stun_attr_password_algorithm_encode(
        buffer, size, &next, 1, 8, nonce));
    testrun(STUN_ATTR_PASSWORD_ALGORITHM ==
            ov_stun_attribute_get_type(buffer, size));
    testrun(12 == ov_stun_attribute_get_length(buffer, size));
    testrun(0 == strncmp((char *)buffer + 8, (char *)nonce, 8));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ov_stun_attr_password_algorithm_decode() {

    size_t size = 100;
    uint8_t buf[size];
    uint8_t *buffer = buf;
    memset(buf, 0, size);

    const uint8_t *para = NULL;

    uint16_t algo = 0;
    uint16_t out = 0;

    uint8_t *nonce = (uint8_t *)"test1234";
    testrun(ov_stun_attr_password_algorithm_encode(
        buffer, size, NULL, 1, 8, nonce));

    testrun(
        !ov_stun_attr_password_algorithm_decode(NULL, size, NULL, NULL, NULL));
    testrun(!ov_stun_attr_password_algorithm_decode(
        NULL, size, &algo, &out, &para));
    testrun(
        !ov_stun_attr_password_algorithm_decode(buffer, 0, &algo, &out, &para));
    testrun(!ov_stun_attr_password_algorithm_decode(
        buffer, size, NULL, &out, &para));
    testrun(!ov_stun_attr_password_algorithm_decode(
        buffer, size, &algo, NULL, &para));
    testrun(!ov_stun_attr_password_algorithm_decode(
        buffer, size, &algo, &out, NULL));

    testrun(ov_stun_attr_password_algorithm_decode(
        buffer, size, &algo, &out, &para));
    testrun(algo == 1);
    testrun(out == 8);
    testrun(0 == memcmp(para, nonce, out));

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
    testrun_test(test_ov_stun_attr_is_password_algorithm);
    testrun_test(test_ov_stun_attr_password_algorithm_encoding_length);
    testrun_test(test_ov_stun_attr_password_algorithm_encode);
    testrun_test(test_ov_ov_stun_attr_password_algorithm_decode);

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
