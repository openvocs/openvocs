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
        @file           ov_turn_attr_icmp_test.c
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#include "ov_turn_attr_icmp.c"
#include <ov_test/testrun.h>

/*      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_turn_attr_is_icmp() {

  uint8_t buffer[100] = {0};

  testrun(!ov_turn_attr_is_icmp(NULL, 10));
  testrun(!ov_turn_attr_is_icmp(buffer, 0));
  testrun(!ov_turn_attr_is_icmp(buffer, 7));
  testrun(!ov_turn_attr_is_icmp(buffer, 10));

  testrun(ov_stun_attribute_set_type(buffer, 4, TURN_ICMP));
  testrun(ov_stun_attribute_set_length(buffer, 4, 8));
  testrun(ov_turn_attr_is_icmp(buffer, 100));
  testrun(ov_turn_attr_is_icmp(buffer, 13));
  testrun(ov_turn_attr_is_icmp(buffer, 12));
  testrun(!ov_turn_attr_is_icmp(buffer, 11));

  testrun(ov_stun_attribute_set_length(buffer, 4, 4));
  testrun(!ov_turn_attr_is_icmp(buffer, 100));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_attr_icmp_encoding_length() {

  testrun(12 == ov_turn_attr_icmp_encoding_length());
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_attr_icmp_encode() {

  uint8_t buffer[100] = {0};
  uint8_t *next = NULL;

  testrun(ov_turn_attr_icmp_encode(buffer, 100, &next, 1, 2, 3));
  testrun(next = buffer + 12);
  testrun(buffer[0] == 0x80);
  testrun(buffer[1] == 0x04);
  testrun(buffer[2] == 0x00);
  testrun(buffer[3] == 0x08);
  testrun(buffer[4] == 0x00);
  testrun(buffer[5] == 0x00);
  testrun(buffer[6] == 0x01);
  testrun(buffer[7] == 0x02);
  testrun(buffer[8] == 0x00);
  testrun(buffer[9] == 0x00);
  testrun(buffer[10] == 0x00);
  testrun(buffer[11] == 0x03);
  testrun(buffer[12] == 0x00);

  testrun(ov_turn_attr_icmp_encode(buffer, 100, &next, 1, 2, 0xabcdef));
  testrun(next = buffer + 12);
  testrun(buffer[0] == 0x80);
  testrun(buffer[1] == 0x04);
  testrun(buffer[2] == 0x00);
  testrun(buffer[3] == 0x08);
  testrun(buffer[4] == 0x00);
  testrun(buffer[5] == 0x00);
  testrun(buffer[6] == 0x01);
  testrun(buffer[7] == 0x02);
  testrun(buffer[8] == 0x00);
  testrun(buffer[9] == 0xab);
  testrun(buffer[10] == 0xcd);
  testrun(buffer[11] == 0xef);
  testrun(buffer[12] == 0x00);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_attr_icmp_decode() {

  uint8_t buffer[100] = {0};

  uint8_t type = 0;
  uint8_t code = 0;
  uint32_t error = 0;

  testrun(ov_turn_attr_icmp_encode(buffer, 100, NULL, 1, 2, 3));

  testrun(ov_turn_attr_icmp_decode(buffer, 100, &type, &code, &error));
  testrun(1 == type);
  testrun(2 == code);
  testrun(3 == error);

  testrun(!ov_turn_attr_icmp_decode(NULL, 100, &type, &code, &error));
  testrun(!ov_turn_attr_icmp_decode(buffer, 100, NULL, &code, &error));
  testrun(!ov_turn_attr_icmp_decode(buffer, 100, &type, NULL, &error));
  testrun(!ov_turn_attr_icmp_decode(buffer, 100, &type, &code, NULL));
  testrun(!ov_turn_attr_icmp_decode(buffer, 11, &type, &code, &error));

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
  testrun_test(test_ov_turn_attr_is_icmp);

  testrun_test(test_ov_turn_attr_icmp_encoding_length);
  testrun_test(test_ov_turn_attr_icmp_encode);
  testrun_test(test_ov_turn_attr_icmp_decode);

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