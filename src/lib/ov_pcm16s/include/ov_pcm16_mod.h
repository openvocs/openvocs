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
#ifndef OV_PCM16_MOD_H
#define OV_PCM16_MOD_H

/*----------------------------------------------------------------------------*/

#include <inttypes.h>
#include <ov_base/ov_vad_config.h>
#include <stdbool.h>
#include <stdlib.h>

/*----------------------------------------------------------------------------*/

/**
 * Scales samples with `in` with scale_factor. Result will be stored in `out`.
 * If number_of_samples == 0, nothing is done.
 */
bool ov_pcm_16_scale_to_32_bare(size_t number_of_samples, int16_t const *in,
                                int32_t *out, double scale_factor);

/**
 * Scales samples with `in` with scale_factor. Result will be stored in `out`.
 * If number_of_samples == 0, nothing is done.
 * The scale factor might fade from `scale_factor_start` to `scale_factor_end`.
 * If you want a constant scale factor, just set both  `scale_factor_start` and
 * `scale_factor_end` to the desired constant factor.
 */
bool ov_pcm_16_fade_to_32(size_t number_of_samples, int16_t const *in,
                          int32_t *out, double scale_factor_start,
                          double scale_factor_end);

/**
 * Calculates the VAD parameters and max amplitude from `in`.
 * If number_of_samples == 0, nothing is done.
 * If `params` is 0, its an error. Nothing is done and `false` returned.
 */
bool ov_pcm_16_get_audio_params(size_t number_of_samples, int16_t const *in,
                                ov_vad_parameters *params,
                                int16_t *max_amplitude);

/**
 * Scales samples with `in` with scale_factor. Result will be stored in `out`.
 * If number_of_samples == 0, nothing is done.
 * If `vad_params` is not 0, the VAD parameters calculated during scaling are
 * stored within.
 * If `max_amplitude` is not 0, the maximum of the amplitudes found in the PCM
 * is stored therein
 */
bool ov_pcm_16_scale_to_32(size_t number_of_samples, int16_t const *in,
                           int32_t *out, double scale_factor,
                           ov_vad_parameters *vad_params,
                           int16_t *max_amplitude);

/**
 * Scale 32bit wide pcm signal
 */
bool ov_pcm_32_scale(size_t number_of_samples, int32_t *pcm32,
                     double scale_factor);

/**
 * Scale the incoming signal:
 * Find maximum amplitude and scale the signal in such a way that the max
 * amplitude equals max_amplitude
 */
bool ov_pcm_32_normalize_to(size_t number_of_samples, int32_t *pcm32,
                            uint32_t max_amplitude);

/**
 * Transforms 32 bit samples to 16 bit samples by clipping at INT16_MAX/MIN
 * If number_of_samples == 0, nothing is done.
 */
bool ov_pcm_32_clip_to_16(size_t number_of_samples, int32_t const *in,
                          int16_t *out);

/**
 * Transforms 32 bit samples to 16 bit samples by audio compression.
 * If number_of_samples == 0, nothing is done.
 */
bool ov_pcm_32_compress_to_16(size_t number_of_samples, int32_t const *in,
                              int16_t *out);

/**
 * Sutracts  `in2` from `in1` and stores result in `in1`.
 * If number_of_samples == 0, nothing is done.
 */
bool ov_pcm_32_subtract(size_t number_of_samples, int32_t *in1,
                        int32_t const *in2);

/**
 * Adds `in1` and `in2` and stores result in `in1` .
 * If number_of_samples == 0, nothing is done.
 */
bool ov_pcm_32_add(size_t number_of_samples, int32_t *in1, int32_t const *in2);

/**
 * Calculates the VAD parameters from `in`.
 * If number_of_samples == 0, nothing is done.
 * If `params` is 0, its an error. Nothing is done and `false` returned.
 */
bool ov_pcm_16_get_vad_parameters(size_t number_of_samples, int16_t const *in,
                                  ov_vad_parameters *params);

/**
 * @see ov_pcm_16_get_vad_parameters
 * Same as ov_pcm_16_get_vad_parameters - but for 32bit samples.
 */
bool ov_pcm_32_get_vad_parameters(size_t number_of_samples, int32_t const *in,
                                  ov_vad_parameters *params);

/**
 * Returns `true` if `params` indicate voice activity.
 */
bool ov_pcm_vad_detected(uint64_t samplerate_hz, ov_vad_parameters params,
                         ov_vad_config limits);

/*----------------------------------------------------------------------------*/
#endif
