/***
        ------------------------------------------------------------------------

        Copyright (c) 2025 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_item_json_test.c
        @author         Töpfer, Markus

        @date           2025-11-29


        ------------------------------------------------------------------------
*/
#include "ov_item_json.c"
#include <ov_test/testrun.h>

#include "../include/ov_string.h"

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int check_json_decode() {

  ov_item *value = NULL;
  char *string = NULL;

  testrun(-1 == json_decode(NULL, NULL, 0));
  testrun(-1 == json_decode(NULL, "{}", 2));
  testrun(-1 == json_decode(&value, NULL, 2));
  testrun(-1 == json_decode(&value, "{}", 0));
  testrun(-1 == json_decode(&value, "{}", 1));
  testrun(2 == json_decode(&value, "{}", 2));
  testrun(ov_item_is_object(value));
  testrun(ov_item_is_empty(value));
  value = ov_item_free(value);

  testrun(0 == value);

  // parse to non existing object value
  testrun(2 == json_decode(&value, "{}", 2));
  testrun(ov_item_is_object(value));
  testrun(ov_item_is_empty(value));
  // parse to existing object value
  testrun(-1 == json_decode(&value, "{}", 100));
  value = ov_item_free(value);
  testrun(2 == json_decode(&value, "{}", 100));
  testrun(ov_item_is_object(value));
  testrun(ov_item_is_empty(value));
  // parse with whitespace
  value = ov_item_free(value);
  testrun(3 == json_decode(&value, " {}", 100));
  testrun(ov_item_is_object(value));
  testrun(ov_item_is_empty(value));
  value = ov_item_free(value);
  testrun(2 == json_decode(&value, "{} ", 100));
  testrun(ov_item_is_object(value));
  testrun(ov_item_is_empty(value));
  value = ov_item_free(value);
  testrun(3 == json_decode(&value, " {} ", 100));
  testrun(ov_item_is_object(value));
  testrun(ov_item_is_empty(value));
  value = ov_item_free(value);
  testrun(6 == json_decode(&value, "\r\n\t {} ", 100));
  testrun(ov_item_is_object(value));
  testrun(ov_item_is_empty(value));
  value = ov_item_free(value);
  // with content
  testrun(9 == json_decode(&value, "{\"key\":1}", 100));
  testrun(ov_item_is_object(value));
  testrun(!ov_item_is_empty(value));

  // parse to non existing array value
  value = ov_item_free(value);
  testrun(2 == json_decode(&value, "[]", 2));
  testrun(ov_item_is_array(value));
  testrun(ov_item_is_empty(value));
  // parse to existing array value
  testrun(-1 == json_decode(&value, "[]", 100));
  value = ov_item_free(value);
  testrun(2 == json_decode(&value, "[]", 100));
  testrun(ov_item_is_array(value));
  testrun(ov_item_is_empty(value));
  value = ov_item_free(value);
  // parse with whitespace
  testrun(3 == json_decode(&value, " []", 100));
  testrun(ov_item_is_array(value));
  testrun(ov_item_is_empty(value));
  value = ov_item_free(value);
  testrun(2 == json_decode(&value, "[] ", 100));
  testrun(ov_item_is_array(value));
  testrun(ov_item_is_empty(value));
  value = ov_item_free(value);
  testrun(3 == json_decode(&value, " [] ", 100));
  testrun(ov_item_is_array(value));
  testrun(ov_item_is_empty(value));
  value = ov_item_free(value);
  testrun(6 == json_decode(&value, "\r\n\t [] ", 100));
  testrun(ov_item_is_array(value));
  testrun(ov_item_is_empty(value));
  value = ov_item_free(value);
  // with content
  testrun(3 == json_decode(&value, "[1]", 100));
  testrun(ov_item_is_array(value));
  testrun(!ov_item_is_empty(value));
  value = ov_item_free(value);

  // parse to non existing string value
  value = ov_item_free(value);
  testrun(3 == json_decode(&value, "\"1\"", 3));
  testrun(ov_item_is_string(value));
  value = ov_item_free(value);

  // parse to non existing number value
  value = ov_item_free(value);
  testrun(1 == json_decode(&value, "1,", 2));
  testrun(ov_item_is_number(value));
  testrun(1 == ov_item_get_number(value));
  // parse to existing array value
  testrun(1 == json_decode(&value, "1,", 100));
  testrun(ov_item_is_number(value));
  testrun(1 == ov_item_get_number(value));
  // parse with whitespace
  testrun(2 == json_decode(&value, " 1,", 100));
  testrun(ov_item_is_number(value));
  testrun(1 == ov_item_get_number(value));
  testrun(1 == json_decode(&value, "1, ", 100));
  testrun(ov_item_is_number(value));
  testrun(1 == ov_item_get_number(value));
  testrun(2 == json_decode(&value, " 1, ", 100));
  testrun(ov_item_is_number(value));
  testrun(1 == ov_item_get_number(value));
  testrun(5 == json_decode(&value, "\r\n\t 1,", 100));
  testrun(ov_item_is_number(value));
  testrun(1 == ov_item_get_number(value));
  // parse to different value
  testrun(-1 == json_decode(&value, "{}", 100));
  value = ov_item_free(value);
  // check number MUST be part of a value
  // testrun(-1 == json_decode(&value, "1e", 100));
  // testrun(-1 == json_decode(&value, "1x", 100));
  // testrun(-1 == json_decode(&value, "1g", 100));
  // testrun(-1 == json_decode(&value, "1e ", 100));

  // valid following items
  testrun(1 == json_decode(&value, "1,", 100));
  testrun(ov_item_is_number(value));
  testrun(1 == ov_item_get_number(value));
  testrun(1 == json_decode(&value, "1]", 100));
  testrun(ov_item_is_number(value));
  testrun(1 == ov_item_get_number(value));
  testrun(1 == json_decode(&value, "1}", 100));
  testrun(ov_item_is_number(value));
  testrun(1 == ov_item_get_number(value));
  testrun(1 == json_decode(&value, "1 ,", 100));
  testrun(ov_item_is_number(value));
  testrun(1 == ov_item_get_number(value));
  testrun(1 == json_decode(&value, "1\r\n\t ,\r\n\t ", 100));
  testrun(ov_item_is_number(value));
  testrun(1 == ov_item_get_number(value));

  // parse to non existing literal value
  value = ov_item_free(value);
  testrun(4 == json_decode(&value, "null", 4));
  testrun(ov_item_is_null(value));
  value = ov_item_free(value);
  testrun(4 == json_decode(&value, "true", 100));
  testrun(ov_item_is_true(value));
  value = ov_item_free(value);
  testrun(5 == json_decode(&value, "false", 100));
  testrun(ov_item_is_false(value));
  value = ov_item_free(value);
  // parse with whitespace
  testrun(5 == json_decode(&value, " null", 100));
  testrun(ov_item_is_null(value));
  value = ov_item_free(value);
  testrun(4 == json_decode(&value, "true ", 100));
  testrun(ov_item_is_true(value));
  value = ov_item_free(value);
  testrun(6 == json_decode(&value, " false ", 100));
  testrun(ov_item_is_false(value));
  value = ov_item_free(value);
  testrun(8 == json_decode(&value, "\r\n\t true", 100));
  testrun(ov_item_is_true(value));
  value = ov_item_free(value);

  // some decode tests
  string = "{}";
  testrun(2 == json_decode(&value, string, strlen(string)));
  testrun(ov_item_is_object(value));
  value = ov_item_free(value);

  string = "[]";
  testrun(2 == json_decode(&value, string, strlen(string)));
  testrun(ov_item_is_array(value));
  value = ov_item_free(value);

  string = "\"abc\"";
  testrun(5 == json_decode(&value, string, strlen(string)));
  testrun(ov_item_is_string(value));
  value = ov_item_free(value);

  string = "-1,";
  testrun(2 == json_decode(&value, string, strlen(string)));
  testrun(ov_item_is_number(value));
  value = ov_item_free(value);

  string = "-1.1e-12,";
  testrun(8 == json_decode(&value, string, strlen(string)));
  testrun(ov_item_is_number(value));
  value = ov_item_free(value);

  string = "1.2E10,";
  testrun(6 == json_decode(&value, string, strlen(string)));
  testrun(ov_item_is_number(value));
  value = ov_item_free(value);

  string = "null";
  testrun(4 == json_decode(&value, string, strlen(string)));
  testrun(ov_item_is_null(value));
  value = ov_item_free(value);

  string = "true";
  testrun(4 == json_decode(&value, string, strlen(string)));
  testrun(ov_item_is_true(value));
  value = ov_item_free(value);

  string = "false";
  testrun(5 == json_decode(&value, string, strlen(string)));
  testrun(ov_item_is_false(value));
  value = ov_item_free(value);

  string = "{\"key\":[ null, true,1, 1212 \n\r, 2]}";
  testrun(strlen(string) ==
          (size_t)json_decode(&value, string, strlen(string)));
  testrun(ov_item_is_object(value));
  testrun(1 == ov_item_count(value));
  testrun(5 == ov_item_count(ov_item_object_get(value, "key")));
  testrun(ov_item_is_null(ov_item_get(value, "/key/0")));
  testrun(ov_item_is_true(ov_item_get(value, "/key/1")));
  testrun(ov_item_is_number(ov_item_get(value, "/key/2")));
  testrun(ov_item_is_number(ov_item_get(value, "/key/3")));
  testrun(ov_item_is_number(ov_item_get(value, "/key/4")));
  value = ov_item_free(value);

  // Double check that parsing works with ordinary strings
  string = "{\"key\":\"abc\"}";
  testrun(strlen(string) ==
          (size_t)json_decode(&value, string, strlen(string)));
  value = ov_item_free(value);

  // Parse quoted backslashes
  string = "{\"key\":\"abc\\\\def\"}";
  testrun(strlen(string) ==
          (size_t)json_decode(&value, string, strlen(string)));

  value = ov_item_free(value);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_from_json() {

  ov_item *item = NULL;
  testrun(NULL == ov_item_from_json(""));

  item = ov_item_from_json("{}");
  testrun(ov_item_is_object(item));
  testrun(ov_item_is_empty(item));
  item = ov_item_free(item);

  item = ov_item_from_json("[]");
  testrun(ov_item_is_array(item));
  testrun(ov_item_is_empty(item));
  item = ov_item_free(item);

  item = ov_item_from_json("\"a\"");
  testrun(ov_item_is_string(item));
  item = ov_item_free(item);

  item = ov_item_from_json("1");
  testrun(ov_item_is_number(item));
  item = ov_item_free(item);

  item = ov_item_from_json("1e");
  testrun(!item);
  item = ov_item_free(item);

  item = ov_item_from_json("null");
  testrun(ov_item_is_null(item));
  item = ov_item_free(item);

  item = ov_item_from_json("true");
  testrun(ov_item_is_true(item));
  item = ov_item_free(item);

  item = ov_item_from_json("false");
  testrun(ov_item_is_false(item));
  item = ov_item_free(item);

  item = ov_item_from_json("{\"abc\":[1,2,[3,4,5, true, false, null]]}");
  testrun(ov_item_is_object(item));
  testrun(!ov_item_is_empty(item));
  item = ov_item_free(item);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_parser_calculate_value() {

  // check parsing
  char *expect = NULL;
  ov_item *value = ov_item_object();
  ov_item_json_stringify_config conf_minimal =
      ov_item_json_config_stringify_minimal();
  ov_item_json_stringify_config conf_default =
      ov_item_json_config_stringify_default();

  expect = "{}";

  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_minimal));
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_default));

  testrun(-1 == parser_calculate(NULL, NULL));
  testrun(-1 == parser_calculate(NULL, &conf_minimal));
  testrun(strlen(expect) == (size_t)parser_calculate(value, NULL));

  testrun(ov_item_object_set(value, "1", ov_item_number(1)));
  expect = "{\"1\":1}";
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_minimal));
  expect = "{\n"
           "\t\"1\":1\n"
           "}";
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_default));
  value = ov_item_free(value);

  value = ov_item_null();
  expect = "null";
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_default));
  value = ov_item_free(value);

  value = ov_item_true();
  expect = "true";
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_default));
  value = ov_item_free(value);

  value = ov_item_false();
  expect = "false";
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_default));
  value = ov_item_free(value);

  value = ov_item_string("abc");
  expect = "\"abc\"";
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_default));
  value = ov_item_free(value);

  value = ov_item_number(123);
  expect = "123";
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_default));
  value = ov_item_free(value);

  value = ov_item_number(-1.23e5);
  expect = "-1.23e5";
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_default));
  value = ov_item_free(value);

  value = ov_item_array();
  expect = "[]";
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_default));
  testrun(ov_item_array_push(value, ov_item_number(1)));
  testrun(ov_item_array_push(value, ov_item_number(2)));
  testrun(ov_item_array_push(value, ov_item_number(3)));
  expect = "[1,2,3]";
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_minimal));
  expect = "[\n"
           "\t1,\n"
           "\t2,\n"
           "\t3\n"
           "]";
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_default));
  value = ov_item_free(value);

  value = ov_item_object();
  expect = "{}";
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_default));
  testrun(ov_item_object_set(value, "1", ov_item_number(1)));
  testrun(ov_item_object_set(value, "2", ov_item_number(2)));
  testrun(ov_item_object_set(value, "3", ov_item_number(3)));
  expect = "{\"1\":1,\"2\":2,\"3\":3}";
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_minimal));
  expect = "{\n"
           "\t\"1\":1,\n"
           "\t\"2\":2,\n"
           "\t\"3\":3\n"
           "}";
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_default));
  value = ov_item_free(value);

  // check with depth
  value = ov_item_object();
  expect = "{}";
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_default));
  testrun(ov_item_object_set(value, "1", ov_item_object()));
  testrun(ov_item_object_set(value, "2", ov_item_array()));
  testrun(ov_item_object_set(value, "3", ov_item_object()));
  expect = "{\"1\":{},\"2\":[],\"3\":{}}";
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_minimal));
  expect = "{\n"
           "\t\"1\":{},\n"
           "\t\"2\":[],\n"
           "\t\"3\":{}\n"
           "}";
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_default));
  ov_item *child = ov_item_object_get(value, "1");
  testrun(child);
  testrun(ov_item_object_set(child, "1.1", ov_item_true()));
  testrun(ov_item_object_set(child, "1.2", ov_item_array()));
  testrun(ov_item_object_set(child, "1.3", ov_item_number(1)));
  expect = "{\n"
           "\t\"1\":\n"
           "\t{\n"
           "\t\t\"1.1\":true,\n"
           "\t\t\"1.2\":[],\n"
           "\t\t\"1.3\":1\n"
           "\t},\n"
           "\t\"2\":[],\n"
           "\t\"3\":{}\n"
           "}";

  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_default));

  child = ov_item_object_get(value, "2");
  testrun(ov_item_array_push(child, ov_item_true()));
  testrun(ov_item_array_push(child, ov_item_array()));
  testrun(ov_item_array_push(child, ov_item_number(1)));
  expect = "{\n"
           "\t\"1\":\n"
           "\t{\n"
           "\t\t\"1.1\":true,\n"
           "\t\t\"1.2\":[],\n"
           "\t\t\"1.3\":1\n"
           "\t},\n"
           "\t\"2\":\n"
           "\t[\n"
           "\t\ttrue,\n"
           "\t\t[],\n"
           "\t\t1\n"
           "\t],\n"
           "\t\"3\":{}\n"
           "}";
  testrun(strlen(expect) == (size_t)parser_calculate(value, &conf_default));
  value = ov_item_free(value);

  ov_item_free(value);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_to_json_with_config() {

  ov_item *item = ov_item_object();
  ov_item *obj = ov_item_object();
  testrun(ov_item_object_set(obj, "1", ov_item_number(1)));
  testrun(ov_item_object_set(obj, "2", ov_item_number(2)));
  testrun(ov_item_object_set(obj, "3", ov_item_number(3)));
  ov_item *arr = ov_item_array();
  testrun(ov_item_array_push(arr, ov_item_true()));
  testrun(ov_item_array_push(arr, ov_item_false()));
  testrun(ov_item_array_push(arr, ov_item_null()));
  testrun(ov_item_array_push(arr, ov_item_array()));
  testrun(ov_item_array_push(arr, ov_item_object()));
  testrun(ov_item_array_push(arr, ov_item_number(1.23e5)));
  testrun(ov_item_array_push(arr, ov_item_string("test")));
  testrun(ov_item_object_set(item, "1", obj));
  testrun(ov_item_object_set(item, "2", arr));
  testrun(ov_item_object_set(item, "3", ov_item_true()));
  testrun(ov_item_object_set(item, "4", ov_item_false()));
  testrun(ov_item_object_set(item, "5", ov_item_null()));
  testrun(ov_item_object_set(item, "6", ov_item_array()));
  testrun(ov_item_object_set(item, "7", ov_item_object()));
  testrun(ov_item_object_set(item, "8", ov_item_number(1.6)));
  testrun(ov_item_object_set(item, "9", ov_item_string("test")));

  char *string = ov_item_to_json_with_config(
      item, ov_item_json_config_stringify_default());

  char *expect = "{\n"
                 "\t\"1\":\n"
                 "\t{\n"
                 "\t\t\"1\":1,\n"
                 "\t\t\"2\":2,\n"
                 "\t\t\"3\":3\n"
                 "\t},\n"
                 "\t\"2\":\n"
                 "\t[\n"
                 "\t\ttrue,\n"
                 "\t\tfalse,\n"
                 "\t\tnull,\n"
                 "\t\t[],\n"
                 "\t\t{},\n"
                 "\t\t123000,\n"
                 "\t\t\"test\"\n"
                 "\t],\n"
                 "\t\"3\":true,\n"
                 "\t\"4\":false,\n"
                 "\t\"5\":null,\n"
                 "\t\"6\":[],\n"
                 "\t\"7\":{},\n"
                 "\t\"8\":1.6,\n"
                 "\t\"9\":\"test\"\n"
                 "}";

  // testrun_log("\n%s\n%s", string, expect);

  testrun(0 == ov_string_compare(string, expect));
  free(string);

  testrun(NULL == ov_item_free(item));
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
  testrun_test(check_json_decode);
  testrun_test(test_ov_item_from_json);

  testrun_test(check_parser_calculate_value);
  testrun_test(test_ov_item_to_json_with_config);

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
