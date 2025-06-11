/***
        ------------------------------------------------------------------------

        Copyright 2018 German Aerospace Center DLR e.V. (GSOC)

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

        ------------------------------------------------------------------------
*//**
        @file           ov_base64_test.c
        @author         Markus Toepfer

        @date           2018-04-12

        @ingroup        ov_basics

        @brief          Unit Tests for base64 encoding.


        ------------------------------------------------------------------------
*/

#include "ov_base64.c"
#include <ov_test/testrun.h>

static bool testing = true;

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_base64_alphabetPosition() {

    uint8_t byte = 0;
    uint8_t alphabet[64] = {};

    testrun(ov_base64_alphabetPosition(byte, alphabet) == -1);

    alphabet[0] = 1;
    byte = 1;
    testrun(ov_base64_alphabetPosition(byte, alphabet) == 0);
    byte = 2;
    testrun(ov_base64_alphabetPosition(byte, alphabet) == -1);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_base64_decode_with_alphabet() {

    uint8_t alphabet[64] = {};
    uint8_t *source = NULL;

    size_t size = 100;
    uint8_t buffer[size];
    uint8_t *ptr = buffer;
    memset(buffer, 0, size);

    testrun(!ov_base64_decode_with_alphabet(NULL, 1, &ptr, &size, alphabet));
    testrun(!ov_base64_decode_with_alphabet(source, 1, &ptr, &size, NULL));
    testrun(!ov_base64_decode_with_alphabet(source, 0, &ptr, &size, alphabet));

    source = (uint8_t *)"NjM_";
    testrun(!ov_base64_decode_with_alphabet(source, 1, &ptr, &size, alphabet));
    testrun(!ov_base64_decode_with_alphabet(source, 2, &ptr, &size, alphabet));
    testrun(!ov_base64_decode_with_alphabet(source, 3, &ptr, &size, alphabet));

    size = 100;
    testrun(!ov_base64_decode_with_alphabet(source, 4, &ptr, &size, alphabet));

    size = 100;
    testrun(!ov_base64_decode_with_alphabet(
        source, 4, &ptr, &size, base64Alphabet));

    size = 100;
    testrun(ov_base64_decode_with_alphabet(
        source, 4, &ptr, &size, base64urlAlphabet));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_base64_encode_with_alphabet() {

    uint8_t alphabet[64] = {};
    uint8_t *source = NULL;

    size_t size = 100;
    uint8_t buffer[size];
    uint8_t *ptr = buffer;
    memset(buffer, 0, size);

    testrun(!ov_base64_encode_with_alphabet(NULL, 1, &ptr, &size, alphabet));
    testrun(!ov_base64_encode_with_alphabet(source, 1, &ptr, &size, NULL));
    testrun(!ov_base64_encode_with_alphabet(source, 0, &ptr, &size, alphabet));

    source = (uint8_t *)"63?";

    size = 100;
    testrun(
        ov_base64_encode_with_alphabet(source, 4, &ptr, &size, base64Alphabet));

    size = 100;
    testrun(ov_base64_encode_with_alphabet(
        source, 4, &ptr, &size, base64urlAlphabet));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_base64_encode() {

    char *rfcTest[] = {"", "f", "fo", "foo", "foob", "fooba", "foobar"};

    char *rfcResult[] = {
        "", "Zg==", "Zm8=", "Zm9v", "Zm9vYg==", "Zm9vYmE=", "Zm9vYmFy"};

    uint8_t *result = NULL;
    size_t rsize = 0;
    size_t bsize = 100;
    uint8_t buffer[bsize];
    uint8_t *ptr = buffer;
    memset(buffer, 0, bsize);

    testrun(!ov_base64_encode(NULL, 0, NULL, 0));
    testrun(!ov_base64_encode((uint8_t *)"test", 4, &result, 0));
    testrun(!ov_base64_encode((uint8_t *)"test", 4, NULL, &rsize));
    testrun(!ov_base64_encode((uint8_t *)"test", 0, &result, &rsize));
    testrun(!ov_base64_encode(NULL, 4, &result, &rsize));

    testrun(ov_base64_encode((uint8_t *)"test", 4, &result, &rsize));
    testrun(result);
    testrun(0 == strncmp("dGVzdA==", (char *)result, rsize));
    free(result);

    // check buffer fill
    testrun(ov_base64_encode((uint8_t *)"test", 4, &ptr, &bsize));
    testrun(0 == strncmp("dGVzdA==", (char *)buffer, bsize));
    testrun(8 == bsize);

    int i;
    for (i = 0; i < 7; i++) {

        bsize = 100;

        if (i == 0) {
            testrun(!ov_base64_encode(
                (uint8_t *)rfcTest[i], strlen(rfcTest[i]), &ptr, &bsize));
        } else {
            testrun(ov_base64_encode(
                (uint8_t *)rfcTest[i], strlen(rfcTest[i]), &ptr, &bsize));

            testrun(0 == strncmp((char *)buffer,
                                 rfcResult[i],
                                 strlen(rfcResult[i])));
            testrun(bsize == strlen(rfcResult[i]));
        }
    }

    char *check = NULL;

    // check 1st value of the alphabet
    check = "00A";
    bsize = 100;
    testrun(ov_base64_encode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(4 == bsize);
    testrun(0 == strncmp("MDBB", (char *)buffer, bsize));

    // check last 4 values of the alphabet
    check = "60<";
    bsize = 100;
    testrun(ov_base64_encode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(4 == bsize);
    testrun(0 == strncmp("NjA8", (char *)buffer, bsize));

    check = "61=";
    bsize = 100;
    testrun(ov_base64_encode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(4 == bsize);
    testrun(0 == strncmp("NjE9", (char *)buffer, bsize));

    check = "62>";
    bsize = 100;
    testrun(ov_base64_encode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(4 == bsize);
    testrun(0 == strncmp("NjI+", (char *)buffer, bsize));

    check = "63?";
    bsize = 100;
    testrun(ov_base64_encode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(4 == bsize);
    testrun(0 == strncmp("NjM/", (char *)buffer, bsize));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_base64_decode() {

    char *rfcTest[] = {"", "f", "fo", "foo", "foob", "fooba", "foobar"};

    char *rfcResult[] = {
        "", "Zg==", "Zm8=", "Zm9v", "Zm9vYg==", "Zm9vYmE=", "Zm9vYmFy"};

    uint8_t *result = NULL;
    size_t rsize = 0;
    size_t bsize = 100;
    uint8_t buffer[bsize];
    uint8_t *ptr = buffer;
    memset(buffer, 0, bsize);

    testrun(!ov_base64_decode(NULL, 0, NULL, 0));
    testrun(!ov_base64_decode((uint8_t *)"test", 4, &result, 0));
    testrun(!ov_base64_decode((uint8_t *)"test", 4, NULL, &rsize));
    testrun(!ov_base64_decode((uint8_t *)"test", 0, &result, &rsize));
    testrun(!ov_base64_decode(NULL, 4, &result, &rsize));

    testrun(!ov_base64_decode((uint8_t *)"test", 7, &result, &rsize));

    testrun(ov_base64_decode((uint8_t *)"test", 4, &result, &rsize));
    free(result);

    // check buffer fill
    testrun(ov_base64_decode((uint8_t *)"test", 4, &ptr, &bsize));

    int i;
    for (i = 0; i < 7; i++) {

        bsize = 100;

        if (i == 0) {
            testrun(!ov_base64_encode(
                (uint8_t *)rfcTest[i], strlen(rfcTest[i]), &ptr, &bsize));
        } else {
            testrun(ov_base64_encode(
                (uint8_t *)rfcTest[i], strlen(rfcTest[i]), &ptr, &bsize));

            testrun(0 == strncmp((char *)buffer,
                                 rfcResult[i],
                                 strlen(rfcResult[i])));
            testrun(bsize == strlen(rfcResult[i]));
        }
    }

    char *check = NULL;

    // check 1st value of the alphabet
    check = "MDBB";
    bsize = 100;
    testrun(ov_base64_decode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(3 == bsize);
    testrun(0 == strncmp("00A", (char *)buffer, bsize));

    // check last 4 values of the alphabet
    check = "NjA8";
    bsize = 100;
    testrun(ov_base64_decode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(3 == bsize);
    testrun(0 == strncmp("60<", (char *)buffer, bsize));

    check = "NjE9";
    bsize = 100;
    testrun(ov_base64_decode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(3 == bsize);
    testrun(0 == strncmp("61=", (char *)buffer, bsize));

    check = "NjI+";
    bsize = 100;
    testrun(ov_base64_decode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(3 == bsize);
    testrun(0 == strncmp("62>", (char *)buffer, bsize));

    check = "NjM/";
    bsize = 100;
    testrun(ov_base64_decode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(3 == bsize);
    testrun(0 == strncmp("63?", (char *)buffer, bsize));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_base64_url_encode() {

    uint8_t *result = NULL;
    size_t rsize = 0;
    size_t bsize = 100;
    uint8_t buffer[bsize];
    uint8_t *ptr = buffer;
    memset(buffer, 0, bsize);

    testrun(!ov_base64_url_encode(NULL, 0, NULL, 0));
    testrun(!ov_base64_url_encode((uint8_t *)"test", 4, &result, 0));
    testrun(!ov_base64_url_encode((uint8_t *)"test", 4, NULL, &rsize));
    testrun(!ov_base64_url_encode((uint8_t *)"test", 0, &result, &rsize));
    testrun(!ov_base64_url_encode(NULL, 4, &result, &rsize));

    testrun(ov_base64_url_encode((uint8_t *)"test", 4, &result, &rsize));
    testrun(result);
    testrun(0 == strncmp("dGVzdA==", (char *)result, rsize));
    free(result);

    char *check = NULL;

    // check 1st value of the alphabet
    check = "00A";
    bsize = 100;
    testrun(
        ov_base64_url_encode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(4 == bsize);
    testrun(0 == strncmp("MDBB", (char *)buffer, bsize));

    // check last 4 values of the alphabet
    check = "60<";
    bsize = 100;
    testrun(
        ov_base64_url_encode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(4 == bsize);
    testrun(0 == strncmp("NjA8", (char *)buffer, bsize));

    check = "61=";
    bsize = 100;
    testrun(
        ov_base64_url_encode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(4 == bsize);
    testrun(0 == strncmp("NjE9", (char *)buffer, bsize));

    check = "62>";
    bsize = 100;
    testrun(
        ov_base64_url_encode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(4 == bsize);
    testrun(0 == strncmp("NjI-", (char *)buffer, bsize));

    check = "63?";
    bsize = 100;
    testrun(
        ov_base64_url_encode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(4 == bsize);
    testrun(0 == strncmp("NjM_", (char *)buffer, bsize));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_base64_url_decode() {

    uint8_t *result = NULL;
    size_t rsize = 0;
    size_t bsize = 100;
    uint8_t buffer[bsize];
    uint8_t *ptr = buffer;
    memset(buffer, 0, bsize);

    testrun(!ov_base64_url_decode(NULL, 0, NULL, 0));
    testrun(!ov_base64_url_decode((uint8_t *)"test", 4, &result, 0));
    testrun(!ov_base64_url_decode((uint8_t *)"test", 4, NULL, &rsize));
    testrun(!ov_base64_url_decode((uint8_t *)"test", 0, &result, &rsize));
    testrun(!ov_base64_url_decode(NULL, 4, &result, &rsize));

    char *check = NULL;

    // check 1st value of the alphabet
    check = "MDBB";
    bsize = 100;
    testrun(
        ov_base64_url_decode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(3 == bsize);
    testrun(0 == strncmp("00A", (char *)buffer, bsize));

    // check last 4 values of the alphabet
    check = "NjA8";
    bsize = 100;
    testrun(
        ov_base64_url_decode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(3 == bsize);
    testrun(0 == strncmp("60<", (char *)buffer, bsize));

    check = "NjE9";
    bsize = 100;
    testrun(
        ov_base64_url_decode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(3 == bsize);
    testrun(0 == strncmp("61=", (char *)buffer, bsize));

    check = "NjI-";
    bsize = 100;
    testrun(
        ov_base64_url_decode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(3 == bsize);
    testrun(0 == strncmp("62>", (char *)buffer, bsize));

    check = "NjM_";
    bsize = 100;
    testrun(
        ov_base64_url_decode((uint8_t *)check, strlen(check), &ptr, &bsize));
    testrun(3 == bsize);
    testrun(0 == strncmp("63?", (char *)buffer, bsize));

    check = "NjI+";
    bsize = 100;
    testrun(
        !ov_base64_url_decode((uint8_t *)check, strlen(check), &ptr, &bsize));

    check = "NjM/";
    bsize = 100;
    testrun(
        !ov_base64_url_decode((uint8_t *)check, strlen(check), &ptr, &bsize));

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

    if (!testing) {
        fprintf(stdout, "TESTING SWITCHED OFF for ov_base64");
        return 1;
    }

    testrun_test(test_ov_base64_alphabetPosition);
    testrun_test(test_ov_base64_decode_with_alphabet);
    testrun_test(test_ov_base64_encode_with_alphabet);

    testrun_test(test_ov_base64_encode);
    testrun_test(test_ov_base64_decode);

    testrun_test(test_ov_base64_url_encode);
    testrun_test(test_ov_base64_url_decode);

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
