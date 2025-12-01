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

     \file               ov_codec_tests.c
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2017-12-06

     \ingroup            empty

     \brief              empty

 **/

#include "ov_codec.c"

#include <ov_arch/ov_arch_math.h>
#include <ov_base/ov_utils.h>
#include <ov_test/ov_test.h>

/*****************************************************************************
                                   Test codec
 ****************************************************************************/

#define TEST_CODEC_TYPE_ID "test_codec"

static const char *test_codec_type_id() { return TEST_CODEC_TYPE_ID; }

/*----------------------------------------------------------------------------*/

static ov_codec *test_codec_free(ov_codec *codec) {

  if (0 != codec) {

    free(codec);
  }

  return 0;
}

/*----------------------------------------------------------------------------*/

static int32_t test_codec_encode(ov_codec *codec, const uint8_t *input,
                                 size_t length, uint8_t *output,
                                 size_t max_out_length) {

  if ((0 == codec) || (0 == input) || (0 == output)) {
    goto error;
  }

  size_t bytes_encoded = 0;

  for (size_t i = 0; i < OV_MIN(length, max_out_length); ++i) {

    output[i] = 1 + input[i];

    ++bytes_encoded;
  }

  return bytes_encoded;

error:

  return -1;
}

/*----------------------------------------------------------------------------*/

static int32_t test_codec_decode(ov_codec *codec, uint64_t seq_number,
                                 const uint8_t *input, size_t length,
                                 uint8_t *output, size_t max_out_length) {

  UNUSED(seq_number);

  if ((0 == codec) || (0 == input) || (0 == output)) {
    goto error;
  }

  size_t bytes_decoded = 0;

  for (size_t i = 0; i < OV_MIN(length, max_out_length); ++i) {

    output[i] = input[i] - 1;
    ++bytes_decoded;
  }

  return bytes_decoded;

error:

  return -1;
}

/*----------------------------------------------------------------------------*/

static ov_json_value const *TEST_PARAMETERS =
    (ov_json_value *)(&TEST_PARAMETERS);

static ov_json_value *test_codec_get_parameters(const ov_codec *codec) {

  if (0 == codec)
    return 0;

  return (ov_json_value *)TEST_PARAMETERS;
}

/*----------------------------------------------------------------------------*/

static const uint32_t TEST_SAMPLERATE = 123;

static uint32_t test_codec_get_samplerate_hertz(const ov_codec *codec) {

  UNUSED(codec);

  return TEST_SAMPLERATE;
}

/*----------------------------------------------------------------------------*/

static uint32_t get_samplerate_hertz_16k(ov_codec const *codec) {

  UNUSED(codec);

  return 16000;
}

/*----------------------------------------------------------------------------*/

static ov_codec *get_test_codec() {

  ov_codec *codec = calloc(1, sizeof(ov_codec));

  OV_ASSERT(0 != codec);

  *codec = (ov_codec){

      .type_id = test_codec_type_id,
      .free = test_codec_free,
      .encode = test_codec_encode,
      .decode = test_codec_decode,
      .get_parameters = test_codec_get_parameters,
      .get_samplerate_hertz = test_codec_get_samplerate_hertz,

  };

  return codec;
}

/*----------------------------------------------------------------------------
 *  HELPERS
 *----------------------------------------------------------------------------*/

ov_json_value *create_test_json(const ov_codec *codec) {

  UNUSED(codec);

  ov_json_value *json = ov_json_object();

  ov_json_object_set(json, "test", ov_json_string("test"));

  return json;
}

/*----------------------------------------------------------------------------*/

const char *codec_id() { return "R'Lyeh"; }

/*----------------------------------------------------------------------------
 * TESTS
 *----------------------------------------------------------------------------*/

static int test_ov_codec_free() {

  testrun(0 == ov_codec_free(0));

  ov_codec *codec = get_test_codec();

  codec = ov_codec_free(codec);

  testrun(0 == codec);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_codec_type_id() {

  testrun(0 == strcmp(CODEC_NONE_STRING, ov_codec_type_id(0)));

  ov_codec *codec = get_test_codec();
  testrun(0 != codec);

  testrun(0 == strcmp(TEST_CODEC_TYPE_ID, ov_codec_type_id(codec)));

  codec = ov_codec_free(codec);
  testrun(0 == codec);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_codec_encode() {

  testrun(0 > ov_codec_encode(0, 0, 0, 0, 0));

  uint8_t input[] = {1, 2, 3, 4, 5};
  uint8_t output[12 * 4] = {0};

  size_t input_len = sizeof(input);
  size_t output_len = sizeof(output);

  ov_codec *c = get_test_codec();

  testrun(0 > ov_codec_encode(c, 0, 0, 0, 0));
  testrun(0 > ov_codec_encode(0, input, 0, 0, 0));
  testrun(0 > ov_codec_encode(c, input, 0, 0, 0));
  testrun(0 > ov_codec_encode(0, 0, input_len, 0, 0));
  testrun(0 > ov_codec_encode(c, 0, input_len, 0, 0));
  testrun(0 > ov_codec_encode(0, input, input_len, 0, 0));
  testrun(0 > ov_codec_encode(c, input, input_len, 0, 0));
  testrun(0 > ov_codec_encode(0, 0, 0, output, 0));
  testrun(0 > ov_codec_encode(c, 0, 0, output, 0));
  testrun(0 > ov_codec_encode(0, input, 0, output, 0));
  testrun(0 == ov_codec_encode(c, input, 0, output, 0));
  testrun(0 > ov_codec_encode(0, input, input_len, output, 0));
  testrun(0 == ov_codec_encode(c, input, input_len, output, 0));
  testrun(0 > ov_codec_encode(0, 0, 0, 0, output_len));
  testrun(0 > ov_codec_encode(c, 0, 0, 0, output_len));
  testrun(0 > ov_codec_encode(0, input, 0, 0, output_len));
  testrun(0 > ov_codec_encode(c, input, 0, 0, output_len));
  testrun(0 > ov_codec_encode(0, 0, input_len, 0, output_len));
  testrun(0 > ov_codec_encode(c, 0, input_len, 0, output_len));
  testrun(0 > ov_codec_encode(0, input, input_len, 0, output_len));
  testrun(0 > ov_codec_encode(c, input, input_len, 0, output_len));
  testrun(0 > ov_codec_encode(0, 0, 0, output, output_len));
  testrun(0 > ov_codec_encode(c, 0, 0, output, output_len));
  testrun(0 > ov_codec_encode(0, input, 0, output, output_len));
  testrun(0 == ov_codec_encode(c, input, 0, output, output_len));
  testrun(0 > ov_codec_encode(0, 0, 0, output, output_len));
  testrun(0 > ov_codec_encode(c, 0, 0, output, output_len));
  testrun(0 > ov_codec_encode(0, 0, input_len, output, output_len));
  testrun(0 > ov_codec_encode(c, 0, input_len, output, output_len));
  testrun(0 > ov_codec_encode(0, input, input_len, output, output_len));
  testrun(sizeof(input) ==
          ov_codec_encode(c, input, input_len, output, output_len));

  for (size_t i = 0; i < sizeof(input); ++i) {
    testrun(1 + input[i] == output[i]);
  }

  memset(output, 0, sizeof(output));

  testrun(2 == ov_codec_encode(c, input, input_len, output, 2));

  testrun(1 + input[0] == output[0]);
  testrun(1 + input[1] == output[1]);

  for (size_t i = 2; i < sizeof(input); ++i) {
    testrun(0 == output[i]);
  }

  memset(output, 0, sizeof(output));

  testrun(2 == ov_codec_encode(c, input, 2, output, output_len));

  testrun(1 + input[0] == output[0]);
  testrun(1 + input[1] == output[1]);

  for (size_t i = 2; i < sizeof(input); ++i) {
    testrun(0 == output[i]);
  }

  // Test resampling
  // Test using 16kHz - that is, when decoding.
  // input is 12 samples.
  // Decoded to 48k, thus the length increase to 4 * 12 = 48 samples

  const int16_t test_input[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  const int16_t test_input_len = sizeof(test_input) / sizeof(test_input[0]);

  c->get_samplerate_hertz = get_samplerate_hertz_16k;
  testrun(ov_codec_enable_resampling(c));

  // 12 samples, from 48k to 16k => 12/3 = 4 samples aka 4 * 2 bytes
  testrun((2 * test_input_len) / 3 == ov_codec_encode(c, (uint8_t *)test_input,
                                                      sizeof(test_input),
                                                      output, sizeof(output)));

  c = ov_codec_free(c);
  testrun(0 == c);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_codec_decode() {

  testrun(0 > ov_codec_decode(0, 0, 0, 0, 0, 0));

  uint8_t input[] = {1, 2, 3, 4, 5};

  // Let's give us some buffer for the output
  uint8_t resample_output[sizeof(input) * 10] = {0};

  size_t input_len = sizeof(input);
  size_t output_len = sizeof(resample_output);

  ov_codec *c = get_test_codec();

  testrun(0 > ov_codec_decode(c, 0, 0, 0, 0, 0));
  testrun(0 > ov_codec_decode(0, 0, input, 0, 0, 0));
  testrun(0 > ov_codec_decode(c, 0, input, 0, 0, 0));
  testrun(0 > ov_codec_decode(0, 0, 0, input_len, 0, 0));
  testrun(0 > ov_codec_decode(c, 0, 0, input_len, 0, 0));
  testrun(0 > ov_codec_decode(0, 0, input, input_len, 0, 0));
  testrun(0 > ov_codec_decode(c, 0, input, input_len, 0, 0));
  testrun(0 > ov_codec_decode(0, 0, 0, 0, resample_output, 0));
  testrun(0 > ov_codec_decode(c, 0, 0, 0, resample_output, 0));
  testrun(0 > ov_codec_decode(0, 0, input, 0, resample_output, 0));
  testrun(0 == ov_codec_decode(c, 0, input, 0, resample_output, 0));
  testrun(0 > ov_codec_decode(0, 0, input, input_len, resample_output, 0));
  testrun(0 == ov_codec_decode(c, 0, input, input_len, resample_output, 0));
  testrun(0 > ov_codec_decode(0, 0, 0, 0, 0, output_len));
  testrun(0 > ov_codec_decode(c, 0, 0, 0, 0, output_len));
  testrun(0 > ov_codec_decode(0, 0, input, 0, 0, output_len));
  testrun(0 > ov_codec_decode(c, 0, input, 0, 0, output_len));
  testrun(0 > ov_codec_decode(0, 0, 0, input_len, 0, output_len));
  testrun(0 > ov_codec_decode(c, 0, 0, input_len, 0, output_len));
  testrun(0 > ov_codec_decode(0, 0, input, input_len, 0, output_len));
  testrun(0 > ov_codec_decode(c, 0, input, input_len, 0, output_len));
  testrun(0 > ov_codec_decode(0, 0, 0, 0, resample_output, output_len));
  testrun(0 > ov_codec_decode(c, 0, 0, 0, resample_output, output_len));
  testrun(0 > ov_codec_decode(0, 0, input, 0, resample_output, output_len));
  testrun(0 == ov_codec_decode(c, 0, input, 0, resample_output, output_len));
  testrun(0 > ov_codec_decode(0, 0, 0, 0, resample_output, output_len));
  testrun(0 > ov_codec_decode(c, 0, 0, 0, resample_output, output_len));
  testrun(0 > ov_codec_decode(0, 0, 0, input_len, resample_output, output_len));
  testrun(0 > ov_codec_decode(c, 0, 0, input_len, resample_output, output_len));
  testrun(0 >
          ov_codec_decode(0, 0, input, input_len, resample_output, output_len));

  testrun(sizeof(input) ==
          ov_codec_decode(c, 0, input, input_len, resample_output, output_len));

  for (size_t i = 0; i < sizeof(input); ++i) {

    testrun(input[i] - 1 == resample_output[i]);
  }

  memset(resample_output, 0, sizeof(resample_output));

  testrun(2 == ov_codec_decode(c, 0, input, input_len, resample_output, 2));

  testrun(input[0] - 1 == resample_output[0]);
  testrun(input[1] - 1 == resample_output[1]);

  for (size_t i = 2; i < sizeof(input); ++i) {
    testrun(0 == resample_output[i]);
  }

  memset(resample_output, 0, sizeof(resample_output));

  testrun(2 == ov_codec_decode(c, 0, input, 2, resample_output, output_len));

  testrun(input[0] - 1 == resample_output[0]);
  testrun(input[1] - 1 == resample_output[1]);

  for (size_t i = 2; i < sizeof(input); ++i) {

    testrun(0 == resample_output[i]);
  }
  //
  // Test resampling
  // Test using 16kHz - that is, when decoding.
  // input is 12 samples.
  // Decoded to 48k, thus the length increase to 4 * 12 = 48 samples

  const int8_t test_input[] = {1, 2, 3, 4};
  const int16_t test_input_len = (int16_t)sizeof(test_input);

  c->get_samplerate_hertz = get_samplerate_hertz_16k;
  testrun(ov_codec_enable_resampling(c));

  // 4 samples, from 16k to 48k => 4*3 = 12 samples aka 12 * 2 bytes
  int32_t result =
      ov_codec_decode(c, 0, (uint8_t *)test_input, sizeof(test_input),
                      resample_output, sizeof(resample_output));

  testrun((test_input_len * 3) == result);

  c = ov_codec_free(c);
  testrun(0 == c);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_codec_get_parameters() {

  testrun(0 == ov_codec_get_parameters(0));

  ov_codec *c = get_test_codec();

  testrun(0 != c);

  testrun(TEST_PARAMETERS == ov_codec_get_parameters(c));

  c = ov_codec_free(c);

  testrun(0 == c);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_codec_get_samplerate_hertz() {

  testrun(0 == ov_codec_get_samplerate_hertz(0));

  ov_codec *c = get_test_codec();

  testrun(TEST_SAMPLERATE == ov_codec_get_samplerate_hertz(c));

  c->get_samplerate_hertz = 0;

  testrun(OV_DEFAULT_SAMPLERATE == ov_codec_get_samplerate_hertz(c));

  c = ov_codec_free(c);

  testrun(0 == c);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_codec_to_json() {

  testrun(0 == ov_codec_to_json(0));

  ov_codec *codec = calloc(1, sizeof(ov_codec));
  codec->get_parameters = create_test_json;
  codec->type_id = codec_id;

  ov_json_value *json = ov_codec_to_json(codec);

  testrun(0 != json);

  char const *type = ov_json_string_get(ov_json_object_get(json, OV_KEY_CODEC));
  testrun(0 != type);
  testrun(0 == strncmp(type, "R'Lyeh", 7));
  type = 0;

  char const *param = ov_json_string_get(ov_json_object_get(json, "test"));

  testrun(0 != param);
  testrun(0 == strncmp(param, "test", 7));
  param = 0;

  json = json->free(json);

  free(codec);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_codec_parameters_get_sample_rate_hertz() {

  testrun(OV_DEFAULT_SAMPLERATE ==
          ov_codec_parameters_get_sample_rate_hertz(0));

  ov_json_value *json = ov_json_object();

  testrun(OV_DEFAULT_SAMPLERATE ==
          ov_codec_parameters_get_sample_rate_hertz(json));

  ov_json_object_set(json, OV_KEY_SAMPLE_RATE_HERTZ, ov_json_number(41233));

  testrun(41233 == ov_codec_parameters_get_sample_rate_hertz(json));

  json = json->free(json);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_codec_parameters_set_sample_rate_hertz() {

  testrun(!ov_codec_parameters_set_sample_rate_hertz(0, 411));

  ov_json_value *json = ov_json_object();
  testrun(0 != json);

  testrun(ov_codec_parameters_set_sample_rate_hertz(json, 411));
  testrun(411 == ov_codec_parameters_get_sample_rate_hertz(json));

  testrun(ov_codec_parameters_set_sample_rate_hertz(json, 412));
  testrun(412 == ov_codec_parameters_get_sample_rate_hertz(json));

  json = json->free(json);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_codec_enable_resampling() {

  testrun(!ov_codec_enable_resampling(0));

  // Remainder is tested in test_ov_codec_encode / decode

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_codec", test_ov_codec_free, test_ov_codec_type_id,
            test_ov_codec_encode, test_ov_codec_decode,
            test_ov_codec_get_parameters, test_ov_codec_get_samplerate_hertz,
            test_ov_codec_to_json,
            test_ov_codec_parameters_get_sample_rate_hertz,
            test_ov_codec_parameters_set_sample_rate_hertz,
            test_ov_codec_enable_resampling);

/*----------------------------------------------------------------------------*/
