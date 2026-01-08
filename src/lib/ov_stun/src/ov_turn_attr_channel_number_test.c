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
        @file           ov_turn_attr_channel_number_test.c
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#include "ov_turn_attr_channel_number.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_turn_attr_is_channel_number() {

    uint8_t buffer[100] = {0};

    testrun(!ov_turn_attr_is_channel_number(NULL, 10));
    testrun(!ov_turn_attr_is_channel_number(buffer, 0));
    testrun(!ov_turn_attr_is_channel_number(buffer, 7));
    testrun(!ov_turn_attr_is_channel_number(buffer, 10));

    testrun(ov_stun_attribute_set_type(buffer, 4, TURN_CHANNEL_NUMBER));
    testrun(ov_stun_attribute_set_length(buffer, 4, 4));
    testrun(ov_turn_attr_is_channel_number(buffer, 100));
    testrun(ov_turn_attr_is_channel_number(buffer, 9));
    testrun(ov_turn_attr_is_channel_number(buffer, 8));
    testrun(!ov_turn_attr_is_channel_number(buffer, 7));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_attr_channel_number_encoding_length() {

    testrun(8 == ov_turn_attr_channel_number_encoding_length());
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_attr_channel_number_encode() {

    uint8_t buffer[100] = {0};
    uint8_t *next = NULL;
    uint16_t out = 0;

    testrun(ov_stun_attribute_set_type(buffer, 4, TURN_CHANNEL_NUMBER));
    testrun(ov_stun_attribute_set_length(buffer, 4, 4));

    testrun(ov_turn_attr_channel_number_encode(buffer, 100, &next, 1));
    testrun(next = buffer + 8);
    testrun(buffer[0] == 0x00);
    testrun(buffer[1] == 0x0C);
    testrun(buffer[2] == 0x00);
    testrun(buffer[3] == 0x04);
    testrun(buffer[4] == 0x00);
    testrun(buffer[5] == 0x01);
    testrun(buffer[6] == 0x00);
    testrun(buffer[7] == 0x00);
    testrun(buffer[8] == 0x00);
    testrun(ov_turn_attr_channel_number_decode(buffer, 100, &out));
    testrun(1 == out);

    testrun(ov_turn_attr_channel_number_encode(buffer, 100, &next, 0xff));
    testrun(next = buffer + 8);
    testrun(buffer[0] == 0x00);
    testrun(buffer[1] == 0x0C);
    testrun(buffer[2] == 0x00);
    testrun(buffer[3] == 0x04);
    testrun(buffer[4] == 0x00);
    testrun(buffer[5] == 0xff);
    testrun(buffer[6] == 0x00);
    testrun(buffer[7] == 0x00);
    testrun(buffer[8] == 0x00);
    testrun(ov_turn_attr_channel_number_decode(buffer, 100, &out));
    testrun(0xff == out);

    testrun(ov_turn_attr_channel_number_encode(buffer, 100, &next, 0x1ff));
    testrun(next = buffer + 8);
    testrun(buffer[0] == 0x00);
    testrun(buffer[1] == 0x0C);
    testrun(buffer[2] == 0x00);
    testrun(buffer[3] == 0x04);
    testrun(buffer[4] == 0x01);
    testrun(buffer[5] == 0xff);
    testrun(buffer[6] == 0x00);
    testrun(buffer[7] == 0x00);
    testrun(buffer[8] == 0x00);
    testrun(ov_turn_attr_channel_number_decode(buffer, 100, &out));
    testrun(0x1ff == out);

    testrun(ov_turn_attr_channel_number_encode(buffer, 100, &next, 0xff1));
    testrun(next = buffer + 8);
    testrun(buffer[0] == 0x00);
    testrun(buffer[1] == 0x0C);
    testrun(buffer[2] == 0x00);
    testrun(buffer[3] == 0x04);
    testrun(buffer[4] == 0x0f);
    testrun(buffer[5] == 0xf1);
    testrun(buffer[6] == 0x00);
    testrun(buffer[7] == 0x00);
    testrun(buffer[8] == 0x00);
    testrun(ov_turn_attr_channel_number_decode(buffer, 100, &out));
    testrun(0xff1 == out);

    testrun(ov_turn_attr_is_channel_number(buffer, 100));
    testrun(ov_turn_attr_is_channel_number(buffer, 9));
    testrun(ov_turn_attr_is_channel_number(buffer, 8));
    testrun(!ov_turn_attr_is_channel_number(buffer, 7));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_attr_channel_number_decode() {

    uint8_t buffer[100] = {0};
    uint16_t out = 0;

    testrun(ov_stun_attribute_set_type(buffer, 4, TURN_CHANNEL_NUMBER));
    testrun(ov_stun_attribute_set_length(buffer, 4, 4));
    testrun(ov_turn_attr_channel_number_encode(buffer, 100, NULL, 0xff1));

    testrun(ov_turn_attr_channel_number_decode(buffer, 100, &out));
    testrun(out == 0xff1);

    testrun(!ov_turn_attr_channel_number_decode(buffer, 7, &out));
    testrun(ov_turn_attr_channel_number_decode(buffer, 8, &out));

    testrun(ov_stun_attribute_set_length(buffer, 4, 8));
    testrun(!ov_turn_attr_channel_number_decode(buffer, 100, &out));
    testrun(ov_stun_attribute_set_length(buffer, 4, 4));
    testrun(ov_turn_attr_channel_number_decode(buffer, 100, &out));

    testrun(ov_stun_attribute_set_type(buffer, 4, TURN_CHANNEL_NUMBER - 1));
    testrun(!ov_turn_attr_channel_number_decode(buffer, 100, &out));

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
    testrun_test(test_ov_turn_attr_is_channel_number);

    testrun_test(test_ov_turn_attr_channel_number_encoding_length);
    testrun_test(test_ov_turn_attr_channel_number_encode);
    testrun_test(test_ov_turn_attr_channel_number_decode);

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
