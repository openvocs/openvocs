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
        @file           ov_json_grammar_test.c
        @author         Markus Toepfer

        @date           2018-12-03

        @ingroup        ov_json_grammar

        @brief          Unit tests of json grammar


        ------------------------------------------------------------------------
*/
#include "ov_json_grammar.c"
#include <ov_test/testrun.h>

#include <ctype.h>
/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_json_is_whitespace() {

    for (int i = 0; i <= 0xff; i++) {

        switch (i) {
            case 0x20:
            case 0x09:
            case 0x0A:
            case 0x0D:
                testrun(ov_json_is_whitespace(i));
                break;
            default:
                testrun(!ov_json_is_whitespace(i));
        }
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_clear_whitespace() {

    size_t size = 100;
    uint8_t buffer[size];
    memset(buffer, '\0', size);

    uint8_t *ptr = buffer;
    size_t len = size;

    memcpy(buffer, "test", 4);

    testrun(!ov_json_clear_whitespace(NULL, NULL));
    testrun(!ov_json_clear_whitespace(&ptr, NULL));
    testrun(!ov_json_clear_whitespace(NULL, &len));

    testrun(ptr == buffer);
    testrun(len == size);

    testrun(ov_json_clear_whitespace(&ptr, &len));

    testrun(ptr == buffer);
    testrun(len == size);

    len = 1;
    testrun(ov_json_clear_whitespace(&ptr, &len));
    testrun(ptr == buffer);
    testrun(len == 1);

    len = 0;
    testrun(ov_json_clear_whitespace(&ptr, &len));
    testrun(ptr == buffer);
    testrun(len == 0);

    memcpy(buffer, " test", 5);
    len = 5;
    ptr = buffer;
    testrun(ov_json_clear_whitespace(&ptr, &len));
    testrun(ptr == buffer + 1);
    testrun(len == 4);

    memcpy(buffer, "  test", 6);
    len = 6;
    ptr = buffer;
    testrun(ov_json_clear_whitespace(&ptr, &len));
    testrun(ptr == buffer + 2);
    testrun(len == 4);

    memcpy(buffer, " \t\n\rtest", 8);
    len = 8;
    ptr = buffer;
    testrun(ov_json_clear_whitespace(&ptr, &len));
    testrun(ptr == buffer + 4);
    testrun(len == 4);

    // \v is not a JSON whitespace
    memcpy(buffer, " \v\n\rtest", 8);
    len = 8;
    ptr = buffer;
    testrun(ov_json_clear_whitespace(&ptr, &len));
    testrun(ptr == buffer + 1);
    testrun(len == 7);

    // whitespace only
    memcpy(buffer, "\n\t\r ", 4);
    len = 4;
    ptr = buffer;
    testrun(ov_json_clear_whitespace(&ptr, &len));
    testrun(ptr == buffer + 4);
    testrun(len == 0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_validate_string() {

    uint8_t buffer[100] = {0};
    strcat((char *)buffer, "test");

    testrun(!ov_json_validate_string(NULL, 4, true));
    testrun(!ov_json_validate_string(NULL, 4, false));
    testrun(!ov_json_validate_string(buffer, 0, true));
    testrun(!ov_json_validate_string(buffer, 0, false));

    testrun(!ov_json_validate_string(buffer, 4, true));
    testrun(ov_json_validate_string(buffer, 4, false));

    buffer[0] = '\"';
    buffer[3] = '\"';

    testrun(ov_json_validate_string(buffer, 4, true));
    testrun(!ov_json_validate_string(buffer, 4, false));

    buffer[0] = '\"';
    buffer[1] = '\"';

    testrun(!ov_json_validate_string(buffer, 4, true));
    testrun(!ov_json_validate_string(buffer, 4, false));

    // escaped quotation
    buffer[0] = '\"';
    buffer[1] = 'a';
    buffer[2] = 0x5C;
    buffer[3] = 0x22;
    buffer[4] = 'd';
    buffer[5] = 'e';
    buffer[6] = 'f';
    buffer[7] = '\"';

    testrun(!ov_json_validate_string(buffer, 2, true));
    testrun(!ov_json_validate_string(buffer, 3, true));
    testrun(!ov_json_validate_string(buffer, 4, true));
    testrun(!ov_json_validate_string(buffer, 5, true));
    testrun(!ov_json_validate_string(buffer, 6, true));
    testrun(!ov_json_validate_string(buffer, 7, true));
    testrun(ov_json_validate_string(buffer, 8, true));

    // double quotation
    buffer[0] = '\"';
    buffer[1] = 'a';
    buffer[2] = 'b';
    buffer[3] = 0x22;
    buffer[4] = 'd';
    buffer[5] = 'e';
    buffer[6] = 'f';
    buffer[7] = '\"';

    testrun(!ov_json_validate_string(buffer, 2, true));
    testrun(!ov_json_validate_string(buffer, 3, true));
    testrun(ov_json_validate_string(buffer, 4, true));
    testrun(!ov_json_validate_string(buffer, 5, true));
    testrun(!ov_json_validate_string(buffer, 6, true));
    testrun(!ov_json_validate_string(buffer, 7, true));
    testrun(!ov_json_validate_string(buffer, 8, true));

    // escaped reverse solidus
    buffer[0] = '\"';
    buffer[1] = 'a';
    buffer[2] = 0x5C;
    buffer[3] = 0x5C;
    buffer[4] = 'd';
    buffer[5] = 'e';
    buffer[6] = 'f';
    buffer[7] = '\"';

    testrun(ov_json_validate_string(buffer, 8, true));

    // unescaped reverse solidus
    buffer[0] = '\"';
    buffer[1] = 'a';
    buffer[2] = 'b';
    buffer[3] = 0x5C;
    buffer[4] = 'd';
    buffer[5] = 'e';
    buffer[6] = 'f';
    buffer[7] = '\"';

    testrun(!ov_json_validate_string(buffer, 8, true));

    // escaped solidus
    buffer[0] = '\"';
    buffer[1] = 'a';
    buffer[2] = 0x5C;
    buffer[3] = 0x2F;
    buffer[4] = 'd';
    buffer[5] = 'e';
    buffer[6] = 'f';
    buffer[7] = '\"';

    testrun(ov_json_validate_string(buffer, 8, true));

    // unescaped solidus
    buffer[0] = '\"';
    buffer[1] = 'a';
    buffer[2] = 'b';
    buffer[3] = 0x2F;
    buffer[4] = 'd';
    buffer[5] = 'e';
    buffer[6] = 'f';
    buffer[7] = '\"';

    testrun(ov_json_validate_string(buffer, 8, true));

    // escaped backspace
    buffer[0] = '\"';
    buffer[1] = 'a';
    buffer[2] = 0x5C;
    buffer[3] = 0x62;
    buffer[4] = 'd';
    buffer[5] = 'e';
    buffer[6] = 'f';
    buffer[7] = '\"';

    testrun(ov_json_validate_string(buffer, 8, true));

    // escaped formfeed
    buffer[0] = '\"';
    buffer[1] = 'a';
    buffer[2] = 0x5C;
    buffer[3] = 0x66;
    buffer[4] = 'd';
    buffer[5] = 'e';
    buffer[6] = 'f';
    buffer[7] = '\"';

    testrun(ov_json_validate_string(buffer, 8, true));

    // escaped linefeed
    buffer[0] = '\"';
    buffer[1] = 'a';
    buffer[2] = 0x5C;
    buffer[3] = 0x6E;
    buffer[4] = 'd';
    buffer[5] = 'e';
    buffer[6] = 'f';
    buffer[7] = '\"';

    testrun(ov_json_validate_string(buffer, 8, true));

    // escaped carriage return
    buffer[0] = '\"';
    buffer[1] = 'a';
    buffer[2] = 0x5C;
    buffer[3] = 0x72;
    buffer[4] = 'd';
    buffer[5] = 'e';
    buffer[6] = 'f';
    buffer[7] = '\"';

    testrun(ov_json_validate_string(buffer, 8, true));

    // escaped tab
    buffer[0] = '\"';
    buffer[1] = 'a';
    buffer[2] = 0x5C;
    buffer[3] = 0x74;
    buffer[4] = 'd';
    buffer[5] = 'e';
    buffer[6] = 'f';
    buffer[7] = '\"';

    testrun(ov_json_validate_string(buffer, 8, true));

    // escaped u (too small)
    buffer[0] = '\"';
    buffer[1] = 'a';
    buffer[2] = 0x5C;
    buffer[3] = 0x75;
    buffer[4] = 'd';
    buffer[5] = 'e';
    buffer[6] = 'f';
    buffer[7] = '\"';

    testrun(!ov_json_validate_string(buffer, 8, true));

    // escaped u
    buffer[0] = '\"';
    buffer[1] = 'a';
    buffer[2] = 0x5C;
    buffer[3] = 0x75;
    buffer[4] = 'd';
    buffer[5] = 'e';
    buffer[6] = 'f';
    buffer[7] = 'g';
    buffer[8] = '\"';

    testrun(ov_json_validate_string(buffer, 9, true));

    // escaped to small
    buffer[0] = '\"';
    buffer[1] = 'a';
    buffer[2] = 0x5C;
    buffer[3] = 0x74;
    buffer[4] = '\"';

    testrun(!ov_json_validate_string(buffer, 3, true));
    testrun(!ov_json_validate_string(buffer, 4, true));
    testrun(ov_json_validate_string(buffer, 5, true));

    // zero content
    buffer[0] = '\"';
    buffer[1] = 'a';
    buffer[2] = 0;
    buffer[3] = 0;
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;
    buffer[8] = '\"';

    testrun(!ov_json_validate_string(buffer, 9, true));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_match_string() {

    uint8_t buf[100] = {0};
    char *buffer = "\"test\"";

    uint8_t *start = (uint8_t *)buffer;
    uint8_t *end = NULL;

    testrun(!ov_json_match_string(NULL, NULL, 0));
    testrun(!ov_json_match_string(NULL, &end, 6));
    testrun(!ov_json_match_string(&start, NULL, 6));

    testrun(start == (uint8_t *)buffer);
    testrun(end == NULL);

    testrun(!ov_json_match_string(&start, &end, 5));

    testrun(start == (uint8_t *)buffer);
    testrun(end == NULL);

    testrun(ov_json_match_string(&start, &end, 6));
    testrun(start == (uint8_t *)buffer + 1);
    testrun(end == (uint8_t *)buffer + 4);
    testrun(start[-1] == '\"');
    testrun(end[0 + 1] == '\"');

    buffer = "\"12345678\"";
    start = (uint8_t *)buffer;
    testrun(ov_json_match_string(&start, &end, strlen(buffer)));
    testrun(start == (uint8_t *)buffer + 1);
    testrun(end == (uint8_t *)buffer + 8);
    testrun(start[-1] == '\"');
    testrun(end[0 + 1] == '\"');

    buffer = "    \"12345678\"    ";
    start = (uint8_t *)buffer;
    testrun(ov_json_match_string(&start, &end, strlen(buffer)));
    testrun(start == (uint8_t *)buffer + 5);
    testrun(end == (uint8_t *)buffer + 4 + 8);
    testrun(start[-1] == '\"');
    testrun(end[0 + 1] == '\"');

    // check all defined JSON whitespaces
    buffer = " \n\t\r\"12345678\"    ";
    start = (uint8_t *)buffer;
    testrun(ov_json_match_string(&start, &end, strlen(buffer)));
    testrun(start == (uint8_t *)buffer + 5);
    testrun(end == (uint8_t *)buffer + 4 + 8);
    testrun(start[-1] == '\"');
    testrun(end[0 + 1] == '\"');

    // check non JSON whitespaces
    buffer = " \v\"12345678\"    ";
    testrun(isspace('\v'));
    testrun(!ov_json_is_whitespace('\v'));
    start = (uint8_t *)buffer;
    testrun(!ov_json_match_string(&start, &end, strlen(buffer)));
    testrun(start == (uint8_t *)buffer);
    testrun(end == NULL);

    // check with JSON whitespaces in string (not a valid string!)
    buffer = "\"a \n \"";
    start = (uint8_t *)buffer;
    testrun(!ov_json_match_string(&start, &end, strlen(buffer)));
    testrun(start == (uint8_t *)buffer);
    testrun(end == NULL);

    // check whitespace only
    buffer = "    ";
    start = (uint8_t *)buffer;
    testrun(!ov_json_match_string(&start, &end, strlen(buffer)));
    testrun(start == (uint8_t *)buffer);
    testrun(end == NULL);

    // check whitespace unclosed
    buffer = "  \"  ";
    start = (uint8_t *)buffer;
    testrun(!ov_json_match_string(&start, &end, strlen(buffer)));
    testrun(start == (uint8_t *)buffer);
    testrun(end == NULL);

    // check empty
    buffer = "  \"\"  ";
    start = (uint8_t *)buffer;
    testrun(!ov_json_match_string(&start, &end, strlen(buffer)));
    testrun(start == (uint8_t *)buffer);
    testrun(end == NULL);

    // check valid
    buffer = "  \" \"  ";
    start = (uint8_t *)buffer;
    testrun(ov_json_match_string(&start, &end, strlen(buffer)));
    testrun(start == (uint8_t *)buffer + 3);
    testrun(end == (uint8_t *)buffer + 3);
    testrun(start[-1] == '\"');
    testrun(end[0 + 1] == '\"');

    // check invalid string content (non UFT8)
    buf[0] = 0x20;
    buf[1] = 0x22;
    buf[2] = 0xFF;
    buf[3] = 0x22;
    start = (uint8_t *)buf;
    testrun(!ov_json_match_string(&start, &end, strlen((char *)buf)));
    testrun(start == (uint8_t *)buf);
    testrun(end == NULL);

    // check UTF8 escaped
    buf[0] = 0x20;
    buf[1] = 0x22; // "
    buf[2] = 0x65; // a
    buf[3] = 0x5c; // reverse solidus
    buf[4] = 0x75; // u
    buf[5] = 0x00; // whatever HEX
    buf[6] = 0x00; // whatever HEX
    buf[7] = 0xC2; // whatever HEX
    buf[8] = 0xBF; // whatever HEX
    buf[9] = 0x22;
    start = buf;
    testrun(ov_json_match_string(&start, &end, 10));
    testrun(start == (uint8_t *)buf + 2);
    testrun(end == (uint8_t *)buf + 8);
    testrun(start[-1] == '\"');
    testrun(end[0 + 1] == '\"');

    // check UTF8 escaped with non UTF8 content
    buf[0] = 0x20;
    buf[1] = 0x22; // "
    buf[2] = 0x65; // a
    buf[3] = 0x5c; // reverse solidus
    buf[4] = 0x75; // u
    buf[5] = 0x00; // valid UTF8 HEX
    buf[6] = 0xC1; // invalid UTF8 HEX
    buf[7] = 0xC2; // valid UTF8 HEX
    buf[8] = 0xBF; // valid UTF8 HEX
    buf[9] = 0x22;
    start = buf;
    testrun(!ov_json_match_string(&start, &end, 10));
    testrun(start == buf);
    testrun(end == NULL);

    char *test =
        "\"Ad reprehenderit amet ullamco quis. "
        "Dolore et ad officia non magna  excepteur laborum."
        "Consectetur in do ullamco duis sunt ipsum velit ea "
        "tempor."
        "In est quis enim ex quis fugiat enim nisi labore."
        "Qui eu nostrud enim proident ut ullamco nisi aute "
        "in.\\r\\n\"";

    start = (uint8_t *)test;
    testrun(ov_json_match_string(&start, &end, strlen(test)));
    testrun(start == (uint8_t *)test + 1);
    testrun(start[-1] == '\"');
    testrun(end[0 + 1] == '\"');

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_match_array() {

    uint64_t size = 1000;

    uint8_t buffer[size];
    memset(buffer, '\0', size);

    uint8_t *start = buffer;
    uint8_t *end = NULL;

    memcpy(buffer, "[]", 2);

    testrun(!ov_json_match_array(NULL, NULL, 0));
    testrun(!ov_json_match_array(&start, NULL, 10));
    testrun(!ov_json_match_array(NULL, &end, 10));
    testrun(!ov_json_match_array(&start, &end, 0));
    testrun(!ov_json_match_array(&start, &end, 1));

    testrun(!ov_json_match_array(&start, &end, 1));

    // special case empty array
    testrun(ov_json_match_array(&start, &end, 2));
    testrun(start == buffer + 1);
    testrun(end == buffer);
    testrun(end - start == -1);
    testrun(start[-1] == '[');
    testrun(end[1] == ']');
    testrun(start[0] == ']');
    testrun(end[0] == '[');

    // array with min string element
    memcpy(buffer, "[1]", 3);
    start = buffer;
    testrun(ov_json_match_array(&start, &end, 3));
    testrun(start == buffer + 1);
    testrun(end == buffer + 1);
    testrun(end - start == 0);
    testrun(start[-1] == '[');
    testrun(end[1] == ']');
    testrun(start[0] == '1');
    testrun(end[0] == '1');

    // array with min string element and leading whitespace
    memcpy(buffer, " \n\t\r [1]", 8);
    start = buffer;
    testrun(ov_json_match_array(&start, &end, 8));
    testrun(start == buffer + 6);
    testrun(end == buffer + 6);
    testrun(end - start == 0);
    testrun(start[-1] == '[');
    testrun(end[1] == ']');
    testrun(start[0] == '1');
    testrun(end[0] == '1');

    // array with empty subarr
    memcpy(buffer, "[[]]", 4);
    start = buffer;
    testrun(ov_json_match_array(&start, &end, 4));
    testrun(start == buffer + 1);
    testrun(end == buffer + 2);
    testrun(end - start == 1);
    testrun(start[-1] == '[');
    testrun(end[1] == ']');
    testrun(start[0] == '[');
    testrun(end[0] == ']');

    // array with empty subarrs
    memcpy(buffer, "[[[[[[[[[]]]]]]]]]", 18);
    start = buffer;
    testrun(ov_json_match_array(&start, &end, 18));
    testrun(start == buffer + 1);
    testrun(end == buffer + 16);
    testrun(end - start == 15);
    testrun(start[-1] == '[');
    testrun(end[1] == ']');
    testrun(start[0] == '[');
    testrun(end[0] == ']');

    // array with non empty subarrs
    memcpy(buffer, "[\"abc\",[1,[2,[3,4,5]]],true]", 28);
    start = buffer;
    testrun(ov_json_match_array(&start, &end, 28));
    testrun(start == buffer + 1);
    testrun(end == buffer + 26);
    testrun(end - start == 25);
    testrun(start[-1] == '[');
    testrun(end[1] == ']');
    testrun(start[0] == '\"');
    testrun(end[0] == 'e');

    // array with non JSON content (no content check done)
    memcpy(buffer, "[not a valid string]", 20);
    start = buffer;
    testrun(ov_json_match_array(&start, &end, 20));
    testrun(start == buffer + 1);
    testrun(end == buffer + 18);
    testrun(end - start == 17);
    testrun(start[-1] == '[');
    testrun(end[1] == ']');
    testrun(start[0] == 'n');
    testrun(end[0] == 'g');

    // array with unclosed brackets
    memcpy(buffer, "[[]", 3);
    start = buffer;
    testrun(!ov_json_match_array(&start, &end, 3));
    testrun(start == buffer);
    testrun(end == NULL);

    // array with unclosed brackets
    memcpy(buffer, "[1[2][", 6);
    start = buffer;
    testrun(!ov_json_match_array(&start, &end, 6));
    testrun(start == buffer);
    testrun(end == NULL);

    // array within a buffer
    memcpy(buffer, "[\"valid\"],[]", 12);
    start = buffer;
    testrun(ov_json_match_array(&start, &end, 12));
    testrun(start == buffer + 1);
    testrun(end == buffer + 7);
    testrun(end - start == 6);
    testrun(start[-1] == '[');
    testrun(end[1] == ']');
    testrun(start[0] == '\"');
    testrun(end[0] == '\"');

    // inner array with non matched bracket
    memcpy(buffer, "[\"abc\",[1,[2,[3,4,5]],true]", 27);
    start = buffer;
    testrun(!ov_json_match_array(&start, &end, 27));
    testrun(start == buffer);
    testrun(end == NULL);

    // no open bracket
    memcpy(buffer, "1]", 2);
    start = buffer;
    testrun(!ov_json_match_array(&start, &end, 2));
    testrun(start == buffer);
    testrun(end == NULL);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_match_object() {

    uint64_t size = 1000;

    uint8_t buffer[size];
    memset(buffer, '\0', size);

    uint8_t *start = buffer;
    uint8_t *end = NULL;

    memcpy(buffer, "{}", 2);

    testrun(!ov_json_match_object(NULL, NULL, 0));
    testrun(!ov_json_match_object(&start, NULL, 10));
    testrun(!ov_json_match_object(NULL, &end, 10));
    testrun(!ov_json_match_object(&start, &end, 0));
    testrun(!ov_json_match_object(&start, &end, 1));

    testrun(!ov_json_match_object(&start, &end, 1));

    // special case empty object
    testrun(ov_json_match_object(&start, &end, 2));
    testrun(start == buffer + 1);
    testrun(end == buffer);
    testrun(end - start == -1);
    testrun(start[-1] == '{');
    testrun(end[1] == '}');
    testrun(start[0] == '}');
    testrun(end[0] == '{');

    // object with min string element
    memcpy(buffer, "{1}", 3);
    start = buffer;
    testrun(ov_json_match_object(&start, &end, 3));
    testrun(start == buffer + 1);
    testrun(end == buffer + 1);
    testrun(end - start == 0);
    testrun(start[-1] == '{');
    testrun(end[1] == '}');
    testrun(start[0] == '1');
    testrun(end[0] == '1');

    // object with min string element and leading whitespace
    memcpy(buffer, " \n\t\r {1}", 8);
    start = buffer;
    testrun(ov_json_match_object(&start, &end, 8));
    testrun(start == buffer + 6);
    testrun(end == buffer + 6);
    testrun(end - start == 0);
    testrun(start[-1] == '{');
    testrun(end[1] == '}');
    testrun(start[0] == '1');
    testrun(end[0] == '1');

    // object with empty subobject
    memcpy(buffer, "{{}}", 4);
    start = buffer;
    testrun(ov_json_match_object(&start, &end, 4));
    testrun(start == buffer + 1);
    testrun(end == buffer + 2);
    testrun(end - start == 1);
    testrun(start[-1] == '{');
    testrun(end[1] == '}');
    testrun(start[0] == '{');
    testrun(end[0] == '}');

    // object with empty subobjects
    memcpy(buffer, "{{{{{{{{{}}}}}}}}}", 18);
    start = buffer;
    testrun(ov_json_match_object(&start, &end, 18));
    testrun(start == buffer + 1);
    testrun(end == buffer + 16);
    testrun(end - start == 15);
    testrun(start[-1] == '{');
    testrun(end[1] == '}');
    testrun(start[0] == '{');
    testrun(end[0] == '}');

    // object with non empty subobjects
    memcpy(buffer, "{\"abc\":{1,{2,{3,4,5}}},true}", 28);
    start = buffer;
    testrun(ov_json_match_object(&start, &end, 28));
    testrun(start == buffer + 1);
    testrun(end == buffer + 26);
    testrun(end - start == 25);
    testrun(start[-1] == '{');
    testrun(end[1] == '}');
    testrun(start[0] == '\"');
    testrun(end[0] == 'e');

    // object with non JSON content (no content check done)
    memcpy(buffer, "{not a valid string}", 20);
    start = buffer;
    testrun(ov_json_match_object(&start, &end, 20));
    testrun(start == buffer + 1);
    testrun(end == buffer + 18);
    testrun(end - start == 17);
    testrun(start[-1] == '{');
    testrun(end[1] == '}');
    testrun(start[0] == 'n');
    testrun(end[0] == 'g');

    // object with unclosed brackets
    memcpy(buffer, "{{]", 3);
    start = buffer;
    testrun(!ov_json_match_object(&start, &end, 3));
    testrun(start == buffer);
    testrun(end == NULL);

    // object with unclosed brackets
    memcpy(buffer, "{1:{2}{", 7);
    start = buffer;
    testrun(!ov_json_match_object(&start, &end, 7));
    testrun(start == buffer);
    testrun(end == NULL);

    // object within a buffer
    memcpy(buffer, "{\"valid\":1},{}", 14);
    start = buffer;
    testrun(ov_json_match_object(&start, &end, 12));
    testrun(start == buffer + 1);
    testrun(end == buffer + 9);
    testrun(end - start == 8);
    testrun(start[-1] == '{');
    testrun(end[1] == '}');
    testrun(start[0] == '\"');
    testrun(end[0] == '1');

    // inner object with non matched bracket
    memcpy(buffer, "{\"abc\":{1,{2,{3,4,5}},true}", 27);
    start = buffer;
    testrun(!ov_json_match_object(&start, &end, 27));
    testrun(start == buffer);
    testrun(end == NULL);

    // no open bracket
    memcpy(buffer, "1}", 2);
    start = buffer;
    testrun(!ov_json_match_object(&start, &end, 2));
    testrun(start == buffer);
    testrun(end == NULL);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_match() {

    char *buffer = "{}";
    uint8_t *first = NULL;

    testrun(!ov_json_match(NULL, 0, true, NULL));
    testrun(!ov_json_match(NULL, 0, false, NULL));

    testrun(!ov_json_match((uint8_t *)buffer, 0, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 0, false, NULL));

    testrun(!ov_json_match(NULL, 1, true, NULL));
    testrun(!ov_json_match(NULL, 1, false, NULL));

    testrun(ov_json_match((uint8_t *)buffer, 1, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 1, false, NULL));

    testrun(ov_json_match((uint8_t *)buffer, 2, true, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 2, false, NULL));

    buffer = "[]";
    testrun(ov_json_match((uint8_t *)buffer, 1, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 1, false, NULL));

    // check literals

    buffer = "[null]";
    testrun(ov_json_match((uint8_t *)buffer, 1, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 1, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 2, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 2, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 3, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 3, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 4, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 4, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 5, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 5, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 6, true, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 6, false, NULL));
    buffer = "[nUll]";
    testrun(ov_json_match((uint8_t *)buffer, 2, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 2, false, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 3, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 3, false, NULL));

    buffer = "[true]";
    testrun(ov_json_match((uint8_t *)buffer, 1, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 1, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 2, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 2, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 3, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 3, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 4, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 4, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 5, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 5, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 6, true, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 6, false, NULL));
    buffer = "[trUe]";
    testrun(ov_json_match((uint8_t *)buffer, 3, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 3, false, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 4, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 4, false, NULL));

    buffer = "[false]";
    testrun(ov_json_match((uint8_t *)buffer, 1, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 1, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 2, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 2, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 3, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 3, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 4, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 4, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 5, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 5, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 6, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 6, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 7, true, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 7, false, NULL));
    buffer = "[fAlse]";
    testrun(ov_json_match((uint8_t *)buffer, 2, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 2, false, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 3, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 3, false, NULL));

    // check literals only (will not evaluate incomplete)
    buffer = "null";
    testrun(ov_json_match((uint8_t *)buffer, 1, true, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 1, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 2, true, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 2, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 3, true, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 3, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 4, true, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 4, false, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 5, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 5, false, NULL));

    buffer = "{}{}";
    testrun(ov_json_match((uint8_t *)buffer, 1, true, &first));
    testrun(first == NULL);
    testrun(!ov_json_match((uint8_t *)buffer, 1, false, &first));
    testrun(first == NULL);
    testrun(ov_json_match((uint8_t *)buffer, 2, true, &first));
    testrun(first == (uint8_t *)buffer + 1);
    testrun(ov_json_match((uint8_t *)buffer, 2, false, &first));
    testrun(first == (uint8_t *)buffer + 1);
    testrun(ov_json_match((uint8_t *)buffer, 3, true, &first));
    testrun(first == (uint8_t *)buffer + 1);
    testrun(!ov_json_match((uint8_t *)buffer, 3, false, &first));
    testrun(first == (uint8_t *)buffer + 1);
    testrun(ov_json_match((uint8_t *)buffer, 4, true, &first));
    testrun(first == (uint8_t *)buffer + 1);

    buffer = "{}{}{}{}{";
    testrun(ov_json_match((uint8_t *)buffer, strlen(buffer), true, &first));
    testrun(first);
    testrun(first == (uint8_t *)buffer + 1);
    testrun(!ov_json_match((uint8_t *)buffer, strlen(buffer), false, &first));
    testrun(first);
    testrun(first == (uint8_t *)buffer + 1);

    buffer = "[][]";
    testrun(ov_json_match((uint8_t *)buffer, 1, true, &first));
    testrun(first == NULL);
    testrun(!ov_json_match((uint8_t *)buffer, 1, false, &first));
    testrun(first == NULL);
    testrun(ov_json_match((uint8_t *)buffer, 2, true, &first));
    testrun(first == (uint8_t *)buffer + 1);
    testrun(ov_json_match((uint8_t *)buffer, 2, false, &first));
    testrun(first == (uint8_t *)buffer + 1);
    testrun(ov_json_match((uint8_t *)buffer, 3, true, &first));
    testrun(first == (uint8_t *)buffer + 1);
    testrun(!ov_json_match((uint8_t *)buffer, 3, false, &first));
    testrun(first == (uint8_t *)buffer + 1);
    testrun(ov_json_match((uint8_t *)buffer, 4, true, &first));
    testrun(first == (uint8_t *)buffer + 1);

    // check w011223345667788901234455667890112233456789011223345677889901
    buffer = " \n\t\r[ \n\t\rnull \n\t\r ,  \n\t\r true  \n\t\r ] \n\t\r [";
    testrun(ov_json_match((uint8_t *)buffer, strlen(buffer), true, &first));
    testrun(first);
    testrun(first == (uint8_t *)buffer + (strlen(buffer) - 7));
    testrun(!ov_json_match((uint8_t *)buffer, strlen(buffer), false, &first));
    testrun(first == (uint8_t *)buffer + strlen(buffer) - 7);

    // check string

    buffer = "\"a\"";
    testrun(ov_json_match((uint8_t *)buffer, 1, true, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 1, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 2, true, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 2, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 3, true, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 3, false, NULL));

    buffer = "[\"a\"]";
    testrun(ov_json_match((uint8_t *)buffer, 1, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 1, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 2, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 2, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 3, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 3, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 4, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 4, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 5, true, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 5, false, NULL));

    // check number

    buffer = "123";
    testrun(ov_json_match((uint8_t *)buffer, 1, true, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 1, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 2, true, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 2, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 3, true, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 3, false, NULL));

    buffer = "[123]";
    testrun(ov_json_match((uint8_t *)buffer, 1, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 1, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 2, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 2, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 3, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 3, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 4, true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, 4, false, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 5, true, NULL));
    testrun(ov_json_match((uint8_t *)buffer, 5, false, NULL));

    buffer = "[-1,123e5]";
    size_t len = strlen(buffer);
    testrun(ov_json_match((uint8_t *)buffer, len, true, NULL));
    testrun(ov_json_match((uint8_t *)buffer, len, false, NULL));

    // check error in number
    buffer = "[-1,123e]";
    testrun(!ov_json_match((uint8_t *)buffer, strlen(buffer), true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, strlen(buffer), false, NULL));

    // check error in number
    buffer = "-1,123e";
    testrun(!ov_json_match((uint8_t *)buffer, strlen(buffer), true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, strlen(buffer), false, NULL));
    buffer = "-1,123e ";
    testrun(!ov_json_match((uint8_t *)buffer, strlen(buffer), true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, strlen(buffer), false, NULL));
    buffer = "-1,123E ";
    testrun(!ov_json_match((uint8_t *)buffer, strlen(buffer), true, NULL));
    testrun(!ov_json_match((uint8_t *)buffer, strlen(buffer), false, NULL));

    // check key value pair
    buffer = "{\"key\":1}";
    len = strlen(buffer);
    testrun(ov_json_match((uint8_t *)buffer, len, true, &first));
    testrun(ov_json_match((uint8_t *)buffer, len, false, &first));
    testrun(first);
    testrun(first == (uint8_t *)buffer + strlen(buffer) - 1);

    // check key value pair with whitespaces
    buffer =
        " \n\t\r{ \n\t\r\"key\" \n\t\r: \n\t\r1 \n\t\r,  "
        "\n\t\r\"2\"\n\t\r\n:\n\t\r\n2\n\t\r\n}\r\n";
    len = strlen(buffer);
    testrun(ov_json_match((uint8_t *)buffer, len, true, &first));
    testrun(ov_json_match((uint8_t *)buffer, len, false, &first));
    testrun(first);
    testrun(first == (uint8_t *)buffer + strlen(buffer) - 3);

    // check cascaded

    //        011233456778990123344
    buffer = "{\"1\":1,\"2\":2}\r\n";
    len = strlen(buffer);
    for (size_t i = 1; i < len; i++) {

        testrun(ov_json_match((uint8_t *)buffer, i, true, &first));

        if (i > 12) {
            testrun(first == (uint8_t *)buffer + 12);
        }
    }

    buffer = "{\"1\":1,\"2\":{\"x\":[{},{},{}]}, \"3\": {}}\r\n";
    len = strlen(buffer);
    for (size_t i = 1; i < len; i++) {

        testrun(ov_json_match((uint8_t *)buffer, i, true, &first));

        if (i > len - 3) {
            testrun(first == (uint8_t *)buffer + len - 3);
            testrun(ov_json_match((uint8_t *)buffer, i, false, &first));

        } else {
            testrun(!ov_json_match((uint8_t *)buffer, i, false, &first));
            // testrun(first == NULL);
        }
    }

    //                                   FAILURE OBJECT
    //        011233456778901233455678901
    buffer = "{\"1\":1,\"2\":{\"x\":[{},{[]},{}]}, \"3\": {}}\r\n";
    len = strlen(buffer);

    for (size_t i = 1; i < len; i++) {

        if (i <= 21) {
            testrun(ov_json_match((uint8_t *)buffer, i, true, &first));
        } else {
            testrun(!ov_json_match((uint8_t *)buffer, i, true, &first));
        }
    }

    // check string error (non UTF8)
    buffer = (char[]){'\"', 'b', 0xff, '\"'};
    testrun(ov_json_match((uint8_t *)buffer, 1, true, &first));
    testrun(ov_json_match((uint8_t *)buffer, 2, true, &first));
    testrun(
        ov_json_match((uint8_t *)buffer, 3, true, &first)); // unclosed string
    testrun(
        !ov_json_match((uint8_t *)buffer, 4, true, &first)); // closed string

    // check number error
    buffer = "-1e ";
    testrun(ov_json_match((uint8_t *)buffer, 1, true, &first));
    testrun(ov_json_match((uint8_t *)buffer, 2, true, &first));
    testrun(!ov_json_match((uint8_t *)buffer, 3, true, &first));
    testrun(!ov_json_match((uint8_t *)buffer, 4, true, &first));

    // check array error
    buffer = "[1,]";
    testrun(ov_json_match((uint8_t *)buffer, 1, true, &first));
    testrun(ov_json_match((uint8_t *)buffer, 2, true, &first));
    testrun(ov_json_match((uint8_t *)buffer, 3, true, &first));
    testrun(!ov_json_match((uint8_t *)buffer, 4, true, &first));
    testrun(!ov_json_match((uint8_t *)buffer, 5, true, &first));

    buffer = "{";
    testrun(ov_json_match((uint8_t *)buffer, 1, true, &first));
    testrun(0 == (char *)first);

    buffer = "1 2 3";
    testrun(ov_json_match((uint8_t *)buffer, 5, true, &first));
    testrun(buffer == (char *)first);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_parsing_escaped_double_quote() {

    char *string = "\"echo_cancellator\"";
    fprintf(stdout, "\n|%s|\n", string);

    size_t len = strlen(string);
    testrun(!ov_json_validate_string((uint8_t *)string, len, false));

    string = "\\\"echo_cancellator\\\"";
    fprintf(stdout, "\n|%s|\n", string);

    len = strlen(string);
    testrun(ov_json_validate_string((uint8_t *)string, len, false));

    return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

    testrun_init();
    testrun_test(test_ov_json_is_whitespace);
    testrun_test(test_ov_json_clear_whitespace);

    testrun_test(test_ov_json_validate_string);

    testrun_test(test_ov_json_match_string);
    testrun_test(test_ov_json_match_array);
    testrun_test(test_ov_json_match_object);

    testrun_test(test_ov_json_match);

    testrun_test(check_parsing_escaped_double_quote);

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
