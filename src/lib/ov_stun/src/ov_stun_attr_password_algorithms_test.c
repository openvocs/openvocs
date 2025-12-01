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
        @file           ov_stun_attr_password_algoritms_test.c
        @author         Markus Töpfer

        @date           2022-01-22


        ------------------------------------------------------------------------
*/
#include "ov_stun_attr_password_algorithms.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_stun_attr_is_password_algorithms() {

  size_t size = 1000;
  uint8_t buf[size];
  uint8_t *buffer = buf;

  memset(buf, 'a', size);

  // prepare valid frame
  testrun(
      ov_stun_attribute_set_type(buffer, size, STUN_ATTR_PASSWORD_ALGORITHMS));
  testrun(ov_stun_attribute_set_length(buffer, size, 1));

  testrun(ov_stun_attr_is_password_algorithms(buffer, size));

  testrun(!ov_stun_attr_is_password_algorithms(NULL, size));
  testrun(!ov_stun_attr_is_password_algorithms(buffer, 0));
  testrun(!ov_stun_attr_is_password_algorithms(buffer, 4));

  // min buffer min valid
  testrun(ov_stun_attr_is_password_algorithms(buffer, 8));

  // length < size
  testrun(ov_stun_attribute_set_length(buffer, size, 2));
  testrun(!ov_stun_attr_is_password_algorithms(buffer, 7));
  testrun(ov_stun_attr_is_password_algorithms(buffer, 8));

  // type not nonce
  testrun(ov_stun_attribute_set_type(buffer, size, 0));
  testrun(!ov_stun_attr_is_password_algorithms(buffer, size));
  testrun(
      ov_stun_attribute_set_type(buffer, size, STUN_ATTR_PASSWORD_ALGORITHMS));
  testrun(ov_stun_attr_is_password_algorithms(buffer, size));

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
  testrun_test(test_ov_stun_attr_is_password_algorithms);

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
