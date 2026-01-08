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
        @author         Michael Beer

        ------------------------------------------------------------------------
*/
#include "../include/ov_counter.h"
#include "../include/ov_utils.h"
#include "ov_config_keys.h"

/*----------------------------------------------------------------------------*/

bool ov_counter_reset(ov_counter *self) {

  if (ov_ptr_valid(self, "Cannot initialize counter - invalid counter")) {

    memset(self, 0, sizeof(*self));
    self->since_usecs = ov_time_get_current_time_usecs();
    return true;
  } else {
    return false;
  }
}

/*----------------------------------------------------------------------------*/

double ov_counter_average_per_sec(ov_counter counter) {

  double timediff_msecs =
      ov_time_get_current_time_usecs() - counter.since_usecs;
  timediff_msecs /= 1000;
  double inverse_timediff_msecs = 1.0f / timediff_msecs;
  double average = counter.counter;
  return average * inverse_timediff_msecs * 1000.0;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_counter_to_json(ov_counter self) {

  ov_json_value *jval = ov_json_object();

  ov_json_object_set(jval, OV_KEY_COUNT, ov_json_number(self.counter));
  ov_json_object_set(jval, OV_KEY_AVERAGE_PER_SEC,
                     ov_json_number(ov_counter_average_per_sec(self)));

  return jval;
}

/*----------------------------------------------------------------------------*/
