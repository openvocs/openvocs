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
        @file           ov_turn_attr_dont_fragment_test.c
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#include "ov_turn_attr_dont_fragment.c"
#include <ov_test/testrun.h>

int test_ov_turn_attr_is_dont_fragment() {

    uint8_t buffer[100] = {0};

    testrun(!ov_turn_attr_is_dont_fragment(NULL, 10));
    testrun(!ov_turn_attr_is_dont_fragment(buffer, 0));
    testrun(!ov_turn_attr_is_dont_fragment(buffer, 7));
    testrun(!ov_turn_attr_is_dont_fragment(buffer, 10));

    testrun(ov_stun_attribute_set_type(buffer, 4, TURN_DONT_FRAGMENT));
    testrun(ov_stun_attribute_set_length(buffer, 4, 0));
    testrun(ov_turn_attr_is_dont_fragment(buffer, 100));
    testrun(ov_turn_attr_is_dont_fragment(buffer, 9));
    testrun(ov_turn_attr_is_dont_fragment(buffer, 4));
    testrun(!ov_turn_attr_is_dont_fragment(buffer, 3));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_attr_dont_fragment_encoding_length() {

    testrun(4 == ov_turn_attr_dont_fragment_encoding_length());
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_attr_dont_fragment_encode() {

    uint8_t buffer[100] = {0};
    uint8_t *next = NULL;

    testrun(ov_stun_attribute_set_type(buffer, 4, TURN_DONT_FRAGMENT));
    testrun(ov_stun_attribute_set_length(buffer, 4, 0));

    testrun(ov_turn_attr_dont_fragment_encode(buffer, 100, &next));
    testrun(next = buffer + 4);
    testrun(buffer[0] == 0x00);
    testrun(buffer[1] == 0x1A);
    testrun(buffer[2] == 0x00);
    testrun(buffer[3] == 0x00);
    testrun(buffer[4] == 0x00);
    testrun(buffer[5] == 0x00);
    testrun(buffer[6] == 0x00);
    testrun(buffer[7] == 0x00);
    testrun(buffer[8] == 0x00);
    testrun(ov_turn_attr_is_dont_fragment(buffer, 100));

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
    testrun_test(test_ov_turn_attr_is_dont_fragment);

    testrun_test(test_ov_turn_attr_dont_fragment_encoding_length);
    testrun_test(test_ov_turn_attr_dont_fragment_encode);

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
