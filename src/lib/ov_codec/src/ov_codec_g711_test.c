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

     \file               ov_codec_g711_tests.c
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2018-06-05

     \ingroup            empty

     \brief              empty

 **/

#include "ov_codec_g711.c"
#include <ov_test/testrun.h>

/******************************************************************************
 *                                   a-law
 ******************************************************************************/
/* a-Law Reference values taken from ITU Recommendation ITU-T G.711 */

/* even bits not yet inverted ... */
uint8_t alaw_encoded[] = {
    0xff, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x80, /* positive values */
    0x00, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x7f  /* negative values */
};

int16_t alaw_decoded[] = {4032, 2112, 1056, 528,  264,  132,   66,    1,
                          -1,   -66,  -132, -264, -528, -1056, -2112, -4032};

/* a-Law Reference signal taken from ITU Recommendation ITU-T G.711, Table
 * 5/G.711 */
/* Fully G.711 A-Law encoded */
uint8_t alaw_reference_encoded[] = {0x34, 0x21, 0x21, 0x34,
                                    0xb4, 0xa1, 0xa1, 0xb4};

int16_t alaw_reference_decoded[] = {-1120, -2624, -2624, -1120,
                                    1120,  2624,  2624,  1120};

/******************************************************************************
 *                                   u-law
 ******************************************************************************/

// TODO: Quantized value 0x7f ???

uint8_t ulaw_encoded[] = {
    0x80, 0x8f, 0x9f, 0xaf, 0xbf,
    0xcf, 0xdf, 0xef, 0xfe, 0xff, /* positive values */
    0x7e, 0x6f, 0x5f, 0x4f, 0x3f,
    0x2f, 0x1f, 0x0f, 0x01, 0x00 /* negative values */
};

int16_t ulaw_decoded[] = {8031, 4191,  2079,  1023,  495,   231,  99,
                          33,   2,     0,     -2,    -33,   -99,  -231,
                          -495, -1023, -2079, -4191, -7775, -8031};

/* u-Law Reference signal taken from ITU Recommendation ITU-T G.711, Table
 * 6/G.711 */
/* Fully G.711 A-Law encoded */
uint8_t ulaw_reference_encoded[] = {0x34, 0x21, 0x21, 0x34,
                                    0xb4, 0xa1, 0xa1, 0xb4};

int16_t ulaw_reference_decoded[] = {-847, -1919, -1919, -847,
                                    847,  1919,  1919,  847};

/*---------------------------------------------------------------------------*/

int test_ov_codec_g711_id() {

    testrun(0 == strncmp("G.711", ov_codec_g711_id(), 5));
    return testrun_log_success();
}

/******************************************************************************
 *                                   TESTS
 ******************************************************************************/

int test_impl_codec_create() {

    ov_codec *codec = impl_codec_create(0, 0);

    struct codec_g711_struct *codec_g711 = (struct codec_g711_struct *)codec;

    testrun(0 != codec);

    testrun(ov_codec_g711_id == codec->type_id);
    testrun(impl_free == codec->free);
    testrun(impl_encode == codec->encode);
    testrun(impl_decode == codec->decode);
    testrun(impl_get_parameters == codec->get_parameters);
    testrun(0 == codec_g711->last_seq_number);

    /* We gave no parameters - thus using u-Law */
    testrun(ulaw_expand == codec_g711->expand);
    testrun(ulaw_compress == codec_g711->compress);

    codec = codec->free(codec);

    ov_json_value *parameters = ov_json_decode("{\"" CONFIG_KEY_G711_LAW "\":"
                                               "\"Toblero"
                                               "ne\"}");

    testrun(0 == impl_codec_create(0, parameters));

    parameters = parameters->free(parameters);
    testrun(0 == parameters);

    parameters = ov_json_decode("{\"" CONFIG_KEY_G711_LAW
                                "\":\"" CONFIG_KEY_G711_ULAW "\""
                                "}");

    codec = impl_codec_create(0, parameters);
    codec_g711 = (struct codec_g711_struct *)codec;
    parameters = parameters->free(parameters);

    testrun(0 != codec);
    testrun(ov_codec_g711_id == codec->type_id);
    testrun(impl_free == codec->free);
    testrun(impl_encode == codec->encode);
    testrun(impl_decode == codec->decode);
    // testrun(impl_get_parameters == codec->impl_get_parameters);
    testrun(0 == codec_g711->last_seq_number);

    testrun(ulaw_expand == codec_g711->expand);
    testrun(ulaw_compress == codec_g711->compress);

    codec = codec->free(codec);

    parameters = ov_json_decode("{\"" CONFIG_KEY_G711_LAW
                                "\":\"" CONFIG_KEY_G711_ALAW "\""
                                "}");

    codec = impl_codec_create(0, parameters);
    codec_g711 = (struct codec_g711_struct *)codec;
    parameters = parameters->free(parameters);

    testrun(0 != codec);
    testrun(ov_codec_g711_id == codec->type_id);
    testrun(impl_free == codec->free);
    testrun(impl_encode == codec->encode);
    testrun(impl_decode == codec->decode);
    // testrun(impl_get_parameters == codec->impl_get_parameters);
    testrun(0 == codec_g711->last_seq_number);

    testrun(alaw_expand == codec_g711->expand);
    testrun(alaw_compress == codec_g711->compress);

    codec = codec->free(codec);

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

    const size_t NUM_SAMPLES = sizeof(alaw_encoded);

    testrun(0 == impl_encode(0, 0, 0, 0, 0));

    uint8_t *out = calloc(NUM_SAMPLES, sizeof(int8_t));

    size_t len_out = NUM_SAMPLES;
    size_t len_in = NUM_SAMPLES * sizeof(int16_t);

    uint8_t *in = (uint8_t *)alaw_decoded;

    testrun(0 == impl_encode(0, in, len_in, out, len_out));

    /* use a-law */

    const char *json_string =
        "{\"" CONFIG_KEY_G711_LAW "\":\"" CONFIG_KEY_G711_ALAW "\"}";
    ov_json_value *parameters = ov_json_decode(json_string);
    testrun(0 != parameters);

    ov_codec *codec = impl_codec_create(0, parameters);

    parameters = parameters->free(parameters);
    testrun(0 == parameters);

    testrun(alaw_compress == ((codec_g711 *)codec)->compress);

    testrun(0 == impl_encode(codec, in, len_in, out, 0));
    testrun(0 == impl_encode(codec, in, len_in, 0, len_out));
    testrun(0 == impl_encode(codec, in, 0, out, len_out));
    testrun(0 == impl_encode(codec, 0, len_in, out, len_out));
    testrun(0 == impl_encode(codec, in, len_in, out, len_out - 2));

    int32_t encoded_bytes = impl_encode(codec, in, len_in, out, len_out);
    testrun(NUM_SAMPLES == (size_t)encoded_bytes);

    testrun(0 == memcmp(alaw_encoded, out, len_out));

    len_in -= 2;

    encoded_bytes = impl_encode(codec, in, len_in, out, len_out);
    testrun(NUM_SAMPLES - 1 == (size_t)encoded_bytes);

    testrun(0 == memcmp(alaw_encoded, out, NUM_SAMPLES - 1));

    free(out);

    testrun(0 == impl_free(codec));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_decode() {

    const size_t NUM_SAMPLES = sizeof(alaw_encoded);

    testrun(0 == impl_decode(0, 0, 0, 0, 0, 0));

    uint8_t *out = calloc(NUM_SAMPLES, sizeof(int16_t));

    size_t len_out = NUM_SAMPLES * sizeof(int16_t);
    size_t len_in = NUM_SAMPLES;

    uint8_t *in = (uint8_t *)alaw_encoded;

    testrun(0 == impl_decode(0, 0, in, len_in, out, len_out));

    /* use a-law */

    const char *json_string =
        "{\"" CONFIG_KEY_G711_LAW "\":\"" CONFIG_KEY_G711_ALAW "\"}";

    ov_json_value *parameters = ov_json_decode(json_string);

    ov_codec *codec = impl_codec_create(0, parameters);
    parameters = parameters->free(parameters);

    testrun(alaw_expand == ((codec_g711 *)codec)->expand);

    testrun(0 == impl_decode(codec, 0, in, len_in, out, 0));
    testrun(0 == impl_decode(codec, 0, in, len_in, 0, len_out));
    testrun(0 == impl_decode(codec, 0, in, 0, out, len_out));
    testrun(0 == impl_decode(codec, 0, 0, len_in, out, len_out));
    testrun(0 == impl_decode(codec, 0, in, len_in, out, len_out - 2));

    int32_t decoded_bytes = impl_decode(codec, 0, in, len_in, out, len_out);
    testrun(NUM_SAMPLES * sizeof(int16_t) == (size_t)decoded_bytes);

    testrun(0 == memcmp(alaw_decoded, out, len_out));

    len_in -= 2;

    decoded_bytes = impl_decode(codec, 0, in, len_in, out, len_out);
    testrun((NUM_SAMPLES - 2) * sizeof(int16_t) == (size_t)decoded_bytes);

    testrun(0 == memcmp(alaw_decoded, out, decoded_bytes));

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

    testrun(1 == ov_json_object_count(json));

    char const *law =
        ov_json_string_get(ov_json_get(json, "/" CONFIG_KEY_G711_LAW));

    testrun(0 == strcmp(CONFIG_KEY_G711_ULAW, law));
    law = 0;

    json = json->free(json);
    codec->free(codec);

    char const *json_string =
        "{\"" CONFIG_KEY_G711_LAW "\":\"" CONFIG_KEY_G711_ALAW "\"}";
    json = ov_json_decode(json_string);

    codec = impl_codec_create(0, json);
    json = json->free(json);

    json = impl_get_parameters(codec);

    testrun(0 != json);

    testrun(1 == ov_json_object_count(json));

    law = ov_json_string_get(ov_json_object_get(json, CONFIG_KEY_G711_LAW));

    testrun(0 == strcmp(CONFIG_KEY_G711_ALAW, law));

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

    /* G.711 is ONLY specified for 8kHz */
    testrun(8000 == impl_get_samplerate_hertz(codec));

    codec = ov_codec_free(codec);
    testrun(0 == codec);

    json = ov_json_object();
    ov_codec_parameters_set_sample_rate_hertz(json, samplerate_hz);

    codec = impl_codec_create(0, json);
    testrun(0 != codec);

    json = ov_json_value_free(json);
    testrun(0 == json);

    testrun(8000 == impl_get_samplerate_hertz(codec));

    codec = ov_codec_free(codec);
    testrun(0 == codec);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_alaw_expand() {

    for (size_t i = 0; i < sizeof(alaw_encoded) / sizeof(alaw_encoded[0]);
         ++i) {

        testrun(alaw_decoded[i] == alaw_expand(alaw_encoded[i]));
    }

    const size_t num_samples =
        sizeof(alaw_reference_encoded) / sizeof(alaw_reference_encoded[0]);

    for (size_t i = 0; i < num_samples; ++i) {

        testrun(alaw_reference_decoded[i] ==
                alaw_expand(alaw_reference_encoded[i]));
    }

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_alaw_compress() {

    const size_t num_samples = sizeof(alaw_decoded) / sizeof(alaw_decoded[0]);

    for (size_t i = 0; i < num_samples; ++i) {

        testrun(alaw_encoded[i] == alaw_compress(alaw_decoded[i]));
    }

    const size_t num_samples_ref =
        sizeof(alaw_reference_encoded) / sizeof(alaw_reference_encoded[0]);

    for (size_t i = 0; i < num_samples_ref; ++i) {

        testrun(alaw_reference_encoded[i] ==
                alaw_compress(alaw_reference_decoded[i]));
    }

    /* Have all possible values run through compressor */
    const int16_t upper_limit_decoded = alaw_decoded[0];
    const int16_t lower_limit_decoded = alaw_decoded[num_samples - 1];

    const int16_t upper_limit_encoded = alaw_encoded[0];
    const int16_t lower_limit_encoded = alaw_encoded[num_samples - 1];

    for (int16_t i = INT16_MIN; i < INT16_MAX; ++i) {

        uint8_t e = alaw_compress(i);

        /* Check whether cut appears if out of bounds */

        if (upper_limit_decoded <= i)
            testrun(upper_limit_encoded == e);
        if (lower_limit_decoded >= i)
            testrun(lower_limit_encoded == e);
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ulaw_expand() {

    for (size_t i = 0; i < sizeof(ulaw_encoded) / sizeof(ulaw_encoded[0]);
         ++i) {

        testrun(ulaw_decoded[i] == ulaw_expand(ulaw_encoded[i]));
    }

    const size_t num_samples =
        sizeof(ulaw_reference_encoded) / sizeof(ulaw_reference_encoded[0]);

    for (size_t i = 0; i < num_samples; ++i) {

        testrun(ulaw_reference_decoded[i] ==
                ulaw_expand(ulaw_reference_encoded[i]));
    }

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_ulaw_compress() {

    const size_t num_samples = sizeof(ulaw_decoded) / sizeof(ulaw_decoded[0]);

    for (size_t i = 0; i < num_samples; ++i) {

        testrun(ulaw_encoded[i] == ulaw_compress(ulaw_decoded[i]));
    }

    const size_t num_samples_ref =
        sizeof(ulaw_reference_encoded) / sizeof(ulaw_reference_encoded[0]);

    for (size_t i = 0; i < num_samples_ref; ++i) {

        testrun(ulaw_reference_encoded[i] ==
                ulaw_compress(ulaw_reference_decoded[i]));
    }

    /* Have all possible values run through compressor */
    const int16_t upper_limit_decoded = ulaw_decoded[0];
    const int16_t lower_limit_decoded = ulaw_decoded[num_samples - 1];

    const int16_t upper_limit_encoded = ulaw_encoded[0];
    const int16_t lower_limit_encoded = ulaw_encoded[num_samples - 1];

    for (int16_t i = INT16_MIN; i < INT16_MAX; ++i) {

        uint8_t e = ulaw_compress(i);

        /* Check whether cut appears if out of bounds */

        if (upper_limit_decoded <= i)
            testrun(upper_limit_encoded == e);
        if (lower_limit_decoded >= i)
            testrun(lower_limit_encoded == e);
    }

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

void init_tests() {

    for (size_t i = 0; i < sizeof(alaw_encoded) / sizeof(alaw_encoded[0]);
         ++i) {

        alaw_encoded[i] = INVERT_EVEN_BITS(alaw_encoded[i]);
    }
}

int all_tests() {

    testrun_init();

    init_tests();

    testrun_test(test_ov_codec_g711_id);
    testrun_test(test_impl_codec_create);
    testrun_test(test_impl_free);
    testrun_test(test_impl_encode);
    testrun_test(test_impl_decode);
    testrun_test(test_impl_get_parameters);
    testrun_test(test_impl_get_samplerate_hertz);
    testrun_test(test_alaw_expand);
    testrun_test(test_alaw_compress);
    testrun_test(test_ulaw_expand);
    testrun_test(test_ulaw_compress);

    return testrun_counter;
}

/*----------------------------------------------------------------------------*/

testrun_run(all_tests);
