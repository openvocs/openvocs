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
        @file           ov_imf_test.c
        @author         Markus Toepfer

        @date           2019-07-19

        @ingroup        ov_core

        @brief          Unit tests of


        ------------------------------------------------------------------------
*/
#include "ov_imf.c"
#include <ov_base/ov_time.h>
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_imf_write_timestamp() {

  size_t size = 1000;
  char buffer[size];
  memset(buffer, 0, size);

  char *next = NULL;

  testrun(!ov_imf_write_timestamp(NULL, 0, NULL));
  testrun(!ov_imf_write_timestamp(buffer, 0, NULL));
  testrun(!ov_imf_write_timestamp(NULL, 30, NULL));
  testrun(!ov_imf_write_timestamp(buffer, 29, NULL));

  struct timeval tv;
  testrun(0 == gettimeofday(&tv, NULL));
  struct tm *current = gmtime(&tv.tv_sec);

  testrun(ov_imf_write_timestamp(buffer, 30, &next));
  struct tm value = ov_imf_timestamp_to_tm(buffer, 30, NULL);
  testrun(next == buffer + 29);

  /*
  ov_log_debug("current %i %i %i %i %i %i \n",
          current->tm_mday,
          current->tm_mon,
          current->tm_year,
          current->tm_hour,
          current->tm_min,
          current->tm_sec);

  ov_log_debug("value %i %i %i %i %i %i",
          value.tm_mday,
          value.tm_mon,
          value.tm_year,
          value.tm_hour,
          value.tm_min,
          value.tm_sec);
  */

  testrun(current->tm_mday == value.tm_mday);
  testrun(current->tm_mon == value.tm_mon);
  testrun(current->tm_year == value.tm_year);
  testrun(current->tm_hour == value.tm_hour);
  testrun(current->tm_min == value.tm_min);
  if (current->tm_sec != value.tm_sec)
    testrun(current->tm_sec == value.tm_sec - 1);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_imf_timestamp_to_tm() {

  struct tm empty = {0};
  size_t size = 1000;
  char buffer[size];
  memset(buffer, 0, size);

  struct timeval tv;
  testrun(0 == gettimeofday(&tv, NULL));
  struct tm *current = gmtime(&tv.tv_sec);

  testrun(ov_imf_write_timestamp(buffer, 30, NULL));

  struct tm value = ov_imf_timestamp_to_tm(buffer, 30, NULL);

  testrun(current->tm_mday == value.tm_mday);
  testrun(current->tm_mon == value.tm_mon);
  testrun(current->tm_year == value.tm_year);
  testrun(current->tm_hour == value.tm_hour);
  testrun(current->tm_min == value.tm_min);
  testrun(current->tm_wday == value.tm_wday);
  if (current->tm_sec != value.tm_sec)
    testrun(current->tm_sec == value.tm_sec - 1);

  value = ov_imf_timestamp_to_tm(NULL, 0, NULL);
  testrun(0 == memcmp(&value, &empty, sizeof(struct tm)));

  value = ov_imf_timestamp_to_tm(buffer, 0, NULL);
  testrun(0 == memcmp(&value, &empty, sizeof(struct tm)));

  value = ov_imf_timestamp_to_tm(NULL, 28, NULL);
  testrun(0 == memcmp(&value, &empty, sizeof(struct tm)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_imf_tm_to_timestamp() {

  size_t size = 1000;
  char buffer[size];
  memset(buffer, 0, size);

  char *next = NULL;

  struct timeval tv;
  testrun(0 == gettimeofday(&tv, NULL));
  struct tm *current = gmtime(&tv.tv_sec);

  testrun(!ov_imf_tm_to_timestamp(NULL, NULL, 0, NULL, NULL));
  testrun(!ov_imf_tm_to_timestamp(current, buffer, 0, NULL, NULL));
  testrun(!ov_imf_tm_to_timestamp(current, NULL, 30, NULL, NULL));
  testrun(!ov_imf_tm_to_timestamp(current, buffer, 29, NULL, NULL));
  testrun(!ov_imf_tm_to_timestamp(NULL, buffer, 30, NULL, NULL));

  testrun(ov_imf_tm_to_timestamp(current, buffer, 30, &next, NULL));
  struct tm value = ov_imf_timestamp_to_tm(buffer, 30, NULL);
  testrun(next == buffer + 29);

  testrun(current->tm_mday == value.tm_mday);
  testrun(current->tm_mon == value.tm_mon);
  testrun(current->tm_year == value.tm_year);
  testrun(current->tm_hour == value.tm_hour);
  testrun(current->tm_min == value.tm_min);
  if (current->tm_sec != value.tm_sec)
    testrun(current->tm_sec == value.tm_sec - 1);

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
  testrun_test(test_ov_imf_write_timestamp);
  testrun_test(test_ov_imf_timestamp_to_tm);
  testrun_test(test_ov_imf_tm_to_timestamp);

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
