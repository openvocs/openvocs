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
        @file           ov_sdp_grammar_test.c
        @author         Markus Toepfer

        @date           2018-04-12

        @ingroup        ov_sdp

        @brief          Uni tests for ov_sdp_grammar


        ------------------------------------------------------------------------
*/

#include "ov_sdp_grammar.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_text() {

    char buffer[100] = {0};

    strcat(buffer, "any utf8 \n \t text");

    testrun(!ov_sdp_is_text(NULL, 0));
    testrun(!ov_sdp_is_text(buffer, 0));
    testrun(!ov_sdp_is_text(NULL, 10));

    testrun(ov_sdp_is_text(buffer, strlen(buffer)));

    // content not UTF_8
    buffer[7] = 0xFF;

    testrun(!ov_sdp_is_text(buffer, strlen(buffer)));
    testrun(!ov_sdp_is_text(buffer, 8));
    testrun(ov_sdp_is_text(buffer, 7));

    buffer[7] = ' ';
    testrun(ov_sdp_is_text(buffer, 8));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_username() {

    char buffer[100] = {0};

    strcat(buffer, "nospaces");

    testrun(!ov_sdp_is_username(NULL, 0));
    testrun(!ov_sdp_is_username(buffer, 0));
    testrun(!ov_sdp_is_username(NULL, 8));

    testrun(ov_sdp_is_username(buffer, strlen(buffer)));

    // content with whitespace
    buffer[3] = ' ';

    testrun(!ov_sdp_is_username(buffer, strlen(buffer)));
    testrun(!ov_sdp_is_username(buffer, 4));
    testrun(ov_sdp_is_username(buffer, 3));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_key() {

    char buffer[100] = {0};
    strcat(buffer, "prompt");

    testrun(!ov_sdp_is_key(NULL, 0));
    testrun(!ov_sdp_is_key(buffer, 0));
    testrun(!ov_sdp_is_key(NULL, 5));

    testrun(ov_sdp_is_key(buffer, strlen(buffer)));

    strcat(buffer, "wrong");
    testrun(!ov_sdp_is_key(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "clear:text");
    testrun(ov_sdp_is_key(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "base64:Zm9vYmFy");
    testrun(ov_sdp_is_base64("Zm9vYmFy", strlen("Zm9vYmFy")));
    testrun(ov_sdp_is_key(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "base64:notbase64");
    testrun(!ov_sdp_is_key(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "uri:file:path");
    testrun(ov_sdp_is_uri("file:path", strlen("file:path")));
    testrun(ov_sdp_is_key(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "uri:@not_invalid");
    testrun(ov_sdp_is_uri("@not_invalid", strlen("@not_invalid")));
    testrun(ov_sdp_is_key(buffer, strlen(buffer)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_base64() {

    char buffer[100] = {0};
    strcat(buffer, "Zm9vYmFy");

    testrun(!ov_sdp_is_base64(NULL, 0));
    testrun(!ov_sdp_is_base64(buffer, 0));
    testrun(!ov_sdp_is_base64(NULL, 5));

    testrun(ov_sdp_is_base64(buffer, strlen(buffer)));

    strcat(buffer, "wrong");
    testrun(!ov_sdp_is_base64(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "Zm9vYmFyx");
    testrun(!ov_sdp_is_base64(buffer, strlen(buffer)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_check_base64() {

    char buffer[100] = {0};
    strcat(buffer, "base64:Zm9vYmFy");

    testrun(!ov_sdp_check_base64(NULL, 0));
    testrun(!ov_sdp_check_base64(buffer, 0));
    testrun(!ov_sdp_check_base64(NULL, 5));

    testrun(ov_sdp_check_base64(buffer, strlen(buffer)));

    strcat(buffer, "wrong");
    testrun(!ov_sdp_check_base64(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "base64:Zm9vYmFyx");
    testrun(!ov_sdp_check_base64(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "base64:");
    testrun(!ov_sdp_check_base64(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "Base64:Zm9vYmFy");
    testrun(!ov_sdp_check_base64(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "base6:Zm9vYmFy");
    testrun(!ov_sdp_check_base64(buffer, strlen(buffer)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_check_clear() {

    char buffer[100] = {0};
    strcat(buffer, "clear:text");

    testrun(!ov_sdp_check_clear(NULL, 0));
    testrun(!ov_sdp_check_clear(buffer, 0));
    testrun(!ov_sdp_check_clear(NULL, 5));

    testrun(ov_sdp_check_clear(buffer, strlen(buffer)));

    buffer[7] = 0xff;
    testrun(!ov_sdp_check_clear(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "clear:");
    testrun(!ov_sdp_check_clear(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "cleAr:text");
    testrun(!ov_sdp_check_clear(buffer, strlen(buffer)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_check_uri() {

    char buffer[100] = {0};
    strcat(buffer, "uri:file:path");

    testrun(!ov_sdp_check_uri(NULL, 0));
    testrun(!ov_sdp_check_uri(buffer, 0));
    testrun(!ov_sdp_check_uri(NULL, 5));

    testrun(ov_sdp_check_uri(buffer, strlen(buffer)));

    buffer[7] = 0xff;
    testrun(!ov_sdp_check_uri(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "uri:");
    testrun(!ov_sdp_check_uri(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "Uri:file:path");
    testrun(!ov_sdp_check_uri(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "uri:file@not_invalid");
    testrun(ov_sdp_check_uri(buffer, strlen(buffer)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_uri() {

    char buffer[100] = {0};
    strcat(buffer, "file:path");

    testrun(!ov_sdp_is_uri(NULL, 0));
    testrun(!ov_sdp_is_uri(buffer, 0));
    testrun(!ov_sdp_is_uri(NULL, 5));

    testrun(ov_sdp_is_uri(buffer, strlen(buffer)));

    buffer[7] = 0xff;
    testrun(!ov_sdp_is_uri(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "file:path");
    testrun(ov_sdp_is_uri(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "file//@:1234567");
    testrun(!ov_sdp_is_uri(buffer, strlen(buffer)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_digit() {

    char buffer[100] = {0};

    strcat(buffer, "01234x");

    testrun(!ov_sdp_is_digit(NULL, 0));
    testrun(!ov_sdp_is_digit(buffer, 0));
    testrun(!ov_sdp_is_digit(NULL, 10));

    testrun(ov_sdp_is_digit(buffer, 5));
    testrun(!ov_sdp_is_digit(buffer, 6));

    buffer[0] = '-';
    testrun(!ov_sdp_is_digit(buffer, 3));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_integer() {

    char buffer[100] = {0};

    strcat(buffer, "1234x");

    testrun(!ov_sdp_is_integer(NULL, 0));
    testrun(!ov_sdp_is_integer(buffer, 0));
    testrun(!ov_sdp_is_integer(NULL, 10));

    testrun(ov_sdp_is_integer(buffer, 4));
    testrun(!ov_sdp_is_integer(buffer, 5));

    buffer[0] = '0';
    testrun(!ov_sdp_is_integer(buffer, 4));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_port() {

    char buffer[100] = {0};

    strcat(buffer, "65535");

    testrun(!ov_sdp_is_port(NULL, 0));
    testrun(!ov_sdp_is_port(buffer, 0));
    testrun(!ov_sdp_is_port(NULL, 5));

    testrun(ov_sdp_is_port(buffer, 4));
    testrun(ov_sdp_is_port(buffer, 5));
    testrun(!ov_sdp_is_port(buffer, 6));

    buffer[4] = '6';
    testrun(!ov_sdp_is_port(buffer, 5));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/
/*
int test_ov_sdp_is_repeat_time() {

        char buffer[100] = { 0 };

        strcat(buffer, "65535");

        testrun(!ov_sdp_is_repeat_time(NULL,   0));
        testrun(!ov_sdp_is_repeat_time(buffer, 0));
        testrun(!ov_sdp_is_repeat_time(NULL,   5));

        testrun( ov_sdp_is_repeat_time(buffer, 4));
        testrun( ov_sdp_is_repeat_time(buffer, 5));
        testrun(!ov_sdp_is_repeat_time(buffer, 6));

        buffer[4] = '6';
        testrun(ov_sdp_is_repeat_time(buffer, 5));
        buffer[0] = '-';
        testrun(!ov_sdp_is_repeat_time(buffer, 5));
        buffer[0] = '0';
        testrun(!ov_sdp_is_repeat_time(buffer, 5));
        buffer[0] = '1';
        testrun(ov_sdp_is_repeat_time(buffer, 5));

        buffer[4] = 'd';
        testrun(ov_sdp_is_repeat_time(buffer, 5));
        buffer[4] = 'h';
        testrun(ov_sdp_is_repeat_time(buffer, 5));
        buffer[4] = 'm';
        testrun(ov_sdp_is_repeat_time(buffer, 5));
        buffer[4] = 's';
        testrun(ov_sdp_is_repeat_time(buffer, 5));
        buffer[4] = 'x';
        testrun(!ov_sdp_is_repeat_time(buffer, 5));

        return testrun_log_success();
}
*/

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_typed_time() {

    char buffer[100] = {0};
    char *string = NULL;

    strcat(buffer, "65535");

    testrun(!ov_sdp_is_typed_time(NULL, 0, false));
    testrun(!ov_sdp_is_typed_time(buffer, 0, false));
    testrun(!ov_sdp_is_typed_time(NULL, 5, false));

    testrun(ov_sdp_is_typed_time(buffer, 4, false));
    testrun(ov_sdp_is_typed_time(buffer, 5, false));
    testrun(!ov_sdp_is_typed_time(buffer, 6, false));

    buffer[4] = '6';
    testrun(ov_sdp_is_typed_time(buffer, 5, false));
    buffer[0] = '-';
    testrun(!ov_sdp_is_typed_time(buffer, 5, false));
    buffer[0] = '0';
    testrun(!ov_sdp_is_typed_time(buffer, 5, false));
    buffer[0] = '1';
    testrun(ov_sdp_is_typed_time(buffer, 5, false));

    buffer[4] = 'd';
    testrun(ov_sdp_is_typed_time(buffer, 5, false));
    buffer[4] = 'h';
    testrun(ov_sdp_is_typed_time(buffer, 5, false));
    buffer[4] = 'm';
    testrun(ov_sdp_is_typed_time(buffer, 5, false));
    buffer[4] = 's';
    testrun(ov_sdp_is_typed_time(buffer, 5, false));
    buffer[4] = 'x';
    testrun(!ov_sdp_is_typed_time(buffer, 5, false));

    string = "-1d";
    testrun(!ov_sdp_is_typed_time(string, strlen(string), false));
    string = "-112313132s";
    testrun(ov_sdp_is_typed_time(string, strlen(string), true));

    return testrun_log_success();
}
/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_time() {

    char buffer[100] = {0};

    strcat(buffer, "12345678901234567890");

    testrun(!ov_sdp_is_time(NULL, 0));
    testrun(!ov_sdp_is_time(buffer, 0));
    testrun(!ov_sdp_is_time(NULL, 5));

    testrun(ov_sdp_is_time(buffer, 10));
    testrun(ov_sdp_is_time(buffer, 15));
    testrun(!ov_sdp_is_time(buffer, 9));

    // check special case 0
    testrun(ov_sdp_is_time("0", 1));

    testrun(ov_sdp_is_time(buffer, 20));

    // check pos start
    buffer[0] = '-';
    testrun(!ov_sdp_is_time(buffer, 15));
    buffer[0] = '0';
    testrun(!ov_sdp_is_time(buffer, 15));
    buffer[0] = '1';
    testrun(ov_sdp_is_time(buffer, 15));

    buffer[4] = 'd';
    testrun(!ov_sdp_is_time(buffer, 15));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_token() {

    char buffer[100] = {0};
    size_t len = 0;

    strcat(buffer, "some_valid_123_token");

    testrun(!ov_sdp_is_token(NULL, 0));
    testrun(ov_sdp_is_token(buffer, strlen(buffer)));

    len = strlen(buffer);

    for (int i = 0; i < 0xff; i++) {

        buffer[1] = i;

        if (i >= 0x5E) {
            if (i <= 0x7E) {
                testrun(ov_sdp_is_token(buffer, len));
            } else {
                testrun(!ov_sdp_is_token(buffer, len));
            }
        } else if (i >= 0x41) {
            if (i <= 0x5A) {
                testrun(ov_sdp_is_token(buffer, len));
            } else {
                testrun(!ov_sdp_is_token(buffer, len));
            }
        } else if (i >= 0x30) {
            if (i <= 0x39) {
                testrun(ov_sdp_is_token(buffer, len));
            } else {
                testrun(!ov_sdp_is_token(buffer, len));
            }
        } else if (i >= 0x2D) {
            if (i <= 0x2E) {
                testrun(ov_sdp_is_token(buffer, len));
            } else {
                testrun(!ov_sdp_is_token(buffer, len));
            }
        } else if (i >= 0x2A) {
            if (i <= 0x2B) {
                testrun(ov_sdp_is_token(buffer, len));
            } else {
                testrun(!ov_sdp_is_token(buffer, len));
            }
        } else if (i >= 0x23) {
            if (i <= 0x27) {
                testrun(ov_sdp_is_token(buffer, len));
            } else {
                testrun(!ov_sdp_is_token(buffer, len));
            }
        } else if (i == 0x21) {
            testrun(ov_sdp_is_token(buffer, len));
        } else {
            testrun(!ov_sdp_is_token(buffer, len));
        }
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_address() {

    char buffer[100];
    memset(buffer, 0, 100);

    strcat(buffer, "some_address");

    testrun(!ov_sdp_is_address(NULL, 0));
    testrun(!ov_sdp_is_address(buffer, 0));
    testrun(!ov_sdp_is_address(NULL, strlen(buffer)));

    for (size_t i = 0; i <= 0xff; i++) {

        buffer[0] = i;

        if (i >= 0x7f) {

            testrun(!ov_sdp_is_address(buffer, strlen(buffer)));

        } else if (i < 0x21) {

            testrun(!ov_sdp_is_address(buffer, strlen(buffer)));

        } else {
            testrun(ov_sdp_is_address(buffer, strlen(buffer)));
        }
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_proto() {

    char *buffer = "PROTO123";

    testrun(!ov_sdp_is_proto(NULL, 0));
    testrun(!ov_sdp_is_proto(buffer, 0));
    testrun(!ov_sdp_is_proto(NULL, 5));

    testrun(ov_sdp_is_proto(buffer, strlen(buffer)));

    buffer = "/PROTO123";
    testrun(!ov_sdp_is_proto(buffer, strlen(buffer)));

    buffer = "PROTO123/";
    testrun(!ov_sdp_is_proto(buffer, strlen(buffer)));

    buffer = "P/R/O/T/O/1/2/3";
    testrun(ov_sdp_is_proto(buffer, strlen(buffer)));
    testrun(!ov_sdp_is_proto(buffer, strlen(buffer) + 1));
    testrun(!ov_sdp_is_proto(buffer, strlen(buffer) + 2));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_byte_string() {

    char buffer[100] = {0};
    strcat(buffer, "test");

    testrun(!ov_sdp_is_byte_string(NULL, 0));
    testrun(!ov_sdp_is_byte_string(buffer, 0));
    testrun(!ov_sdp_is_byte_string(NULL, 4));

    testrun(ov_sdp_is_byte_string(buffer, strlen(buffer)));

    for (size_t i = 0; i <= 0xff; i++) {

        buffer[1] = i;
        switch (i) {

        case 0x00:
        case '\r':
        case '\n':
            testrun(!ov_sdp_is_byte_string(buffer, 4));
            break;
        default:
            testrun(ov_sdp_is_byte_string(buffer, 4));
            break;
        }
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_verify_phone_buffer() {

    char buffer[100] = {0};
    strcat(buffer, "+12");

    testrun(!ov_sdp_verify_phone_buffer(NULL, 0));
    testrun(!ov_sdp_verify_phone_buffer(buffer, 0));
    testrun(!ov_sdp_verify_phone_buffer(NULL, 4));

    testrun(ov_sdp_verify_phone_buffer(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "+12 ");
    testrun(ov_sdp_verify_phone_buffer(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "+12-");
    testrun(ov_sdp_verify_phone_buffer(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "+1-");
    testrun(ov_sdp_verify_phone_buffer(buffer, strlen(buffer)));

    buffer[3] = ' ';
    testrun(ov_sdp_verify_phone_buffer(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "+test");
    testrun(!ov_sdp_verify_phone_buffer(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "1 2     3   2332 -    121212     ");
    testrun(ov_sdp_verify_phone_buffer(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "+1 2  -- 3 -2332 -    121-2  -");
    testrun(ov_sdp_verify_phone_buffer(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "+ 2  -- 3 -2332 -    121-2  -");
    testrun(!ov_sdp_verify_phone_buffer(buffer, strlen(buffer)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_email_safe() {

    char buffer[100] = {0};
    strcat(buffer, "test");

    testrun(!ov_sdp_is_email_safe(NULL, 0));
    testrun(!ov_sdp_is_email_safe(buffer, 0));
    testrun(!ov_sdp_is_email_safe(NULL, 4));

    testrun(ov_sdp_is_email_safe(buffer, strlen(buffer)));

    for (size_t i = 0; i <= 0xff; i++) {

        buffer[1] = i;
        switch (i) {

        case 0x00:
        case 0x0A:
        case 0x0D:
        case 0x28:
        case 0x29:
        case 0x3C:
        case 0x3E:
            testrun(!ov_sdp_is_email_safe(buffer, 4));
            break;
        default:
            testrun(ov_sdp_is_email_safe(buffer, 4));
            break;
        }
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_phone() {

    char buffer[100] = {0};
    strcat(buffer, "+12");

    testrun(!ov_sdp_is_phone(NULL, 0));
    testrun(!ov_sdp_is_phone(buffer, 0));
    testrun(!ov_sdp_is_phone(NULL, 4));

    testrun(ov_sdp_is_phone(buffer, strlen(buffer)));

    // phone only
    memset(buffer, 0, 100);
    strcat(buffer, "+12 ");
    testrun(ov_sdp_is_phone(buffer, strlen(buffer)));

    memset(buffer, 0, 100);
    strcat(buffer, "+2  -- 3 -2332 -    121-2  -");
    testrun(ov_sdp_is_phone(buffer, strlen(buffer)));

    // phone only wrong
    memset(buffer, 0, 100);
    strcat(buffer, " 123");
    testrun(!ov_sdp_is_phone(buffer, strlen(buffer)));

    // with ()
    memset(buffer, 0, 100);
    strcat(buffer, "+123(safe");
    testrun(!ov_sdp_is_phone(buffer, strlen(buffer)));
    buffer[strlen(buffer) - 1] = ')';
    testrun(ov_sdp_is_phone(buffer, strlen(buffer)));
    buffer[4] = '4';
    testrun(!ov_sdp_is_phone(buffer, strlen(buffer)));

    // with <>
    memset(buffer, 0, 100);
    strcat(buffer, "safe<1234");
    testrun(!ov_sdp_is_phone(buffer, strlen(buffer)));
    buffer[strlen(buffer) - 1] = '>';
    testrun(ov_sdp_is_phone(buffer, strlen(buffer)));
    buffer[4] = '4';
    testrun(!ov_sdp_is_phone(buffer, strlen(buffer)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_addr_spec() {

    char *buffer = "test@mail";

    testrun(!ov_sdp_is_addr_spec(NULL, 0));
    testrun(!ov_sdp_is_addr_spec(buffer, 0));
    testrun(!ov_sdp_is_addr_spec(NULL, 9));

    testrun(ov_sdp_is_addr_spec(buffer, strlen(buffer)));

    buffer = "test@";
    testrun(!ov_sdp_is_addr_spec(buffer, strlen(buffer)));

    buffer = "@test";
    testrun(!ov_sdp_is_addr_spec(buffer, strlen(buffer)));

    buffer = "a@b";
    testrun(ov_sdp_is_addr_spec(buffer, strlen(buffer)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_email() {

    char buffer[100] = {0};
    strcat(buffer, "e@mail");

    testrun(!ov_sdp_is_email(NULL, 0));
    testrun(!ov_sdp_is_email(buffer, 0));
    testrun(!ov_sdp_is_email(NULL, 6));

    testrun(ov_sdp_is_email(buffer, strlen(buffer)));

    // email only
    memset(buffer, 0, 100);
    strcat(buffer, "e@mail");
    testrun(ov_sdp_is_email(buffer, strlen(buffer)));

    // mail only wrong
    memset(buffer, 0, 100);
    strcat(buffer, "@mail");
    testrun(!ov_sdp_is_email(buffer, strlen(buffer)));

    // with ()
    memset(buffer, 0, 100);
    strcat(buffer, "e@mail (name");
    testrun(!ov_sdp_is_email(buffer, strlen(buffer)));
    buffer[strlen(buffer) - 1] = ')';
    testrun(ov_sdp_is_email(buffer, strlen(buffer)));
    buffer[1] = '4';
    testrun(!ov_sdp_is_email(buffer, strlen(buffer)));

    // single space
    memset(buffer, 0, 100);
    strcat(buffer, "e@mail(name)");
    testrun(!ov_sdp_is_email(buffer, strlen(buffer)));
    buffer[5] = ' ';
    testrun(ov_sdp_is_email(buffer, strlen(buffer)));

    // with <>
    memset(buffer, 0, 100);
    strcat(buffer, "name <e@mail");
    testrun(!ov_sdp_is_email(buffer, strlen(buffer)));
    buffer[strlen(buffer) - 1] = '>';
    testrun(ov_sdp_is_email(buffer, strlen(buffer)));
    buffer[4] = '4';
    testrun(!ov_sdp_is_email(buffer, strlen(buffer)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_ip6() {

    char *buffer = "::";

    testrun(!ov_sdp_is_ip6(NULL, 0));
    testrun(!ov_sdp_is_ip6(buffer, 0));
    testrun(!ov_sdp_is_ip6(NULL, 1));

    testrun(ov_sdp_is_ip6(buffer, strlen(buffer)));

    buffer = "::1";
    testrun(ov_sdp_is_ip6(buffer, strlen(buffer)));

    buffer = "::FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF";
    testrun(ov_sdp_is_ip6(buffer, strlen(buffer)));

    buffer = "::FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF";
    testrun(ov_sdp_is_ip6(buffer, strlen(buffer)));

    buffer = "::FFFF";
    testrun(ov_sdp_is_ip6(buffer, strlen(buffer)));

    // some negative tests
    buffer = "::::1";
    testrun(!ov_sdp_is_ip6(buffer, strlen(buffer)));

    buffer = "::FFFF:FFFFF";
    testrun(!ov_sdp_is_ip6(buffer, strlen(buffer)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_ip4() {

    char *buffer = "1.1.1.1";

    testrun(!ov_sdp_is_ip4(NULL, 0));
    testrun(!ov_sdp_is_ip4(buffer, 0));
    testrun(!ov_sdp_is_ip4(NULL, 1));

    testrun(ov_sdp_is_ip4(buffer, strlen(buffer)));

    buffer = "127.0.0.1";
    testrun(ov_sdp_is_ip4(buffer, strlen(buffer)));

    buffer = "255.255.255.255";
    testrun(ov_sdp_is_ip4(buffer, strlen(buffer)));

    buffer = "129.123.1.12";
    testrun(ov_sdp_is_ip4(buffer, strlen(buffer)));

    buffer = "256.233.233.233";
    testrun(!ov_sdp_is_ip4(buffer, strlen(buffer)));

    // some negative tests
    buffer = "200.100.1";
    testrun(!ov_sdp_is_ip4(buffer, strlen(buffer)));

    buffer = "1.2.3.";
    testrun(!ov_sdp_is_ip4(buffer, strlen(buffer)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_ip6_multicast() {

    char *buffer = "FF::1";

    testrun(!ov_sdp_is_ip6_multicast(NULL, 0));
    testrun(!ov_sdp_is_ip6_multicast(buffer, 0));
    testrun(!ov_sdp_is_ip6_multicast(NULL, 1));

    testrun(ov_sdp_is_ip6_multicast(buffer, strlen(buffer)));

    buffer = "FF::1/12345";
    testrun(ov_sdp_is_ip6_multicast(buffer, strlen(buffer)));

    buffer = "FF::ffff/";
    testrun(!ov_sdp_is_ip6_multicast(buffer, strlen(buffer)));

    buffer = "FF::1/100000";
    testrun(!ov_sdp_is_ip6_multicast(buffer, strlen(buffer)));

    buffer = "FF::1/0";
    testrun(!ov_sdp_is_ip6_multicast(buffer, strlen(buffer)));

    buffer = "ff::1/1";
    testrun(ov_sdp_is_ip6_multicast(buffer, strlen(buffer)));

    buffer = "fF::1/65535";
    testrun(ov_sdp_is_ip6_multicast(buffer, strlen(buffer)));

    buffer = "ff::1/65536";
    testrun(!ov_sdp_is_ip6_multicast(buffer, strlen(buffer)));

    // no FF start
    buffer = "fa::1/1";
    testrun(!ov_sdp_is_ip6_multicast(buffer, strlen(buffer)));

    buffer = "af::1/1";
    testrun(!ov_sdp_is_ip6_multicast(buffer, strlen(buffer)));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_ip4_multicast() {

    char *string = 0;
    char buffer[100];
    memset(buffer, 0, 100);
    strcat(buffer, "230.0.0.0/1");

    testrun(!ov_sdp_is_ip4_multicast(NULL, 0));
    testrun(!ov_sdp_is_ip4_multicast(buffer, 0));
    testrun(!ov_sdp_is_ip4_multicast(NULL, 1));

    testrun(ov_sdp_is_ip4_multicast(buffer, strlen(buffer)));

    for (size_t i = 1; i < 0xff; i++) {

        buffer[0] = i;
        buffer[1] = '3';
        buffer[2] = '5';
        buffer[3] = '.';
        buffer[4] = '0';

        if (i == 0x32) {
            testrun(ov_sdp_is_ip4_multicast(buffer, strlen(buffer)));
        } else {
            testrun(!ov_sdp_is_ip4_multicast(buffer, strlen(buffer)));
        }

        buffer[0] = '2';
        buffer[1] = i;

        switch (i) {

        case '2':
        case '3':
            testrun(ov_sdp_is_ip4_multicast(buffer, strlen(buffer)));
            break;

        default:
            testrun(!ov_sdp_is_ip4_multicast(buffer, strlen(buffer)));
        }

        buffer[0] = '2';
        buffer[1] = '2';
        buffer[2] = i;

        switch (i) {

        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            testrun(ov_sdp_is_ip4_multicast(buffer, strlen(buffer)));
            break;

        default:
            testrun(!ov_sdp_is_ip4_multicast(buffer, strlen(buffer)));
        }

        buffer[0] = '2';
        buffer[1] = '3';
        buffer[2] = i;

        switch (i) {

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            testrun(ov_sdp_is_ip4_multicast(buffer, strlen(buffer)));
            break;

        default:
            testrun(!ov_sdp_is_ip4_multicast(buffer, strlen(buffer)));
        }

        buffer[0] = '2';
        buffer[1] = '3';
        buffer[2] = '0';
        buffer[3] = i;

        if (i == '.') {
            testrun(ov_sdp_is_ip4_multicast(buffer, strlen(buffer)));
        } else {
            testrun(!ov_sdp_is_ip4_multicast(buffer, strlen(buffer)));
        }

        buffer[0] = '2';
        buffer[1] = '3';
        buffer[2] = '0';
        buffer[3] = '.';
        buffer[4] = i;

        if ((i < 0x30) || (i > 0x39)) {
            testrun(!ov_sdp_is_ip4_multicast(buffer, strlen(buffer)));
        } else {
            testrun(ov_sdp_is_ip4_multicast(buffer, strlen(buffer)));
        }
    }

    // no ttl
    string = "230.0.0.0";
    testrun(!ov_sdp_is_ip4_multicast(string, strlen(string)));
    string = "230.0.0.1";
    testrun(!ov_sdp_is_ip4_multicast(string, strlen(string)));
    string = "230.0.0.1/";
    testrun(!ov_sdp_is_ip4_multicast(string, strlen(string)));

    string = "230.0.0.1/0";
    testrun(ov_sdp_is_ip4_multicast(string, strlen(string)));

    // ttl + port
    string = "230.0.0.1/0/12345";
    testrun(ov_sdp_is_ip4_multicast(string, strlen(string)));

    // ttl to long
    string = "230.0.0.1/1234/12345";
    testrun(!ov_sdp_is_ip4_multicast(string, strlen(string)));

    // ttl 3 digits
    string = "230.0.0.1/123/12345";
    testrun(ov_sdp_is_ip4_multicast(string, strlen(string)));

    // ttl 3 digits
    string = "230.0.0.1/999/12345";
    testrun(ov_sdp_is_ip4_multicast(string, strlen(string)));

    // port > 65535
    string = "230.0.0.1/999/65536";
    testrun(!ov_sdp_is_ip4_multicast(string, strlen(string)));

    string = "230.0.0.1/999/65535";
    testrun(ov_sdp_is_ip4_multicast(string, strlen(string)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_fqdn() {

    char buffer[100];
    memset(buffer, 0, 100);
    strcat(buffer, "abcd");

    testrun(!ov_sdp_is_fqdn(NULL, 0));
    testrun(!ov_sdp_is_fqdn(buffer, 0));
    testrun(!ov_sdp_is_fqdn(NULL, 1));

    testrun(ov_sdp_is_fqdn(buffer, strlen(buffer)));

    for (size_t i = 1; i < 0xff; i++) {

        buffer[0] = i;

        if (!isalnum(i)) {

            if (i == '-') {

                testrun(ov_sdp_is_fqdn(buffer, strlen(buffer)));

            } else if (i == '.') {

                testrun(ov_sdp_is_fqdn(buffer, strlen(buffer)));

            } else {

                testrun(!ov_sdp_is_fqdn(buffer, strlen(buffer)));
            }

        } else {

            testrun(ov_sdp_is_fqdn(buffer, strlen(buffer)));
        }
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_extn_addr() {

    char *buffer = "some_address";

    testrun(!ov_sdp_is_extn_addr(NULL, 0));
    testrun(!ov_sdp_is_extn_addr(buffer, 0));
    testrun(!ov_sdp_is_extn_addr(NULL, 5));

    testrun(ov_sdp_is_extn_addr(buffer, strlen(buffer)));

    // non valid ip
    buffer = "256.256.256.256";
    testrun(ov_sdp_is_extn_addr(buffer, strlen(buffer)));

    // valid ip4
    buffer = "1.1.1.1";
    testrun(ov_sdp_is_extn_addr(buffer, strlen(buffer)));

    // non valid ip6
    buffer = "::::1";
    testrun(ov_sdp_is_extn_addr(buffer, strlen(buffer)));

    // valid ip6
    buffer = "::1";
    testrun(ov_sdp_is_extn_addr(buffer, strlen(buffer)));

    // number
    buffer = "1";
    testrun(ov_sdp_is_extn_addr(buffer, strlen(buffer)));

    // string
    buffer = "host";
    testrun(ov_sdp_is_extn_addr(buffer, strlen(buffer)));

    buffer = "test.org";
    testrun(ov_sdp_is_extn_addr(buffer, strlen(buffer)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_multicast_address() {

    char *buffer = "some_address";

    testrun(!ov_sdp_is_multicast_address(NULL, 0));
    testrun(!ov_sdp_is_multicast_address(buffer, 0));
    testrun(!ov_sdp_is_multicast_address(NULL, 5));

    testrun(ov_sdp_is_multicast_address(buffer, strlen(buffer)));

    // non valid ip
    buffer = "256.256.256.256";
    testrun(ov_sdp_is_multicast_address(buffer, strlen(buffer)));

    // valid ip4
    buffer = "1.1.1.1";
    testrun(ov_sdp_is_multicast_address(buffer, strlen(buffer)));

    // non valid ip6
    buffer = "::::1";
    testrun(ov_sdp_is_multicast_address(buffer, strlen(buffer)));

    // valid ip6
    buffer = "::1";
    testrun(ov_sdp_is_multicast_address(buffer, strlen(buffer)));

    // number
    buffer = "1";
    testrun(ov_sdp_is_multicast_address(buffer, strlen(buffer)));

    // string
    buffer = "host";
    testrun(ov_sdp_is_multicast_address(buffer, strlen(buffer)));

    buffer = "test.org";
    testrun(ov_sdp_is_multicast_address(buffer, strlen(buffer)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_is_unicast_address() {

    char *buffer = "some_address";

    testrun(!ov_sdp_is_unicast_address(NULL, 0));
    testrun(!ov_sdp_is_unicast_address(buffer, 0));
    testrun(!ov_sdp_is_unicast_address(NULL, 5));

    testrun(ov_sdp_is_unicast_address(buffer, strlen(buffer)));

    // non valid ip
    buffer = "256.256.256.256";
    testrun(ov_sdp_is_unicast_address(buffer, strlen(buffer)));

    // valid ip4
    buffer = "1.1.1.1";
    testrun(ov_sdp_is_unicast_address(buffer, strlen(buffer)));

    // non valid ip6
    buffer = "::::1";
    testrun(ov_sdp_is_unicast_address(buffer, strlen(buffer)));

    // valid ip6
    buffer = "::1";
    testrun(ov_sdp_is_unicast_address(buffer, strlen(buffer)));

    // number
    buffer = "1";
    testrun(ov_sdp_is_unicast_address(buffer, strlen(buffer)));

    // string
    buffer = "host";
    testrun(ov_sdp_is_unicast_address(buffer, strlen(buffer)));

    buffer = "test.org";
    testrun(ov_sdp_is_unicast_address(buffer, strlen(buffer)));

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
    testrun_test(test_ov_sdp_is_text);
    testrun_test(test_ov_sdp_is_username);
    testrun_test(test_ov_sdp_is_key);
    testrun_test(test_ov_sdp_is_base64);
    testrun_test(test_ov_sdp_check_base64);
    testrun_test(test_ov_sdp_check_clear);
    testrun_test(test_ov_sdp_check_uri);
    testrun_test(test_ov_sdp_is_uri);
    testrun_test(test_ov_sdp_is_digit);
    testrun_test(test_ov_sdp_is_integer);
    testrun_test(test_ov_sdp_is_port);
    testrun_test(test_ov_sdp_is_typed_time);
    testrun_test(test_ov_sdp_is_time);
    testrun_test(test_ov_sdp_is_token);
    testrun_test(test_ov_sdp_is_address);
    testrun_test(test_ov_sdp_is_proto);
    testrun_test(test_ov_sdp_is_byte_string);
    testrun_test(test_ov_sdp_verify_phone_buffer);
    testrun_test(test_ov_sdp_is_email_safe);
    testrun_test(test_ov_sdp_is_phone);
    testrun_test(test_ov_sdp_is_addr_spec);
    testrun_test(test_ov_sdp_is_email);
    testrun_test(test_ov_sdp_is_ip6);
    testrun_test(test_ov_sdp_is_ip4);
    testrun_test(test_ov_sdp_is_ip6_multicast);
    testrun_test(test_ov_sdp_is_ip4_multicast);
    testrun_test(test_ov_sdp_is_fqdn);
    testrun_test(test_ov_sdp_is_extn_addr);
    testrun_test(test_ov_sdp_is_multicast_address);
    testrun_test(test_ov_sdp_is_unicast_address);

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
