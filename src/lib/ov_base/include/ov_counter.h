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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

        Small implementation for counters, that is variables that are used
        to count e.g. incoming packets within a certain timespan.
        It provides a reliable method to get average counts per second,
        rebust to counter overflows.

        It provides additional functionality like the average count per sec.

        The counter is initialized to 0 and its start time is started by
        `ov_counter_reset` .


        ------------------------------------------------------------------------
*/
#ifndef OV_COUNTER_H
#define OV_COUNTER_H
/*----------------------------------------------------------------------------*/
#include "ov_json_value.h"
#include "ov_time.h"
#include <inttypes.h>

/*----------------------------------------------------------------------------*/

typedef struct {

  uint32_t counter;
  uint64_t since_usecs;

} ov_counter;

bool ov_counter_reset(ov_counter *self);

#define ov_counter_increase(c, increment)                                      \
  __ov_counter_increase_internal(c, increment)

double ov_counter_average_per_sec(ov_counter counter);

ov_json_value *ov_counter_to_json(ov_counter self);

/*****************************************************************************
                                   INTERNALS
 ****************************************************************************/

#define __ov_counter_increase_internal(c, increment)                           \
  do {                                                                         \
    uint32_t inc32 = (uint32_t)increment;                                      \
    uint32_t diff = UINT32_MAX - c.counter;                                    \
    if (diff > inc32) {                                                        \
      /* No overflow */                                                        \
      c.counter += inc32;                                                      \
    } else {                                                                   \
      /* Counter would overflow -> Reset */                                    \
      c.counter = 0;                                                           \
      c.since_usecs = ov_time_get_current_time_usecs();                        \
    }                                                                          \
  } while (0)

/*----------------------------------------------------------------------------*/
#endif
