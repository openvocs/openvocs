/***

  Copyright   2018, 2020    German Aerospace Center DLR e.V.,
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

 ***/
/**


  \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
  \date               2018-08-30

 **/
/*----------------------------------------------------------------------------*/
#include <fcntl.h>
#include <math.h>
#include <ov_base/ov_utils.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_json.h>

#include <ov_codec/ov_codec_factory.h>
#include <ov_codec/ov_codec_pcm16_signed.h>

#include "../include/ov_pcm_gen.h"
#include <ov_base/ov_random.h>

/******************************************************************************
 *                               DATA STRUCTURE
 ******************************************************************************/

typedef struct internal_sinusoids internal_sinusoids;
typedef struct internal_white_noise internal_white_noise;
typedef struct internal_buffer internal_buffer;

typedef struct {

    ov_pcm_gen public;

    ov_pcm_gen_config config;

    union {
        internal_sinusoids *sinusoids;
        internal_white_noise *white_noise;
        internal_buffer *buffer;
        void *specific; /* Convenience */
    };

} internal_pcm_gen;

static inline size_t get_num_samples(const internal_pcm_gen *restrict pcm_gen);

/******************************************************************************
 *                                 SINUSOIDS
 ******************************************************************************/

struct internal_sinusoids {
    ov_pcm_gen_sinusoids parameters;

    struct sinusoid_gen_state {
        double phase;
        double wobble_phase;
    } state;

    double (*get_frequency)(internal_pcm_gen *params);
};

/*----------------------------------------------------------------------------*/

static double get_frequency_constant(internal_pcm_gen *self);
static double get_frequency_wobble(internal_pcm_gen *self);

/*----------------------------------------------------------------------------*/

static internal_sinusoids *
create_internal_sinusoids(ov_pcm_gen_sinusoids *const restrict params) {

    internal_sinusoids *internal = calloc(1, sizeof(internal_sinusoids));

    internal->parameters = *params;

    internal->get_frequency = get_frequency_constant;

    if ((0 != params->wobble.period_secs) &&
        (0 != params->wobble.frequency_disp_hertz)) {
        internal->get_frequency = get_frequency_wobble;
    }

    return internal;
}

/*----------------------------------------------------------------------------*/

static double get_frequency_constant(internal_pcm_gen *self) {

    OV_ASSERT(0 != self);

    internal_sinusoids *params = self->sinusoids;

    OV_ASSERT(0 != params);

    return params->parameters.frequency_hertz;
}

/*---------------------------------------------------------------------------*/

static double get_frequency_wobble(internal_pcm_gen *self) {

    OV_ASSERT(self);
    internal_sinusoids *sinusoids = self->sinusoids;
    OV_ASSERT(0 != sinusoids);
    ov_pcm_gen_sinusoids *params = &sinusoids->parameters;

    double frequency = params->frequency_hertz;

    if (!params->wobble.frequency_disp_hertz) {

        goto finish;
    }

    double phase_correction = sinusoids->state.wobble_phase;

    if (phase_correction > 2.0 * 3.1415) {

        phase_correction -= 2.0 * 3.1415;
    }

    double sr = self->config.sample_rate_hertz;
    sr *= params->wobble.period_secs;

    double sin_argument = 1;
    sin_argument /= sr;

    sin_argument *= 2.0 * 3.1415;
    sin_argument += phase_correction;

    frequency += params->wobble.frequency_disp_hertz * sin(sin_argument);

    sinusoids->state.wobble_phase = sin_argument;

finish:

    return frequency;
}

/*----------------------------------------------------------------------------*/

static ov_buffer *impl_generate_sinusoids(ov_pcm_gen *self) {

    OV_ASSERT(0 != self);
    if (0 == self) {

        ov_log_error("Called without ov_pcm_gen (0 pointer)");
        goto error;
    }

    internal_pcm_gen *pcm_gen = (internal_pcm_gen *)self;

    internal_sinusoids *internal = pcm_gen->sinusoids;

    double sr = pcm_gen->config.sample_rate_hertz;
    const size_t num_samples = get_num_samples(pcm_gen);
    const size_t length = num_samples * sizeof(int16_t);

    OV_ASSERT(0 != length);

    ov_buffer *buffer = ov_buffer_create(length);

    OV_ASSERT(0 != buffer);
    OV_ASSERT(0 != buffer->start);
    OV_ASSERT(length <= buffer->capacity);

    buffer->length = length;

    int16_t *output = (int16_t *)buffer->start;

    double phase = internal->state.phase;
    double amplitude = 0;
    double frequency = 0;

    const double PHASE_SHIFT_PER_SAMPLE = 2.0 * M_PI / sr;

    for (size_t i = 0; i < num_samples; i++) {

        frequency = internal->get_frequency(pcm_gen);
        phase += PHASE_SHIFT_PER_SAMPLE * frequency;

        amplitude = sin(phase) * INT16_MAX;

        if (2.0 * M_PI < phase) {

            phase = asin(sin(phase));
            amplitude = sin(phase) * INT16_MAX;
        }

        *output = (int16_t)amplitude;
        output++;
    }

    internal->state.phase = phase;

    return buffer;

error:

    return 0;
}

/*****************************************************************************
                                  WHITE NOISE
 ****************************************************************************/

struct internal_white_noise {
    uint16_t max_amplitude;
};

/*----------------------------------------------------------------------------*/

static internal_white_noise *
create_internal_white_noise(ov_pcm_gen_white_noise *const params) {

    if (0 == params) {

        ov_log_error("No parameters given");
        goto error;
    }

    if (INT16_MAX < params->max_amplitude) {
        ov_log_error("Amplitude out of range");
        goto error;
    }

    internal_white_noise *wnoise = calloc(1, sizeof(internal_white_noise));
    OV_ASSERT(0 != wnoise);

    wnoise->max_amplitude = params->max_amplitude;

    return wnoise;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

static ov_buffer *impl_generate_white_noise(ov_pcm_gen *self) {

    ov_buffer *buffer = 0;

    OV_ASSERT(0 != self);
    if (0 == self) {

        ov_log_error("Called without ov_pcm_gen (0 pointer)");
        goto error;
    }

    internal_pcm_gen *pcm_gen = (internal_pcm_gen *)self;

    internal_white_noise *internal = pcm_gen->white_noise;

    OV_ASSERT(0 != internal);

    const uint16_t max_amplitude = internal->max_amplitude;

    if (max_amplitude > INT16_MAX) {
        ov_log_error("Max amplitude out of range");
        goto error;
    }

    const size_t num_samples = get_num_samples(pcm_gen);
    const size_t length = num_samples * sizeof(int16_t);

    OV_ASSERT(0 != length);

    buffer = ov_buffer_create(length);

    OV_ASSERT(0 != buffer);
    OV_ASSERT(0 != buffer->start);
    OV_ASSERT(length <= buffer->capacity);

    buffer->length = length;

    int16_t *output = (int16_t *)buffer->start;

    for (size_t i = 0; i < num_samples; ++i) {
        int32_t rval = ov_random_range(0, 2 * max_amplitude);
        rval -= max_amplitude;
        output[i] = (int16_t)rval;
    }

    return buffer;

error:

    buffer = ov_buffer_free(buffer);
    OV_ASSERT(0 == buffer);

    return 0;
}

/******************************************************************************
 *                              FROM_FILE GENERATOR
 ******************************************************************************/

struct internal_buffer {

    uint8_t *buffer;
    uint64_t size;
    uint64_t current_pos;
};

/*----------------------------------------------------------------------------*/

static bool decode_buffer_unsafe(struct internal_buffer *internal,
                                 ov_json_value *codec_config) {

    OV_ASSERT(0 != codec_config);

    bool retval = false;

    uint8_t *input_buffer = 0;

    ov_codec_factory *factory = ov_codec_factory_create_standard();
    ov_codec *codec = 0;

    codec = ov_codec_factory_get_codec_from_json(factory, codec_config, 0);

    if (0 == codec) {
        goto error;
    }

    // TODO: CODECS in general produce more output than input -
    // provide larger output buffer
    input_buffer = internal->buffer;
    internal->buffer = calloc(1, internal->size);

    codec->decode(codec, 0, input_buffer, internal->size, internal->buffer,
                  internal->size);

    retval = true;

error:

    if (0 != input_buffer) {
        free(input_buffer);
        input_buffer = 0;
    }

    if (0 != factory) {
        factory = ov_codec_factory_free(factory);
    }

    if (0 != codec) {
        codec = codec->free(codec);
    }
    OV_ASSERT(0 == factory);
    OV_ASSERT(0 == codec);
    OV_ASSERT(0 == input_buffer);

    return retval;
}

/*----------------------------------------------------------------------------*/

void *create_internal_buffer(const ov_pcm_gen_from_file *config) {

    uint8_t *buffer = 0;

    bool free_codec_config = false;

    internal_buffer *internal = 0;
    int fd = -1;

    if (0 == config) {
        goto error;
    }
    char *path = config->file_name;
    if (0 == path)
        goto error;

    struct stat fstate = {0};

    if ((0 != stat(path, &fstate)) || (!S_ISREG(fstate.st_mode))) {
        ov_log_error("Requested file does not exist: %s", path);
        goto error;
    }

    internal = calloc(1, sizeof(internal_buffer));

    internal->size = fstate.st_size;
    internal->buffer = calloc(1, fstate.st_size);

    fd = open(path, O_RDONLY);

    if (0 > fd) {

        ov_log_error("Could not open file %s: %s", path, strerror(errno));
        goto error;
    }

    if (fstate.st_size != read(fd, internal->buffer, fstate.st_size)) {

        ov_log_error("Could not read entire file: %s", strerror(errno));
        goto error;
    }

    close(fd);
    fd = -1;

    ov_json_value *codec_config = config->codec_config;

    if (0 == codec_config) {

        /* default to PCM_16S_BE */
        char codec_config_str[256] = {0};
        snprintf(codec_config_str, sizeof(codec_config_str) - 1,
                 "{\"%s\":\"%s\"}", OV_KEY_CODEC, ov_codec_pcm16_signed_id());

        codec_config_str[255] = 0;

        codec_config = ov_json_value_from_string(codec_config_str,
                                                 strlen(codec_config_str));

        free_codec_config = true;
    }

    decode_buffer_unsafe(internal, codec_config);

    goto teardown;

error:

    if (0 != buffer) {
        free(buffer);
        buffer = 0;
    }

    if (0 != internal) {
        buffer = internal->buffer;
        free(internal);
        internal = 0;
    }

    OV_ASSERT(0 == internal);

teardown:

    if (free_codec_config) {

        codec_config = codec_config->free(codec_config);
    }

    if (-1 < fd) {
        close(fd);
        fd = -1;
    }

    OV_ASSERT(0 == buffer);
    OV_ASSERT(-1 == fd);

    return internal;
}

/*----------------------------------------------------------------------------*/

static ov_buffer *impl_generate_buffer(ov_pcm_gen *self) {

    if (0 == self) {
        ov_log_error("No self (0 pointer)");
        goto error;
    }

    internal_pcm_gen *pcm_gen = (internal_pcm_gen *)self;

    internal_buffer *internal = pcm_gen->buffer;

    if (0 == internal->buffer) {
        ov_log_error("No buffer to replay");
        goto error;
    }

    const size_t num_samples = get_num_samples(pcm_gen);
    const size_t length = num_samples * sizeof(int16_t);

    ov_buffer *buffer = ov_buffer_create(length);

    OV_ASSERT(0 != buffer);
    OV_ASSERT(0 != buffer->start);
    OV_ASSERT(length <= buffer->capacity);
    buffer->length = length;

    int8_t *output = (int8_t *)buffer->start;

    size_t current_pos = internal->current_pos;
    size_t bufsize = internal->size;

    for (size_t i = 0; i < length; ++i, ++current_pos) {

        if (bufsize <= current_pos) {
            current_pos = 0;
        }

        output[i] = internal->buffer[current_pos];
    }

    internal->current_pos = current_pos;

    return buffer;

error:

    return 0;
}

/******************************************************************************
 *                              HELPER FUNCTIONS
 ******************************************************************************/

static inline size_t get_num_samples(const internal_pcm_gen *restrict pcm_gen) {

    OV_ASSERT(0 != pcm_gen);
    size_t num_samples = pcm_gen->config.frame_length_usecs;

    num_samples /= 1000;
    num_samples *= pcm_gen->config.sample_rate_hertz;
    num_samples /= 1000;

    return num_samples;
}

/******************************************************************************
 *                                 INTERFACE
 ******************************************************************************/

static void *impl_free(void *self) {

    if (0 == self)
        return 0;

    internal_pcm_gen *internal = self;

    void *func = internal->public.generate_frame;

    if (impl_generate_sinusoids == func) {

        internal_sinusoids *s = internal->sinusoids;
        internal->sinusoids = 0;

        if (0 != s) {
            free(s);
            s = 0;
        }

        goto finish;
    }

    if (impl_generate_white_noise == func) {

        internal_white_noise *w = internal->white_noise;
        internal->white_noise = 0;

        if (0 != w) {
            free(w);
            w = 0;
        }

        goto finish;
    }

    if (impl_generate_buffer == func) {

        internal_buffer *sp = internal->buffer;
        internal->buffer = 0;

        if (0 != sp->buffer) {
            free(sp->buffer);
            sp->buffer = 0;
            sp->size = 0;
        }

        free(sp);
        sp = 0;

        goto finish;
    }

finish:

    free(internal);
    internal = 0;

    return internal;
}

/******************************************************************************
 *                              PUBLIC FUNCTIONS
 ******************************************************************************/

ov_pcm_gen *ov_pcm_gen_create(ov_pcm_gen_type type,
                              ov_pcm_gen_config parameters,
                              void *restrict specific) {

    internal_pcm_gen *internal = 0;

    if (0 == specific) {

        ov_log_error("No specific given (0 pointer)");
        goto error;
    }
    internal = calloc(1, sizeof(internal_pcm_gen));

    void *internal_params = 0;
    ov_buffer *(*generate)(ov_pcm_gen *self) = 0;

    switch (type) {

    case OV_SINUSOIDS:

        internal_params = create_internal_sinusoids(specific);
        generate = impl_generate_sinusoids;
        break;

    case OV_WHITE_NOISE:
        internal_params = create_internal_white_noise(specific);
        generate = impl_generate_white_noise;
        break;

    case OV_FROM_FILE:

        internal_params = create_internal_buffer(specific);
        generate = impl_generate_buffer;
        break;

    default:
        OV_ASSERT(!"MUST NEVER HAPPEN");
        ov_log_error("Unknown type");
        return 0;
    }

    if (0 == internal_params)
        goto error;

    if (0 == generate)
        goto error;

    *internal = (internal_pcm_gen){
        .public.generate_frame = generate,
        .public.free = impl_free,
        .config = parameters,
        .specific = internal_params,
    };

    return (ov_pcm_gen *)internal;

error:

    if (0 != internal) {

        free(internal);
        internal = 0;
    }

    return (ov_pcm_gen *)internal;
}

/*----------------------------------------------------------------------------*/

ov_pcm_gen *ov_pcm_gen_free(ov_pcm_gen *generator) {

    if (0 == generator) {
        return 0;
    }

    return generator->free(generator);
}

/*----------------------------------------------------------------------------*/

ov_buffer *ov_pcm_gen_generate_frame(ov_pcm_gen *generator) {

    if (0 == generator) {
        return 0;
    }

    return generator->generate_frame(generator);
}

/*----------------------------------------------------------------------------*/

bool ov_pcm_gen_config_print(FILE *out, ov_pcm_gen_config const *config,
                             size_t indentation_level) {

    if (INT_MAX < indentation_level) {
        return false;
    }

    if (0 == out) {
        return false;
    }

    if (0 == config) {
        fprintf(out, "(NULL POINTER)");
        return false;
    }

    int indent = (int)indentation_level;

    fprintf(out,
            "%*.sSample rate (Hz): %f    Frame length(usec): %" PRIu64 "\n",
            indent, " ", config->sample_rate_hertz, config->frame_length_usecs);

    return true;
}

/*----------------------------------------------------------------------------*/
