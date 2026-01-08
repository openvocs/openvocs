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

#include "ov_pcm16_mod.c"
#include "ov_pcm16_mod.h"
#include <math.h>
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

#define DEQUALS(x, y, d) (fabs(x - y) < d)

#define NUM_SAMPLES 10
int32_t const EMPTY[NUM_SAMPLES] = {0};

/*----------------------------------------------------------------------------*/

static int ov_pcm_16_scale_test() {

  int16_t const IN[NUM_SAMPLES] = {1, 2, 3, 4, 0, 4, 3, 2, 1, -1};
  int32_t const RF[NUM_SAMPLES] = {2, 4, 6, 8, 0, 8, 6, 4, 2, -2};

  double zero_crossings_per_sample = 3.0 / 10.0;
  double power_per_sample = 1 * 1 + 2 * 2 + 3 * 3 + 4 * 4;
  power_per_sample *= 2.0;
  power_per_sample += 1.0 * 1.0;
  power_per_sample /= (double)NUM_SAMPLES;

  int32_t out[NUM_SAMPLES] = {0};

  testrun(ov_pcm_16_scale_to_32(0, 0, 0, 0, 0, 0));
  testrun(0 == memcmp(EMPTY, out, sizeof(out)));

  testrun(!ov_pcm_16_scale_to_32(NUM_SAMPLES, 0, 0, 0, 0, 0));

  testrun(ov_pcm_16_scale_to_32(0, IN, 0, 0, 0, 0));
  testrun(0 == memcmp(EMPTY, out, sizeof(out)));

  testrun(!ov_pcm_16_scale_to_32(NUM_SAMPLES, IN, 0, 0, 0, 0));

  testrun(ov_pcm_16_scale_to_32(0, 0, out, 0, 0, 0));
  testrun(0 == memcmp(EMPTY, out, sizeof(out)));

  testrun(!ov_pcm_16_scale_to_32(NUM_SAMPLES, 0, out, 0, 0, 0));
  testrun(ov_pcm_16_scale_to_32(NUM_SAMPLES, IN, out, 0, 0, 0));

  testrun(0 == memcmp(EMPTY, out, NUM_SAMPLES * sizeof(int32_t)));

  testrun(ov_pcm_16_scale_to_32(0, 0, 0, 2, 0, 0));
  testrun(0 == memcmp(EMPTY, out, sizeof(out)));

  testrun(!ov_pcm_16_scale_to_32(NUM_SAMPLES, 0, 0, 2, 0, 0));
  testrun(!ov_pcm_16_scale_to_32(NUM_SAMPLES, IN, 0, 2, 0, 0));
  testrun(ov_pcm_16_scale_to_32(NUM_SAMPLES, IN, out, 2, 0, 0));

  testrun(0 == memcmp(RF, out, sizeof(out)));

  ov_vad_parameters vad = {0};
  memset(out, 0, sizeof(out));

  testrun(ov_pcm_16_scale_to_32(NUM_SAMPLES, IN, out, 2, &vad, 0));

  testrun(0 == memcmp(RF, out, sizeof(out)));
  testrun(
      DEQUALS(power_per_sample, vad.powerlevel_density_per_sample, 0.00001));

  testrun(DEQUALS(zero_crossings_per_sample, vad.zero_crossings_per_sample,
                  0.00001));

  memset(out, 0, sizeof(out));
  int16_t max = 0;

  testrun(ov_pcm_16_scale_to_32(NUM_SAMPLES, IN, out, 2, 0, &max));

  testrun(4 == max);

  // Check again for max value actually using abs
  int16_t const IN2[NUM_SAMPLES] = {1, 2, 3, -9, 0, 4, 3, 2, 1, -1};
  testrun(ov_pcm_16_scale_to_32(NUM_SAMPLES, IN2, out, 2, 0, &max));

  testrun(9 == max);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int ov_pcm_32_scale_test() {

  int32_t const IN[NUM_SAMPLES] = {1, 2, 3, 4, 0, 4, 3, 2, 1, -1};
  int32_t const RF[NUM_SAMPLES] = {2, 4, 6, 8, 0, 8, 6, 4, 2, -2};

  int32_t out[NUM_SAMPLES] = {0};

  testrun(ov_pcm_32_scale(0, 0, 0));
  testrun(0 == memcmp(EMPTY, out, sizeof(out)));

  testrun(!ov_pcm_32_scale(NUM_SAMPLES, 0, 0));

  memcpy(out, IN, sizeof(IN));

  testrun(ov_pcm_32_scale(0, out, 0));

  testrun(ov_pcm_32_scale(NUM_SAMPLES, out, 0));
  testrun(0 == memcmp(EMPTY, out, sizeof(out)));

  memcpy(out, IN, sizeof(IN));

  testrun(ov_pcm_32_scale(NUM_SAMPLES, out, 2));
  testrun(0 == memcmp(RF, out, sizeof(RF)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int ov_pcm_32_normalize_to_test() {

  int32_t samples[NUM_SAMPLES] = {1, 17, 19, 23, 29, 31, 33, 10, 8, 2};
  int32_t ref[NUM_SAMPLES] = {3, 51, 57, 69, 87, 93, 100, 30, 24, 6};

  testrun(ov_pcm_32_normalize_to(0, 0, 100));
  testrun(!ov_pcm_32_normalize_to(NUM_SAMPLES, 0, 100));
  testrun(ov_pcm_32_normalize_to(0, samples, 100));

  // Check nothing changed yet
  testrun(1 == samples[0]);

  testrun(ov_pcm_32_normalize_to(NUM_SAMPLES, samples, 100));

  for (size_t i = 0; i < NUM_SAMPLES; ++i) {

    testrun(ref[i] == samples[i]);
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int ov_pcm_32_clip_to_16_test() {

#define NUM 4

  _Static_assert(NUM <= NUM_SAMPLES, "NUM exceeds max NUM");

  int32_t const IN[NUM] = {-17, 37, ((int32_t)INT16_MIN) - 1,
                           ((int32_t)INT16_MAX) + 1};

  int16_t const REF[NUM] = {-17, 37, INT16_MIN, INT16_MAX};
  int16_t out[NUM] = {0};

  testrun(ov_pcm_32_clip_to_16(0, 0, 0));
  testrun(!ov_pcm_32_clip_to_16(NUM, 0, 0));
  testrun(ov_pcm_32_clip_to_16(0, IN, 0));
  testrun(!ov_pcm_32_clip_to_16(NUM, IN, 0));
  testrun(ov_pcm_32_clip_to_16(0, IN, out));

  testrun(0 == memcmp(out, EMPTY, sizeof(out)));

  testrun(ov_pcm_32_clip_to_16(NUM, IN, out));

  testrun(0 == memcmp(out, REF, sizeof(out)));

#undef NUM

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int ov_pcm_32_compress_to_16_test() {
  // bool ov_pcm_32_compress_to_16(size_t number_of_samples,
  //                              int32_t *in,
  //                              int16_t *out);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int ov_pcm_32_subtract_test() {

  int32_t const IN1[NUM_SAMPLES] = {1, 2, 3, 4, 0, 4, 3, 2, INT32_MIN, -1};
  int32_t const IN2[NUM_SAMPLES] = {2, 1, 0, -1, 0, 4, 3, 2, 3, -1};
  int32_t const RF[NUM_SAMPLES] = {-1, 1, 3, 5, 0, 0, 0, 0, INT32_MIN, 0};

  int32_t in1[NUM_SAMPLES] = {0};

  memcpy(in1, IN1, sizeof(in1));

  testrun(ov_pcm_32_subtract(0, 0, 0));
  testrun(!ov_pcm_32_subtract(NUM_SAMPLES, 0, 0));
  testrun(ov_pcm_32_subtract(0, in1, 0));
  testrun(!ov_pcm_32_subtract(NUM_SAMPLES, in1, 0));
  testrun(ov_pcm_32_subtract(0, 0, IN2));
  testrun(!ov_pcm_32_subtract(NUM_SAMPLES, 0, IN2));
  testrun(ov_pcm_32_subtract(NUM_SAMPLES, in1, IN2));

  testrun(0 == memcmp(in1, RF, sizeof(in1)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int ov_pcm_32_add_test() {

  int32_t const IN1[NUM_SAMPLES] = {1, 2, 3, 4, 0, 4, 3, 2, 1, -1};
  int32_t const IN2[NUM_SAMPLES] = {2, 1, 0, -1, 0, 4, 3, 2, 1, -1};
  int32_t const RF[NUM_SAMPLES] = {3, 3, 3, 3, 0, 8, 6, 4, 2, -2};

  int32_t in1[NUM_SAMPLES] = {0};

  memcpy(in1, IN1, sizeof(in1));

  testrun(ov_pcm_32_add(0, 0, 0));
  testrun(!ov_pcm_32_add(NUM_SAMPLES, 0, 0));
  testrun(ov_pcm_32_add(0, in1, 0));
  testrun(!ov_pcm_32_add(NUM_SAMPLES, in1, 0));
  testrun(ov_pcm_32_add(0, 0, IN2));
  testrun(!ov_pcm_32_add(NUM_SAMPLES, 0, IN2));
  testrun(ov_pcm_32_add(NUM_SAMPLES, in1, IN2));

  testrun(0 == memcmp(in1, RF, sizeof(in1)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int ov_pcm_32_get_vad_parameters_test() {

  int32_t const IN[NUM_SAMPLES] = {1, 2, 3, 4, 0, 4, 3, 2, 1, -1};

  double zero_crossings_per_sample = 3.0 / 10.0;
  double power_per_sample = 1 * 1 + 2 * 2 + 3 * 3 + 4 * 4;
  power_per_sample *= 2.0;
  power_per_sample += 1.0 * 1.0;
  power_per_sample /= (double)NUM_SAMPLES;

  testrun(ov_pcm_32_get_vad_parameters(0, 0, 0));
  testrun(!ov_pcm_32_get_vad_parameters(NUM_SAMPLES, 0, 0));
  testrun(ov_pcm_32_get_vad_parameters(0, IN, 0));
  testrun(!ov_pcm_32_get_vad_parameters(NUM_SAMPLES, IN, 0));

  ov_vad_parameters vad = {0};

  testrun(ov_pcm_32_get_vad_parameters(NUM_SAMPLES, IN, &vad));

  testrun(
      DEQUALS(power_per_sample, vad.powerlevel_density_per_sample, 0.00001));

  testrun(DEQUALS(zero_crossings_per_sample, vad.zero_crossings_per_sample,
                  0.00001));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int ov_pcm_vad_detected_test() {

  // bool ov_pcm_vad_detected(uint64_t samplerate_hz,
  //                         ov_vad_parameters params,
  //                         ov_vad_config limits);
  //

  ov_vad_config limits = {
      .powerlevel_density_threshold_db = -40.0,
      .zero_crossings_rate_threshold_hertz = 20.0,
  };

  ov_vad_parameters params = {
      .powerlevel_density_per_sample = 0.001,
      .zero_crossings_per_sample = .1,
  };

  /* Voice will be detected, if both
   * - powerlevel > 10^(-40/(2 * 10)) = 0.01
   * - zero_crossings * samplerate < 20 -> happens for samplerate < 200
   */

  // zero_crossings IS satisfied, but not powerlevel

  testrun(!ov_pcm_vad_detected(1, params, limits));
  testrun(!ov_pcm_vad_detected(10, params, limits));
  testrun(!ov_pcm_vad_detected(100, params, limits));

  // both are satisfied

  params.powerlevel_density_per_sample = 0.02;

  testrun(ov_pcm_vad_detected(1, params, limits));
  testrun(ov_pcm_vad_detected(10, params, limits));
  testrun(ov_pcm_vad_detected(100, params, limits));
  testrun(ov_pcm_vad_detected(200, params, limits));

  // Now zero_crossings is not satisfied anymore
  testrun(!ov_pcm_vad_detected(201, params, limits));
  testrun(!ov_pcm_vad_detected(12009, params, limits));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int ov_pcm_16_scale_to_32_bare_test() {

  bool ov_pcm_16_scale_to_32_bare(size_t number_of_samples, int16_t const *in,
                                  int32_t *out, double scale_factor);

  int16_t const IN[NUM_SAMPLES] = {1, 2, 3, 4, 0, 4, 3, 2, 1, -1};
  int32_t const RF[NUM_SAMPLES] = {2, 4, 6, 8, 0, 8, 6, 4, 2, -2};

  int32_t out[NUM_SAMPLES] = {0};

  testrun(ov_pcm_16_scale_to_32_bare(0, 0, 0, 0));
  testrun(0 == memcmp(EMPTY, out, sizeof(out)));

  testrun(!ov_pcm_16_scale_to_32_bare(NUM_SAMPLES, 0, 0, 0));

  testrun(ov_pcm_16_scale_to_32_bare(0, IN, 0, 0));
  testrun(0 == memcmp(EMPTY, out, sizeof(out)));

  testrun(!ov_pcm_16_scale_to_32_bare(NUM_SAMPLES, IN, 0, 0));

  testrun(ov_pcm_16_scale_to_32_bare(0, 0, out, 0));
  testrun(0 == memcmp(EMPTY, out, sizeof(out)));

  testrun(!ov_pcm_16_scale_to_32_bare(NUM_SAMPLES, 0, out, 0));
  testrun(ov_pcm_16_scale_to_32_bare(NUM_SAMPLES, IN, out, 0));

  testrun(0 == memcmp(EMPTY, out, NUM_SAMPLES * sizeof(int32_t)));

  testrun(ov_pcm_16_scale_to_32_bare(0, 0, 0, 2));
  testrun(0 == memcmp(EMPTY, out, sizeof(out)));

  testrun(!ov_pcm_16_scale_to_32_bare(NUM_SAMPLES, 0, 0, 2));
  testrun(!ov_pcm_16_scale_to_32_bare(NUM_SAMPLES, IN, 0, 2));
  testrun(ov_pcm_16_scale_to_32_bare(NUM_SAMPLES, IN, out, 2));

  testrun(0 == memcmp(RF, out, sizeof(out)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int ov_pcm_16_fade_to_32_test() {
  int16_t const IN[NUM_SAMPLES] = {1, 2, 3, 4, 0, 4, 3, 2, 1, -1};
  int32_t const RF[NUM_SAMPLES] = {2, 4, 6, 8, 0, 8, 6, 4, 2, -2};
  int32_t out[NUM_SAMPLES] = {0};
  testrun(ov_pcm_16_fade_to_32(0, 0, 0, 0, 0));
  testrun(0 == memcmp(EMPTY, out, sizeof(out)));
  testrun(!ov_pcm_16_fade_to_32(NUM_SAMPLES, 0, 0, 0, 0));
  testrun(ov_pcm_16_fade_to_32(0, IN, 0, 0, 0));
  testrun(0 == memcmp(EMPTY, out, sizeof(out)));
  testrun(!ov_pcm_16_fade_to_32(NUM_SAMPLES, IN, 0, 0, 0));
  testrun(ov_pcm_16_fade_to_32(0, 0, out, 0, 0));
  testrun(0 == memcmp(EMPTY, out, sizeof(out)));

  testrun(!ov_pcm_16_fade_to_32(NUM_SAMPLES, 0, out, 0, 0));
  testrun(ov_pcm_16_fade_to_32(NUM_SAMPLES, IN, out, 0, 0));
  testrun(0 == memcmp(EMPTY, out, NUM_SAMPLES * sizeof(int32_t)));
  testrun(ov_pcm_16_fade_to_32(0, 0, 0, 2, 2));
  testrun(0 == memcmp(EMPTY, out, sizeof(out)));
  testrun(!ov_pcm_16_fade_to_32(NUM_SAMPLES, 0, 0, 2, 2));
  testrun(!ov_pcm_16_fade_to_32(NUM_SAMPLES, IN, 0, 2, 2));
  testrun(ov_pcm_16_fade_to_32(NUM_SAMPLES, IN, out, 2, 2));
  testrun(0 == memcmp(RF, out, sizeof(out)));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int ov_pcm_16_get_audio_params_test() {

  bool ov_pcm_16_get_audio_params(size_t number_of_samples, int16_t const *in,
                                  ov_vad_parameters *params,
                                  int16_t *max_amplitude);

  int16_t const IN[NUM_SAMPLES] = {1, 2, 3, 4, 0, 4, 3, 2, 1, -1};

  double zero_crossings_per_sample = 3.0 / 10.0;
  double power_per_sample = 1 * 1 + 2 * 2 + 3 * 3 + 4 * 4;
  power_per_sample *= 2.0;
  power_per_sample += 1.0 * 1.0;
  power_per_sample /= (double)NUM_SAMPLES;

  testrun(ov_pcm_16_get_audio_params(0, 0, 0, 0));
  testrun(!ov_pcm_16_get_audio_params(NUM_SAMPLES, 0, 0, 0));
  testrun(ov_pcm_16_get_audio_params(0, IN, 0, 0));
  testrun(!ov_pcm_16_get_audio_params(NUM_SAMPLES, IN, 0, 0));

  int16_t max = 0;

  testrun(ov_pcm_16_get_audio_params(0, 0, 0, &max));
  testrun(!ov_pcm_16_get_audio_params(NUM_SAMPLES, 0, 0, &max));
  testrun(ov_pcm_16_get_audio_params(0, IN, 0, &max));
  testrun(!ov_pcm_16_get_audio_params(NUM_SAMPLES, IN, 0, &max));

  ov_vad_parameters vad = {0};

  testrun(ov_pcm_16_get_audio_params(NUM_SAMPLES, IN, &vad, 0));

  testrun(
      DEQUALS(power_per_sample, vad.powerlevel_density_per_sample, 0.00001));

  testrun(DEQUALS(zero_crossings_per_sample, vad.zero_crossings_per_sample,
                  0.00001));

  max = 0;
  memset(&vad, 0, sizeof(vad));

  testrun(ov_pcm_16_get_audio_params(NUM_SAMPLES, IN, &vad, &max));

  testrun(4 == max);
  testrun(
      DEQUALS(power_per_sample, vad.powerlevel_density_per_sample, 0.00001));

  testrun(DEQUALS(zero_crossings_per_sample, vad.zero_crossings_per_sample,
                  0.00001));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_pcm16_mod", ov_pcm_16_scale_to_32_bare_test,
            ov_pcm_16_fade_to_32_test, ov_pcm_16_get_audio_params_test,
            ov_pcm_16_scale_test, ov_pcm_32_scale_test,
            ov_pcm_32_normalize_to_test, ov_pcm_32_clip_to_16_test,
            ov_pcm_32_compress_to_16_test, ov_pcm_32_subtract_test,
            ov_pcm_32_add_test, ov_pcm_32_get_vad_parameters_test,
            ov_pcm_vad_detected_test);
