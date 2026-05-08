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

     \file               ov_codec_pcm16_signed_tests.c
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2018-01-08

     \ingroup            empty

     \brief              empty

 **/

#include "ov_codec_pcm16_signed.c"
#include <ov_base/ov_constants.h>
#include <ov_test/ov_test.h>

/*---------------------------------------------------------------------------*/

const char *test_data = "testdata";

#if OV_BYTE_ORDER == OV_BIG_ENDIAN
const char *expected_data_be = "testdata";
const char *expected_data_le = "ettsadat";
#else
#if OV_BYTE_ORDER == OV_LITTLE_ENDIAN
/* We expect 2 subsequent bytes flipped ... */
const char *expected_data_be = "ettsadat";
const char *expected_data_le = "testdata";
#else
#error("Unknown byte order")
#endif
#endif

/*---------------------------------------------------------------------------*/

int test_ov_codec_pcm16_signed_id() {

    testrun(0 == strcmp("pcm16_signed", ov_codec_pcm16_signed_id()));
    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_codec_create() {

    ov_codec *codec = impl_codec_create(0, 0);

    testrun(0 != codec);

    struct ov_codec_pcm16_signed_struct *codec_pcm16_signed =
        (struct ov_codec_pcm16_signed_struct *)codec;

    testrun(ov_codec_pcm16_signed_id == codec->type_id);
    testrun(impl_free == codec->free);
    testrun(impl_encode_be == codec->encode);
    testrun(impl_decode_be == codec->decode);
    testrun(impl_get_parameters == codec->get_parameters);
    testrun(0 == codec_pcm16_signed->last_seq_number);

    codec = codec->free(codec);

    char const *params_le =
        "{\"" OV_KEY_ENDIANNESS "\":\"" OV_KEY_LITTLE_ENDIAN "\"}";

    ov_json_value *parameters =
        ov_json_value_from_string(params_le, strlen(params_le) + 1);

    testrun(0 != parameters);

    codec = impl_codec_create(0, parameters);

    testrun(0 != codec);

    codec_pcm16_signed = (struct ov_codec_pcm16_signed_struct *)codec;

    testrun(ov_codec_pcm16_signed_id == codec->type_id);
    testrun(impl_free == codec->free);
    testrun(impl_encode_le == codec->encode);
    testrun(impl_decode_le == codec->decode);
    testrun(impl_get_parameters == codec->get_parameters);
    testrun(0 == codec_pcm16_signed->last_seq_number);

    codec = codec->free(codec);

    parameters = ov_json_value_free(parameters);
    testrun(0 == parameters);

    char const *params_be =
        "{\"" OV_KEY_ENDIANNESS "\":\"" OV_KEY_BIG_ENDIAN "\"}";

    parameters = ov_json_value_from_string(params_be, strlen(params_be) + 1);

    testrun(0 != parameters);

    codec = impl_codec_create(0, parameters);

    testrun(0 != codec);

    codec_pcm16_signed = (struct ov_codec_pcm16_signed_struct *)codec;

    testrun(ov_codec_pcm16_signed_id == codec->type_id);
    testrun(impl_free == codec->free);
    testrun(impl_encode_be == codec->encode);
    testrun(impl_decode_be == codec->decode);
    testrun(impl_get_parameters == codec->get_parameters);
    testrun(0 == codec_pcm16_signed->last_seq_number);

    codec = codec->free(codec);

    parameters = ov_json_value_free(parameters);
    testrun(0 == parameters);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_free() {

    testrun(0 == impl_free(0));

    ov_codec *codec = impl_codec_create(0, 0);
    testrun(0 == impl_free(codec));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_encode_be() {

    testrun(0 == impl_encode_be(0, 0, 0, 0, 0));
    const size_t len = sizeof(test_data);

    char *out_buffer = calloc(1, sizeof(test_data));

    uint8_t *in = (uint8_t *)test_data;
    uint8_t *out = (uint8_t *)out_buffer;

    testrun(0 == impl_encode_be(0, in, len, out, len));

    ov_codec *codec = impl_codec_create(0, 0);

    testrun(0 == impl_encode_be(codec, in, len, out, 0));
    testrun(0 == impl_encode_be(codec, in, len, 0, len));
    testrun(0 == impl_encode_be(codec, in, 0, out, len));
    testrun(0 == impl_encode_be(codec, 0, len, out, len));
    testrun(0 == impl_encode_be(codec, in, len, out, len - 1));

    int32_t encoded_bytes = impl_encode_be(codec, in, len, out, len);
    testrun(0 < encoded_bytes);
    testrun(len == (uint32_t)encoded_bytes);

    testrun(0 == strncmp((char *)expected_data_be, (char *)out, len));

    free(out);

    testrun(0 == impl_free(codec));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_decode_be() {

    testrun(0 == impl_decode_be(0, 0, 0, 0, 0, 0));

    const size_t len = sizeof(test_data);

    char *out_buffer = calloc(1, sizeof(test_data));

    uint8_t *in = (uint8_t *)test_data;
    uint8_t *out = (uint8_t *)out_buffer;

    testrun(0 == impl_decode_be(0, 0, in, len, out, len));

    ov_codec *codec = impl_codec_create(0, 0);

    testrun(0 == impl_decode_be(codec, 0, in, len, out, 0));
    testrun(0 == impl_decode_be(codec, 0, in, len, 0, len));
    testrun(0 == impl_decode_be(codec, 0, in, 0, out, len));
    testrun(0 == impl_decode_be(codec, 0, 0, len, out, len));
    testrun(0 == impl_decode_be(codec, 0, in, len, out, len - 1));

    int32_t decoded_bytes = impl_decode_be(codec, 0, in, len, out, len);
    testrun(0 < decoded_bytes);
    testrun(len == (uint32_t)decoded_bytes);

    testrun(0 == strncmp((char *)expected_data_be, (char *)out, len));

    free(out);

    testrun(0 == impl_free(codec));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_encode_le() {

    testrun(0 == impl_encode_le(0, 0, 0, 0, 0));
    const size_t len = sizeof(test_data);

    char *out_buffer = calloc(1, sizeof(test_data));

    uint8_t *in = (uint8_t *)test_data;
    uint8_t *out = (uint8_t *)out_buffer;

    testrun(0 == impl_encode_le(0, in, len, out, len));

    ov_codec *codec = impl_codec_create(0, 0);

    testrun(0 == impl_encode_le(codec, in, len, out, 0));
    testrun(0 == impl_encode_le(codec, in, len, 0, len));
    testrun(0 == impl_encode_le(codec, in, 0, out, len));
    testrun(0 == impl_encode_le(codec, 0, len, out, len));
    testrun(0 == impl_encode_le(codec, in, len, out, len - 1));

    int32_t encoded_bytes = impl_encode_le(codec, in, len, out, len);
    testrun(0 < encoded_bytes);
    testrun(len == (uint32_t)encoded_bytes);

    testrun(0 == strncmp((char *)expected_data_le, (char *)out, len));

    free(out);

    testrun(0 == impl_free(codec));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_decode_le() {

    testrun(0 == impl_decode_le(0, 0, 0, 0, 0, 0));

    const size_t len = sizeof(test_data);

    char *out_buffer = calloc(1, sizeof(test_data));

    uint8_t *in = (uint8_t *)test_data;
    uint8_t *out = (uint8_t *)out_buffer;

    testrun(0 == impl_decode_le(0, 0, in, len, out, len));

    ov_codec *codec = impl_codec_create(0, 0);

    testrun(0 == impl_decode_le(codec, 0, in, len, out, 0));
    testrun(0 == impl_decode_le(codec, 0, in, len, 0, len));
    testrun(0 == impl_decode_le(codec, 0, in, 0, out, len));
    testrun(0 == impl_decode_le(codec, 0, 0, len, out, len));
    testrun(0 == impl_decode_le(codec, 0, in, len, out, len - 1));

    int32_t decoded_bytes = impl_decode_le(codec, 0, in, len, out, len);
    testrun(0 < decoded_bytes);
    testrun(len == (uint32_t)decoded_bytes);

    testrun(0 == strncmp((char *)expected_data_le, (char *)out, len));

    free(out);

    testrun(0 == impl_free(codec));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_get_parameters() {

    testrun(0 == impl_get_parameters(0));

    ov_codec *codec = impl_codec_create(0, 0);
    ov_json_value *json = impl_get_parameters(codec);

    testrun(0 != json);

    char const *endianness =
        ov_json_string_get(ov_json_get(json, "/" OV_KEY_ENDIANNESS));

    testrun(0 != endianness);

    testrun(0 == strcmp(OV_KEY_BIG_ENDIAN, endianness));

    json = json->free(json);
    testrun(0 == json);

    codec = codec->free(codec);
    testrun(0 == codec);

    /*************************************************************************
                              Explicit big endian
     ************************************************************************/

    char const *params_be =
        "{\"" OV_KEY_ENDIANNESS "\":\"" OV_KEY_BIG_ENDIAN "\"}";

    ov_json_value *parameters =
        ov_json_value_from_string(params_be, strlen(params_be) + 1);

    testrun(0 != parameters);

    codec = impl_codec_create(0, parameters);

    json = impl_get_parameters(codec);
    testrun(0 != json);

    endianness = ov_json_string_get(ov_json_get(json, "/" OV_KEY_ENDIANNESS));
    testrun(0 != endianness);

    testrun(0 == strcmp(OV_KEY_BIG_ENDIAN, endianness));

    parameters = ov_json_value_free(parameters);
    testrun(0 == parameters);

    json = json->free(json);
    testrun(0 == json);

    codec = codec->free(codec);
    testrun(0 == codec);

    /*************************************************************************
                                 Little endian
     ************************************************************************/

    char const *params_le =
        "{\"" OV_KEY_ENDIANNESS "\":\"" OV_KEY_LITTLE_ENDIAN "\"}";

    parameters = ov_json_value_from_string(params_le, strlen(params_le) + 1);

    testrun(0 != parameters);

    codec = impl_codec_create(0, parameters);

    json = impl_get_parameters(codec);
    testrun(0 != json);

    endianness = ov_json_string_get(ov_json_get(json, "/" OV_KEY_ENDIANNESS));
    testrun(0 != endianness);

    testrun(0 == strcmp(OV_KEY_LITTLE_ENDIAN, endianness));

    parameters = ov_json_value_free(parameters);
    testrun(0 == parameters);

    json = json->free(json);
    testrun(0 == json);

    codec = codec->free(codec);
    testrun(0 == codec);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

static int test_impl_get_samplerate_hertz() {

    uint32_t samplerate_hz = 16000;

    ov_json_value *json = ov_json_object();
    ov_codec_parameters_set_sample_rate_hertz(json, samplerate_hz);

    ov_codec *codec = impl_codec_create(0, json);
    testrun(0 != codec);

    json = ov_json_value_free(json);
    testrun(0 == json);

    /* PCM codec is sample rate unaware - thus default is assumed */
    testrun(OV_DEFAULT_SAMPLERATE == ov_codec_get_samplerate_hertz(codec));

    codec = ov_codec_free(codec);
    testrun(0 == codec);

    json = ov_json_object();
    ov_codec_parameters_set_sample_rate_hertz(json, samplerate_hz);

    codec = impl_codec_create(0, json);
    testrun(0 != codec);

    json = ov_json_value_free(json);
    testrun(0 == json);

    /* PCM codec is sample rate unaware - thus default is assumed */
    testrun(OV_DEFAULT_SAMPLERATE == ov_codec_get_samplerate_hertz(codec));

    codec = ov_codec_free(codec);
    testrun(0 == codec);

    return testrun_log_success();
}
/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_codec_pcm16_signed", test_ov_codec_pcm16_signed_id,
            test_impl_codec_create, test_impl_free, test_impl_encode_be,
            test_impl_decode_be, test_impl_encode_le, test_impl_decode_le,
            test_impl_get_parameters, test_impl_get_samplerate_hertz);
