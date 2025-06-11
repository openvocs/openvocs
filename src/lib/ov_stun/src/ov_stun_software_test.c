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
        @file           ov_stun_software_test.c
        @author         Markus Toepfer

        @date           2018-10-23

        @ingroup        ov_stun

        @brief          Unit tests of RFC 5389 attribute "software"


        ------------------------------------------------------------------------
*/

#include "ov_stun_software.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_stun_attribute_frame_is_software() {

    size_t size = 1000;
    uint8_t buf[size];
    uint8_t *buffer = buf;

    memset(buf, 'a', size);

    // prepare valid frame
    testrun(ov_stun_attribute_set_type(buffer, size, STUN_SOFTWARE));
    testrun(ov_stun_attribute_set_length(buffer, size, 1));

    testrun(ov_stun_attribute_frame_is_software(buffer, size));

    testrun(!ov_stun_attribute_frame_is_software(NULL, size));
    testrun(!ov_stun_attribute_frame_is_software(buffer, 0));
    testrun(!ov_stun_attribute_frame_is_software(buffer, 4));

    // min buffer min valid
    testrun(ov_stun_attribute_frame_is_software(buffer, 8));

    // length < size
    testrun(ov_stun_attribute_set_length(buffer, size, 2));
    testrun(!ov_stun_attribute_frame_is_software(buffer, 7));
    testrun(ov_stun_attribute_frame_is_software(buffer, 8));

    // type not software
    testrun(ov_stun_attribute_set_type(buffer, size, 0));
    testrun(!ov_stun_attribute_frame_is_software(buffer, size));
    testrun(ov_stun_attribute_set_type(buffer, size, STUN_SOFTWARE));
    testrun(ov_stun_attribute_frame_is_software(buffer, size));

    // check length
    for (size_t i = 1; i < size; i++) {

        testrun(ov_stun_attribute_set_type(buffer, size, STUN_SOFTWARE));
        testrun(ov_stun_attribute_set_length(buffer, size, i));
        if (i <= 763) {
            testrun(ov_stun_attribute_frame_is_software(buffer, size));
        } else {
            testrun(!ov_stun_attribute_frame_is_software(buffer, size));
        }
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_software_encoding_length() {

    uint8_t *name = (uint8_t *)"name";

    testrun(0 == ov_stun_software_encoding_length(NULL, 0));
    testrun(0 == ov_stun_software_encoding_length(name, 0));
    testrun(0 == ov_stun_software_encoding_length(NULL, 10));

    size_t pad = 0;

    for (size_t i = 1; i < 1000; i++) {

        pad = i % 4;
        if (pad != 0) pad = 4 - pad;

        if (i <= 763) {
            testrun(4 + i + pad == ov_stun_software_encoding_length(name, i));
        } else {
            testrun(0 == ov_stun_software_encoding_length(name, i));
        }
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_software_validate() {

    size_t size = 1000;
    uint8_t buf[size];
    uint8_t *buffer = buf;

    memset(buf, 'a', size);

    for (size_t i = 1; i < size; i++) {

        if (i <= 763) {
            testrun(ov_stun_software_validate(buffer, i));
        } else {
            testrun(!ov_stun_software_validate(buffer, i));
        }
    }

    // on UTF8 content
    buf[10] = 0xff;
    testrun(ov_stun_software_validate(buffer, 10));
    testrun(!ov_stun_software_validate(buffer, 11));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_software_encode() {

    size_t size = 100;
    uint8_t buf[size];
    uint8_t *buffer = buf;
    uint8_t *next = NULL;

    uint8_t *software = (uint8_t *)"test1234";

    testrun(!ov_stun_software_encode(NULL, 0, NULL, NULL, 0));
    testrun(!ov_stun_software_encode(NULL, size, NULL, software, 8));
    testrun(!ov_stun_software_encode(buffer, 0, NULL, software, 8));
    testrun(!ov_stun_software_encode(buffer, size, NULL, NULL, 8));
    testrun(!ov_stun_software_encode(buffer, size, NULL, software, 0));

    memset(buf, 0, size);
    testrun(ov_stun_software_encode(buffer, size, NULL, software, 8));
    testrun(STUN_SOFTWARE == ov_stun_attribute_get_type(buffer, size));
    testrun(8 == ov_stun_attribute_get_length(buffer, size));
    testrun(0 == strncmp((char *)buffer + 4, (char *)software, 8));

    memset(buf, 0, size);
    testrun(ov_stun_software_encode(buffer, size, NULL, software, 2));
    testrun(STUN_SOFTWARE == ov_stun_attribute_get_type(buffer, size));
    testrun(2 == ov_stun_attribute_get_length(buffer, size));
    testrun(0 == strncmp((char *)buffer + 4, (char *)software, 2));

    // size to small
    memset(buf, 0, size);
    testrun(!ov_stun_software_encode(buffer, 7, NULL, software, 2));
    testrun(ov_stun_software_encode(buffer, 8, &next, software, 2));
    testrun(STUN_SOFTWARE == ov_stun_attribute_get_type(buffer, size));
    testrun(2 == ov_stun_attribute_get_length(buffer, size));
    testrun(0 == strncmp((char *)buffer + 4, (char *)software, 2));
    testrun(next == buffer + 4 + 4);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_software_decode() {

    size_t size = 100;
    uint8_t buf[size];
    uint8_t *buffer = buf;

    uint8_t *name = NULL;
    size_t length = 0;
    memset(buf, 0, size);

    testrun(memcpy(buffer, "1234ABCDEFGH", 12));
    testrun(ov_stun_attribute_set_type(buffer, size, STUN_SOFTWARE));
    testrun(ov_stun_attribute_set_length(buffer, size, 2));

    testrun(!ov_stun_software_decode(NULL, size, NULL, NULL));
    testrun(!ov_stun_software_decode(NULL, size, &name, &length));
    testrun(!ov_stun_software_decode(buffer, 0, &name, &length));
    testrun(!ov_stun_software_decode(buffer, size, NULL, &length));
    testrun(!ov_stun_software_decode(buffer, size, &name, NULL));

    testrun(ov_stun_software_decode(buffer, size, &name, &length));
    testrun(name == buffer + 4);
    testrun(length == 2);

    // frame invalid
    testrun(ov_stun_attribute_set_length(buffer, size, 0));
    testrun(!ov_stun_software_decode(buffer, size, &name, &length));
    testrun(ov_stun_attribute_set_length(buffer, size, 1));
    testrun(ov_stun_software_decode(buffer, size, &name, &length));
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
    testrun_test(test_ov_stun_attribute_frame_is_software);
    testrun_test(test_ov_stun_software_encoding_length);
    testrun_test(test_ov_stun_software_validate);
    testrun_test(test_ov_stun_software_encode);
    testrun_test(test_ov_stun_software_decode);

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
