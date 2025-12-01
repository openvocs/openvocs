/***

Copyright   2018        German Aerospace Center DLR e.V.,
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

     \file               ov_codec_opus_tests.c
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2018-02-28

     \ingroup            empty

     \brief              empty

 **/

#include "ov_codec_opus.c"

#include <math.h>
#include <ov_base/ov_utils.h>

#include <ov_test/ov_test.h>

#include <ov_base/ov_buffer.h>

/******************************************************************************
 *                         MANUAL encoding / decoding
 ******************************************************************************/

static ov_buffer *generate_pcm_sin(double freq_hz, double samplerate_hz,
                                   uint64_t frame_length_usecs) {

  /* Code taken from ov_pcm_gen.c - could not use their functions directly
   * because ov_pcm16s depends on ov_codec, thus would create cyclic
   * dependencies ...
   */
  const size_t num_samples = frame_length_usecs * samplerate_hz / (1000 * 1000);

  const size_t length = num_samples * sizeof(int16_t);

  OV_ASSERT(0 != length);

  ov_buffer *buffer = ov_buffer_create(length);

  OV_ASSERT(0 != buffer);
  OV_ASSERT(0 != buffer->start);
  OV_ASSERT(length <= buffer->capacity);

  buffer->length = length;

  int16_t *output = (int16_t *)buffer->start;

  double phase = 0;
  double amplitude = 0;

  const double PHASE_SHIFT_PER_SAMPLE = 2.0 * M_PI / samplerate_hz;

  for (size_t i = 0; i < num_samples; i++) {

    phase += PHASE_SHIFT_PER_SAMPLE * freq_hz;

    amplitude = sin(phase) * INT16_MAX;

    if (2.0 * M_PI < phase) {

      phase = asin(sin(phase));
      amplitude = sin(phase) * INT16_MAX;
    }

    *output = (int16_t)amplitude;
    output++;
  }

  return buffer;
}

/*----------------------------------------------------------------------------*/

static int16_t *interleave(size_t num_samples, int16_t const *in_1,
                           int16_t const *in_2, int16_t *out) {

  OV_ASSERT(0 != num_samples);
  OV_ASSERT(0 != num_samples);
  OV_ASSERT(0 != in_1);
  OV_ASSERT(0 != in_2);
  OV_ASSERT(0 != out);

  for (size_t i = 0; num_samples > i; ++i) {

    out[2 * i] = in_1[i];
    out[2 * i + 1] = in_2[i];
  }

  return out;
}

/*----------------------------------------------------------------------------*/

static ov_buffer *encode(ov_buffer const *in, int samplerate_hz, int channels) {

  const size_t samples_per_frame = in->length / (sizeof(int16_t) * channels);
  const size_t out_max_len = 8 * 1024;

  int error = OPUS_OK;

  OpusEncoder *encoder = opus_encoder_create(samplerate_hz, channels,
                                             OPUS_APPLICATION_VOIP, &error);

  if (OPUS_OK != error) {
    testrun_log_failure("Opus encoder: Error %i (OK = %i)\n", error, OPUS_OK);
    return 0;
  }

  OV_ASSERT(0 != encoder);

  ov_buffer *out = ov_buffer_create(out_max_len);

  int retval = opus_encode(encoder, (int16_t *)in->start, samples_per_frame,
                           (unsigned char *)out->start, out->capacity);

  opus_encoder_destroy(encoder);

  out->length = (size_t)retval;

  if (0 > retval) {

    testrun_log_failure("Opus encode error: %i\n", retval);
    out = ov_buffer_free(out);
  }

  encoder = 0;

  return out;
}
/*---------------------------------------------------------------------------*/

const char *test_data = "testdata";

/*---------------------------------------------------------------------------*/

static int test_ov_codec_opus_id() {

  testrun(0 == strcmp("opus", ov_codec_opus_id()));
  return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

static int test_impl_codec_create() {

  ov_codec *codec = impl_codec_create(0, 0);

  ov_codec_opus *codec_opus = (ov_codec_opus *)codec;

  testrun(0 != codec);

  testrun(ov_codec_opus_id == codec->type_id);
  testrun(impl_free == codec->free);
  testrun(impl_encode == codec->encode);
  testrun(impl_decode == codec->decode);
  testrun(impl_get_parameters == codec->get_parameters);
  testrun(0 == codec_opus->last_seq_number);

  /* Check that codec was set to default values */
  ov_json_value *params = codec->get_parameters(codec);
  testrun(0 != params);

  uint32_t sample_rate_hertz =
      ov_codec_parameters_get_sample_rate_hertz(params);

  ov_json_value *null_params = ov_json_object();

  testrun(ov_codec_parameters_get_sample_rate_hertz(null_params) ==
          sample_rate_hertz);

  null_params = null_params->free(null_params);
  params = params->free(params);
  codec = codec->free(codec);
  testrun(0 == codec);

  params = ov_json_object();
  ov_codec_parameters_set_sample_rate_hertz(params, 24000);

  codec = impl_codec_create(0, params);
  params = params->free(params);
  testrun(0 == params);

  /* Check that codec was set to default values */
  params = codec->get_parameters(codec);
  testrun(0 != params);

  sample_rate_hertz = ov_codec_parameters_get_sample_rate_hertz(params);

  testrun(24000 == sample_rate_hertz);

  params = params->free(params);
  testrun(0 == params);

  codec = codec->free(codec);

  return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

static int test_impl_free() {

  testrun(0 == impl_free(0));

  ov_codec *codec = impl_codec_create(0, 0);
  codec = impl_free(codec);
  testrun(0 == codec);

  ov_json_value *json = ov_json_object();
  ov_codec_parameters_set_sample_rate_hertz(json, 41233);

  codec = impl_codec_create(0, json);
  json = json->free(json);
  codec = impl_free(codec);
  testrun(0 == codec);

  return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

/* at 48k, Opus allows for 120, 240, 480, 960 samples, 2 bytes each */
#define INPUT_BUFFER_SIZE_BYTES (240 * 2)
#define BUFFER_SIZE_BYTES 1024

static int test_impl_encode() {

  testrun(0 == impl_decode(0, 0, 0, 0, 0, 0));

  const int32_t input_length = INPUT_BUFFER_SIZE_BYTES;
  uint8_t input[INPUT_BUFFER_SIZE_BYTES];

  uint8_t output[BUFFER_SIZE_BYTES] = {};

  for (size_t i = 0; i < sizeof(input); ++i) {

    input[i] = i;
  }

  ov_json_value *json = ov_json_object();
  ov_codec_parameters_set_sample_rate_hertz(json, 48000);

  ov_codec *codec = impl_codec_create(0, json);

  json = json->free(json);

  int32_t bytes_written =
      impl_encode(codec, input, input_length, output, BUFFER_SIZE_BYTES);

  testrun(bytes_written > 0);
  testrun(bytes_written <= input_length);

  bool output_nonzero = false;

  for (size_t i = 0; i < (uint32_t)bytes_written; ++i) {

    if (0 != output[i]) {

      output_nonzero = true;
      break;
    }
  }

  testrun(output_nonzero);

  codec = codec->free(codec);

  return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

static int test_impl_decode() {

  const int samplerate_hz = 48000;

  testrun(0 == impl_decode(0, 0, 0, 0, 0, 0));

  const int32_t input_length = INPUT_BUFFER_SIZE_BYTES;
  uint8_t input[INPUT_BUFFER_SIZE_BYTES];

  uint8_t output[BUFFER_SIZE_BYTES] = {};
  uint8_t decoded[BUFFER_SIZE_BYTES] = {};

  for (size_t i = 0; i < sizeof(input); ++i) {

    input[i] = i;
  }

  ov_json_value *json = ov_json_object();
  ov_codec_parameters_set_sample_rate_hertz(json, samplerate_hz);

  ov_codec *codec = impl_codec_create(0, json);

  json = json->free(json);

  int32_t enc_bytes_written =
      impl_encode(codec, input, input_length, output, BUFFER_SIZE_BYTES);

  testrun(enc_bytes_written > 0);
  testrun(enc_bytes_written <= input_length);

  int32_t dec_bytes_written =
      impl_decode(codec, 1 + ((ov_codec_opus *)codec)->last_seq_number, output,
                  enc_bytes_written, decoded, BUFFER_SIZE_BYTES);

  testrun(input_length == dec_bytes_written);

  /**************************************************************************
   *                Now check decoding from mono / stereo stream
   **************************************************************************/

  ov_buffer *channel_1 = generate_pcm_sin(150, samplerate_hz, 60000);
  ov_buffer *channel_2 = generate_pcm_sin(500, samplerate_hz, 60000);

  const size_t samples_in_channel_1 = channel_1->length / 2;

  /*-----------------------------------------------------------------------*/
  /* Check mono input */

  testrun_log("Mono input to mono decoder:\n");

  ov_buffer *channel_1_encoded = encode(channel_1, samplerate_hz, 1);

  testrun(0 != channel_1_encoded);
  testrun(0 != channel_1_encoded->length);
  testrun(channel_1->length >= channel_1_encoded->length);

  const size_t samples_in_frame = opus_packet_get_samples_per_frame(
      channel_1_encoded->start, samplerate_hz);

  const size_t samples_in_packet =
      opus_decoder_get_nb_samples(((ov_codec_opus *)codec)->decoder,
                                  channel_1_encoded->start, samplerate_hz);

  ov_buffer *decoded_result = ov_buffer_create(2 * channel_1->length);
  testrun(0 != decoded_result);
  testrun(2 * channel_1->length <= decoded_result->capacity);

  /* Check some parameters of encoded signal */

  testrun(1 == opus_packet_get_nb_channels(channel_1_encoded->start));
  testrun(samples_in_channel_1 == samples_in_packet);

  testrun_log("Opus packet contains %i channels, %zu samples per "
              "frame&channel, %zu samples in packet\n",
              opus_packet_get_nb_channels(channel_1_encoded->start),
              samples_in_frame, samples_in_packet);

  /* And decode */

  memset(decoded_result->start, 0, decoded_result->capacity);

  decoded_result->length =
      impl_decode(codec, 1 + ((ov_codec_opus *)codec)->last_seq_number,
                  channel_1_encoded->start, channel_1_encoded->length,
                  decoded_result->start, decoded_result->capacity);

  testrun_log("Decoded %zu samples, expected %zu samples\n",
              decoded_result->length / 2, samples_in_packet);
  testrun_log("Decoded to %zu bytes, expected %zu bytes\n",
              decoded_result->length, channel_1->length);

  testrun(decoded_result->length == channel_1->length);

  memset(decoded_result->start, 0, decoded_result->capacity);

  channel_1_encoded = ov_buffer_free(channel_1_encoded);
  testrun(0 == channel_1_encoded);

  /*-----------------------------------------------------------------------*/
  /* Check stereo input */

  testrun_log("Stereo input to mono decoder:\n");

  ov_buffer *interleaved = ov_buffer_create(2 * channel_1->length);
  testrun(0 != interleaved);
  testrun(2 * channel_1->length <= interleaved->capacity);

  testrun((int16_t *)interleaved->start ==
          interleave(samples_in_channel_1, (int16_t *)channel_1->start,
                     (int16_t *)channel_2->start,
                     (int16_t *)interleaved->start));

  interleaved->length = 2 * samples_in_channel_1 * sizeof(int16_t);

  ov_buffer *interleaved_encoded = encode(interleaved, samplerate_hz, 2);

  testrun(0 != interleaved_encoded);
  testrun(0 != interleaved_encoded->length);
  testrun(interleaved->length >= interleaved_encoded->length);

  const size_t samples_in_frame_stereo = opus_packet_get_samples_per_frame(
      interleaved_encoded->start, samplerate_hz);

  const size_t samples_in_packet_stereo =
      opus_decoder_get_nb_samples(((ov_codec_opus *)codec)->decoder,
                                  interleaved_encoded->start, samplerate_hz);

  /* Check some parameters of encoded signal */

  testrun(2 == opus_packet_get_nb_channels(interleaved_encoded->start));

  testrun_log("Opus packet contains %i channels, %zu samples per "
              "frame&channel, %zu samples in packet\n",
              opus_packet_get_nb_channels(interleaved_encoded->start),
              samples_in_frame_stereo, samples_in_packet_stereo);

  /* And decode */

  memset(decoded_result->start, 0, decoded_result->capacity);

  decoded_result->length =
      impl_decode(codec, 1 + ((ov_codec_opus *)codec)->last_seq_number,
                  interleaved_encoded->start, interleaved_encoded->length,
                  decoded_result->start, decoded_result->capacity);

  testrun_log("Decoded %zu samples, expected %zu samples\n",
              decoded_result->length / 2, samples_in_packet);
  testrun_log("Decoded to %zu bytes, expected %zu bytes\n",
              decoded_result->length, channel_1->length);

  testrun(decoded_result->length == channel_1->length);

  memset(decoded_result->start, 0, decoded_result->capacity);

  interleaved = ov_buffer_free(interleaved);
  interleaved_encoded = ov_buffer_free(interleaved_encoded);

  testrun(0 == interleaved);
  testrun(0 == interleaved_encoded);

  /**************************************************************************
   *                                  Clean up
   **************************************************************************/

  channel_1 = ov_buffer_free(channel_1);
  channel_2 = ov_buffer_free(channel_2);

  decoded_result = ov_buffer_free(decoded_result);

  codec = codec->free(codec);

  testrun(0 == channel_1);
  testrun(0 == channel_2);

  testrun(0 == decoded_result);

  testrun(0 == codec);

  return testrun_log_success();
}

#undef INPUT_BUFFER_SIZE_BYTES
#undef BUFFER_SIZE_BYTES

/*---------------------------------------------------------------------------*/

static int test_impl_get_parameters() {

  testrun(0 == impl_get_parameters(0));

  ov_codec *codec = impl_codec_create(0, 0);
  ov_json_value *json = impl_get_parameters(codec);

  testrun(0 != json);

  uint32_t sample_rate_hertz = ov_codec_parameters_get_sample_rate_hertz(0);

  testrun(sample_rate_hertz == ov_codec_parameters_get_sample_rate_hertz(json));

  json = json->free(json);
  codec->free(codec);

  json = ov_json_object();
  testrun(ov_codec_parameters_set_sample_rate_hertz(json, 24000));

  codec = impl_codec_create(0, json);

  json = json->free(json);

  json = codec->get_parameters(codec);

  testrun(24000 == ov_codec_parameters_get_sample_rate_hertz(json));

  json = json->free(json);
  codec->free(codec);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_impl_get_samplesrate_hertz() {

  uint32_t samplerate_hz = 16000;

  ov_json_value *json = ov_json_object();
  ov_codec_parameters_set_sample_rate_hertz(json, samplerate_hz);

  ov_codec *codec = impl_codec_create(0, json);
  testrun(0 != codec);

  json = ov_json_value_free(json);
  testrun(0 == json);

  testrun(samplerate_hz == impl_get_samplerate_hertz(codec));

  codec = ov_codec_free(codec);
  testrun(0 == codec);

  json = ov_json_object();
  ov_codec_parameters_set_sample_rate_hertz(json, samplerate_hz);

  codec = impl_codec_create(0, json);
  testrun(0 != codec);

  json = ov_json_value_free(json);
  testrun(0 == json);

  testrun(samplerate_hz == impl_get_samplerate_hertz(codec));

  codec = ov_codec_free(codec);
  testrun(0 == codec);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_codec_opus", test_ov_codec_opus_id, test_impl_codec_create,
            test_impl_free, test_impl_encode, test_impl_decode,
            test_impl_get_parameters, test_impl_get_samplesrate_hertz);

/*----------------------------------------------------------------------------*/
