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

#include "../include/ov_pcm_resampler.h"

#include <math.h>
#include <ov_arch/ov_arch_math.h>
#include <ov_base/ov_utils.h>

/*----------------------------------------------------------------------------*/

static double sinc(double x) {

  if (0 == x) {
    return 1.0;
  }

  return sin(x) / x;
}

/*----------------------------------------------------------------------------*/

static double h(double t, double pi_times_samplerate_in_hz) {

  if (0 == t) {
    return 1.0;
  }

  double val = t * pi_times_samplerate_in_hz;

  return sinc(val);
}

/*----------------------------------------------------------------------------*/

struct ov_pcm_16_resampler {

  double samplerate_in_hz;
  double samplerate_out_hz;

  double in_sample_length_s;
  double out_sample_length_s;

  size_t max_in_samples;
  size_t max_out_samples;

  // Structure of array:
  // i = in_samples_index + out_samples_index * max_number_of_out_samples
  double *h;
  size_t h_size;
};

/*----------------------------------------------------------------------------*/

static size_t to_h_index(size_t out_index, size_t in_index, size_t max_in) {

  return out_index * max_in + in_index;
}

/*----------------------------------------------------------------------------*/

static double h_at(ov_pcm_16_resampler const *self, size_t out_index,
                   size_t in_index) {

  OV_ASSERT(0 != self);

  size_t i = to_h_index(out_index, in_index, self->max_in_samples);

  OV_ASSERT(i < self->h_size);

  return self->h[i];
}

/*----------------------------------------------------------------------------*/

ov_pcm_16_resampler *
ov_pcm_16_resampler_create(size_t max_number_of_in_samples,
                           size_t max_number_of_out_samples,
                           double samplerate_in_hz, double samplerate_out_hz) {

  double in_sample_length_s = samplerate_in_hz;
  in_sample_length_s = 1.0 / in_sample_length_s;

  double out_sample_length_s = samplerate_out_hz;
  out_sample_length_s = 1.0 / samplerate_out_hz;

  double pi_times_samplerate_in_hz = samplerate_in_hz * M_PI;

  ov_pcm_16_resampler *resampler = calloc(1, sizeof(ov_pcm_16_resampler));
  OV_ASSERT(0 != resampler);

  size_t h_size = max_number_of_in_samples * max_number_of_out_samples;

  resampler->h = calloc(1, sizeof(double) * h_size);
  resampler->h_size = h_size;

  if (0 == resampler->h) {
    goto error;
  }

  OV_ASSERT(0 != resampler->h);

  resampler->max_in_samples = max_number_of_in_samples;
  resampler->max_out_samples = max_number_of_out_samples;

  for (size_t i = 0; i < max_number_of_in_samples; ++i) {
    for (size_t o = 0; o < max_number_of_out_samples; o++) {

      resampler->h[to_h_index(o, i, max_number_of_in_samples)] =
          h(o * out_sample_length_s - i * in_sample_length_s,
            pi_times_samplerate_in_hz);
    }
  }

  resampler->samplerate_in_hz = samplerate_in_hz;
  resampler->in_sample_length_s = in_sample_length_s;
  resampler->samplerate_out_hz = samplerate_out_hz;
  resampler->out_sample_length_s = out_sample_length_s;

  return resampler;

error:

  resampler = ov_pcm_16_resampler_free(resampler);
  OV_ASSERT(0 == resampler);

  return resampler;
}

/*----------------------------------------------------------------------------*/

ov_pcm_16_resampler *ov_pcm_16_resampler_free(ov_pcm_16_resampler *self) {

  if (0 == self) {
    goto error;
  }

  if (0 != self->h) {
    free(self->h);
  }

  free(self);

  self = 0;

error:

  return self;
}

/*----------------------------------------------------------------------------*/

static bool resample_nocheck(ov_pcm_16_resampler const *self, int16_t const *in,
                             size_t no_in_samples, int16_t *out,
                             size_t no_out_samples) {

  OV_ASSERT(0 != self);
  OV_ASSERT(0 != self->h);
  OV_ASSERT(0 != in);
  OV_ASSERT(0 != out);
  OV_ASSERT(0 < no_in_samples);
  OV_ASSERT(0 < no_out_samples);

  OV_ASSERT(no_in_samples <= self->max_in_samples);
  OV_ASSERT(no_out_samples <= self->max_out_samples);

  for (size_t m = 0; m < no_out_samples; ++m) {

    double sum = 0;

    for (size_t n = 0; n < no_in_samples; ++n) {
      sum += in[n] * h_at(self, m, n);
    }

    out[m] = sum;
  }

  return true;
}

/*----------------------------------------------------------------------------*/

ssize_t ov_pcm_16_resample(ov_pcm_16_resampler *self, int16_t const *in,
                           size_t num_in_samples, int16_t *out,
                           size_t out_samples_capacity) {

  if (0 == in) {
    goto error;
  }

  if (0 == self) {
    return 0;
  }

  if (0 == num_in_samples) {
    return 0;
  }

  OV_ASSERT(0 != in);
  OV_ASSERT(0 < num_in_samples);

  double in_sample_length_s = 1.0 / (self->samplerate_in_hz);
  double out_sample_length_s = 1.0 / (self->samplerate_out_hz);

  double length_s = in_sample_length_s * num_in_samples;

  double num_output_samples_d = length_s;
  num_output_samples_d /= out_sample_length_s;

  size_t num_output_samples = round(num_output_samples_d);

  if ((0 == out) || (0 == out_samples_capacity)) {
    return num_output_samples;
  }

  OV_ASSERT(0 != out);

  size_t num_output_samples_to_generate = num_output_samples;

  if (num_output_samples_to_generate > out_samples_capacity) {
    num_output_samples_to_generate = out_samples_capacity;
  }

  if (!resample_nocheck(self, in, num_in_samples, out,
                        num_output_samples_to_generate)) {
    goto error;
  }

  return num_output_samples;

error:

  return -1;
}

/*****************************************************************************
                      Direct implementation - for ref only
 ****************************************************************************/

static double restored_signal(size_t m, const double out_sample_length_s,
                              int16_t const *samples, size_t num_samples,
                              const double in_sample_length_s,
                              const double pi_times_samplerate_in_hz) {

  double sum = 0;

  for (size_t n = 0; n < num_samples; ++n) {
    double t = m * out_sample_length_s - n * in_sample_length_s;
    sum += samples[n] * h(t, pi_times_samplerate_in_hz);
  }

  return sum;
}

/*----------------------------------------------------------------------------*/

static bool resample_uncached_nocheck(ov_pcm_16_resampler const *self,
                                      int16_t const *in,
                                      size_t number_of_in_samples, int16_t *out,
                                      size_t number_of_out_samples) {

  // Upsampling done via interpolation as described here:
  // https://ccrma.stanford.edu/~jos/resample/Theory_Ideal_Bandlimited_Interpolation.html

  const double in_sample_length_s = self->in_sample_length_s;
  const double out_sample_length_s = self->out_sample_length_s;

  const double pi_times_samplerate_in_hz = self->samplerate_in_hz * M_PI;

  for (size_t m = 0; m < number_of_out_samples; ++m) {
    out[m] = restored_signal(m, out_sample_length_s, in, number_of_in_samples,
                             in_sample_length_s, pi_times_samplerate_in_hz);
  }

  return true;
}

/*----------------------------------------------------------------------------*/

ssize_t ov_pcm_16_resample_uncached(ov_pcm_16_resampler const *self,
                                    int16_t const *in,
                                    const size_t num_in_samples, int16_t *out,
                                    const size_t out_samples_capacity) {

  if (0 == self) {
    goto error;
  }

  if (0 == in) {
    goto error;
  }

  const double samplerate_in_hz = self->samplerate_in_hz;
  const double samplerate_out_hz = self->samplerate_out_hz;

  if ((0 == samplerate_in_hz) || (0 == samplerate_out_hz)) {
    goto error;
  }

  if (0 == num_in_samples) {
    return 0;
  }

  if (samplerate_in_hz == samplerate_out_hz) {
    return 0;
  }

  OV_ASSERT(0 != in);
  OV_ASSERT(0 < num_in_samples);

  const double length_s = self->in_sample_length_s * num_in_samples;

  const size_t num_out_samples = round(length_s / self->out_sample_length_s);

  if ((0 == out) || (0 == out_samples_capacity)) {
    return num_out_samples;
  }

  OV_ASSERT(0 != out);

  const size_t num_out_samples_to_generate =
      OV_MIN(num_out_samples, out_samples_capacity);

  bool success = resample_uncached_nocheck(self, in, num_in_samples, out,
                                           num_out_samples_to_generate);

  if (!success) {
    goto error;
  }

  return num_out_samples;

error:

  return -1;
}
