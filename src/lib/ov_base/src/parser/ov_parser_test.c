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
        @file           ov_parser_test.c
        @author         Markus Toepfer

        @date           2019-01-30

        @ingroup        ov_parser

        @brief          Unit tests of


        ------------------------------------------------------------------------
*/

#include "ov_parser.c"

#include "../../include/ov_buffer.h"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_parser_cast() {

  ov_parser_config config;
  memset(&config, 0, sizeof(config));
  ov_parser *parser = ov_parser_dummy_create(config);

  testrun(parser);
  testrun(ov_parser_cast(parser));

  for (size_t i = 0; i < 0xffff; i++) {

    parser->magic_byte = i;

    if (i == OV_PARSER_MAGIC_BYTE) {
      testrun(ov_parser_cast(parser));
    } else {
      testrun(!ov_parser_cast(parser));
    }
  }

  parser->magic_byte = OV_PARSER_MAGIC_BYTE;
  testrun(NULL == ov_parser_free(parser));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool dummy_enable_buffer(const ov_parser *self) {

  if (!self)
    return false;

  return true;
}

/*----------------------------------------------------------------------------*/

int test_ov_parser_verify_interface() {

  ov_parser_config config;
  memset(&config, 0, sizeof(config));
  ov_parser *parser = ov_parser_dummy_create(config);

  testrun(parser);

  testrun(false == ov_parser_verify_interface(NULL));
  testrun(true == ov_parser_verify_interface(parser));

  parser->magic_byte = 0;
  testrun(false == ov_parser_verify_interface(parser));
  parser->magic_byte = OV_PARSER_MAGIC_BYTE;
  testrun(true == ov_parser_verify_interface(parser));

  parser->free = NULL;
  testrun(false == ov_parser_verify_interface(parser));
  parser->free = impl_dummy_parser_free;
  testrun(true == ov_parser_verify_interface(parser));

  parser->decode = NULL;
  testrun(false == ov_parser_verify_interface(parser));
  parser->decode = impl_dummy_parser_decode;
  testrun(true == ov_parser_verify_interface(parser));

  parser->buffer.is_enabled = NULL;
  testrun(false == ov_parser_verify_interface(parser));
  parser->buffer.is_enabled = impl_dummy_parser_buffer_is_enabled;
  testrun(true == ov_parser_verify_interface(parser));

  // buffering not enabled
  testrun(false == parser->buffer.is_enabled(parser));
  parser->buffer.has_data = NULL;
  parser->buffer.empty_out = NULL;
  testrun(true == ov_parser_verify_interface(parser));

  // buffering enabled
  parser->buffer.is_enabled = dummy_enable_buffer;
  testrun(true == parser->buffer.is_enabled(parser));
  parser->buffer.has_data = NULL;
  parser->buffer.empty_out = NULL;
  testrun(false == ov_parser_verify_interface(parser));
  parser->buffer.has_data = NULL;
  parser->buffer.empty_out = impl_dummy_parser_buffer_empty_out;
  testrun(false == ov_parser_verify_interface(parser));
  parser->buffer.has_data = impl_dummy_parser_buffer_has_data;
  parser->buffer.empty_out = NULL;
  testrun(false == ov_parser_verify_interface(parser));
  parser->buffer.has_data = impl_dummy_parser_buffer_has_data;
  parser->buffer.empty_out = impl_dummy_parser_buffer_empty_out;
  testrun(true == ov_parser_verify_interface(parser));

  testrun(NULL == ov_parser_free(parser));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_parser_set_head() {

  ov_parser_config config;
  memset(&config, 0, sizeof(config));
  ov_parser *parser = ov_parser_dummy_create(config);

  testrun(parser);
  testrun(ov_parser_cast(parser));

  for (size_t i = 0; i < 0xffff; i++) {

    parser->magic_byte = i;
    testrun(ov_parser_set_head(parser, i));
    testrun(parser->magic_byte == OV_PARSER_MAGIC_BYTE);
    testrun(parser->type == i);
  }

  // reset to free
  testrun(ov_parser_set_head(parser, IMPL_DUMMY_PARSER));
  testrun(NULL == ov_parser_free(parser));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_parser_free() {

  ov_parser_config config;
  memset(&config, 0, sizeof(config));
  ov_parser *parser = ov_parser_dummy_create(config);

  testrun(parser);
  testrun(NULL == ov_parser_free(NULL));
  testrun(NULL == ov_parser_free(parser));

  parser = ov_parser_dummy_create((ov_parser_config){0});
  parser->free = NULL;
  testrun(parser == ov_parser_free(parser));
  parser->free = impl_dummy_parser_free;
  testrun(NULL == ov_parser_free(parser));

  parser = ov_parser_dummy_create((ov_parser_config){0});
  parser->magic_byte = 0;
  testrun(parser == ov_parser_free(parser));
  parser->magic_byte = OV_PARSER_MAGIC_BYTE;
  testrun(NULL == ov_parser_free(parser));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_parser_free_chain() {

  ov_parser_config config;
  memset(&config, 0, sizeof(config));
  ov_parser *parser1 = ov_parser_dummy_create(config);
  ov_parser *parser2 = ov_parser_dummy_create(config);
  ov_parser *parser3 = ov_parser_dummy_create(config);

  testrun(parser1);
  testrun(NULL == parser_free_chain(NULL));
  testrun(NULL == parser_free_chain(parser1));

  parser1 = ov_parser_dummy_create(config);
  parser1->next = parser2;
  parser2->next = parser3;

  testrun(NULL == parser_free_chain(parser1));

  parser1 = ov_parser_dummy_create(config);
  parser2 = ov_parser_dummy_create(config);
  parser3 = ov_parser_dummy_create(config);

  // create chain from external
  parser1->next = parser2;
  parser2->next = parser3;

  testrun_log("... expecting some error message log");
  // cast failed
  parser2->magic_byte = 0;
  testrun(parser2 == parser_free_chain(parser1));
  // NOTE parser1 was freed!

  parser2->magic_byte = OV_PARSER_MAGIC_BYTE;
  testrun(NULL == parser_free_chain(parser2));

  // check valgrind run

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool dummy_buffer_has_data(const ov_parser *self) {

  if (!self)
    return false;

  return true;
}

/*----------------------------------------------------------------------------*/

int test_ov_parser_trash_buffered_data() {

  ov_parser_config config;
  memset(&config, 0, sizeof(config));

  char *name = "parser1";
  memcpy(config.name, name, strlen(name));
  ov_parser *parser1 = ov_parser_dummy_create(config);

  name = "parser2";
  memcpy(config.name, name, strlen(name));
  ov_parser *parser2 = ov_parser_dummy_create(config);

  name = "parser3";
  memcpy(config.name, name, strlen(name));
  ov_parser *parser3 = ov_parser_dummy_create(config);

  testrun(parser1);
  testrun(false == parser1->buffer.is_enabled(parser1));
  testrun(false == parser2->buffer.is_enabled(parser2));
  testrun(false == parser3->buffer.is_enabled(parser3));
  parser1->next = parser2;
  parser2->next = parser3;

  testrun(!ov_parser_trash_buffered_data(NULL));

  // with disabled buffering
  testrun(false == parser1->buffer.is_enabled(parser1));
  testrun(false == parser2->buffer.is_enabled(parser2));
  testrun(false == parser3->buffer.is_enabled(parser3));
  testrun(ov_parser_trash_buffered_data(parser1));

  // enable buffering somewhere in the chain
  // with enabled (empty) buffer (correct config)
  parser3->buffer.is_enabled = dummy_enable_buffer;
  testrun(false == parser1->buffer.is_enabled(parser1));
  testrun(false == parser2->buffer.is_enabled(parser2));
  testrun(true == parser3->buffer.is_enabled(parser3));
  testrun(true == ov_parser_verify_interface(parser3));
  testrun(false == parser3->buffer.has_data(parser3));
  testrun(ov_parser_trash_buffered_data(parser1));
  testrun(ov_parser_trash_buffered_data(parser3));
  testrun(ov_parser_trash_buffered_data(parser1));

  // failure verify (chained run MUST fail)
  parser3->buffer.empty_out = NULL;
  testrun(false == ov_parser_verify_interface(parser3));
  testrun(false == parser3->buffer.has_data(parser3));
  testrun(!ov_parser_trash_buffered_data(parser3));
  testrun(!ov_parser_trash_buffered_data(parser1));

  void *data = NULL;
  void *(*free_data)(void *) = NULL;

  // reset empty out
  parser3->buffer.empty_out = impl_dummy_parser_buffer_empty_out;

  // empty_out (no data)
  testrun(false == parser3->buffer.has_data(parser3));
  testrun(false == parser3->buffer.empty_out(parser3, &data, &free_data));
  testrun(true == ov_parser_verify_interface(parser3));
  testrun(ov_parser_trash_buffered_data(parser3));
  testrun(ov_parser_trash_buffered_data(parser1));

  // failure empty_out (with data)
  parser3->buffer.has_data = dummy_buffer_has_data;
  testrun(true == parser3->buffer.has_data(parser3));
  testrun(false == parser3->buffer.empty_out(parser3, &data, &free_data));
  testrun(true == ov_parser_verify_interface(parser3));
  testrun(!ov_parser_trash_buffered_data(parser1));
  testrun(!ov_parser_trash_buffered_data(parser3));

  testrun_log("flow of trash buffer tested without buffered data");

  testrun(NULL == parser_free_chain(parser1));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static void *dont_free(void *data) {

  if (data)
    return NULL;
  return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_parser_state dummy_buffer_copy_parse(ov_parser *self,
                                               ov_parser_data *const data) {

  ov_parser *parser = AS_DUMMY_PARSER(self);
  if (!parser || !data)
    goto error;

  if (!ov_buffer_cast(data->in.data))
    goto error;

  OV_ASSERT(data->in.data);
  OV_ASSERT(data->in.free);

  // copy incoming to outgoing
  data->out.data = NULL;
  if (!ov_buffer_copy(&data->out.data, data->in.data))
    goto error;

  data->out.free = data->in.free;

  // unset incoming
  data->in.data = data->in.free(data->in.data);
  data->in.free = NULL;

  return OV_PARSER_SUCCESS;
error:
  return OV_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

int test_ov_parser_encode() {

  ov_parser_config config;
  memset(&config, 0, sizeof(config));

  char *name = "parser1";
  memcpy(config.name, name, strlen(name));
  ov_parser *parser1 = ov_parser_dummy_create(config);

  name = "parser2";
  memcpy(config.name, name, strlen(name));
  ov_parser *parser2 = ov_parser_dummy_create(config);

  name = "parser3";
  memcpy(config.name, name, strlen(name));
  ov_parser *parser3 = ov_parser_dummy_create(config);

  parser1->next = parser2;
  parser2->next = parser3;

  ov_parser_data data;
  memset(&data, 0, sizeof(data));
  void *dummy_data = calloc(100, sizeof(char));

  // check without data input

  testrun(OV_PARSER_ERROR == ov_parser_encode(NULL, NULL));
  testrun(OV_PARSER_ERROR == ov_parser_encode(parser1, NULL));
  testrun(OV_PARSER_ERROR == ov_parser_encode(NULL, &data));

  testrun(OV_PARSER_ERROR == ov_parser_encode(parser1, &data));
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(NULL == data.out.data);
  testrun(NULL == data.out.free);

  // check chained with data
  data.in.data = dummy_data;
  data.in.free = dont_free;
  data.out.data = NULL;
  data.out.free = NULL;
  testrun(OV_PARSER_SUCCESS == ov_parser_encode(parser1, &data));
  testrun(dummy_data == data.in.data);
  testrun(dont_free == data.in.free);
  testrun(dummy_data == data.out.data);
  testrun(dont_free == data.out.free);

  free(dummy_data);
  dummy_data = NULL;

  /*
   *      Dummy parser in chained run with a free function will
   *      fail, as the input will be set at data.in and data.out
   *      THIS IS RELATED TO THE DUMMY IMPLEMENTEATION!
   *      The dummy implementation is a DUMMY for SOME, but not ALL
   *      Test cased.
   */

  data.in.data = dummy_data;
  data.in.free = NULL;
  data.out.data = NULL;
  data.out.free = NULL;
  testrun(OV_PARSER_ERROR == ov_parser_encode(parser1, &data));
  testrun(dummy_data == data.in.data);
  testrun(NULL == data.in.free);
  testrun(dummy_data == data.out.data);
  testrun(NULL == data.out.free);
  ov_buffer_free(dummy_data);

  /*
   *      Check chained run with a dummy buffer copy parse
   *      implementation. The input will be freed, but the
   *      output MUST have the same content.
   *
   */

  char *content = "ABCD";

  data.out.data = NULL;
  data.out.free = NULL;
  data.in.data = ov_buffer_create(0);
  data.in.free = ov_buffer_free;
  testrun(ov_buffer_set(data.in.data, content, strlen(content)));
  parser1->encode = dummy_buffer_copy_parse;
  parser2->encode = dummy_buffer_copy_parse;
  parser3->encode = dummy_buffer_copy_parse;
  testrun(OV_PARSER_SUCCESS == ov_parser_encode(parser1, &data));
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(ov_buffer_free == data.out.free);
  testrun(ov_buffer_cast(data.out.data));
  testrun(0 == strncmp((char *)((ov_buffer *)data.out.data)->start, content,
                       strlen(content)));
  testrun(NULL == data.out.free(data.out.data));

  testrun(NULL == parser_free_chain(parser1));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

int test_ov_parser_decode() {

  ov_parser_config config;
  memset(&config, 0, sizeof(config));

  char *name = "parser1";
  memcpy(config.name, name, strlen(name));
  ov_parser *parser1 = ov_parser_dummy_create(config);

  name = "parser2";
  memcpy(config.name, name, strlen(name));
  ov_parser *parser2 = ov_parser_dummy_create(config);

  name = "parser3";
  memcpy(config.name, name, strlen(name));
  ov_parser *parser3 = ov_parser_dummy_create(config);

  parser1->next = parser2;
  parser2->next = parser3;

  ov_parser_data data;
  memset(&data, 0, sizeof(data));
  void *dummy_data = calloc(100, sizeof(char));

  // check without data input

  testrun(OV_PARSER_ERROR == ov_parser_decode(NULL, NULL));
  testrun(OV_PARSER_ERROR == ov_parser_decode(parser1, NULL));
  testrun(OV_PARSER_ERROR == ov_parser_decode(NULL, &data));

  testrun(OV_PARSER_ERROR == ov_parser_decode(parser1, &data));
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(NULL == data.out.data);
  testrun(NULL == data.out.free);

  // check chained with data
  data.in.data = dummy_data;
  data.in.free = dont_free;
  data.out.data = NULL;
  data.out.free = NULL;
  testrun(OV_PARSER_SUCCESS == ov_parser_decode(parser1, &data));
  testrun(dummy_data == data.in.data);
  testrun(dont_free == data.in.free);
  testrun(dummy_data == data.out.data);
  testrun(dont_free == data.out.free);

  free(dummy_data);
  dummy_data = NULL;

  /*
   *      Dummy parser in chained run with a free function will
   *      fail, as the input will be set at data.in and data.out
   *      THIS IS RELATED TO THE DUMMY IMPLEMENTEATION!
   *      The dummy implementation is a DUMMY for SOME, but not ALL
   *      Test cased.
   */

  data.in.data = dummy_data;
  data.in.free = NULL;
  data.out.data = NULL;
  data.out.free = NULL;
  testrun(OV_PARSER_ERROR == ov_parser_decode(parser1, &data));
  testrun(dummy_data == data.in.data);
  testrun(NULL == data.in.free);
  testrun(dummy_data == data.out.data);
  testrun(NULL == data.out.free);
  ov_buffer_free(dummy_data);

  /*
   *      Check chained run with a dummy buffer copy parse
   *      implementation. The input will be freed, but the
   *      output MUST have the same content.
   *
   */

  char *content = "ABCD";

  data.out.data = NULL;
  data.out.free = NULL;
  data.in.data = ov_buffer_create(0);
  data.in.free = ov_buffer_free;
  testrun(ov_buffer_set(data.in.data, content, strlen(content)));
  parser1->decode = dummy_buffer_copy_parse;
  parser2->decode = dummy_buffer_copy_parse;
  parser3->decode = dummy_buffer_copy_parse;
  testrun(OV_PARSER_SUCCESS == ov_parser_decode(parser1, &data));
  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(ov_buffer_free == data.out.free);
  testrun(ov_buffer_cast(data.out.data));
  testrun(0 == strncmp((char *)((ov_buffer *)data.out.data)->start, content,
                       strlen(content)));
  testrun(NULL == data.out.free(data.out.data));

  testrun(NULL == parser_free_chain(parser1));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_parser_dummy_create() {

  ov_parser_config config;
  memset(&config, 0, sizeof(config));

  ov_parser *parser = ov_parser_dummy_create(config);

  testrun(parser);
  testrun(ov_parser_cast(parser));
  testrun(ov_parser_verify_interface(parser));
  testrun(parser->next == NULL);
  testrun(parser->name != NULL);
  testrun(0 == strncmp(parser->name, "DUMMY", 5));

  // check functions
  testrun(parser->free == impl_dummy_parser_free);
  testrun(parser->decode == impl_dummy_parser_decode);
  testrun(parser->encode == impl_dummy_parser_encode);
  testrun(parser->buffer.is_enabled == impl_dummy_parser_buffer_is_enabled);
  testrun(parser->buffer.has_data == impl_dummy_parser_buffer_has_data);
  testrun(parser->buffer.empty_out == impl_dummy_parser_buffer_empty_out);

  config.buffering = true;
  testrun(!ov_parser_dummy_create(config));

  config.buffering = false;
  strcat(config.name, "next");

  ov_parser *prev = ov_parser_dummy_create(config);
  testrun(ov_parser_cast(prev));
  prev->next = parser;
  testrun(ov_parser_verify_interface(prev));
  testrun(prev->next == parser);
  testrun(prev->name != NULL);
  testrun(0 == strncmp(prev->name, "next", 4));

  // check functions
  testrun(prev->free == impl_dummy_parser_free);
  testrun(prev->decode == impl_dummy_parser_decode);
  testrun(prev->encode == impl_dummy_parser_encode);
  testrun(prev->buffer.is_enabled == impl_dummy_parser_buffer_is_enabled);
  testrun(prev->buffer.has_data == impl_dummy_parser_buffer_has_data);
  testrun(prev->buffer.empty_out == impl_dummy_parser_buffer_empty_out);

  testrun(NULL == parser_free_chain(prev));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_parser_data_clear() {

  ov_parser_data data;
  memset(&data, 0, sizeof(data));

  ov_buffer *buffer1 = ov_buffer_create(100);
  ov_buffer *buffer2 = ov_buffer_create(100);

  testrun(!ov_parser_data_clear(NULL));
  testrun(ov_parser_data_clear(&data));

  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(NULL == data.out.data);
  testrun(NULL == data.out.free);

  data.in.data = buffer1;
  data.out.data = buffer2;
  testrun(ov_parser_data_clear(&data));

  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(NULL == data.out.data);
  testrun(NULL == data.out.free);

  // check buffer still reachable
  testrun(ov_buffer_set(buffer1, "test", 4));
  testrun(ov_buffer_set(buffer2, "test", 4));

  data.in.data = buffer1;
  data.in.free = ov_buffer_free;
  data.out.data = buffer2;
  data.out.free = ov_buffer_free;
  testrun(ov_parser_data_clear(&data));

  testrun(NULL == data.in.data);
  testrun(NULL == data.in.free);
  testrun(NULL == data.out.data);
  testrun(NULL == data.out.free);

  // check valgrind

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_parser_state_to_string() {

  const char *result = ov_parser_state_to_string(OV_PARSER_ERROR);
  testrun(0 == strncmp(result, OV_KEY_ERROR, strlen(OV_KEY_ERROR)));

  result = ov_parser_state_to_string(OV_PARSER_MISMATCH);
  testrun(0 == strncmp(result, OV_KEY_MISMATCH, strlen(OV_KEY_MISMATCH)));

  result = ov_parser_state_to_string(OV_PARSER_PROGRESS);
  testrun(0 == strncmp(result, OV_KEY_PROGRESS, strlen(OV_KEY_PROGRESS)));

  result = ov_parser_state_to_string(OV_PARSER_SUCCESS);
  testrun(0 == strncmp(result, OV_KEY_SUCCESS, strlen(OV_KEY_SUCCESS)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_parser_state_from_string() {

  testrun(OV_PARSER_ERROR ==
          ov_parser_state_from_string(OV_KEY_ERROR, strlen(OV_KEY_ERROR)));

  testrun(OV_PARSER_ERROR ==
          ov_parser_state_from_string(OV_KEY_ERROR, strlen(OV_KEY_ERROR) - 1));

  testrun(OV_PARSER_MISMATCH == ov_parser_state_from_string(
                                    OV_KEY_MISMATCH, strlen(OV_KEY_MISMATCH)));

  testrun(OV_PARSER_ERROR == ov_parser_state_from_string(
                                 OV_KEY_MISMATCH, strlen(OV_KEY_MISMATCH) - 1));

  testrun(OV_PARSER_PROGRESS == ov_parser_state_from_string(
                                    OV_KEY_PROGRESS, strlen(OV_KEY_PROGRESS)));

  testrun(OV_PARSER_ERROR == ov_parser_state_from_string(
                                 OV_KEY_PROGRESS, strlen(OV_KEY_PROGRESS) - 1));

  testrun(OV_PARSER_SUCCESS ==
          ov_parser_state_from_string(OV_KEY_SUCCESS, strlen(OV_KEY_SUCCESS)));

  testrun(OV_PARSER_ERROR == ov_parser_state_from_string(
                                 OV_KEY_SUCCESS, strlen(OV_KEY_SUCCESS) - 1));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/
/*
static bool (*custom_parser_fill_third) (int number, ov_parser_data * const
data){

        if (!data)
                goto error;

        data->in.data = NULL;
        data->in.free = NULL;

        data->out.data = NULL;
        data->out.free = NULL;

        switch (number) {

                case 1:
                        data->in.data = "one";
                        break;

                case 2:
                        data->in.data = "two";
                        break;

                case 3:
                        data->in.data = "three";
                        break;

                default:
                        return false;

        }

        return true;
}
*/
/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

  testrun_init();
  testrun_test(test_ov_parser_cast);
  testrun_test(test_ov_parser_verify_interface);
  testrun_test(test_ov_parser_set_head);
  testrun_test(test_ov_parser_free);
  testrun_test(check_parser_free_chain);
  testrun_test(test_ov_parser_trash_buffered_data);
  testrun_test(test_ov_parser_decode);
  testrun_test(test_ov_parser_encode);

  testrun_test(test_ov_parser_data_clear);

  testrun_test(test_ov_parser_dummy_create);

  testrun_test(test_ov_parser_state_to_string);
  testrun_test(test_ov_parser_state_from_string);

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
