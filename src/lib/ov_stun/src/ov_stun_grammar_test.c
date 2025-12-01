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

        This file is part of the openvocs project. https://openvocs.org

        ------------------------------------------------------------------------
*//**
        @file           ov_stun_grammar_test.c
        @author         Markus Toepfer

        @date           2018-06-21

        @ingroup        ov_stun

        @brief


        ------------------------------------------------------------------------
*/

#include "ov_stun_grammar.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

int test_ov_stun_grammar_offset_linear_whitespace() {

  char buffer[100] = {0};
  char *start = buffer;
  char *next = NULL;

  testrun(!ov_stun_grammar_offset_linear_whitespace(NULL, 10, &next));
  testrun(!ov_stun_grammar_offset_linear_whitespace(start, 10, NULL));
  testrun(!ov_stun_grammar_offset_linear_whitespace(start, 0, &next));

  // no whitespace contained
  testrun(ov_stun_grammar_offset_linear_whitespace(start, 10, &next));
  testrun(next == start);

  buffer[0] = ' ';
  buffer[1] = '\n';
  buffer[2] = '\r';
  buffer[3] = '\n';
  buffer[4] = '\t';
  buffer[5] = '\v';
  buffer[6] = '\f';

  testrun(ov_stun_grammar_offset_linear_whitespace(start, 10, &next));
  testrun(next == start + 7);

  // whitespace only
  testrun(!ov_stun_grammar_offset_linear_whitespace(start, 5, &next));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_grammar_is_qdtext() {

  uint8_t buffer[100] = {0};
  uint8_t *start = buffer;

  testrun(!ov_stun_grammar_is_qdtext(NULL, 0));
  testrun(!ov_stun_grammar_is_qdtext(start, 0));
  testrun(!ov_stun_grammar_is_qdtext(NULL, 12));

  // no text contained
  testrun(!ov_stun_grammar_is_qdtext(start, 1));
  testrun(!ov_stun_grammar_is_qdtext(start, 10));

  // some valid UTF8
  buffer[0] = 0xF0; // Gothic Letter Ahsa U+10330
  buffer[1] = 0x90;
  buffer[2] = 0x8C;
  buffer[3] = 0xB0;
  buffer[4] = 0xF0; // Gothic Letter Bairkan U+10331
  buffer[5] = 0x90;
  buffer[6] = 0x8C;
  buffer[7] = 0xB1;
  buffer[8] = 0xF0; // Gothic Letter Giba U+10332
  buffer[9] = 0x90;
  buffer[10] = 0x8C;
  buffer[11] = 0xB2;

  // UTF8 only
  testrun(ov_stun_grammar_is_qdtext(start, 12));

  // test ascii
  for (int i = 0; i <= 0xff; i++) {

    buffer[12] = i;
    if (i > 0x7E) {

      // non ascii, non complete UTF8
      testrun(!ov_stun_grammar_is_qdtext(start, 13));

    } else if ((i == 0x5c) || (i == 0x22) || (i < 0x21)) {

      // non allowed ascii
      testrun(!ov_stun_grammar_is_qdtext(start, 13));
    } else {
      // allowed ascii
      testrun(ov_stun_grammar_is_qdtext(start, 13));
    }
  }

  // add some valid ascii sequence
  buffer[12] = 't';
  buffer[13] = 'e';
  buffer[14] = 's';
  buffer[15] = 't';

  testrun(ov_stun_grammar_is_qdtext(start, 13));
  testrun(ov_stun_grammar_is_qdtext(start, 14));
  testrun(ov_stun_grammar_is_qdtext(start, 15));
  testrun(ov_stun_grammar_is_qdtext(start, 16));

  // check with whitespace offset
  buffer[0] = ' ';
  buffer[1] = '\r';
  buffer[2] = '\n';
  buffer[3] = '\t';
  testrun(ov_stun_grammar_is_qdtext(start, 13));
  testrun(ov_stun_grammar_is_qdtext(start, 14));
  testrun(ov_stun_grammar_is_qdtext(start, 15));
  testrun(ov_stun_grammar_is_qdtext(start, 16));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_grammar_is_quoted_pair() {

  uint8_t buffer[100] = {0};
  uint8_t *start = buffer;

  buffer[0] = 0x5C; //  \x
  buffer[1] = 'x';

  testrun(!ov_stun_grammar_is_quoted_pair(NULL, 0));
  testrun(!ov_stun_grammar_is_quoted_pair(start, 0));
  testrun(!ov_stun_grammar_is_quoted_pair(NULL, 2));

  // no text contained
  testrun(!ov_stun_grammar_is_quoted_pair(start, 1));
  testrun(!ov_stun_grammar_is_quoted_pair(start, 3));

  testrun(ov_stun_grammar_is_quoted_pair(start, 2));

  // test ascii
  for (int i = 0; i <= 0xff; i++) {

    buffer[0] = i;
    if (i == 0x5C) {
      testrun(ov_stun_grammar_is_quoted_pair(start, 2));
      testrun(!ov_stun_grammar_is_quoted_pair(start, 3));
    } else {
      testrun(!ov_stun_grammar_is_quoted_pair(start, 2));
      testrun(!ov_stun_grammar_is_quoted_pair(start, 3));
    }
    buffer[0] = 0x5C;

    buffer[1] = i;
    if ((i > 0x7F) || (i == 0x0A) || (i == 0x0D)) {
      testrun(!ov_stun_grammar_is_quoted_pair(start, 2));
      testrun(!ov_stun_grammar_is_quoted_pair(start, 3));
    } else {
      testrun(ov_stun_grammar_is_quoted_pair(start, 2));
      testrun(!ov_stun_grammar_is_quoted_pair(start, 3));
    }
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_grammar_is_quoted_string_content() {

  uint8_t buffer[100] = {0};
  uint8_t *start = buffer;

  buffer[0] = 0x5C; //  \x
  buffer[1] = 'x';

  testrun(!ov_stun_grammar_is_quoted_string_content(NULL, 0));
  testrun(!ov_stun_grammar_is_quoted_string_content(start, 0));
  testrun(!ov_stun_grammar_is_quoted_string_content(NULL, 2));

  testrun(!ov_stun_grammar_is_quoted_string_content(start, 1));
  testrun(!ov_stun_grammar_is_quoted_string_content(start, 3));

  // quoted pair only
  testrun(ov_stun_grammar_is_quoted_string_content(start, 2));

  memcpy(buffer, "test", 4);
  testrun(ov_stun_grammar_is_quoted_string_content(start, 4));
  testrun(!ov_stun_grammar_is_quoted_string_content(start, 5));

  // quoted pair after text
  buffer[4] = 0x5C;
  buffer[5] = 'x';
  testrun(ov_stun_grammar_is_quoted_string_content(start, 4));
  testrun(!ov_stun_grammar_is_quoted_string_content(start, 5));
  testrun(ov_stun_grammar_is_quoted_string_content(start, 6));
  testrun(!ov_stun_grammar_is_quoted_string_content(start, 7));

  memcpy(buffer + 6, "abcd", 4);
  testrun(ov_stun_grammar_is_quoted_string_content(start, 6));
  testrun(ov_stun_grammar_is_quoted_string_content(start, 7));
  testrun(ov_stun_grammar_is_quoted_string_content(start, 8));
  testrun(ov_stun_grammar_is_quoted_string_content(start, 9));
  testrun(ov_stun_grammar_is_quoted_string_content(start, 10));
  testrun(!ov_stun_grammar_is_quoted_string_content(start, 11));

  // non allowed escape
  buffer[5] = 0x0A;
  testrun(!ov_stun_grammar_is_quoted_string_content(start, 10));
  buffer[5] = 0x0D;
  testrun(!ov_stun_grammar_is_quoted_string_content(start, 10));
  buffer[5] = 0xFF;
  testrun(!ov_stun_grammar_is_quoted_string_content(start, 10));
  buffer[5] = 0x09;
  testrun(ov_stun_grammar_is_quoted_string_content(start, 10));

  // non allowed text
  buffer[7] = 0x22;
  testrun(!ov_stun_grammar_is_quoted_string_content(start, 10));
  testrun(ov_stun_grammar_is_quoted_string_content(start, 7));
  testrun(!ov_stun_grammar_is_quoted_string_content(start, 9));
  buffer[7] = 'x';
  testrun(ov_stun_grammar_is_quoted_string_content(start, 7));

  // multiple escapes

  buffer[0] = 'A';
  buffer[1] = 'B';
  buffer[2] = 0x5C; //  \x
  buffer[3] = 'x';
  buffer[4] = 0x5C; //  \x
  buffer[5] = 'x';
  buffer[6] = 0x5C; //  \x
  buffer[7] = 'x';
  buffer[8] = 0x5C; //  \x
  buffer[9] = 'x';
  buffer[10] = 'C'; //  \x
  buffer[11] = 'D';
  buffer[12] = 0x5C; //  \x
  buffer[13] = 'x';
  buffer[14] = 'E';
  buffer[15] = 'F';

  testrun(!ov_stun_grammar_is_quoted_string_content(start, 13));
  testrun(ov_stun_grammar_is_quoted_string_content(start, 14));
  testrun(ov_stun_grammar_is_quoted_string_content(start, 15));
  testrun(ov_stun_grammar_is_quoted_string_content(start, 16));
  testrun(!ov_stun_grammar_is_quoted_string_content(start, 17));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_grammar_is_quoted_string() {

  uint8_t buffer[100] = {0};
  uint8_t *start = buffer;

  buffer[0] = ' ';
  buffer[1] = '"';
  buffer[2] = 'x';
  buffer[3] = '"';

  testrun(!ov_stun_grammar_is_quoted_string(NULL, 0));
  testrun(!ov_stun_grammar_is_quoted_string(start, 0));
  testrun(!ov_stun_grammar_is_quoted_string(NULL, 4));

  testrun(!ov_stun_grammar_is_quoted_string(start, 1));
  testrun(!ov_stun_grammar_is_quoted_string(start, 2));
  testrun(!ov_stun_grammar_is_quoted_string(start, 3));

  testrun(ov_stun_grammar_is_quoted_string(start, 4));

  buffer[0] = ' ';
  buffer[1] = '"';
  buffer[2] = '"';
  // empty
  testrun(ov_stun_grammar_is_quoted_string(start, 3));

  buffer[0] = ' ';
  buffer[1] = '"';
  buffer[2] = 'x';
  buffer[3] = '"';

  for (int i = 0; i <= 0xff; i++) {

    buffer[0] = i;
    if (!isspace(i)) {
      testrun(!ov_stun_grammar_is_quoted_string(start, 4));
    } else {
      testrun(ov_stun_grammar_is_quoted_string(start, 4));
    }
    buffer[0] = ' ';

    buffer[1] = i;
    if (i != '"') {
      testrun(!ov_stun_grammar_is_quoted_string(start, 4));
    } else {
      testrun(ov_stun_grammar_is_quoted_string(start, 4));
    }
    buffer[1] = '"';

    buffer[3] = i;
    if (i != '"') {
      testrun(!ov_stun_grammar_is_quoted_string(start, 4));
    } else {
      testrun(ov_stun_grammar_is_quoted_string(start, 4));
    }
    buffer[3] = '"';

    buffer[2] = i;
    if (ov_stun_grammar_is_quoted_string_content(buffer + 2, 1)) {
      testrun(ov_stun_grammar_is_quoted_string(start, 4));
    } else {
      testrun(!ov_stun_grammar_is_quoted_string(start, 4));
    }
    buffer[2] = 'x';
  }

  buffer[0] = ' ';
  buffer[1] = '"';
  buffer[2] = 'x';
  buffer[3] = 'y';
  buffer[4] = 'z';
  buffer[5] = '1';
  buffer[6] = '2';
  buffer[7] = '"';

  testrun(ov_stun_grammar_is_quoted_string(buffer, strlen((char *)buffer)));

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
  testrun_test(test_ov_stun_grammar_offset_linear_whitespace);
  testrun_test(test_ov_stun_grammar_is_qdtext);
  testrun_test(test_ov_stun_grammar_is_quoted_pair);
  testrun_test(test_ov_stun_grammar_is_quoted_string_content);
  testrun_test(test_ov_stun_grammar_is_quoted_string);

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
