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
        @file           ov_string_test.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-11-08

        @ingroup        ov_basics

        @brief          Unit tests of


        ------------------------------------------------------------------------
*/
#include "ov_string.c"
#include <ov_test/ov_test.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_free() {

  char *string = calloc(1, sizeof(char));

  testrun(NULL == ov_free(NULL));

  testrun(NULL != string);
  string = ov_free(string);
  testrun(NULL == string);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_string_copy() {

  testrun(0 == ov_string_copy(0, 0, 0));
  testrun(0 == ov_string_copy(0, 0, 10));

  char target[10] = {0};

  testrun(0 == ov_string_copy(target, 0, 0));
  testrun(0 == ov_string_copy(target, 0, sizeof(target)));

  testrun(target == ov_string_copy(target, "abc", sizeof(target)));
  testrun(0 == ov_string_compare(target, "abc"));

  char const *too_long = "123456789012";
  testrun(target == ov_string_copy(target, too_long, sizeof(target)));
  testrun(0 == ov_string_compare(target, "123456789"));

  char *result = ov_string_copy(0, too_long, sizeof(target));
  testrun(0 == ov_string_compare(result, "123456789"));

  result = ov_free(result);
  testrun(0 == result);

  result = ov_string_copy(0, too_long, 0);
  testrun(0 == ov_string_compare(result, too_long));

  result = ov_free(result);
  testrun(0 == result);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_string_dup() {

  testrun(0 == ov_string_dup(0));

  /* string not zero-terminated within the max amount of chars */

  char too_long[OV_STRING_DEFAULT_SIZE + 1];
  memset(too_long, 1, sizeof(too_long));

  testrun(0 == ov_string_dup(too_long));

  char const *good_string = "hugin";

  char *copy = ov_string_dup(good_string);

  testrun(0 != copy);
  testrun(0 == copy[strlen("hugin")]);
  testrun(0 == strcmp("hugin", copy));

  free(copy);

  too_long[OV_STRING_DEFAULT_SIZE - 1] = 0;
  copy = ov_string_dup(too_long);

  testrun(0 != copy);
  testrun(0 == copy[OV_STRING_DEFAULT_SIZE - 1]);

  for (size_t i = 0; OV_STRING_DEFAULT_SIZE - 1 > i; ++i) {

    testrun(1 == copy[i]);
  }

  free(copy);

  too_long[OV_STRING_DEFAULT_SIZE] = 0;

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_string_len() {

  testrun(0 == ov_string_len(0));

  char const *one = "1";
  testrun(strlen(one) == ov_string_len(one));

  char const *achtzehn = "18";
  testrun(strlen(achtzehn) == ov_string_len(achtzehn));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_string_compare() {

  testrun(0 == ov_string_compare(0, 0));
  testrun(0 != ov_string_compare(0, "munin"));
  testrun(0 != ov_string_compare("hugin", 0));

  testrun(0 == ov_string_compare("mimir", "mimir"));
  testrun(0 > ov_string_compare("hugin", "munin"));
  testrun(0 < ov_string_compare("munin", "mimir"));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_string_equal() {

  testrun(ov_string_equal(0, 0));
  testrun(!ov_string_equal(0, "munin"));
  testrun(!ov_string_equal("hugin", 0));

  testrun(ov_string_equal("mimir", "mimir"));
  testrun(!ov_string_equal("hugin", "munin"));
  testrun(!ov_string_equal("munin", "mimir"));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_string_equal_nocase() {

  testrun(ov_string_equal_nocase(0, 0));
  testrun(!ov_string_equal_nocase("abc", 0));
  testrun(!ov_string_equal_nocase(0, "abc"));
  testrun(ov_string_equal_nocase("a", "a"));
  testrun(ov_string_equal_nocase("a", "A"));
  testrun(ov_string_equal_nocase("A", "a"));
  testrun(ov_string_equal_nocase("A", "A"));
  testrun(ov_string_equal_nocase("ab", "ab"));
  testrun(ov_string_equal_nocase("Ab", "ab"));
  testrun(ov_string_equal_nocase("Ab", "ab"));
  testrun(ov_string_equal_nocase("AB", "ab"));
  testrun(ov_string_equal_nocase("ab", "Ab"));
  testrun(ov_string_equal_nocase("Ab", "aB"));
  testrun(ov_string_equal_nocase("Ab", "AB"));
  testrun(!ov_string_equal_nocase("A", "ab"));
  testrun(!ov_string_equal_nocase("Ab", "A"));
  testrun(!ov_string_equal_nocase("Ab", "a"));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_string_startswith() {

  testrun(!ov_string_startswith(0, 0));
  testrun(!ov_string_startswith(0, "abs"));
  testrun(!ov_string_startswith("cdef", 0));
  testrun(!ov_string_startswith("cdef", "abs"));
  testrun(ov_string_startswith("cdef", "c"));
  testrun(ov_string_startswith("cdef", "cd"));
  testrun(ov_string_startswith("cdef", "cde"));
  testrun(ov_string_startswith("cdef", "cdef"));
  testrun(!ov_string_startswith("cdef", "cdefg"));
  testrun(!ov_string_startswith("cdef", "cdf"));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_string_sanitize() {

  testrun(0 != ov_string_sanitize(0));

  char const *test_string = "Test String";

  testrun(test_string == ov_string_sanitize(test_string));

  return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      CONVERSIONS
 *
 *      ------------------------------------------------------------------------
 */

static int test_ov_string_to_uint16(void) {

  bool ok = true;
  char value_string[255];
  uint16_t testval;

  testrun((0 == ov_string_to_uint16("", &ok)) && !ok, "Empty string");
  ok = true;

  /* There should be more tests regarding invalid strings */

  testrun((0 == ov_string_to_uint16("a", &ok)) && !ok, "Invalid string");
  testrun((0 == ov_string_to_uint16("b", &ok)) && !ok, "Invalid string");
  ok = true;
  testrun((0 == ov_string_to_uint16("-1", &ok)) && !ok, "negative number");
  ok = true;

  for (testval = 0; testval < UINT16_MAX; testval++) {

    snprintf(value_string, 254, "%u", testval);
    testrun((testval == ov_string_to_uint16(value_string, &ok)) && ok,
            "Valid value");
  }

  snprintf(value_string, 254, "%" PRIu32, (uint32_t)(UINT16_MAX) + 1);
  ok = true;
  testrun((0 == ov_string_to_uint16(value_string, &ok)) && !ok,
          "Value overflow");

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_string_to_uint32(void) {

  bool ok = true;
  char value_string[255];
  uint32_t testval;

  testrun((0 == ov_string_to_uint32("", &ok)) && !ok, "Empty string");
  ok = true;

  /* There should be more tests regarding invalid strings */
  testrun((0 == ov_string_to_uint32("a", &ok)) && !ok, "Invalid string");
  testrun((0 == ov_string_to_uint32("b", &ok)) && !ok, "Invalid string");
  ok = true;
  testrun((0 == ov_string_to_uint32("-1", &ok)) && !ok, "negative number");
  ok = true;

  for (testval = 0; testval < UINT16_MAX; testval++) {

    snprintf(value_string, 254, "%u", testval);
    // fprintf(stderr, "Checking %s\n", value_string);
    testrun((testval == ov_string_to_uint32(value_string, &ok)) && ok,
            "Valid value");
  }

  for (testval = UINT32_MAX - UINT16_MAX; testval < UINT32_MAX; testval++) {

    snprintf(value_string, 254, "%u", testval);
    // fprintf(stderr, "Checking %s\n", value_string);
    testrun((testval == ov_string_to_uint32(value_string, &ok)) && ok,
            "Valid value");
  }

  snprintf(value_string, 254, "%" PRIu64, (uint64_t)(UINT32_MAX) + 1);

  ok = true;

  testrun((0 == ov_string_to_uint32(value_string, &ok)) && !ok,
          "Value overflow");

  return testrun_log_success();
}
/*----------------------------------------------------------------------------*/

int test_ov_string_to_uint64(void) {

  uint64_t size = 100;
  char buffer[size];
  memset(buffer, '\0', size);

  memcpy(buffer, "123", 3);

  uint64_t number = 0;

  testrun(!ov_string_to_uint64(NULL, 0, NULL));
  testrun(!ov_string_to_uint64(buffer, 3, NULL));
  testrun(!ov_string_to_uint64(buffer, 0, &number));
  testrun(!ov_string_to_uint64(NULL, 3, &number));

  testrun(ov_string_to_uint64(buffer, 3, &number));
  testrun(123 == number);

  testrun(ov_string_to_uint64(buffer, 3, &number));
  testrun(123 == number);

  testrun(ov_string_to_uint64(buffer, 2, &number));
  testrun(12 == number);

  testrun(ov_string_to_uint64(buffer, 1, &number));
  testrun(1 == number);

  testrun(ov_string_to_uint64(buffer, 30, &number));
  testrun(123 == number);

  memcpy(buffer, "-12345", 6);
  testrun(!ov_string_to_uint64(buffer, 30, &number));

  memcpy(buffer, "012345", 7);
  testrun(ov_string_to_uint64(buffer, 30, &number));
  testrun(12345 == number);

  memcpy(buffer, "123A45", 7);
  testrun(ov_string_to_uint64(buffer, 3, &number));
  testrun(123 == number);

  testrun(!ov_string_to_uint64(buffer, 4, &number));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_string_to_int64(void) {

  uint64_t size = 100;
  char buffer[size];
  memset(buffer, '\0', size);

  memcpy(buffer, "123", 3);

  int64_t number = 0;

  testrun(ov_string_to_int64(buffer, 3, &number));
  testrun(123 == number);

  testrun(ov_string_to_int64(buffer, 3, &number));
  testrun(123 == number);

  testrun(ov_string_to_int64(buffer, 2, &number));
  testrun(12 == number);

  testrun(ov_string_to_int64(buffer, 1, &number));
  testrun(1 == number);

  testrun(ov_string_to_int64(buffer, 30, &number));
  testrun(123 == number);

  memcpy(buffer, "-12345", 6);
  testrun(ov_string_to_int64(buffer, 30, &number));
  testrun(-12345 == number);

  memcpy(buffer, "012345", 7);
  testrun(ov_string_to_int64(buffer, 30, &number));
  testrun(12345 == number);

  memcpy(buffer, "123A45", 7);
  testrun(ov_string_to_int64(buffer, 3, &number));
  testrun(123 == number);

  testrun(!ov_string_to_int64(buffer, 4, &number));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_string_to_double(void) {

  uint64_t size = 100;
  char buffer[size];
  memset(buffer, '\0', size);

  memcpy(buffer, "123", 3);

  double number = 0;

  testrun(ov_string_to_double(buffer, 3, &number));
  testrun(123 == number);

  testrun(ov_string_to_double(buffer, 3, &number));
  testrun(123 == number);

  testrun(ov_string_to_double(buffer, 2, &number));
  testrun(12 == number);

  testrun(ov_string_to_double(buffer, 1, &number));
  testrun(1 == number);

  testrun(ov_string_to_double(buffer, 30, &number));
  testrun(123 == number);

  memcpy(buffer, "-12345", 6);
  testrun(ov_string_to_double(buffer, 30, &number));
  testrun(-12345 == number);

  memcpy(buffer, "012345", 7);
  testrun(ov_string_to_double(buffer, 30, &number));
  testrun(12345 == number);

  memcpy(buffer, "123A45", 7);
  testrun(ov_string_to_double(buffer, 3, &number));
  testrun(123 == number);

  testrun(!ov_string_to_double(buffer, 4, &number));

  memcpy(buffer, "-1.2345", strlen("-1.2345"));
  testrun(ov_string_to_double(buffer, 30, &number));
  testrun(-1.2345 == number);

  memset(buffer, '\0', size);
  memcpy(buffer, "-1e5", strlen("-1e5"));
  testrun(ov_string_to_double(buffer, 30, &number));
  testrun(-1.e5 == number);

  return testrun_log_success();
}

/*****************************************************************************
                                      TRIM
 ****************************************************************************/

static bool check_ltrimmed_equals(char const *in, char const *chars,
                                  char const *ref) {

  char *in_dup = strdup(in);

  char const *trimmed = ov_string_ltrim(in_dup, chars);

  bool equals = (0 == ov_string_compare(trimmed, ref));

  free(in_dup);

  return equals;
}

/*----------------------------------------------------------------------------*/

int test_ov_string_ltrim() {

  testrun(0 == ov_string_ltrim(0, 0));
  testrun(0 == ov_string_ltrim(" abcd", 0));

  testrun(check_ltrimmed_equals("", " ", ""));
  testrun(check_ltrimmed_equals(" abcd", " ", "abcd"));
  testrun(check_ltrimmed_equals(" \t   abcd", " \t", "abcd"));
  testrun(check_ltrimmed_equals(" \t   abcd", "ab \t", "cd"));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool check_rtrimmed_equals(char const *in, char const *chars,
                                  char const *ref) {

  char *in_dup = strdup(in);
  char *trimmed = ov_string_rtrim(in_dup, chars);

  bool equals = (0 == ov_string_compare(trimmed, ref));

  ov_free(in_dup);

  return equals;
}

/*----------------------------------------------------------------------------*/

int test_ov_string_rtrim() {

  testrun(0 == ov_string_rtrim(0, 0));
  testrun(0 == ov_string_rtrim(" abcd", 0));

  testrun(check_rtrimmed_equals("", " ", ""));
  testrun(check_rtrimmed_equals(" abcd", " ", " abcd"));
  testrun(check_rtrimmed_equals(" \t   abcd", " \t", " \t   abcd"));
  testrun(check_rtrimmed_equals(" \t   abcd", "ab \t", " \t   abcd"));

  testrun(check_rtrimmed_equals(" abcd ", " ", " abcd"));
  testrun(check_rtrimmed_equals(" abcd   ", " ", " abcd"));
  testrun(check_rtrimmed_equals(" \t   abcd   \t  ", " \t", " \t   abcd"));
  testrun(check_rtrimmed_equals(" \t   abcd\t   \t\t", "dc \t", " \t   ab"));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_string_value_for_key() {

  testrun(0 == ov_string_value_for_key(0, 0, 0, 0));
  testrun(0 == ov_string_value_for_key("a=1", 0, 0, 0));
  testrun(0 == ov_string_value_for_key(0, "a", 0, 0));
  testrun(0 == ov_string_value_for_key("a=1", "a", 0, 0));
  testrun(0 == ov_string_value_for_key(0, 0, '=', 0));
  testrun(0 == ov_string_value_for_key("a=1", 0, '=', 0));
  testrun(0 == ov_string_value_for_key(0, "a", '=', 0));
  testrun(0 == ov_string_value_for_key("a=1", "a", '=', 0));
  testrun(0 == ov_string_value_for_key(0, 0, 0, ";"));
  testrun(0 == ov_string_value_for_key("a=1", 0, 0, ";"));
  testrun(0 == ov_string_value_for_key(0, "a", 0, ";"));
  testrun(0 == ov_string_value_for_key("a=1", "a", 0, ";"));
  testrun(0 == ov_string_value_for_key(0, 0, '=', ";"));
  testrun(0 == ov_string_value_for_key("a=1", 0, '=', ";"));
  testrun(0 == ov_string_value_for_key(0, "a", '=', ";"));

  testrun(0 == ov_string_value_for_key("a=1", "b", '=', ";"));

  ov_buffer *value = ov_string_value_for_key("a=1", "a", '=', ";");
  testrun(ov_buffer_equals(value, "1"));
  value = ov_buffer_free(value);

  value = ov_string_value_for_key("b=1;a=2", "a", '=', ";");
  testrun(ov_buffer_equals(value, "2"));
  value = ov_buffer_free(value);

  value = ov_string_value_for_key("b=1;a=2", "b", '=', ";");
  testrun(ov_buffer_equals(value, "1"));
  value = ov_buffer_free(value);

  // Effectively do not split into several kv pairs at all ...
  value = ov_string_value_for_key("b=1;a=2", "b", '=', "");
  testrun(ov_buffer_equals(value, "1;a=2"));
  value = ov_buffer_free(value);

  testrun(0 == ov_string_value_for_key("b=1;a=2", "a", '=', ""));
  testrun(0 == ov_string_value_for_key("b=1;a=2", "a", '=', ","));
  testrun(0 == ov_string_value_for_key("b=1;a=2", "a", ':', ";"));

  value = ov_string_value_for_key("b=1,a=2,c=3", "c", '=', ",");
  testrun(ov_buffer_equals(value, "3"));
  value = ov_buffer_free(value);

  value = ov_string_value_for_key("b:1,a:2,c:3", "c", ':', ",");
  testrun(ov_buffer_equals(value, "3"));
  value = ov_buffer_free(value);

  value = ov_string_value_for_key("b=1,a=2,c=3", "a", '=', ",");
  testrun(ov_buffer_equals(value, "2"));
  value = ov_buffer_free(value);

  value = ov_string_value_for_key("b=1,a=2,c=3", "b", '=', ",");
  testrun(ov_buffer_equals(value, "1"));
  value = ov_buffer_free(value);

  // Case encountered in the wild
  value = ov_string_value_for_key("<sip:7247@127.0.0.1>;tag=csbCJl~dW", "tag",
                                  '=', ";");
  testrun(ov_buffer_equals(value, "csbCJl~dW"));
  value = ov_buffer_free(value);

  return testrun_log_success();
}

/*****************************************************************************
                                OV_DATAFUNCTIONS
 ****************************************************************************/

int test_ov_string_data_functions() {

  ov_data_function functions = ov_string_data_functions();

  testrun(functions.clear == ov_string_data_clear);
  testrun(functions.free == ov_string_data_free);
  testrun(functions.copy == ov_string_data_copy);
  testrun(functions.dump == ov_string_data_dump);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_string_data_clear() {

  char *string = strdup("test");

  testrun(string[0] == 't');

  testrun(!ov_string_data_clear(NULL));
  testrun(ov_string_data_clear(string));

  testrun(string[0] == 0);
  testrun(strlen(string) == 0);

  free(string);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_string_data_free() {

  char *string = strdup("test");

  testrun(NULL == ov_string_data_free(NULL));
  testrun(NULL == ov_string_data_free(string));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_string_data_copy() {

  char *string = strdup("test");
  char *copy = NULL;

  testrun(!ov_string_data_copy(NULL, NULL));
  testrun(!ov_string_data_copy((void **)&copy, NULL));
  testrun(!ov_string_data_copy(NULL, string));

  testrun(ov_string_data_copy((void **)&copy, string));

  // check same content
  testrun(strncmp(string, copy, strlen(string)) == 0);

  // check independent content
  string = ov_string_data_free(string);
  testrun(strncmp("test", copy, strlen(copy)) == 0);
  copy = ov_string_data_free(copy);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_string_data_dump() {

  char *string = strdup("test");

  testrun(string[0] == 't');

  testrun(!ov_string_data_dump(NULL, NULL));
  testrun(!ov_string_data_dump(stdout, NULL));
  testrun(!ov_string_data_dump(NULL, string));

  testrun_log("TEST DUMP FOLLOWS");
  testrun(ov_string_data_dump(stdout, string));

  free(string);
  return testrun_log_success();
}
/*
 *      ------------------------------------------------------------------------
 *
 *                        STRING PARING
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_string_pointer() {

  ov_list *list = NULL;
  size_t size = 100;
  char source[size];
  char *delim = "\n";
  size_t i = 0;

  for (i = 0; i < size; i += 2) {
    source[i] = 'x';
    source[i + 1] = '\n';
  }
  source[size] = 0;

  testrun(!ov_string_pointer(NULL, 0, NULL, 0));
  testrun(!ov_string_pointer(NULL, 1, delim, 1));
  testrun(!ov_string_pointer(source, 0, delim, 1));
  testrun(!ov_string_pointer(source, 1, NULL, 1));
  testrun(!ov_string_pointer(source, 1, delim, 0));

  // -------------------------------------------------------------
  // Example testing 1
  // -------------------------------------------------------------

  list = ov_string_pointer(source, size, delim, strlen(delim));
  testrun(list);
  testrun(list->count(list) == 50);

  for (i = 1; i <= list->count(list); i++) {
    testrun(strncmp(list->get(list, i), "x", 1) == 0);
  }
  list = (ov_list *)list->free(list);

  // -------------------------------------------------------------
  // Example testing 2
  // -------------------------------------------------------------

  delim = "123";
  snprintf(source, size, "abc%sdefg%shijkl%sxyz", delim, delim, delim);

  list = ov_string_pointer(source, size, delim, strlen(delim));
  testrun(list);
  testrun(list->count(list) == 4);
  testrun(strncmp(list->get(list, 1), "abc123defg123hijkl123xyz",
                  strlen("abc123defg123hijkl123xyz")) == 0);
  testrun(strncmp(list->get(list, 2), "defg123hijkl123xyz",
                  strlen("defg123hijkl123xyz")) == 0);

  testrun(strncmp(list->get(list, 3), "hijkl123xyz", strlen("hijkl123xyz")) ==
          0);

  testrun(strncmp(list->get(list, 4), "xyz", strlen("xyz")) == 0);
  list = (ov_list *)list->free(list);

  // -------------------------------------------------------------
  // DELIMITER NOT CONTAINED
  // -------------------------------------------------------------

  delim = "123";
  snprintf(source, size, "abc\ndefg\nhijkl\nxyz\n");

  list = ov_string_pointer(source, size, delim, strlen(delim));
  testrun(list);
  testrun(list->count(list) == 1);
  testrun(strncmp(list->get(list, 1), "abc\ndefg\nhijkl\nxyz\n",
                  strlen("abc\ndefg\nhijkl\nxyz\n")) == 0);
  list = (ov_list *)list->free(list);

  // -------------------------------------------------------------
  // DELIMITER LAST
  // -------------------------------------------------------------

  delim = "\n";
  snprintf(source, size, "abc\ndefg\nhijkl\nxyz\n");

  list = ov_string_pointer(source, size, delim, strlen(delim));
  testrun(list);

  // ov_list_dump(stdout, list);

  testrun(list->count(list) == 4);
  testrun(strncmp(list->get(list, 1), "abc\ndefg\nhijkl\nxyz\n",
                  strlen("abc\ndefg\nhijkl\nxyz\n")) == 0);
  testrun(strncmp(list->get(list, 2), "defg\nhijkl\nxyz\n",
                  strlen("defg\nhijkl\nxyz\n")) == 0);
  testrun(strncmp(list->get(list, 3), "hijkl\nxyz\n", strlen("hijkl\nxyz\n")) ==
          0);
  testrun(strncmp(list->get(list, 4), "xyz\n", strlen("xyz\n")) == 0);
  list = (ov_list *)list->free(list);

  // -------------------------------------------------------------
  // DELIMITER FIRST
  // -------------------------------------------------------------

  delim = "\n";
  snprintf(source, size, "\nabc\ndefg\nhijkl\nxyz\n");

  list = ov_string_pointer(source, size, delim, strlen(delim));
  testrun(list);

  // ov_list_dump(stdout, list);

  testrun(list->count(list) == 5);
  testrun(strncmp(list->get(list, 1), "\nabc\ndefg\nhijkl\nxyz\n",
                  strlen("\nabc\ndefg\nhijkl\nxyz\n")) == 0);
  testrun(strncmp(list->get(list, 2), "abc\ndefg\nhijkl\nxyz\n",
                  strlen("abc\ndefg\nhijkl\nxyz\n")) == 0);
  testrun(strncmp(list->get(list, 3), "defg\nhijkl\nxyz\n",
                  strlen("defg\nhijkl\nxyz\n")) == 0);
  testrun(strncmp(list->get(list, 4), "hijkl\nxyz\n", strlen("hijkl\nxyz\n")) ==
          0);
  testrun(strncmp(list->get(list, 5), "xyz\n", strlen("xyz\n")) == 0);
  list = (ov_list *)list->free(list);

  // -------------------------------------------------------------
  // DELIMITER Partial last
  // -------------------------------------------------------------

  delim = "123";
  snprintf(source, size, "a123b123c12d");

  list = ov_string_pointer(source, size, delim, strlen(delim));
  testrun(list);

  // ov_list_dump(stdout, list);

  testrun(list->count(list) == 3);
  testrun(strncmp(list->get(list, 1), "a123b123c12d", strlen("a123b123c12d")) ==
          0);
  testrun(strncmp(list->get(list, 2), "b123c12d", strlen("b123c12d")) == 0);
  testrun(strncmp(list->get(list, 3), "c12d", strlen("c12d")) == 0);
  list = (ov_list *)list->free(list);

  // -------------------------------------------------------------
  // DELIMITER Partial last
  // -------------------------------------------------------------

  delim = "123";
  snprintf(source, size, "a123b123c12");

  list = ov_string_pointer(source, size, delim, strlen(delim));
  testrun(list);

  // ov_list_dump(stdout, list);

  testrun(list->count(list) == 3);
  testrun(strncmp(list->get(list, 1), "a123b123c12", strlen("a123b123c12")) ==
          0);
  testrun(strncmp(list->get(list, 2), "b123c12", strlen("b123c12")) == 0);
  testrun(strncmp(list->get(list, 3), "c12", strlen("c12")) == 0);
  list = (ov_list *)list->free(list);

  // -------------------------------------------------------------
  // DELIMITER EMPTY
  // -------------------------------------------------------------

  delim = "\n";
  snprintf(source, size, "\nabc\ndefg\n\nxyz\n");

  list = ov_string_pointer(source, size, delim, strlen(delim));
  testrun(list);

  // ov_list_dump(stdout, list);

  testrun(list->count(list) == 5);
  testrun(strncmp(list->get(list, 1), "\nabc\ndefg\n\nxyz\n",
                  strlen("\nabc\ndefg\n\nxyz\n")) == 0);
  testrun(strncmp(list->get(list, 2), "abc\ndefg\n\nxyz\n",
                  strlen("abc\ndefg\n\nxyz\n")) == 0);
  testrun(strncmp(list->get(list, 3), "defg\n\nxyz\n",
                  strlen("defg\n\nxyz\n")) == 0);
  testrun(strncmp(list->get(list, 4), "\nxyz\n", strlen("\nxyz\n")) == 0);
  testrun(strncmp(list->get(list, 5), "xyz\n", strlen("xyz\n")) == 0);

  list = (ov_list *)list->free(list);

  // -------------------------------------------------------------
  // CHECK internal delimiter
  // -------------------------------------------------------------

  memset(source, '\0', size);
  delim = "/";
  snprintf(source, size, "P/R/O/T/O/1/2/3");

  list = ov_string_pointer(source, strlen(source), delim, strlen(delim));
  testrun(list);

  // ov_list_dump(stdout, list);

  testrun(list->count(list) == 8);
  testrun(strncmp(list->get(list, 1), "P/R/O/T/O/1/2/3",
                  strlen("P/R/O/T/O/1/2/3")) == 0);
  testrun(strncmp(list->get(list, 2), "R/O/T/O/1/2/3",
                  strlen("R/O/T/O/1/2/3")) == 0);
  testrun(strncmp(list->get(list, 3), "O/T/O/1/2/3", strlen("O/T/O/1/2/3")) ==
          0);
  testrun(strncmp(list->get(list, 4), "T/O/1/2/3", strlen("T/O/1/2/3")) == 0);
  testrun(strncmp(list->get(list, 5), "O/1/2/3", strlen("O/1/2/3")) == 0);
  testrun(strncmp(list->get(list, 6), "1/2/3", strlen("1/2/3")) == 0);
  testrun(strncmp(list->get(list, 7), "2/3", strlen("2/3")) == 0);
  testrun(strncmp(list->get(list, 8), "3", strlen("3")) == 0);

  list = (ov_list *)list->free(list);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_string_split() {

  ov_list *list = NULL;
  size_t size = 1000;
  char source[size];
  char *delim = "\n";
  size_t i = 0;

  memset(source, 0, size);

  for (i = 0; i < size; i += 2) {
    source[i] = 'x';
    source[i + 1] = '\n';
  }
  source[size] = 0;

  testrun(!ov_string_split(NULL, 0, NULL, 0, false));
  testrun(!ov_string_split(NULL, 1, delim, 1, false));
  testrun(!ov_string_split(source, 0, delim, 1, false));
  testrun(!ov_string_split(source, 1, NULL, 1, false));
  testrun(!ov_string_split(source, 1, delim, 0, false));

  // -------------------------------------------------------------
  // Example testing 1
  // -------------------------------------------------------------

  list = ov_string_split(source, size, delim, strlen(delim), false);
  testrun(list);
  testrun(list->count(list) == 500);
  // ov_list_dump(stdout, list);

  for (i = 1; i <= list->count(list); i++) {
    testrun(strncmp(list->get(list, i), "x", 1) == 0);
  }
  list = (ov_list *)list->free(list);

  // -------------------------------------------------------------
  // Example testing 2
  // -------------------------------------------------------------

  delim = "123";
  memset(source, 0, size);
  snprintf(source, size, "abc%sdefg%shijkl%sxyz", delim, delim, delim);

  list = ov_string_split(source, size, delim, strlen(delim), false);
  testrun(list);
  testrun(list->count(list) == 4);
  testrun(strncmp(list->get(list, 1), "abc", strlen("abc")) == 0);
  testrun(strncmp(list->get(list, 2), "defg", strlen("defg")) == 0);

  testrun(strncmp(list->get(list, 3), "hijkl", strlen("hijkl")) == 0);

  testrun(strncmp(list->get(list, 4), "xyz", strlen("xyz")) == 0);
  list = (ov_list *)list->free(list);

  // -------------------------------------------------------------
  // DELIMITER NOT CONTAINED
  // -------------------------------------------------------------

  delim = "123";
  snprintf(source, size, "abc\ndefg\nhijkl\nxyz\n");

  list = ov_string_split(source, size, delim, strlen(delim), false);
  testrun(list);
  testrun(list->count(list) == 1);
  testrun(strncmp(list->get(list, 1), "abc\ndefg\nhijkl\nxyz\n",
                  strlen("abc\ndefg\nhijkl\nxyz\n")) == 0);
  list = (ov_list *)list->free(list);

  // -------------------------------------------------------------
  // DELIMITER LAST
  // -------------------------------------------------------------

  delim = "\n";
  snprintf(source, size, "abc\ndefg\nhijkl\nxyz\n");

  list = ov_string_split(source, size, delim, strlen(delim), false);
  testrun(list);

  // ov_list_dump(stdout, list);

  testrun(list->count(list) == 4);
  testrun(strncmp(list->get(list, 1), "abc", strlen("abc")) == 0);
  testrun(strncmp(list->get(list, 2), "defg", strlen("defg")) == 0);
  testrun(strncmp(list->get(list, 3), "hijkl", strlen("hijkl")) == 0);
  testrun(strncmp(list->get(list, 4), "xyz", strlen("xyz")) == 0);
  list = (ov_list *)list->free(list);

  list = ov_string_split(source, size, delim, strlen(delim), true);
  testrun(list);

  // ov_list_dump(stdout, list);

  testrun(list->count(list) == 4);
  testrun(strncmp(list->get(list, 1), "abc\n", strlen("abc\n")) == 0);
  testrun(strncmp(list->get(list, 2), "defg\n", strlen("defg\n")) == 0);
  testrun(strncmp(list->get(list, 3), "hijkl\n", strlen("hijkl\n")) == 0);
  testrun(strncmp(list->get(list, 4), "xyz\n", strlen("xyz\n")) == 0);
  list = (ov_list *)list->free(list);

  // -------------------------------------------------------------
  // DELIMITER FIRST
  // -------------------------------------------------------------

  delim = "\n";
  snprintf(source, size, "\nabc\ndefg\nhijkl\nxyz\n");

  list = ov_string_split(source, size, delim, strlen(delim), false);
  testrun(list);

  // ov_list_dump(stdout, list);

  testrun(list->count(list) == 5);
  testrun(NULL == list->get(list, 1));
  testrun(strncmp(list->get(list, 2), "abc", strlen("abc")) == 0);
  testrun(strncmp(list->get(list, 3), "defg", strlen("defg")) == 0);
  testrun(strncmp(list->get(list, 4), "hijkl", strlen("hijkl")) == 0);
  testrun(strncmp(list->get(list, 5), "xyz", strlen("xyz")) == 0);
  list = (ov_list *)list->free(list);

  list = ov_string_split(source, size, delim, strlen(delim), true);
  testrun(list);

  // ov_list_dump(stdout, list);

  testrun(list->count(list) == 5);
  testrun(strncmp(list->get(list, 1), "\n", strlen("\n")) == 0);
  testrun(strncmp(list->get(list, 2), "abc\n", strlen("abc\n")) == 0);
  testrun(strncmp(list->get(list, 3), "defg\n", strlen("defg\n")) == 0);
  testrun(strncmp(list->get(list, 4), "hijkl\n", strlen("hijkl\n")) == 0);
  testrun(strncmp(list->get(list, 5), "xyz\n", strlen("xyz\n")) == 0);
  list = (ov_list *)list->free(list);

  // -------------------------------------------------------------
  // DELIMITER EMPTY
  // -------------------------------------------------------------

  delim = "\n";
  snprintf(source, size, "\nabc\ndefg\n\nxyz\n");

  list = ov_string_split(source, size, delim, 1, false);
  testrun(list);

  testrun(list->count(list) == 5);
  testrun(NULL == list->get(list, 1));
  testrun(strncmp(list->get(list, 2), "abc", strlen("abc")) == 0);
  testrun(strncmp(list->get(list, 3), "defg", strlen("defg")) == 0);
  testrun(NULL == list->get(list, 4));
  testrun(strncmp(list->get(list, 5), "xyz", strlen("xyz")) == 0);
  list = (ov_list *)list->free(list);

  list = ov_string_split(source, size, delim, strlen(delim), true);
  testrun(list);

  // ov_list_dump(stdout, list);

  testrun(list->count(list) == 5);
  testrun(strncmp(list->get(list, 1), "\n", strlen("\n")) == 0);
  testrun(strncmp(list->get(list, 2), "abc\n", strlen("abc\n")) == 0);
  testrun(strncmp(list->get(list, 3), "defg\n", strlen("defg\n")) == 0);
  testrun(strncmp(list->get(list, 4), "\n", strlen("\n")) == 0);
  testrun(strncmp(list->get(list, 5), "xyz\n", strlen("xyz\n")) == 0);
  list = (ov_list *)list->free(list);

  // -------------------------------------------------------------
  // DELIMITER Partial last
  // -------------------------------------------------------------

  delim = "123";
  snprintf(source, size, "a123b123c12d");

  list = ov_string_split(source, size, delim, strlen(delim), false);
  testrun(list);

  // ov_list_dump(stdout, list);

  testrun(list->count(list) == 3);
  testrun(strncmp(list->get(list, 1), "a", strlen("a")) == 0);
  testrun(strncmp(list->get(list, 2), "b", strlen("b")) == 0);
  testrun(strncmp(list->get(list, 3), "c12d", strlen("c12d")) == 0);
  list = (ov_list *)list->free(list);

  list = ov_string_split(source, size, delim, strlen(delim), true);
  testrun(list);

  // ov_list_dump(stdout, list);

  testrun(list->count(list) == 3);
  testrun(strncmp(list->get(list, 1), "a123", strlen("a123")) == 0);
  testrun(strncmp(list->get(list, 2), "b123", strlen("b123")) == 0);
  testrun(strncmp(list->get(list, 3), "c12d", strlen("c12d")) == 0);
  list = (ov_list *)list->free(list);

  // -------------------------------------------------------------
  // DELIMITER Partial last
  // -------------------------------------------------------------

  delim = "123";
  snprintf(source, size, "a123b123c12");

  list = ov_string_split(source, size, delim, strlen(delim), false);
  testrun(list);

  // ov_list_dump(stdout, list);

  testrun(list->count(list) == 3);
  testrun(strncmp(list->get(list, 1), "a", strlen("a")) == 0);
  testrun(strncmp(list->get(list, 2), "b", strlen("b")) == 0);
  testrun(strncmp(list->get(list, 3), "c12", strlen("c12")) == 0);
  list = (ov_list *)list->free(list);

  list = ov_string_split(source, size, delim, strlen(delim), true);
  testrun(list);

  // ov_list_dump(stdout, list);

  testrun(list->count(list) == 3);
  testrun(strncmp(list->get(list, 1), "a123", strlen("a123")) == 0);
  testrun(strncmp(list->get(list, 2), "b123", strlen("b123")) == 0);
  testrun(strncmp(list->get(list, 3), "c12", strlen("c12")) == 0);
  list = (ov_list *)list->free(list);

  return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *                        STRING COMBINE
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

int test_ov_string_append() {

  char *dest = NULL;
  char *src = NULL;
  char *expect = NULL;

  size_t d_size = 0;
  size_t s_size = 0;

  testrun(false == ov_string_append(NULL, NULL, NULL, 0));
  testrun(false == ov_string_append(NULL, &d_size, src, s_size));
  testrun(false == ov_string_append(&dest, NULL, src, s_size));
  testrun(false == ov_string_append(&dest, &d_size, NULL, s_size));
  testrun(false == ov_string_append(&dest, &d_size, src, 0));

  // -------------------------------------------------------------
  // Positive test, no realloc
  // -------------------------------------------------------------

  d_size = 10;
  dest = calloc(d_size, sizeof(char));
  strcat(dest, "test");
  src = "1";
  expect = "test1";

  testrun(true == ov_string_append(&dest, &d_size, src, strlen(src)));
  testrun(strncmp(expect, dest, strlen(expect)) == 0);
  testrun(d_size == 10);

  free(dest);
  dest = NULL;

  // -------------------------------------------------------------
  // Positive test, no realloc
  // -------------------------------------------------------------

  d_size = 6;
  dest = calloc(d_size, sizeof(char));
  strcat(dest, "test");
  src = "1";
  expect = "test1";

  testrun(true == ov_string_append(&dest, &d_size, src, strlen(src)));
  testrun(strncmp(expect, dest, strlen(expect)) == 0);
  testrun(d_size == 6);

  free(dest);
  dest = NULL;

  // -------------------------------------------------------------
  // Positive test, realloc
  // -------------------------------------------------------------

  d_size = 5;
  dest = calloc(d_size, sizeof(char));
  strcat(dest, "test");
  src = "1";
  expect = "test1";

  testrun(true == ov_string_append(&dest, &d_size, src, strlen(src)));
  testrun(strncmp(expect, dest, strlen(expect)) == 0);
  testrun(d_size == 6);

  free(dest);
  dest = NULL;

  // -------------------------------------------------------------
  // Allocate string
  // -------------------------------------------------------------

  dest = NULL;
  d_size = 0;

  src = "1";
  expect = "1";

  testrun(true == ov_string_append(&dest, &d_size, src, strlen(src)));
  testrun(strncmp(expect, dest, strlen(expect)) == 0);
  testrun(d_size == 2);

  free(dest);
  dest = NULL;

  // -------------------------------------------------------------
  // Check append not CUT
  // -------------------------------------------------------------

  d_size = 15;
  dest = calloc(d_size, sizeof(char));
  strcat(dest, "0123456789");
  src = "xxx";
  expect = "0123456789xxx";

  // dest length shorter dest string
  d_size = 9;
  expect = "012345678xxx";
  testrun(true == ov_string_append(&dest, &d_size, src, strlen(src)));
  testrun(strncmp(expect, dest, strlen(expect)) == 0);
  testrun(d_size == strlen(expect) + 1);

  free(dest);
  dest = NULL;

  // -------------------------------------------------------------
  // Check source len bound to src string length
  // -------------------------------------------------------------

  d_size = 15;
  dest = calloc(d_size, sizeof(char));
  strcat(dest, "0123456789");
  src = "xxx";
  expect = "0123456789xxx";

  // source len > strlen(source)
  testrun(true == ov_string_append(&dest, &d_size, src, 4));
  testrun(strncmp(expect, dest, strlen(expect)) == 0);
  testrun(d_size == 15);

  strcpy(dest, "0123456789");
  d_size = 15;

  // source len  == strlen(source)
  testrun(true == ov_string_append(&dest, &d_size, src, 3));
  testrun(strncmp(expect, dest, strlen(expect)) == 0);
  testrun(d_size == 15);

  free(dest);
  dest = NULL;

  // -------------------------------------------------------------
  // Check source len < strlen(source)
  // -------------------------------------------------------------

  d_size = 15;
  dest = calloc(d_size, sizeof(char));
  strcat(dest, "0123456789");
  src = "xxx";
  expect = "0123456789xx";

  // source len  < strlen(source)
  testrun(true == ov_string_append(&dest, &d_size, src, 2));
  testrun(strncmp(expect, dest, strlen(expect)) == 0);
  testrun(d_size == 15);

  free(dest);
  dest = NULL;

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_string_replace_all() {

  size_t size = 0;
  char *ptr = NULL;

  char *source = "1\n2\n3\n4";
  char *delim1 = "\n";
  char *delim2 = ":";
  char *expect = "1:2:3:4";

  testrun(false ==
          ov_string_replace_all(NULL, NULL, NULL, 0, NULL, 0, NULL, 0, false));

  testrun(false == ov_string_replace_all(NULL, &size, source, strlen(source),
                                         delim1, strlen(delim1), NULL, 0,
                                         false));

  testrun(false == ov_string_replace_all(&ptr, NULL, source, strlen(source),
                                         delim1, strlen(delim1), NULL, 0,
                                         false));

  testrun(false == ov_string_replace_all(&ptr, &size, NULL, strlen(source),
                                         delim1, strlen(delim1), NULL, 0,
                                         false));

  testrun(false == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                         NULL, strlen(delim1), NULL, 0, false));

  testrun(false == ov_string_replace_all(&ptr, &size, source, 0, delim1,
                                         strlen(delim1), NULL, 0, false));

  testrun(false == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                         delim1, 0, NULL, 0, false));

  // -------------------------------------------------------------
  // exchange with NULL (unset all delimiters)
  // -------------------------------------------------------------

  expect = "1234";
  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), NULL, 0,
                                        false));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), NULL, 0, true));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  // -------------------------------------------------------------
  // exchange with delimiter of same size
  // -------------------------------------------------------------

  source = "1\n2\n3\n4";
  delim1 = "\n";
  delim2 = ":";
  expect = "1:2:3:4";

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), false));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  expect = "1:2:3:4:";

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), true));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  // -------------------------------------------------------------
  // exchange with delimiter of bigger size
  // -------------------------------------------------------------

  source = "1\n2\n3\n4";
  delim1 = "\n";
  delim2 = ":::";
  expect = "1:::2:::3:::4";

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), false));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  expect = "1:::2:::3:::4:::";

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), true));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  // -------------------------------------------------------------
  // exchange with delimiter of smaller size
  // -------------------------------------------------------------

  source = "1:::2:::3:::4";
  delim1 = ":::";
  delim2 = " ";
  expect = "1 2 3 4";

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), false));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  expect = "1 2 3 4 ";

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), true));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  // -------------------------------------------------------------
  // delimiter not found
  // -------------------------------------------------------------

  source = "1:::2:::3:::4";
  delim1 = " ";
  delim2 = "\r\n";
  expect = source;

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), false));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), true));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  // -------------------------------------------------------------
  // exchange with same delimiter
  // -------------------------------------------------------------

  source = "1 2 3 4";
  delim1 = " ";
  delim2 = " ";
  expect = source;

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), false));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), true));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  // -------------------------------------------------------------
  // delimiter first
  // -------------------------------------------------------------

  source = " 1 2 3 4";
  delim1 = " ";
  delim2 = "x";
  expect = "x1x2x3x4";

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), false));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  expect = "x1x2x3x4x";

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), true));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  // -------------------------------------------------------------
  // delimiter last
  // -------------------------------------------------------------

  source = "1 2 3 4 ";
  delim1 = " ";
  delim2 = "x";
  expect = "1x2x3x4";

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), false));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  expect = "1x2x3x4x";

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), true));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  // -------------------------------------------------------------
  // Inside string replacement
  // -------------------------------------------------------------

  source = "Whatever ";
  delim1 = "at";
  delim2 = "en";
  expect = "Whenever";

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), false));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), true));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  source = "CallCall ";
  delim1 = "ll";
  delim2 = "xx";
  expect = "CaxxCaxx";

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), false));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  expect = "CaxxCa";

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), true));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  // -------------------------------------------------------------
  // Inside string replacement (Growing string)
  // -------------------------------------------------------------

  source = "abcxxdef";
  delim1 = "xx";
  delim2 = "0123456789";
  expect = "abc0123456789def";

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), false));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), true));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  // -------------------------------------------------------------
  // Inside string replacement (Shrinking string)
  // -------------------------------------------------------------

  source = "abc0123456789def";
  delim1 = "0123456789";
  delim2 = "xx";
  expect = "abcxxdef";

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), false));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), true));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  // -------------------------------------------------------------
  // Example testing
  // -------------------------------------------------------------

  size = 0;
  free(ptr);
  ptr = NULL;

  source = "whenever_whenever";
  delim1 = "en";
  delim2 = "_at_";
  expect = "wh_at_ever_wh_at_ever";

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), false));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  expect = "wh_at_ever_wh_at_ever_at_";

  testrun(true == ov_string_replace_all(&ptr, &size, source, strlen(source),
                                        delim1, strlen(delim1), delim2,
                                        strlen(delim2), true));

  testrun(strncmp(expect, ptr, strlen(expect)) == 0);
  testrun(size == OV_STRING_DEFAULT_SIZE);

  size = 0;
  free(ptr);
  ptr = NULL;

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_string_find() {

  char *source = "abcd123ef";
  char *delim = "123";
  const char *result = NULL;

  testrun(!ov_string_find(NULL, 0, NULL, 0));
  testrun(ov_string_find(source, strlen(source), delim, strlen(delim)));
  testrun(!ov_string_find(NULL, strlen(source), delim, strlen(delim)));
  testrun(!ov_string_find(source, 0, delim, strlen(delim)));
  testrun(!ov_string_find(source, strlen(source), NULL, strlen(delim)));
  testrun(!ov_string_find(source, strlen(source), delim, 0));

  result = ov_string_find(source, strlen(source), delim, strlen(delim));
  testrun(result == source + 4);

  result = ov_string_find(source, 7, delim, strlen(delim));
  testrun(result == source + 4);

  result = ov_string_find(source, 6, delim, strlen(delim));
  testrun(!result);

  delim = "d";
  result = ov_string_find(source, strlen(source), delim, strlen(delim));
  testrun(result == source + 3);
  result = ov_string_find(source, 4, delim, strlen(delim));
  testrun(result == source + 3);
  result = ov_string_find(source, 3, delim, strlen(delim));
  testrun(!result);

  delim = "D";
  result = ov_string_find(source, strlen(source), delim, strlen(delim));
  testrun(!result);

  delim = "abcd123ef";
  result = ov_string_find(source, strlen(source), delim, strlen(delim));
  testrun(result);
  testrun(result == source);
  result = ov_string_find(source, strlen(source), delim, strlen(delim) - 1);
  testrun(result);
  result = ov_string_find(source, strlen(source) - 1, delim, strlen(delim));
  testrun(!result);

  result = ov_string_find(source, strlen(source) - 1, delim, strlen(delim) - 1);
  testrun(result == source);

  // some more tests included in general test flow
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_string_parse_hex_digits() {

  char *str = "0x";
  char *next = NULL;

  testrun(0 == ov_string_parse_hex_digits(NULL, 0, NULL));
  testrun(0 == ov_string_parse_hex_digits(str, 0, &next));
  testrun(next == str);
  testrun(0 == ov_string_parse_hex_digits(str, 0, NULL));
  testrun(0 == ov_string_parse_hex_digits(NULL, 0, &next));
  testrun(next == NULL);

  testrun(0 == ov_string_parse_hex_digits(str, 1, &next));
  testrun(next == str + 1);

  testrun(0 == ov_string_parse_hex_digits(str, 2, &next));
  testrun(next == str + 1);

  str = "1F";
  testrun(0x1F == ov_string_parse_hex_digits(str, strlen(str), &next));
  testrun(next == str + 2);

  str = "C";
  testrun(0x0C == ov_string_parse_hex_digits(str, strlen(str), &next));
  testrun(next == str + 1);

  str = "EC";
  testrun(0xEC == ov_string_parse_hex_digits(str, strlen(str), &next));
  testrun(next == str + 2);

  str = "ECx";
  testrun(0xEC == ov_string_parse_hex_digits(str, strlen(str), &next));
  testrun(next == str + 2);

  str = "ecx";
  testrun(0xEC == ov_string_parse_hex_digits(str, strlen(str), &next));
  testrun(next == str + 2);

  str = "12ecx";
  testrun(0x12EC == ov_string_parse_hex_digits(str, strlen(str), &next));
  testrun(next == str + 4);

  str = "120xecx";
  testrun(0x0120 == ov_string_parse_hex_digits(str, strlen(str), &next));
  testrun(next == str + 3);

  //     012345678901234567890123456
  str = "ABCDef01abcdEF";
  testrun(0xABCDef01abcdEF ==
          ov_string_parse_hex_digits(str, strlen(str), &next));
  testrun(next == str + strlen(str));

  //     01234567890123456
  str = "1BCDef0123456789v";
  testrun(0x1BCDef0123456789 ==
          ov_string_parse_hex_digits(str, strlen(str), &next));
  testrun(next == str + strlen(str) - 1);

  // MAX int64_t
  //     012345678901234567890123456
  str = "7fffffffffffffff";
  int64_t max = INT64_MAX;
  testrun(max == ov_string_parse_hex_digits(str, strlen(str), &next));
  testrun(next == str + strlen(str));

  // int64_t overflow prevention
  //     012345678901234567890123456
  str = "Ffffffffffffffff";
  testrun(0 == ov_string_parse_hex_digits(str, strlen(str), &next));
  testrun(next == str);

  return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

OV_TEST_RUN("ov_string", test_ov_free, test_ov_string_len, test_ov_string_copy,
            test_ov_string_dup, test_ov_string_compare, test_ov_string_equal,
            test_ov_string_equal_nocase, test_ov_string_startswith,
            test_ov_string_sanitize,

            test_ov_string_to_uint16, test_ov_string_to_uint32,
            test_ov_string_to_uint64, test_ov_string_to_int64,
            test_ov_string_to_double,

            test_ov_string_ltrim, test_ov_string_rtrim,

            test_ov_string_value_for_key,

            test_ov_string_data_functions, test_ov_string_data_clear,
            test_ov_string_data_free, test_ov_string_data_copy,
            test_ov_string_data_dump,

            test_ov_string_pointer, test_ov_string_split,

            test_ov_string_append,

            test_ov_string_replace_all,

            test_ov_string_parse_hex_digits,

            test_ov_string_find);
