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
        @file           ov_json_pointer_test.c
        @author         Markus Toepfer

        @date           2018-12-05

        @ingroup        ov_json_pointer

        @brief          Unit tests of ov_json_pointer


        ------------------------------------------------------------------------
*/
#include "ov_json_pointer.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

int check_json_pointer_parse_token() {

  uint64_t size = 100;
  char buffer[size];
  memset(buffer, '\0', size);

  memcpy(buffer, "/abc", 4);

  char *token = NULL;
  size_t length = 0;

  testrun(!json_pointer_parse_token(NULL, 0, NULL, NULL));
  testrun(!json_pointer_parse_token(NULL, size, &token, &length));
  testrun(!json_pointer_parse_token(buffer, 0, &token, &length));
  testrun(!json_pointer_parse_token(buffer, size, NULL, &length));
  testrun(!json_pointer_parse_token(buffer, size, &token, NULL));
  testrun(json_pointer_parse_token(buffer, 1, &token, &length));

  // empty token
  testrun(json_pointer_parse_token(buffer, 1, &token, &length));
  testrun(length == 0);
  testrun(token[0] == '/');

  testrun(json_pointer_parse_token(buffer, 2, &token, &length));
  testrun(length == 1);
  testrun(token[0] == 'a');
  testrun(token[length - 1] == 'a');

  testrun(json_pointer_parse_token(buffer, 3, &token, &length));
  testrun(length == 2);
  testrun(token[0] == 'a');
  testrun(token[length - 1] == 'b');

  testrun(json_pointer_parse_token(buffer, 4, &token, &length));
  testrun(length == 3);
  testrun(token[0] == 'a');
  testrun(token[length - 1] == 'c');

  testrun(json_pointer_parse_token(buffer, 5, &token, &length));
  testrun(length == 3);
  testrun(token[0] == 'a');
  testrun(token[length - 1] == 'c');

  testrun(json_pointer_parse_token(buffer, 100, &token, &length));
  testrun(length == 3);
  testrun(token[0] == 'a');
  testrun(token[length - 1] == 'c');

  //              0123456789
  memcpy(buffer, "/123/45/6/", 10);

  testrun(json_pointer_parse_token(buffer, 2, &token, &length));
  testrun(length == 1);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '1');

  testrun(json_pointer_parse_token(buffer, 3, &token, &length));
  testrun(length == 2);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '2');

  testrun(json_pointer_parse_token(buffer, 4, &token, &length));
  testrun(length == 3);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '3');

  testrun(json_pointer_parse_token(buffer, 5, &token, &length));
  testrun(length == 3);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '3');

  testrun(json_pointer_parse_token(buffer, 100, &token, &length));
  testrun(length == 3);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '3');

  // not starting with '/'
  testrun(!json_pointer_parse_token(buffer + 1, 2, &token, &length));
  testrun(!json_pointer_parse_token(buffer + 2, 2, &token, &length));
  testrun(!json_pointer_parse_token(buffer + 3, 2, &token, &length));

  // check second pointer
  testrun(json_pointer_parse_token(buffer + 4, 2, &token, &length));
  testrun(length == 1);
  testrun(token[0] == '4');
  testrun(token[length - 1] == '4');
  testrun(json_pointer_parse_token(buffer + 4, 3, &token, &length));
  testrun(length == 2);
  testrun(token[0] == '4');
  testrun(token[length - 1] == '5');
  testrun(json_pointer_parse_token(buffer + 4, 1000, &token, &length));
  testrun(length == 2);
  testrun(token[0] == '4');
  testrun(token[length - 1] == '5');

  // check third pointer
  testrun(!json_pointer_parse_token(buffer + 5, 2, &token, &length));
  testrun(!json_pointer_parse_token(buffer + 6, 2, &token, &length));
  testrun(json_pointer_parse_token(buffer + 7, 2, &token, &length));
  testrun(length == 1);
  testrun(token[0] == '6');
  testrun(token[length - 1] == '6');
  testrun(json_pointer_parse_token(buffer + 7, 3, &token, &length));
  testrun(length == 1);
  testrun(token[0] == '6');
  testrun(token[length - 1] == '6');
  testrun(json_pointer_parse_token(buffer + 7, 1000, &token, &length));
  testrun(length == 1);
  testrun(token[0] == '6');
  testrun(token[length - 1] == '6');

  // check empty followed by null pointer
  testrun(!json_pointer_parse_token(buffer + 10, 2, &token, &length));
  testrun(!json_pointer_parse_token(buffer + 11, 2, &token, &length));
  testrun(!json_pointer_parse_token(buffer + 12, 2, &token, &length));

  //              0123456789
  memcpy(buffer, "/1~0/45/6/", 10);
  testrun(json_pointer_parse_token(buffer, 2, &token, &length));
  testrun(length == 1);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '1');

  testrun(json_pointer_parse_token(buffer, 3, &token, &length));
  testrun(length == 2);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '~');

  testrun(json_pointer_parse_token(buffer, 4, &token, &length));
  testrun(length == 3);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '0');

  testrun(json_pointer_parse_token(buffer, 5, &token, &length));
  testrun(length == 3);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '0');

  testrun(json_pointer_parse_token(buffer, 100, &token, &length));
  testrun(length == 3);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '0');

  //              0123456789
  memcpy(buffer, "/1~1/45/6/", 10);
  testrun(json_pointer_parse_token(buffer, 2, &token, &length));
  testrun(length == 1);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '1');

  testrun(json_pointer_parse_token(buffer, 3, &token, &length));
  testrun(length == 2);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '~');

  testrun(json_pointer_parse_token(buffer, 4, &token, &length));
  testrun(length == 3);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '1');

  testrun(json_pointer_parse_token(buffer, 5, &token, &length));
  testrun(length == 3);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '1');

  testrun(json_pointer_parse_token(buffer, 100, &token, &length));
  testrun(length == 3);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '1');

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_json_pointer_replace_special_encoding() {

  uint64_t size = 100;
  char buffer1[size];
  char buffer2[size];
  memset(buffer1, '\0', size);
  memset(buffer2, '\0', size);

  memcpy(buffer1, "123abcdefg", 10);

  char *result = buffer2;

  testrun(!json_pointer_replace_special_encoding(NULL, size, buffer1, "abc",
                                                 "xyz"));

  testrun(
      !json_pointer_replace_special_encoding(result, size, NULL, "abc", "xyz"));

  testrun(!json_pointer_replace_special_encoding(result, size, buffer1, NULL,
                                                 "xyz"));

  testrun(!json_pointer_replace_special_encoding(result, size, buffer1, "abc",
                                                 NULL));

  testrun(
      !json_pointer_replace_special_encoding(result, 0, buffer1, "abc", "xyz"));

  testrun(strlen(buffer2) == 0);

  // replace with same length
  testrun(json_pointer_replace_special_encoding(result, size, buffer1, "abc",
                                                "xyz"));

  testrun(strlen(buffer2) == strlen(buffer1));
  testrun(strncmp("123xyzdefg", buffer2, strlen(buffer2)) == 0);
  memset(buffer2, '\0', size);

  // replace with shorter length
  memset(buffer2, '\0', size);
  testrun(
      json_pointer_replace_special_encoding(result, size, buffer1, "abc", "x"));

  testrun(strlen(buffer2) == strlen(buffer1) - 2);
  testrun(strncmp("123xdefg", buffer2, strlen(buffer2)) == 0);

  // replace first
  memset(buffer2, '\0', size);
  testrun(
      json_pointer_replace_special_encoding(result, size, buffer1, "1", "x"));

  testrun(strlen(buffer2) == strlen(buffer1));
  testrun(strncmp("x23abcdefg", buffer2, strlen(buffer2)) == 0);

  // replace last
  memset(buffer2, '\0', size);
  testrun(
      json_pointer_replace_special_encoding(result, size, buffer1, "g", "x"));

  testrun(strlen(buffer2) == strlen(buffer1));
  testrun(strncmp("123abcdefx", buffer2, strlen(buffer2)) == 0);

  // replace (not included)
  memset(buffer2, '\0', size);
  testrun(
      json_pointer_replace_special_encoding(result, size, buffer1, "vvv", "x"));

  testrun(strlen(buffer2) == strlen(buffer1));
  testrun(strncmp("123abcdefg", buffer2, strlen(buffer2)) == 0);

  // replace all
  memset(buffer2, '\0', size);
  testrun(json_pointer_replace_special_encoding(result, size, buffer1,
                                                "123abcdefg", "x"));

  testrun(strlen(buffer2) == 1);
  testrun(strncmp("x", buffer2, strlen(buffer2)) == 0);

  // replace up to length
  memset(buffer2, '\0', size);
  testrun(
      json_pointer_replace_special_encoding(result, 9, buffer1, "abc", "xyz"));
  testrun(strlen(buffer2) == 9);
  testrun(strncmp("123xyzdef", buffer2, strlen(buffer2)) == 0);

  memset(buffer2, '\0', size);
  testrun(
      json_pointer_replace_special_encoding(result, 8, buffer1, "abc", "xyz"));
  testrun(strlen(buffer2) == 8);
  testrun(strncmp("123xyzde", buffer2, strlen(buffer2)) == 0);

  memset(buffer2, '\0', size);
  testrun(
      json_pointer_replace_special_encoding(result, 7, buffer1, "abc", "xyz"));
  testrun(strlen(buffer2) == 7);
  testrun(strncmp("123xyzd", buffer2, strlen(buffer2)) == 0);

  memset(buffer2, '\0', size);
  testrun(
      json_pointer_replace_special_encoding(result, 6, buffer1, "abc", "xyz"));
  testrun(strlen(buffer2) == 6);
  testrun(strncmp("123xyz", buffer2, strlen(buffer2)) == 0);

  memset(buffer2, '\0', size);
  testrun(
      !json_pointer_replace_special_encoding(result, 5, buffer1, "abc", "xyz"));
  testrun(strlen(buffer2) == 0);

  memcpy(buffer1, "/123/abc/x", 10);

  memset(buffer2, '\0', size);
  testrun(
      json_pointer_replace_special_encoding(result, 10, buffer1, "~1", "/"));
  testrun(strlen(buffer2) == 10);
  testrun(strncmp("/123/abc/x", buffer2, strlen(buffer2)) == 0);

  memset(buffer2, '\0', size);
  testrun(json_pointer_replace_special_encoding(result, 4, buffer1, "~1", "/"));
  testrun(strlen(buffer2) == 4);
  testrun(strncmp("/123", buffer2, strlen(buffer2)) == 0);

  memcpy(buffer1, "12~1abc", 7);
  memset(buffer2, '\0', size);
  testrun(json_pointer_replace_special_encoding(result, 7, buffer1, "~1", "/"));
  testrun(strlen(buffer2) == 6);
  testrun(strncmp("12/abc", buffer2, strlen(buffer2)) == 0);

  memcpy(buffer1, "12~0abc", 7);
  memset(buffer2, '\0', size);
  testrun(json_pointer_replace_special_encoding(result, 7, buffer1, "~0", "~"));
  testrun(strlen(buffer2) == 6);
  testrun(strncmp("12~abc", buffer2, strlen(buffer2)) == 0);

  memcpy(buffer1, "12~3abc", 7);
  memset(buffer2, '\0', size);
  testrun(json_pointer_replace_special_encoding(result, 7, buffer1, "~0", "~"));
  testrun(strlen(buffer2) == 7);
  testrun(strncmp("12~3abc", buffer2, strlen(buffer2)) == 0);

  memcpy(buffer1, "12~~0ab", 7);
  memset(buffer2, '\0', size);
  testrun(json_pointer_replace_special_encoding(result, 7, buffer1, "~0", "~"));
  testrun(strlen(buffer2) == 6);
  testrun(strncmp("12~~ab", buffer2, strlen(buffer2)) == 0);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_json_pointer_escape_token() {

  uint64_t size = 100;
  char buffer1[size];
  char buffer2[size];
  memset(buffer1, '\0', size);
  memset(buffer2, '\0', size);

  memcpy(buffer1, "123abcdefg", 10);

  char *result = buffer2;

  testrun(!json_pointer_escape_token(NULL, size, result, size));

  testrun(!json_pointer_escape_token(buffer1, 0, result, size));

  testrun(!json_pointer_escape_token(buffer1, size, NULL, size));

  testrun(!json_pointer_escape_token(buffer1, size, result, size - 1));

  // tokens not contained
  testrun(json_pointer_escape_token(buffer1, size, result, size));

  // log("buffer1 |%s|\nbuffer2 |%s|", buffer1, buffer2);
  testrun(strlen(buffer2) == strlen(buffer1));
  testrun(strncmp("123abcdefg", buffer2, strlen(buffer2)) == 0);

  // ~0 contained
  memcpy(buffer1, "123~0cdefg", 10);
  memset(buffer2, '\0', size);

  testrun(json_pointer_escape_token(buffer1, 10, result, size));
  testrun(strlen(buffer2) == strlen(buffer1) - 1);
  testrun(strncmp("123~cdefg", buffer2, strlen(buffer2)) == 0);

  // ~1 contained
  memcpy(buffer1, "123~1cdefg", 10);
  memset(buffer2, '\0', size);

  testrun(json_pointer_escape_token(buffer1, size, result, size));
  testrun(strlen(buffer2) == strlen(buffer1) - 1);
  testrun(strncmp("123\\cdefg", buffer2, strlen(buffer2)) == 0);

  // replacement order
  memcpy(buffer1, "123~01defg", 10);
  memset(buffer2, '\0', size);
  testrun(json_pointer_escape_token(buffer1, size, result, size));
  testrun(strlen(buffer2) == strlen(buffer1) - 1);
  testrun(strncmp("123~1defg", buffer2, strlen(buffer2)) == 0);

  // multiple replacements
  memcpy(buffer1, "~0~0~01d~0g", 11);
  memset(buffer2, '\0', size);
  testrun(json_pointer_escape_token(buffer1, 11, result, size));
  testrun(strlen(buffer2) == strlen(buffer1) - 4);
  testrun(strncmp("~~~1d~g", buffer2, strlen(buffer2)) == 0);

  // multiple replacements
  memcpy(buffer1, "~0~1~01d~1g", 11);
  memset(buffer2, '\0', size);
  testrun(json_pointer_escape_token(buffer1, size, result, size));
  testrun(strlen(buffer2) == strlen(buffer1) - 4);
  testrun(strncmp("~\\~1d\\g", buffer2, strlen(buffer2)) == 0);

  // multiple replacements
  memcpy(buffer1, "array/5", 7);
  memset(buffer2, '\0', size);
  testrun(json_pointer_escape_token(buffer1, 5, result, size));
  testrun(strlen(buffer2) == 5);
  testrun(strncmp("array", buffer2, strlen(buffer2)) == 0);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_json_pointer_get_token_in_parent() {

  ov_json_value *v1 = NULL;
  ov_json_value *v2 = NULL;
  ov_json_value *v3 = NULL;
  ov_json_value *v4 = NULL;
  ov_json_value *v0 = NULL;
  ov_json_value *v5 = NULL;
  ov_json_value *v6 = NULL;
  ov_json_value *v7 = NULL;
  ov_json_value *v8 = NULL;
  ov_json_value *v9 = NULL;

  ov_json_value *result = NULL;
  ov_json_value *val = NULL;
  ov_json_value *value = ov_json_object();

  v1 = ov_json_true();
  ov_json_object_set(value, "key", v1);

  uint64_t size = 100;
  char buffer1[size];
  char buffer2[size];
  memset(buffer1, '\0', size);
  memset(buffer2, '\0', size);

  strcpy(buffer1, "key");

  testrun(!json_pointer_get_token_in_parent(NULL, NULL, 0));
  testrun(!json_pointer_get_token_in_parent(value, NULL, 3));

  // empty key is value
  testrun(value == json_pointer_get_token_in_parent(value, "key", 0));

  val = ov_json_object_get(value, "key");
  result = json_pointer_get_token_in_parent(value, "key", 3);
  testrun(result);
  testrun(result->parent == value);
  testrun(result == val);

  // test empty object
  value = ov_json_value_free(value);
  value = ov_json_object();
  testrun(!json_pointer_get_token_in_parent(value, "key", 3));
  ov_json_object_set(value, "key", ov_json_true());
  val = ov_json_object_get(value, "key");
  result = json_pointer_get_token_in_parent(value, "key", 3);
  testrun(result);
  testrun(result->parent == value);
  testrun(result == val);

  // check key match
  testrun(!json_pointer_get_token_in_parent(value, "key", 2));
  testrun(!json_pointer_get_token_in_parent(value, "Key ", 3));
  testrun(!json_pointer_get_token_in_parent(value, "key ", 4));

  // add some members
  v1 = ov_json_true();
  testrun(ov_json_object_set(value, "1", v1));
  v2 = ov_json_true();
  testrun(ov_json_object_set(value, "2", v2));
  v3 = ov_json_true();
  testrun(ov_json_object_set(value, "3", v3));
  v4 = ov_json_true();
  testrun(ov_json_object_set(value, "4", v4));

  result = json_pointer_get_token_in_parent(value, "1", 1);
  testrun(result);
  testrun(result->parent == value);
  testrun(result == v1);

  result = json_pointer_get_token_in_parent(value, "2", 1);
  testrun(result);
  testrun(result->parent == value);
  testrun(result == v2);

  result = json_pointer_get_token_in_parent(value, "3", 1);
  testrun(result);
  testrun(result->parent == value);
  testrun(result == v3);

  result = json_pointer_get_token_in_parent(value, "4", 1);
  testrun(result);
  testrun(result->parent == value);
  testrun(result == v4);
  value = ov_json_value_free(value);

  value = ov_json_array();
  testrun(0 == ov_json_array_count(value));
  testrun(!json_pointer_get_token_in_parent(value, "1", 1));
  testrun(!json_pointer_get_token_in_parent(value, "2", 1));
  testrun(!json_pointer_get_token_in_parent(value, "3", 1));
  v0 = json_pointer_get_token_in_parent(value, "-", 1);
  testrun(1 == ov_json_array_count(value));

  v1 = ov_json_true();
  testrun(ov_json_array_push(value, v1)) v2 = ov_json_true();
  testrun(ov_json_array_push(value, v2)) v3 = ov_json_true();
  testrun(ov_json_array_push(value, v3)) v4 = ov_json_true();
  testrun(ov_json_array_push(value, v4));

  ov_json_array_dump(stdout, value);

  result = json_pointer_get_token_in_parent(value, "0", 1);
  testrun(result == v0);
  result = json_pointer_get_token_in_parent(value, "1", 1);
  testrun(result == v1);
  result = json_pointer_get_token_in_parent(value, "2", 1);
  testrun(result == v2);
  result = json_pointer_get_token_in_parent(value, "3", 1);
  testrun(result == v3);
  result = json_pointer_get_token_in_parent(value, "4", 1);
  testrun(result == v4);
  testrun(5 == ov_json_array_count(value));

  // add a new array item
  result = json_pointer_get_token_in_parent(value, "-", 1);
  testrun(6 == ov_json_array_count(value));
  testrun(result->parent == value);

  // get with leading zero
  result = json_pointer_get_token_in_parent(value, "00", 2);
  testrun(result == v0);
  result = json_pointer_get_token_in_parent(value, "01", 2);
  testrun(result == v1);
  result = json_pointer_get_token_in_parent(value, "02", 2);
  testrun(result == v2);
  result = json_pointer_get_token_in_parent(value, "03", 2);
  testrun(result == v3);
  result = json_pointer_get_token_in_parent(value, "04", 2);
  testrun(result == v4);

  // not array or object
  testrun(!json_pointer_get_token_in_parent(v1, "3", 1));
  value = ov_json_value_free(value);

  value = ov_json_object();
  testrun(!json_pointer_get_token_in_parent(value, "foo", 3));
  testrun(ov_json_object_set(value, "foo", ov_json_true()));
  result = json_pointer_get_token_in_parent(value, "foo", 3);
  testrun(result);
  testrun(result->parent == value);
  testrun(result == ov_json_object_get(value, "foo"));

  v1 = ov_json_true();
  v2 = ov_json_true();
  v3 = ov_json_true();
  v4 = ov_json_true();
  v5 = ov_json_true();
  v6 = ov_json_true();
  v7 = ov_json_true();
  v8 = ov_json_true();
  v9 = ov_json_true();
  testrun(ov_json_object_set(value, "0", v1));
  testrun(ov_json_object_set(value, "a/b", v2));
  testrun(ov_json_object_set(value, "c%d", v3));
  testrun(ov_json_object_set(value, "e^f", v4));
  testrun(ov_json_object_set(value, "g|h", v5));
  testrun(ov_json_object_set(value, "i\\\\j", v6));
  testrun(ov_json_object_set(value, "k\"l", v7));
  testrun(ov_json_object_set(value, " ", v8));
  testrun(ov_json_object_set(value, "m~n", v9));

  result = json_pointer_get_token_in_parent(value, "", 0);
  testrun(result == value);
  result = json_pointer_get_token_in_parent(value, "0", 1);
  testrun(result == v1);
  result = json_pointer_get_token_in_parent(value, "a/b", 3);
  testrun(result == v2);
  result = json_pointer_get_token_in_parent(value, "c%d", 3);
  testrun(result == v3);
  result = json_pointer_get_token_in_parent(value, "e^f", 3);
  testrun(result == v4);
  result = json_pointer_get_token_in_parent(value, "g|h", 3);
  testrun(result == v5);
  result = json_pointer_get_token_in_parent(value, "i\\\\j", 4);
  testrun(result == v6);
  result = json_pointer_get_token_in_parent(value, "k\"l", 3);
  testrun(result == v7);
  result = json_pointer_get_token_in_parent(value, " ", 1);
  testrun(result == v8);
  result = json_pointer_get_token_in_parent(value, "m~n", 3);
  testrun(result == v9);

  value = ov_json_value_free(value);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_get() {

  ov_json_value *array = NULL;
  ov_json_value *object = NULL;
  ov_json_value *v0 = NULL;
  ov_json_value *v1 = NULL;
  ov_json_value *v2 = NULL;
  ov_json_value *v3 = NULL;
  ov_json_value *v4 = NULL;
  ov_json_value *v5 = NULL;

  ov_json_value const *result = NULL;
  ov_json_value *value = ov_json_object();
  v0 = ov_json_true();
  ov_json_object_set(value, "key", v0);

  uint64_t size = 100;
  char buffer1[size];
  memset(buffer1, '\0', size);

  strcpy(buffer1, "key");

  testrun(!ov_json_get(NULL, NULL));
  testrun(!ov_json_get(value, NULL));
  testrun(!ov_json_get(NULL, "key"));

  // empty key is value
  testrun(value == ov_json_get(value, ""));
  testrun(!ov_json_get(value, "/"));
  testrun(!ov_json_get(value, "/k"));
  testrun(!ov_json_get(value, "/ke"));
  testrun(v0 == ov_json_get(value, "/key"));

  // key not contained
  testrun(!ov_json_get(value, "1"));

  // add some childs
  array = ov_json_array();
  ov_json_object_set(value, "array", array);
  object = ov_json_object();
  ov_json_object_set(value, "object", object);
  v1 = ov_json_true();
  ov_json_object_set(value, "1", v1);
  v2 = ov_json_false();
  ov_json_object_set(value, "2", v2);

  // add some child to the array
  v3 = ov_json_true();
  v4 = ov_json_true();
  v5 = ov_json_object();
  ov_json_array_push(array, v3);
  ov_json_array_push(array, v4);
  ov_json_array_push(array, v5);

  // add child to object in array
  ov_json_object_set(v5, "5", ov_json_array());
  ov_json_object_set(v5, "key6", ov_json_object());
  ov_json_object_set(v5, "key7", ov_json_object());

  // add child to object
  ov_json_object_set(object, "8", ov_json_object());

  // positive testing

  testrun(value == ov_json_get(value, ""));
  testrun(v0 == ov_json_get(value, "/key"));
  testrun(array == ov_json_get(value, "/array"));
  testrun(object == ov_json_get(value, "/object"));

  testrun(ov_json_get(value, "/key"));
  testrun(ov_json_get(value, "/array"));
  testrun(ov_json_get(value, "/object"));
  testrun(!ov_json_get(value, "/key "));
  testrun(!ov_json_get(value, "/array "));
  testrun(!ov_json_get(value, "/object "));

  testrun(v3 == ov_json_get(value, "/array/0"));
  testrun(v4 == ov_json_get(value, "/array/1"));
  testrun(v5 == ov_json_get(value, "/array/2"));

  testrun(ov_json_get(value, "/array/2/5"));
  testrun(ov_json_get(value, "/array/2/key6"));
  testrun(ov_json_get(value, "/array/2/key7"));

  testrun(object == ov_json_get(value, "/object"));
  testrun(ov_json_get(value, "/object/8"));

  testrun(v1 == ov_json_get(value, "/1"));
  testrun(v2 == ov_json_get(value, "/2"));

  // add new array member
  testrun(3 == ov_json_array_count(array));
  result = ov_json_get(value, "/array/-");
  testrun(result);
  testrun(4 == ov_json_array_count(array));

  // try to add new object member
  testrun(!ov_json_get(value, "/-"));

  // key not contained
  testrun(!ov_json_get(value, "/objectX"));
  testrun(!ov_json_get(value, "/obj"));
  testrun(!ov_json_get(value, "/array/2/key1"));
  testrun(!ov_json_get(value, "/array/2/k"));

  // out of index
  testrun(4 == ov_json_array_count(array));
  testrun(ov_json_get(value, "/array/0"));
  testrun(ov_json_get(value, "/array/1"));
  testrun(ov_json_get(value, "/array/2"));
  testrun(ov_json_get(value, "/array/3"));
  testrun(!ov_json_get(value, "/array/4"));
  testrun(!ov_json_get(value, "/array/5"));
  testrun(!ov_json_get(value, "/array/6"));
  testrun(!ov_json_get(value, "/array/7"));

  // not an index
  testrun(array == ov_json_get(value, "/array/"));
  testrun(array == ov_json_get(value, "/array/"));
  testrun(array == ov_json_get(value, "/array/"));

  // not an object
  testrun(object == ov_json_get(value, "/object"));
  testrun(object == ov_json_get(value, "/object/"));
  testrun(!ov_json_get(value, "/object/ "));
  testrun(!ov_json_get(value, "/objec "));

  // no value to return
  testrun(!ov_json_get(value, "/"));

  // no starting pointer slash
  testrun(!ov_json_get(value, "object"));
  testrun(!ov_json_get(value, "object/"));

  ov_json_value_free(value);

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
  testrun_test(check_json_pointer_parse_token);
  testrun_test(check_json_pointer_replace_special_encoding);
  testrun_test(check_json_pointer_escape_token);
  testrun_test(check_json_pointer_get_token_in_parent);

  testrun_test(test_ov_json_get);

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
