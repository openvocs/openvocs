/***
        ------------------------------------------------------------------------

        Copyright (c) 2018 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_stun_error_code_test.c
        @author         Markus Toepfer

        @date           2018-10-22

        @ingroup        ov_stun

        @brief          Unit tests of RFC 5389 attribute "error code"


        ------------------------------------------------------------------------
*/

#include "ov_stun_error_code.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_stun_attribute_frame_is_error_code() {

    size_t size = 1000;
    uint8_t buf[size];
    uint8_t *buffer = buf;

    memset(buf, 'a', size);

    // prepare valid frame
    testrun(ov_stun_attribute_set_type(buffer, size, STUN_ERROR_CODE));
    testrun(ov_stun_attribute_set_length(buffer, size, 1));

    testrun(ov_stun_attribute_frame_is_error_code(buffer, size));

    testrun(!ov_stun_attribute_frame_is_error_code(NULL, size));
    testrun(!ov_stun_attribute_frame_is_error_code(buffer, 0));
    testrun(!ov_stun_attribute_frame_is_error_code(buffer, 7));

    // min buffer min valid
    testrun(ov_stun_attribute_set_length(buffer, size, 1));
    testrun(ov_stun_attribute_frame_is_error_code(buffer, 9));

    // length < size
    testrun(ov_stun_attribute_set_length(buffer, size, 4));
    testrun(!ov_stun_attribute_frame_is_error_code(buffer, 11));
    testrun(ov_stun_attribute_frame_is_error_code(buffer, 12));

    // type not error_code
    testrun(ov_stun_attribute_set_type(buffer, size, 0));
    testrun(!ov_stun_attribute_frame_is_error_code(buffer, size));
    testrun(ov_stun_attribute_set_type(buffer, size, STUN_ERROR_CODE));
    testrun(ov_stun_attribute_frame_is_error_code(buffer, size));

    // check length
    for (size_t i = 1; i < size; i++) {

        testrun(ov_stun_attribute_set_type(buffer, size, STUN_ERROR_CODE));
        testrun(ov_stun_attribute_set_length(buffer, size, i));
        if (i <= 763) {
            testrun(ov_stun_attribute_frame_is_error_code(buffer, size));
        } else {
            testrun(!ov_stun_attribute_frame_is_error_code(buffer, size));
        }
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_error_code_encoding_length() {

    uint8_t *name = (uint8_t *)"name";

    testrun(0 == ov_stun_error_code_encoding_length(NULL, 0));
    testrun(0 == ov_stun_error_code_encoding_length(name, 0));
    testrun(0 == ov_stun_error_code_encoding_length(NULL, 10));

    size_t pad = 0;

    for (size_t i = 1; i < 1000; i++) {

        pad = i % 4;
        if (pad != 0) pad = 4 - pad;

        if (i <= 763) {
            testrun(8 + i + pad == ov_stun_error_code_encoding_length(name, i));
        } else {
            testrun(0 == ov_stun_error_code_encoding_length(name, i));
        }
    }

    return testrun_log_success();
}
/*----------------------------------------------------------------------------*/

int test_ov_stun_error_code_decode_code() {

    uint8_t buffer[100] = {0};
    uint8_t *attribute = buffer;

    testrun(0 == ov_stun_error_code_decode_code(NULL, 0));
    testrun(0 == ov_stun_error_code_decode_code(attribute, 0));
    testrun(0 == ov_stun_error_code_decode_code(NULL, 100));

    for (size_t i = 1; i < 100; i++) {

        if (i < 10) {

            buffer[6] = i;
            buffer[7] = 0;
            testrun(i * 100 == ov_stun_error_code_decode_code(attribute, 100));

        } else {

            buffer[6] = i;
            buffer[7] = 0;
            testrun((i & 0x0F) * 100 ==
                    ov_stun_error_code_decode_code(attribute, 100));
        }

        buffer[6] = 1;
        buffer[7] = i;
        testrun(i + 100 == ov_stun_error_code_decode_code(attribute, 100));
    }

    return testrun_log_success();
}
/*----------------------------------------------------------------------------*/

int test_ov_stun_error_code_decode_phrase() {

    uint8_t buffer[100] = {0};
    uint8_t *attribute = buffer;

    uint8_t *phrase = NULL;
    size_t plength = 0;
    size_t length = 10;

    testrun(ov_stun_attribute_set_type(attribute, 100, STUN_ERROR_CODE));
    testrun(ov_stun_attribute_set_length(attribute, 100, length));

    testrun(!ov_stun_error_code_decode_phrase(NULL, 0, NULL, 0));
    testrun(!ov_stun_error_code_decode_phrase(NULL, 100, &phrase, &plength));
    testrun(!ov_stun_error_code_decode_phrase(attribute, 0, &phrase, &plength));
    testrun(!ov_stun_error_code_decode_phrase(attribute, 100, NULL, &plength));
    testrun(!ov_stun_error_code_decode_phrase(attribute, 100, &phrase, 0));

    testrun(
        ov_stun_error_code_decode_phrase(attribute, 100, &phrase, &plength));
    testrun(phrase == attribute + 8);
    testrun(plength == length - 4);

    length = 33;
    testrun(ov_stun_attribute_set_length(attribute, 100, length));
    testrun(
        ov_stun_error_code_decode_phrase(attribute, 100, &phrase, &plength));
    testrun(phrase == attribute + 8);
    testrun(plength == length - 4);

    // type not correct
    testrun(ov_stun_attribute_set_type(attribute, 100, 0));
    testrun(
        !ov_stun_error_code_decode_phrase(attribute, 100, &phrase, &plength));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_error_code_encode() {

    size_t size = 100;
    uint8_t buffer[size];
    memset(buffer, 0, size);

    uint8_t *next = NULL;
    uint8_t *attribute = buffer;
    uint8_t *phrase = (uint8_t *)"test";
    size_t plength = 4;

    testrun(!ov_stun_error_code_encode(NULL, 0, NULL, 0, NULL, 0));

    testrun(!ov_stun_error_code_encode(NULL, size, NULL, 0, phrase, plength));
    testrun(!ov_stun_error_code_encode(attribute, 0, NULL, 0, phrase, plength));
    testrun(
        !ov_stun_error_code_encode(attribute, size, NULL, 0, NULL, plength));
    testrun(!ov_stun_error_code_encode(attribute, size, NULL, 0, phrase, 0));
    testrun(
        !ov_stun_error_code_encode(attribute, size, NULL, 0, phrase, plength));
    testrun(!ov_stun_error_code_encode(
        attribute, size, NULL, 299, phrase, plength));
    testrun(!ov_stun_error_code_encode(
        attribute, size, NULL, 700, phrase, plength));

    testrun(
        ov_stun_error_code_encode(attribute, size, NULL, 300, phrase, plength));
    testrun(STUN_ERROR_CODE == ov_stun_attribute_get_type(attribute, size));
    testrun((int64_t)plength + 4 ==
            ov_stun_attribute_get_length(attribute, size));
    testrun(300 == ov_stun_error_code_decode_code(attribute, size));

    memset(buffer, 0, size);
    plength = 3;
    testrun(ov_stun_error_code_encode(
        attribute, size, &next, 699, phrase, plength));
    testrun(STUN_ERROR_CODE == ov_stun_attribute_get_type(attribute, size));
    testrun(next == attribute + 4 + 8); // padded to 4 byte border
    testrun(699 == ov_stun_error_code_decode_code(attribute, size));
    testrun((int64_t)plength + 4 ==
            ov_stun_attribute_get_length(attribute, size));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_error_code_set_try_alternate() {

    size_t size = 100;
    uint8_t buffer[size];
    memset(buffer, 0, size);

    uint8_t *next = NULL;
    char *expect = "try alternate";
    uint8_t *attribute = buffer;

    size_t pad = 0;
    pad = strlen(expect) % 4;
    if (pad != 0) pad = 4 - pad;

    testrun(!ov_stun_error_code_set_try_alternate(NULL, 0, NULL));
    testrun(!ov_stun_error_code_set_try_alternate(attribute, 0, NULL));
    testrun(!ov_stun_error_code_set_try_alternate(NULL, size, NULL));

    // size to small
    testrun(
        !ov_stun_error_code_set_try_alternate(attribute, strlen(expect), NULL));

    testrun(ov_stun_error_code_set_try_alternate(attribute, size, NULL));
    testrun(STUN_ERROR_CODE == ov_stun_attribute_get_type(attribute, size));
    testrun(strlen(expect) + 4 ==
            (size_t)ov_stun_attribute_get_length(attribute, size));
    testrun(300 == ov_stun_error_code_decode_code(attribute, size));
    testrun(0 == strncmp((char *)attribute + 8, expect, strlen(expect)));

    // override
    testrun(ov_stun_error_code_set_try_alternate(attribute, size, &next));
    testrun(STUN_ERROR_CODE == ov_stun_attribute_get_type(attribute, size));
    testrun(strlen(expect) + 4 ==
            (size_t)ov_stun_attribute_get_length(attribute, size));
    testrun(300 == ov_stun_error_code_decode_code(attribute, size));
    testrun(0 == strncmp((char *)attribute + 8, expect, strlen(expect)));
    testrun(next == attribute + strlen(expect) + 8 + pad);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_error_code_set_bad_request() {

    size_t size = 100;
    uint8_t buffer[size];
    memset(buffer, 0, size);

    uint8_t *next = NULL;
    char *expect = "bad request";
    uint8_t *attribute = buffer;

    size_t pad = 0;
    pad = strlen(expect) % 4;
    if (pad != 0) pad = 4 - pad;

    testrun(!ov_stun_error_code_set_bad_request(NULL, 0, NULL));
    testrun(!ov_stun_error_code_set_bad_request(attribute, 0, NULL));
    testrun(!ov_stun_error_code_set_bad_request(NULL, size, NULL));

    // size to small
    testrun(
        !ov_stun_error_code_set_bad_request(attribute, strlen(expect), NULL));

    testrun(ov_stun_error_code_set_bad_request(attribute, size, NULL));
    testrun(STUN_ERROR_CODE == ov_stun_attribute_get_type(attribute, size));
    testrun(strlen(expect) + 4 ==
            (size_t)ov_stun_attribute_get_length(attribute, size));
    testrun(400 == ov_stun_error_code_decode_code(attribute, size));
    testrun(0 == strncmp((char *)attribute + 8, expect, strlen(expect)));

    // override
    testrun(ov_stun_error_code_set_bad_request(attribute, size, &next));
    testrun(STUN_ERROR_CODE == ov_stun_attribute_get_type(attribute, size));
    testrun(strlen(expect) + 4 ==
            (size_t)ov_stun_attribute_get_length(attribute, size));
    testrun(400 == ov_stun_error_code_decode_code(attribute, size));
    testrun(0 == strncmp((char *)attribute + 8, expect, strlen(expect)));
    testrun(next == attribute + strlen(expect) + 8 + pad);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_error_code_set_unauthorized() {

    size_t size = 100;
    uint8_t buffer[size];
    memset(buffer, 0, size);

    uint8_t *next = NULL;
    char *expect = "unauthorized";
    uint8_t *attribute = buffer;

    size_t pad = 0;
    pad = strlen(expect) % 4;
    if (pad != 0) pad = 4 - pad;

    testrun(!ov_stun_error_code_set_unauthorized(NULL, 0, NULL));
    testrun(!ov_stun_error_code_set_unauthorized(attribute, 0, NULL));
    testrun(!ov_stun_error_code_set_unauthorized(NULL, size, NULL));

    // size to small
    testrun(
        !ov_stun_error_code_set_unauthorized(attribute, strlen(expect), NULL));

    testrun(ov_stun_error_code_set_unauthorized(attribute, size, NULL));
    testrun(STUN_ERROR_CODE == ov_stun_attribute_get_type(attribute, size));
    testrun(strlen(expect) + 4 ==
            (size_t)ov_stun_attribute_get_length(attribute, size));
    testrun(401 == ov_stun_error_code_decode_code(attribute, size));
    testrun(0 == strncmp((char *)attribute + 8, expect, strlen(expect)));

    // override
    testrun(ov_stun_error_code_set_unauthorized(attribute, size, &next));
    testrun(STUN_ERROR_CODE == ov_stun_attribute_get_type(attribute, size));
    testrun(strlen(expect) + 4 ==
            (size_t)ov_stun_attribute_get_length(attribute, size));
    testrun(401 == ov_stun_error_code_decode_code(attribute, size));
    testrun(0 == strncmp((char *)attribute + 8, expect, strlen(expect)));
    testrun(next == attribute + strlen(expect) + 8 + pad);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_error_code_set_unknown_attribute() {

    size_t size = 100;
    uint8_t buffer[size];
    memset(buffer, 0, size);

    uint8_t *next = NULL;
    char *expect = "unknown attribute";
    uint8_t *attribute = buffer;

    size_t pad = 0;
    pad = strlen(expect) % 4;
    if (pad != 0) pad = 4 - pad;

    testrun(!ov_stun_error_code_set_unknown_attribute(NULL, 0, NULL));
    testrun(!ov_stun_error_code_set_unknown_attribute(attribute, 0, NULL));
    testrun(!ov_stun_error_code_set_unknown_attribute(NULL, size, NULL));

    // size to small
    testrun(!ov_stun_error_code_set_unknown_attribute(
        attribute, strlen(expect), NULL));

    testrun(ov_stun_error_code_set_unknown_attribute(attribute, size, NULL));
    testrun(STUN_ERROR_CODE == ov_stun_attribute_get_type(attribute, size));
    testrun(strlen(expect) + 4 ==
            (size_t)ov_stun_attribute_get_length(attribute, size));
    testrun(420 == ov_stun_error_code_decode_code(attribute, size));
    testrun(0 == strncmp((char *)attribute + 8, expect, strlen(expect)));

    // override
    testrun(ov_stun_error_code_set_unknown_attribute(attribute, size, &next));
    testrun(STUN_ERROR_CODE == ov_stun_attribute_get_type(attribute, size));
    testrun(strlen(expect) + 4 ==
            (size_t)ov_stun_attribute_get_length(attribute, size));
    testrun(420 == ov_stun_error_code_decode_code(attribute, size));
    testrun(0 == strncmp((char *)attribute + 8, expect, strlen(expect)));
    testrun(next == attribute + strlen(expect) + 8 + pad);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_error_code_set_stale_nonce() {

    size_t size = 100;
    uint8_t buffer[size];
    memset(buffer, 0, size);

    uint8_t *next = NULL;
    char *expect = "stale nonce";
    uint8_t *attribute = buffer;

    size_t pad = 0;
    pad = strlen(expect) % 4;
    if (pad != 0) pad = 4 - pad;

    testrun(!ov_stun_error_code_set_stale_nonce(NULL, 0, NULL));
    testrun(!ov_stun_error_code_set_stale_nonce(attribute, 0, NULL));
    testrun(!ov_stun_error_code_set_stale_nonce(NULL, size, NULL));

    // size to small
    testrun(
        !ov_stun_error_code_set_stale_nonce(attribute, strlen(expect), NULL));

    testrun(ov_stun_error_code_set_stale_nonce(attribute, size, NULL));
    testrun(STUN_ERROR_CODE == ov_stun_attribute_get_type(attribute, size));
    testrun(strlen(expect) + 4 ==
            (size_t)ov_stun_attribute_get_length(attribute, size));
    testrun(438 == ov_stun_error_code_decode_code(attribute, size));
    testrun(0 == strncmp((char *)attribute + 8, expect, strlen(expect)));

    // override
    testrun(ov_stun_error_code_set_stale_nonce(attribute, size, &next));
    testrun(STUN_ERROR_CODE == ov_stun_attribute_get_type(attribute, size));
    testrun(strlen(expect) + 4 ==
            (size_t)ov_stun_attribute_get_length(attribute, size));
    testrun(438 == ov_stun_error_code_decode_code(attribute, size));
    testrun(0 == strncmp((char *)attribute + 8, expect, strlen(expect)));
    testrun(next == attribute + strlen(expect) + 8 + pad);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_error_code_set_server_error() {

    size_t size = 100;
    uint8_t buffer[size];
    memset(buffer, 0, size);

    uint8_t *next = NULL;
    char *expect = "server error";
    uint8_t *attribute = buffer;

    size_t pad = 0;
    pad = strlen(expect) % 4;
    if (pad != 0) pad = 4 - pad;

    testrun(!ov_stun_error_code_set_server_error(NULL, 0, NULL));
    testrun(!ov_stun_error_code_set_server_error(attribute, 0, NULL));
    testrun(!ov_stun_error_code_set_server_error(NULL, size, NULL));

    // size to small
    testrun(
        !ov_stun_error_code_set_server_error(attribute, strlen(expect), NULL));

    testrun(ov_stun_error_code_set_server_error(attribute, size, NULL));
    testrun(STUN_ERROR_CODE == ov_stun_attribute_get_type(attribute, size));
    testrun(strlen(expect) + 4 ==
            (size_t)ov_stun_attribute_get_length(attribute, size));
    testrun(500 == ov_stun_error_code_decode_code(attribute, size));
    testrun(0 == strncmp((char *)attribute + 8, expect, strlen(expect)));

    // override
    testrun(ov_stun_error_code_set_server_error(attribute, size, &next));
    testrun(STUN_ERROR_CODE == ov_stun_attribute_get_type(attribute, size));
    testrun(strlen(expect) + 4 ==
            (size_t)ov_stun_attribute_get_length(attribute, size));
    testrun(500 == ov_stun_error_code_decode_code(attribute, size));
    testrun(0 == strncmp((char *)attribute + 8, expect, strlen(expect)));
    testrun(next == attribute + strlen(expect) + 8 + pad);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_error_code_generate_response() {

    size_t size = 100;
    uint8_t frame[size];
    memset(frame, 0, size);

    uint8_t response[size];
    memset(response, 0, size);

    uint8_t transaction_id[12] = {0};
    uint8_t *id = transaction_id;

    char *expect = "server error";
    size_t pad = 0;
    pad = strlen(expect) % 4;
    if (pad != 0) pad = 4 - pad;
    size_t len = strlen(expect) + pad + 4 + 4;

    // prepare frame
    testrun(ov_stun_frame_generate_transaction_id(transaction_id));
    testrun(ov_stun_frame_set_transaction_id(frame, size, id));
    testrun(ov_stun_frame_set_magic_cookie(frame, size));
    testrun(ov_stun_frame_set_method(frame, size, 100));
    testrun(ov_stun_frame_set_length(frame, size, 4));
    testrun(ov_stun_frame_set_request(frame, size));

    testrun(!ov_stun_error_code_generate_response(NULL, 0, NULL, 0, NULL));
    testrun(!ov_stun_error_code_generate_response(
        NULL, 20, response, 20, ov_stun_error_code_set_server_error));
    testrun(!ov_stun_error_code_generate_response(
        frame, 0, NULL, 20, ov_stun_error_code_set_server_error));
    testrun(!ov_stun_error_code_generate_response(
        frame, 20, response, 0, ov_stun_error_code_set_server_error));
    testrun(
        !ov_stun_error_code_generate_response(frame, 20, response, 20, NULL));

    memset(response, 0, size);
    testrun(ov_stun_error_code_generate_response(
        frame, 20, response, size, ov_stun_error_code_set_server_error));
    testrun(STUN_ERROR_CODE == ov_stun_attribute_get_type(response + 20, size));
    testrun(strlen(expect) + 4 ==
            (size_t)ov_stun_attribute_get_length(response + 20, size));
    testrun(ov_stun_frame_is_valid(response, len + 20));
    testrun(ov_stun_frame_has_magic_cookie(response, len + 20));
    testrun(100 == ov_stun_frame_get_method(response, len + 20));
    testrun(len == (size_t)ov_stun_frame_get_length(response, len + 20));
    testrun(ov_stun_frame_class_is_error_response(response, len + 20));
    testrun(0 == memcmp(id,
                        ov_stun_frame_get_transaction_id(response, len + 20),
                        12));

    memset(response, 0, size);
    expect = "stale nonce";
    pad = strlen(expect) % 4;
    if (pad != 0) pad = 4 - pad;
    len = strlen(expect) + pad + 4 + 4;
    testrun(ov_stun_error_code_generate_response(
        frame, 20, response, size, ov_stun_error_code_set_stale_nonce));
    testrun(STUN_ERROR_CODE == ov_stun_attribute_get_type(response + 20, size));
    testrun(strlen(expect) + 4 ==
            (size_t)ov_stun_attribute_get_length(response + 20, size));
    testrun(ov_stun_frame_is_valid(response, len + 20));
    testrun(ov_stun_frame_has_magic_cookie(response, len + 20));
    testrun(100 == ov_stun_frame_get_method(response, len + 20));
    testrun(len == (size_t)ov_stun_frame_get_length(response, len + 20));
    testrun(ov_stun_frame_class_is_error_response(response, len + 20));
    testrun(0 == memcmp(id,
                        ov_stun_frame_get_transaction_id(response, len + 20),
                        12));

    // response size to small
    memset(response, 0, size);
    testrun(!ov_stun_error_code_generate_response(
        frame, 20, response, len + 19, ov_stun_error_code_set_stale_nonce));
    // response length min
    testrun(ov_stun_error_code_generate_response(
        frame, 20, response, len + 20, ov_stun_error_code_set_stale_nonce));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

    testrun_init();
    testrun_test(test_ov_stun_attribute_frame_is_error_code);
    testrun_test(test_ov_stun_error_code_encoding_length);

    testrun_test(test_ov_stun_error_code_decode_code);
    testrun_test(test_ov_stun_error_code_decode_phrase);
    testrun_test(test_ov_stun_error_code_encode);

    testrun_test(test_ov_stun_error_code_set_try_alternate);
    testrun_test(test_ov_stun_error_code_set_bad_request);
    testrun_test(test_ov_stun_error_code_set_unauthorized);
    testrun_test(test_ov_stun_error_code_set_unknown_attribute);
    testrun_test(test_ov_stun_error_code_set_stale_nonce);
    testrun_test(test_ov_stun_error_code_set_server_error);

    testrun_test(test_ov_stun_error_code_generate_response);

    return testrun_counter;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST EXECUTION                                                  #EXEC
 *
 *      ------------------------------------------------------------------------
 */

testrun_run(all_tests);
