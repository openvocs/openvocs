/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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

#include "ov_pcm_gen_pulse.h"
#include "ov_pulse_context.h"
#include <ov_base/ov_utils.h>

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_pcm_gen public;

    ov_pcm_gen_config config;
    ov_pulse_context *context;

} pcm_internal;

/*----------------------------------------------------------------------------*/

static inline size_t get_num_samples(const pcm_internal *restrict pcm_gen) {

    OV_ASSERT(0 != pcm_gen);
    size_t num_samples = pcm_gen->config.frame_length_usecs;
    num_samples /= 1000;
    num_samples *= pcm_gen->config.sample_rate_hertz;
    num_samples /= 1000;

    return num_samples;
}

/*----------------------------------------------------------------------------*/

static ov_buffer *impl_generate_frame(ov_pcm_gen *self) {

    ov_buffer *buffer = 0;

    if (0 == self) {
        ov_log_error("No self (0 pointer)");
        goto error;
    }

    pcm_internal *internal = (pcm_internal *)self;

    if (0 == internal->context) {
        ov_log_error("No Pulse context");
        goto error;
    }

    const size_t num_samples = get_num_samples(internal);
    const size_t length = num_samples * sizeof(int16_t);

    buffer = ov_buffer_create(length);

    OV_ASSERT(0 != buffer);
    OV_ASSERT(0 != buffer->start);
    OV_ASSERT(length <= buffer->capacity);
    buffer->length = length;

    uint8_t *output = buffer->start;

    const size_t bytes_read = ov_pulse_read(internal->context, output, length);

    if (bytes_read != length) {

        ov_log_error("Requested %zu bytes, but only got %zu bytes\n", length,
                     bytes_read);
        memset(output, 0, length);
    }

    return buffer;

error:

    if (0 != buffer) {

        buffer = ov_buffer_free(buffer);
    }

    return 0;
}

/*----------------------------------------------------------------------------*/

void *impl_free(void *self) {

    if (0 == self)
        return 0;

    pcm_internal *internal = self;

    if (0 != internal->context) {
        internal->context = ov_pulse_disconnect(internal->context);
    }

    free(internal);

    return 0;
}

/*----------------------------------------------------------------------------*/

ov_pcm_gen *ov_pcm_gen_pulse_create(ov_pcm_gen_config config,
                                    ov_pcm_gen_pulse *pulse_config) {

    ov_pulse_context *context = 0;

    if (0 == pulse_config) {

        ov_log_error("No configuration given");
        goto error;
    }

    ov_pulse_parameters params = {

        .sample_rate_hertz = config.sample_rate_hertz,
        .usecs_to_buffer = 2000 * 1000,
        .frame_length_usecs = config.frame_length_usecs,

        .server = pulse_config->server,

    };

    context = ov_pulse_connect(params);

    if (0 == context) {
        goto error;
    }

    pcm_internal *internal = calloc(1, sizeof(pcm_internal));

    internal->config = config;
    internal->context = context;
    internal->public = (ov_pcm_gen){
        .generate_frame = impl_generate_frame,
        .free = impl_free,
    };

    return &internal->public;

error:

    OV_ASSERT(0 == context);

    return 0;
}
