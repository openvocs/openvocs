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
        @file           ov_turn_attr_data_test.c
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#include "ov_turn_attr_data.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_turn_attr_is_data() {

    size_t size = 1000;
    uint8_t buf[size];
    uint8_t *buffer = buf;

    memset(buf, 'a', size);

    // prepare valid frame
    testrun(ov_stun_attribute_set_type(buffer, size, TURN_ATTR_DATA));
    testrun(ov_stun_attribute_set_length(buffer, size, 1));

    testrun(ov_turn_attr_is_data(buffer, size));

    testrun(!ov_turn_attr_is_data(NULL, size));
    testrun(!ov_turn_attr_is_data(buffer, 0));
    testrun(!ov_turn_attr_is_data(buffer, 4));

    // min buffer min valid
    testrun(ov_turn_attr_is_data(buffer, 8));

    // length < size
    testrun(ov_stun_attribute_set_length(buffer, size, 2));
    testrun(!ov_turn_attr_is_data(buffer, 7));
    testrun(ov_turn_attr_is_data(buffer, 8));

    // type not nonce
    testrun(ov_stun_attribute_set_type(buffer, size, 0));
    testrun(!ov_turn_attr_is_data(buffer, size));
    testrun(ov_stun_attribute_set_type(buffer, size, TURN_ATTR_DATA));
    testrun(ov_turn_attr_is_data(buffer, size));

    // check length
    for (size_t i = 1; i < size; i++) {

        testrun(ov_stun_attribute_set_type(buffer, size, TURN_ATTR_DATA));
        testrun(ov_stun_attribute_set_length(buffer, size, i));
        testrun(ov_turn_attr_is_data(buffer, size));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_attr_data_encoding_length() {

    testrun(4 == ov_turn_attr_data_encoding_length(0));
    testrun(4 + 10 + 2 == ov_turn_attr_data_encoding_length(10));

    size_t pad = 0;

    for (size_t i = 1; i < 1000; i++) {

        pad = i % 4;
        if (pad != 0) pad = 4 - pad;

        testrun(4 + i + pad == ov_turn_attr_data_encoding_length(i));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_attr_data_encode() {

    size_t size = 100;
    uint8_t buf[size];
    uint8_t *buffer = buf;
    uint8_t *next = NULL;

    uint8_t *nonce = (uint8_t *)"test1234";

    testrun(!ov_turn_attr_data_encode(NULL, 0, NULL, NULL, 0));
    testrun(!ov_turn_attr_data_encode(NULL, size, NULL, nonce, 8));
    testrun(!ov_turn_attr_data_encode(buffer, 0, NULL, nonce, 8));
    testrun(!ov_turn_attr_data_encode(buffer, size, NULL, NULL, 8));
    testrun(!ov_turn_attr_data_encode(buffer, size, NULL, nonce, 0));

    memset(buf, 0, size);
    testrun(ov_turn_attr_data_encode(buffer, size, NULL, nonce, 8));
    testrun(TURN_ATTR_DATA == ov_stun_attribute_get_type(buffer, size));
    testrun(8 == ov_stun_attribute_get_length(buffer, size));
    testrun(0 == strncmp((char *)buffer + 4, (char *)nonce, 8));

    memset(buf, 0, size);
    testrun(ov_turn_attr_data_encode(buffer, size, NULL, nonce, 2));
    testrun(TURN_ATTR_DATA == ov_stun_attribute_get_type(buffer, size));
    testrun(2 == ov_stun_attribute_get_length(buffer, size));
    testrun(0 == strncmp((char *)buffer + 4, (char *)nonce, 2));

    // size to small
    memset(buf, 0, size);
    testrun(!ov_turn_attr_data_encode(buffer, 7, NULL, nonce, 2));
    testrun(ov_turn_attr_data_encode(buffer, 8, &next, nonce, 2));
    testrun(TURN_ATTR_DATA == ov_stun_attribute_get_type(buffer, size));
    testrun(2 == ov_stun_attribute_get_length(buffer, size));
    testrun(0 == strncmp((char *)buffer + 4, (char *)nonce, 2));
    testrun(next == buffer + 4 + 4);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_attr_data_decode() {

    size_t size = 100;
    uint8_t buf[size];
    uint8_t *buffer = buf;

    const uint8_t *name = NULL;
    size_t length = 0;
    memset(buf, 0, size);

    testrun(memcpy(buffer, "1234ABCDEFGH", 12));
    testrun(ov_stun_attribute_set_type(buffer, size, TURN_ATTR_DATA));
    testrun(ov_stun_attribute_set_length(buffer, size, 2));

    testrun(!ov_turn_attr_data_decode(NULL, size, NULL, NULL));
    testrun(!ov_turn_attr_data_decode(NULL, size, &name, &length));
    testrun(!ov_turn_attr_data_decode(buffer, 0, &name, &length));
    testrun(!ov_turn_attr_data_decode(buffer, size, NULL, &length));
    testrun(!ov_turn_attr_data_decode(buffer, size, &name, NULL));

    testrun(ov_turn_attr_data_decode(buffer, size, &name, &length));
    testrun(name == buffer + 4);
    testrun(length == 2);

    // frame invalid
    testrun(ov_stun_attribute_set_length(buffer, size, 0));
    testrun(!ov_turn_attr_data_decode(buffer, size, &name, &length));
    testrun(ov_stun_attribute_set_length(buffer, size, 1));
    testrun(ov_turn_attr_data_decode(buffer, size, &name, &length));
    testrun(name == buffer + 4);
    testrun(length == 1);

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
    testrun_test(test_ov_turn_attr_is_data);
    testrun_test(test_ov_turn_attr_data_encoding_length);
    testrun_test(test_ov_turn_attr_data_encode);
    testrun_test(test_ov_turn_attr_data_decode);

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
