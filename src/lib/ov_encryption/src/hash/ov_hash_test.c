/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_hash_test.c
        @author         Markus Toepfer

        @date           2019-05-01

        @ingroup        ov_hash

        @brief          Unit tests of


        ------------------------------------------------------------------------
*/
#include "ov_hash.c"
#include <ov_base/ov_data_function.h>
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_hash_function_to_string() {

    char *expect = NULL;

    expect = "sha1";
    testrun(0 == strncmp(ov_hash_function_to_string(OV_HASH_SHA1),
                         expect,
                         strlen(expect)));

    expect = "sha256";
    testrun(0 == strncmp(ov_hash_function_to_string(OV_HASH_SHA256),
                         expect,
                         strlen(expect)));

    expect = "sha512";
    testrun(0 == strncmp(ov_hash_function_to_string(OV_HASH_SHA512),
                         expect,
                         strlen(expect)));

    expect = "md5";
    testrun(0 == strncmp(ov_hash_function_to_string(OV_HASH_MD5),
                         expect,
                         strlen(expect)));

    expect = "unspec";
    testrun(0 ==
            strncmp(ov_hash_function_to_string(0), expect, strlen(expect)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_hash_function_to_RFC8122_string() {

    char *expect = NULL;

    expect = "sha-1";
    testrun(0 == strncmp(ov_hash_function_to_RFC8122_string(OV_HASH_SHA1),
                         expect,
                         strlen(expect)));

    expect = "sha-256";
    testrun(0 == strncmp(ov_hash_function_to_RFC8122_string(OV_HASH_SHA256),
                         expect,
                         strlen(expect)));

    expect = "sha-512";
    testrun(0 == strncmp(ov_hash_function_to_RFC8122_string(OV_HASH_SHA512),
                         expect,
                         strlen(expect)));

    expect = "md5";
    testrun(0 == strncmp(ov_hash_function_to_RFC8122_string(OV_HASH_MD5),
                         expect,
                         strlen(expect)));

    testrun(NULL == ov_hash_function_to_RFC8122_string(0));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_hash_function_from_string() {

    char *src = "md5";

    testrun(OV_HASH_UNSPEC == ov_hash_function_from_string(NULL, 0));
    testrun(OV_HASH_UNSPEC == ov_hash_function_from_string(NULL, strlen(src)));
    testrun(OV_HASH_UNSPEC == ov_hash_function_from_string(src, 0));

    testrun(OV_HASH_MD5 == ov_hash_function_from_string(src, strlen(src)));

    src = "sha1";
    testrun(OV_HASH_SHA1 == ov_hash_function_from_string(src, strlen(src)));
    src = "sha-1";
    testrun(OV_HASH_SHA1 == ov_hash_function_from_string(src, strlen(src)));

    src = "sha256";
    testrun(OV_HASH_SHA256 == ov_hash_function_from_string(src, strlen(src)));
    src = "sha-256";
    testrun(OV_HASH_SHA256 == ov_hash_function_from_string(src, strlen(src)));

    src = "sha512";
    testrun(OV_HASH_SHA512 == ov_hash_function_from_string(src, strlen(src)));
    src = "sha-512";
    testrun(OV_HASH_SHA512 == ov_hash_function_from_string(src, strlen(src)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_hash_function_to_EVP() {

    testrun(EVP_sha1() == (EVP_MD *)ov_hash_function_to_EVP(OV_HASH_SHA1));
    testrun(EVP_sha256() == (EVP_MD *)ov_hash_function_to_EVP(OV_HASH_SHA256));
    testrun(EVP_sha512() == (EVP_MD *)ov_hash_function_to_EVP(OV_HASH_SHA512));
    testrun(EVP_md5() == (EVP_MD *)ov_hash_function_to_EVP(OV_HASH_MD5));
    testrun(NULL == (EVP_MD *)ov_hash_function_to_EVP(OV_HASH_UNSPEC));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_hash_string() {
    uint8_t *src = (uint8_t *)"some test data set to check";
    size_t src_len = strlen((char *)src);
    uint8_t *result = NULL;
    size_t res_size = 0;

    testrun(!ov_hash_string(OV_HASH_MD5, NULL, 0, NULL, NULL));
    testrun(!ov_hash_string(OV_HASH_MD5, NULL, src_len, &result, &res_size));
    testrun(!ov_hash_string(OV_HASH_MD5, src, 0, NULL, &res_size));
    testrun(!ov_hash_string(OV_HASH_MD5, src, src_len, &result, NULL));
    testrun(!ov_hash_string(0, src, src_len, &result, &res_size));

    testrun(ov_hash_string(OV_HASH_MD5, src, src_len, &result, &res_size));

    testrun(res_size == 16);
    testrun(result[0] == 0x6b);
    testrun(result[1] == 0x40);
    testrun(result[2] == 0x8a);
    testrun(result[3] == 0xed);
    testrun(result[4] == 0xde);
    testrun(result[5] == 0xeb);
    testrun(result[6] == 0x04);
    testrun(result[7] == 0x81);
    testrun(result[8] == 0x11);
    testrun(result[9] == 0xbc);
    testrun(result[10] == 0x6d);
    testrun(result[11] == 0xb0);
    testrun(result[12] == 0x16);
    testrun(result[13] == 0x7a);
    testrun(result[14] == 0x77);
    testrun(result[15] == 0x8c);

    result = ov_data_pointer_free(result);
    res_size = 0;
    testrun(ov_hash_string(OV_HASH_SHA1, src, src_len, &result, &res_size));

    testrun(res_size == 20);
    testrun(result[0] == 0x6b);
    testrun(result[1] == 0x56);
    testrun(result[2] == 0x39);
    testrun(result[3] == 0xf2);
    testrun(result[4] == 0xd4);
    testrun(result[5] == 0xb2);
    testrun(result[6] == 0xd4);
    testrun(result[7] == 0x45);
    testrun(result[8] == 0xb0);
    testrun(result[9] == 0x60);
    testrun(result[10] == 0x2b);
    testrun(result[11] == 0xd7);
    testrun(result[12] == 0x1c);
    testrun(result[13] == 0x93);
    testrun(result[14] == 0x8a);
    testrun(result[15] == 0x77);
    testrun(result[16] == 0x97);
    testrun(result[17] == 0x45);
    testrun(result[18] == 0x58);
    testrun(result[19] == 0x05);

    result = ov_data_pointer_free(result);
    res_size = 0;
    testrun(ov_hash_string(OV_HASH_SHA256, src, src_len, &result, &res_size));

    testrun(res_size == 32);
    testrun(result[0] == 0x84);
    testrun(result[1] == 0xbd);
    testrun(result[2] == 0xe6);
    testrun(result[3] == 0xb3);
    testrun(result[4] == 0xce);
    testrun(result[5] == 0x9a);
    testrun(result[6] == 0x49);
    testrun(result[7] == 0xd5);
    testrun(result[8] == 0x5a);
    testrun(result[9] == 0x4f);
    testrun(result[10] == 0x5d);
    testrun(result[11] == 0xc6);
    testrun(result[12] == 0x83);
    testrun(result[13] == 0xef);
    testrun(result[14] == 0x64);
    testrun(result[15] == 0xda);
    testrun(result[16] == 0x3c);
    testrun(result[17] == 0x6b);
    testrun(result[18] == 0x86);
    testrun(result[19] == 0xa7);
    testrun(result[20] == 0x88);
    testrun(result[21] == 0xb0);
    testrun(result[22] == 0x8d);
    testrun(result[23] == 0x04);
    testrun(result[24] == 0x3f);
    testrun(result[25] == 0x00);
    testrun(result[26] == 0xdb);
    testrun(result[27] == 0x63);
    testrun(result[28] == 0xc0);
    testrun(result[29] == 0x66);
    testrun(result[30] == 0xe6);
    testrun(result[31] == 0xa3);

    result = ov_data_pointer_free(result);
    res_size = 0;
    testrun(ov_hash_string(OV_HASH_SHA512, src, src_len, &result, &res_size));

    testrun(res_size == 64);
    testrun(result[0] == 0xf4);
    testrun(result[1] == 0xed);
    testrun(result[2] == 0xd4);
    testrun(result[3] == 0xca);
    testrun(result[4] == 0x22);
    testrun(result[5] == 0xcc);
    testrun(result[6] == 0xd1);
    testrun(result[7] == 0x03);
    testrun(result[8] == 0x62);
    testrun(result[9] == 0xc6);
    testrun(result[10] == 0xa8);
    testrun(result[11] == 0xc7);
    testrun(result[12] == 0x24);
    testrun(result[13] == 0x92);
    testrun(result[14] == 0xdc);
    testrun(result[15] == 0x07);
    testrun(result[16] == 0x5b);
    testrun(result[17] == 0xfc);
    testrun(result[18] == 0x7e);
    testrun(result[19] == 0xdc);
    testrun(result[20] == 0x6e);
    testrun(result[21] == 0x02);
    testrun(result[22] == 0x1b);
    testrun(result[23] == 0x5e);
    testrun(result[24] == 0xc9);
    testrun(result[25] == 0x97);
    testrun(result[26] == 0x8c);
    testrun(result[27] == 0x71);
    testrun(result[28] == 0x0c);
    testrun(result[29] == 0x76);
    testrun(result[30] == 0x79);
    testrun(result[31] == 0xf0);
    testrun(result[32] == 0x3c);
    testrun(result[33] == 0x4c);
    testrun(result[34] == 0x8b);
    testrun(result[35] == 0x74);
    testrun(result[36] == 0xd2);
    testrun(result[37] == 0x0d);
    testrun(result[38] == 0x36);
    testrun(result[39] == 0x18);
    testrun(result[40] == 0x5c);
    testrun(result[41] == 0x8a);
    testrun(result[42] == 0xae);
    testrun(result[43] == 0x4f);
    testrun(result[44] == 0xfa);
    testrun(result[45] == 0xf7);
    testrun(result[46] == 0x80);
    testrun(result[47] == 0x6c);
    testrun(result[48] == 0x73);
    testrun(result[49] == 0xe7);
    testrun(result[50] == 0x77);
    testrun(result[51] == 0xa2);
    testrun(result[52] == 0x1b);
    testrun(result[53] == 0xc7);
    testrun(result[54] == 0xd4);
    testrun(result[55] == 0x7c);
    testrun(result[56] == 0xf3);
    testrun(result[57] == 0x50);
    testrun(result[58] == 0xdc);
    testrun(result[59] == 0X6b);
    testrun(result[60] == 0Xb4);
    testrun(result[61] == 0Xcb);
    testrun(result[62] == 0X37);
    testrun(result[63] == 0X51);

    result = ov_data_pointer_free(result);

    // check fill mode

    uint8_t buffer[100];
    memset(buffer, 0, 100);

    result = buffer;
    res_size = 100;

    testrun(ov_hash_string(OV_HASH_SHA512, src, src_len, &result, &res_size));

    testrun(res_size == 64);
    testrun(result[0] == 0xf4);
    testrun(result[1] == 0xed);
    testrun(result[2] == 0xd4);
    testrun(result[3] == 0xca);
    testrun(result[4] == 0x22);
    testrun(result[5] == 0xcc);
    testrun(result[6] == 0xd1);
    testrun(result[7] == 0x03);
    testrun(result[8] == 0x62);
    testrun(result[9] == 0xc6);
    testrun(result[10] == 0xa8);
    testrun(result[11] == 0xc7);
    testrun(result[12] == 0x24);
    testrun(result[13] == 0x92);
    testrun(result[14] == 0xdc);
    testrun(result[15] == 0x07);
    testrun(result[16] == 0x5b);
    testrun(result[17] == 0xfc);
    testrun(result[18] == 0x7e);
    testrun(result[19] == 0xdc);
    testrun(result[20] == 0x6e);
    testrun(result[21] == 0x02);
    testrun(result[22] == 0x1b);
    testrun(result[23] == 0x5e);
    testrun(result[24] == 0xc9);
    testrun(result[25] == 0x97);
    testrun(result[26] == 0x8c);
    testrun(result[27] == 0x71);
    testrun(result[28] == 0x0c);
    testrun(result[29] == 0x76);
    testrun(result[30] == 0x79);
    testrun(result[31] == 0xf0);
    testrun(result[32] == 0x3c);
    testrun(result[33] == 0x4c);
    testrun(result[34] == 0x8b);
    testrun(result[35] == 0x74);
    testrun(result[36] == 0xd2);
    testrun(result[37] == 0x0d);
    testrun(result[38] == 0x36);
    testrun(result[39] == 0x18);
    testrun(result[40] == 0x5c);
    testrun(result[41] == 0x8a);
    testrun(result[42] == 0xae);
    testrun(result[43] == 0x4f);
    testrun(result[44] == 0xfa);
    testrun(result[45] == 0xf7);
    testrun(result[46] == 0x80);
    testrun(result[47] == 0x6c);
    testrun(result[48] == 0x73);
    testrun(result[49] == 0xe7);
    testrun(result[50] == 0x77);
    testrun(result[51] == 0xa2);
    testrun(result[52] == 0x1b);
    testrun(result[53] == 0xc7);
    testrun(result[54] == 0xd4);
    testrun(result[55] == 0x7c);
    testrun(result[56] == 0xf3);
    testrun(result[57] == 0x50);
    testrun(result[58] == 0xdc);
    testrun(result[59] == 0X6b);
    testrun(result[60] == 0Xb4);
    testrun(result[61] == 0Xcb);
    testrun(result[62] == 0X37);
    testrun(result[63] == 0X51);

    for (size_t i = 64; i < 100; i++)
        testrun(buffer[i] == 0);

    // check buffer fit
    memset(buffer, 0, 100);

    result = buffer;
    res_size = 64;

    testrun(ov_hash_string(OV_HASH_SHA512, src, src_len, &result, &res_size));

    testrun(result[53] == 0xc7);
    testrun(result[64] == 0);

    // check buffer < min
    memset(buffer, 0, 100);

    result = buffer;
    res_size = 63;

    testrun(!ov_hash_string(OV_HASH_SHA512, src, src_len, &result, &res_size));

    for (size_t i = 0; i < 100; i++)
        testrun(buffer[i] == 0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_hash() {

    const char *src[] = {"some test data set to check",
                         "string2",
                         "string3",
                         "some other string\r\n",
                         "any \t\n\r other\n string content"};

    uint8_t *result = NULL;
    size_t res_size = 0;

    testrun(!ov_hash(OV_HASH_MD5, NULL, 0, NULL, NULL));
    testrun(!ov_hash(OV_HASH_MD5, NULL, 1, &result, &res_size));
    testrun(!ov_hash(OV_HASH_MD5, src, 0, NULL, &res_size));
    testrun(!ov_hash(OV_HASH_MD5, src, 1, &result, NULL));
    testrun(!ov_hash(0, src, 1, &result, &res_size));

    testrun(ov_hash(OV_HASH_MD5, src, 1, &result, &res_size));

    testrun(res_size == 16);
    testrun(result[0] == 0x6b);
    testrun(result[1] == 0x40);
    testrun(result[2] == 0x8a);
    testrun(result[3] == 0xed);
    testrun(result[4] == 0xde);
    testrun(result[5] == 0xeb);
    testrun(result[6] == 0x04);
    testrun(result[7] == 0x81);
    testrun(result[8] == 0x11);
    testrun(result[9] == 0xbc);
    testrun(result[10] == 0x6d);
    testrun(result[11] == 0xb0);
    testrun(result[12] == 0x16);
    testrun(result[13] == 0x7a);
    testrun(result[14] == 0x77);
    testrun(result[15] == 0x8c);

    result = ov_data_pointer_free(result);
    res_size = 0;

    testrun(ov_hash(OV_HASH_MD5, src, 5, &result, &res_size));

    testrun(res_size == 16);
    testrun(result[0] == 0xdc);
    testrun(result[1] == 0x8e);
    testrun(result[2] == 0xad);
    testrun(result[3] == 0x58);
    testrun(result[4] == 0x65);
    testrun(result[5] == 0x32);
    testrun(result[6] == 0xcb);
    testrun(result[7] == 0x18);
    testrun(result[8] == 0x18);
    testrun(result[9] == 0xfb);
    testrun(result[10] == 0xf8);
    testrun(result[11] == 0x66);
    testrun(result[12] == 0x22);
    testrun(result[13] == 0xe6);
    testrun(result[14] == 0xcf);
    testrun(result[15] == 0xfc);

    result = ov_data_pointer_free(result);
    res_size = 0;

    testrun(ov_hash(OV_HASH_SHA1, src, 1, &result, &res_size));

    testrun(res_size == 20);
    testrun(result[0] == 0x6b);
    testrun(result[1] == 0x56);
    testrun(result[2] == 0x39);
    testrun(result[3] == 0xf2);
    testrun(result[4] == 0xd4);
    testrun(result[5] == 0xb2);
    testrun(result[6] == 0xd4);
    testrun(result[7] == 0x45);
    testrun(result[8] == 0xb0);
    testrun(result[9] == 0x60);
    testrun(result[10] == 0x2b);
    testrun(result[11] == 0xd7);
    testrun(result[12] == 0x1c);
    testrun(result[13] == 0x93);
    testrun(result[14] == 0x8a);
    testrun(result[15] == 0x77);
    testrun(result[16] == 0x97);
    testrun(result[17] == 0x45);
    testrun(result[18] == 0x58);
    testrun(result[19] == 0x05);

    result = ov_data_pointer_free(result);
    res_size = 0;

    testrun(ov_hash(OV_HASH_SHA1, src, 5, &result, &res_size));

    testrun(res_size == 20);
    testrun(result[0] == 0xdf);
    testrun(result[1] == 0x90);
    testrun(result[2] == 0xf2);
    testrun(result[3] == 0x08);
    testrun(result[4] == 0xe7);
    testrun(result[5] == 0x06);
    testrun(result[6] == 0x27);
    testrun(result[7] == 0xdc);
    testrun(result[8] == 0x6b);
    testrun(result[9] == 0x04);
    testrun(result[10] == 0x07);
    testrun(result[11] == 0x6e);
    testrun(result[12] == 0x7f);
    testrun(result[13] == 0xf7);
    testrun(result[14] == 0x67);
    testrun(result[15] == 0x4a);
    testrun(result[16] == 0x4c);
    testrun(result[17] == 0x9b);
    testrun(result[18] == 0x26);
    testrun(result[19] == 0x6a);

    result = ov_data_pointer_free(result);
    res_size = 0;

    testrun(ov_hash(OV_HASH_SHA256, src, 1, &result, &res_size));

    testrun(res_size == 32);
    testrun(result[0] == 0x84);
    testrun(result[1] == 0xbd);
    testrun(result[2] == 0xe6);
    testrun(result[3] == 0xb3);
    testrun(result[4] == 0xce);
    testrun(result[5] == 0x9a);
    testrun(result[6] == 0x49);
    testrun(result[7] == 0xd5);
    testrun(result[8] == 0x5a);
    testrun(result[9] == 0x4f);
    testrun(result[10] == 0x5d);
    testrun(result[11] == 0xc6);
    testrun(result[12] == 0x83);
    testrun(result[13] == 0xef);
    testrun(result[14] == 0x64);
    testrun(result[15] == 0xda);
    testrun(result[16] == 0x3c);
    testrun(result[17] == 0x6b);
    testrun(result[18] == 0x86);
    testrun(result[19] == 0xa7);
    testrun(result[20] == 0x88);
    testrun(result[21] == 0xb0);
    testrun(result[22] == 0x8d);
    testrun(result[23] == 0x04);
    testrun(result[24] == 0x3f);
    testrun(result[25] == 0x00);
    testrun(result[26] == 0xdb);
    testrun(result[27] == 0x63);
    testrun(result[28] == 0xc0);
    testrun(result[29] == 0x66);
    testrun(result[30] == 0xe6);
    testrun(result[31] == 0xa3);

    result = ov_data_pointer_free(result);
    res_size = 0;

    testrun(ov_hash(OV_HASH_SHA256, src, 5, &result, &res_size));

    testrun(res_size == 32);
    testrun(result[0] == 0x38);
    testrun(result[1] == 0xbf);
    testrun(result[2] == 0x16);
    testrun(result[3] == 0x5b);
    testrun(result[4] == 0x0a);
    testrun(result[5] == 0x22);
    testrun(result[6] == 0xe4);
    testrun(result[7] == 0x2d);
    testrun(result[8] == 0x3c);
    testrun(result[9] == 0xea);
    testrun(result[10] == 0xcf);
    testrun(result[11] == 0x27);
    testrun(result[12] == 0xd4);
    testrun(result[13] == 0xc5);
    testrun(result[14] == 0x7a);
    testrun(result[15] == 0x76);
    testrun(result[16] == 0x02);
    testrun(result[17] == 0xd1);
    testrun(result[18] == 0xa1);
    testrun(result[19] == 0xc1);
    testrun(result[20] == 0x07);
    testrun(result[21] == 0xf4);
    testrun(result[22] == 0x17);
    testrun(result[23] == 0xeb);
    testrun(result[24] == 0xc9);
    testrun(result[25] == 0xfe);
    testrun(result[26] == 0x03);
    testrun(result[27] == 0xf5);
    testrun(result[28] == 0xfd);
    testrun(result[29] == 0x7e);
    testrun(result[30] == 0xb4);
    testrun(result[31] == 0xe3);

    result = ov_data_pointer_free(result);
    res_size = 0;

    testrun(ov_hash(OV_HASH_SHA512, src, 1, &result, &res_size));

    testrun(res_size == 64);
    testrun(result[0] == 0xf4);
    testrun(result[1] == 0xed);
    testrun(result[2] == 0xd4);
    testrun(result[3] == 0xca);
    testrun(result[4] == 0x22);
    testrun(result[5] == 0xcc);
    testrun(result[6] == 0xd1);
    testrun(result[7] == 0x03);
    testrun(result[8] == 0x62);
    testrun(result[9] == 0xc6);
    testrun(result[10] == 0xa8);
    testrun(result[11] == 0xc7);
    testrun(result[12] == 0x24);
    testrun(result[13] == 0x92);
    testrun(result[14] == 0xdc);
    testrun(result[15] == 0x07);
    testrun(result[16] == 0x5b);
    testrun(result[17] == 0xfc);
    testrun(result[18] == 0x7e);
    testrun(result[19] == 0xdc);
    testrun(result[20] == 0x6e);
    testrun(result[21] == 0x02);
    testrun(result[22] == 0x1b);
    testrun(result[23] == 0x5e);
    testrun(result[24] == 0xc9);
    testrun(result[25] == 0x97);
    testrun(result[26] == 0x8c);
    testrun(result[27] == 0x71);
    testrun(result[28] == 0x0c);
    testrun(result[29] == 0x76);
    testrun(result[30] == 0x79);
    testrun(result[31] == 0xf0);
    testrun(result[32] == 0x3c);
    testrun(result[33] == 0x4c);
    testrun(result[34] == 0x8b);
    testrun(result[35] == 0x74);
    testrun(result[36] == 0xd2);
    testrun(result[37] == 0x0d);
    testrun(result[38] == 0x36);
    testrun(result[39] == 0x18);
    testrun(result[40] == 0x5c);
    testrun(result[41] == 0x8a);
    testrun(result[42] == 0xae);
    testrun(result[43] == 0x4f);
    testrun(result[44] == 0xfa);
    testrun(result[45] == 0xf7);
    testrun(result[46] == 0x80);
    testrun(result[47] == 0x6c);
    testrun(result[48] == 0x73);
    testrun(result[49] == 0xe7);
    testrun(result[50] == 0x77);
    testrun(result[51] == 0xa2);
    testrun(result[52] == 0x1b);
    testrun(result[53] == 0xc7);
    testrun(result[54] == 0xd4);
    testrun(result[55] == 0x7c);
    testrun(result[56] == 0xf3);
    testrun(result[57] == 0x50);
    testrun(result[58] == 0xdc);
    testrun(result[59] == 0X6b);
    testrun(result[60] == 0Xb4);
    testrun(result[61] == 0Xcb);
    testrun(result[62] == 0X37);
    testrun(result[63] == 0X51);

    result = ov_data_pointer_free(result);
    res_size = 0;

    testrun(ov_hash(OV_HASH_SHA512, src, 5, &result, &res_size));

    testrun(res_size == 64);
    testrun(result[0] == 0xa8);
    testrun(result[1] == 0xfb);
    testrun(result[2] == 0x07);
    testrun(result[3] == 0x4b);
    testrun(result[4] == 0x34);
    testrun(result[5] == 0x5d);
    testrun(result[6] == 0xf6);
    testrun(result[7] == 0xb7);
    testrun(result[8] == 0x0d);
    testrun(result[9] == 0xb7);
    testrun(result[10] == 0x8e);
    testrun(result[11] == 0x9c);
    testrun(result[12] == 0x5e);
    testrun(result[13] == 0x0b);
    testrun(result[14] == 0x5a);
    testrun(result[15] == 0x8c);
    testrun(result[16] == 0xb7);
    testrun(result[17] == 0x15);
    testrun(result[18] == 0xe2);
    testrun(result[19] == 0xc5);
    testrun(result[20] == 0x3e);
    testrun(result[21] == 0x46);
    testrun(result[22] == 0x97);
    testrun(result[23] == 0xfd);
    testrun(result[24] == 0xee);
    testrun(result[25] == 0x05);
    testrun(result[26] == 0x73);
    testrun(result[27] == 0x3b);
    testrun(result[28] == 0xf5);
    testrun(result[29] == 0x6f);
    testrun(result[30] == 0x91);
    testrun(result[31] == 0x5d);
    testrun(result[32] == 0xfd);
    testrun(result[33] == 0x28);
    testrun(result[34] == 0x6e);
    testrun(result[35] == 0xb4);
    testrun(result[36] == 0xbb);
    testrun(result[37] == 0x4a);
    testrun(result[38] == 0x6a);
    testrun(result[39] == 0x3b);
    testrun(result[40] == 0x0b);
    testrun(result[41] == 0x94);
    testrun(result[42] == 0xce);
    testrun(result[43] == 0xbf);
    testrun(result[44] == 0x85);
    testrun(result[45] == 0x08);
    testrun(result[46] == 0xe3);
    testrun(result[47] == 0x6c);
    testrun(result[48] == 0xf6);
    testrun(result[49] == 0xd9);
    testrun(result[50] == 0x21);
    testrun(result[51] == 0x8b);
    testrun(result[52] == 0x6b);
    testrun(result[53] == 0xf0);
    testrun(result[54] == 0x40);
    testrun(result[55] == 0xf4);
    testrun(result[56] == 0x43);
    testrun(result[57] == 0x2d);
    testrun(result[58] == 0xff);
    testrun(result[59] == 0X94);
    testrun(result[60] == 0Xbd);
    testrun(result[61] == 0X45);
    testrun(result[62] == 0X02);
    testrun(result[63] == 0Xcc);

    result = ov_data_pointer_free(result);
    res_size = 0;

    // check fill mode

    uint8_t buffer[100];
    memset(buffer, 0, 100);

    result = buffer;
    res_size = 100;

    testrun(ov_hash(OV_HASH_SHA512, src, 5, &result, &res_size));

    testrun(res_size == 64);
    testrun(result[0] == 0xa8);
    testrun(result[1] == 0xfb);
    testrun(result[2] == 0x07);
    testrun(result[3] == 0x4b);
    testrun(result[4] == 0x34);
    testrun(result[5] == 0x5d);
    testrun(result[6] == 0xf6);
    testrun(result[7] == 0xb7);
    testrun(result[8] == 0x0d);
    testrun(result[9] == 0xb7);
    testrun(result[10] == 0x8e);
    testrun(result[11] == 0x9c);
    testrun(result[12] == 0x5e);
    testrun(result[13] == 0x0b);
    testrun(result[14] == 0x5a);
    testrun(result[15] == 0x8c);
    testrun(result[16] == 0xb7);
    testrun(result[17] == 0x15);
    testrun(result[18] == 0xe2);
    testrun(result[19] == 0xc5);
    testrun(result[20] == 0x3e);
    testrun(result[21] == 0x46);
    testrun(result[22] == 0x97);
    testrun(result[23] == 0xfd);
    testrun(result[24] == 0xee);
    testrun(result[25] == 0x05);
    testrun(result[26] == 0x73);
    testrun(result[27] == 0x3b);
    testrun(result[28] == 0xf5);
    testrun(result[29] == 0x6f);
    testrun(result[30] == 0x91);
    testrun(result[31] == 0x5d);
    testrun(result[32] == 0xfd);
    testrun(result[33] == 0x28);
    testrun(result[34] == 0x6e);
    testrun(result[35] == 0xb4);
    testrun(result[36] == 0xbb);
    testrun(result[37] == 0x4a);
    testrun(result[38] == 0x6a);
    testrun(result[39] == 0x3b);
    testrun(result[40] == 0x0b);
    testrun(result[41] == 0x94);
    testrun(result[42] == 0xce);
    testrun(result[43] == 0xbf);
    testrun(result[44] == 0x85);
    testrun(result[45] == 0x08);
    testrun(result[46] == 0xe3);
    testrun(result[47] == 0x6c);
    testrun(result[48] == 0xf6);
    testrun(result[49] == 0xd9);
    testrun(result[50] == 0x21);
    testrun(result[51] == 0x8b);
    testrun(result[52] == 0x6b);
    testrun(result[53] == 0xf0);
    testrun(result[54] == 0x40);
    testrun(result[55] == 0xf4);
    testrun(result[56] == 0x43);
    testrun(result[57] == 0x2d);
    testrun(result[58] == 0xff);
    testrun(result[59] == 0X94);
    testrun(result[60] == 0Xbd);
    testrun(result[61] == 0X45);
    testrun(result[62] == 0X02);
    testrun(result[63] == 0Xcc);

    for (size_t i = 64; i < 100; i++)
        testrun(buffer[i] == 0);

    // check buffer fit
    memset(buffer, 0, 100);

    result = buffer;
    res_size = 64;

    testrun(ov_hash(OV_HASH_SHA512, src, 5, &result, &res_size));

    testrun(result[63] == 0xcc);
    testrun(result[64] == 0);

    // check buffer < min
    memset(buffer, 0, 100);

    result = buffer;
    res_size = 63;

    testrun(!ov_hash(OV_HASH_SHA512, src, 5, &result, &res_size));

    for (size_t i = 0; i < 100; i++)
        testrun(buffer[i] == 0);

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
    testrun_test(test_ov_hash_function_to_string);
    testrun_test(test_ov_hash_function_to_RFC8122_string);
    testrun_test(test_ov_hash_function_from_string);
    testrun_test(test_ov_hash_function_to_EVP);

    testrun_test(test_ov_hash_string);
    testrun_test(test_ov_hash);

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
