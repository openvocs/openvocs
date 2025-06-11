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
        @file           ov_turn_attr_address_error_code_test.c
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#include "ov_turn_attr_address_error_code.c"
#include <ov_test/testrun.h>

int test_ov_turn_attr_is_address_error_code() {

    uint8_t buffer[100] = {0};

    testrun(!ov_turn_attr_is_address_error_code(NULL, 10));
    testrun(!ov_turn_attr_is_address_error_code(buffer, 0));
    testrun(!ov_turn_attr_is_address_error_code(buffer, 7));
    testrun(!ov_turn_attr_is_address_error_code(buffer, 10));

    testrun(ov_stun_attribute_set_type(buffer, 4, TURN_ADDRESS_ERROR_CODE));
    testrun(ov_stun_attribute_set_length(buffer, 4, 4));
    testrun(!ov_turn_attr_is_address_error_code(buffer, 100));
    buffer[4] = 0x01;
    testrun(!ov_turn_attr_is_address_error_code(buffer, 100));
    buffer[7] = 0x01;
    testrun(ov_turn_attr_is_address_error_code(buffer, 100));
    testrun(ov_turn_attr_is_address_error_code(buffer, 12));
    testrun(!ov_turn_attr_is_address_error_code(buffer, 11));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_attr_address_error_code_encoding_length() {

    testrun(12 == ov_turn_attr_address_error_code_encoding_length(4));
    testrun(8 + 10 == ov_turn_attr_address_error_code_encoding_length(10));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_attr_address_error_code_encode() {

    uint8_t buffer[100] = {0};
    uint8_t *next = NULL;

    testrun(ov_stun_attribute_set_type(buffer, 4, TURN_ADDRESS_ERROR_CODE));
    testrun(ov_stun_attribute_set_length(buffer, 4, 4));

    testrun(ov_turn_attr_address_error_code_encode(
        buffer, 100, &next, 1, 102, (uint8_t *)"test", 4));
    testrun(next = buffer + 13);
    testrun(buffer[0] == 0x80);
    testrun(buffer[1] == 0x01);
    testrun(buffer[2] == 0x00);
    testrun(buffer[3] == 0x08);
    testrun(buffer[4] == 0x01);
    testrun(buffer[5] == 0x00);
    testrun(buffer[6] == 0x01);
    testrun(buffer[7] == 0x02);
    testrun(buffer[8] == 't');
    testrun(buffer[9] == 'e');
    testrun(buffer[10] == 's');
    testrun(buffer[11] == 't');
    testrun(buffer[12] == 0);
    testrun(buffer[13] == 0);

    testrun(ov_turn_attr_address_error_code_encode(
        buffer, 100, &next, 0x02, 999, (uint8_t *)"testa", 5));
    testrun(next = buffer + 8);
    testrun(buffer[0] == 0x80);
    testrun(buffer[1] == 0x01);
    testrun(buffer[2] == 0x00);
    testrun(buffer[3] == 0x09);
    testrun(buffer[4] == 0x02);
    testrun(buffer[5] == 0x00);
    testrun(buffer[6] == 0x09);
    testrun(buffer[7] == 0x63);
    testrun(buffer[8] == 't');
    testrun(buffer[9] == 'e');
    testrun(buffer[10] == 's');
    testrun(buffer[11] == 't');
    testrun(buffer[12] == 'a');
    testrun(buffer[13] == 0);
    testrun(next = buffer + 13);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_attr_address_error_code_decode() {

    uint8_t buffer[100] = {0};

    uint8_t family = 0;
    uint16_t code = 0;
    const uint8_t *phrase = NULL;
    size_t len = 0;

    testrun(ov_turn_attr_address_error_code_encode(
        buffer, 100, NULL, 0x02, 999, (uint8_t *)"testa", 5));

    testrun(!ov_turn_attr_address_error_code_decode(
        NULL, 0, NULL, NULL, NULL, NULL));

    testrun(!ov_turn_attr_address_error_code_decode(
        NULL, 100, &family, &code, &phrase, &len));

    testrun(!ov_turn_attr_address_error_code_decode(
        buffer, 100, NULL, &code, &phrase, &len));

    testrun(!ov_turn_attr_address_error_code_decode(
        buffer, 100, &family, NULL, &phrase, &len));

    testrun(!ov_turn_attr_address_error_code_decode(
        buffer, 100, &family, &code, NULL, &len));

    testrun(!ov_turn_attr_address_error_code_decode(
        buffer, 100, &family, &code, &phrase, NULL));

    testrun(ov_turn_attr_address_error_code_decode(
        buffer, 100, &family, &code, &phrase, &len));

    testrun(2 == family);
    testrun(999 == code);
    testrun(5 == len);
    testrun(0 == memcmp("testa", phrase, len));

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
    testrun_test(test_ov_turn_attr_is_address_error_code);

    testrun_test(test_ov_turn_attr_address_error_code_encoding_length);
    testrun_test(test_ov_turn_attr_address_error_code_encode);
    testrun_test(test_ov_turn_attr_address_error_code_decode);

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
