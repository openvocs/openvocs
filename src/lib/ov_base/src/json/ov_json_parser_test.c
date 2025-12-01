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
        @file           ov_json_parser_test.c
        @author         Markus Toepfer

        @date           2018-12-05

        @ingroup        ov_json_parser

        @brief          Unit tests of a default JSON string parser.

        ------------------------------------------------------------------------
*/
#include "ov_json_parser.c"
#include <ov_test/testrun.h>

int check_json_parser_write_if_not_null() {

  char buffer[100] = {0};

  char *text = "test";
  char *next = buffer;
  size_t open = 100;

  testrun(json_parser_write_if_not_null(NULL, NULL, NULL));
  testrun(json_parser_write_if_not_null(NULL, &next, NULL));
  testrun(buffer[0] == 0);
  testrun(json_parser_write_if_not_null(NULL, &next, &open));
  testrun(buffer[0] == 0);
  testrun(json_parser_write_if_not_null(NULL, NULL, &open));
  testrun(buffer[0] == 0);
  testrun(!json_parser_write_if_not_null(text, NULL, NULL));
  testrun(!json_parser_write_if_not_null(text, &next, NULL));
  testrun(!json_parser_write_if_not_null(text, NULL, &open));
  testrun(buffer[0] == 0);

  testrun(json_parser_write_if_not_null(text, &next, &open));
  testrun(buffer[0] == 't');
  testrun(buffer[1] == 'e');
  testrun(buffer[2] == 's');
  testrun(buffer[3] == 't');
  testrun(buffer[4] == 0);
  testrun(open == 96);
  testrun(next == buffer + 4);
  testrun(next[0] == 0);
  testrun(next[-1] == 't');
  testrun(next[-2] == 's');
  testrun(next[-3] == 'e');
  testrun(next[-4] == 't');

  text = "abc";

  testrun(json_parser_write_if_not_null(text, &next, &open));
  testrun(buffer[0] == 't');
  testrun(buffer[1] == 'e');
  testrun(buffer[2] == 's');
  testrun(buffer[3] == 't');
  testrun(buffer[4] == 'a');
  testrun(buffer[5] == 'b');
  testrun(buffer[6] == 'c');
  testrun(buffer[7] == 0);
  testrun(open == 93);
  testrun(next == buffer + 7);
  testrun(next[0] == 0);
  testrun(next[-1] == 'c');
  testrun(next[-2] == 'b');
  testrun(next[-3] == 'a');
  testrun(next[-4] == 't');

  // write to frame end
  open = 2;
  text = "12";

  testrun(json_parser_write_if_not_null(text, &next, &open));
  testrun(buffer[0] == 't');
  testrun(buffer[1] == 'e');
  testrun(buffer[2] == 's');
  testrun(buffer[3] == 't');
  testrun(buffer[4] == 'a');
  testrun(buffer[5] == 'b');
  testrun(buffer[6] == 'c');
  testrun(buffer[7] == '1');
  testrun(buffer[8] == '2');
  testrun(buffer[9] == 0);
  testrun(open == 0);
  testrun(next == buffer + 9);
  testrun(next[0] == 0);
  testrun(next[-1] == '2');
  testrun(next[-2] == '1');
  testrun(next[-3] == 'c');
  testrun(next[-4] == 'b');

  // try to write beyond frame end
  open = 2;
  text = "xxx";

  testrun(!json_parser_write_if_not_null(text, &next, &open));
  testrun(buffer[0] == 't');
  testrun(buffer[1] == 'e');
  testrun(buffer[2] == 's');
  testrun(buffer[3] == 't');
  testrun(buffer[4] == 'a');
  testrun(buffer[5] == 'b');
  testrun(buffer[6] == 'c');
  testrun(buffer[7] == '1');
  testrun(buffer[8] == '2');
  testrun(buffer[9] == 0);
  testrun(open == 2);
  testrun((char *)next == (char *)buffer + 9);
  testrun(next[0] == 0);
  testrun(next[-1] == '2');
  testrun(next[-2] == '1');
  testrun(next[-3] == 'c');
  testrun(next[-4] == 'b');

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_config_stringify_minimal() {

  ov_json_stringify_config config = ov_json_config_stringify_minimal();

  testrun(config.literal.item.intro == NULL);
  testrun(config.literal.item.outro == NULL);
  testrun(config.literal.item.separator == NULL);
  testrun(config.literal.item.delimiter == NULL);
  testrun(config.literal.entry.intro == NULL);
  testrun(config.literal.entry.outro == NULL);
  testrun(config.literal.entry.indent == NULL);

  testrun(strncmp(config.string.item.intro, "\"", 1) == 0);
  testrun(strncmp(config.string.item.outro, "\"", 1) == 0);
  testrun(config.string.item.separator == NULL);
  testrun(config.string.item.delimiter == NULL)
      testrun(config.string.entry.intro == NULL);
  testrun(config.string.entry.outro == NULL);
  testrun(config.string.entry.indent == NULL);

  testrun(strncmp(config.array.item.intro, "[", 1) == 0);
  testrun(strncmp(config.array.item.outro, "]", 1) == 0);
  testrun(strncmp(config.array.item.separator, ",", 1) == 0);
  testrun(config.array.item.delimiter == NULL)
      testrun(config.array.entry.intro == NULL);
  testrun(config.array.entry.outro == NULL);
  testrun(config.array.entry.indent == NULL);

  testrun(strncmp(config.object.item.intro, "{", 1) == 0);
  testrun(strncmp(config.object.item.outro, "}", 1) == 0);
  testrun(strncmp(config.object.item.separator, ",", 1) == 0);
  testrun(strncmp(config.object.item.delimiter, ":", 1) == 0);
  testrun(config.object.entry.intro == NULL);
  testrun(config.object.entry.outro == NULL);
  testrun(config.object.entry.indent == NULL);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_config_stringify_default() {

  ov_json_stringify_config config = ov_json_config_stringify_default();

  testrun(config.literal.item.intro == NULL);
  testrun(config.literal.item.outro == NULL);
  testrun(config.literal.item.separator == NULL);
  testrun(config.literal.item.delimiter == NULL);
  testrun(config.literal.entry.intro == NULL);
  testrun(config.literal.entry.outro == NULL);
  testrun(config.literal.entry.indent == NULL);

  testrun(strncmp(config.string.item.intro, "\"", 1) == 0);
  testrun(strncmp(config.string.item.outro, "\"", 1) == 0);
  testrun(config.string.item.separator == NULL);
  testrun(config.string.item.delimiter == NULL);
  testrun(config.string.entry.intro == NULL);
  testrun(config.string.entry.outro == NULL);
  testrun(config.string.entry.indent == NULL);

  testrun(strncmp(config.array.item.intro, "[\n", 2) == 0);
  testrun(strncmp(config.array.item.out, "\n", 1) == 0);
  testrun(strncmp(config.array.item.outro, "]", 1) == 0);
  testrun(strncmp(config.array.item.separator, ",\n", 2) == 0);
  testrun(config.array.item.delimiter == NULL);
  testrun(config.array.entry.intro == NULL);
  testrun(config.array.entry.outro == NULL);
  testrun(config.array.entry.depth == true);
  testrun(strncmp(config.array.entry.indent, "\t", 1) == 0);

  testrun(strncmp(config.object.item.intro, "{\n", 2) == 0);
  testrun(strncmp(config.object.item.out, "\n", 1) == 0);
  testrun(strncmp(config.object.item.outro, "}", 1) == 0);
  testrun(strncmp(config.object.item.separator, ",\n", 2) == 0);
  testrun(strncmp(config.object.item.delimiter, ":", 1) == 0);
  testrun(config.object.entry.intro == NULL);
  testrun(config.object.entry.outro == NULL);
  testrun(config.object.entry.depth == true);
  testrun(strncmp(config.object.entry.indent, "\t", 1) == 0);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_stringify_config_validate() {

  ov_json_stringify_config config = ov_json_config_stringify_default();

  testrun(ov_json_stringify_config_validate(&config));
  config = ov_json_config_stringify_minimal();
  testrun(ov_json_stringify_config_validate(&config));

  testrun(!ov_json_stringify_config_validate(NULL));

  testrun(ov_json_stringify_config_validate(&config));
  config.string.item.intro = NULL;
  testrun(!ov_json_stringify_config_validate(&config));
  config.string.item.intro = "X";
  testrun(ov_json_stringify_config_validate(&config));

  testrun(ov_json_stringify_config_validate(&config));
  config.string.item.outro = NULL;
  testrun(!ov_json_stringify_config_validate(&config));
  config.string.item.outro = "X";
  testrun(ov_json_stringify_config_validate(&config));

  testrun(ov_json_stringify_config_validate(&config));
  config.array.item.intro = NULL;
  testrun(!ov_json_stringify_config_validate(&config));
  config.array.item.intro = "X";
  testrun(ov_json_stringify_config_validate(&config));

  testrun(ov_json_stringify_config_validate(&config));
  config.array.item.outro = NULL;
  testrun(!ov_json_stringify_config_validate(&config));
  config.array.item.outro = "X";
  testrun(ov_json_stringify_config_validate(&config));

  testrun(ov_json_stringify_config_validate(&config));
  config.array.item.separator = NULL;
  testrun(!ov_json_stringify_config_validate(&config));
  config.array.item.separator = "X";
  testrun(ov_json_stringify_config_validate(&config));

  testrun(ov_json_stringify_config_validate(&config));
  config.object.item.intro = NULL;
  testrun(!ov_json_stringify_config_validate(&config));
  config.object.item.intro = "X";
  testrun(ov_json_stringify_config_validate(&config));

  testrun(ov_json_stringify_config_validate(&config));
  config.object.item.outro = NULL;
  testrun(!ov_json_stringify_config_validate(&config));
  config.object.item.outro = "X";
  testrun(ov_json_stringify_config_validate(&config));

  testrun(ov_json_stringify_config_validate(&config));
  config.object.item.separator = NULL;
  testrun(!ov_json_stringify_config_validate(&config));
  config.object.item.separator = "X";
  testrun(ov_json_stringify_config_validate(&config));

  testrun(ov_json_stringify_config_validate(&config));
  config.object.item.delimiter = NULL;
  testrun(!ov_json_stringify_config_validate(&config));
  config.object.item.delimiter = "X";
  testrun(ov_json_stringify_config_validate(&config));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_parser_collocate_ascending() {

  ov_list *list = ov_list_create((ov_list_config){0});

  testrun(!ov_json_parser_collocate_ascending(NULL, NULL));
  testrun(!ov_json_parser_collocate_ascending("key", NULL));
  testrun(!ov_json_parser_collocate_ascending(NULL, list));

  testrun(ov_json_parser_collocate_ascending("key", list));
  testrun(1 == ov_list_count(list));
  testrun(0 == strncmp("key", list->get(list, 1), 3));

  testrun(ov_json_parser_collocate_ascending("c", list));
  testrun(2 == ov_list_count(list));
  testrun(0 == strncmp("c", list->get(list, 1), 1));
  testrun(0 == strncmp("key", list->get(list, 2), 3));

  testrun(ov_json_parser_collocate_ascending("b", list));
  testrun(3 == ov_list_count(list));
  testrun(0 == strncmp("b", list->get(list, 1), 1));
  testrun(0 == strncmp("c", list->get(list, 2), 1));
  testrun(0 == strncmp("key", list->get(list, 3), 3));

  testrun(ov_json_parser_collocate_ascending("d", list));
  testrun(4 == ov_list_count(list));
  testrun(0 == strncmp("b", list->get(list, 1), 1));
  testrun(0 == strncmp("c", list->get(list, 2), 1));
  testrun(0 == strncmp("d", list->get(list, 3), 1));
  testrun(0 == strncmp("key", list->get(list, 4), 3));

  testrun(ov_json_parser_collocate_ascending("abc", list));
  testrun(ov_json_parser_collocate_ascending("ab", list));
  testrun(ov_json_parser_collocate_ascending("a", list));
  testrun(ov_json_parser_collocate_ascending("b1", list));
  testrun(ov_json_parser_collocate_ascending("b2", list));

  testrun(9 == ov_list_count(list));
  testrun(0 == strncmp("a", list->get(list, 1), 1));
  testrun(0 == strncmp("ab", list->get(list, 2), 2));
  testrun(0 == strncmp("abc", list->get(list, 3), 3));
  testrun(0 == strncmp("b", list->get(list, 4), 1));
  testrun(0 == strncmp("b1", list->get(list, 5), 2));
  testrun(0 == strncmp("b2", list->get(list, 6), 2));
  testrun(0 == strncmp("c", list->get(list, 7), 1));
  testrun(0 == strncmp("d", list->get(list, 8), 1));
  testrun(0 == strncmp("key", list->get(list, 9), 3));

  ov_list_free(list);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_parser_calculate() {

  // check parsing
  char *expect = NULL;
  ov_json_value *value = ov_json_object();
  ov_json_stringify_config conf_minimal = ov_json_config_stringify_minimal();
  ov_json_stringify_config conf_default = ov_json_config_stringify_default();

  expect = "{}";

  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_minimal));
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_default));

  testrun(-1 == ov_json_parser_calculate(NULL, NULL));
  testrun(-1 == ov_json_parser_calculate(NULL, &conf_minimal));
  testrun(strlen(expect) == (size_t)ov_json_parser_calculate(value, NULL));

  testrun(ov_json_object_set(value, "1", ov_json_number(1)));
  expect = "{\"1\":1}";
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_minimal));
  expect = "{\n"
           "\t\"1\":1\n"
           "}";
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_default));
  value = ov_json_value_free(value);

  value = ov_json_literal(OV_JSON_NULL);
  expect = "null";
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_default));
  value = ov_json_value_free(value);

  value = ov_json_literal(OV_JSON_TRUE);
  expect = "true";
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_default));
  value = ov_json_value_free(value);

  value = ov_json_literal(OV_JSON_FALSE);
  expect = "false";
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_default));
  value = ov_json_value_free(value);

  value = ov_json_string("abc");
  expect = "\"abc\"";
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_default));
  value = ov_json_value_free(value);

  value = ov_json_number(123);
  expect = "123";
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_default));
  value = ov_json_value_free(value);

  value = ov_json_number(-1.23e5);
  expect = "-1.23e5";
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_default));
  value = ov_json_value_free(value);

  value = ov_json_array();
  expect = "[]";
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_default));
  testrun(ov_json_array_push(value, ov_json_number(1)));
  testrun(ov_json_array_push(value, ov_json_number(2)));
  testrun(ov_json_array_push(value, ov_json_number(3)));
  expect = "[1,2,3]";
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_minimal));
  expect = "[\n"
           "\t1,\n"
           "\t2,\n"
           "\t3\n"
           "]";
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_default));
  value = ov_json_value_free(value);

  value = ov_json_object();
  expect = "{}";
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_default));
  testrun(ov_json_object_set(value, "1", ov_json_number(1)));
  testrun(ov_json_object_set(value, "2", ov_json_number(2)));
  testrun(ov_json_object_set(value, "3", ov_json_number(3)));
  expect = "{\"1\":1,\"2\":2,\"3\":3}";
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_minimal));
  expect = "{\n"
           "\t\"1\":1,\n"
           "\t\"2\":2,\n"
           "\t\"3\":3\n"
           "}";
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_default));
  value = ov_json_value_free(value);

  // check with depth
  value = ov_json_object();
  expect = "{}";
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_default));
  testrun(ov_json_object_set(value, "1", ov_json_object()));
  testrun(ov_json_object_set(value, "2", ov_json_array()));
  testrun(ov_json_object_set(value, "3", ov_json_object()));
  expect = "{\"1\":{},\"2\":[],\"3\":{}}";
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_minimal));
  expect = "{\n"
           "\t\"1\":{},\n"
           "\t\"2\":[],\n"
           "\t\"3\":{}\n"
           "}";
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_default));
  ov_json_value *child = ov_json_object_get(value, "1");
  testrun(child);
  testrun(ov_json_object_set(child, "1.1", ov_json_true()));
  testrun(ov_json_object_set(child, "1.2", ov_json_array()));
  testrun(ov_json_object_set(child, "1.3", ov_json_number(1)));
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

  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_default));

  child = ov_json_object_get(value, "2");
  testrun(ov_json_array_push(child, ov_json_true()));
  testrun(ov_json_array_push(child, ov_json_array()));
  testrun(ov_json_array_push(child, ov_json_number(1)));
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
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_default));
  value = ov_json_value_free(value);

  ov_json_value_free(value);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_parser_encode() {

  size_t size = 5000;
  char buffer[size];
  memset(buffer, 0, size);

  // check parsing
  char *expect = NULL;
  ov_json_value *value = ov_json_object();
  ov_json_stringify_config conf_minimal = ov_json_config_stringify_minimal();
  ov_json_stringify_config conf_default = ov_json_config_stringify_default();

  expect = "{}";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer, size));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));

  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_default,
                                                          NULL, buffer, size));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));

  memset(buffer, 0, size);
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_encode(value, &conf_default,
                                        ov_json_parser_collocate_ascending,
                                        buffer, size));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));

  // check input error
  testrun(-1 == ov_json_parser_encode(NULL, &conf_default, NULL, buffer, size));
  testrun(-1 == ov_json_parser_encode(value, &conf_default, NULL, NULL, size));
  testrun(-1 == ov_json_parser_encode(value, &conf_default, NULL, buffer, 0));
  testrun(-1 == ov_json_parser_encode(value, &conf_default, NULL, buffer, 1));

  // no config (use minimal as default)
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_default,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));

  // min buffer size
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_default,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));

  // check object
  testrun(ov_json_object_set(value, "1", ov_json_number(1)));
  expect = "{\"1\":1}";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_minimal));
  expect = "{\n"
           "\t\"1\":1\n"
           "}";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_default,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_default));
  value = ov_json_value_free(value);

  // check array
  value = ov_json_array();
  expect = "[]";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_minimal));
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  ov_json_array_push(value, ov_json_number(1));
  ov_json_array_push(value, ov_json_number(2));
  ov_json_array_push(value, ov_json_number(3));
  expect = "[1,2,3]";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_minimal));
  expect = "[\n"
           "\t1,\n"
           "\t2,\n"
           "\t3\n"
           "]";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_default,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  value = ov_json_value_free(value);

  // check number
  value = ov_json_number(1);
  expect = "1";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_minimal));
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_default,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  testrun(ov_json_number_set(value, -1.23e1));
  expect = "-12.3";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer, size));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_minimal));
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_default,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  value = ov_json_value_free(value);

  // check string
  value = ov_json_string("test");
  expect = "\"test\"";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_minimal));
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_default,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  value = ov_json_value_free(value);

  // check literal
  value = ov_json_null();
  expect = "null";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_minimal));
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_default,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  value = ov_json_value_free(value);

  // check literal
  value = ov_json_true();
  expect = "true";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_minimal));
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_default,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  value = ov_json_value_free(value);

  // check literal
  value = ov_json_false();
  expect = "false";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_calculate(value, &conf_minimal));
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_default,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  value = ov_json_value_free(value);

  value = ov_json_array();
  testrun(ov_json_array_push(value, ov_json_number(1)));
  testrun(ov_json_array_push(value, ov_json_number(2)));
  testrun(ov_json_array_push(value, ov_json_number(3)));
  expect = "[1,2,3]";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer,
                                                          strlen(expect)));
  expect = "[\n"
           "\t1,\n"
           "\t2,\n"
           "\t3\n"
           "]";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_default,
                                                          NULL, buffer,
                                                          strlen(expect)));
  value = ov_json_value_free(value);

  value = ov_json_object();
  testrun(ov_json_object_set(value, "1", ov_json_number(1)));
  testrun(ov_json_object_set(value, "2", ov_json_number(2)));
  testrun(ov_json_object_set(value, "3", ov_json_number(3)));
  expect = "{\"1\":1,\"3\":3,\"2\":2}";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer,
                                                          strlen(expect)));
  memset(buffer, 0, size);
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_encode(value, &conf_minimal,
                                        ov_json_parser_collocate_ascending,
                                        buffer, strlen(expect)));

  expect = "{\n"
           "\t\"1\":1,\n"
           "\t\"2\":2,\n"
           "\t\"3\":3\n"
           "}";
  memset(buffer, 0, size);
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_encode(value, &conf_default,
                                        ov_json_parser_collocate_ascending,
                                        buffer, strlen(expect)));

  expect = "{\n"
           "\t\"1\":1,\n"
           "\t\"3\":3,\n"
           "\t\"2\":2\n"
           "}";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_default,
                                                          NULL, buffer,
                                                          strlen(expect)));
  value = ov_json_value_free(value);

  // check with depth
  value = ov_json_object();
  testrun(ov_json_object_set(value, "1", ov_json_object()));
  testrun(ov_json_object_set(value, "2", ov_json_array()));
  testrun(ov_json_object_set(value, "3", ov_json_object()));
  expect = "{\"1\":{},\"3\":[],\"2\":{}}";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_minimal,
                                                          NULL, buffer,
                                                          strlen(expect)));
  expect = "{\"1\":{},\"2\":[],\"3\":{}}";
  memset(buffer, 0, size);
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_encode(value, &conf_minimal,
                                        ov_json_parser_collocate_ascending,
                                        buffer, strlen(expect)));
  expect = "{\n"
           "\t\"1\":{},\n"
           "\t\"3\":[],\n"
           "\t\"2\":{}\n"
           "}";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_default,
                                                          NULL, buffer,
                                                          strlen(expect)));

  expect = "{\n"
           "\t\"1\":{},\n"
           "\t\"2\":[],\n"
           "\t\"3\":{}\n"
           "}";
  memset(buffer, 0, size);
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_encode(value, &conf_default,
                                        ov_json_parser_collocate_ascending,
                                        buffer, strlen(expect)));

  ov_json_value *child = ov_json_object_get(value, "1");
  testrun(child);
  testrun(ov_json_object_set(child, "1.1", ov_json_true()));
  testrun(ov_json_object_set(child, "1.2", ov_json_array()));
  testrun(ov_json_object_set(child, "1.3", ov_json_number(1)));
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

  memset(buffer, 0, size);

  testrun(strlen(expect) ==
          (size_t)ov_json_parser_encode(value, &conf_default,
                                        ov_json_parser_collocate_ascending,
                                        buffer, strlen(expect)));

  child = ov_json_object_get(value, "2");
  testrun(ov_json_array_push(child, ov_json_true()));
  testrun(ov_json_array_push(child, ov_json_array()));
  testrun(ov_json_array_push(child, ov_json_number(1)));
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

  testrun(strlen(expect) ==
          (size_t)ov_json_parser_encode(value, &conf_default,
                                        ov_json_parser_collocate_ascending,
                                        buffer, strlen(expect)));

  child = (ov_json_value *)ov_json_get(value, "/1/1.2");
  testrun(child);
  testrun(ov_json_array_push(child, ov_json_object()));
  testrun(ov_json_array_push(child, ov_json_object()));
  testrun(ov_json_array_push(child, ov_json_object()));
  child = (ov_json_value *)ov_json_get(value, "/1/1.2/1");
  testrun(child);
  testrun(ov_json_is_object(child));
  testrun(ov_json_object_set(child, "A", ov_json_number(1)));
  testrun(ov_json_object_set(child, "B", ov_json_number(2)));
  testrun(ov_json_object_set(child, "C", ov_json_number(3)));

  expect = "{\n"
           "\t\"1\":\n"
           "\t{\n"
           "\t\t\"1.1\":true,\n"
           "\t\t\"1.2\":\n"
           "\t\t[\n"
           "\t\t\t{},\n"
           "\t\t\t{\n"
           "\t\t\t\t\"A\":1,\n"
           "\t\t\t\t\"B\":2,\n"
           "\t\t\t\t\"C\":3\n"
           "\t\t\t},\n"
           "\t\t\t{}\n"
           "\t\t],\n"
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

  memset(buffer, 0, size);
  value = ov_json_value_free(value);

  // check with depth, intro and outro
  conf_minimal.intro = "\n";
  conf_minimal.outro = "\r\n";
  conf_default.intro = "\n";
  conf_default.outro = "\r\n";
  value = ov_json_object();
  testrun(ov_json_object_set(value, "1", ov_json_object()));
  testrun(ov_json_object_set(value, "2", ov_json_array()));
  testrun(ov_json_object_set(value, "3", ov_json_object()));
  expect = "\n{\"1\":{},\"3\":{},\"2\":[]}\r\n";
  memset(buffer, 0, size);
  size_t slen = strlen(expect);
  testrun(slen == (size_t)ov_json_parser_encode(value, &conf_minimal, NULL,
                                                buffer, slen));
  /*
  testrun_log(    "buffer %s|%"PRIu64"\n"
                  "expect %s|%"PRIu64"\n",
                  buffer, strlen(buffer), expect, strlen(expect));
  */
  testrun(0 == strncmp(buffer, expect, strlen(expect)));

  expect = "\n{\"1\":{},\"2\":[],\"3\":{}}\r\n";
  memset(buffer, 0, size);
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_encode(value, &conf_minimal,
                                        ov_json_parser_collocate_ascending,
                                        buffer, strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));

  expect = "\n"
           "{\n"
           "\t\"1\":{},\n"
           "\t\"3\":{},\n"
           "\t\"2\":[]\n"
           "}\r\n";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_default,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));
  value = ov_json_value_free(value);

  // check with depth, intro and outro
  conf_minimal.intro = NULL;
  conf_minimal.outro = "\n";
  conf_default.intro = NULL;
  conf_default.outro = "\n";
  value = ov_json_object();
  testrun(ov_json_object_set(value, "1", ov_json_object()));
  testrun(ov_json_object_set(value, "2", ov_json_array()));
  testrun(ov_json_object_set(value, "3", ov_json_object()));
  expect = "{\"1\":{},\"3\":{},\"2\":[]}\n";
  memset(buffer, 0, size);
  slen = strlen(expect);
  testrun(slen == (size_t)ov_json_parser_encode(value, &conf_minimal, NULL,
                                                buffer, slen));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));

  expect = "{\"1\":{},\"2\":[],\"3\":{}}\n";
  memset(buffer, 0, size);
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_encode(value, &conf_minimal,
                                        ov_json_parser_collocate_ascending,
                                        buffer, strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));

  expect = "{\n"
           "\t\"1\":{},\n"
           "\t\"3\":{},\n"
           "\t\"2\":[]\n"
           "}\n";
  memset(buffer, 0, size);
  testrun(strlen(expect) == (size_t)ov_json_parser_encode(value, &conf_default,
                                                          NULL, buffer,
                                                          strlen(expect)));
  testrun(0 == strncmp(buffer, expect, strlen(expect)));

  /*
          ov_json_parser_encode(
                  value,
                  &conf_default,
                  ov_json_parser_collocate_ascending,
                  buffer, strlen(expect));

          testrun_log("%s\n|\n%s", buffer, expect);
  */
  testrun(strlen(expect) ==
          (size_t)ov_json_parser_encode(value, &conf_default,
                                        ov_json_parser_collocate_ascending,
                                        buffer, strlen(expect)));

  value = ov_json_value_free(value);
  ov_json_value_free(value);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_parser_decode() {

  ov_json_value *value = NULL;
  char *string = NULL;

  testrun(-1 == ov_json_parser_decode(NULL, NULL, 0));
  testrun(-1 == ov_json_parser_decode(NULL, "{}", 2));
  testrun(-1 == ov_json_parser_decode(&value, NULL, 2));
  testrun(-1 == ov_json_parser_decode(&value, "{}", 0));
  testrun(-1 == ov_json_parser_decode(&value, "{}", 1));
  testrun(2 == ov_json_parser_decode(&value, "{}", 2));
  testrun(ov_json_is_object(value));
  testrun(ov_json_object_is_empty(value));
  value = value->free(value);

  testrun(0 == value);

  // parse to non existing object value
  testrun(2 == ov_json_parser_decode(&value, "{}", 2));
  testrun(ov_json_is_object(value));
  testrun(ov_json_object_is_empty(value));
  // parse to existing object value
  testrun(2 == ov_json_parser_decode(&value, "{}", 100));
  testrun(ov_json_is_object(value));
  testrun(ov_json_object_is_empty(value));
  // parse with whitespace
  testrun(3 == ov_json_parser_decode(&value, " {}", 100));
  testrun(ov_json_is_object(value));
  testrun(ov_json_object_is_empty(value));
  testrun(2 == ov_json_parser_decode(&value, "{} ", 100));
  testrun(ov_json_is_object(value));
  testrun(ov_json_object_is_empty(value));
  testrun(3 == ov_json_parser_decode(&value, " {} ", 100));
  testrun(ov_json_is_object(value));
  testrun(ov_json_object_is_empty(value));
  testrun(6 == ov_json_parser_decode(&value, "\r\n\t {} ", 100));
  testrun(ov_json_is_object(value));
  testrun(ov_json_object_is_empty(value));
  // parse to different value
  testrun(-1 == ov_json_parser_decode(&value, "[]", 100));
  // with content
  testrun(9 == ov_json_parser_decode(&value, "{\"key\":1}", 100));
  testrun(ov_json_is_object(value));
  testrun(!ov_json_object_is_empty(value));

  // parse to non existing array value
  value = ov_json_value_free(value);
  testrun(2 == ov_json_parser_decode(&value, "[]", 2));
  testrun(ov_json_is_array(value));
  testrun(ov_json_array_is_empty(value));
  // parse to existing array value
  testrun(2 == ov_json_parser_decode(&value, "[]", 100));
  testrun(ov_json_is_array(value));
  testrun(ov_json_array_is_empty(value));
  // parse with whitespace
  testrun(3 == ov_json_parser_decode(&value, " []", 100));
  testrun(ov_json_is_array(value));
  testrun(ov_json_array_is_empty(value));
  testrun(2 == ov_json_parser_decode(&value, "[] ", 100));
  testrun(ov_json_is_array(value));
  testrun(ov_json_array_is_empty(value));
  testrun(3 == ov_json_parser_decode(&value, " [] ", 100));
  testrun(ov_json_is_array(value));
  testrun(ov_json_array_is_empty(value));
  testrun(6 == ov_json_parser_decode(&value, "\r\n\t [] ", 100));
  testrun(ov_json_is_array(value));
  testrun(ov_json_array_is_empty(value));
  // parse to different value
  testrun(-1 == ov_json_parser_decode(&value, "{}", 100));
  // with content
  testrun(3 == ov_json_parser_decode(&value, "[1]", 100));
  testrun(ov_json_is_array(value));
  testrun(!ov_json_array_is_empty(value));
  value = ov_json_value_free(value);

  // parse to non existing string value
  value = ov_json_value_free(value);
  testrun(3 == ov_json_parser_decode(&value, "\"1\"", 3));
  testrun(ov_json_is_string(value));
  testrun(0 == strncmp(ov_json_string_get(value), "1", 1));
  // parse to existing array value
  testrun(3 == ov_json_parser_decode(&value, "\"1\"", 100));
  testrun(ov_json_is_string(value));
  testrun(0 == strncmp(ov_json_string_get(value), "1", 1));
  // parse with whitespace
  testrun(4 == ov_json_parser_decode(&value, " \"1\"", 100));
  testrun(ov_json_is_string(value));
  testrun(0 == strncmp(ov_json_string_get(value), "1", 1));
  testrun(3 == ov_json_parser_decode(&value, "\"1\" ", 100));
  testrun(ov_json_is_string(value));
  testrun(0 == strncmp(ov_json_string_get(value), "1", 1));
  testrun(4 == ov_json_parser_decode(&value, " \"1\" ", 100));
  testrun(ov_json_is_string(value));
  testrun(0 == strncmp(ov_json_string_get(value), "1", 1));
  testrun(7 == ov_json_parser_decode(&value, "\r\n\t \"1\"", 100));
  testrun(ov_json_is_string(value));
  testrun(0 == strncmp(ov_json_string_get(value), "1", 1));
  // parse to different value
  testrun(-1 == ov_json_parser_decode(&value, "{}", 100));
  value = ov_json_value_free(value);

  // parse to non existing number value
  value = ov_json_value_free(value);
  testrun(1 == ov_json_parser_decode(&value, "1,", 2));
  testrun(ov_json_is_number(value));
  testrun(1 == ov_json_number_get(value));
  // parse to existing array value
  testrun(1 == ov_json_parser_decode(&value, "1,", 100));
  testrun(ov_json_is_number(value));
  testrun(1 == ov_json_number_get(value));
  // parse with whitespace
  testrun(2 == ov_json_parser_decode(&value, " 1,", 100));
  testrun(ov_json_is_number(value));
  testrun(1 == ov_json_number_get(value));
  testrun(1 == ov_json_parser_decode(&value, "1, ", 100));
  testrun(ov_json_is_number(value));
  testrun(1 == ov_json_number_get(value));
  testrun(2 == ov_json_parser_decode(&value, " 1, ", 100));
  testrun(ov_json_is_number(value));
  testrun(1 == ov_json_number_get(value));
  testrun(5 == ov_json_parser_decode(&value, "\r\n\t 1,", 100));
  testrun(ov_json_is_number(value));
  testrun(1 == ov_json_number_get(value));
  // parse to different value
  testrun(-1 == ov_json_parser_decode(&value, "{}", 100));
  value = ov_json_value_free(value);
  // check number MUST be part of a value
  testrun(-1 == ov_json_parser_decode(&value, "1e", 100));
  testrun(-1 == ov_json_parser_decode(&value, "1x", 100));
  testrun(-1 == ov_json_parser_decode(&value, "1g", 100));
  testrun(-1 == ov_json_parser_decode(&value, "1e ", 100));

  // valid following items
  testrun(1 == ov_json_parser_decode(&value, "1,", 100));
  testrun(ov_json_is_number(value));
  testrun(1 == ov_json_number_get(value));
  testrun(1 == ov_json_parser_decode(&value, "1]", 100));
  testrun(ov_json_is_number(value));
  testrun(1 == ov_json_number_get(value));
  testrun(1 == ov_json_parser_decode(&value, "1}", 100));
  testrun(ov_json_is_number(value));
  testrun(1 == ov_json_number_get(value));
  testrun(1 == ov_json_parser_decode(&value, "1 ,", 100));
  testrun(ov_json_is_number(value));
  testrun(1 == ov_json_number_get(value));
  testrun(1 == ov_json_parser_decode(&value, "1\r\n\t ,\r\n\t ", 100));
  testrun(ov_json_is_number(value));
  testrun(1 == ov_json_number_get(value));

  // parse to non existing literal value
  value = ov_json_value_free(value);
  testrun(4 == ov_json_parser_decode(&value, "null", 4));
  testrun(ov_json_is_null(value));
  // parse to existing literal value
  testrun(4 == ov_json_parser_decode(&value, "true", 100));
  testrun(ov_json_is_true(value));
  testrun(5 == ov_json_parser_decode(&value, "false", 100));
  testrun(ov_json_is_false(value));
  // parse with whitespace
  testrun(5 == ov_json_parser_decode(&value, " null", 100));
  testrun(ov_json_is_null(value));
  testrun(4 == ov_json_parser_decode(&value, "true ", 100));
  testrun(ov_json_is_true(value));
  testrun(6 == ov_json_parser_decode(&value, " false ", 100));
  testrun(ov_json_is_false(value));
  testrun(8 == ov_json_parser_decode(&value, "\r\n\t true", 100));
  testrun(ov_json_is_true(value));
  // parse to different value
  testrun(-1 == ov_json_parser_decode(&value, "{}", 100));
  value = ov_json_value_free(value);

  // some decode tests
  string = "{}";
  testrun(2 == ov_json_parser_decode(&value, string, strlen(string)));
  testrun(ov_json_is_object(value));
  value = ov_json_value_free(value);

  string = "[]";
  testrun(2 == ov_json_parser_decode(&value, string, strlen(string)));
  testrun(ov_json_is_array(value));
  value = ov_json_value_free(value);

  string = "\"abc\"";
  testrun(5 == ov_json_parser_decode(&value, string, strlen(string)));
  testrun(ov_json_is_string(value));
  value = ov_json_value_free(value);

  string = "-1,";
  testrun(2 == ov_json_parser_decode(&value, string, strlen(string)));
  testrun(ov_json_is_number(value));
  value = ov_json_value_free(value);

  string = "-1.1e-12,";
  testrun(8 == ov_json_parser_decode(&value, string, strlen(string)));
  testrun(ov_json_is_number(value));
  value = ov_json_value_free(value);

  string = "1.2E10,";
  testrun(6 == ov_json_parser_decode(&value, string, strlen(string)));
  testrun(ov_json_is_number(value));
  value = ov_json_value_free(value);

  string = "null";
  testrun(4 == ov_json_parser_decode(&value, string, strlen(string)));
  testrun(ov_json_is_null(value));
  value = ov_json_value_free(value);

  string = "true";
  testrun(4 == ov_json_parser_decode(&value, string, strlen(string)));
  testrun(ov_json_is_true(value));
  value = ov_json_value_free(value);

  string = "false";
  testrun(5 == ov_json_parser_decode(&value, string, strlen(string)));
  testrun(ov_json_is_false(value));
  value = ov_json_value_free(value);

  string = "{\"key\":[ null, true,1, 1212 \n\r, 2]}";
  testrun(strlen(string) ==
          (size_t)ov_json_parser_decode(&value, string, strlen(string)));
  testrun(ov_json_is_object(value));
  testrun(1 == ov_json_object_count(value));
  testrun(5 == ov_json_array_count(ov_json_object_get(value, "key")));
  testrun(ov_json_is_null(ov_json_get(value, "/key/0")));
  testrun(ov_json_is_true(ov_json_get(value, "/key/1")));
  testrun(ov_json_is_number(ov_json_get(value, "/key/2")));
  testrun(ov_json_is_number(ov_json_get(value, "/key/3")));
  testrun(ov_json_is_number(ov_json_get(value, "/key/4")));

  // Double check that parsing works with ordinary strings
  string = "{\"key\":\"abc\"}";
  testrun(strlen(string) ==
          (size_t)ov_json_parser_decode(&value, string, strlen(string)));

  // Parse quoted backslashes
  string = "{\"key\":\"abc\\\\def\"}";
  testrun(strlen(string) ==
          (size_t)ov_json_parser_decode(&value, string, strlen(string)));

  value = ov_json_value_free(value);

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
  testrun_test(check_json_parser_write_if_not_null);
  testrun_test(test_ov_json_config_stringify_minimal);
  testrun_test(test_ov_json_config_stringify_default);
  testrun_test(test_ov_json_stringify_config_validate);
  testrun_test(test_ov_json_parser_collocate_ascending);

  testrun_test(test_ov_json_parser_calculate);
  testrun_test(test_ov_json_parser_encode);
  testrun_test(test_ov_json_parser_decode);

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
