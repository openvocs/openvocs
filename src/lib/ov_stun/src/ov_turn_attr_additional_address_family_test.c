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
        @file           ov_turn_attr_additional_address_family_test.c
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#include "ov_turn_attr_additional_address_family.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_turn_attr_is_additional_addr_family() {

    uint8_t buffer[100] = {0};

    testrun(!ov_turn_attr_is_additional_addr_family(NULL, 10));
    testrun(!ov_turn_attr_is_additional_addr_family(buffer, 0));
    testrun(!ov_turn_attr_is_additional_addr_family(buffer, 7));
    testrun(!ov_turn_attr_is_additional_addr_family(buffer, 10));

    testrun(
        ov_stun_attribute_set_type(buffer, 4, TURN_ADDITIONAL_ADDRESS_FAMILY));
    testrun(ov_stun_attribute_set_length(buffer, 4, 4));
    testrun(ov_turn_attr_is_additional_addr_family(buffer, 100));
    testrun(ov_turn_attr_is_additional_addr_family(buffer, 9));
    testrun(ov_turn_attr_is_additional_addr_family(buffer, 8));
    testrun(!ov_turn_attr_is_additional_addr_family(buffer, 7));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_attr_additional_addr_family_encoding_length() {

    testrun(8 == ov_turn_attr_additional_addr_family_encoding_length());
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_attr_additional_addr_family_encode() {

    uint8_t buffer[100] = {0};
    uint8_t *next = NULL;
    uint16_t out = 0;

    testrun(
        ov_stun_attribute_set_type(buffer, 4, TURN_ADDITIONAL_ADDRESS_FAMILY));
    testrun(ov_stun_attribute_set_length(buffer, 4, 4));

    testrun(ov_turn_attr_additional_addr_family_encode(buffer, 100, &next, 1));
    testrun(next = buffer + 8);
    testrun(buffer[0] == 0x80);
    testrun(buffer[1] == 0x00);
    testrun(buffer[2] == 0x00);
    testrun(buffer[3] == 0x04);
    testrun(buffer[4] == 0x01);
    testrun(buffer[5] == 0x00);
    testrun(buffer[6] == 0x00);
    testrun(buffer[7] == 0x00);
    testrun(buffer[8] == 0x00);
    testrun(ov_turn_attr_additional_addr_family_decode(buffer, 100, &out));
    testrun(0x01 == out);

    testrun(
        !ov_turn_attr_additional_addr_family_encode(buffer, 100, &next, 0xff));

    testrun(ov_turn_attr_additional_addr_family_encode(buffer, 100, &next, 2));
    testrun(next = buffer + 8);
    testrun(buffer[0] == 0x80);
    testrun(buffer[1] == 0x00);
    testrun(buffer[2] == 0x00);
    testrun(buffer[3] == 0x04);
    testrun(buffer[4] == 0x02);
    testrun(buffer[5] == 0x00);
    testrun(buffer[6] == 0x00);
    testrun(buffer[7] == 0x00);
    testrun(buffer[8] == 0x00);
    testrun(ov_turn_attr_additional_addr_family_decode(buffer, 100, &out));
    testrun(0x02 == out);

    testrun(ov_turn_attr_is_additional_addr_family(buffer, 100));
    testrun(ov_turn_attr_is_additional_addr_family(buffer, 9));
    testrun(ov_turn_attr_is_additional_addr_family(buffer, 8));
    testrun(!ov_turn_attr_is_additional_addr_family(buffer, 7));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_attr_additional_addr_family_decode() {

    uint8_t buffer[100] = {0};
    uint16_t out = 0;

    testrun(
        ov_stun_attribute_set_type(buffer, 4, TURN_ADDITIONAL_ADDRESS_FAMILY));
    testrun(ov_stun_attribute_set_length(buffer, 4, 4));
    testrun(
        ov_turn_attr_additional_addr_family_encode(buffer, 100, NULL, 0x01));

    testrun(ov_turn_attr_additional_addr_family_decode(buffer, 100, &out));
    testrun(out == 0x01);

    testrun(!ov_turn_attr_additional_addr_family_decode(buffer, 7, &out));
    testrun(ov_turn_attr_additional_addr_family_decode(buffer, 8, &out));

    testrun(ov_stun_attribute_set_length(buffer, 4, 8));
    testrun(!ov_turn_attr_additional_addr_family_decode(buffer, 100, &out));
    testrun(ov_stun_attribute_set_length(buffer, 4, 4));
    testrun(ov_turn_attr_additional_addr_family_decode(buffer, 100, &out));

    testrun(ov_stun_attribute_set_type(buffer, 4,
                                       TURN_ADDITIONAL_ADDRESS_FAMILY - 1));
    testrun(!ov_turn_attr_additional_addr_family_decode(buffer, 100, &out));

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
    testrun_test(test_ov_turn_attr_is_additional_addr_family);

    testrun_test(test_ov_turn_attr_additional_addr_family_encoding_length);
    testrun_test(test_ov_turn_attr_additional_addr_family_encode);
    testrun_test(test_ov_turn_attr_additional_addr_family_decode);

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
