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

     \file               ov_codec_raw_tests.c
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2017-12-07

     \ingroup            empty

     \brief              empty

 **/

#include "ov_codec_raw.c"
#include <ov_base/ov_constants.h>
#include <ov_test/ov_test.h>

/*---------------------------------------------------------------------------*/

int test_ov_codec_raw_id() {

    testrun(0 == strncmp("raw", ov_codec_raw_id(), 4));
    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_codec_create() {

    ov_codec *codec = impl_codec_create(0, 0);

    struct ov_codec_raw_struct *codec_raw = (struct ov_codec_raw_struct *)codec;

    testrun(0 != codec);

    testrun(ov_codec_raw_id == codec->type_id);
    testrun(impl_free == codec->free);
    testrun(impl_encode == codec->encode);
    testrun(impl_decode == codec->decode);
    testrun(impl_get_parameters == codec->get_parameters);
    testrun(0 == codec_raw->last_seq_number);

    codec = codec->free(codec);

    testrun(0 == codec);

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

int test_impl_encode() {

    testrun(0 == impl_encode(0, 0, 0, 0, 0));

    const char *test_data = "testdata";

    const size_t len = sizeof(test_data);

    char *out_buffer = calloc(1, sizeof(test_data));

    uint8_t *in = (uint8_t *)test_data;
    uint8_t *out = (uint8_t *)out_buffer;

    testrun(0 == impl_encode(0, in, len, out, len));

    ov_codec *codec = impl_codec_create(0, 0);

    testrun(0 == impl_encode(codec, in, len, out, 0));
    testrun(0 == impl_encode(codec, in, len, 0, len));
    testrun(0 == impl_encode(codec, in, 0, out, len));
    testrun(0 == impl_encode(codec, 0, len, out, len));
    testrun(0 == impl_encode(codec, in, len, out, len - 1));

    int32_t encoded_bytes = impl_encode(codec, in, len, out, len);

    testrun(0 < encoded_bytes);
    testrun(len == (uint32_t)encoded_bytes);

    testrun(0 == strncmp((char *)in, (char *)out, len));

    free(out_buffer);

    testrun(0 == impl_free(codec));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_decode() {

    testrun(0 == impl_decode(0, 0, 0, 0, 0, 0));

    const char *test_data = "testdata";

    const size_t len = sizeof(test_data);

    char *out_buffer = calloc(1, sizeof(test_data));

    uint8_t *in = (uint8_t *)test_data;
    uint8_t *out = (uint8_t *)out_buffer;

    testrun(0 == impl_decode(0, 0, in, len, out, len));

    ov_codec *codec = impl_codec_create(0, 0);

    testrun(0 == impl_decode(codec, 0, in, len, out, 0));
    testrun(0 == impl_decode(codec, 0, in, len, 0, len));
    testrun(0 == impl_decode(codec, 0, in, 0, out, len));
    testrun(0 == impl_decode(codec, 0, 0, len, out, len));
    testrun(0 == impl_decode(codec, 0, in, len, out, len - 1));

    int32_t impl_decoded_bytes = impl_decode(codec, 0, in, len, out, len);

    testrun(0 < impl_decoded_bytes);
    testrun(len == (uint32_t)impl_decoded_bytes);

    testrun(0 == strncmp((char *)in, (char *)out, len));

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

    testrun(0 == ov_json_object_count(json));

    json = json->free(json);
    codec->free(codec);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

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

OV_TEST_RUN("ov_codec_raw",
            test_ov_codec_raw_id,
            test_impl_codec_create,
            test_impl_free,
            test_impl_encode,
            test_impl_decode,
            test_impl_get_parameters,
            test_impl_get_samplerate_hertz);

/*----------------------------------------------------------------------------*/
