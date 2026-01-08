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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_PCM_RESAMPLER_H
#define OV_PCM_RESAMPLER_H

#include <stddef.h>
#include <sys/types.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_pcm_16_resampler ov_pcm_16_resampler;

ov_pcm_16_resampler *ov_pcm_16_resampler_create(
    size_t max_number_of_in_samples, size_t max_number_of_out_samples,
    double sample_rate_in_hertz, double sample_rate_out_hertz);

ov_pcm_16_resampler *ov_pcm_16_resampler_free(ov_pcm_16_resampler *self);

/**
 * Resamples PCM.
 *
 * @param in buffer containing PCM to be resampled.
 * @param number_of_samples Number of 16bit samples in in
 * @param out buffer to write resampled PCM to. Must be sufficiently large.
 * @param out_samples_capacity size of out buffer in 32bit words.
 *
 * @return Number of 32 bit words actually required for output buffer. If larger
 * than out_size_words, out buffer too small and nothing was actually done.
 * Negative in case of error. If 0 is returned, no resampling was required and
 * hence nothing done. `out` was not altered.
 *
 */
ssize_t ov_pcm_16_resample(ov_pcm_16_resampler *self, int16_t const *in,
                           size_t number_of_samples, int16_t *out,
                           size_t out_samples_capacity);

/*----------------------------------------------------------------------------*/

/**
 * Resamples PCM without precalculating the convolution coefficients in advance
 * @see ov_pcm_16_resample
 */
ssize_t ov_pcm_16_resample_uncached(ov_pcm_16_resampler const *self,
                                    int16_t const *in,
                                    const size_t num_in_samples, int16_t *out,
                                    const size_t out_samples_capacity);

/*----------------------------------------------------------------------------*/
#endif
