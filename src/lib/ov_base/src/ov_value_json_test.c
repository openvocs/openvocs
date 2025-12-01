/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_value_json_test.c

        @author         Michael J. Beer, DLR/GSOC
        @author         Markus Töpfer

        @date           2021-04-01


        ------------------------------------------------------------------------
*/

#define OV_VALUE_TEST

#include <ov_test/ov_test.h>

#include "../include/ov_value.h"
#include "ov_value_json.c"

#include "../include/ov_dump.h"

/*----------------------------------------------------------------------------*/

static bool check_parse(char const *str, ov_value *ref,
                        char const *ref_remainder) {

  bool success_p = false;

  ov_buffer buf = {

      .start = (uint8_t *)str,
      .length = strlen(str) + 1,

  };

  char const *remainder = 0;

  ov_value *v = ov_value_parse_json(&buf, &remainder);
  /*
      char *parsed_str = ov_value_to_string(v);
      fprintf(stderr, "'%s' parsed to '%s'\n", str, parsed_str);
      free(parsed_str);
  */
  if (!ov_value_match(v, ref)) {

    testrun_log_error("Parsed object does not match reference");
    goto error;
  }

  if ((0 == ref_remainder) && (0 != remainder)) {

    testrun_log_error("Remainders don't match: Expected null pointer, got "
                      "'%s'",
                      remainder);

    goto error;
  }

  if ((0 != ref_remainder) && (0 == remainder)) {

    testrun_log_error("Remainders don't match: Expected '%s', got null "
                      "pointer",
                      ref_remainder);
    goto error;
  }

  if ((0 != ref_remainder) && (0 != remainder) &&
      (0 != strcmp(ref_remainder, remainder))) {

    testrun_log_error("Remainders don't match- expected '%s' - got '%s'\n",
                      ref_remainder, remainder);

    goto error;
  }

  success_p = true;

error:

  ref = ov_value_free(ref);
  v = ov_value_free(v);
  return success_p;
}

/*----------------------------------------------------------------------------*/

static int check_parse_null() {

  ov_value *val = NULL;

  const char *next = NULL;
  char *in = "nullnext";

  // incomplete null
  for (size_t i = 1; i < 4; i++) {

    next = NULL;
    testrun(!parse_null(in, i, &next));
    testrun(next == in + i);
  }

  val = parse_null(in, 4, &next);
  testrun(val);
  testrun(ov_value_is_null(val));
  testrun(next == in + 4);
  val = ov_value_free(val);

  // must be whitespace cleared
  in = " \nnull ";
  testrun(!parse_null(in, strlen(in), &next));
  testrun(next == in);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int check_parse_true() {

  ov_value *val = NULL;

  const char *next = NULL;
  char *in = "truenext";

  // incomplete null
  for (size_t i = 1; i < 4; i++) {

    next = NULL;
    testrun(!parse_true(in, i, &next));
    testrun(next == in + i);
  }

  val = parse_true(in, 4, &next);
  testrun(val);
  testrun(ov_value_is_true(val));
  testrun(next == in + 4);
  val = ov_value_free(val);

  // must be whitespace cleared
  in = " \ntrue ";
  testrun(!parse_true(in, strlen(in), &next));
  testrun(next == in);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int check_parse_false() {

  ov_value *val = NULL;

  const char *next = NULL;
  char *in = "falsenext";

  // incomplete null
  for (size_t i = 1; i < 5; i++) {

    next = NULL;
    testrun(!parse_false(in, i, &next));
    testrun(next == in + i);
  }

  val = parse_false(in, 5, &next);
  testrun(val);
  testrun(ov_value_is_false(val));
  testrun(next == in + 5);
  val = ov_value_free(val);

  // must be whitespace cleared
  in = " \nfalse ";
  testrun(!parse_false(in, strlen(in), &next));
  testrun(next == in);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int check_parse_number() {

  ov_value *val = NULL;

  const char *next = NULL;
  char *in = "123456next";

  for (size_t i = 1; i < 6; i++) {

    val = parse_number(in, i, &next);
    testrun(val);
    testrun(next == in + i);

    switch (i) {

    case 1:
      testrun(1 == ov_value_get_number(val));
      break;

    case 2:
      testrun(12 == ov_value_get_number(val));
      break;

    case 3:
      testrun(123 == ov_value_get_number(val));
      break;

    case 4:
      testrun(1234 == ov_value_get_number(val));
      break;

    case 5:
      testrun(12345 == ov_value_get_number(val));
      break;

    case 6:
      testrun(123456 == ov_value_get_number(val));
      break;
    }

    val = ov_value_free(val);
  }

  in = "+1e5";
  val = parse_number(in, strlen(in), &next);
  testrun(next == in + strlen(in));
  testrun(1e5 == ov_value_get_number(val));
  val = ov_value_free(val);

  in = "1e-5";
  val = parse_number(in, strlen(in), &next);
  testrun(next == in + strlen(in));
  testrun(1e-5 == ov_value_get_number(val));
  val = ov_value_free(val);

  in = "-123e-5";
  val = parse_number(in, strlen(in), &next);
  testrun(next == in + strlen(in));
  testrun(-123e-5 == ov_value_get_number(val));
  val = ov_value_free(val);

  in = "+1e";
  testrun(!parse_number(in, strlen(in), &next));
  testrun(next == in + strlen(in));

  in = "+1e-";
  testrun(!parse_number(in, strlen(in), &next));
  testrun(next == in + strlen(in));

  in = "+1e+";
  testrun(!parse_number(in, strlen(in), &next));
  testrun(next == in + strlen(in));

  in = "-1e-";
  testrun(!parse_number(in, strlen(in), &next));
  testrun(next == in + strlen(in));

  in = "-1e+";
  testrun(!parse_number(in, strlen(in), &next));
  testrun(next == in + strlen(in));

  in = "-1E-";
  testrun(!parse_number(in, strlen(in), &next));
  testrun(next == in + strlen(in));

  in = "-1E+";
  testrun(!parse_number(in, strlen(in), &next));
  testrun(next == in + strlen(in));

  in = "+1e-1";
  val = parse_number(in, strlen(in), &next);
  testrun(next == in + strlen(in));
  testrun(1e-1 == ov_value_get_number(val));
  val = ov_value_free(val);

  in = "+1e-1 ";
  val = parse_number(in, strlen(in), &next);
  testrun(next == in + strlen(in) - 1);
  testrun(1e-1 == ov_value_get_number(val));
  val = ov_value_free(val);

  in = "+1E-1";
  val = parse_number(in, strlen(in), &next);
  testrun(next == in + strlen(in));
  testrun(1e-1 == ov_value_get_number(val));
  val = ov_value_free(val);

  in = "+1E-1 ";
  val = parse_number(in, strlen(in), &next);
  testrun(next == in + strlen(in) - 1);
  testrun(1e-1 == ov_value_get_number(val));
  val = ov_value_free(val);

  in = "9223372036854775807 ";
  val = parse_number(in, strlen(in), &next);
  testrun(next == in + strlen(in) - 1);
  testrun(9223372036854775807.0 == ov_value_get_number(val));
  val = ov_value_free(val);

  double d = -9223372036854775807.0;

  in = "-9223372036854775807 ";
  val = parse_number(in, strlen(in), &next);
  testrun(next == in + strlen(in) - 1);
  testrun(d == ov_value_get_number(val));
  val = ov_value_free(val);

  /* UINT64_T MAX 18446744073709551614,
   * causes warnings of to large in test
   * so testing only with int64_t max/min done */

  in = "1.7E-308 ";
  val = parse_number(in, strlen(in), &next);
  testrun(next == in + strlen(in) - 1);
  testrun(1.7E-308 == ov_value_get_number(val));
  val = ov_value_free(val);

  in = "1.7E+308 ";
  val = parse_number(in, strlen(in), &next);
  testrun(next == in + strlen(in) - 1);
  testrun(1.7E+308 == ov_value_get_number(val));
  val = ov_value_free(val);

  in = "1.7x+308 ";
  val = parse_number(in, strlen(in), &next);
  testrun(next == in + 3);
  testrun(1.7 == ov_value_get_number(val));
  val = ov_value_free(val);

  // non numbers
  in = "no number";
  next = NULL;
  testrun(!parse_number(in, strlen(in), &next));
  testrun(next == in);

  in = "1.7ex";
  next = NULL;
  testrun(!parse_number(in, strlen(in), &next));
  testrun(next == in);

  in = "1.7e-x";
  next = NULL;
  testrun(!parse_number(in, strlen(in), &next));
  testrun(next == in);

  in = "1.7e+x";
  next = NULL;
  testrun(!parse_number(in, strlen(in), &next));
  testrun(next == in);

  in = "1.7ex";
  next = NULL;
  testrun(!parse_number(in, strlen(in), &next));
  testrun(next == in);

  in = ".7";
  next = NULL;
  testrun(!parse_number(in, strlen(in), &next));
  testrun(next == in);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int check_unescape_json() {

  ov_buffer *buf = ov_buffer_create(1000);

  const char *in = "a";
  const char *expect = NULL;

  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(in, buf->start, buf->length));
  testrun(1 == buf->length);

  in = "abcd";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(in, buf->start, buf->length));
  testrun(4 == buf->length);

  in = " abcd ";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(in, buf->start, buf->length));
  testrun(6 == buf->length);

  in = "single\\\"quote";
  expect = "single\"quote";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "double\\\"quote\\\"";
  expect = "double\"quote\"";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\\"";
  expect = "\"";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\\\";
  expect = "\\";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\b";
  expect = "\b";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\f";
  expect = "\f";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\n";
  expect = "\n";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\r";
  expect = "\r";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\t";
  expect = "\t";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\/";
  expect = "/";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\u0040";
  expect = "@";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\u0021";
  expect = "!";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\u0100";
  expect = "Ā";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\u0100\\u0101";
  expect = "Āā";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\u03A0";
  expect = "Π";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\u03a0";
  expect = "Π";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\u1000";
  expect = "က";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\u144C";
  expect = "ᑌ";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\u3e18";
  expect = "㸘";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\uFFFD";
  expect = "�";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\uFB00";
  expect = "ﬀ";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  in = "\\u221E";
  expect = "∞";
  testrun(unescape_json((uint8_t *)in, strlen(in), buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  char b[10] = {0};

  /*
      C/C++/Java source code  "\u3E18"
      UTF-8 (hex) 0xE3 0xB8 0x98
  */

  b[0] = 'x';
  b[1] = 0xe3;
  b[2] = 0xb8;
  b[3] = 0x98;
  b[4] = 'y';

  expect = "x㸘y";
  testrun(unescape_json((uint8_t *)b, 5, buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  /*
      C/C++/Java source code  "\uFB00"
      UTF-8 (hex) 0xEF 0xAC 0x80
  */

  b[0] = 'x';
  b[1] = 0xef;
  b[2] = 0xac;
  b[3] = 0x80;
  b[4] = 'y';

  expect = "xﬀy";
  testrun(unescape_json((uint8_t *)b, 5, buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  /*
      C/C++/Java source code  "\u221e"
      UTF-8 (hex) 0xE2 0x88 0x9E
  */

  b[0] = 'x';
  b[1] = 0xe2;
  b[2] = 0x88;
  b[3] = 0x9E;
  b[4] = 'y';

  expect = "x∞y";
  testrun(unescape_json((uint8_t *)b, 5, buf));
  testrun(0 == memcmp(expect, buf->start, buf->length));
  testrun(strlen(expect) == buf->length);

  b[0] = 'x';
  b[1] = '\"';
  b[2] = 'y';

  testrun(!unescape_json((uint8_t *)b, 3, buf));

  // check all possible byte inputs at pos 1
  for (size_t i = 0; i < 0xff; i++) {

    b[1] = i;
    if (i < 0x1F) {
      // control char
      testrun(!unescape_json((uint8_t *)b, 3, buf));
    } else if (i == '\"') {
      // quote
      testrun(!unescape_json((uint8_t *)b, 3, buf));
    } else if (i == '\\') {
      // slash
      testrun(!unescape_json((uint8_t *)b, 3, buf));
    } else if (i > 127) {
      // non ascii
      testrun(!unescape_json((uint8_t *)b, 3, buf));
    } else {
      testrun(unescape_json((uint8_t *)b, 3, buf));
    }
  }

  buf = ov_buffer_free(buf);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int check_parse_string() {

  ov_value *val = NULL;

  const char *next = NULL;
  char *in = "a";

  testrun(!parse_string(in, strlen(in), &next));
  testrun(next == in);

  in = "\"";
  testrun(!parse_string(in, strlen(in), &next));
  testrun(next == in + 1);

  in = "\"a";
  testrun(!parse_string(in, strlen(in), &next));
  testrun(next == in + 2);

  in = "\"a\"";
  val = parse_string(in, strlen(in), &next);
  testrun(val);
  testrun(0 == strcmp("a", ov_value_get_string(val)));
  testrun(next == in + strlen(in));
  val = ov_value_free(val);

  in = "\"a\\\"test\\\"of inline quotes\"";
  val = parse_string(in, strlen(in), &next);
  testrun(val);
  testrun(0 == strcmp("a\"test\"of inline quotes", ov_value_get_string(val)));
  testrun(next == in + strlen(in));
  val = ov_value_free(val);

  next = NULL;
  testrun(!parse_string(in, strlen(in) - 1, &next));
  testrun(next == in + strlen(in) - 1);

  // check mix of all escapes, ascii, UTF8 and escaped UTF8
  in = "\" \\\" \\\\ \\b \\/ … \\f \\n \\r \\t \\u3E18\\u221e∞ a \"";
  val = parse_string(in, strlen(in), &next);
  testrun(val);
  testrun(0 == strcmp(" \" \\ \b / … \f \n \r \t 㸘∞∞ a ",
                      ov_value_get_string(val)));
  testrun(next == in + strlen(in));
  val = ov_value_free(val);

  // check streaming byte by byte input
  for (size_t i = 1; i < strlen(in); i++) {

    testrun(!parse_string(in, i, &next));
    testrun(next == in + i);
  }

  // unescaped line break is not supported by JSON spec
  in = "\"error here ->\n\"";
  testrun(!parse_string(in, strlen(in), &next));
  testrun(next == in + 14);

  // vertical tab is not supported by JSON spec
  in = "\"error here ->\\\v\"";
  testrun(!parse_string(in, strlen(in), &next));
  testrun(next == in + 15);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int check_parse_list() {

  ov_value *val = NULL;

  const char *next = NULL;
  char *in = "a";

  testrun(!parse_list(in, strlen(in), &next));
  testrun(next == in);

  in = "[]";
  testrun(!parse_list(in, 1, &next));
  testrun(next == in + 1);

  val = parse_list(in, strlen(in), &next);
  testrun(val);
  testrun(next == in + strlen(in));
  testrun(ov_value_is_list(val));
  testrun(0 == ov_value_count(val));
  val = ov_value_free(val);

  in = "[ \n\r\t ]";
  val = parse_list(in, strlen(in), &next);
  testrun(val);
  testrun(next == in + strlen(in));
  testrun(ov_value_is_list(val));
  testrun(0 == ov_value_count(val));
  val = ov_value_free(val);

  in = "[1,2,3]";
  val = parse_list(in, strlen(in), &next);
  testrun(val);
  testrun(next == in + strlen(in));
  testrun(ov_value_is_list(val));
  testrun(3 == ov_value_count(val));
  val = ov_value_free(val);

  in = "[ \n\r\t 1 \n\r\t , \n\r\t 2 \n\r\t , \n\r\t 3 \n\r\t ]";
  val = parse_list(in, strlen(in), &next);
  testrun(val);
  testrun(next == in + strlen(in));
  testrun(ov_value_is_list(val));
  testrun(3 == ov_value_count(val));
  val = ov_value_free(val);

  in = "[1,2,3,]";
  testrun(!parse_list(in, strlen(in), &next));
  testrun(next == in + strlen(in) - 1);

  for (size_t i = 1; i < strlen(in); i++) {

    testrun(!parse_list(in, i, &next));
    testrun(next == in + i);
  }

  in = "[1,2,3, \"string\" , [], \n null \n\r\t, true, false ]";
  val = parse_list(in, strlen(in), &next);
  testrun(val);
  testrun(next == in + strlen(in));
  testrun(ov_value_is_list(val));
  testrun(8 == ov_value_count(val));
  val = ov_value_free(val);

  for (size_t i = 1; i < strlen(in); i++) {

    testrun(!parse_list(in, i, &next));
    testrun(next == in + i);
  }

  //    012345677890123455667
  in = "[1,2,3,\"error->\n\"]";
  testrun(!parse_list(in, strlen(in), &next));
  testrun(next == in + 15);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int check_parse_object() {

  ov_value *val = NULL;

  const char *next = NULL;
  char *in = "a";

  testrun(!parse_object(in, strlen(in), &next));
  testrun(next == in);

  in = "{}";
  testrun(!parse_object(in, 1, &next));
  testrun(next == in + 1);

  val = parse_object(in, strlen(in), &next);
  testrun(val);
  testrun(next == in + strlen(in));
  testrun(ov_value_is_object(val));
  testrun(0 == ov_value_count(val));
  val = ov_value_free(val);

  in = "{\"a\":1}";
  val = parse_object(in, strlen(in), &next);
  testrun(val);
  testrun(next == in + strlen(in));
  testrun(ov_value_is_object(val));
  testrun(1 == ov_value_count(val));
  testrun(1 == ov_value_get_number(ov_value_object_get(val, "a")));
  val = ov_value_free(val);

  in = "{ \n\r\t \"a\" \n\r\t : \n\r\t 1 \n\r\t , \n\r\t \"b\" : 2 }";
  val = parse_object(in, strlen(in), &next);
  testrun(val);
  testrun(next == in + strlen(in));
  testrun(ov_value_is_object(val));
  testrun(2 == ov_value_count(val));
  testrun(1 == ov_value_get_number(ov_value_object_get(val, "a")));
  testrun(2 == ov_value_get_number(ov_value_object_get(val, "b")));
  val = ov_value_free(val);

  in = "{\"a\":1,\"b\":{\"c\":2}}";
  val = parse_object(in, strlen(in), &next);
  testrun(val);
  testrun(next == in + strlen(in));
  testrun(ov_value_is_object(val));
  testrun(2 == ov_value_count(val));
  testrun(1 == ov_value_get_number(ov_value_object_get(val, "a")));
  testrun(2 == ov_value_get_number(
                   ov_value_object_get(ov_value_object_get(val, "b"), "c")));
  val = ov_value_free(val);

  // check byte by byte parsing
  for (size_t i = 1; i < strlen(in); i++) {

    val = parse_object(in, i, &next);
    testrun(!val);
    testrun(next == in + i);
  }

  in = "{\"a\":{}}";
  val = parse_object(in, strlen(in), &next);
  testrun(next == in + strlen(in));
  testrun(val);
  testrun(ov_value_is_object(val));
  testrun(1 == ov_value_count(val));
  testrun(ov_value_is_object(ov_value_object_get(val, "a")));
  testrun(0 == ov_value_count(ov_value_object_get(val, "a")));
  val = ov_value_free(val);

  in = "{\"a\":1,\"b\":{\"c\":[2,\"a\",{}]}}";
  val = parse_object(in, strlen(in), &next);
  testrun(next == in + strlen(in));
  testrun(val);
  testrun(ov_value_is_object(val));
  testrun(2 == ov_value_count(val));
  testrun(1 == ov_value_get_number(ov_value_object_get(val, "a")));
  testrun(3 == ov_value_count(
                   ov_value_object_get(ov_value_object_get(val, "b"), "c")));
  val = ov_value_free(val);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_parse_json() {

  testrun(check_parse("null", ov_value_null(), ""));
  testrun(check_parse("true", ov_value_true(), ""));
  testrun(check_parse("false", ov_value_false(), ""));

  testrun(check_parse("    null", ov_value_null(), ""));
  testrun(check_parse("    null bgh", ov_value_null(), " bgh"));

  testrun(check_parse("\"vali\"", ov_value_string("vali"), ""));
  testrun(check_parse("    \"vali\"", ov_value_string("vali"), ""));
  testrun(check_parse("    \"vali\" \"Baldr\"", ov_value_string("vali"),
                      " \"Baldr\""));

  testrun(check_parse("    \"one\" ", ov_value_string("one"), " "));

  testrun(check_parse("  \t\r\n  \"all whitespaces\"\t \r\n",
                      ov_value_string("all whitespaces"), "\t \r\n"));

  testrun(check_parse(" \"some \\\\ slash escape\" ",
                      ov_value_string("some \\ slash escape"), " "));

  testrun(check_parse("    \"vali \\\"the avenger\\\"\" \"Baldr\"",
                      ov_value_string("vali \"the avenger\""), " \"Baldr\""));

  char const *unterminated_string = "    \"vali \\\"the avenger \\\\ Baldr";

  ov_buffer buf = {
      .start = (uint8_t *)unterminated_string,
      .length = strlen(unterminated_string),
  };

  testrun(0 == ov_value_parse_json(&buf, 0));

  buf.length += 10;
  buf.start = calloc(1, buf.length);
  strcpy((char *)buf.start, unterminated_string);

  testrun(0 == ov_value_parse_json(&buf, 0));

  free(buf.start);
  buf.start = 0;

  // Numbers

  testrun(check_parse("42", ov_value_number(42), ""));
  testrun(check_parse("+42", ov_value_number(42), ""));
  testrun(check_parse("-42", ov_value_number(-42), ""));

  testrun(check_parse("13.37", ov_value_number(13.37), ""));
  testrun(check_parse("+13.37", ov_value_number(13.37), ""));
  testrun(check_parse("-13.37", ov_value_number(-13.37), ""));
  testrun(check_parse("\t    \t-13.37", ov_value_number(-13.37), ""));
  testrun(check_parse(" \t-13.37\t\"vali\"", ov_value_number(-13.37),
                      "\t\"vali\""));

  // Lists

  testrun(check_parse(" []", ov_value_list(0), ""));
  testrun(check_parse("\t [true]\n", ov_value_list(ov_value_true()), "\n"));

  testrun(check_parse("\t [1916, null]\n",
                      ov_value_list(ov_value_number(1916), ov_value_null()),
                      "\n"));

  testrun(check_parse(" \n\n\n   [\"\\\"Naftagn\\\"\", \"Cthulhu\", 12.112] "
                      ", \"",
                      ov_value_list(ov_value_string("\"Naftagn\""),
                                    ov_value_string("Cthulhu"),
                                    ov_value_number(12.112)),
                      " , \""));

  testrun(check_parse(" \n\n\n   [\"\\\"Naftagn\\\"\", [\"Cthulhu\", "
                      "12.112]] , \"",
                      ov_value_list(ov_value_string("\"Naftagn\""),
                                    ov_value_list(ov_value_string("Cthulhu"),
                                                  ov_value_number(12.112))),
                      " , \""));

  testrun(check_parse(
      " \n\n\n   [[\"Naftagn\"], [\"Cthulhu\", "
      "[true, null], 12.112]] , \"",
      ov_value_list(
          ov_value_list(ov_value_string("Naftagn")),
          ov_value_list(ov_value_string("Cthulhu"),
                        ov_value_list(ov_value_true(), ov_value_null()),
                        ov_value_number(12.112))),
      " , \""));

  testrun(
      check_parse("{\"key1\" : [1, 2]}blablablubb",
                  OBJECT(0, PAIR("key1", ov_value_list(ov_value_number(1),
                                                       ov_value_number(2)))),
                  "blablablubb"));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_value_json", check_parse_null, check_parse_true,
            check_parse_false, check_parse_number, check_unescape_json,
            check_parse_string, check_parse_list, check_parse_object,
            test_ov_value_parse_json);
