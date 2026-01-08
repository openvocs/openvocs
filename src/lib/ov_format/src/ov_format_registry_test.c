/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "ov_format_registry.c"

#include <ov_test/ov_test.h>
#include <ov_test/ov_test_file.h>

#include <ov_base/ov_buffer.h>

/*----------------------------------------------------------------------------*/

typedef struct {

  uint32_t magic_bytes;
  ov_buffer in;
  size_t index;

  void *options;

} ft_tokenizer_data;

const uint32_t tokenizer_magic_bytes = 0x11ff22ee;

/*----------------------------------------------------------------------------*/

static ov_buffer ft_tokenizer_next_token(ov_format *f, size_t req_bytes,
                                         void *data) {
  UNUSED(req_bytes);

  size_t token_length = 0;
  char *allocated = 0;

  ov_buffer buf = {0};

  OV_ASSERT(0 != data);
  ft_tokenizer_data *ftd = data;

  if (0 == data)
    goto error;

  if (0 == ftd->in.start) {
    ftd->in = ov_format_payload_read_chunk_nocopy(f, 255);
  }

  if (0 == ftd->in.start) {

    OV_ASSERT(0 == ftd->in.length);
    return ftd->in;
  }

  if ((ftd->index >= ftd->in.length) || (0 == ftd->in.start[ftd->index])) {
    goto eof;
  }

  char *next_char = (char *)ftd->in.start + ftd->index;
  char *start = next_char;

  while (ftd->index < ftd->in.length) {

    ++ftd->index;

    if (' ' == *next_char) {

      goto finish;
    }

    ++next_char;
  };

finish:

  token_length = next_char - start + 1;
  allocated = calloc(token_length, sizeof(char));

  memcpy(allocated, start, token_length - 1);

  allocated[token_length - 1] = 0;

  buf = (ov_buffer){
      .start = (uint8_t *)allocated,
      .capacity = token_length,
      .length = token_length,

  };

  return buf;

eof:

  buf = (ov_buffer){0};

error:

  return buf;
};

/*----------------------------------------------------------------------------*/

static void *ft_tokenizer_create_data(ov_format *f, void *options) {

  UNUSED(f);

  ft_tokenizer_data *data = calloc(1, sizeof(ft_tokenizer_data));

  data->magic_bytes = tokenizer_magic_bytes;
  data->options = options;

  return data;
}

/*----------------------------------------------------------------------------*/

static void *ft_tokenizer_free_data(void *data) {

  if (0 != data) {
    free(data);
    data = 0;
  }

  return data;
}

/*----------------------------------------------------------------------------*/

static void *ft_get_tokenizer_options(ov_format *f) {

  ft_tokenizer_data *tdata = ov_format_get_custom_data(f);

  if (0 == tdata)
    return 0;

  return tdata->options;
}
/*----------------------------------------------------------------------------*/

static void *ft_dummy_data_seen = 0;

static ov_buffer ft_dummy_next_chunk(ov_format *f, size_t req_bytes,
                                     void *data) {

  ft_dummy_data_seen = data;
  return ov_format_payload_read_chunk_nocopy(f, req_bytes);
  ;
}

/*----------------------------------------------------------------------------*/

static void *ft_dummy_data_to_set = 0;

static void *ft_dummy_create_data(ov_format *f, void *options) {

  UNUSED(f);
  OV_ASSERT(0 == options);

  return ft_dummy_data_to_set;
}

/*----------------------------------------------------------------------------*/

static void *ft_dummy_data_to_free = 0;

static void *ft_dummy_free_data(void *data) {

  ft_dummy_data_to_free = data;
  return 0;
}

/*----------------------------------------------------------------------------*/

static bool handler_is_in_registry(char const *type,
                                   ov_format_registry *registry) {

  bool result = false;

  if (0 == type)
    return false;

  char const *test_string = "Geir nu garmr mjok fyr gnipahellir - festr man "
                            "slitna en freki renna";

  const size_t test_string_len = strlen(test_string) + 1;

  ov_format *dummy_bare =
      ov_format_from_memory((uint8_t *)test_string, test_string_len, OV_READ);

  if (0 == dummy_bare)
    return false;

  ov_format *type_fmt = ov_format_as(dummy_bare, type, 0, registry);

  if (0 != type_fmt) {

    result = true;
    type_fmt = ov_format_close(type_fmt);
    dummy_bare = 0;
  }

  if (0 != dummy_bare) {

    dummy_bare = ov_format_close(dummy_bare);
  }

  OV_ASSERT(0 == type_fmt);
  OV_ASSERT(0 == dummy_bare);

  return result;
}

/*****************************************************************************
                                     TESTS
 ****************************************************************************/

static int test_ov_format_as() {

  testrun(0 == ov_format_as(0, 0, 0, 0));

  /**************************************************************************
   *                             Prepare test file
   **************************************************************************/

  char const *test_string = "Geir nu garmr mjok fyr gnipahellir - festr man "
                            "slitna en freki renna";

  const size_t test_string_len = strlen(test_string) + 1;

  char file_path[OV_TEST_FILE_TMP_PATH_LEN] = {0};

  int fd = ov_test_file_tmp_write((uint8_t *)test_string, test_string_len,
                                  file_path);

  OV_ASSERT(0 < fd);

  close(fd);
  fd = -1;

  /*------------------------------------------------------------------------*/

  ov_format *infile = ov_format_open(file_path, OV_READ);
  testrun(0 != infile);

  testrun(0 == ov_format_as(0, "tokenizer", 0, 0));

  /* Simple file type - no internal data */

  ov_format_handler tokenizer = {
      .next_chunk = ft_tokenizer_next_token,
      .create_data = ft_tokenizer_create_data,
      .free_data = ft_tokenizer_free_data,
  };

  testrun(ov_format_registry_register_type("tokenizer", tokenizer, 0));

  testrun(0 == ov_format_as(0, "tokenizer", 0, 0));
  testrun(0 == ov_format_as(infile, "unknown_type", 0, 0));

  /* test whether 'options' is passed to create function */
  ov_format *tokenizer_file = ov_format_as(infile, "tokenizer", &tokenizer, 0);
  infile = 0;

  testrun(0 != tokenizer_file);

  testrun(&tokenizer == ft_get_tokenizer_options(tokenizer_file));

  ov_buffer *token = ov_format_payload_read_chunk(tokenizer_file, 1);

  while (0 != token) {

    token = ov_buffer_free(token);

    token = ov_format_payload_read_chunk(tokenizer_file, 1);
  };

  tokenizer_file = ov_format_close(tokenizer_file);

  testrun(0 == tokenizer_file);

  /**************************************************************************
   *                    Check changinc presence of clalbacks
   **************************************************************************/

  ov_format_handler ft_dummy = {0};

  testrun(ov_format_registry_register_type("ftd1", ft_dummy, 0));

  infile = ov_format_open(file_path, OV_READ);
  testrun(0 != infile);

  ov_format *ftd_file = ov_format_as(infile, "ftd1", 0, 0);
  infile = 0;

  testrun(0 != ftd_file);

  testrun(0 == ov_format_payload_read_chunk(ftd_file, 255));

  testrun(0 == ft_dummy_data_seen);
  testrun(0 == ft_dummy_data_to_set);
  testrun(0 == ft_dummy_data_to_free);

  ftd_file = ov_format_close(ftd_file);
  testrun(0 == ftd_file);

  /*-----------------------------------------------------------------------*/

  ft_dummy = (ov_format_handler){

      .next_chunk = ft_dummy_next_chunk,

  };

  testrun(ov_format_registry_register_type("ftd2", ft_dummy, 0));

  infile = ov_format_open(file_path, OV_READ);
  testrun(0 != infile);

  ftd_file = ov_format_as(infile, "ftd2", 0, 0);
  infile = 0;

  testrun(0 != ftd_file);

  ov_buffer *in = ov_format_payload_read_chunk(ftd_file, 255);
  testrun(0 != in);

  testrun(0 == ft_dummy_data_seen);
  testrun(0 == ft_dummy_data_to_free);

  testrun(0 == strcmp(test_string, (char *)in->start));
  testrun(test_string_len == in->length);

  in = ov_buffer_free(in);
  testrun(0 == in);

  ftd_file = ov_format_close(ftd_file);
  testrun(0 == ftd_file);

  testrun(0 == ft_dummy_data_seen);
  testrun(0 == ft_dummy_data_to_free);

  /*-----------------------------------------------------------------------*/

  ft_dummy = (ov_format_handler){

      .next_chunk = ft_dummy_next_chunk,
      .create_data = ft_dummy_create_data,

  };

  char const *dummy_data = "hoenir";

  ft_dummy_data_to_set = (void *)dummy_data;

  /* Should fail because we set create_data but not free_data */
  testrun(!ov_format_registry_register_type("ftd3", ft_dummy, 0));

  /*-----------------------------------------------------------------------*/

  ft_dummy = (ov_format_handler){

      .next_chunk = ft_dummy_next_chunk,
      .create_data = ft_dummy_create_data,
      .free_data = ft_dummy_free_data,

  };

  testrun(ov_format_registry_register_type("ftd3", ft_dummy, 0));

  infile = ov_format_open(file_path, OV_READ);
  testrun(0 != infile);

  ftd_file = ov_format_as(infile, "ftd3", 0, 0);
  infile = 0;

  testrun(0 != ftd_file);

  in = ov_format_payload_read_chunk(ftd_file, 255);
  testrun(0 != in);

  testrun(dummy_data == ft_dummy_data_seen);
  testrun(0 == ft_dummy_data_to_free);

  ft_dummy_data_seen = 0;

  testrun(0 == strcmp(test_string, (char *)in->start));
  testrun(test_string_len == in->length);

  in = ov_buffer_free(in);
  testrun(0 == in);

  ftd_file = ov_format_close(ftd_file);
  testrun(0 == ftd_file);

  testrun(0 == ft_dummy_data_seen);
  testrun(dummy_data == ft_dummy_data_to_free);

  /**************************************************************************
   *                                  Clean up
   **************************************************************************/

  unlink(file_path);
  ov_format_registry_clear(0);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_format_registry_register_type() {

  ov_format_handler ft_test = {0};

  testrun(!handler_is_in_registry("hoenir", 0));

  testrun(!ov_format_registry_register_type(0, ft_test, 0));

  testrun(ov_format_registry_register_type("hoenir", ft_test, 0));

  testrun(handler_is_in_registry("hoenir", 0));

  ft_test = (ov_format_handler){

      .next_chunk = ft_tokenizer_next_token,
      .write_chunk = 0,
      .create_data = ft_tokenizer_create_data,
      .free_data = ft_tokenizer_free_data,

  };

  testrun(ov_format_registry_register_type("huginn", ft_test, 0));

  testrun(handler_is_in_registry("hoenir", 0));
  testrun(handler_is_in_registry("huginn", 0));

  testrun(!ov_format_registry_register_type("huginn", ft_test, 0));

  testrun(handler_is_in_registry("hoenir", 0));
  testrun(handler_is_in_registry("huginn", 0));

  /* Registering same handler under different name works */
  testrun(ov_format_registry_register_type("huginn1", ft_test, 0));

  testrun(handler_is_in_registry("hoenir", 0));
  testrun(handler_is_in_registry("huginn", 0));
  testrun(handler_is_in_registry("huginn1", 0));

  /* registering a too long of a type name does not work */

  char type_too_long[OV_FORMAT_TYPE_MAX_STR_LEN + 2] = {0};
  memset(type_too_long, 'l', sizeof(type_too_long));

  type_too_long[OV_FORMAT_TYPE_MAX_STR_LEN + 1] = 0;

  testrun(!ov_format_registry_register_type(type_too_long, ft_test, 0));

  testrun(handler_is_in_registry("hoenir", 0));
  testrun(handler_is_in_registry("huginn", 0));
  testrun(handler_is_in_registry("huginn1", 0));
  testrun(!handler_is_in_registry(type_too_long, 0));

  ov_format_registry_clear(0);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_format_registry_unregister_type() {

  ov_format_handler ft_test = {0};

  ft_test = (ov_format_handler){

      .next_chunk = ft_tokenizer_next_token,
      .write_chunk = 0,
      .create_data = ft_tokenizer_create_data,
      .free_data = ft_tokenizer_free_data,

  };

  testrun(ov_format_registry_register_type("huginn", ft_test, 0));

  testrun(!ov_format_registry_register_type("huginn", ft_test, 0));

  testrun(handler_is_in_registry("huginn", 0));

  testrun(ov_format_registry_unregister_type("huginn", 0, 0));

  testrun(!handler_is_in_registry("huginn", 0));

  /*                Check return of old handler if there                    */

  testrun(ov_format_registry_register_type("huginn", ft_test, 0));
  ov_format_handler old_handler = {0};

  testrun(ov_format_registry_unregister_type("huginn", &old_handler, 0));

  testrun(0 == ov_format_registry_clear(0));

  testrun(ft_test.next_chunk == old_handler.next_chunk);
  testrun(ft_test.write_chunk == old_handler.write_chunk);
  testrun(ft_test.create_data == old_handler.create_data);
  testrun(ft_test.free_data == old_handler.free_data);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_format_registry_clear() {

  testrun(0 == ov_format_registry_clear(0));

  ov_format_handler ft_test = (ov_format_handler){

      .next_chunk = ft_tokenizer_next_token,
      .write_chunk = 0,
      .create_data = ft_tokenizer_create_data,
      .free_data = ft_tokenizer_free_data,

  };

  /*                           Global registry */

  testrun(ov_format_registry_register_type("huginn", ft_test, 0));

  testrun(handler_is_in_registry("huginn", 0));
  testrun(!handler_is_in_registry("munin", 0));

  testrun(0 == ov_format_registry_clear(0));

  testrun(!handler_is_in_registry("huginn", 0));
  testrun(!handler_is_in_registry("munin", 0));

  testrun(ov_format_registry_register_type("huginn", ft_test, 0));
  testrun(ov_format_registry_register_type("munin", ft_test, 0));

  testrun(handler_is_in_registry("huginn", 0));
  testrun(handler_is_in_registry("munin", 0));

  /*                            Local history */

  ov_format_registry *local = ov_format_registry_create();

  testrun(handler_is_in_registry("huginn", 0));
  testrun(handler_is_in_registry("munin", 0));

  testrun(!handler_is_in_registry("huginn", local));
  testrun(!handler_is_in_registry("munin", local));

  testrun(ov_format_registry_register_type("huginn", ft_test, local));

  testrun(handler_is_in_registry("huginn", 0));
  testrun(handler_is_in_registry("munin", 0));

  testrun(handler_is_in_registry("huginn", local));
  testrun(!handler_is_in_registry("munin", local));

  testrun(ov_format_registry_register_type("munin", ft_test, local));

  testrun(handler_is_in_registry("huginn", 0));
  testrun(handler_is_in_registry("munin", 0));

  testrun(handler_is_in_registry("huginn", local));
  testrun(handler_is_in_registry("munin", local));

  /*               local history uncleared, global cleared */

  testrun(0 == ov_format_registry_clear(0));

  testrun(!handler_is_in_registry("huginn", 0));
  testrun(!handler_is_in_registry("munin", 0));

  testrun(handler_is_in_registry("huginn", local));
  testrun(handler_is_in_registry("munin", local));

  /*               local history cleared, global uncleared */

  testrun(ov_format_registry_register_type("huginn", ft_test, 0));
  testrun(ov_format_registry_register_type("munin", ft_test, 0));

  testrun(handler_is_in_registry("huginn", 0));
  testrun(handler_is_in_registry("munin", 0));

  testrun(handler_is_in_registry("huginn", local));
  testrun(handler_is_in_registry("munin", local));

  local = ov_format_registry_clear(local);
  testrun(0 == local);

  testrun(handler_is_in_registry("huginn", 0));
  testrun(handler_is_in_registry("munin", 0));

  /*                        all registries cleared */

  testrun(0 == ov_format_registry_clear(0));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_format_registry_create() {

  ov_format_registry *registry = ov_format_registry_create();

  testrun(0 != registry);

  /*                        Register test handler */

  ov_format_handler ft_test = (ov_format_handler){

      .next_chunk = ft_tokenizer_next_token,
      .write_chunk = 0,
      .create_data = ft_tokenizer_create_data,
      .free_data = ft_tokenizer_free_data,

  };

  testrun(ov_format_registry_register_type("huginn", ft_test, registry));

  /*                     Dummy format to use as base */

  char const *test_string = "Geir nu garmr mjok fyr gnipahellir - festr man "
                            "slitna en freki renna";

  const size_t test_string_len = strlen(test_string) + 1;

  ov_format *infile =
      ov_format_from_memory((uint8_t *)test_string, test_string_len, OV_READ);
  testrun(0 != infile);

  testrun(0 == ov_format_as(infile, "huginn", 0, 0));
  ov_format *huginn = ov_format_as(infile, "huginn", 0, registry);

  testrun(0 != huginn);

  huginn = ov_format_close(huginn);
  testrun(0 == huginn);

  registry = ov_format_registry_clear(registry);
  testrun(0 == registry);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_format_registry", test_ov_format_as,
            test_ov_format_registry_register_type,
            test_ov_format_registry_unregister_type,
            test_ov_format_registry_clear, test_ov_format_registry_create);
