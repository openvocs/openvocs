/***

Copyright   2017        German Aerospace Center DLR e.V.,
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

     \file               ov_codec.c
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2017-12-06

     \ingroup            empty

     \brief              empty

 **/

#include <ov_base/ov_utils.h>

#include "../include/ov_codec.h"
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_constants.h>
#include <ov_base/ov_json.h>

#include <ov_arch/ov_arch_math.h>

#include <ov_pcm16s/ov_pcm_resampler.h>

/*****************************************************************************
                                   Resampling
 ****************************************************************************/

struct resampling {
    ov_pcm_16_resampler *resampler_encode;
    ov_pcm_16_resampler *resampler_decode;
    int16_t *data;
    size_t data_capacity_samples;
};

/*----------------------------------------------------------------------------*/

/**
 * The internal samplerate is fixed to 48k, therefore we only require
 * @param max_num_samples_out maximum number of supported samples at 48k
 * @param sample_rate to decode from / encode to
 */
static struct resampling *resampling_create(size_t max_num_out_samples,
                                            uint64_t sample_rate_out_hz) {

    if (OV_DEFAULT_SAMPLERATE == sample_rate_out_hz) {
        // No resampling required
        return 0;
    }

    double sample_period_s = max_num_out_samples;
    sample_period_s /= sample_rate_out_hz;

    double sample_rate_in_hz_d = OV_DEFAULT_SAMPLERATE;

    double max_num_in_samples_d = (sample_period_s * sample_rate_in_hz_d);
    size_t max_num_in_samples = max_num_in_samples_d;

    struct resampling *res = calloc(1, sizeof(struct resampling));
    OV_ASSERT(0 != res);

    size_t max_num_samples = OV_MAX(max_num_in_samples, max_num_out_samples);
    res->data = calloc(1, sizeof(int16_t) * max_num_samples);
    res->data_capacity_samples = max_num_samples;

    ov_log_info("Enabling resampling from %zu (max %zu samples) to %zu (max "
                "%zu samples)",
                OV_DEFAULT_SAMPLERATE, max_num_in_samples, sample_rate_out_hz,
                max_num_out_samples);

    res->resampler_decode =
        ov_pcm_16_resampler_create(max_num_out_samples, max_num_in_samples,
                                   sample_rate_out_hz, OV_DEFAULT_SAMPLERATE);

    res->resampler_encode =
        ov_pcm_16_resampler_create(max_num_in_samples, max_num_out_samples,
                                   OV_DEFAULT_SAMPLERATE, sample_rate_out_hz);

    return res;
}

/*----------------------------------------------------------------------------*/

static struct resampling *resampling_free(struct resampling *resampling) {

    if (0 == resampling) {
        goto error;
    }

    resampling->resampler_decode =
        ov_pcm_16_resampler_free(resampling->resampler_decode);

    resampling->resampler_encode =
        ov_pcm_16_resampler_free(resampling->resampler_encode);

    if (0 != resampling->data) {
        free(resampling->data);
        resampling->data = 0;
    }

    free(resampling);

    resampling = 0;

error:

    return resampling;
}

/*----------------------------------------------------------------------------*/

typedef enum {

    RESAMPLE_DECODE,
    RESAMPLE_ENCODE

} resample_dir;

static ov_pcm_16_resampler *
get_resampler_for(struct resampling const *resampling, resample_dir direction) {

    if (0 == resampling) {
        return 0;
    }

    switch (direction) {

    case RESAMPLE_DECODE:

        return resampling->resampler_decode;

    case RESAMPLE_ENCODE:

        return resampling->resampler_encode;
    };

    return 0;
}

/*----------------------------------------------------------------------------*/

static ssize_t resample_if_necessary_nocheck(
    struct resampling *resampling, resample_dir direction, uint8_t const *input,
    size_t length_bytes, uint8_t **resampled, size_t *resampled_length_bytes) {

    ssize_t out_samples = 0;

    OV_ASSERT(0 != resampled);
    OV_ASSERT(0 != resampled_length_bytes);

    ov_pcm_16_resampler *resampler = get_resampler_for(resampling, direction);

    if (0 == resampler) {
        // ov_log_info("Resampling not required");
        goto finish;
    }

    int16_t *data = resampling->data;
    size_t capacity_samples = resampling->data_capacity_samples;

    ov_log_info("Resampling ...");

    OV_ASSERT(0 != resampler);
    OV_ASSERT(0 != data);

    size_t num_input_samples = length_bytes / sizeof(int16_t);

    out_samples = ov_pcm_16_resample(resampler, (int16_t *)input,
                                     num_input_samples, data, capacity_samples);

    if (0 > out_samples) {
        ov_log_error("Resampling failed");
        goto error;
    }

    if (0 == out_samples) {
        ov_log_info("No resampling required");
        *resampled = (uint8_t *)input;
        *resampled_length_bytes = length_bytes;
        goto finish;
    }

    if (capacity_samples < (size_t)out_samples) {

        ov_log_error("Resampling: Output buffer too small - "
                     "is: %zu samples, should be %zu samples",
                     capacity_samples, out_samples);
        goto error;
    }

    *resampled = (uint8_t *)data;
    *resampled_length_bytes = sizeof(int16_t) * out_samples;

finish:

    return out_samples;

error:

    return -1;
}

/*****************************************************************************
                                     CODEC
 ****************************************************************************/

char const CODEC_NONE_STRING[] = "None";

/*----------------------------------------------------------------------------*/

ov_codec *ov_codec_free(ov_codec *codec) {

    if (0 == codec)
        return 0;

    OV_ASSERT(0 != codec->free);

    codec->resampling = resampling_free(codec->resampling);

    return codec->free(codec);
}

/*----------------------------------------------------------------------------*/

char const *ov_codec_type_id(ov_codec const *codec) {

    if (0 == codec) {
        ov_log_error("No codec given");
        return CODEC_NONE_STRING;
    }

    OV_ASSERT(0 != codec->type_id);

    return codec->type_id(codec);
}

/*----------------------------------------------------------------------------*/

int32_t ov_codec_encode(ov_codec *codec, const uint8_t *input, size_t length,
                        uint8_t *output, size_t max_out_length) {

    if (0 == codec) {

        ov_log_error("No codec given");
        return -1;
    }

    OV_ASSERT(0 != codec->encode);

    uint8_t *resampled_input = 0;
    size_t resampled_input_len = 0;

    ssize_t out_samples = resample_if_necessary_nocheck(
        codec->resampling, RESAMPLE_ENCODE, input, length, &resampled_input,
        &resampled_input_len);

    if (0 > out_samples) {
        return -1;
    }

    if (0 < out_samples) {
        input = resampled_input;
        length = resampled_input_len;
    }

    // 0 == out_samples : Nothing done - we can use input/length

    return codec->encode(codec, input, length, output, max_out_length);
}

/*----------------------------------------------------------------------------*/

int32_t ov_codec_decode(ov_codec *codec, uint64_t seq_number,
                        const uint8_t *input, size_t length, uint8_t *output,
                        size_t max_out_length) {

    int32_t result = -1;

    if (0 == codec) {

        ov_log_error("No codec given");
        return -1;
    }

    OV_ASSERT(0 != codec->decode);

    result =
        codec->decode(codec, seq_number, input, length, output, max_out_length);

    if (0 > result) {
        ov_log_error("Decoding failed");
        goto error;
    }

    if (0 == codec->resampling) {
        // ov_log_info("No resampling required");
        goto finish;
    }

    OV_ASSERT(0 != output);
    length = result;

    uint8_t *resampled_input = 0;
    size_t resampled_input_len = 0;

    ssize_t out_samples = resample_if_necessary_nocheck(
        codec->resampling, RESAMPLE_DECODE, output, length, &resampled_input,
        &resampled_input_len);

    if (0 == out_samples) {
        goto finish;
    }

    if (0 > out_samples) {
        goto error;
    }

    OV_ASSERT(0 < out_samples);

    size_t bytes_to_copy = OV_MIN(resampled_input_len, max_out_length);

    memcpy(output, resampled_input, bytes_to_copy);

    return bytes_to_copy;

finish:

    return result;

error:

    return result;
}

/*----------------------------------------------------------------------------*/

int8_t ov_codec_get_rtp_payload_type(ov_codec const *codec) {
    if (0 == codec) {
        ov_log_error(
            "Cannot get RTP payload type for undefined codec (null pointer)");
        return 0;
    } else if (0 == codec->rtp_payload_type) {
        return -1;
    } else {
        return codec->rtp_payload_type(codec);
    }
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_codec_get_parameters(const ov_codec *codec) {
    if (0 == codec) {
        ov_log_error("No codec given");
        return 0;
    }

    OV_ASSERT(0 != codec->get_parameters);

    return codec->get_parameters(codec);
}

/*----------------------------------------------------------------------------*/

uint32_t ov_codec_get_samplerate_hertz(ov_codec const *codec) {
    if (0 == codec) {
        ov_log_error("No codec given");
        return 0;
    }

    if (0 == codec->get_samplerate_hertz) {
        ov_log_info("Codec does not provide samplerate - assuming default one");
        return OV_DEFAULT_SAMPLERATE;
    }

    return codec->get_samplerate_hertz(codec);
}

/******************************************************************************
 *                         STANDARD CODEC PARAMETERS
 ******************************************************************************/

ov_json_value *ov_codec_to_json(const ov_codec *codec) {
    ov_json_value *json = 0;

    if (0 == codec)
        goto error;

    json = codec->get_parameters(codec);

    if (0 == json)
        goto error;

    const char *type = codec->type_id(codec);
    if (0 == type)
        goto error;

    ov_json_object_set(json, OV_KEY_CODEC, ov_json_string(type));

    return json;

error:

    if (0 != json)
        json = json->free(json);

    return 0;
}

/*---------------------------------------------------------------------------*/

uint32_t ov_codec_parameters_get_sample_rate_hertz(const ov_json_value *json) {
    if (0 == json)
        goto error;

    double sample_rate_hertz =
        ov_json_number_get(ov_json_get(json, "/" OV_KEY_SAMPLE_RATE_HERTZ));

    if (1 > sample_rate_hertz)
        goto error;

    double max = (double)INT64_MAX;
    if (max < sample_rate_hertz)
        goto error;

    return (uint32_t)sample_rate_hertz;

error:

    return OV_DEFAULT_SAMPLERATE;
}

/*---------------------------------------------------------------------------*/

bool ov_codec_parameters_set_sample_rate_hertz(ov_json_value *json,
                                               uint32_t sample_rate_hertz) {
    ov_json_value *old = 0;

    if (0 == json)
        goto error;

    /* If entry already contained, get rid of it ... */

    old =
        ov_json_value_copy(0, ov_json_get(json, "/" OV_KEY_SAMPLE_RATE_HERTZ));

    if (!ov_json_object_del(json, OV_KEY_SAMPLE_RATE_HERTZ)) {
        goto error;
    }

    if (!ov_json_object_set(json, OV_KEY_SAMPLE_RATE_HERTZ,
                            ov_json_number(sample_rate_hertz))) {
        goto error;
    }

    if (0 != old) {
        old = old->free(old);
    }

    OV_ASSERT(0 == old);

    return true;

error:

    /* Remember: In case of error,
     * json SHOULD NOT HAVE BEEN ALTERED AFTER CALL */

    if (old) {
        ov_json_object_set(json, OV_KEY_SAMPLE_RATE_HERTZ, old);
        old = 0;
    }

    OV_ASSERT(0 == old);

    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_codec_enable_resampling(ov_codec *codec) {
    if (0 == codec) {
        goto error;
    }

    uint32_t sample_rate = ov_codec_get_samplerate_hertz(codec);

    size_t max_num_samples = OV_MAX_FRAME_LENGTH_SAMPLES;

    if (OV_DEFAULT_SAMPLERATE != sample_rate) {
        codec->resampling = resampling_create(max_num_samples, sample_rate);
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/
