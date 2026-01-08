/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_mimetype_test.c
        @author         Markus Toepfer

        @date           2019-07-23

        @ingroup        ov_core

        @brief          Unit tests of


        ------------------------------------------------------------------------
*/
#include "ov_mimetype.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_mimetype_from_file_extension() {

  char *request = "css";
  char *expect = "text/css";

  const char *result =
      ov_mimetype_from_file_extension(request, strlen(request));
  testrun(result);
  testrun(0 == strncmp(result, expect, strlen(expect)));

  request = "gif";
  expect = "image/gif";
  result = ov_mimetype_from_file_extension(request, strlen(request));
  testrun(result);
  testrun(0 == strncmp(result, expect, strlen(expect)));

  request = "png";
  expect = "image/png";
  result = ov_mimetype_from_file_extension(request, strlen(request));
  testrun(result);
  testrun(0 == strncmp(result, expect, strlen(expect)));

  request = "webm";
  expect = "video/webm";
  result = ov_mimetype_from_file_extension(request, strlen(request));
  testrun(result);
  testrun(0 == strncmp(result, expect, strlen(expect)));

  request = "7z";
  expect = "application/x-7z-compressed";
  result = ov_mimetype_from_file_extension(request, strlen(request));
  testrun(result);
  testrun(0 == strncmp(result, expect, strlen(expect)));

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
  testrun_test(test_ov_mimetype_from_file_extension);

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
