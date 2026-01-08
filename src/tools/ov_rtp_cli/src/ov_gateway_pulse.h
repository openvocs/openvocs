/***

Copyright   2018    German Aerospace Center DLR e.V.,
                        German Space Operations Center (GSOC)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    This file is part of the openvocs project. http://openvocs.org

***/ /**

     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2018-05-09

 **/
/*---------------------------------------------------------------------------*/

#ifndef OV_GATEWAY_PULSE_H
#define OV_GATEWAY_PULSE_H

#include "ov_gateway.h"

typedef struct {

  char *server;
  char *playback_device;
  char *record_device;
  uint64_t usecs_to_buffer;
  uint64_t sample_rate_hertz;
  uint64_t frame_length_usecs;

} ov_gateway_pulse_configuration;

/******************************************************************************
 *                                 FUNCTIONS
 ******************************************************************************/

ov_gateway *ov_gateway_pulse_create(ov_gateway_pulse_configuration cfg);

/*---------------------------------------------------------------------------*/

#endif /* ov_gateway_pulse.h */
