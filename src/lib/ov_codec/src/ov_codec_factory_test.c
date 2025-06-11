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

     \file               ov_codec_factory_tests.c
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2017-12-06

     \ingroup            empty

     \brief              empty

 **/

#include "ov_codec_factory.c"

#include <ov_base/ov_utils.h>
#include <ov_test/ov_test.h>

/******************************************************************************
 *                               Other helpers
 ******************************************************************************/

/*
 * This functio nis here only to have a neat way for inspecting
 * factories in gdb...
 */
char *factory_to_string(const ov_codec_factory *factory) {

    if (0 == factory) {

        return strdup("NULL");
    }

    char s1[4096] = {0};

    const size_t BUFSIZE = sizeof(s1);

    snprintf(s1, BUFSIZE, "{ ");

    codec_entry *e = (codec_entry *)factory;

    while (e->next) {

        strncat(s1, " ", BUFSIZE);
        strncat(s1, e->next->codec_name, BUFSIZE);

        e = e->next;
    }

    strncat(s1, "}", BUFSIZE);
    s1[sizeof(s1) - 1] = 0;

    return strdup(s1);
}

/*****************************************************************************
                                  Codec Mockup
 ****************************************************************************/

#define MOCKUP_MAGIC_NUMBER 0x12faaf21

/*----------------------------------------------------------------------------*/

struct mockup_codec {

    ov_codec public;

    ov_json_value *parameters;
};

/*----------------------------------------------------------------------------*/

static const char *mockup_type_id() { return "mockup_1"; }

/*----------------------------------------------------------------------------*/

static ov_codec *mockup_free(ov_codec *codec) {

    if (0 == codec) {
        return codec;
    }

    struct mockup_codec *mc = (struct mockup_codec *)codec;

    mc->parameters = ov_json_value_free(mc->parameters);
    TEST_ASSERT(0 == mc->parameters);

    free(codec);

    return 0;
}

/*----------------------------------------------------------------------------*/

static int32_t mockup_encode(ov_codec *codec,
                             const uint8_t *input,
                             size_t length,
                             uint8_t *output,
                             size_t max_out_length) {

    TEST_ASSERT(0 != codec);
    TEST_ASSERT(0 != input);
    TEST_ASSERT(0 != output);

    size_t l = (length > max_out_length) ? max_out_length : length;

    memcpy(output, input, l);

    return l / sizeof(int16_t);
}

/*----------------------------------------------------------------------------*/

static int32_t mockup_decode(ov_codec *codec,
                             uint64_t seq_number,
                             const uint8_t *input,
                             size_t length,
                             uint8_t *output,
                             size_t max_out_length) {

    UNUSED(seq_number);

    TEST_ASSERT(0 != codec);
    TEST_ASSERT(0 != input);
    TEST_ASSERT(0 != output);

    size_t l = (length > max_out_length) ? max_out_length : length;

    memcpy(output, input, l);

    return l / sizeof(int16_t);
}

/*----------------------------------------------------------------------------*/

static ov_json_value *mockup_parameters(const ov_codec *codec) {

    TEST_ASSERT(0 != codec);

    struct mockup_codec *mc = (struct mockup_codec *)codec;

    return mc->parameters;
}

/*----------------------------------------------------------------------------*/

static uint32_t mockup_samplerate_hertz(const ov_codec *codec) {

    TEST_ASSERT(0 != codec);

    struct mockup_codec *mc = (struct mockup_codec *)codec;

    uint32_t sr = ov_json_number_get(
        ov_json_get(mc->parameters, "/" OV_KEY_SAMPLE_RATE_HERTZ));

    if (0 == sr) {
        return OV_DEFAULT_SAMPLERATE;
    }

    return sr;
}

/*----------------------------------------------------------------------------*/

ov_codec *mockup_codec_create(uint32_t ssid, const ov_json_value *parameters) {

    UNUSED(ssid);

    if (0 == parameters) return 0;

    struct mockup_codec *mc = calloc(1, sizeof(struct mockup_codec));

    ov_json_value_copy((void **)&mc->parameters, parameters);
    TEST_ASSERT(0 != mc->parameters);

    ov_codec *c = &mc->public;
    c->type_id = mockup_type_id;
    c->type = MOCKUP_MAGIC_NUMBER;

    c->decode = mockup_decode;
    c->encode = mockup_encode;

    c->free = mockup_free;

    c->get_parameters = mockup_parameters;
    c->get_samplerate_hertz = mockup_samplerate_hertz;

    return &mc->public;
}

/*****************************************************************************
                                    MOCKUP 2
 ****************************************************************************/

#define MOCKUP_2_MAGIC_NUMBER 0x12345678

static char const *mockup_2_type_id() { return "mockup_2"; }

/*----------------------------------------------------------------------------*/

ov_codec *mockup_2_codec_create(uint32_t ssid,
                                const ov_json_value *parameters) {

    if (0 == parameters) return 0;

    ov_codec *c = mockup_codec_create(ssid, parameters);
    TEST_ASSERT(0 != c);

    c->type = MOCKUP_2_MAGIC_NUMBER;
    c->type_id = mockup_2_type_id;

    return c;
}

/*****************************************************************************
                                    MOCKUP 3
 ****************************************************************************/

#define MOCKUP_3_MAGIC_NUMBER 0x87654321

static char const *mockup_3_type_id() { return "mockup_3"; }

/*----------------------------------------------------------------------------*/

ov_codec *mockup_3_codec_create(uint32_t ssid,
                                const ov_json_value *parameters) {

    if (0 == parameters) return 0;

    ov_codec *c = mockup_codec_create(ssid, parameters);
    TEST_ASSERT(0 != c);

    c->type = MOCKUP_3_MAGIC_NUMBER;
    c->type_id = mockup_3_type_id;

    return c;
}

/*****************************************************************************
                                JSON TEST CODEC
 ****************************************************************************/

struct json_test_codec {

    ov_codec codec;
    ov_json_value *json;
};

ov_json_value *json_test_codec_get_parameters(const ov_codec *codec) {

    struct json_test_codec *jtc = (struct json_test_codec *)codec;

    char const *json_string = ov_json_encode(jtc->json);
    ov_json_value *json = ov_json_decode(json_string);

    return json;
}

/*----------------------------------------------------------------------------*/

ov_codec *json_test_codec_free(ov_codec *codec) {

    if (0 == codec) return 0;

    struct json_test_codec *jtc = (struct json_test_codec *)codec;

    if (jtc->json) jtc->json = jtc->json->free(jtc->json);

    free(codec);

    return 0;
}

/*----------------------------------------------------------------------------*/

ov_codec *mockup_codec_create_json(uint32_t ssid,
                                   const ov_json_value *parameters) {

    UNUSED(ssid);

    struct json_test_codec *jtc = calloc(1, sizeof(struct json_test_codec));

    ov_codec *codec = (ov_codec *)jtc;

    codec->type = 0xff00ee11;
    codec->get_parameters = json_test_codec_get_parameters;
    codec->free = json_test_codec_free;

    char const *json_string = ov_json_encode(parameters);
    jtc->json = ov_json_decode(json_string);
    json_string = 0;

    return codec;
}

/******************************************************************************
 *  TESTS
 ******************************************************************************/

int test_ov_codec_factory_create() {

    ov_codec_factory *factory = ov_codec_factory_create();
    testrun(0 != factory);

    ov_codec_factory_free(factory);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_codec_factory_create_standard() {

    ov_codec_factory *factory = ov_codec_factory_create_standard();
    testrun(0 != factory);

    ov_codec *codec =
        ov_codec_factory_get_codec(factory, ov_codec_raw_id(), 0, 0);

    testrun(0 == strncmp(ov_codec_raw_id(), (char *)codec->type_id(codec), 4));

    codec = ov_codec_free(codec);

    codec =
        ov_codec_factory_get_codec(factory, ov_codec_pcm16_signed_id(), 0, 0);
    testrun(0 != codec);

    codec = ov_codec_free(codec);

    codec = ov_codec_factory_get_codec(factory, ov_codec_opus_id(), 0, 0);
    testrun(0 != codec);

    codec = ov_codec_free(codec);

    codec = ov_codec_factory_get_codec(factory, ov_codec_g711_id(), 0, 0);
    testrun(0 != codec);

    testrun(0 == strncmp(ov_codec_g711_id(), codec->type_id(codec), 4));

    testrun(0 != codec->resampling);

    codec = ov_codec_free(codec);
    ov_codec_factory_free(factory);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_codec_factory_free() {

    testrun(0 == ov_codec_factory_free(0));

    ov_codec_factory *factory = ov_codec_factory_create();
    testrun(0 != factory);

    testrun(0 == ov_codec_factory_free(factory));

    factory = ov_codec_factory_create();
    testrun(0 != factory);

    /* TODO: Test using non-empty factories */

    testrun(0 == ov_codec_factory_free(factory));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_find_entry() {

    /* Thats an internal function, therefore we use internals for testing
     * this function ...*/

    const char *KEY = "TEST KEY";
    const char *KEY_2 = "TEST KEY 2";
    const char *KEY_3 = "TEST KEY 3";

    testrun(0 == find_entry(0, KEY));

    codec_entry *list = calloc(1, sizeof(codec_entry));

    testrun(0 == find_entry(0, KEY));

    codec_entry *e = calloc(1, sizeof(codec_entry));
    e->codec_name = strdup(KEY);
    list->next = e;

    testrun(list == find_entry(list, KEY));
    testrun(0 == find_entry(list, KEY_2));

    e = calloc(1, sizeof(codec_entry));
    e->codec_name = strdup(KEY_2);
    e->next = list->next;
    list->next = e;

    testrun(list->next == find_entry(list, KEY));
    testrun(list == find_entry(list, KEY_2));
    testrun(0 == find_entry(list, KEY_3));

    /* different search order ... */

    testrun(list == find_entry(list, KEY_2));
    testrun(0 == find_entry(list, KEY_3));
    testrun(list->next == find_entry(list, KEY));

    free(list->next->codec_name);
    list->next->codec_name = strdup(KEY_3);

    testrun(list->next == find_entry(list, KEY));
    testrun(0 == find_entry(list, KEY_2));
    testrun(list == find_entry(list, KEY_3));

    /* different search order ... */

    testrun(list == find_entry(list, KEY_3));
    testrun(0 == find_entry(list, KEY_2));
    testrun(list->next == find_entry(list, KEY));

    e = calloc(1, sizeof(codec_entry));
    e->codec_name = strdup(KEY_2);
    e->next = list->next;
    list->next = e;

    testrun(list->next == find_entry(list, KEY_3));
    testrun(list == find_entry(list, KEY_2));
    testrun(list->next->next == find_entry(list, KEY));

    testrun(0 == ov_codec_factory_free(list));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_codec_factory_install_codec() {

    const char *STRING = "testName";
    const char *STRING_2 = "testName2";
    const char *STRING_3 = "testName3";

    ov_codec_factory *factory = ov_codec_factory_create();

    testrun(0 == ov_codec_factory_install_codec(0, 0, 0));
    testrun(0 == find_entry(g_factory, STRING));

    testrun(0 == ov_codec_factory_install_codec(factory, 0, 0));
    testrun(0 == find_entry(g_factory, STRING));

    testrun(0 == ov_codec_factory_install_codec(factory, STRING, 0));
    testrun(0 == find_entry(g_factory, STRING));

    testrun(0 == ov_codec_factory_install_codec(0, STRING, 0));
    testrun(0 == find_entry(g_factory, STRING));

    /* Test global factory */

    testrun(0 ==
            ov_codec_factory_install_codec(0, STRING, mockup_codec_create));

    codec_entry *entry = find_entry(g_factory, STRING);
    testrun(0 != entry);
    testrun(entry->next->get_codec_for == mockup_codec_create);
    entry = 0;

    testrun(mockup_codec_create ==
            ov_codec_factory_install_codec(0, STRING, mockup_codec_create));

    /* Test with dedicated factory */

    testrun(0 == ov_codec_factory_install_codec(
                     factory, STRING, mockup_codec_create));

    testrun(0 == find_entry(factory, STRING_2));
    testrun(0 == find_entry(factory, STRING_3));

    entry = find_entry(factory, STRING);
    testrun(entry);
    testrun(entry->next->get_codec_for == mockup_codec_create);

    testrun(mockup_codec_create == ov_codec_factory_install_codec(
                                       factory, STRING, mockup_codec_create));

    testrun(0 == find_entry(factory, STRING_2));
    testrun(0 == find_entry(factory, STRING_3));

    entry = find_entry(g_factory, STRING);
    testrun(entry);
    testrun(entry->next->get_codec_for == mockup_codec_create);

    /* install 2nd codec generator */

    testrun(0 == ov_codec_factory_install_codec(
                     factory, STRING_2, mockup_2_codec_create));

    entry = find_entry(factory, STRING);
    testrun(entry);
    testrun(entry->next->get_codec_for == mockup_codec_create);

    entry = find_entry(factory, STRING_2);
    testrun(entry);
    testrun(entry->next->get_codec_for == mockup_2_codec_create);

    testrun(0 == find_entry(factory, STRING_3));

    testrun(mockup_codec_create == ov_codec_factory_install_codec(
                                       factory, STRING, mockup_codec_create));
    /* Now, replace old codec creator by new one, return old one */
    testrun(
        mockup_2_codec_create ==
        ov_codec_factory_install_codec(factory, STRING_2, mockup_codec_create));

    entry = find_entry(factory, STRING);
    testrun(entry);
    testrun(entry->next->get_codec_for == mockup_codec_create);

    entry = find_entry(factory, STRING_2);
    testrun(entry);
    testrun(entry->next->get_codec_for == mockup_codec_create);

    testrun(0 == find_entry(factory, STRING_3));

    /* Add a third codec generator */

    testrun(0 == ov_codec_factory_install_codec(
                     factory, STRING_3, mockup_3_codec_create));

    entry = find_entry(factory, STRING);
    testrun(entry);
    testrun(entry->next->get_codec_for == mockup_codec_create);

    entry = find_entry(factory, STRING_2);
    testrun(entry);
    testrun(entry->next->get_codec_for == mockup_codec_create);

    entry = find_entry(factory, STRING_3);
    testrun(entry);
    testrun(entry->next->get_codec_for == mockup_3_codec_create);

    testrun(mockup_codec_create == ov_codec_factory_install_codec(
                                       factory, STRING, mockup_2_codec_create));
    testrun(mockup_codec_create == ov_codec_factory_install_codec(
                                       factory, STRING_2, mockup_codec_create));

    entry = find_entry(factory, STRING);
    testrun(entry);
    testrun(entry->next->get_codec_for == mockup_2_codec_create);

    entry = find_entry(factory, STRING_2);
    testrun(entry);
    testrun(entry->next->get_codec_for == mockup_codec_create);

    entry = find_entry(factory, STRING_3);
    testrun(entry);
    testrun(entry->next->get_codec_for == mockup_3_codec_create);

    testrun(0 == ov_codec_factory_free(factory));
    testrun(0 == ov_codec_factory_free(0));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_codec_factory_get_codec() {

    const char *CODEC_NAME_1 = "CODEC 1";
    const char *CODEC_NAME_2 = "CODEC 2";
    const char *CODEC_NAME_3 = "CODEC 3";

    testrun(0 == ov_codec_factory_get_codec(0, 0, 0, 0));

    ov_codec_factory *factory = ov_codec_factory_create();
    testrun(0 == ov_codec_factory_get_codec(factory, 0, 0, 0));
    testrun(0 == ov_codec_factory_get_codec(factory, CODEC_NAME_1, 0, 0));
    testrun(0 == ov_codec_factory_get_codec(factory, CODEC_NAME_1, 12, 0));

    testrun(0 == ov_codec_factory_install_codec(
                     factory, CODEC_NAME_1, mockup_codec_create));

    testrun(0 == ov_codec_factory_get_codec(factory, 0, 0, 0));
    testrun(0 == ov_codec_factory_get_codec(factory, CODEC_NAME_1, 0, 0));

    testrun(0 == ov_codec_factory_get_codec(factory, CODEC_NAME_1, 12, 0));
    testrun(0 == ov_codec_factory_get_codec(factory, CODEC_NAME_2, 12, 0));
    testrun(0 == ov_codec_factory_get_codec(factory, CODEC_NAME_3, 12, 0));

    ov_json_value *cparams = ov_json_object();
    TEST_ASSERT(0 != cparams);

    ov_codec *c =
        ov_codec_factory_get_codec(factory, CODEC_NAME_1, 12, cparams);
    cparams = ov_json_value_free(cparams);

    testrun(MOCKUP_MAGIC_NUMBER == c->type);
    testrun(OV_DEFAULT_SAMPLERATE == ov_codec_get_samplerate_hertz(c));
    testrun(0 == c->resampling);

    c = ov_codec_free(c);
    testrun(0 == c);

    /*-----------------------------------------------------------------------*/

    testrun(0 == ov_codec_factory_get_codec(factory, CODEC_NAME_2, 12, 0));
    testrun(0 == ov_codec_factory_get_codec(factory, CODEC_NAME_3, 12, 0));

    testrun(0 == ov_codec_factory_install_codec(
                     factory, CODEC_NAME_3, mockup_3_codec_create));

    cparams = ov_json_object();
    TEST_ASSERT(0 != cparams);

    c = ov_codec_factory_get_codec(factory, CODEC_NAME_3, 12, cparams);
    cparams = ov_json_value_free(cparams);

    testrun(MOCKUP_3_MAGIC_NUMBER == c->type);
    testrun(OV_DEFAULT_SAMPLERATE == ov_codec_get_samplerate_hertz(c));
    testrun(0 == c->resampling);

    c = ov_codec_free(c);
    testrun(0 == c);

    /*-----------------------------------------------------------------------*/

    testrun(0 == ov_codec_factory_get_codec(factory, CODEC_NAME_2, 12, 0));

    cparams = ov_json_object();
    TEST_ASSERT(0 != cparams);

    c = ov_codec_factory_get_codec(factory, CODEC_NAME_1, 12, cparams);
    cparams = ov_json_value_free(cparams);

    testrun(MOCKUP_MAGIC_NUMBER == c->type);
    testrun(OV_DEFAULT_SAMPLERATE == ov_codec_get_samplerate_hertz(c));
    testrun(0 == c->resampling);

    c = ov_codec_free(c);
    testrun(0 == c);

    /*------------------------------------------------------------------------*/
    /* Check with different samplerate - resampling enabled ? */

    testrun(0 == ov_codec_factory_get_codec(factory, CODEC_NAME_2, 12, 0));

    char const *cparams_16kHz_str = "{\"" OV_KEY_SAMPLE_RATE_HERTZ "\":16000}";

    cparams =
        ov_json_value_from_string(cparams_16kHz_str, strlen(cparams_16kHz_str));

    TEST_ASSERT(0 != cparams);

    c = ov_codec_factory_get_codec(factory, CODEC_NAME_1, 12, cparams);
    cparams = ov_json_value_free(cparams);

    testrun(MOCKUP_MAGIC_NUMBER == c->type);
    testrun(16000 == ov_codec_get_samplerate_hertz(c));
    testrun(0 != c->resampling);

    c = ov_codec_free(c);
    testrun(0 == c);

    /*-----------------------------------------------------------------------*/

    testrun(0 == ov_codec_factory_free(factory));

    /* Check with global factory */
    cparams =
        ov_json_value_from_string(cparams_16kHz_str, strlen(cparams_16kHz_str));
    TEST_ASSERT(0 != cparams);

    testrun(0 == ov_codec_factory_get_codec(0, CODEC_NAME_1, 12, cparams));

    testrun(0 == ov_codec_factory_install_codec(
                     0, CODEC_NAME_1, mockup_codec_create));

    c = ov_codec_factory_get_codec(0, CODEC_NAME_1, 12, cparams);
    cparams = ov_json_value_free(cparams);

    testrun(MOCKUP_MAGIC_NUMBER == c->type);
    testrun(16000 == ov_codec_get_samplerate_hertz(c));
    testrun(0 != c->resampling);

    c = ov_codec_free(c);
    testrun(0 == c);

    testrun(0 == ov_codec_factory_free(0));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_codec_factory_get_codec_from_json() {

    const char *CODEC_NAME = "JSON_CODEC";

    ov_codec_factory *factory = ov_codec_factory_create();

    testrun(0 == ov_codec_factory_install_codec(
                     factory, CODEC_NAME, mockup_codec_create_json));

    ov_json_value *json = ov_json_object();

    ov_json_value *codec_name = ov_json_string(CODEC_NAME);
    testrun(0 != codec_name);

    ov_json_object_set(json, OV_KEY_CODEC, codec_name);
    codec_name = 0;

    ov_json_value *additional_param = ov_json_string("naftagn");
    testrun(0 != additional_param);

    ov_json_object_set(json, "cthulhu", additional_param);
    additional_param = 0;

    ov_codec *codec = ov_codec_factory_get_codec_from_json(factory, json, 0);

    testrun(0 == ov_codec_factory_free(factory));
    json = json->free(json);

    testrun(0 != codec);

    json = codec->get_parameters(codec);
    testrun(0 != json);

    codec = ov_codec_free(codec);
    testrun(0 == codec);

    testrun(2 == ov_json_object_count(json));

    json = json->free(json);

    factory = ov_codec_factory_create_standard();
    testrun(0 != factory);

    json = ov_json_object();

    codec_name = ov_json_string(ov_codec_g711_id());
    testrun(0 != codec_name);

    ov_json_object_set(json, OV_KEY_CODEC, codec_name);
    codec_name = 0;

    additional_param = ov_json_string("alas_my_ass");
    testrun(0 != additional_param);

    ov_json_object_set(json, "fire_in_the_hole", additional_param);
    additional_param = 0;

    codec = ov_codec_factory_get_codec_from_json(factory, json, 0);
    testrun(0 != codec);

    testrun(0 == ov_codec_factory_free(factory));
    json = json->free(json);

    json = codec->get_parameters(codec);
    testrun(0 != json);

    testrun(1 == ov_json_object_count(json));

    json = json->free(json);
    testrun(0 == json);

    codec = ov_codec_free(codec);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_codec_factory_install_codec_from_so() {

    char const *invalid_so = OPENVOCS_ROOT "/build/lib/ov_base.so";

    ov_codec_factory *f = ov_codec_factory_create();

    ov_result res = {0};

    testrun(!ov_codec_factory_install_codec_from_so(0, 0, 0));
    testrun(!ov_codec_factory_install_codec_from_so(f, 0, 0));
    testrun(!ov_codec_factory_install_codec_from_so(0, invalid_so, 0));
    testrun(!ov_codec_factory_install_codec_from_so(f, invalid_so, 0));

    testrun(!ov_codec_factory_install_codec_from_so(0, 0, &res));

    testrun(res.error_code != OV_ERROR_NOERROR);
    ov_result_clear(&res);

    testrun(!ov_codec_factory_install_codec_from_so(f, 0, &res));

    testrun(res.error_code != OV_ERROR_NOERROR);
    ov_result_clear(&res);

    testrun(!ov_codec_factory_install_codec_from_so(0, invalid_so, &res));

    testrun(res.error_code != OV_ERROR_NOERROR);
    ov_result_clear(&res);

    testrun(!ov_codec_factory_install_codec_from_so(f, invalid_so, &res));

    testrun(res.error_code != OV_ERROR_NOERROR);
    ov_result_clear(&res);

    // Now try with proper SO
    // the raw codec provides the plugin hooks required to load.
    char const *raw_so = OPENVOCS_ROOT "/build/lib/libov_codec2.so";

    testrun(!ov_codec_factory_install_codec_from_so(0, 0, 0));
    testrun(!ov_codec_factory_install_codec_from_so(f, 0, 0));

    /*************************************************************************
                         Successful codec registration
     ************************************************************************/

    testrun(ov_codec_factory_install_codec_from_so(0, raw_so, 0));

    ov_json_value *params = ov_json_object();

    char const *test_codec = "test_codec";

    ov_codec *raw = ov_codec_factory_get_codec(0, test_codec, 132, params);

    testrun(0 != raw);

    raw = ov_codec_free(raw);

    /*************************************************************************
                                  Should fail
     ************************************************************************/

    testrun(!ov_codec_factory_install_codec_from_so(0, 0, &res));

    testrun(res.error_code != OV_ERROR_NOERROR);
    ov_result_clear(&res);

    testrun(!ov_codec_factory_install_codec_from_so(f, 0, &res));

    testrun(res.error_code != OV_ERROR_NOERROR);
    ov_result_clear(&res);

    /*************************************************************************
                Register already registered codec should succeed
     ************************************************************************/

    testrun(ov_codec_factory_install_codec_from_so(0, raw_so, &res));

    testrun(res.error_code == OV_ERROR_NOERROR);
    ov_result_clear(&res);

    // We should be able to create a codec for 'raw' still
    raw = ov_codec_factory_get_codec(0, test_codec, 132, params);
    testrun(0 != raw);

    raw = ov_codec_free(raw);

    /*************************************************************************
              Try actually retrieving codec from dedicated factory
     ************************************************************************/

    testrun(0 == ov_codec_factory_get_codec(f, test_codec, 132, params));

    testrun(ov_codec_factory_install_codec_from_so(f, raw_so, 0));

    raw = ov_codec_factory_get_codec(f, test_codec, 132, params);
    testrun(0 != raw);

    raw = ov_codec_free(raw);
    f = ov_codec_factory_free(f);

    // Load again with res provided

    f = ov_codec_factory_create();
    testrun(0 == ov_codec_factory_get_codec(f, test_codec, 132, params));

    testrun(0 == ov_codec_factory_get_codec(f, test_codec, 132, params));
    testrun(ov_codec_factory_install_codec_from_so(f, raw_so, &res));
    testrun(OV_ERROR_NOERROR == res.error_code);

    raw = ov_codec_factory_get_codec(f, test_codec, 132, params);
    testrun(0 != raw);

    raw = ov_codec_free(raw);
    f = ov_codec_factory_free(f);

    /*************************************************************************
     check wether the codec is properly installed into global factory again
     ************************************************************************/

    ov_codec_factory_free(0);

    testrun(0 == ov_codec_factory_get_codec(0, test_codec, 132, params));

    testrun(0 == ov_codec_factory_get_codec(0, test_codec, 132, params));
    testrun(ov_codec_factory_install_codec_from_so(0, raw_so, &res));
    testrun(OV_ERROR_NOERROR == res.error_code);

    raw = ov_codec_factory_get_codec(0, test_codec, 132, params);
    testrun(0 != raw);

    params = ov_json_value_free(params);
    raw = ov_codec_free(raw);
    ov_codec_factory_free(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_codec_factory_install_codec_from_so_dir() {

    testrun(0 > ov_codec_factory_install_codec_from_so_dir(0, 0, 0));

    ov_codec_factory *f = ov_codec_factory_create();

    testrun(0 > ov_codec_factory_install_codec_from_so_dir(f, 0, 0));

    ov_json_value *params = ov_json_object();
    char const *test_codec = "test_codec";
    char const *so_dir = OPENVOCS_ROOT "/build/lib/";

    testrun(0 == ov_codec_factory_get_codec(0, test_codec, 132, params));

    testrun(0 < ov_codec_factory_install_codec_from_so_dir(f, so_dir, 0));

    ov_codec *codec = ov_codec_factory_get_codec(f, test_codec, 132, params);

    testrun(0 != codec);

    codec = ov_codec_free(codec);
    params = ov_json_value_free(params);
    f = ov_codec_factory_free(f);

    // The global factory must always be destroyed since it might have been
    // created implicitly (as in here) by other calls
    ov_codec_factory_free(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_codec_factory",
            test_ov_codec_factory_create,
            test_ov_codec_factory_create_standard,
            test_ov_codec_factory_free,
            test_find_entry,
            test_ov_codec_factory_install_codec,
            test_ov_codec_factory_get_codec,
            test_ov_codec_factory_get_codec_from_json,
            test_ov_codec_factory_install_codec_from_so,
            test_ov_codec_factory_install_codec_from_so_dir);

/*----------------------------------------------------------------------------*/
