/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/
/*----------------------------------------------------------------------------*/

#include "../include/ov_config.h"
#include "ov_counter.c"
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

bool ov_counter_reset(ov_counter *self);

int test_ov_counter_reset() {

  testrun(!ov_counter_reset(0));

  ov_counter counter = {0};

  double now_usecs = ov_time_get_current_time_usecs();
  testrun(ov_counter_reset(&counter));
  testrun(0 == counter.counter);
  testrun(now_usecs <= counter.since_usecs);

  counter.counter = 314;
  counter.since_usecs = 18;

  now_usecs = ov_time_get_current_time_usecs();
  testrun(ov_counter_reset(&counter));
  testrun(0 == counter.counter);
  testrun(now_usecs <= counter.since_usecs);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_counter_increase() {

  ov_counter counter = {0};
  ov_counter_reset(&counter);

  uint64_t usecs = counter.since_usecs;

  ov_counter_increase(counter, 1);
  testrun(1 == counter.counter);

  ov_counter_increase(counter, 17);
  testrun(18 == counter.counter);

  ov_counter_increase(counter, 17);
  testrun(35 == counter.counter);

  ov_counter_increase(counter, UINT32_MAX - 34);
  testrun(0 == counter.counter);
  testrun(usecs <= counter.since_usecs);
  usecs = counter.since_usecs;

  ov_counter_increase(counter, 23);
  testrun(23 == counter.counter);

  ov_counter_increase(counter, UINT32_MAX - counter.counter);
  testrun(0 == counter.counter);
  testrun(usecs <= counter.since_usecs);
  usecs = counter.since_usecs;

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_counter_average_per_sec() {

  ov_counter counter = {0};

  testrun(0.0 == ov_counter_average_per_sec(counter));

  counter.counter = 132;

  double now_usecs = ov_time_get_current_time_usecs();
  double avg_secs = ov_counter_average_per_sec(counter);
  double after_usecs = ov_time_get_current_time_usecs();

  testrun(133.0f * 1000.0f * 1000.0f / now_usecs > avg_secs);
  testrun(131.0f * 1000.0f * 1000.0f / after_usecs < avg_secs);

  counter.since_usecs = ov_time_get_current_time_usecs();

  now_usecs = ov_time_get_current_time_usecs();
  avg_secs = ov_counter_average_per_sec(counter);
  after_usecs = ov_time_get_current_time_usecs();

  testrun(133.0f * 1000.0f * 1000.0f / (now_usecs - counter.since_usecs) >=
          avg_secs);
  testrun(131.0f * 1000.0f * 1000.0f / (after_usecs - counter.since_usecs) <=
          avg_secs);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_counter_to_json() {

  ov_json_value *jval = ov_counter_to_json((ov_counter){0});
  testrun(0 != jval);

  testrun(0 == ov_config_double_or_default(jval, OV_KEY_COUNT, 17));
  testrun(0 == ov_config_double_or_default(jval, OV_KEY_AVERAGE_PER_SEC, 17));

  jval = ov_json_value_free(jval);

  jval = ov_counter_to_json((ov_counter){
      .counter = 39,
      .since_usecs = 0,
  });

  testrun(0 != jval);

  double now_usecs = ov_time_get_current_time_usecs();
  double avg_secs =
      ov_config_double_or_default(jval, OV_KEY_AVERAGE_PER_SEC, 12345);
  double after_usecs = ov_time_get_current_time_usecs();

  testrun(39 == ov_config_double_or_default(jval, OV_KEY_COUNT, 17));
  testrun(40.0f * 1000.0f * 1000.0f / now_usecs > avg_secs);
  testrun(38.0f * 1000.0f * 1000.0f / after_usecs < avg_secs);

  jval = ov_json_value_free(jval);
  testrun(0 == jval);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_counter", test_ov_counter_reset, test_ov_counter_increase,
            test_ov_counter_average_per_sec, test_ov_counter_to_json);
