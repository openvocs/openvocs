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
        @file           ov_buffer_test.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2019-01-25

        @ingroup        ov_buffer

        @brief          Unit tests of


        ------------------------------------------------------------------------
*/
#include "ov_buffer.c"
#include "ov_string.h"
#include <ov_test/ov_test.h>
#include <ov_test/testrun.h>

/*----------------------------------------------------------------------------*/

int test_ov_buffer_create() {

  ov_buffer *buffer = NULL;

  buffer = ov_buffer_create(0);
  testrun(buffer);
  testrun(ov_buffer_cast(buffer));
  testrun(NULL == buffer->start);
  testrun(0 == buffer->length);
  testrun(0 == buffer->capacity);
  testrun(NULL == ov_buffer_free(buffer));

  buffer = ov_buffer_create(100);
  testrun(buffer);
  testrun(ov_buffer_cast(buffer));
  testrun(NULL != buffer->start);
  testrun(0 == buffer->length);
  testrun(100 == buffer->capacity);
  testrun(NULL == ov_buffer_free(buffer));

  buffer = ov_buffer_create(1000);
  testrun(buffer);
  testrun(ov_buffer_cast(buffer));
  testrun(NULL != buffer->start);
  testrun(0 == buffer->length);
  testrun(1000 == buffer->capacity);
  testrun(NULL == ov_buffer_free(buffer));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_buffer_from_string() {

  ov_buffer *buffer = NULL;

  testrun(NULL == ov_buffer_from_string(NULL));

  buffer = ov_buffer_from_string("ab");
  testrun(buffer);
  testrun(ov_buffer_cast(buffer));
  testrun(NULL != buffer->start);
  testrun(2 == buffer->length);
  testrun(3 == buffer->capacity);
  testrun(0 == strncmp("ab", (char *)buffer->start, buffer->length));

  // The content of the buffer should still be 0 - terminated
  testrun(0 == buffer->start[buffer->length]);

  testrun(NULL == ov_buffer_free(buffer));

  buffer = ov_buffer_from_string("abcd");
  testrun(buffer);
  testrun(ov_buffer_cast(buffer));
  testrun(NULL != buffer->start);
  testrun(4 == buffer->length);
  testrun(5 == buffer->capacity);
  testrun(0 == strncmp("abcd", (char *)buffer->start, buffer->length));
  // The content of the buffer should still be 0 - terminated
  testrun(0 == buffer->start[buffer->length]);

  testrun(NULL == ov_buffer_free(buffer));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_buffer_concat() {

  testrun(0 == ov_buffer_concat(0, 0));

  ov_buffer *b1 = ov_buffer_from_string("123");
  testrun(3 == b1->length);

  testrun(b1 == ov_buffer_concat(b1, 0));
  ov_buffer *result = ov_buffer_concat(0, b1);

  testrun(result != b1);

  testrun(0 != result);
  testrun(result->length = b1->length);
  testrun(0 == memcmp(result->start, b1->start, b1->length));

  result = ov_buffer_free(result);

  ov_buffer *empty = ov_buffer_create(1);
  empty->length = 0;

  testrun(b1 == ov_buffer_concat(b1, empty));
  testrun(3 == b1->length);

  empty = ov_buffer_free(empty);

  ov_buffer *b2 = ov_buffer_from_string("4567");
  testrun(4 == b2->length);

  // Force target buffer too small
  b1->capacity = 4;

  b1 = ov_buffer_concat(b1, b2);
  testrun(7 == b1->length);

  testrun(0 == ov_string_compare((char *)b1->start, "1234567"));

  b1 = ov_buffer_free(b1);

  // Target buffer with sufficient mem
  b1 = ov_buffer_create(7);
  strcpy((char *)b1->start, "ab");
  b1->length = 2;

  result = ov_buffer_concat(b1, b2);
  testrun(b1 == result);
  result = 0;
  testrun(6 == b1->length);

  testrun(0 == ov_string_compare((char *)b1->start, "ab4567"));

  result = ov_buffer_free(result);

  b2 = ov_buffer_free(b2);
  b1 = ov_buffer_free(b1);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_buffer_cast() {

  ov_buffer *buffer = ov_buffer_create(0);

  testrun(buffer);
  testrun(ov_buffer_cast(buffer));

  for (size_t i = 0; i < 0xffff; i++) {

    buffer->magic_byte = i;

    if (i == OV_BUFFER_MAGIC_BYTE) {
      testrun(ov_buffer_cast(buffer));
    } else {
      testrun(!ov_buffer_cast(buffer));
    }
  }

  buffer->magic_byte = OV_BUFFER_MAGIC_BYTE;
  testrun(NULL == ov_buffer_free(buffer));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_buffer_set() {

  ov_buffer *buffer = ov_buffer_create(0);

  testrun(buffer);
  testrun(ov_buffer_cast(buffer));

  char *data = "test";
  testrun(!ov_buffer_set(NULL, NULL, 0));
  testrun(!ov_buffer_set(NULL, data, strlen(data)));
  testrun(!ov_buffer_set(buffer, NULL, strlen(data)));
  testrun(!ov_buffer_set(buffer, data, 0));

  testrun(ov_buffer_set(buffer, data, strlen(data)));
  testrun(NULL != buffer->start);
  testrun(4 == buffer->length);
  testrun(4 == buffer->capacity);
  testrun(0 == strncmp(data, (char *)buffer->start, buffer->length));
  // check independent copy
  testrun(data != (char *)buffer->start);
  // check additional zero termination byte
  testrun(4 == strlen((char *)buffer->start));

  // set some longer data
  data = "abcd1234";
  testrun(ov_buffer_set(buffer, data, strlen(data)));
  testrun(NULL != buffer->start);
  testrun(8 == buffer->length);
  testrun(8 == buffer->capacity);
  testrun(0 == strncmp(data, (char *)buffer->start, buffer->length));

  // set some smaller data
  data = "1234";
  testrun(ov_buffer_set(buffer, data, strlen(data)));
  testrun(NULL != buffer->start);
  testrun(4 == buffer->length);
  testrun(8 == buffer->capacity);
  testrun(0 == strncmp(data, (char *)buffer->start, buffer->length));

  testrun(NULL == ov_buffer_free(buffer));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_buffer_clear() {

  ov_buffer *buffer = ov_buffer_create(0);

  testrun(buffer);
  testrun(ov_buffer_cast(buffer));

  testrun(!ov_buffer_clear(NULL));

  char *data = "test";

  // clear pointer buffer (empty)
  testrun(NULL == buffer->start);
  testrun(0 == buffer->length);
  testrun(0 == buffer->capacity);
  testrun(ov_buffer_clear(buffer));
  testrun(NULL == buffer->start);
  testrun(0 == buffer->length);
  testrun(0 == buffer->capacity);

  // clear pointer buffer (set)
  buffer->start = (uint8_t *)data;
  buffer->length = 3;
  testrun(ov_buffer_clear(buffer));
  testrun(NULL == buffer->start);
  testrun(0 == buffer->length);
  testrun(0 == buffer->capacity);

  // clear allocated buffer (set)
  testrun(ov_buffer_set(buffer, data, strlen(data)));
  testrun(ov_buffer_clear(buffer));
  testrun(NULL != buffer->start);
  testrun(0 == buffer->length);
  testrun(4 == buffer->capacity);
  for (size_t i = 0; i <= buffer->capacity; i++) {
    testrun(buffer->start[i] == 0);
  }

  // clear allocate buffer (empty)
  testrun(ov_buffer_clear(buffer));
  testrun(NULL != buffer->start);
  testrun(0 == buffer->length);
  testrun(4 == buffer->capacity);
  for (size_t i = 0; i <= buffer->capacity; i++) {
    testrun(buffer->start[i] == 0);
  }

  testrun(NULL == ov_buffer_free(buffer));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_buffer_free() {

  ov_buffer *buffer = ov_buffer_create(0);

  testrun(NULL == ov_buffer_free(NULL));
  testrun(NULL == ov_buffer_free(buffer));

  char *data = "test";

  // free pointer buffer (empty)
  buffer = ov_buffer_create(0);
  testrun(NULL == ov_buffer_free(buffer));

  // clear pointer buffer (set)
  buffer = ov_buffer_create(0);
  testrun(0 != buffer);

  buffer->start = (uint8_t *)data;
  buffer->length = 3;
  testrun(NULL == ov_buffer_free(buffer));

  // clear allocated buffer (set)
  buffer = ov_buffer_from_string(data);
  testrun(NULL == ov_buffer_free(buffer));

  // clear allocate buffer (empty)
  buffer = ov_buffer_from_string(data);
  testrun(ov_buffer_clear(buffer));
  testrun(NULL == ov_buffer_free(buffer));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_buffer_copy() {

  ov_buffer *buffer = ov_buffer_create(0);
  ov_buffer *copy = NULL;

  testrun(NULL == ov_buffer_copy(NULL, NULL));

  copy = ov_buffer_copy(NULL, buffer);
  testrun(copy != NULL);
  testrun(NULL == copy->start);
  testrun(0 == copy->length);
  testrun(0 == copy->capacity);
  copy = ov_buffer_free(copy);
  testrun(0 == copy);

  testrun(NULL == ov_buffer_copy((void **)&copy, NULL));
  testrun(copy == NULL);

  // copy empty pointer buffer
  testrun(ov_buffer_copy((void **)&copy, buffer));
  testrun(copy != NULL);
  testrun(NULL == copy->start);
  testrun(0 == copy->length);
  testrun(0 == copy->capacity);

  copy = ov_buffer_free(copy);
  testrun(0 == copy);

  char *data = "test";
  buffer->start = (uint8_t *)data;
  buffer->length = 3;

  copy = ov_buffer_copy(0, buffer);
  testrun(NULL != copy->start);
  testrun(3 == copy->length);
  testrun(3 == copy->capacity);
  testrun(0 == strncmp(data, (char *)copy->start, copy->length));

  copy = ov_buffer_free(copy);
  testrun(0 == copy);

  // copy non empty pointer buffer
  testrun(NULL != ov_buffer_copy((void **)&copy, buffer));
  testrun(copy);
  testrun(NULL != copy->start);
  testrun(3 == copy->length);
  testrun(3 == copy->capacity);
  testrun(0 == strncmp(data, (char *)copy->start, copy->length));

  // copy from allocated to pointer buffer
  testrun(buffer == ov_buffer_copy((void **)&buffer, copy));
  testrun(NULL != buffer->start);
  testrun(3 == buffer->length);
  testrun(3 == buffer->capacity);
  testrun(0 == strncmp(data, (char *)buffer->start, buffer->length));
  testrun(copy->start != buffer->start);

  // copy with longer data
  data = "12345678";
  testrun(ov_buffer_set(buffer, data, strlen(data)));
  testrun(copy == ov_buffer_copy((void **)&copy, buffer));
  testrun(NULL != copy->start);
  testrun(8 == copy->length);
  testrun(8 == copy->capacity);
  testrun(0 == strncmp(data, (char *)copy->start, copy->length));

  testrun(NULL == ov_buffer_free(buffer));
  testrun(NULL == ov_buffer_free(copy));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_buffer_dump() {

  ov_buffer *buffer = ov_buffer_create(0);

  testrun(!ov_buffer_dump(NULL, NULL));
  testrun(!ov_buffer_dump(NULL, buffer));
  testrun(!ov_buffer_dump(stdout, NULL));

  // dump empty pointer buffer
  testrun(ov_buffer_dump(stdout, buffer));

  char *data = "test";
  buffer->start = (uint8_t *)data;
  buffer->length = 3;
  // dump non empty pointer buffer
  testrun(ov_buffer_dump(stdout, buffer));

  testrun(ov_buffer_set(buffer, data, strlen(data)));
  testrun(ov_buffer_dump(stdout, buffer));

  testrun(NULL == ov_buffer_free(buffer));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_buffer_push() {

  ov_buffer *buffer = ov_buffer_create(0);
  testrun(buffer);
  testrun(buffer->capacity == 0);

  testrun(!ov_buffer_push(NULL, NULL, 0));
  testrun(!ov_buffer_push(NULL, "test", 4));
  testrun(!ov_buffer_push(buffer, NULL, 4));
  testrun(!ov_buffer_push(buffer, "test", 0));

  testrun(NULL == buffer->start);
  testrun(0 == buffer->length);
  testrun(0 == buffer->capacity);

  testrun(ov_buffer_push(buffer, "test", 4));
  testrun(NULL != buffer->start);
  testrun(4 == buffer->length);
  testrun(4 == buffer->capacity);
  testrun(0 == strncmp((char *)buffer->start, "test", 4));

  testrun(ov_buffer_push(buffer, "abcd", 2));
  testrun(NULL != buffer->start);
  testrun(6 == buffer->length);
  testrun(6 == buffer->capacity);
  testrun(0 == strncmp((char *)buffer->start, "testab", 6));

  testrun(ov_buffer_push(buffer, "abcd", 4));
  testrun(NULL != buffer->start);
  testrun(10 == buffer->length);
  testrun(10 == buffer->capacity);
  testrun(0 == strncmp((char *)buffer->start, "testababcd", 10));

  testrun(NULL == ov_buffer_free(buffer));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_buffer_data_functions() {

  ov_data_function func = ov_buffer_data_functions();
  testrun(func.clear == ov_buffer_clear);
  testrun(func.free == ov_buffer_free);
  testrun(func.copy == ov_buffer_copy);
  testrun(func.dump == ov_buffer_dump);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_buffer_enable_caching() {

  ov_buffer_enable_caching(1);

  ov_buffer *buffer = ov_buffer_create(3);

  testrun(0 != buffer);
  testrun(3 <= buffer->capacity);

  testrun(0 == ov_buffer_free(buffer));

  ov_buffer *buffer_2 = ov_buffer_create(2);

  /*
   * Since we cache, and `buffer` was only freed, `buffer_2` should actually
   * be `buffer`
   *
   * BEWARE:
   * This actually tests implementation details,
   * but the only way I see to test anything reasonable in here...
   */

  testrun(buffer == buffer_2);
  testrun(2 <= buffer_2->capacity);

  testrun(0 == ov_buffer_free(buffer_2));

  buffer_2 = ov_buffer_create(7);

  testrun(buffer == buffer_2);

  /* Don't need `buffer` as reference any more */
  buffer = 0;

  testrun(7 <= buffer_2->capacity);

  buffer_2 = ov_buffer_free(buffer_2);
  testrun(0 == buffer_2);

  /* Should enable default cache size */
  ov_buffer_enable_caching(0);

  buffer = ov_buffer_create(3);

  testrun(0 != buffer);
  testrun(3 <= buffer->capacity);

  testrun(0 == ov_buffer_free(buffer));

  /* Now check if we get the same buffer again ... */
  /* See note further up */

  buffer_2 = ov_buffer_create(2);

  testrun(buffer == buffer_2);
  testrun(2 <= buffer_2->capacity);

  testrun(0 == ov_buffer_free(buffer_2));

  buffer_2 = ov_buffer_create(7);

  testrun(buffer == buffer_2);
  testrun(7 <= buffer_2->capacity);

  buffer_2 = ov_buffer_free(buffer_2);

  testrun(0 == buffer_2);

  ov_registered_cache_free_all();
  g_cache = NULL;
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_buffer_shift() {

  ov_buffer *buffer = ov_buffer_create(100);

  testrun(buffer);
  testrun(NULL != buffer->start);
  testrun(0 == buffer->length);
  testrun(100 == buffer->capacity);

  for (size_t i = 0; i < 100; i++) {
    buffer->start[i] = i;
  }
  buffer->length = 100;

  uint8_t *ptr = buffer->start + 10;

  testrun(!ov_buffer_shift(NULL, NULL));
  testrun(!ov_buffer_shift(buffer, NULL));
  testrun(!ov_buffer_shift(NULL, ptr));

  testrun(ov_buffer_shift(buffer, ptr));
  // ov_buffer_dump(stdout, buffer);
  testrun(buffer->start[0] == 10);
  testrun(buffer->length == 90);
  testrun(buffer->capacity == 100);

  for (size_t i = 0; i < 100; i++) {

    if (i < 90) {
      testrun(buffer->start[i] == i + 10);
    } else {
      testrun(buffer->start[i] == 0);
    }
  }

  // try to shift > length
  ptr = buffer->start + buffer->length + 1;
  testrun(!ov_buffer_shift(buffer, ptr));
  testrun(buffer->start[0] == 10);
  testrun(buffer->length == 90);
  testrun(buffer->capacity == 100);

  // try to shift by length
  ptr = buffer->start + buffer->length;
  testrun(ov_buffer_shift(buffer, ptr));
  testrun(buffer->start[0] == 0);
  testrun(buffer->length == 0);
  testrun(buffer->capacity == 100);

  char *somewhere = "somewhere";

  ptr = (uint8_t *)somewhere;
  testrun(!ov_buffer_shift(buffer, ptr));

  testrun(NULL == ov_buffer_free(buffer));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_buffer_shift_length() {

  ov_buffer *buffer = ov_buffer_create(100);

  testrun(buffer);
  testrun(NULL != buffer->start);
  testrun(0 == buffer->length);
  testrun(100 == buffer->capacity);

  for (size_t i = 0; i < 100; i++) {
    buffer->start[i] = i;
  }
  buffer->length = 100;

  testrun(!ov_buffer_shift_length(NULL, 0));
  testrun(!ov_buffer_shift_length(NULL, 10));

  testrun(ov_buffer_shift_length(buffer, 0));

  testrun(ov_buffer_shift_length(buffer, 10));
  testrun(buffer->start[0] == 10);
  testrun(buffer->length == 90);
  testrun(buffer->capacity == 100);

  for (size_t i = 0; i < 100; i++) {

    if (i < 90) {
      testrun(buffer->start[i] == i + 10);
    } else {
      testrun(buffer->start[i] == 0);
    }
  }

  // try to shift > length
  testrun(!ov_buffer_shift_length(buffer, buffer->length + 1));
  testrun(buffer->start[0] == 10);
  testrun(buffer->length == 90);
  testrun(buffer->capacity == 100);

  // try to shift by length
  testrun(ov_buffer_shift_length(buffer, buffer->length));
  testrun(buffer->start[0] == 0);
  testrun(buffer->length == 0);
  testrun(buffer->capacity == 100);

  testrun(!ov_buffer_shift_length(buffer, 1000));

  testrun(NULL == ov_buffer_free(buffer));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_buffer_extend() {

  ov_buffer *buffer = ov_buffer_create(1);

  testrun(buffer);
  testrun(NULL != buffer->start);
  testrun(0 == buffer->length);
  testrun(1 == buffer->capacity);

  buffer->start[0] = 'a';
  buffer->length = 1;

  testrun(!ov_buffer_extend(NULL, 0));
  testrun(!ov_buffer_extend(buffer, 0));
  testrun(!ov_buffer_extend(NULL, 1));

  testrun(ov_buffer_extend(buffer, 1));
  testrun(buffer->length == 1);
  testrun(buffer->capacity == 2);
  testrun(buffer->start[0] == 'a');

  testrun(ov_buffer_extend(buffer, 100));
  testrun(buffer->length == 1);
  testrun(buffer->capacity == 102);
  testrun(buffer->start[0] == 'a');

  testrun(ov_buffer_extend(buffer, 10));
  testrun(buffer->length == 1);
  testrun(buffer->capacity == 112);
  testrun(buffer->start[0] == 'a');

  testrun(ov_buffer_extend(buffer, 8));
  testrun(buffer->length == 1);
  testrun(buffer->capacity == 120);
  testrun(buffer->start[0] == 'a');

  testrun(NULL == ov_buffer_free(buffer));

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

OV_TEST_RUN("ov_buffer", test_ov_buffer_create, test_ov_buffer_from_string,
            test_ov_buffer_concat, test_ov_buffer_cast, test_ov_buffer_set,
            test_ov_buffer_clear, test_ov_buffer_free, test_ov_buffer_copy,
            test_ov_buffer_dump, test_ov_buffer_push,
            test_ov_buffer_data_functions, test_ov_buffer_enable_caching,
            test_ov_buffer_shift, test_ov_buffer_shift_length,
            test_ov_buffer_extend);
