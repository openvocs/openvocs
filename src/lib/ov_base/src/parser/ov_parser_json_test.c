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
        @file           ov_impl_parser_json_test.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2019-01-31

        @ingroup        ov_parser_json

        @brief          Unit tests of ov_parser_json

                        Due to the special parser configuration with a
                        stringify config, ov_parser_interface_tests
                        cannot be applied.



        ------------------------------------------------------------------------
*/
#include "ov_parser_json.c"
#include <ov_test/testrun.h>

static void clear_test_data(ov_parser_data *data) {

  if (!data)
    return;

  if (data->out.free)
    data->out.free(data->out.data);

  data->out.data = NULL;
  data->out.free = NULL;

  if (data->in.free)
    data->in.free(data->in.data);

  data->in.data = NULL;
  data->out.free = NULL;
  return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_parser_json_create() {

  ov_parser_config config;
  memset(&config, 0, sizeof(config));

  ov_json_stringify_config stringify = ov_json_config_stringify_minimal();

  ov_parser *parser = NULL;
  JsonParser *jp = NULL;

  testrun(!ov_parser_json_create(config, stringify));

  // check minimal config
  parser = ov_parser_json_create(config, stringify);
  testrun(parser);
  jp = AS_JSON_PARSER(parser);
  testrun(jp);

  // check functions
  testrun(parser->free == impl_parser_json_free);
  testrun(parser->decode == impl_parser_json_decode);
  testrun(parser->encode == impl_parser_json_encode);
  testrun(parser->buffer.is_enabled == impl_parser_json_buffer_is_enabled);
  testrun(parser->buffer.has_data == impl_parser_json_buffer_has_data);
  testrun(parser->buffer.empty_out == impl_parser_json_buffer_empty_out);

  // check config
  testrun(false == jp->config.buffering);
  testrun(NULL == jp->config.custom);

  testrun(0 == strncmp(parser->name, OV_PARSER_JSON_NAME,
                       strlen(OV_PARSER_JSON_NAME)));

  testrun(NULL == ov_parser_free(parser));

  // check maximal config
  strcat(config.name, "MAX");
  config.buffering = true;
  parser = ov_parser_json_create(config, stringify);
  testrun(parser);
  jp = AS_JSON_PARSER(parser);
  testrun(jp);

  // check functions
  testrun(parser->free == impl_parser_json_free);
  testrun(parser->decode == impl_parser_json_decode);
  testrun(parser->encode == impl_parser_json_encode);
  testrun(parser->buffer.is_enabled == impl_parser_json_buffer_is_enabled);
  testrun(parser->buffer.has_data == impl_parser_json_buffer_has_data);
  testrun(parser->buffer.empty_out == impl_parser_json_buffer_empty_out);

  // check config
  testrun(true == jp->config.buffering);
  testrun(NULL == jp->config.custom);

  testrun(0 == strncmp(parser->name, "MAX", 3));
  testrun(NULL == ov_parser_free(parser));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_parser_json_create_default() {

  ov_json_stringify_config stringify = ov_json_config_stringify_minimal();

  ov_parser_config config;
  memset(&config, 0, sizeof(config));

  ov_parser *parser = NULL;
  JsonParser *jp = NULL;

  testrun(!ov_parser_json_create_default(config));

  // check minimal config
  parser = ov_parser_json_create_default(config);
  testrun(parser);
  jp = AS_JSON_PARSER(parser);
  testrun(jp);

  // check functions
  testrun(parser->free == impl_parser_json_free);
  testrun(parser->decode == impl_parser_json_decode);
  testrun(parser->encode == impl_parser_json_encode);
  testrun(parser->buffer.is_enabled == impl_parser_json_buffer_is_enabled);
  testrun(parser->buffer.has_data == impl_parser_json_buffer_has_data);
  testrun(parser->buffer.empty_out == impl_parser_json_buffer_empty_out);

  testrun(0 ==
          memcmp(&jp->stringify, &stringify, sizeof(ov_json_stringify_config)));

  // check config
  testrun(false == jp->config.buffering);
  testrun(NULL == jp->config.custom);

  testrun(0 == strncmp(parser->name, OV_PARSER_JSON_NAME,
                       strlen(OV_PARSER_JSON_NAME)));

  testrun(NULL == ov_parser_free(parser));

  // check maximal config
  strcat(config.name, "MAX");
  config.buffering = true;
  parser = ov_parser_json_create_default(config);
  testrun(parser);
  jp = AS_JSON_PARSER(parser);
  testrun(jp);

  // check functions
  testrun(parser->free == impl_parser_json_free);
  testrun(parser->decode == impl_parser_json_decode);
  testrun(parser->encode == impl_parser_json_encode);
  testrun(parser->buffer.is_enabled == impl_parser_json_buffer_is_enabled);
  testrun(parser->buffer.has_data == impl_parser_json_buffer_has_data);
  testrun(parser->buffer.empty_out == impl_parser_json_buffer_empty_out);

  // check config
  testrun(true == jp->config.buffering);
  testrun(NULL == jp->config.custom);

  testrun(0 == strncmp(parser->name, "MAX", 3));
  testrun(NULL == ov_parser_free(parser));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_parser_json_free() {

  ov_parser_config config = {.buffering = true};

  ov_json_stringify_config stringify = ov_json_config_stringify_minimal();

  ov_buffer *buffer = ov_buffer_create(100);

  ov_parser_data data;
  memset(&data, 0, sizeof(data));

  ov_parser *parser = NULL;
  parser = ov_parser_json_create(config, stringify);
  testrun(parser);
  testrun(parser->free);
  testrun(NULL == parser->free(parser));

  // check free with buffered data
  parser = ov_parser_json_create(config, stringify);
  testrun(parser);

  // check input (which will be buffered)
  char *string = "{\"key\":\"value\",";
  testrun(ov_buffer_set(buffer, string, strlen(string)));
  data.in.data = buffer;
  data.in.free = ov_buffer_free;
  testrun(OV_PARSER_PROGRESS == impl_parser_json_decode(parser, &data));
  testrun(NULL == data.out.data);
  testrun(NULL == data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(parser->buffer.has_data(parser));

  testrun(NULL == parser->free(parser));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_parser_json_encode() {

  ov_parser_data data;
  memset(&data, 0, sizeof(data));
  ov_parser_config config = {0};

  ov_json_stringify_config stringify = ov_json_config_stringify_minimal();

  char *expect = NULL;
  ov_buffer *buffer = NULL;
  ov_parser *parser = ov_parser_json_create(config, stringify);
  testrun(parser);

  // prepare some data
  ov_json_value *value = ov_json_object();
  testrun(ov_json_is_object(value));

  data.in.data = value;
  data.in.free = ov_json_value_free;

  testrun(OV_PARSER_ERROR == impl_parser_json_encode(NULL, NULL));
  testrun(OV_PARSER_ERROR == impl_parser_json_encode(parser, NULL));
  testrun(OV_PARSER_ERROR == impl_parser_json_encode(NULL, &data));

  testrun(OV_PARSER_SUCCESS == impl_parser_json_encode(parser, &data));
  testrun(NULL != data.out.data);
  testrun(NULL != data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);

  expect = "{}";
  buffer = ov_buffer_cast(data.out.data);
  testrun(buffer);
  testrun(ov_buffer_free == data.out.free);
  testrun(0 == strncmp((char *)buffer->start, expect, strlen(expect)));
  testrun(NULL == data.out.free(data.out.data));
  memset(&data, 0, sizeof(data));

  value = ov_json_array();
  data.in.data = value;
  data.in.free = ov_json_value_free;
  testrun(OV_PARSER_SUCCESS == impl_parser_json_encode(parser, &data));
  testrun(NULL != data.out.data);
  testrun(NULL != data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  expect = "[]";
  buffer = ov_buffer_cast(data.out.data);
  testrun(buffer);
  testrun(ov_buffer_free == data.out.free);
  testrun(0 == strncmp((char *)buffer->start, expect, strlen(expect)));
  testrun(NULL == data.out.free(data.out.data));
  memset(&data, 0, sizeof(data));

  // non json input
  data.in.data = ov_buffer_create(0);
  testrun(OV_PARSER_ERROR == impl_parser_json_encode(parser, &data));
  testrun(NULL == data.out.data);
  testrun(NULL == data.out.free);
  testrun(NULL != data.in.data);
  testrun(NULL == data.in.free);
  testrun(NULL == ov_buffer_free(data.in.data));
  memset(&data, 0, sizeof(data));

  // FULL IO cycle
  expect = "{\"key\":[\"value\",1,[],{}]}";
  value = ov_json_read(expect, strlen(expect));
  testrun(ov_json_is_object(value));
  data.in.data = value;
  data.in.free = ov_json_value_free;
  testrun(OV_PARSER_SUCCESS == impl_parser_json_encode(parser, &data));
  buffer = ov_buffer_cast(data.out.data);
  testrun(buffer);
  testrun(ov_buffer_free == data.out.free);
  testrun(0 == strncmp((char *)buffer->start, expect, strlen(expect)));
  testrun(NULL == data.out.free(data.out.data));
  memset(&data, 0, sizeof(data));
  testrun(NULL == ov_parser_free(parser));

  parser = ov_parser_json_create(config, stringify);
  testrun(parser);

  expect = "{\"key\":[\"value\",1,[],{}]}";
  value = ov_json_read(expect, strlen(expect));
  testrun(ov_json_is_object(value));
  data.in.data = value;
  data.in.free = ov_json_value_free;
  testrun(OV_PARSER_SUCCESS == impl_parser_json_encode(parser, &data));
  buffer = ov_buffer_cast(data.out.data);
  testrun(buffer);
  testrun(ov_buffer_free == data.out.free);
  testrun(0 == strncmp((char *)buffer->start, expect, strlen(expect)));
  testrun(NULL == data.out.free(data.out.data));
  memset(&data, 0, sizeof(data));

  // strict mode will NOT allow non OBJECT input
  expect = "[\"value\",1,[],{}]";
  value = ov_json_read(expect, strlen(expect));
  testrun(ov_json_is_array(value));
  data.in.data = value;
  data.in.free = ov_json_value_free;
  testrun(OV_PARSER_SUCCESS == impl_parser_json_encode(parser, &data));
  ov_parser_data_clear(&data);
  memset(&data, 0, sizeof(data));

  testrun(NULL == ov_parser_free(parser));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_json_parser_encode_default() {

  ov_parser_data data;
  memset(&data, 0, sizeof(data));
  ov_parser_config config = {0};

  ov_json_stringify_config stringify = ov_json_config_stringify_default();

  char *expect = NULL;
  ov_buffer *buffer = NULL;
  ov_parser *parser = ov_parser_json_create(config, stringify);
  testrun(parser);

  // prepare some data
  ov_json_value *value = ov_json_object();
  testrun(ov_json_is_object(value));

  data.in.data = value;
  data.in.free = ov_json_value_free;

  testrun(OV_PARSER_ERROR == impl_parser_json_encode(NULL, NULL));
  testrun(OV_PARSER_ERROR == impl_parser_json_encode(parser, NULL));
  testrun(OV_PARSER_ERROR == impl_parser_json_encode(NULL, &data));

  testrun(OV_PARSER_SUCCESS == impl_parser_json_encode(parser, &data));
  testrun(NULL != data.out.data);
  testrun(NULL != data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);

  expect = "{}";
  buffer = ov_buffer_cast(data.out.data);
  testrun(buffer);
  testrun(ov_buffer_free == data.out.free);
  testrun(0 == strncmp((char *)buffer->start, expect, strlen(expect)));
  testrun(NULL == data.out.free(data.out.data));
  memset(&data, 0, sizeof(data));

  value = ov_json_array();
  data.in.data = value;
  data.in.free = ov_json_value_free;
  testrun(OV_PARSER_SUCCESS == impl_parser_json_encode(parser, &data));
  testrun(NULL != data.out.data);
  testrun(NULL != data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  expect = "[]";
  buffer = ov_buffer_cast(data.out.data);
  testrun(buffer);
  testrun(ov_buffer_free == data.out.free);
  testrun(0 == strncmp((char *)buffer->start, expect, strlen(expect)));
  testrun(NULL == data.out.free(data.out.data));
  memset(&data, 0, sizeof(data));

  // non json input
  data.in.data = ov_buffer_create(0);
  testrun(OV_PARSER_ERROR == impl_parser_json_encode(parser, &data));
  testrun(NULL == data.out.data);
  testrun(NULL == data.out.free);
  testrun(NULL != data.in.data);
  testrun(NULL == data.in.free);
  testrun(NULL == ov_buffer_free(data.in.data));
  memset(&data, 0, sizeof(data));

  // FULL IO cycle
  expect = "{\n\t\"key\":\n\t[\n\t\t\"value\",\n\t\t1,\n\t\t[],\n\t\t{}"
           "\n\t]\n}";
  value = ov_json_read(expect, strlen(expect));
  testrun(ov_json_is_object(value));
  data.in.data = value;
  data.in.free = ov_json_value_free;
  testrun(OV_PARSER_SUCCESS == impl_parser_json_encode(parser, &data));
  buffer = ov_buffer_cast(data.out.data);
  testrun(buffer);
  testrun(ov_buffer_free == data.out.free);
  testrun(0 == strncmp((char *)buffer->start, expect, strlen(expect)));
  testrun(NULL == data.out.free(data.out.data));
  memset(&data, 0, sizeof(data));
  testrun(NULL == ov_parser_free(parser));

  parser = ov_parser_json_create(config, stringify);
  testrun(parser);

  expect = "{\n\t\"key\":\n\t[\n\t\t\"value\",\n\t\t1,\n\t\t[],\n\t\t{}"
           "\n\t]\n}";
  value = ov_json_read(expect, strlen(expect));
  testrun(ov_json_is_object(value));
  data.in.data = value;
  data.in.free = ov_json_value_free;
  testrun(OV_PARSER_SUCCESS == impl_parser_json_encode(parser, &data));
  buffer = ov_buffer_cast(data.out.data);
  testrun(buffer);
  testrun(ov_buffer_free == data.out.free);
  testrun(0 == strncmp((char *)buffer->start, expect, strlen(expect)));
  testrun(NULL == data.out.free(data.out.data));
  memset(&data, 0, sizeof(data));

  testrun(NULL == ov_parser_free(parser));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_parser_json_decode() {

  ov_parser_data data;
  memset(&data, 0, sizeof(data));
  ov_parser_config config = {0};

  ov_json_stringify_config stringify = ov_json_config_stringify_default();

  char *string = NULL;
  ov_buffer *buffer = NULL;
  ov_parser *parser = ov_parser_json_create(config, stringify);
  testrun(parser);

  ov_json_value *value = NULL;

  // prepare some data
  string = "{}";
  buffer = ov_buffer_create(100);
  testrun(ov_buffer_set(buffer, string, strlen(string)));

  data.in.data = buffer;
  data.in.free = ov_buffer_free;

  testrun(OV_PARSER_ERROR == impl_parser_json_decode(NULL, NULL));
  testrun(OV_PARSER_ERROR == impl_parser_json_decode(parser, NULL));
  testrun(OV_PARSER_ERROR == impl_parser_json_decode(NULL, &data));

  testrun(OV_PARSER_SUCCESS == impl_parser_json_decode(parser, &data));
  testrun(NULL != data.out.data);
  testrun(NULL != data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);

  value = data.out.data;
  testrun(ov_json_is_object(value));
  testrun(ov_json_object_is_empty(value));
  testrun(NULL == ov_json_value_free(value));
  memset(&data, 0, sizeof(data));

  testrun(false == config.buffering);

  string = "[]";
  buffer = ov_buffer_create(100);
  testrun(ov_buffer_set(buffer, string, strlen(string)));
  data.in.data = buffer;
  data.in.free = NULL;
  testrun(OV_PARSER_SUCCESS == impl_parser_json_decode(parser, &data));
  testrun(NULL != data.out.data);
  testrun(NULL != data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(ov_json_is_array(data.out.data));
  testrun(NULL == data.out.free(data.out.data));

  JsonParser *jParser = AS_JSON_PARSER(parser);
  testrun(jParser);

  string = "[]";
  testrun(ov_buffer_set(buffer, string, strlen(string)));
  memset(&data, 0, sizeof(data));
  data.in.data = buffer;
  data.in.free = NULL;
  testrun(OV_PARSER_SUCCESS == impl_parser_json_decode(parser, &data));
  testrun(NULL != data.out.data);
  testrun(NULL != data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  value = data.out.data;
  testrun(ov_json_is_array(value));
  testrun(ov_json_array_is_empty(value));
  testrun(NULL == ov_json_value_free(value));
  memset(&data, 0, sizeof(data));

  // check non buffering mode
  string = "{\"key\":";
  testrun(ov_buffer_set(buffer, string, strlen(string)));
  memset(&data, 0, sizeof(data));
  data.in.data = buffer;
  data.in.free = ov_buffer_free;
  testrun(OV_PARSER_MISMATCH == impl_parser_json_decode(parser, &data));
  testrun(NULL == data.out.data);
  testrun(NULL == data.out.free);
  testrun(NULL != data.in.data);
  testrun(NULL != data.in.free);

  // check valid input
  string = "{\"key\":\"value\", \"1\": { \"x\": [1,2,null, false]}, "
           "\"2\":2}";
  testrun(ov_buffer_set(buffer, string, strlen(string)));
  data.in.data = buffer;
  data.in.free = NULL;
  testrun(OV_PARSER_SUCCESS == impl_parser_json_decode(parser, &data));
  testrun(NULL != data.out.data);
  testrun(NULL != data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);

  value = data.out.data;
  testrun(ov_json_is_object(value));

  // check value content
  testrun(ov_json_is_string(ov_json_get(value, "/key")));
  testrun(ov_json_is_object(ov_json_get(value, "/1")));
  testrun(ov_json_is_array(ov_json_get(value, "/1/x")));
  testrun(1 == ov_json_number_get(ov_json_get(value, "/1/x/0")));
  testrun(2 == ov_json_number_get(ov_json_get(value, "/1/x/1")));
  testrun(ov_json_is_null(ov_json_get(value, "/1/x/2")));
  testrun(ov_json_is_false(ov_json_get(value, "/1/x/3")));
  testrun(ov_json_is_number(ov_json_get(value, "/2")));

  testrun(NULL == data.out.free(data.out.data));
  memset(&data, 0, sizeof(data));

  // check invalid input
  string = "{\"key\":\"value\",";
  testrun(ov_buffer_set(buffer, string, strlen(string)));
  data.in.data = buffer;
  data.in.free = NULL;
  testrun(OV_PARSER_MISMATCH == impl_parser_json_decode(parser, &data));
  testrun(NULL == data.out.data);
  testrun(NULL == data.out.free);
  testrun(NULL != data.in.data);
  testrun(NULL == data.in.free);
  testrun(buffer == data.in.data);

  // BUFFER POINTER MUST be untouched
  testrun(buffer == data.in.data);

  // check non buffer input
  memset(&data, 0, sizeof(data));
  data.in.data = ov_json_array();
  data.in.free = ov_json_value_free;
  testrun(OV_PARSER_ERROR == impl_parser_json_decode(parser, &data));
  testrun(NULL == data.out.data);
  testrun(NULL == data.out.free);
  testrun(NULL != data.in.data);
  testrun(NULL != data.in.free);
  testrun(NULL == data.in.free(data.in.data));

  testrun(NULL == ov_parser_free(parser));

  // check buffering parser
  config.buffering = true;
  parser = ov_parser_json_create(config, stringify);
  testrun(parser);
  testrun(true == parser->buffer.is_enabled(parser));

  // check valid input (same as non buffering)
  string = "{\"key\":\"value\", \"1\": { \"x\": [1,2,null, false]}, "
           "\"2\":2}";
  testrun(ov_buffer_set(buffer, string, strlen(string)));
  memset(&data, 0, sizeof(data));
  data.in.data = buffer;
  data.in.free = NULL;
  testrun(OV_PARSER_SUCCESS == impl_parser_json_decode(parser, &data));
  testrun(NULL != data.out.data);
  testrun(NULL != data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);

  value = data.out.data;
  testrun(ov_json_is_object(value));

  // check value content
  testrun(ov_json_is_string(ov_json_get(value, "/key")));
  testrun(ov_json_is_object(ov_json_get(value, "/1")));
  testrun(ov_json_is_array(ov_json_get(value, "/1/x")));
  testrun(1 == ov_json_number_get(ov_json_get(value, "/1/x/0")));
  testrun(2 == ov_json_number_get(ov_json_get(value, "/1/x/1")));
  testrun(ov_json_is_null(ov_json_get(value, "/1/x/2")));
  testrun(ov_json_is_false(ov_json_get(value, "/1/x/3")));
  testrun(ov_json_is_number(ov_json_get(value, "/2")));

  testrun(NULL == data.out.free(data.out.data));

  string = "{ \"1\":1, ";
  testrun(ov_buffer_set(buffer, string, strlen(string)));
  memset(&data, 0, sizeof(data));
  data.in.data = buffer;
  data.in.free = NULL;
  testrun(OV_PARSER_PROGRESS == impl_parser_json_decode(parser, &data));
  testrun(NULL == data.out.data);
  testrun(NULL == data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(true == parser->buffer.has_data(parser));

  string = "\"2\":2, ";
  testrun(ov_buffer_set(buffer, string, strlen(string)));
  memset(&data, 0, sizeof(data));
  data.in.data = buffer;
  data.in.free = NULL;
  testrun(OV_PARSER_PROGRESS == impl_parser_json_decode(parser, &data));
  testrun(NULL == data.out.data);
  testrun(NULL == data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(true == parser->buffer.has_data(parser));

  string = "\"3\":3}";
  testrun(ov_buffer_set(buffer, string, strlen(string)));
  memset(&data, 0, sizeof(data));
  data.in.data = buffer;
  data.in.free = NULL;
  testrun(OV_PARSER_SUCCESS == impl_parser_json_decode(parser, &data));
  testrun(NULL != data.out.data);
  testrun(NULL != data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(false == parser->buffer.has_data(parser));

  testrun(ov_json_is_object(data.out.data));
  testrun(1 == ov_json_number_get(ov_json_get(data.out.data, "/1")));
  testrun(2 == ov_json_number_get(ov_json_get(data.out.data, "/2")));
  testrun(3 == ov_json_number_get(ov_json_get(data.out.data, "/3")));
  testrun(3 == ov_json_object_count(data.out.data));
  testrun(NULL == data.out.free(data.out.data));

  // check with JSON whitespace after match

  string = "{ \"1\":1, ";
  testrun(ov_buffer_set(buffer, string, strlen(string)));
  memset(&data, 0, sizeof(data));
  data.in.data = buffer;
  data.in.free = NULL;
  testrun(OV_PARSER_PROGRESS == impl_parser_json_decode(parser, &data));
  testrun(NULL == data.out.data);
  testrun(NULL == data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(true == parser->buffer.has_data(parser));

  string = "\"2\":2}\r\n\t ";
  testrun(ov_buffer_set(buffer, string, strlen(string)));
  memset(&data, 0, sizeof(data));
  data.in.data = buffer;
  data.in.free = NULL;
  testrun(OV_PARSER_SUCCESS == impl_parser_json_decode(parser, &data));
  testrun(NULL != data.out.data);
  testrun(NULL != data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(true == parser->buffer.has_data(parser));
  testrun(ov_json_is_object(data.out.data));
  testrun(1 == ov_json_number_get(ov_json_get(data.out.data, "/1")));
  testrun(2 == ov_json_number_get(ov_json_get(data.out.data, "/2")));
  testrun(2 == ov_json_object_count(data.out.data));
  testrun(NULL == data.out.free(data.out.data));

  /*
      // only JSON whitespace left
      memset(&data, 0, sizeof(data));
      // Originally, the parser returned OV_PARSER_MISMATCH
      // When buffering, the parser must return OV_PARSER_PROGRESS
      // since in buffering mode, we never know what comes next -
      // if our buffer contains but '    ', the next chunk might contain valid
      // JSON See #342
      testrun(OV_PARSER_PROGRESS == impl_parser_json_decode(parser, &data));
      testrun(NULL == data.out.data);
      testrun(NULL == data.out.free);
      testrun(NULL == data.in.data);
      testrun(NULL == data.in.free);
      testrun(true == parser->buffer.has_data(parser));
  */
  ov_buffer *raw = NULL;
  void *(*free_raw)(void *) = NULL;
  testrun(true == parser->buffer.empty_out(parser, (void **)&raw, &free_raw));
  testrun(false == parser->buffer.has_data(parser));
  testrun(ov_buffer_cast(raw));
  testrun(ov_buffer_free == free_raw);
  string = "\r\n\t ";
  testrun(strlen(string) == raw->length);
  testrun(0 == strncmp((char *)raw->start, string, strlen(string)));
  testrun(NULL == free_raw(raw));

  // check with 2 JSON objects
  string = "{ \"1\":1, ";
  testrun(ov_buffer_set(buffer, string, strlen(string)));
  memset(&data, 0, sizeof(data));
  data.in.data = buffer;
  data.in.free = NULL;
  testrun(OV_PARSER_PROGRESS == impl_parser_json_decode(parser, &data));
  testrun(NULL == data.out.data);
  testrun(NULL == data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(true == parser->buffer.has_data(parser));

  string = "\"2\":2}{}{\"3\":3}";
  testrun(ov_buffer_set(buffer, string, strlen(string)));
  memset(&data, 0, sizeof(data));
  data.in.data = buffer;
  data.in.free = NULL;
  testrun(OV_PARSER_SUCCESS == impl_parser_json_decode(parser, &data));
  testrun(NULL != data.out.data);
  testrun(NULL != data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(true == parser->buffer.has_data(parser));
  testrun(ov_json_is_object(data.out.data));
  testrun(1 == ov_json_number_get(ov_json_get(data.out.data, "/1")));
  testrun(2 == ov_json_number_get(ov_json_get(data.out.data, "/2")));
  testrun(2 == ov_json_object_count(data.out.data));
  testrun(NULL == data.out.free(data.out.data));
  memset(&data, 0, sizeof(data));
  testrun(OV_PARSER_SUCCESS == impl_parser_json_decode(parser, &data));
  testrun(NULL != data.out.data);
  testrun(NULL != data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(true == parser->buffer.has_data(parser));
  testrun(ov_json_is_object(data.out.data));
  testrun(ov_json_object_is_empty(data.out.data));
  testrun(NULL == data.out.free(data.out.data));
  memset(&data, 0, sizeof(data));
  testrun(OV_PARSER_SUCCESS == impl_parser_json_decode(parser, &data));
  testrun(NULL != data.out.data);
  testrun(NULL != data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(false == parser->buffer.has_data(parser));
  testrun(ov_json_is_object(data.out.data));
  testrun(3 == ov_json_number_get(ov_json_get(data.out.data, "/3")));
  testrun(1 == ov_json_object_count(data.out.data));
  testrun(NULL == data.out.free(data.out.data));

  // check with valid content followed by invalid content
  string = "{ \"1\":1, ";
  testrun(ov_buffer_set(buffer, string, strlen(string)));
  memset(&data, 0, sizeof(data));
  data.in.data = buffer;
  data.in.free = NULL;
  testrun(OV_PARSER_PROGRESS == impl_parser_json_decode(parser, &data));
  testrun(NULL == data.out.data);
  testrun(NULL == data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(true == parser->buffer.has_data(parser));

  string = "{,}";
  testrun(ov_buffer_set(buffer, string, strlen(string)));
  memset(&data, 0, sizeof(data));
  data.in.data = buffer;
  data.in.free = NULL;
  testrun(OV_PARSER_MISMATCH == impl_parser_json_decode(parser, &data));
  testrun(NULL == data.out.data);
  testrun(NULL == data.out.free);
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(true == parser->buffer.has_data(parser));

  // check dump
  testrun(true == parser->buffer.empty_out(parser, (void **)&raw, &free_raw));
  testrun(false == parser->buffer.has_data(parser));
  testrun(ov_buffer_cast(raw));
  testrun(ov_buffer_free == free_raw);
  string = "{ \"1\":1, {,}";
  testrun(strlen(string) == raw->length);
  testrun(0 == strncmp((char *)raw->start, string, strlen(string)));
  testrun(NULL == free_raw(raw));

  // check invalid input type (no buffer)

  memset(&data, 0, sizeof(data));
  data.in.data = ov_json_array();
  data.in.free = NULL;
  testrun(OV_PARSER_ERROR == impl_parser_json_decode(parser, &data));
  testrun(NULL == data.out.data);
  testrun(NULL == data.out.free);
  testrun(NULL != data.in.data);
  testrun(NULL == data.in.free);
  testrun(false == parser->buffer.has_data(parser));
  testrun(NULL == ov_json_value_free(data.in.data));

  testrun(NULL == ov_parser_free(parser));

  // Error case encountered in OPS:
  // Buffering mode
  // Parser is fed some valid, but incomplete JSON

  config.buffering = true;
  parser = ov_parser_json_create(config, stringify);
  testrun(0 != parser);
  testrun(true == parser->buffer.is_enabled(parser));

  string = "{";
  testrun(ov_buffer_set(buffer, string, strlen(string)));
  memset(&data, 0, sizeof(data));
  data.in.data = buffer;
  data.in.free = 0;
  testrun(OV_PARSER_PROGRESS == impl_parser_json_decode(parser, &data));

  testrun(0 == ov_json_value_free(data.in.data));
  testrun(0 == ov_parser_free(parser));

  // Issue #342

  string = " ";

  config.buffering = true;
  parser = ov_parser_json_create(config, stringify);
  testrun(0 != parser);
  testrun(true == parser->buffer.is_enabled(parser));

  testrun(ov_buffer_set(buffer, string, strlen(string)));
  buffer->start[0] = 0;
  memset(&data, 0, sizeof(data));
  data.in.data = buffer;
  data.in.free = 0;

  testrun(OV_PARSER_PROGRESS == impl_parser_json_decode(parser, &data));

  testrun(0 == ov_json_value_free(data.in.data));
  testrun(0 == ov_parser_free(parser));

  // Clean up

  testrun(NULL == ov_buffer_free(buffer));

  return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

static bool decoder_fill_input(bool valid, ov_parser_data *const data) {

  if (!data)
    return false;

  data->out.data = NULL;
  data->out.free = NULL;

  char *string = NULL;

  data->in.data = ov_buffer_create(100);
  data->in.free = ov_buffer_free;

  if (valid) {

    string = "{\"key\":\"value\"}";

  } else {

    string = "{\"invalid JSON string\"}";
  }

  if (ov_buffer_set(data->in.data, string, strlen(string)))
    return true;

  ov_buffer_free(data->in.data);
  return false;
}

/*----------------------------------------------------------------------------*/

static bool decoder_fill_third(int number, ov_parser_data *const data) {

  if (!data)
    return false;

  data->out.data = NULL;
  data->out.free = NULL;

  data->in.data = ov_buffer_create(100);
  data->in.free = ov_buffer_free;

  char *string = NULL;

  switch (number) {

  case 1:
    string = "{\"1\":1,";
    break;
  case 2:
    string = "\"2\":2,";
    break;
  case 3:
    string = "\"3\":3}";
    break;
  }

  if (ov_buffer_set(data->in.data, string, strlen(string)))
    return true;

  ov_buffer_free(data->in.data);
  return false;
}

/*----------------------------------------------------------------------------*/

int test_impl_parser_json_buffer_is_enabled() {

  ov_parser_config config = {0};
  ov_json_stringify_config stringify = ov_json_config_stringify_default();

  ov_parser *parser = ov_parser_json_create(config, stringify);
  testrun(parser);

  testrun(!parser->buffer.is_enabled(parser));
  testrun(NULL == ov_parser_free(parser));

  config.buffering = true;
  parser = ov_parser_json_create(config, stringify);
  testrun(parser);
  testrun(parser->buffer.is_enabled(parser));

  testrun(NULL == ov_parser_free(parser));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_parser_json_buffer_has_data() {

  ov_json_stringify_config stringify = ov_json_config_stringify_default();

  ov_parser_config config = {.buffering = false};

  ov_parser *parser = ov_parser_json_create(config, stringify);
  testrun(parser);
  testrun(ov_parser_verify_interface(parser));

  testrun(!parser->buffer.is_enabled(parser));
  testrun(!parser->buffer.has_data(parser));
  testrun(NULL == ov_parser_free(parser));

  config.buffering = true;
  parser = ov_parser_json_create(config, stringify);
  testrun(parser);
  testrun(parser->buffer.is_enabled(parser));
  testrun(!parser->buffer.has_data(parser));

  ov_parser_data data;
  memset(&data, 0, sizeof(data));
  testrun(decoder_fill_third(1, &data));
  testrun(data.in.data);
  testrun(OV_PARSER_PROGRESS == parser->decode(parser, &data));
  testrun(NULL == data.out.data);
  testrun(parser->buffer.has_data(parser));

  // clear test data
  clear_test_data(&data);

  testrun(NULL == ov_parser_free(parser));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_parser_json_buffer_empty_out() {

  ov_parser_state state = OV_PARSER_ERROR;
  ov_json_stringify_config stringify = ov_json_config_stringify_default();

  ov_parser_config config = {.buffering = true};

  ov_parser *parser = ov_parser_json_create(config, stringify);
  testrun(parser);
  testrun(ov_parser_verify_interface(parser));

  void *raw = NULL;
  void *(*free_raw)(void *) = NULL;

  testrun(NULL != parser->buffer.empty_out);

  // empty out without any data
  testrun(!parser->buffer.empty_out(NULL, NULL, NULL));
  testrun(!parser->buffer.empty_out(NULL, &raw, &free_raw));
  testrun(!parser->buffer.empty_out(parser, NULL, &free_raw));
  testrun(!parser->buffer.empty_out(parser, &raw, NULL));

  testrun(!parser->buffer.has_data(parser));
  testrun(parser->buffer.empty_out(parser, &raw, &free_raw));
  testrun(NULL == raw);
  testrun(NULL == free_raw);

  ov_parser_data data;
  memset(&data, 0, sizeof(data));

  testrun(decoder_fill_input(true, &data));
  testrun(data.in.data);
  testrun(OV_PARSER_SUCCESS == ov_parser_decode(parser, &data));
  testrun(parser->buffer.empty_out(parser, &raw, &free_raw));
  testrun(NULL == raw);
  testrun(NULL == free_raw);
  // clear test data
  clear_test_data(&data);

  testrun(decoder_fill_input(false, &data));
  testrun(data.in.data);
  state = ov_parser_decode(parser, &data);
  if (OV_PARSER_MISMATCH != state)
    testrun(state == OV_PARSER_ERROR);

  if (NULL != data.out.data) {

    testrun(parser->buffer.has_data(parser));
    testrun(parser->buffer.empty_out(parser, &raw, &free_raw));
    testrun(!parser->buffer.has_data(parser));
    testrun(NULL != raw);
    if (free_raw)
      free_raw(raw);
  }

  // clear test data
  clear_test_data(&data);
  raw = NULL;
  free_raw = NULL;

  testrun(parser->buffer.empty_out(parser, &raw, &free_raw));
  if (free_raw)
    raw = free_raw(raw);

  testrun(decoder_fill_third(1, &data));
  testrun(data.in.data);
  state = ov_parser_decode(parser, &data);
  fprintf(stdout, "STATUS %i\n", state);
  testrun(OV_PARSER_PROGRESS == state);
  testrun(NULL == data.out.data);
  testrun(parser->buffer.has_data(parser));
  // clear test data
  clear_test_data(&data);
  testrun(decoder_fill_third(2, &data));
  testrun(data.in.data);
  testrun(OV_PARSER_PROGRESS == ov_parser_decode(parser, &data));
  testrun(NULL == data.out.data);
  testrun(parser->buffer.has_data(parser));
  testrun(parser->buffer.empty_out(parser, &raw, &free_raw));
  testrun(!parser->buffer.has_data(parser));
  testrun(NULL != raw);
  if (free_raw)
    free_raw(raw);
  // clear test data
  clear_test_data(&data);
  raw = NULL;
  free_raw = NULL;

  testrun(decoder_fill_third(1, &data));
  testrun(data.in.data);
  testrun(OV_PARSER_PROGRESS == ov_parser_decode(parser, &data));
  testrun(NULL == data.out.data);
  testrun(parser->buffer.has_data(parser));

  // clear test data
  clear_test_data(&data);
  testrun(decoder_fill_input(false, &data));
  testrun(data.in.data);
  state = ov_parser_decode(parser, &data);
  if (OV_PARSER_MISMATCH != state)
    testrun(state == OV_PARSER_ERROR);
  testrun(parser->buffer.has_data(parser));
  testrun(parser->buffer.empty_out(parser, &raw, &free_raw));
  testrun(!parser->buffer.has_data(parser));
  testrun(NULL != raw);
  if (free_raw)
    free_raw(raw);
  // clear test data
  clear_test_data(&data);
  raw = NULL;
  free_raw = NULL;

  testrun(NULL == ov_parser_free(parser));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int all_tests() {

  testrun_init();

  testrun_test(test_ov_parser_json_create);
  testrun_test(test_ov_parser_json_create_default);

  testrun_test(test_impl_parser_json_free);

  testrun_test(test_impl_parser_json_encode);
  testrun_test(test_impl_parser_json_decode);

  testrun_test(check_json_parser_encode_default);

  testrun_test(test_impl_parser_json_buffer_is_enabled);
  testrun_test(test_impl_parser_json_buffer_has_data);
  testrun_test(test_impl_parser_json_buffer_empty_out);

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
