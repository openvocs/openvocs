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
        @file           ov_stun_attributes_rfc5245_test.c
        @author         Markus Toepfer

        @date           2018-11-21

        @ingroup        ov_ice

        @brief          Unit tests of RFC 5245 STUN attributes.

        ------------------------------------------------------------------------
*/
#include "ov_stun_attributes_rfc5245.c"
#include <ov_arch/ov_byteorder.h>
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_stun_attribute_frame_is_priority() {

  size_t size = 100;
  uint8_t buffer[size];
  memset(buffer, 0, size);

  testrun(!ov_stun_attribute_frame_is_priority(NULL, 0));
  testrun(!ov_stun_attribute_frame_is_priority(buffer, 0));
  testrun(!ov_stun_attribute_frame_is_priority(NULL, size));

  // empty buffer
  testrun(!ov_stun_attribute_frame_is_priority(buffer, size));

  // correct buffer
  testrun(ov_stun_attribute_set_type(buffer, 4, ICE_PRIORITY));
  testrun(ov_stun_attribute_set_length(buffer, 4, 4));
  testrun(ov_stun_attribute_frame_is_priority(buffer, size));

  // check buffer length
  testrun(!ov_stun_attribute_frame_is_priority(buffer, 6));
  testrun(!ov_stun_attribute_frame_is_priority(buffer, 7));
  testrun(ov_stun_attribute_frame_is_priority(buffer, 8));
  testrun(ov_stun_attribute_frame_is_priority(buffer, 9));

  // check attribute type
  testrun(ov_stun_attribute_set_type(buffer, 4, 1));
  testrun(!ov_stun_attribute_frame_is_priority(buffer, size));

  // check attribute length
  testrun(ov_stun_attribute_set_type(buffer, 4, ICE_PRIORITY));
  testrun(ov_stun_attribute_set_length(buffer, 4, 3));
  testrun(!ov_stun_attribute_frame_is_priority(buffer, size));
  testrun(ov_stun_attribute_set_length(buffer, 4, 4));
  testrun(ov_stun_attribute_frame_is_priority(buffer, size));
  testrun(ov_stun_attribute_set_length(buffer, 4, 5));
  testrun(!ov_stun_attribute_frame_is_priority(buffer, size));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_attribute_frame_is_use_candidate() {

  size_t size = 100;
  uint8_t buffer[size];
  memset(buffer, 0, size);

  testrun(!ov_stun_attribute_frame_is_use_candidate(NULL, 0));
  testrun(!ov_stun_attribute_frame_is_use_candidate(buffer, 0));
  testrun(!ov_stun_attribute_frame_is_use_candidate(NULL, size));

  // empty buffer
  testrun(!ov_stun_attribute_frame_is_use_candidate(buffer, size));

  // correct buffer
  testrun(ov_stun_attribute_set_type(buffer, 4, ICE_USE_CANDIDATE));
  testrun(ov_stun_attribute_set_length(buffer, 4, 0));
  testrun(ov_stun_attribute_frame_is_use_candidate(buffer, size));

  // check buffer length
  testrun(!ov_stun_attribute_frame_is_use_candidate(buffer, 2));
  testrun(!ov_stun_attribute_frame_is_use_candidate(buffer, 3));
  testrun(ov_stun_attribute_frame_is_use_candidate(buffer, 4));
  testrun(ov_stun_attribute_frame_is_use_candidate(buffer, 5));

  // check attribute type
  testrun(ov_stun_attribute_set_type(buffer, 4, 1));
  testrun(!ov_stun_attribute_frame_is_use_candidate(buffer, size));

  // check attribute length
  testrun(ov_stun_attribute_set_type(buffer, 4, ICE_USE_CANDIDATE));
  testrun(ov_stun_attribute_set_length(buffer, 4, 1));
  testrun(!ov_stun_attribute_frame_is_use_candidate(buffer, size));
  testrun(ov_stun_attribute_set_length(buffer, 4, 2));
  testrun(!ov_stun_attribute_frame_is_use_candidate(buffer, size));
  testrun(ov_stun_attribute_set_length(buffer, 4, 0));
  testrun(ov_stun_attribute_frame_is_use_candidate(buffer, size));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_attribute_frame_is_ice_controlled() {

  size_t size = 100;
  uint8_t buffer[size];
  memset(buffer, 0, size);

  testrun(!ov_stun_attribute_frame_is_ice_controlled(NULL, 0));
  testrun(!ov_stun_attribute_frame_is_ice_controlled(buffer, 0));
  testrun(!ov_stun_attribute_frame_is_ice_controlled(NULL, size));

  // empty buffer
  testrun(!ov_stun_attribute_frame_is_ice_controlled(buffer, size));

  // correct buffer
  testrun(ov_stun_attribute_set_type(buffer, 4, ICE_CONTROLLED));
  testrun(ov_stun_attribute_set_length(buffer, 4, 8));
  testrun(ov_stun_attribute_frame_is_ice_controlled(buffer, size));

  // check buffer length
  testrun(!ov_stun_attribute_frame_is_ice_controlled(buffer, 10));
  testrun(!ov_stun_attribute_frame_is_ice_controlled(buffer, 11));
  testrun(ov_stun_attribute_frame_is_ice_controlled(buffer, 12));
  testrun(ov_stun_attribute_frame_is_ice_controlled(buffer, 13));

  // check attribute type
  testrun(ov_stun_attribute_set_type(buffer, 4, 1));
  testrun(!ov_stun_attribute_frame_is_ice_controlled(buffer, size));

  // check attribute length
  testrun(ov_stun_attribute_set_type(buffer, 4, ICE_CONTROLLED));
  testrun(ov_stun_attribute_set_length(buffer, 4, 7));
  testrun(!ov_stun_attribute_frame_is_ice_controlled(buffer, size));
  testrun(ov_stun_attribute_set_length(buffer, 4, 9));
  testrun(!ov_stun_attribute_frame_is_ice_controlled(buffer, size));
  testrun(ov_stun_attribute_set_length(buffer, 4, 8));
  testrun(ov_stun_attribute_frame_is_ice_controlled(buffer, size));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_attribute_frame_is_ice_controlling() {

  size_t size = 100;
  uint8_t buffer[size];
  memset(buffer, 0, size);

  testrun(!ov_stun_attribute_frame_is_ice_controlling(NULL, 0));
  testrun(!ov_stun_attribute_frame_is_ice_controlling(buffer, 0));
  testrun(!ov_stun_attribute_frame_is_ice_controlling(NULL, size));

  // empty buffer
  testrun(!ov_stun_attribute_frame_is_ice_controlling(buffer, size));

  // correct buffer
  testrun(ov_stun_attribute_set_type(buffer, 4, ICE_CONTROLLING));
  testrun(ov_stun_attribute_set_length(buffer, 4, 8));
  testrun(ov_stun_attribute_frame_is_ice_controlling(buffer, size));

  // check buffer length
  testrun(!ov_stun_attribute_frame_is_ice_controlling(buffer, 10));
  testrun(!ov_stun_attribute_frame_is_ice_controlling(buffer, 11));
  testrun(ov_stun_attribute_frame_is_ice_controlling(buffer, 12));
  testrun(ov_stun_attribute_frame_is_ice_controlling(buffer, 13));

  // check attribute type
  testrun(ov_stun_attribute_set_type(buffer, 4, 1));
  testrun(!ov_stun_attribute_frame_is_ice_controlling(buffer, size));

  // check attribute length
  testrun(ov_stun_attribute_set_type(buffer, 4, ICE_CONTROLLING));
  testrun(ov_stun_attribute_set_length(buffer, 4, 7));
  testrun(!ov_stun_attribute_frame_is_ice_controlling(buffer, size));
  testrun(ov_stun_attribute_set_length(buffer, 4, 9));
  testrun(!ov_stun_attribute_frame_is_ice_controlling(buffer, size));
  testrun(ov_stun_attribute_set_length(buffer, 4, 8));
  testrun(ov_stun_attribute_frame_is_ice_controlling(buffer, size));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_ice_priority_encoding_length() {

  testrun(8 == ov_stun_ice_priority_encoding_length());

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_ice_use_candidate_encoding_length() {

  testrun(4 == ov_stun_ice_use_candidate_encoding_length());

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_ice_controlled_encoding_length() {

  testrun(12 == ov_stun_ice_controlled_encoding_length());

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_ice_controlling_encoding_length() {

  testrun(12 == ov_stun_ice_controlling_encoding_length());

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_ice_priority_encode() {

  size_t size = 100;
  uint8_t buffer[size];
  memset(buffer, 0, size);
  uint8_t *next = NULL;

  testrun(!ov_stun_ice_priority_encode(NULL, 0, NULL, 0));
  testrun(!ov_stun_ice_priority_encode(buffer, 0, NULL, 0));
  testrun(!ov_stun_ice_priority_encode(NULL, 8, NULL, 0));

  // buffer to small
  testrun(!ov_stun_ice_priority_encode(buffer, 7, NULL, 0));

  testrun(ov_stun_ice_priority_encode(buffer, 8, NULL, 0));
  testrun(ICE_PRIORITY == ov_stun_attribute_get_type(buffer, 4));
  testrun(4 == ov_stun_attribute_get_length(buffer, 4));
  testrun(0 == ntohl(*(uint32_t *)(buffer + 4)));

  testrun(ov_stun_ice_priority_encode(buffer, 8, NULL, 1));
  testrun(ICE_PRIORITY == ov_stun_attribute_get_type(buffer, 4));
  testrun(4 == ov_stun_attribute_get_length(buffer, 4));
  testrun(1 == ntohl(*(uint32_t *)(buffer + 4)));

  testrun(ov_stun_ice_priority_encode(buffer, 8, &next, 0xface));
  testrun(ICE_PRIORITY == ov_stun_attribute_get_type(buffer, 4));
  testrun(4 == ov_stun_attribute_get_length(buffer, 4));
  testrun(0xface == ntohl(*(uint32_t *)(buffer + 4)));
  testrun(next == buffer + 8);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_ice_use_candidate_encode() {

  size_t size = 100;
  uint8_t buffer[size];
  memset(buffer, 0, size);
  uint8_t *next = NULL;

  testrun(!ov_stun_ice_use_candidate_encode(NULL, 0, NULL));
  testrun(!ov_stun_ice_use_candidate_encode(buffer, 0, NULL));
  testrun(!ov_stun_ice_use_candidate_encode(NULL, 8, NULL));

  // buffer to small
  testrun(!ov_stun_ice_use_candidate_encode(buffer, 3, NULL));

  testrun(ov_stun_ice_use_candidate_encode(buffer, 4, NULL));
  testrun(ICE_USE_CANDIDATE == ov_stun_attribute_get_type(buffer, 4));
  testrun(0 == ov_stun_attribute_get_length(buffer, 4));

  testrun(ov_stun_ice_use_candidate_encode(buffer, 4, &next));
  testrun(ICE_USE_CANDIDATE == ov_stun_attribute_get_type(buffer, 4));
  testrun(0 == ov_stun_attribute_get_length(buffer, 4));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_ice_controlled_encode() {

  size_t size = 100;
  uint8_t buffer[size];
  memset(buffer, 0, size);
  uint8_t *next = NULL;

  testrun(!ov_stun_ice_controlled_encode(NULL, 0, NULL, 0));
  testrun(!ov_stun_ice_controlled_encode(buffer, 0, NULL, 0));
  testrun(!ov_stun_ice_controlled_encode(NULL, 8, NULL, 0));

  // buffer to small
  testrun(!ov_stun_ice_controlled_encode(buffer, 11, NULL, 0));

  testrun(ov_stun_ice_controlled_encode(buffer, 12, NULL, 0));
  testrun(ICE_CONTROLLED == ov_stun_attribute_get_type(buffer, 4));
  testrun(8 == ov_stun_attribute_get_length(buffer, 4));
  testrun(0 == *(uint64_t *)(buffer + 4));

  testrun(ov_stun_ice_controlled_encode(buffer, 12, NULL, 1));
  testrun(ICE_CONTROLLED == ov_stun_attribute_get_type(buffer, 4));
  testrun(8 == ov_stun_attribute_get_length(buffer, 4));
  testrun(1 == OV_BE64TOH(*(uint64_t *)(buffer + 4)));

  testrun(ov_stun_ice_controlled_encode(buffer, 12, &next, 0xface));
  testrun(ICE_CONTROLLED == ov_stun_attribute_get_type(buffer, 4));
  testrun(8 == ov_stun_attribute_get_length(buffer, 4));
  testrun(0xface == OV_BE64TOH(*(uint64_t *)(buffer + 4)));
  testrun(next == buffer + 12);

  testrun(ov_stun_ice_controlled_encode(buffer, size, &next, 0xface1234));
  testrun(ICE_CONTROLLED == ov_stun_attribute_get_type(buffer, 4));
  testrun(8 == ov_stun_attribute_get_length(buffer, 4));
  testrun(0xface1234 == OV_BE64TOH(*(uint64_t *)(buffer + 4)));
  testrun(next == buffer + 12);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_ice_controlling_encode() {

  size_t size = 100;
  uint8_t buffer[size];
  memset(buffer, 0, size);
  uint8_t *next = NULL;

  testrun(!ov_stun_ice_controlling_encode(NULL, 0, NULL, 0));
  testrun(!ov_stun_ice_controlling_encode(buffer, 0, NULL, 0));
  testrun(!ov_stun_ice_controlling_encode(NULL, 8, NULL, 0));

  // buffer to small
  testrun(!ov_stun_ice_controlling_encode(buffer, 11, NULL, 0));

  testrun(ov_stun_ice_controlling_encode(buffer, 12, NULL, 0));
  testrun(ICE_CONTROLLING == ov_stun_attribute_get_type(buffer, 4));
  testrun(8 == ov_stun_attribute_get_length(buffer, 4));
  testrun(0 == OV_BE64TOH(*(uint64_t *)(buffer + 4)));

  testrun(ov_stun_ice_controlling_encode(buffer, 12, NULL, 1));
  testrun(ICE_CONTROLLING == ov_stun_attribute_get_type(buffer, 4));
  testrun(8 == ov_stun_attribute_get_length(buffer, 4));
  testrun(1 == OV_BE64TOH(*(uint64_t *)(buffer + 4)));

  testrun(ov_stun_ice_controlling_encode(buffer, 12, &next, 0xface));
  testrun(ICE_CONTROLLING == ov_stun_attribute_get_type(buffer, 4));
  testrun(8 == ov_stun_attribute_get_length(buffer, 4));
  testrun(0xface == OV_BE64TOH(*(uint64_t *)(buffer + 4)));
  testrun(next == buffer + 12);

  testrun(ov_stun_ice_controlling_encode(buffer, size, &next, 0xface1234));
  testrun(ICE_CONTROLLING == ov_stun_attribute_get_type(buffer, 4));
  testrun(8 == ov_stun_attribute_get_length(buffer, 4));
  testrun(0xface1234 == OV_BE64TOH(*(uint64_t *)(buffer + 4)));
  testrun(next == buffer + 12);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_ice_priority_decode() {

  size_t size = 100;
  uint8_t buffer[size];
  memset(buffer, 0, size);

  uint32_t number = 0;

  testrun(!ov_stun_ice_priority_decode(NULL, 0, NULL));
  testrun(!ov_stun_ice_priority_decode(buffer, 0, &number));
  testrun(!ov_stun_ice_priority_decode(NULL, 8, &number));
  testrun(!ov_stun_ice_priority_decode(buffer, 8, NULL));

  testrun(ov_stun_ice_priority_encode(buffer, 8, NULL, 0));
  testrun(ICE_PRIORITY == ov_stun_attribute_get_type(buffer, 4));
  testrun(4 == ov_stun_attribute_get_length(buffer, 4));
  testrun(0 == ntohl(*(uint32_t *)(buffer + 4)));

  // buffer to small
  testrun(!ov_stun_ice_priority_decode(buffer, 7, &number));

  testrun(ov_stun_ice_priority_decode(buffer, 8, &number));
  testrun(number == 0);

  testrun(ov_stun_ice_priority_encode(buffer, 8, NULL, 0x12345678));
  testrun(ov_stun_ice_priority_decode(buffer, 8, &number));
  testrun(number == 0x12345678);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_ice_controlled_decode() {

  size_t size = 100;
  uint8_t buffer[size];
  memset(buffer, 0, size);

  uint64_t number = 0;

  testrun(!ov_stun_ice_controlled_decode(NULL, 0, NULL));
  testrun(!ov_stun_ice_controlled_decode(buffer, 0, &number));
  testrun(!ov_stun_ice_controlled_decode(NULL, 12, &number));
  testrun(!ov_stun_ice_controlled_decode(buffer, 12, NULL));

  testrun(ov_stun_ice_controlled_encode(buffer, 12, NULL, 0));
  testrun(ICE_CONTROLLED == ov_stun_attribute_get_type(buffer, 4));
  testrun(8 == ov_stun_attribute_get_length(buffer, 4));
  testrun(0 == ntohl(*(uint32_t *)(buffer + 4)));

  // buffer to small
  testrun(!ov_stun_ice_controlled_decode(buffer, 11, &number));

  testrun(ov_stun_ice_controlled_decode(buffer, 12, &number));
  testrun(number == 0);

  testrun(
      ov_stun_ice_controlled_encode(buffer, size, NULL, 0x1234567812345678));
  testrun(ov_stun_ice_controlled_decode(buffer, size, &number));
  testrun(number == 0x1234567812345678);

  testrun(
      ov_stun_ice_controlled_encode(buffer, size, NULL, 0xffffffffffffffff));
  testrun(ov_stun_ice_controlled_decode(buffer, size, &number));
  testrun(number == 0xffffffffffffffff);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_ice_controlling_decode() {

  size_t size = 100;
  uint8_t buffer[size];
  memset(buffer, 0, size);

  uint64_t number = 0;

  testrun(!ov_stun_ice_controlling_decode(NULL, 0, NULL));
  testrun(!ov_stun_ice_controlling_decode(buffer, 0, &number));
  testrun(!ov_stun_ice_controlling_decode(NULL, 12, &number));
  testrun(!ov_stun_ice_controlling_decode(buffer, 12, NULL));

  testrun(ov_stun_ice_controlling_encode(buffer, 12, NULL, 0));
  testrun(ICE_CONTROLLING == ov_stun_attribute_get_type(buffer, 4));
  testrun(8 == ov_stun_attribute_get_length(buffer, 4));
  testrun(0 == ntohl(*(uint32_t *)(buffer + 4)));

  // buffer to small
  testrun(!ov_stun_ice_controlling_decode(buffer, 11, &number));

  testrun(ov_stun_ice_controlling_decode(buffer, 12, &number));
  testrun(number == 0);

  testrun(
      ov_stun_ice_controlling_encode(buffer, size, NULL, 0x1234567812345678));
  testrun(ov_stun_ice_controlling_decode(buffer, size, &number));
  testrun(number == 0x1234567812345678);

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

  testrun_test(test_ov_stun_attribute_frame_is_priority);
  testrun_test(test_ov_stun_attribute_frame_is_use_candidate);
  testrun_test(test_ov_stun_attribute_frame_is_ice_controlled);
  testrun_test(test_ov_stun_attribute_frame_is_ice_controlling);

  testrun_test(test_ov_stun_ice_priority_encoding_length);
  testrun_test(test_ov_stun_ice_use_candidate_encoding_length);
  testrun_test(test_ov_stun_ice_controlled_encoding_length);
  testrun_test(test_ov_stun_ice_controlling_encoding_length);

  testrun_test(test_ov_stun_ice_priority_encode);
  testrun_test(test_ov_stun_ice_use_candidate_encode);
  testrun_test(test_ov_stun_ice_controlled_encode);
  testrun_test(test_ov_stun_ice_controlling_encode);

  testrun_test(test_ov_stun_ice_priority_decode);
  testrun_test(test_ov_stun_ice_controlled_decode);
  testrun_test(test_ov_stun_ice_controlling_decode);

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
