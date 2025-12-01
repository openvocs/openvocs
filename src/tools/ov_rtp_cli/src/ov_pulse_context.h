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

     \file               ov_pulse_context.h
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2018-02-22

     \ingroup            empty

 **/

#ifndef ov_pulse_h
#define ov_pulse_h

#include <pulse/context.h>

#include <ov_base/ov_buffer.h>
#include <ov_base/ov_json.h>

/******************************************************************************
 *                              TYPEDEFS
 ******************************************************************************/

struct ov_pulse_parameters_struct {

  char const *name;

  uint64_t sample_rate_hertz;
  uint64_t usecs_to_buffer;

  size_t frame_length_usecs;

  char const *server;

  char const *playback_device;
  char const *record_device;
};

typedef struct ov_pulse_parameters_struct ov_pulse_parameters;

typedef struct ov_pulse_context_struct ov_pulse_context;

/******************************************************************************
 *                                 FUNCTIONS
 ******************************************************************************/

ov_pulse_context *ov_pulse_connect(ov_pulse_parameters parameters);

/*---------------------------------------------------------------------------*/

/**
 * Disconnects from server & disposes of context.
 * @return pointer to context on failure or 0 on success
 */
ov_pulse_context *ov_pulse_disconnect(ov_pulse_context *context);

/*---------------------------------------------------------------------------*/

/**
 * @return Number of bytes written or 0 in case of error
 */
bool ov_pulse_write(ov_pulse_context *context, ov_buffer const *input);

/*---------------------------------------------------------------------------*/

/**
 * Will always consume an amount of bytes equivalent to frame_length_usecs.
 * If less bytes are requested, the abundant bytes will be dropped!
 * Thus you should take care to initialize ov_pulse appropriately.
 * @return Number of bytes written or 0 in case of error
 */
size_t ov_pulse_read(ov_pulse_context *context, uint8_t *buffer, size_t nbytes);

/*---------------------------------------------------------------------------*/

ov_json_value *ov_pulse_get_state(ov_pulse_context *restrict context);

/*---------------------------------------------------------------------------*/
#endif /* ov_pulse.h */
