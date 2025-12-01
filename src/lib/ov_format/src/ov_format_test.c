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

#include <ov_test/ov_test.h>
#include <ov_test/ov_test_file.h>

#include "ov_format.c"

void print_buffer_as_string(FILE *outstream, ov_buffer *buf) {

  if (0 == buf) {

    fprintf(outstream, "Buffer: NULL\n");
    return;
  }

  char *out = calloc(1, buf->length + 1);

  memcpy(out, buf->start, buf->length);
  out[buf->length] = 0;
  fprintf(outstream, "Buffer: %zu: %s\n", buf->length, out);
  free(out);

  out = 0;
};

/*****************************************************************************
                    Dummy format - just passes data through
 ****************************************************************************/

static ov_buffer dummy_next_chunk(ov_format *f, size_t req_bytes, void *data) {
  UNUSED(data);

  return ov_format_payload_read_chunk_nocopy(f, req_bytes);
};

/*----------------------------------------------------------------------------*/

static ssize_t dummy_write_chunk(ov_format *f, ov_buffer const *chunk,
                                 void *data) {

  UNUSED(data);

  return ov_format_payload_write_chunk(f, chunk);
}

/*----------------------------------------------------------------------------*/

static ssize_t dummy_overwrite(ov_format *f, size_t offset,
                               ov_buffer const *chunk, void *data) {

  UNUSED(data);

  return ov_format_payload_overwrite(f, offset, chunk);
}

/*----------------------------------------------------------------------------*/

static ov_format *dummy_ready_format_lower_layer = 0;
static void *dummy_ready_format_data = 0;

static bool dummy_ready_format(ov_format *f, void *data) {

  dummy_ready_format_lower_layer = f;

  dummy_ready_format_data = data;

  return true;
}

/*----------------------------------------------------------------------------*/

static void *dummy_data_to_create = 0;

static void *dummy_create_data() { return dummy_data_to_create; }

/*----------------------------------------------------------------------------*/

static void *dummy_freed_data = 0;

static void *dummy_free_data(void *data) {

  dummy_freed_data = data;

  return 0;
}

/*----------------------------------------------------------------------------*/

static bool
dummy_handler_format_check_reset_internals(ov_format *lower_layer_expected,
                                           void *custom_data_expected) {

  bool ok = dummy_ready_format_lower_layer == lower_layer_expected;
  ok = ok && (dummy_ready_format_data == custom_data_expected);
  ok = ok && (dummy_freed_data = custom_data_expected);

  dummy_ready_format_lower_layer = 0;
  dummy_ready_format_data = 0;
  dummy_freed_data = 0;

  return ok;
}

/*----------------------------------------------------------------------------*/

static ov_format_handler dummy_handler = {

    .next_chunk = dummy_next_chunk,
    .write_chunk = dummy_write_chunk,
    .overwrite = dummy_overwrite,
    .ready_format = dummy_ready_format,
    .create_data = dummy_create_data,
    .free_data = dummy_free_data,

};

/*****************************************************************************
                    Dummy 2 format - just passes data through 2nd tier
 ****************************************************************************/

static ov_buffer dummy_2_next_chunk(ov_format *f, size_t req_bytes,
                                    void *data) {
  UNUSED(data);

  return ov_format_payload_read_chunk_nocopy(f, req_bytes);
};

/*----------------------------------------------------------------------------*/

static ssize_t dummy_2_write_chunk(ov_format *f, ov_buffer const *chunk,
                                   void *data) {

  UNUSED(data);

  return ov_format_payload_write_chunk(f, chunk);
}

/*----------------------------------------------------------------------------*/

static ssize_t dummy_2_overwrite(ov_format *f, size_t offset,
                                 ov_buffer const *chunk, void *data) {

  UNUSED(data);

  return ov_format_payload_overwrite(f, offset, chunk);
}

/*----------------------------------------------------------------------------*/

static ov_format *dummy_2_ready_format_lower_layer = 0;
static void *dummy_2_ready_format_data = 0;

static bool dummy_2_ready_format(ov_format *f, void *data) {

  dummy_2_ready_format_lower_layer = f;

  dummy_2_ready_format_data = data;

  return true;
}

/*----------------------------------------------------------------------------*/

static void *dummy_2_data_to_create = 0;

static void *dummy_2_create_data() { return dummy_2_data_to_create; }

/*----------------------------------------------------------------------------*/

static void *dummy_2_freed_data = 0;

static void *dummy_2_free_data(void *data) {

  dummy_2_freed_data = data;

  return 0;
}

/*----------------------------------------------------------------------------*/

static bool
dummy_2_handler_format_check_reset_internals(ov_format *lower_layer_expected,
                                             void *custom_data_expected) {

  bool ok = dummy_2_ready_format_lower_layer == lower_layer_expected;
  ok = ok && (dummy_2_ready_format_data == custom_data_expected);
  ok = ok && (dummy_2_freed_data = custom_data_expected);

  dummy_2_ready_format_lower_layer = 0;
  dummy_2_ready_format_data = 0;
  dummy_2_freed_data = 0;

  return ok;
}

/*----------------------------------------------------------------------------*/

static ov_format_handler dummy_2_handler = {

    .next_chunk = dummy_2_next_chunk,
    .write_chunk = dummy_2_write_chunk,
    .overwrite = dummy_2_overwrite,
    .ready_format = dummy_2_ready_format,
    .create_data = dummy_2_create_data,
    .free_data = dummy_2_free_data,

};

/*****************************************************************************
                                     TESTS
 ****************************************************************************/

static int test_ov_format_open() {

  testrun(0 == ov_format_open(0, OV_READ));
  testrun(0 == ov_format_open("./ov_format_test.we_do_not_exist", OV_READ));

  /**************************************************************************
   *                                    READ
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

  ov_format *f = ov_format_open(file_path, OV_READ);

  testrun(0 != f);
  testrun(FILE_TYPE == f->type);

  ov_buffer *read = ov_format_payload_read_chunk(f, test_string_len);
  testrun(0 != read);

  f = ov_format_close(f);
  testrun(0 == f);

  testrun(0 == strcmp(test_string, (char *)read->start));

  read = ov_buffer_free(read);

  unlink(file_path);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_from_memory() {

  uint8_t test_data[] = {1,   2,   3,   4,   5,   6,   7,   8,  9,
                         'h', 'e', 'i', 'm', 'd', 'a', 'l', 'l'};

  const size_t test_data_len = sizeof(test_data);

  testrun(0 == ov_format_from_memory(0, 0, OV_READ));
  testrun(0 == ov_format_from_memory(test_data, 0, OV_READ));
  testrun(0 == ov_format_from_memory(0, test_data_len, OV_READ));

  ov_format *fmt = ov_format_from_memory(test_data, test_data_len, OV_READ);
  testrun(0 != fmt);

  uint8_t *index_ptr = test_data;
  size_t bytes_received = 0;

  ov_buffer buffer = ov_format_payload_read_chunk_nocopy(fmt, 5);
  testrun(0 != buffer.start);

  while (0 != buffer.start) {

    testrun(0 != buffer.length);

    testrun(0 == memcmp(buffer.start, index_ptr, buffer.length));

    index_ptr += buffer.length;
    bytes_received += buffer.length;

    buffer = ov_format_payload_read_chunk_nocopy(fmt, 5);
  }

  testrun(test_data_len == bytes_received);

  fmt = ov_format_close(fmt);

  /*************************************************************************
                              WRITE TO MEMORY
   ************************************************************************/

  uint8_t out_buffer[sizeof(test_data) - 1] = {0};
  const size_t out_length = sizeof(out_buffer);

  _Static_assert(sizeof(out_buffer) + 1 == sizeof(test_data),
                 "out_buffer ought to be smaller than test_data");

  testrun(0 == ov_format_from_memory(0, 0, OV_WRITE));
  testrun(0 == ov_format_from_memory(out_buffer, 0, OV_WRITE));

  /*                          Auto-extend mode                              */

  fmt = ov_format_from_memory(0, 2, OV_WRITE);

  testrun(0 != fmt);

  ov_buffer test_buf = {
      .start = test_data,
      .length = 1,
  };

  size_t remaining_test_data_bytes = test_data_len;

  /* Write 1 byte */

  testrun(1 == ov_format_payload_write_chunk(fmt, &test_buf));

  ov_buffer target_buf = ov_format_get_memory(fmt);

  testrun(0 != target_buf.start);
  testrun(1 == target_buf.length);

  testrun(0 == memcmp(test_data, target_buf.start, target_buf.length));

  --remaining_test_data_bytes;

  test_buf.start += 1;

  /* Write 0 - length */

  test_buf.length = 0;

  testrun(0 == ov_format_payload_write_chunk(fmt, &test_buf));

  target_buf = ov_format_get_memory(fmt);

  testrun(0 != target_buf.start);
  testrun(1 == target_buf.length);

  testrun(0 == memcmp(test_data, target_buf.start, target_buf.length));

  /* Write 4 - byte chunk */

  test_buf.length = 4;

  testrun(4 == ov_format_payload_write_chunk(fmt, &test_buf));

  target_buf = ov_format_get_memory(fmt);

  testrun(0 != target_buf.start);
  testrun(1 + 4 == target_buf.length);

  testrun(0 == memcmp(test_data, target_buf.start, target_buf.length));

  remaining_test_data_bytes -= 4;

  /* Write remainder of test data */

  test_buf.start += 4;
  test_buf.length = remaining_test_data_bytes;

  testrun((ssize_t)remaining_test_data_bytes ==
          ov_format_payload_write_chunk(fmt, &test_buf));

  target_buf = ov_format_get_memory(fmt);

  testrun(0 != target_buf.start);
  testrun(1 + 4 + remaining_test_data_bytes == target_buf.length);
  testrun(sizeof(test_data) == target_buf.length);

  testrun(0 == memcmp(test_data, target_buf.start, target_buf.length));

  fmt = ov_format_close(fmt);
  testrun(0 == fmt);

  target_buf = (ov_buffer){0};

  /*                   Auto-extend - stacked formats                        */

  fmt = ov_format_from_memory(0, 1, OV_WRITE);

  testrun(0 != fmt);

  int test_int = 1;
  dummy_data_to_create = &test_int;

  ov_format *stacked_tier_1 = ov_format_wrap(fmt, "dummy1", &dummy_handler, 0);
  testrun(0 != stacked_tier_1);

  fmt = 0;

  ov_format *stacked_tier_2 =
      ov_format_wrap(stacked_tier_1, "dummy2", &dummy_handler, 0);
  testrun(0 != stacked_tier_2);

  stacked_tier_1 = 0;

  test_buf = (ov_buffer){
      .start = test_data,
      .length = 1,
  };

  remaining_test_data_bytes = test_data_len;

  /* Write 1 byte */

  testrun(1 == ov_format_payload_write_chunk(stacked_tier_2, &test_buf));

  target_buf = ov_format_get_memory(stacked_tier_2);

  testrun(0 != target_buf.start);
  testrun(1 == target_buf.length);

  testrun(0 == memcmp(test_data, target_buf.start, target_buf.length));

  --remaining_test_data_bytes;

  test_buf.start += 1;

  /* Write 0 - length */

  test_buf.length = 0;

  testrun(0 == ov_format_payload_write_chunk(stacked_tier_2, &test_buf));

  target_buf = ov_format_get_memory(stacked_tier_2);

  testrun(0 != target_buf.start);
  testrun(1 == target_buf.length);

  testrun(0 == memcmp(test_data, target_buf.start, target_buf.length));

  /* Write 4 - byte chunk */

  test_buf.length = 4;

  testrun(4 == ov_format_payload_write_chunk(stacked_tier_2, &test_buf));

  target_buf = ov_format_get_memory(stacked_tier_2);

  testrun(0 != target_buf.start);
  testrun(1 + 4 == target_buf.length);

  testrun(0 == memcmp(test_data, target_buf.start, target_buf.length));

  remaining_test_data_bytes -= 4;

  /* Write remainder of test data */

  test_buf.start += 4;
  test_buf.length = remaining_test_data_bytes;

  testrun((ssize_t)remaining_test_data_bytes ==
          ov_format_payload_write_chunk(stacked_tier_2, &test_buf));

  target_buf = ov_format_get_memory(stacked_tier_2);

  testrun(0 != target_buf.start);
  testrun(1 + 4 + remaining_test_data_bytes == target_buf.length);

  testrun(0 == memcmp(test_data, target_buf.start, target_buf.length));

  stacked_tier_2 = ov_format_close(stacked_tier_2);
  testrun(0 == stacked_tier_2);

  target_buf = (ov_buffer){0};

  /*                          Fixed - size mode                             */

  fmt = ov_format_from_memory(out_buffer, out_length, OV_WRITE);

  remaining_test_data_bytes = test_data_len;

  testrun(0 != fmt);

  test_buf = (ov_buffer){
      .start = test_data,
      .length = 1,
  };

  /* Write 1 byte */

  testrun(1 == ov_format_payload_write_chunk(fmt, &test_buf));

  target_buf = ov_format_get_memory(fmt);

  testrun(0 != target_buf.start);
  testrun(1 == target_buf.length);

  testrun(0 == memcmp(test_data, target_buf.start, target_buf.length));

  --remaining_test_data_bytes;

  test_buf.start += 1;

  /* Write 0 - length */

  test_buf.length = 0;

  testrun(0 == ov_format_payload_write_chunk(fmt, &test_buf));

  target_buf = ov_format_get_memory(fmt);

  testrun(0 != target_buf.start);
  testrun(1 == target_buf.length);

  testrun(0 == memcmp(test_data, target_buf.start, target_buf.length));

  /* Write 4 - byte chunk */

  test_buf.length = 4;

  testrun(4 == ov_format_payload_write_chunk(fmt, &test_buf));

  target_buf = ov_format_get_memory(fmt);

  testrun(0 != target_buf.start);
  testrun(1 + 4 == target_buf.length);

  testrun(0 == memcmp(test_data, target_buf.start, target_buf.length));

  remaining_test_data_bytes -= 4;

  /* Write remainder of test data */

  test_buf.start += 4;
  test_buf.length = remaining_test_data_bytes;

  ssize_t bytes_written = ov_format_payload_write_chunk(fmt, &test_buf);

  testrun(bytes_written == (ssize_t)remaining_test_data_bytes - 1);

  target_buf = ov_format_get_memory(fmt);

  testrun(0 != target_buf.start);
  testrun(1 + 4 + (size_t)bytes_written == target_buf.length);

  testrun(0 == memcmp(test_data, target_buf.start, target_buf.length));

  /*     Try to write more data than there is space left in format          */

  /* Should fail */
  testrun(0 == ov_format_payload_write_chunk(fmt, &test_buf));

  /* memory should not have changed */
  target_buf = ov_format_get_memory(fmt);

  testrun(0 != target_buf.start);
  testrun(1 + 4 + (size_t)bytes_written == target_buf.length);

  testrun(0 == memcmp(test_data, target_buf.start, target_buf.length));

  fmt = ov_format_close(fmt);
  testrun(0 == fmt);

  target_buf = (ov_buffer){0};

  /*************************************************************************
                              Stacked formats
   ************************************************************************/

  fmt = ov_format_from_memory(out_buffer, out_length, OV_WRITE);

  testrun(0 != fmt);

  stacked_tier_1 = ov_format_wrap(fmt, "dummy1", &dummy_handler, 0);
  testrun(0 != stacked_tier_1);

  fmt = 0;

  stacked_tier_2 = ov_format_wrap(stacked_tier_1, "dummy2", &dummy_handler, 0);
  testrun(0 != stacked_tier_2);

  stacked_tier_1 = 0;

  test_buf = (ov_buffer){
      .start = test_data,
      .length = 1,
  };

  remaining_test_data_bytes = test_data_len;

  /* Write 1 byte */

  testrun(1 == ov_format_payload_write_chunk(stacked_tier_2, &test_buf));

  target_buf = ov_format_get_memory(stacked_tier_2);

  testrun(0 != target_buf.start);
  testrun(1 == target_buf.length);

  testrun(0 == memcmp(test_data, target_buf.start, target_buf.length));

  --remaining_test_data_bytes;

  test_buf.start += 1;

  /* Write 0 - length */

  test_buf.length = 0;

  testrun(0 == ov_format_payload_write_chunk(stacked_tier_2, &test_buf));

  target_buf = ov_format_get_memory(stacked_tier_2);

  testrun(0 != target_buf.start);
  testrun(1 == target_buf.length);

  testrun(0 == memcmp(test_data, target_buf.start, target_buf.length));

  /* Write 4 - byte chunk */

  test_buf.length = 4;

  testrun(4 == ov_format_payload_write_chunk(stacked_tier_2, &test_buf));

  target_buf = ov_format_get_memory(stacked_tier_2);

  testrun(0 != target_buf.start);
  testrun(1 + 4 == target_buf.length);

  testrun(0 == memcmp(test_data, target_buf.start, target_buf.length));

  remaining_test_data_bytes -= 4;

  /* Write remainder of test data */

  test_buf.start += 4;
  test_buf.length = remaining_test_data_bytes;

  bytes_written = ov_format_payload_write_chunk(stacked_tier_2, &test_buf);

  testrun(bytes_written == (ssize_t)remaining_test_data_bytes - 1);

  target_buf = ov_format_get_memory(stacked_tier_2);

  testrun(0 != target_buf.start);
  testrun(1 + 4 + (size_t)bytes_written == target_buf.length);

  testrun(0 == memcmp(test_data, target_buf.start, target_buf.length));

  /*     Try to write more data than there is space left in format          */

  /* Should fail */
  testrun(0 == ov_format_payload_write_chunk(stacked_tier_2, &test_buf));

  /* memory should not have changed */
  target_buf = ov_format_get_memory(stacked_tier_2);

  testrun(0 != target_buf.start);
  testrun(1 + 4 + (size_t)bytes_written == target_buf.length);

  testrun(0 == memcmp(test_data, target_buf.start, target_buf.length));

  stacked_tier_2 = ov_format_close(stacked_tier_2);
  testrun(0 == stacked_tier_2);

  target_buf = (ov_buffer){0};

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_get_memory() {

  ov_buffer mem = ov_format_get_memory(0);

  testrun(0 == mem.start);
  testrun(0 == mem.length);

  char ref_data[] = "gleipnir";
  size_t ref_len = sizeof(ref_data);

  /*               READ formats should not return memory                    */

  ov_format *fmt = ov_format_from_memory((uint8_t *)ref_data, ref_len, OV_READ);

  testrun(0 != fmt);

  mem = ov_format_get_memory(fmt);

  testrun(0 == mem.start);
  testrun(0 == mem.length);

  fmt = ov_format_close(fmt);
  testrun(0 == fmt);

  /*         WRITE should return its internal buffer as written             */

  fmt = ov_format_from_memory(0, 1, OV_WRITE);

  testrun(0 != fmt);

  int test_int = 1;
  dummy_data_to_create = &test_int;

  mem = ov_format_get_memory(fmt);

  /* No content yet */

  testrun(0 != mem.start);
  testrun(0 == mem.length);

  ov_buffer write_buf = {

      .start = (uint8_t *)ref_data,
      .length = 3,

  };

  testrun((ssize_t)write_buf.length ==
          ov_format_payload_write_chunk(fmt, &write_buf));

  mem = ov_format_get_memory(fmt);

  testrun(0 != mem.start);
  testrun(write_buf.length == mem.length);

  testrun(0 == memcmp(mem.start, write_buf.start, write_buf.length));

  /* Write remainder - entire buffer should be returned */

  write_buf = (ov_buffer){

      .start = (uint8_t *)ref_data + 3,
      .length = ref_len - 3,

  };

  testrun((ssize_t)write_buf.length ==
          ov_format_payload_write_chunk(fmt, &write_buf));

  mem = ov_format_get_memory(fmt);

  testrun(0 != mem.start);
  testrun(ref_len == mem.length);

  testrun(0 == memcmp(mem.start, ref_data, ref_len));

  fmt = ov_format_close(fmt);
  testrun(0 == fmt);

  /*************************************************************************
                              Stacked formats
   ************************************************************************/

  fmt = ov_format_from_memory(0, 1, OV_WRITE);

  testrun(0 != fmt);

  ov_format *stacked_tier_1 = ov_format_wrap(fmt, "dummy1", &dummy_handler, 0);
  testrun(0 != stacked_tier_1);

  dummy_data_to_create = &test_int;

  /* Check whether callbacks are called ... */

  mem = ov_format_get_memory(stacked_tier_1);

  /* No content yet */

  testrun(0 != mem.start);
  testrun(0 == mem.length);

  testrun(dummy_handler_format_check_reset_internals(fmt, &test_int));

  int test_int_2 = 2;
  dummy_2_data_to_create = &test_int_2;

  ov_format *stacked_tier_2 =
      ov_format_wrap(stacked_tier_1, "dummy2", &dummy_2_handler, 0);
  testrun(0 != stacked_tier_2);

  mem = ov_format_get_memory(stacked_tier_2);

  /* No content yet */

  testrun(0 != mem.start);
  testrun(0 == mem.length);

  testrun(dummy_2_handler_format_check_reset_internals(stacked_tier_1,
                                                       &test_int_2));
  testrun(dummy_handler_format_check_reset_internals(fmt, &test_int));

  write_buf = (ov_buffer){

      .start = (uint8_t *)ref_data,
      .length = 3,

  };

  testrun((ssize_t)write_buf.length ==
          ov_format_payload_write_chunk(stacked_tier_2, &write_buf));

  mem = ov_format_get_memory(stacked_tier_2);

  testrun(0 != mem.start);
  testrun(write_buf.length == mem.length);

  testrun(0 == memcmp(mem.start, write_buf.start, write_buf.length));

  testrun(dummy_2_handler_format_check_reset_internals(stacked_tier_1,
                                                       &test_int_2));
  testrun(dummy_handler_format_check_reset_internals(fmt, &test_int));

  /* Write remainder - entire buffer should be returned */

  write_buf = (ov_buffer){

      .start = (uint8_t *)ref_data + 3,
      .length = ref_len - 3,

  };

  testrun((ssize_t)write_buf.length ==
          ov_format_payload_write_chunk(stacked_tier_2, &write_buf));

  mem = ov_format_get_memory(stacked_tier_2);

  testrun(0 != mem.start);
  testrun(ref_len == mem.length);

  testrun(0 == memcmp(mem.start, ref_data, ref_len));

  testrun(dummy_2_handler_format_check_reset_internals(stacked_tier_1,
                                                       &test_int_2));
  testrun(dummy_handler_format_check_reset_internals(fmt, &test_int));

  stacked_tier_1 = 0;
  fmt = 0;

  stacked_tier_2 = ov_format_close(stacked_tier_2);
  testrun(0 == stacked_tier_2);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_close() {

  testrun(0 == ov_format_close(0));

  /* Check that
   * 1. ready_format is called
   * 2. free_data is called
   */

  ov_format *mem = ov_format_from_memory(0, 1, OV_WRITE);
  testrun(0 != mem);

  int test_int = 1;

  dummy_data_to_create = &test_int;

  ov_format *check_called_fmt = ov_format_wrap(mem, "dummy", &dummy_handler, 0);

  testrun(0 != check_called_fmt);

  dummy_ready_format_lower_layer = 0;
  dummy_ready_format_data = 0;
  dummy_freed_data = 0;

  check_called_fmt = ov_format_close(check_called_fmt);

  testrun(0 == check_called_fmt);

  testrun(mem == dummy_ready_format_lower_layer);
  testrun(&test_int == dummy_ready_format_data);
  testrun(&test_int == dummy_freed_data);

  return testrun_log_success();
}

/*****************************************************************************
                            ov_format_has_more_data
 ****************************************************************************/

/*
 * Does not return any data every second time ov_format_payload_read_chunk* is
 * called to mimik faulty chunks
 */

typedef struct {

  uint32_t magic_bytes;
  bool faulty;

} interrupting_data;

const uint32_t interrupting_magic_bytes = 0x11ff22ee;

static bool is_interrupting_data(void *data) {

  if (0 == data)
    return false;

  interrupting_data *tdata = data;

  return interrupting_magic_bytes == tdata->magic_bytes;
}

/*----------------------------------------------------------------------------*/

static ov_buffer interrupting_next_chunk(ov_format *f, size_t req_bytes,
                                         void *data) {

  ov_buffer buf = {0};

  OV_ASSERT(0 != data);
  interrupting_data *idata = data;

  if (!is_interrupting_data(idata))
    goto error;

  if (idata->faulty) {
    idata->faulty = false;
    goto error;
  }

  idata->faulty = true;

  return ov_format_payload_read_chunk_nocopy(f, req_bytes);

error:

  return buf;
};

/*----------------------------------------------------------------------------*/

static void *interrupting_create_data(ov_format *f, void *options) {

  UNUSED(f);
  UNUSED(options);

  interrupting_data *data = calloc(1, sizeof(interrupting_data));

  data->magic_bytes = interrupting_magic_bytes;
  data->faulty = false;

  return data;
}

/*----------------------------------------------------------------------------*/

static void *interrupting_free_data(void *data) {

  if (0 != data) {
    free(data);
    data = 0;
  }

  return data;
}

static int test_ov_format_has_more_data() {

  testrun(!ov_format_has_more_data(0));

  char const *test_string = "Geir nu garmr mjok fyr gnipahellir - festr man "
                            "slitna en freki renna";

  const size_t test_string_len = strlen(test_string) + 1;

  /*------------------------------------------------------------------------*/

  ov_format *mem_fmt =
      ov_format_from_memory((uint8_t *)test_string, test_string_len, OV_READ);
  testrun(0 != mem_fmt);

  ov_format_handler interrupting_handler = {

      .next_chunk = interrupting_next_chunk,
      .create_data = interrupting_create_data,
      .free_data = interrupting_free_data,
  };

  ov_format *interrupting_format =
      ov_format_wrap(mem_fmt, "interrupting", &interrupting_handler, 0);

  size_t num_bytes_read = 0;
  bool some_reads_failed = false;

  while (ov_format_has_more_data(interrupting_format)) {

    ov_buffer buf = ov_format_payload_read_chunk_nocopy(interrupting_format, 1);

    if (0 == buf.start) {

      some_reads_failed = true;
      testrun_log("Read failed (Expected)");
      continue;
    }

    num_bytes_read += buf.length;
  }

  testrun(some_reads_failed);
  testrun(test_string_len == num_bytes_read);

  interrupting_format = ov_format_close(interrupting_format);

  testrun(0 == interrupting_format);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_payload_read_chunk() {

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

  /*************************************************************************
                        Read in everything available
   ************************************************************************/

  ov_format *f = ov_format_open(file_path, OV_READ);

  testrun(0 != f);
  testrun(FILE_TYPE == f->type);

  ov_buffer *read = ov_format_payload_read_chunk(f, 0);
  testrun(0 != read);

  f = ov_format_close(f);
  testrun(0 == f);

  testrun(0 == strcmp(test_string, (char *)read->start));

  read = ov_buffer_free(read);

  /**************************************************************************
   *                             Read in one chunk
   **************************************************************************/

  f = ov_format_open(file_path, OV_READ);

  testrun(0 != f);
  testrun(FILE_TYPE == f->type);

  read = ov_format_payload_read_chunk(f, test_string_len);
  testrun(0 != read);

  f = ov_format_close(f);
  testrun(0 == f);

  testrun(0 == strcmp(test_string, (char *)read->start));

  read = ov_buffer_free(read);

  /**************************************************************************
   *                           Read in several chunks
   **************************************************************************/

  f = ov_format_open(file_path, OV_READ);

  testrun(0 != f);

  char const *compare_ptr = test_string;

  do {

    read = ov_format_payload_read_chunk(f, 5);

    if (0 == read)
      break;

    print_buffer_as_string(stderr, read);

    testrun(0 == memcmp(compare_ptr, read->start, read->length));

    compare_ptr += read->length;

    read = ov_buffer_free(read);

  } while (true);

  testrun(test_string + test_string_len == compare_ptr);

  f = ov_format_close(f);
  testrun(0 == f);

  unlink(file_path);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_payload_read_chunk_nocopy() {

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

  /**************************************************************************
   *                             Read in one chunk
   **************************************************************************/

  ov_format *f = ov_format_open(file_path, OV_READ);

  testrun(0 != f);
  testrun(FILE_TYPE == f->type);

  ov_buffer read = ov_format_payload_read_chunk_nocopy(f, test_string_len);

  testrun((0 != read.length) && (0 != read.start));

  testrun(0 == strcmp(test_string, (char *)read.start));

  f = ov_format_close(f);
  testrun(0 == f);

  read.length = 0;
  read.start = 0;

  /**************************************************************************
   *                           Read in several chunks
   **************************************************************************/

  f = ov_format_open(file_path, OV_READ);

  testrun(0 != f);

  char const *compare_ptr = test_string;

  do {

    read = ov_format_payload_read_chunk_nocopy(f, 5);

    if (0 == read.length)
      break;

    print_buffer_as_string(stderr, &read);

    testrun(0 == memcmp(compare_ptr, read.start, read.length));

    compare_ptr += read.length;

    read.start = 0;
    read.length = 0;

  } while (true);

  testrun(test_string + test_string_len == compare_ptr);

  f = ov_format_close(f);
  testrun(0 == f);

  unlink(file_path);

  return testrun_log_success();
}

/*****************************************************************************
                                  Format types
 ****************************************************************************/

typedef struct {

  uint32_t magic_bytes;
  ov_buffer in;
  size_t index;

  void *options;

} ft_tokenizer_data;

const uint32_t tokenizer_magic_bytes = 0x11ff22ee;

static bool is_tokenizer_data(void *data) {

  if (0 == data)
    return false;

  ft_tokenizer_data *tdata = data;

  return tokenizer_magic_bytes == tdata->magic_bytes;
}

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

/******************************************************************************
 *                             Dummy - file type
 ******************************************************************************/

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

static int test_ov_format_payload_write_chunk() {

  char const *TEST_WRITE_FILE = "/tmp/"
                                "test_ov_format_payload_write_chunk.data";

  char const *TEST_DATA_STR = "Der Tod ist besser als ein bitteres Leben und "
                              "ewige Ruhe besser als stete Krankheit.";

  const size_t TEST_DATA_LENGTH = sizeof(TEST_DATA_STR);

  uint8_t *TEST_DATA = (uint8_t *)TEST_DATA_STR;

  testrun(0 > ov_format_payload_write_chunk(0, 0));

  ov_format *file_fmt = ov_format_open(TEST_WRITE_FILE, OV_WRITE);

  testrun(0 != file_fmt);

  testrun(0 > ov_format_payload_write_chunk(file_fmt, 0));

  const ov_buffer testdata = {

      .start = TEST_DATA,
      .length = TEST_DATA_LENGTH,

  };

  testrun((ssize_t)TEST_DATA_LENGTH ==
          ov_format_payload_write_chunk(file_fmt, &testdata));

  file_fmt = ov_format_close(file_fmt);
  testrun(0 == file_fmt);

  file_fmt = ov_format_open(TEST_WRITE_FILE, OV_READ);
  testrun(0 != file_fmt);

  /* Writing to a read-only file should not be possible */
  testrun(0 > ov_format_payload_write_chunk(file_fmt, &testdata));

  ov_buffer recv = ov_format_payload_read_chunk_nocopy(file_fmt, 0);

  testrun(0 != recv.start);
  testrun(TEST_DATA_LENGTH == recv.length);
  testrun(0 == memcmp(TEST_DATA, recv.start, TEST_DATA_LENGTH));

  file_fmt = ov_format_close(file_fmt);
  testrun(0 == file_fmt);

  unlink(TEST_WRITE_FILE);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_payload_overwrite() {

  testrun(-1 == ov_format_payload_overwrite(0, 0, 0));

  ov_format *mem_fmt = ov_format_from_memory(0, 1, OV_WRITE);
  testrun(0 != mem_fmt);

  testrun(-1 == ov_format_payload_overwrite(mem_fmt, 0, 0));

  const ov_buffer ref_data = {
      .start = (uint8_t *)"abcdefghi",
      .length = 9,
  };

  testrun(-1 == ov_format_payload_overwrite(0, 3, 0));

  testrun(-1 == ov_format_payload_overwrite(mem_fmt, 3, 0));

  testrun(-1 == ov_format_payload_overwrite(0, 0, &ref_data));

  /* Out-of-bounds */
  testrun(-1 == ov_format_payload_overwrite(mem_fmt, 0, &ref_data));

  testrun(ref_data.length ==
          (size_t)ov_format_payload_write_chunk(mem_fmt, &ref_data));

  ov_buffer mem_buffer = ov_format_get_memory(mem_fmt);
  testrun(0 != mem_buffer.start);
  testrun(mem_buffer.length == ref_data.length);
  testrun(0 == memcmp(mem_buffer.start, ref_data.start, ref_data.length));

  testrun(ref_data.length ==
          (size_t)ov_format_payload_overwrite(mem_fmt, 0, &ref_data));

  mem_buffer = ov_format_get_memory(mem_fmt);
  testrun(0 != mem_buffer.start);
  testrun(mem_buffer.length == ref_data.length);
  testrun(0 == memcmp(mem_buffer.start, ref_data.start, ref_data.length));

  /* Overwrite some existing bytes */
  ov_buffer overwrite_data = {
      .start = (uint8_t *)"zyx",
      .length = 3,
  };

  testrun(overwrite_data.length ==
          (size_t)ov_format_payload_overwrite(mem_fmt, 0, &overwrite_data));

  /* Totally 'in-bounds' */
  mem_buffer = ov_format_get_memory(mem_fmt);
  testrun(0 != mem_buffer.start);
  testrun(mem_buffer.length == ref_data.length);
  testrun(0 == memcmp(mem_buffer.start, "zyxdefghi", ref_data.length));

  /* Totally 'in-bounds' - somewhere in the middle */
  testrun(3 ==
          (size_t)ov_format_payload_overwrite(mem_fmt, 4, &overwrite_data));

  mem_buffer = ov_format_get_memory(mem_fmt);
  testrun(0 != mem_buffer.start);
  testrun(mem_buffer.length == ref_data.length);
  testrun(0 == memcmp(mem_buffer.start, "zyxdzyxhi", ref_data.length));

  /* Partially 'in-bounds' - failure */
  testrun(-1 == ov_format_payload_overwrite(mem_fmt, 8, &overwrite_data));

  mem_buffer = ov_format_get_memory(mem_fmt);
  testrun(0 != mem_buffer.start);
  testrun(mem_buffer.length == ref_data.length);
  testrun(0 == memcmp(mem_buffer.start, "zyxdzyxhi", ref_data.length));

  /* Totally out-of-bounds -> failure */
  testrun(-1 == ov_format_payload_overwrite(mem_fmt, ref_data.length,
                                            &overwrite_data));

  mem_buffer = ov_format_get_memory(mem_fmt);
  testrun(0 != mem_buffer.start);
  testrun(mem_buffer.length == ref_data.length);
  testrun(0 == memcmp(mem_buffer.start, "zyxdzyxhi", ref_data.length));

  /* Check that we did not mess up just appending */

  testrun(3 == ov_format_payload_write_chunk(mem_fmt, &overwrite_data));

  mem_buffer = ov_format_get_memory(mem_fmt);
  testrun(0 != mem_buffer.start);
  testrun(mem_buffer.length == ref_data.length + 3);
  testrun(0 == memcmp(mem_buffer.start, "zyxdzyxhi", ref_data.length));
  testrun(0 == memcmp(mem_buffer.start + ref_data.length, "zyx", 3));

  mem_fmt = ov_format_close(mem_fmt);
  testrun(0 == mem_fmt);

  /*************************************************************************
                                File format
   ************************************************************************/

  /* Same checks as above, but with file format */

  char file_path[] = "/tmp/ov_format_test.XXXXXX";

  int tmp_fd = mkstemp(file_path);

  testrun(0 < tmp_fd);
  close(tmp_fd);

  tmp_fd = -1;

  ov_format *file_fmt = ov_format_open(file_path, OV_WRITE);
  testrun(0 != file_fmt);

  testrun(-1 == ov_format_payload_overwrite(file_fmt, 0, 0));
  testrun(-1 == ov_format_payload_overwrite(file_fmt, 3, 0));

  /* Out-of-bounds */
  testrun(-1 == ov_format_payload_overwrite(file_fmt, 0, &ref_data));

  testrun(ref_data.length ==
          (size_t)ov_format_payload_write_chunk(file_fmt, &ref_data));

  testrun(ref_data.length ==
          (size_t)ov_format_payload_overwrite(file_fmt, 0, &ref_data));

  /* Overwrite some existing bytes */
  overwrite_data = (ov_buffer){
      .start = (uint8_t *)"zyx",
      .length = 3,
  };

  /* Totally 'in-bounds' */

  testrun(overwrite_data.length ==
          (size_t)ov_format_payload_overwrite(file_fmt, 0, &overwrite_data));

  /* Totally 'in-bounds' - somewhere in the middle */
  testrun(3 ==
          (size_t)ov_format_payload_overwrite(file_fmt, 4, &overwrite_data));

  /* Partially 'in-bounds' - failure */
  testrun(-1 == ov_format_payload_overwrite(file_fmt, 8, &overwrite_data));

  /* Totally out-of-bounds -> failure */
  testrun(-1 == ov_format_payload_overwrite(file_fmt, ref_data.length,
                                            &overwrite_data));

  /* Check that we did not mess up just appending */

  testrun(3 == ov_format_payload_write_chunk(file_fmt, &overwrite_data));

  file_fmt = ov_format_close(file_fmt);
  testrun(0 == file_fmt);

  /* Check file content */

  file_fmt = ov_format_open(file_path, OV_READ);
  testrun(0 != file_fmt);

  ov_buffer read_data =
      ov_format_payload_read_chunk_nocopy(file_fmt, ref_data.length + 3 + 1);

  testrun(ref_data.length + 3 == read_data.length);
  testrun(0 == memcmp(read_data.start, "zyxdzyxhizyx", ref_data.length + 3));

  file_fmt = ov_format_close(file_fmt);
  testrun(0 == file_fmt);

  unlink(file_path);

  /*************************************************************************
                              Stacking formats
   ************************************************************************/

  mem_fmt = ov_format_from_memory(0, 1, OV_WRITE);
  testrun(0 != mem_fmt);

  ov_format *stacked_fmt = ov_format_wrap(mem_fmt, "dummy", &dummy_handler, 0);
  testrun(0 != stacked_fmt);

  testrun(-1 == ov_format_payload_overwrite(stacked_fmt, 0, 0));

  testrun(-1 == ov_format_payload_overwrite(0, 3, 0));

  testrun(-1 == ov_format_payload_overwrite(stacked_fmt, 3, 0));

  testrun(-1 == ov_format_payload_overwrite(0, 0, &ref_data));

  /* Out-of-bounds */
  testrun(-1 == ov_format_payload_overwrite(stacked_fmt, 0, &ref_data));

  testrun(ref_data.length ==
          (size_t)ov_format_payload_write_chunk(stacked_fmt, &ref_data));

  mem_buffer = ov_format_get_memory(stacked_fmt);
  testrun(0 != mem_buffer.start);
  testrun(mem_buffer.length == ref_data.length);
  testrun(0 == memcmp(mem_buffer.start, ref_data.start, ref_data.length));

  testrun(ref_data.length ==
          (size_t)ov_format_payload_overwrite(stacked_fmt, 0, &ref_data));

  mem_buffer = ov_format_get_memory(stacked_fmt);
  testrun(0 != mem_buffer.start);
  testrun(mem_buffer.length == ref_data.length);
  testrun(0 == memcmp(mem_buffer.start, ref_data.start, ref_data.length));

  /* Overwrite some existing bytes */
  overwrite_data = (ov_buffer){
      .start = (uint8_t *)"zyx",
      .length = 3,
  };

  testrun(overwrite_data.length ==
          (size_t)ov_format_payload_overwrite(stacked_fmt, 0, &overwrite_data));

  /* Totally 'in-bounds' */
  mem_buffer = ov_format_get_memory(stacked_fmt);
  testrun(0 != mem_buffer.start);
  testrun(mem_buffer.length == ref_data.length);
  testrun(0 == memcmp(mem_buffer.start, "zyxdefghi", ref_data.length));

  /* Totally 'in-bounds' - somewhere in the middle */
  testrun(3 ==
          (size_t)ov_format_payload_overwrite(stacked_fmt, 4, &overwrite_data));

  mem_buffer = ov_format_get_memory(stacked_fmt);
  testrun(0 != mem_buffer.start);
  testrun(mem_buffer.length == ref_data.length);
  testrun(0 == memcmp(mem_buffer.start, "zyxdzyxhi", ref_data.length));

  /* Partially 'in-bounds' - failure */
  testrun(-1 == ov_format_payload_overwrite(stacked_fmt, 8, &overwrite_data));

  mem_buffer = ov_format_get_memory(stacked_fmt);
  testrun(0 != mem_buffer.start);
  testrun(mem_buffer.length == ref_data.length);
  testrun(0 == memcmp(mem_buffer.start, "zyxdzyxhi", ref_data.length));

  /* Totally out-of-bounds -> failure */
  testrun(-1 == ov_format_payload_overwrite(stacked_fmt, ref_data.length,
                                            &overwrite_data));

  mem_buffer = ov_format_get_memory(stacked_fmt);
  testrun(0 != mem_buffer.start);
  testrun(mem_buffer.length == ref_data.length);
  testrun(0 == memcmp(mem_buffer.start, "zyxdzyxhi", ref_data.length));

  /* Check that we did not mess up just appending */

  testrun(3 == ov_format_payload_write_chunk(stacked_fmt, &overwrite_data));

  mem_buffer = ov_format_get_memory(stacked_fmt);
  testrun(0 != mem_buffer.start);
  testrun(mem_buffer.length == ref_data.length + 3);
  testrun(0 == memcmp(mem_buffer.start, "zyxdzyxhi", ref_data.length));
  testrun(0 == memcmp(mem_buffer.start + ref_data.length, "zyx", 3));

  stacked_fmt = ov_format_close(stacked_fmt);
  testrun(0 == stacked_fmt);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_wrap() {

  testrun(0 == ov_format_wrap(0, 0, 0, 0));

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

  testrun(0 == ov_format_wrap(infile, 0, 0, 0));
  testrun(0 == ov_format_wrap(0, "tokenizer", 0, 0));

  /* Simple file type - no internal data */

  ov_format_handler tokenizer = {
      .next_chunk = ft_tokenizer_next_token,
      .create_data = ft_tokenizer_create_data,
      .free_data = ft_tokenizer_free_data,
  };

  testrun(0 == ov_format_wrap(0, 0, &tokenizer, 0));
  testrun(0 == ov_format_wrap(infile, 0, &tokenizer, 0));
  testrun(0 == ov_format_wrap(0, "tokenizer", &tokenizer, 0));

  testrun(0 == ov_format_wrap(0, "tokenizer", &tokenizer, 0));

  /* test whether 'options' is passed to create function */
  ov_format *tokenizer_file =
      ov_format_wrap(infile, "tokenizer", &tokenizer, &tokenizer);
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

  infile = ov_format_open(file_path, OV_READ);
  testrun(0 != infile);

  ov_format *ftd_file = ov_format_wrap(infile, "ftd1", &ft_dummy, 0);
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

  infile = ov_format_open(file_path, OV_READ);
  testrun(0 != infile);

  ftd_file = ov_format_wrap(infile, "ftd2", &ft_dummy, 0);
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

  char const *dummy_data = "hoenir";
  ft_dummy_data_to_set = (void *)dummy_data;

  ft_dummy = (ov_format_handler){

      .next_chunk = ft_dummy_next_chunk,
      .create_data = ft_dummy_create_data,
      .free_data = ft_dummy_free_data,

  };

  infile = ov_format_open(file_path, OV_READ);
  testrun(0 != infile);

  ftd_file = ov_format_wrap(infile, "ftd3", &ft_dummy, 0);
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

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_format_get_custom_data() {

  char const *test_string = "Geir nu garmr mjok fyr gnipahellir - festr man "
                            "slitna en freki renna";

  const size_t test_string_len = strlen(test_string) + 1;

  /*------------------------------------------------------------------------*/

  ov_format *mem_fmt =
      ov_format_from_memory((uint8_t *)test_string, test_string_len, OV_READ);
  testrun(0 != mem_fmt);

  testrun(0 == ov_format_wrap(0, "tokenizer", 0, 0));

  /* Simple file type - no internal data */

  ov_format_handler tokenizer = {
      .next_chunk = ft_tokenizer_next_token,
      .create_data = ft_tokenizer_create_data,
      .free_data = ft_tokenizer_free_data,
  };

  ov_format *tokenizer_file =
      ov_format_wrap(mem_fmt, "tokenizer", &tokenizer, 0);

  mem_fmt = 0;

  testrun(0 != tokenizer_file);

  testrun(0 == ov_format_get_custom_data(0));

  testrun(is_tokenizer_data(ov_format_get_custom_data(tokenizer_file)));

  tokenizer_file = ov_format_close(tokenizer_file);

  testrun(0 == tokenizer_file);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_format_get() {

  testrun(0 == ov_format_get(0, 0));

  char const *test_string = "Geir nu garmr mjok fyr gnipahellir - festr man "
                            "slitna en freki renna";

  const size_t test_string_len = strlen(test_string) + 1;

  ov_format *mem_fmt =
      ov_format_from_memory((uint8_t *)test_string, test_string_len, OV_READ);

  testrun(0 != mem_fmt);

  testrun(0 == ov_format_get(mem_fmt, "1"));
  testrun(0 == ov_format_get(mem_fmt, "tokenizer"));
  testrun(mem_fmt == ov_format_get(mem_fmt, MEM_TYPE));

  /*------------------------------------------------------------------------*/

  ov_format_handler tokenizer = {
      .next_chunk = ft_tokenizer_next_token,
      .create_data = ft_tokenizer_create_data,
      .free_data = ft_tokenizer_free_data,
  };

  ov_format *tokenizer_0 = ov_format_wrap(mem_fmt, "tokenizer", &tokenizer, 0);
  testrun(0 != tokenizer_0);

  testrun(mem_fmt == ov_format_get(tokenizer_0, MEM_TYPE));
  testrun(tokenizer_0 == ov_format_get(tokenizer_0, "tokenizer"));
  testrun(0 == ov_format_get(tokenizer_0, "1"));

  testrun(mem_fmt == ov_format_get(tokenizer_0, MEM_TYPE));

  /* Register 2nd tokenizer layer */

  ov_format *tokenizer_1 = ov_format_wrap(tokenizer_0, "1", &tokenizer, 0);
  testrun(0 != tokenizer_1);

  testrun(0 == ov_format_get(mem_fmt, "tokenizer"));
  testrun(0 == ov_format_get(mem_fmt, "1"));

  testrun(mem_fmt == ov_format_get(tokenizer_1, MEM_TYPE));
  testrun(tokenizer_0 == ov_format_get(tokenizer_1, "tokenizer"));
  testrun(tokenizer_1 == ov_format_get(tokenizer_1, "1"));

  tokenizer_1 = ov_format_close(tokenizer_1);

  testrun(0 == tokenizer_1);

  tokenizer_0 = 0;
  mem_fmt = 0;

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_buffered() {

  testrun(0 == ov_format_buffered(0, 0));

  char const *ref_data = "Hymir";

  ov_format *buffered =
      ov_format_buffered((uint8_t *)ref_data, strlen(ref_data));

  testrun(0 != buffered);

  ov_buffer data = ov_format_payload_read_chunk_nocopy(buffered, 0);

  testrun(data.length == strlen(ref_data));
  testrun(0 == strcmp((char const *)data.start, ref_data));

  buffered = ov_format_close(buffered);

  testrun(0 == buffered);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_buffered_update() {

  testrun(!ov_format_buffered_update(0, 0, 0));

  char const *ref_data = "Ymir";
  char const *ref_data_2 = "Bor";

  ov_format *buffered =
      ov_format_buffered((uint8_t *)ref_data, strlen(ref_data));

  testrun(0 != buffered);

  testrun(!ov_format_buffered_update(buffered, 0, 0));
  testrun(!ov_format_buffered_update(0, (uint8_t *)ref_data, 0));
  testrun(!ov_format_buffered_update(buffered, (uint8_t *)ref_data, 0));
  testrun(!ov_format_buffered_update(0, 0, strlen(ref_data)));
  testrun(!ov_format_buffered_update(buffered, 0, strlen(ref_data)));
  testrun(!ov_format_buffered_update(0, (uint8_t *)ref_data, strlen(ref_data)));

  ov_buffer data = ov_format_payload_read_chunk_nocopy(buffered, 0);

  testrun(data.length == strlen(ref_data));
  testrun(0 == strcmp((char const *)data.start, ref_data));

  data = ov_format_payload_read_chunk_nocopy(buffered, 0);

  testrun(data.length == strlen(ref_data));
  testrun(0 == strcmp((char const *)data.start, ref_data));

  data = ov_format_payload_read_chunk_nocopy(buffered, 0);

  testrun(data.length == strlen(ref_data));
  testrun(0 == strcmp((char const *)data.start, ref_data));

  testrun(ov_format_buffered_update(buffered, (uint8_t *)ref_data_2,
                                    strlen(ref_data_2)));

  data = ov_format_payload_read_chunk_nocopy(buffered, 0);

  testrun(data.length == strlen(ref_data_2));
  testrun(0 == strcmp((char const *)data.start, ref_data_2));

  data = ov_format_payload_read_chunk_nocopy(buffered, 0);

  testrun(data.length == strlen(ref_data_2));
  testrun(0 == strcmp((char const *)data.start, ref_data_2));

  buffered = ov_format_close(buffered);

  testrun(0 == buffered);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_format", test_ov_format_open, test_ov_format_from_memory,
            test_ov_format_get_memory, test_ov_format_close,
            test_ov_format_has_more_data, test_ov_format_payload_read_chunk,
            test_ov_format_payload_read_chunk_nocopy,
            test_ov_format_payload_write_chunk,
            test_ov_format_payload_overwrite, test_ov_format_wrap,
            test_ov_format_get_custom_data, test_ov_format_get,
            test_ov_format_buffered, test_ov_format_buffered_update);
