/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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

#include "../include/ov_pcm16_mod.h"
#include <math.h>
#include <ov_base/ov_utils.h>

/*----------------------------------------------------------------------------*/

#define ABS(x) (((x) < 0) ? -(x) : (x))

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/*----------------------------------------------------------------------------*/

static int16_t clip_to_16_bit(int32_t val) {

    if (INT16_MAX < val) {
        val = INT16_MAX;
    }

    if (INT16_MIN > val) {
        val = INT16_MIN;
    }

    return (int16_t)val;
}

/*----------------------------------------------------------------------------*/

static int32_t clip_to_32_bit(int64_t val) {

    if (INT32_MAX < val) {
        val = INT32_MAX;
    }

    if (INT32_MIN > val) {
        val = INT32_MIN;
    }

    return (int32_t)val;
}

/*----------------------------------------------------------------------------*/

bool ov_pcm_16_scale_to_32_bare(size_t number_of_samples, int16_t const *in,
                                int32_t *out, double scale_factor) {

    if (0 == number_of_samples)
        return true;

    if ((0 == in) || (0 == out)) {
        goto error;
    }

    for (size_t i = 0; i < number_of_samples; ++i) {

        int32_t val = in[i];

        double dval = val;
        dval *= scale_factor;

        out[i] = (int32_t)dval;
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_pcm_16_fade_to_32(size_t number_of_samples, int16_t const *in,
                          int32_t *out, double scale_factor_start,
                          double scale_factor_end) {

    if (0 == number_of_samples) {

        return true;

    } else if ((0 == in) || (0 == out)) {

        return false;

    } else {

        double scale_factor_step = scale_factor_end - scale_factor_start;
        scale_factor_step /= number_of_samples;
        double scale_factor = scale_factor_start;
        for (size_t i = 0; i < number_of_samples; ++i) {
            int32_t val = in[i];
            double dval = val;
            dval *= scale_factor;
            scale_factor += scale_factor_step;
            out[i] = (int32_t)dval;
        }
        return true;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_pcm_16_get_audio_params(size_t number_of_samples, int16_t const *in,
                                ov_vad_parameters *params,
                                int16_t *max_amplitude) {

    if (0 == number_of_samples)
        return true;

    if ((0 == in) || (0 == params)) {
        goto error;
    }

    int16_t oldval = 0;

    double zero_crossings = 0;
    double power = 0;

    int16_t max = 0;

    oldval = in[0];

    for (size_t i = 0; i < number_of_samples; ++i) {

        int16_t val = in[i];

        if (0 >= val * oldval) {
            ++zero_crossings;
        }

        power += val * val;

        int16_t absval = abs(val);

        if (absval > max) {
            max = absval;
        }

        oldval = val;
    }

    params->powerlevel_density_per_sample = power / number_of_samples;
    params->zero_crossings_per_sample = zero_crossings / number_of_samples;

    if (0 != max_amplitude) {
        *max_amplitude = max;
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_pcm_16_scale_to_32(size_t number_of_samples, int16_t const *in,
                           int32_t *out, double scale_factor,
                           ov_vad_parameters *vad_params,
                           int16_t *max_amplitude) {

    if (0 == number_of_samples)
        return true;

    if ((0 == in) || (0 == out)) {
        goto error;
    }

    double zero_crossings = 0;
    double power = 0;

    int16_t oldval = in[0];
    int16_t max = 0;

    for (size_t i = 0; i < number_of_samples; ++i) {

        int32_t val = in[i];

        int16_t absval = abs(in[i]);

        if (absval > max) {
            max = absval;
        }

        if (oldval * val <= 0) {
            ++zero_crossings;
        }

        power += val * val;

        double dval = val;
        dval *= scale_factor;

        out[i] = (int32_t)dval;

        oldval = val;
    }

    if (0 != vad_params) {

        vad_params->powerlevel_density_per_sample = power / number_of_samples;

        vad_params->zero_crossings_per_sample =
            zero_crossings / number_of_samples;
    }

    if (0 != max_amplitude) {
        *max_amplitude = max;
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_pcm_32_scale(size_t number_of_samples, int32_t *pcm32,
                     double scale_factor) {

    if (0 == number_of_samples)
        return true;

    if (0 == pcm32) {
        goto error;
    }

    for (size_t i = 0; i < number_of_samples; ++i) {

        double dval = pcm32[i];
        dval *= scale_factor;

        pcm32[i] = (int32_t)dval;
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_pcm_32_normalize_to(size_t number_of_samples, int32_t *pcm32,
                            uint32_t max_amplitude) {

    if (0 == number_of_samples)
        return true;

    if (0 == pcm32) {
        goto error;
    }

    uint32_t max_val = 0;

    for (size_t i = 0; i < number_of_samples; ++i) {

        if (max_val < (uint32_t)ABS(pcm32[i])) {
            max_val = pcm32[i];
        }
    }

    double scale = max_amplitude;
    double max_val_d = max_val;

    scale /= max_val_d;

    return ov_pcm_32_scale(number_of_samples, pcm32, scale);

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_pcm_32_clip_to_16(size_t number_of_samples, int32_t const *in,
                          int16_t *out) {

    if (0 == number_of_samples) {
        return true;
    }

    if ((0 == in) || (0 == out)) {
        goto error;
    }

    for (size_t i = 0; i < number_of_samples; ++i) {

        out[i] = clip_to_16_bit(in[i]);
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_pcm_32_compress_to_16(size_t number_of_samples, int32_t const *in,
                              int16_t *out) {

    UNUSED(number_of_samples);
    UNUSED(in);
    UNUSED(out);

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_pcm_32_subtract(size_t number_of_samples, int32_t *in1,
                        int32_t const *in2) {

    if (0 == number_of_samples)
        return true;

    if ((0 == in1) || (0 == in2)) {
        goto error;
    }

    for (size_t i = 0; i < number_of_samples; ++i) {

        int64_t result = in1[i];
        result -= in2[i];

        in1[i] = clip_to_32_bit(result);
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_pcm_32_add(size_t number_of_samples, int32_t *in1, int32_t const *in2) {

    if (0 == number_of_samples)
        return true;

    if ((0 == in1) || (0 == in2)) {
        goto error;
    }

    for (size_t s = 0; s < number_of_samples; ++s) {

        int64_t val = in1[s];
        val += in2[s];
        in1[s] = clip_to_32_bit(val);
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_pcm_16_get_vad_parameters(size_t number_of_samples, int16_t const *in,
                                  ov_vad_parameters *params) {

    if (0 == number_of_samples)
        return true;

    if ((0 == in) || (0 == params)) {
        goto error;
    }

    int16_t oldval = 0;

    double zero_crossings = 0;
    double power = 0;

    oldval = in[0];

    for (size_t i = 0; i < number_of_samples; ++i) {

        int32_t val = in[i];

        if (0 >= val * oldval) {
            ++zero_crossings;
        }

        power += val * val;

        oldval = val;
    }

    params->powerlevel_density_per_sample = power / number_of_samples;
    params->zero_crossings_per_sample = zero_crossings / number_of_samples;

    return true;

error:

    return false;
}

bool ov_pcm_32_get_vad_parameters(size_t number_of_samples, int32_t const *in,
                                  ov_vad_parameters *params) {

    if (0 == number_of_samples)
        return true;

    if ((0 == in) || (0 == params)) {
        goto error;
    }

    int32_t oldval = 0;

    double zero_crossings = 0;
    double power = 0;

    oldval = in[0];

    for (size_t i = 0; i < number_of_samples; ++i) {

        int32_t val = in[i];

        if (0 >= val * oldval) {
            ++zero_crossings;
        }

        power += val * val;

        oldval = val;
    }

    params->powerlevel_density_per_sample = power / number_of_samples;
    params->zero_crossings_per_sample = zero_crossings / number_of_samples;

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_pcm_vad_detected(uint64_t samplerate_hz, ov_vad_parameters params,
                         ov_vad_config limits) {

    double zero_crossings_hz = params.zero_crossings_per_sample;
    zero_crossings_hz *= samplerate_hz;

    double powerlevel_density = params.powerlevel_density_per_sample;

    /*
    ov_log_debug(
        "VAD: Detected Zero-X: %f Hz (limit %f Hz), PowerLevel Density %f dB "
        "(limit %f dB)",
        zero_crossings_hz,
        limits.zero_crossings_rate_threshold_hertz,
        log(powerlevel_density) * 20,
        limits.powerlevel_density_threshold_db);
    */

    if (zero_crossings_hz > limits.zero_crossings_rate_threshold_hertz) {
        /*
                ov_log_info(
                    "VAD: No voice - zero crossings too high: %f (%f per "
                    "sample) - threshold "
                    "%f",
                    zero_crossings_hz,
                    params.zero_crossings_per_sample,
                    limits.zero_crossings_rate_threshold_hertz);
        */
        return false;
    }

    double powerlevel_density_limit = limits.powerlevel_density_threshold_db;
    powerlevel_density_limit /= 20.0;
    powerlevel_density_limit = pow(10.0, powerlevel_density_limit);

    if (powerlevel_density <= powerlevel_density_limit) {
        /*
                ov_log_info("VAD: No voice - powerlevel too low: %f - threshold:
           %f", powerlevel_density, powerlevel_density_limit);
        */
        return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/
