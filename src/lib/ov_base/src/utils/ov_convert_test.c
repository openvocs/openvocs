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
        @file           ov_convert_test.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-03-06

        @ingroup        ov_basics

        @brief          Testing of basic convertions.


        ------------------------------------------------------------------------
*/

#include "ov_convert.c"
#include <ov_test/testrun.h>

#include <float.h>
#include <math.h>
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

int test_ov_convert_msecs_to_samples() {

    // ov_convert_msecs_to_samples(msecs, samplerate_hz)                  ;

    testrun(0 == ov_convert_msecs_to_samples(5, 0));
    testrun(0 == ov_convert_msecs_to_samples(0, 10000));

    testrun(50 == ov_convert_msecs_to_samples(5, 10000));
    testrun(10 == ov_convert_msecs_to_samples(1, 10000));
    testrun(15 == ov_convert_msecs_to_samples(1, 15000));
    testrun(81 == ov_convert_msecs_to_samples(9, 9000));
    testrun(448 == ov_convert_msecs_to_samples(10, 44800));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_convert_samples_to_msecs() {

    testrun(0 == ov_convert_samples_to_msecs(5, 0));
    testrun(0 == ov_convert_samples_to_msecs(0, 18000));

    return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      STRING TO ...
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_convert_string_to_uint64(void) {

    uint64_t size = 100;
    char buffer[size];
    memset(buffer, '\0', size);

    memcpy(buffer, "123", 3);

    uint64_t number = 0;

    testrun(!ov_convert_string_to_uint64(NULL, 0, NULL));
    testrun(!ov_convert_string_to_uint64(buffer, 3, NULL));
    testrun(!ov_convert_string_to_uint64(buffer, 0, &number));
    testrun(!ov_convert_string_to_uint64(NULL, 3, &number));

    testrun(ov_convert_string_to_uint64(buffer, 3, &number));
    testrun(123 == number);

    testrun(ov_convert_string_to_uint64(buffer, 3, &number));
    testrun(123 == number);

    testrun(ov_convert_string_to_uint64(buffer, 2, &number));
    testrun(12 == number);

    testrun(ov_convert_string_to_uint64(buffer, 1, &number));
    testrun(1 == number);

    testrun(ov_convert_string_to_uint64(buffer, 30, &number));
    testrun(123 == number);

    memcpy(buffer, "-12345", 6);
    testrun(!ov_convert_string_to_uint64(buffer, 30, &number));

    memcpy(buffer, "012345", 7);
    testrun(ov_convert_string_to_uint64(buffer, 30, &number));
    testrun(12345 == number);

    memcpy(buffer, "123A45", 7);
    testrun(ov_convert_string_to_uint64(buffer, 3, &number));
    testrun(123 == number);

    testrun(!ov_convert_string_to_uint64(buffer, 4, &number));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_convert_string_to_int64(void) {

    uint64_t size = 100;
    char buffer[size];
    memset(buffer, '\0', size);

    memcpy(buffer, "123", 3);

    int64_t number = 0;

    testrun(ov_convert_string_to_int64(buffer, 3, &number));
    testrun(123 == number);

    testrun(ov_convert_string_to_int64(buffer, 3, &number));
    testrun(123 == number);

    testrun(ov_convert_string_to_int64(buffer, 2, &number));
    testrun(12 == number);

    testrun(ov_convert_string_to_int64(buffer, 1, &number));
    testrun(1 == number);

    testrun(ov_convert_string_to_int64(buffer, 30, &number));
    testrun(123 == number);

    memcpy(buffer, "-12345", 6);
    testrun(ov_convert_string_to_int64(buffer, 30, &number));
    testrun(-12345 == number);

    memcpy(buffer, "012345", 7);
    testrun(ov_convert_string_to_int64(buffer, 30, &number));
    testrun(12345 == number);

    memcpy(buffer, "123A45", 7);
    testrun(ov_convert_string_to_int64(buffer, 3, &number));
    testrun(123 == number);

    testrun(!ov_convert_string_to_int64(buffer, 4, &number));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_convert_string_to_double(void) {

    uint64_t size = 100;
    char buffer[size];
    memset(buffer, '\0', size);

    memcpy(buffer, "123", 3);

    double number = 0;

    testrun(ov_convert_string_to_double(buffer, 3, &number));
    testrun(123 == number);

    testrun(ov_convert_string_to_double(buffer, 3, &number));
    testrun(123 == number);

    testrun(ov_convert_string_to_double(buffer, 2, &number));
    testrun(12 == number);

    testrun(ov_convert_string_to_double(buffer, 1, &number));
    testrun(1 == number);

    testrun(ov_convert_string_to_double(buffer, 30, &number));
    testrun(123 == number);

    memcpy(buffer, "-12345", 6);
    testrun(ov_convert_string_to_double(buffer, 30, &number));
    testrun(-12345 == number);

    memcpy(buffer, "012345", 7);
    testrun(ov_convert_string_to_double(buffer, 30, &number));
    testrun(12345 == number);

    memcpy(buffer, "123A45", 7);
    testrun(ov_convert_string_to_double(buffer, 3, &number));
    testrun(123 == number);

    testrun(!ov_convert_string_to_double(buffer, 4, &number));

    memcpy(buffer, "-1.2345", strlen("-1.2345"));
    testrun(ov_convert_string_to_double(buffer, 30, &number));
    testrun(-1.2345 == number);

    memset(buffer, '\0', size);
    memcpy(buffer, "-1e5", strlen("-1e5"));
    testrun(ov_convert_string_to_double(buffer, 30, &number));
    testrun(-1.e5 == number);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_convert_double_to_string() {

    uint64_t size = 100;
    char buffer[size];
    memset(buffer, '\0', size);

    char *string = NULL;
    size_t length = 0;
    double number = 1.1;

    testrun(!ov_convert_double_to_string(0, NULL, NULL));
    testrun(!ov_convert_double_to_string(number, NULL, &length));
    testrun(!ov_convert_double_to_string(number, &string, NULL));

    // non allocated pointer
    testrun(ov_convert_double_to_string(number, &string, &length));
    testrun(string);
    testrun(length == 3);
    testrun(strncmp(string, "1.1", 3) == 0);
    free(string);
    string = NULL;

    // non allocated pointer
    testrun(ov_convert_double_to_string(1e-05, &string, &length));
    testrun(string);
    testrun(length == 5);
    testrun(strncmp(string, "1e-05", 5) == 0);
    free(string);
    string = NULL;

    // non allocated pointer
    testrun(ov_convert_double_to_string(DBL_MAX, &string, &length));
    testrun(string);
    testrun(length == 12);
    testrun(strncmp(string, "1.79769e+308", 12) == 0);
    free(string);
    string = NULL;

    // non allocated pointer
    testrun(ov_convert_double_to_string(-DBL_MAX, &string, &length));
    testrun(string);
    testrun(length == 13);
    testrun(strncmp(string, "-1.79769e+308", 13) == 0);
    free(string);
    string = NULL;

    // non allocated pointer
    testrun(ov_convert_double_to_string(DBL_MIN, &string, &length));
    testrun(string);
    testrun(length == 12);
    testrun(strncmp(string, "2.22507e-308", 12) == 0);
    free(string);
    string = NULL;

    // preallocated allocated pointer
    length = size;
    string = buffer;
    testrun(ov_convert_double_to_string(1e-05, &string, &length));
    testrun(length == 5);
    testrun(strncmp(buffer, "1e-05", 5) == 0);

    // preallocated allocated pointer
    length = size;
    string = buffer;
    testrun(ov_convert_double_to_string(-1.123, &string, &length));
    testrun(length == 6);
    testrun(strncmp(buffer, "-1.123", 6) == 0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_convert_int64_to_string() {

    uint64_t size = 100;
    char buffer[size];
    memset(buffer, '\0', size);

    char *string = NULL;
    size_t length = 0;
    int64_t number = 1;

    testrun(!ov_convert_int64_to_string(0, NULL, NULL));
    testrun(!ov_convert_int64_to_string(number, NULL, &length));
    testrun(!ov_convert_int64_to_string(number, &string, NULL));

    // non allocated pointer
    testrun(ov_convert_int64_to_string(number, &string, &length));
    testrun(string);
    testrun(length == 1);
    testrun(strncmp(string, "1", 1) == 0);
    free(string);
    string = NULL;

    // non allocated pointer
    testrun(ov_convert_int64_to_string(-1, &string, &length));
    testrun(string);
    testrun(length == 2);
    testrun(strncmp(string, "-1", 2) == 0);
    free(string);
    string = NULL;

    // non allocated pointer
    testrun(ov_convert_int64_to_string(INT64_MAX, &string, &length));
    testrun(string);
    testrun(length == 19);
    testrun(strncmp(string, "9223372036854775807", 19) == 0);
    free(string);
    string = NULL;

    // non allocated pointer
    testrun(ov_convert_int64_to_string(INT64_MIN, &string, &length));
    testrun(string);
    testrun(length == 20);
    testrun(strncmp(string, "-9223372036854775808", 20) == 0);
    free(string);
    string = NULL;

    // preallocated allocated pointer
    length = size;
    string = buffer;
    testrun(ov_convert_int64_to_string(12345, &string, &length));
    testrun(length == 5);
    testrun(strncmp(buffer, "12345", 5) == 0);

    // preallocated allocated pointer
    length = size;
    string = buffer;
    testrun(ov_convert_int64_to_string(-12345, &string, &length));
    testrun(length == 6);
    testrun(strncmp(buffer, "-12345", 6) == 0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_convert_uint64_to_string() {

    uint64_t size = 100;
    char buffer[size];
    memset(buffer, '\0', size);

    char *string = NULL;
    size_t length = 0;
    uint64_t number = 123;

    testrun(!ov_convert_uint64_to_string(0, NULL, NULL));
    testrun(!ov_convert_uint64_to_string(number, NULL, &length));
    testrun(!ov_convert_uint64_to_string(number, &string, NULL));

    // non allocated pointer
    testrun(ov_convert_uint64_to_string(number, &string, &length));
    testrun(string);
    testrun(length == 3);
    testrun(strncmp(string, "123", 3) == 0);
    free(string);
    string = NULL;

    // non allocated pointer
    testrun(ov_convert_uint64_to_string(INT64_MAX, &string, &length));
    testrun(string);
    testrun(length == 19);
    testrun(strncmp(string, "9223372036854775807", 19) == 0);
    free(string);
    string = NULL;

    // non allocated pointer
    testrun(ov_convert_uint64_to_string(UINT64_MAX, &string, &length));
    testrun(string);
    testrun(length == 20);
    testrun(strncmp(string, "18446744073709551615", 20) == 0);
    free(string);
    string = NULL;

    // preallocated allocated pointer
    length = size;
    string = buffer;
    testrun(ov_convert_uint64_to_string(12345, &string, &length));
    testrun(length == 5);
    testrun(strncmp(buffer, "12345", 5) == 0);

    // preallocated allocated pointer
    length = size;
    string = buffer;
    testrun(ov_convert_uint64_to_string(-1, &string, &length));
    testrun(length == 20);
    testrun(strncmp(string, "18446744073709551615", 20) == 0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_convert_hex_string_to_uint64(void) {

    uint64_t size = 100;
    char buffer[size];
    memset(buffer, '\0', size);

    memcpy(buffer, "A", 1);

    uint64_t number = 0;

    testrun(!ov_convert_hex_string_to_uint64(NULL, 0, NULL));
    testrun(!ov_convert_hex_string_to_uint64(buffer, 1, NULL));
    testrun(!ov_convert_hex_string_to_uint64(buffer, 0, &number));
    testrun(!ov_convert_hex_string_to_uint64(NULL, 1, &number));

    testrun(ov_convert_hex_string_to_uint64(buffer, 1, &number));
    testrun(0xA == number);

    memset(buffer, '\0', size);
    buffer[0] = 'A';
    testrun(ov_convert_hex_string_to_uint64(buffer, 30, &number));
    testrun(10 == number);
    buffer[0] = 'B';
    testrun(ov_convert_hex_string_to_uint64(buffer, 30, &number));
    testrun(11 == number);
    buffer[0] = 'C';
    testrun(ov_convert_hex_string_to_uint64(buffer, 30, &number));
    testrun(12 == number);
    buffer[0] = 'D';
    testrun(ov_convert_hex_string_to_uint64(buffer, 30, &number));
    testrun(13 == number);
    buffer[0] = 'E';
    testrun(ov_convert_hex_string_to_uint64(buffer, 30, &number));
    testrun(14 == number);
    buffer[0] = 'F';
    testrun(ov_convert_hex_string_to_uint64(buffer, 30, &number));
    testrun(15 == number);

    memset(buffer, '\0', size);
    strcat(buffer, "0xabc12");
    testrun(ov_convert_hex_string_to_uint64(buffer, 30, &number));
    testrun(0xabc12 == number);

    memset(buffer, '\0', size);
    strcat(buffer, "FFFFFFFFFFFFFFFF");
    testrun(ov_convert_hex_string_to_uint64(buffer, 30, &number));
    testrun(INT64_MAX == number);

    memset(buffer, '\0', size);
    strcat(buffer, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
    testrun(ov_convert_hex_string_to_uint64(buffer, strlen(buffer), &number));
    testrun(INT64_MAX == number);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_convert_binary_to_hex() {

    size_t size = 100;
    uint8_t binary[size];
    memset(binary, '\0', size);

    uint8_t buffer[size * 2];
    memset(buffer, '\0', size * 2);
    size_t length = 0;

    uint8_t *out = NULL;
    size_t olen = 0;

    uint8_t *ptr = buffer;

    // check all different values

    for (size_t i = 0; i < 0xff; i++) {

        binary[0] = i;
        length = size;
        testrun(ov_convert_binary_to_hex(binary, 1, &ptr, &length));
        testrun(2 == length);
        testrun(buffer[0] = "0123456789ABCDEF"[i >> 4]);
        testrun(buffer[1] = "0123456789ABCDEF"[i & 0x0f]);
    }

    // check some string
    binary[0] = 250;
    binary[1] = 206;
    binary[2] = 1;
    binary[3] = 16;
    binary[4] = 222;
    binary[5] = 173;
    binary[6] = 66;
    length = size;
    testrun(ov_convert_binary_to_hex(binary, 7, &ptr, &length));
    testrun(length == 14);
    buffer[0] = 'F';
    buffer[1] = 'A';
    buffer[2] = 'C';
    buffer[3] = 'E';
    buffer[4] = '0';
    buffer[5] = '1';
    buffer[6] = '1';
    buffer[7] = '0';
    buffer[8] = 'D';
    buffer[9] = 'E';
    buffer[10] = 'A';
    buffer[11] = 'D';
    buffer[12] = '4';
    buffer[13] = '2';
    buffer[14] = 0;

    // check creation and length cut

    testrun(ov_convert_binary_to_hex(binary, 5, &out, &olen));
    testrun(olen == 10);
    out[0] = 'F';
    out[1] = 'A';
    out[2] = 'C';
    out[3] = 'E';
    out[4] = '0';
    out[5] = '1';
    out[6] = '1';
    out[7] = '0';
    out[8] = 'D';
    out[9] = 'E';
    out[10] = 0;
    free(out);

    // invalid input
    testrun(!ov_convert_binary_to_hex(NULL, 5, &out, &olen));
    testrun(!ov_convert_binary_to_hex(binary, 0, &out, &olen));
    testrun(!ov_convert_binary_to_hex(binary, 5, NULL, &olen));
    testrun(!ov_convert_binary_to_hex(binary, 5, &out, NULL));
    testrun(!ov_convert_binary_to_hex(NULL, 0, NULL, NULL));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_convert_hex_to_binary() {

    char *hex = "0123456789ABCDEF";
    size_t len = strlen(hex);

    size_t size = 100;
    uint8_t buffer[size];
    memset(buffer, '\0', size);

    uint8_t *out = buffer;
    size_t olen = size;

    testrun(!ov_convert_hex_to_binary(NULL, 0, NULL, NULL));
    testrun(!ov_convert_hex_to_binary(NULL, len, &out, &olen));
    testrun(!ov_convert_hex_to_binary((uint8_t *)hex, 0, &out, &olen));
    testrun(!ov_convert_hex_to_binary((uint8_t *)hex, len, NULL, &olen));
    testrun(!ov_convert_hex_to_binary((uint8_t *)hex, len, &out, NULL));

    testrun(ov_convert_hex_to_binary((uint8_t *)hex, len, &out, &olen));
    testrun(buffer[0] == 0x01);
    testrun(buffer[1] == 0x23);
    testrun(buffer[2] == 0x45);
    testrun(buffer[3] == 0x67);
    testrun(buffer[4] == 0x89);
    testrun(buffer[5] == 0xAB);
    testrun(buffer[6] == 0xCD);
    testrun(buffer[7] == 0xEF);
    testrun(buffer[8] == 0x00);
    testrun(olen == 8);

    memset(buffer, 0, size);

    hex = "FACE0110DEAD";
    len = strlen(hex);
    olen = size;
    testrun(ov_convert_hex_to_binary((uint8_t *)hex, len, &out, &olen));
    testrun(buffer[0] == 0xFA);
    testrun(buffer[1] == 0xCE);
    testrun(buffer[2] == 0x01);
    testrun(buffer[3] == 0x10);
    testrun(buffer[4] == 0xDE);
    testrun(buffer[5] == 0xAD);
    testrun(buffer[6] == 0x00);
    testrun(buffer[7] == 0x00);
    testrun(buffer[8] == 0x00);
    testrun(olen == 6);

    // check created
    out = NULL;
    testrun(ov_convert_hex_to_binary((uint8_t *)hex, len, &out, &olen));
    testrun(buffer[0] == 0xFA);
    testrun(buffer[1] == 0xCE);
    testrun(buffer[2] == 0x01);
    testrun(buffer[3] == 0x10);
    testrun(buffer[4] == 0xDE);
    testrun(buffer[5] == 0xAD);
    testrun(buffer[6] == 0x00);
    testrun(buffer[7] == 0x00);
    testrun(buffer[8] == 0x00);
    testrun(olen == 6);
    free(out);

    // check case ignore
    out = buffer;
    memset(buffer, 0, size);
    hex = "face0110dead";
    len = strlen(hex);
    olen = size;
    testrun(ov_convert_hex_to_binary((uint8_t *)hex, len, &out, &olen));
    testrun(buffer[0] == 0xFA);
    testrun(buffer[1] == 0xCE);
    testrun(buffer[2] == 0x01);
    testrun(buffer[3] == 0x10);
    testrun(buffer[4] == 0xDE);
    testrun(buffer[5] == 0xAD);
    testrun(buffer[6] == 0x00);
    testrun(buffer[7] == 0x00);
    testrun(buffer[8] == 0x00);
    testrun(olen == 6);

    // check whitespace ignore
    out = buffer;
    memset(buffer, 0, size);
    hex = " f\na\tc\ve\n0\n1\n1 0 D e a   d\n\n";
    len = strlen(hex);
    olen = size;
    testrun(ov_convert_hex_to_binary((uint8_t *)hex, len, &out, &olen));
    testrun(buffer[0] == 0xFA);
    testrun(buffer[1] == 0xCE);
    testrun(buffer[2] == 0x01);
    testrun(buffer[3] == 0x10);
    testrun(buffer[4] == 0xDE);
    testrun(buffer[5] == 0xAD);
    testrun(buffer[6] == 0x00);
    testrun(buffer[7] == 0x00);
    testrun(buffer[8] == 0x00);
    testrun(olen == 6);

    // check 0x ignore
    out = buffer;
    memset(buffer, 0, size);
    hex = "0xfa0xce0x010x100xde0xad";
    len = strlen(hex);
    olen = size;
    testrun(ov_convert_hex_to_binary((uint8_t *)hex, len, &out, &olen));
    testrun(buffer[0] == 0xFA);
    testrun(buffer[1] == 0xCE);
    testrun(buffer[2] == 0x01);
    testrun(buffer[3] == 0x10);
    testrun(buffer[4] == 0xDE);
    testrun(buffer[5] == 0xAD);
    testrun(buffer[6] == 0x00);
    testrun(buffer[7] == 0x00);
    testrun(buffer[8] == 0x00);
    testrun(olen == 6);

    // check 0x + whitespace + case ignore
    out = buffer;
    memset(buffer, 0, size);
    hex = "0xFa 0xce 0x01 0x10 0xde 0xAD";
    len = strlen(hex);
    olen = size;
    testrun(ov_convert_hex_to_binary((uint8_t *)hex, len, &out, &olen));
    testrun(buffer[0] == 0xFA);
    testrun(buffer[1] == 0xCE);
    testrun(buffer[2] == 0x01);
    testrun(buffer[3] == 0x10);
    testrun(buffer[4] == 0xDE);
    testrun(buffer[5] == 0xAD);
    testrun(buffer[6] == 0x00);
    testrun(buffer[7] == 0x00);
    testrun(buffer[8] == 0x00);
    testrun(olen == 6);

    // check 0x errors
    out = buffer;
    memset(buffer, 0, size);
    hex = "0xFA0xCE";
    len = strlen(hex);
    olen = size;
    testrun(ov_convert_hex_to_binary((uint8_t *)hex, len, &out, &olen));
    testrun(buffer[0] == 0xFA);
    testrun(buffer[1] == 0xCE);
    testrun(buffer[2] == 0x00);
    testrun(olen == 2);

    hex = "0xFA0xC";
    len = strlen(hex);
    olen = size;
    testrun(!ov_convert_hex_to_binary((uint8_t *)hex, len, &out, &olen));

    hex = "0xFA0xc e";
    len = strlen(hex);
    olen = size;
    testrun(!ov_convert_hex_to_binary((uint8_t *)hex, len, &out, &olen));

    hex = "0xFA0x";
    len = strlen(hex);
    olen = size;
    testrun(!ov_convert_hex_to_binary((uint8_t *)hex, len, &out, &olen));

    hex = "0xFA0x0xce";
    len = strlen(hex);
    olen = size;
    testrun(!ov_convert_hex_to_binary((uint8_t *)hex, len, &out, &olen));

    hex = "0xFA0xXY";
    len = strlen(hex);
    olen = size;
    testrun(!ov_convert_hex_to_binary((uint8_t *)hex, len, &out, &olen));

    hex = "FAnohex";
    len = strlen(hex);
    olen = size;
    testrun(!ov_convert_hex_to_binary((uint8_t *)hex, len, &out, &olen));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool dequals(double a, double b, double sigma) {

    fprintf(stderr, "Expect %f    Got %f\n", a, b);

    return fabs(sigma) >= fabs(b - a);
}

int test_ov_convert_from_db() {

    testrun(dequals(1, ov_convert_from_db(0), 0.0001));
    testrun(dequals(2, ov_convert_from_db(3), 0.01));
    testrun(dequals(4, ov_convert_from_db(6), 0.1));
    testrun(dequals(10, ov_convert_from_db(10), 0.01));
    testrun(dequals(100, ov_convert_from_db(20), 0.01));
    testrun(dequals(1000, ov_convert_from_db(30), 0.01));
    testrun(dequals(1.0 / 10.0, ov_convert_from_db(-10), 0.01));
    testrun(dequals(1.0 / 100.0, ov_convert_from_db(-20), 0.01));
    testrun(dequals(1.0 / 1000.0, ov_convert_from_db(-30), 0.01));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_convert_to_db() {

    testrun(dequals(0, ov_convert_to_db(1), 0.0001));
    testrun(dequals(3, ov_convert_to_db(2), 0.1));
    testrun(dequals(6, ov_convert_to_db(4), 0.1));
    testrun(dequals(10, ov_convert_to_db(10), 0.01));
    testrun(dequals(20, ov_convert_to_db(100), 0.01));
    testrun(dequals(30, ov_convert_to_db(1000), 0.01));
    testrun(dequals(-10, ov_convert_to_db(1.0 / 10.0), 0.01));
    testrun(dequals(-20, ov_convert_to_db(1.0 / 100.0), 0.01));
    testrun(dequals(-30, ov_convert_to_db(1.0 / 1000.0), 0.01));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_convert_from_vol_percent() {

    testrun(0 == ov_convert_from_vol_percent(0, 0));

    testrun(100 == ov_convert_from_vol_percent(100, 0));
    testrun(100 == ov_convert_from_vol_percent(200, 0));

    testrun(0 == ov_convert_from_vol_percent(0, 100));

    testrun(100 == ov_convert_from_vol_percent(100, 100));
    testrun(100 == ov_convert_from_vol_percent(200, 100));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_convert_to_vol_percent() {

    testrun(0 == ov_convert_to_vol_percent(0, 0));
    testrun(0 == ov_convert_to_vol_percent(0, 10));

    testrun(100 == ov_convert_to_vol_percent(100, 0));
    testrun(100 == ov_convert_to_vol_percent(200, 0));

    for (uint8_t i = 10; i <= 100; i += 10) {
        fprintf(
            stderr,
            "%" PRIu8 ": %" PRIu8 " => %" PRIu8 "\n",
            i,
            ov_convert_from_vol_percent(i, 0),
            ov_convert_to_vol_percent(ov_convert_from_vol_percent(i, 0), 0));

        testrun(i == ov_convert_to_vol_percent(
                         ov_convert_from_vol_percent(i, 0), 0));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_convert",
            test_ov_convert_msecs_to_samples,
            test_ov_convert_samples_to_msecs,
            test_ov_convert_string_to_uint64,
            test_ov_convert_string_to_int64,
            test_ov_convert_string_to_double,
            test_ov_convert_double_to_string,
            test_ov_convert_int64_to_string,
            test_ov_convert_uint64_to_string,
            test_ov_convert_hex_string_to_uint64,
            test_ov_convert_binary_to_hex,
            test_ov_convert_hex_to_binary,
            test_ov_convert_from_db,
            test_ov_convert_to_db,
            test_ov_convert_from_vol_percent,
            test_ov_convert_to_vol_percent);

/*----------------------------------------------------------------------------*/
