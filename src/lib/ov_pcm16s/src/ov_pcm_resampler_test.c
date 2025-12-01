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

#include "ov_pcm_resampler.c"
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

static int ov_pcm_16_resampler_create_test() { return testrun_log_success(); }

/*----------------------------------------------------------------------------*/

static int ov_pcm_16_resampler_free_test() { return testrun_log_success(); }

/*****************************************************************************
                                   RESAMPLING
 ****************************************************************************/

static void print_samples(FILE *f, int16_t const *samples, size_t num_samples,
                          double samplerate_hz) {

  TEST_ASSERT(0 != f);
  TEST_ASSERT(0 != samples);
  TEST_ASSERT(0 < num_samples);
  TEST_ASSERT(0 < samplerate_hz);

  fprintf(f, "#\n# Samples: %zu\n#\n", num_samples);

  double t = 0;

  for (size_t i = 0; i < num_samples; ++i) {

    // fprintf(stderr, "Sample %zu; Before %" PRIi16 "\n", i, samples[i]);

    t = i;
    t /= samplerate_hz;

    fprintf(f, "%f    %" PRIi16 "\n", t, samples[i]);
  }

  fprintf(f, "#\n#\n");
}

/*----------------------------------------------------------------------------*/

static void print_samples_to_file(char const *path, int16_t const *sig_in,
                                  size_t num_samples, double samplerate_hz) {

  TEST_ASSERT(0 != path);

  FILE *f = fopen(path, "w");
  TEST_ASSERT(0 != f);

  print_samples(f, sig_in, num_samples, samplerate_hz);
  fprintf(f, "##########################\n#\n#\n#\n#\n");

  fclose(f);
  f = 0;
}

/*----------------------------------------------------------------------------*/
// Signal generator

double g_sig_default_periods[] = {100, 82, 19, 6};

double *g_sig_periods_s = g_sig_default_periods;
size_t g_sig_num_periods = 4;

int16_t g_sig_max_amplitude = 1000;

static int16_t sig(size_t i) {

  double ma = g_sig_max_amplitude;
  ma /= g_sig_num_periods;

  double result = 0;

  double t = i;
  double t_2_pi = 2.0 * 3.1415 * t;

  for (size_t i_p = 0; i_p < g_sig_num_periods; ++i_p) {

    result += ma * sin(t_2_pi / g_sig_periods_s[i_p]);
  }

  return (int16_t)result;
}

/*----------------------------------------------------------------------------*/

static void fill_with_signal(int16_t *in, size_t num_samples) {

  for (size_t i = 0; i < num_samples; ++i) {
    in[i] = sig(i);
  }
}

/*----------------------------------------------------------------------------*/

static int ov_pcm_16_resample_test() {

  int16_t IN[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

  size_t out_length_samples = 5 * sizeof(IN) / sizeof(int16_t);

  int16_t *out = calloc(1, out_length_samples);

  ov_pcm_16_resampler *resampler = ov_pcm_16_resampler_create(120, 120, 16, 12);

  testrun(0 != resampler);

  testrun(0 > ov_pcm_16_resample(resampler, 0, 0, 0, 0));

  testrun(0 == ov_pcm_16_resample(resampler, IN, 0, 0, 0));

  testrun(0 == ov_pcm_16_resample(0, IN, 0, out, 6));

  testrun(3 == ov_pcm_16_resample(resampler, IN, 4, 0, 0));
  testrun(3 == ov_pcm_16_resample(resampler, IN, 4, out, 0));
  testrun(3 == ov_pcm_16_resample(resampler, IN, 4, 0, 5));

  testrun(3 == ov_pcm_16_resample(resampler, IN, 4, out, out_length_samples));

  free(out);
  out = 0;

  resampler = ov_pcm_16_resampler_free(resampler);
  testrun(0 == resampler);

  //

  double samplerate_in_hz = 1237;
  double samplerate_out_hz = 1239;

  const size_t num_samples = 200;

  int16_t sig_in[num_samples];

  double our_periods[] = {100, 82, 19, 6};
  g_sig_periods_s = our_periods;
  g_sig_num_periods = 4;

  fill_with_signal(sig_in, num_samples);

  print_samples_to_file("/tmp/in.csv", sig_in, num_samples, samplerate_in_hz);

  resampler = ov_pcm_16_resampler_create(num_samples, num_samples,
                                         samplerate_in_hz, samplerate_out_hz);
  testrun(0 != resampler);

  out = calloc(1, sizeof(int16_t) * num_samples);
  out_length_samples = num_samples;

  ssize_t out_samples = ov_pcm_16_resample_uncached(
      resampler, sig_in, num_samples, out, out_length_samples);

  testrun(0 < out_samples);

  size_t num_out_samples = (size_t)out_samples;

  if (num_out_samples > out_length_samples) {
    fprintf(stdout,
            "Warning: Number of required output samples %zu higher than "
            "actual output buffer: %zu\n",
            num_out_samples, out_length_samples);
  }

  print_samples_to_file("/tmp/out.csv", out, num_out_samples,
                        samplerate_out_hz);

  printf("Done resampling\n");

  /*------------------------------------------------------------------------*/

  free(out);
  out = 0;

  resampler = ov_pcm_16_resampler_free(resampler);
  testrun(0 == resampler);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_pcm_resampler", ov_pcm_16_resampler_create_test,
            ov_pcm_16_resampler_free_test, ov_pcm_16_resample_test);
