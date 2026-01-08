/***

  Copyright   2018,2020    German Aerospace Center DLR e.V.,
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

#include <ov_base/ov_utils.h>

#include "ov_gateway_pulse.h"
#include "ov_pulse_context.h"

/******************************************************************************
 *                             INTERNAL CONSTANTS
 ******************************************************************************/

#define KEY_UNDERFLOWS "underflows"
#define KEY_NO_DATA "no_data"
#define KEY_FRAMES_PER_WRITE "frames_per_write"

/*---------------------------------------------------------------------------*/

const uint8_t GATEWAY_PULSE_TYPE = 0x50;

/******************************************************************************
 *                                 CONSTANTS
 ******************************************************************************/

const uint64_t USECS_TO_BUFFER_DEFAULT = 2 * 1000 * 1000;
char const *const GATEWAY_PULSE_DEFAULT_APP_NAME = "gateway_pulse";

/******************************************************************************
 *                                  TYPEDEFS
 ******************************************************************************/

typedef struct {

  ov_gateway public;

  char *name;
  ov_pulse_context *pulse_context;

} gateway_pulse;

/******************************************************************************
 *                      PROTOTYPES FOR PRIVATE FUNCTIONS
 ******************************************************************************/

static gateway_pulse *as_gateway_pulse(void *self);

static bool setup(gateway_pulse *self, ov_gateway_pulse_configuration config);

static ov_gateway *impl_free(ov_gateway *self);

static ov_buffer *impl_get_pcm_s16(ov_gateway *self, size_t requested_samples);

static bool impl_put_pcm_s16(ov_gateway *self, const ov_buffer *data);

static ov_json_value *impl_get_state(ov_gateway *self);

/******************************************************************************
 *                                 FUNCTIONS
 ******************************************************************************/

ov_gateway *ov_gateway_pulse_create(ov_gateway_pulse_configuration config) {

  gateway_pulse *pap = calloc(1, sizeof(gateway_pulse));

  ov_gateway *gateway = (ov_gateway *)pap;
  gateway->magic_number = GATEWAY_PULSE_TYPE;

  gateway->get_state = impl_get_state;

  gateway->free = impl_free;

  gateway->get_pcm_s16 = impl_get_pcm_s16;
  gateway->put_pcm_s16 = impl_put_pcm_s16;

  pap->pulse_context = 0;

  pap->name = strdup(GATEWAY_PULSE_DEFAULT_APP_NAME);

  if (!setup(pap, config)) {

    goto error;
  }

  return (ov_gateway *)pap;

error:

  pap = (gateway_pulse *)pap->public.free((ov_gateway *)pap);

  OV_ASSERT(0 == pap);

  return gateway;
}

/******************************************************************************
 *                             PRIVATE FUNCTIONS
 ******************************************************************************/

static gateway_pulse *as_gateway_pulse(void *self) {

  if (0 == self)
    return 0;

  gateway_pulse const *gwp = (gateway_pulse const *)self;

  if (GATEWAY_PULSE_TYPE != gwp->public.magic_number) {
    return 0;
  }

  return self;
}

/*---------------------------------------------------------------------------*/

static bool setup(gateway_pulse *self, ov_gateway_pulse_configuration config) {

  uint64_t usecs_to_buffer = USECS_TO_BUFFER_DEFAULT;

  gateway_pulse *pap = as_gateway_pulse(self);

  if (0 == pap) {

    ov_log_error("No or wrong gateway given");
    goto error;
  }

  if (0 != config.usecs_to_buffer) {

    usecs_to_buffer = config.usecs_to_buffer;
  }

  ov_pulse_context *context = pap->pulse_context;

  if (0 != context) {

    context = ov_pulse_disconnect(context);
  }

  if (0 != context) {

    ov_log_error("Could not disconnect pulse context");
    goto error;
  }

  pap->pulse_context = 0;

  ov_pulse_parameters params = (ov_pulse_parameters){

      .name = pap->name,
      .sample_rate_hertz = config.sample_rate_hertz,
      .usecs_to_buffer = usecs_to_buffer,
      .frame_length_usecs = config.frame_length_usecs,
      .server = config.server,
      .playback_device = config.playback_device,
      .record_device = config.record_device,

  };

  pap->pulse_context = ov_pulse_connect(params);

  if (!pap->pulse_context) {

    ov_log_error("Could not connect to PA server");
    goto error;
  }

  return true;

error:

  return false;
}

/*---------------------------------------------------------------------------*/

static ov_json_value *impl_get_state(ov_gateway *self) {

  gateway_pulse *pap = as_gateway_pulse(self);

  if (0 == pap) {

    ov_log_error("No or wrong gateway given");
    goto error;
  }

  ov_json_value *pulse = 0;

  ov_pulse_context *context = pap->pulse_context;

  if (0 != context) {

    pulse = ov_pulse_get_state(context);
  }

  if (0 == pulse) {

    pulse = ov_json_object();
  }

  return pulse;

error:

  return 0;
}

/*---------------------------------------------------------------------------*/

static ov_gateway *impl_free(ov_gateway *self) {

  gateway_pulse *pap = as_gateway_pulse(self);

  if (0 == pap) {

    ov_log_error("No or wrong gateway given");
    goto error;
  }

  if (0 != pap->pulse_context) {

    pap->pulse_context = ov_pulse_disconnect(pap->pulse_context);
  }

  if (0 != pap->name) {

    free(pap->name);
  }

  pap->name = 0;

  free(pap);

  self = 0;

error:

  return self;
}

/*---------------------------------------------------------------------------*/

static ov_buffer *impl_get_pcm_s16(ov_gateway *self, size_t requested_samples) {

  gateway_pulse *gwp = as_gateway_pulse(self);

  if (0 == gwp) {

    ov_log_error("No or wrong gateway given");
    goto error;
  }

  ov_pulse_context *context = gwp->pulse_context;

  if (0 == context) {

    ov_log_error("Not connected to PA");
    goto error;
  }

  ov_buffer *receiver = ov_buffer_create(requested_samples * sizeof(uint16_t));

  if (0 == receiver) {

    ov_log_error("Could not create buffer for write PCM to");
    goto error;
  }

  OV_ASSERT(requested_samples * sizeof(uint16_t) <= receiver->capacity);

  receiver->length = ov_pulse_read(context, receiver->start,
                                   requested_samples * sizeof(uint16_t));

  return receiver;

error:

  return 0;
}

/*---------------------------------------------------------------------------*/

static bool impl_put_pcm_s16(ov_gateway *self, const ov_buffer *data) {

  if (0 == data) {

    ov_log_error("No data given (0 pointer)");
    goto error;
  }

  gateway_pulse *gwp = as_gateway_pulse(self);

  if (0 == gwp) {

    ov_log_error("No or wrong gateway given");
    goto error;
  }

  ov_pulse_context *context = gwp->pulse_context;

  if (0 == context) {

    ov_log_error("Not connected to PA");
    goto error;
  }

  return ov_pulse_write(context, data);

error:

  return false;
}

/*----------------------------------------------------------------------------*/
