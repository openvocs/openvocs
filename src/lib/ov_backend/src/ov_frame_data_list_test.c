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

        @author         Michael J. Beer

        ------------------------------------------------------------------------
*/

#include <ov_base/ov_utils.h>

#include <ov_test/ov_test.h>

#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_socket.h>

#include "ov_frame_data_list.c"

/*----------------------------------------------------------------------------*/

static bool list_is_empty(ov_frame_data_list const *list) {

  if (0 == list->frames) {
    return false;
  }

  for (size_t i = 0; i < list->capacity; ++i) {

    if (0 != list->frames[i]) {
      return false;
    }
  }

  return true;
}

/*----------------------------------------------------------------------------*/

static int test_ov_frame_data_list_enable_caching() {

  // Get clean
  ov_registered_cache_free_all();

  // No real caching probably - however, we should be able to create & free
  ov_frame_data_list_enable_caching(0);

  ov_frame_data_list *list = ov_frame_data_list_create(12);

  testrun(0 != list);

  list = ov_frame_data_list_free(list);
  testrun(0 == list);

  // Empty cache if there
  for (size_t i = 0; i < 10; ++i) {
    ov_frame_data_list *list = ov_frame_data_list_create(1);
    free_frame_data_list(list);
  }

  // Real caching
  ov_frame_data_list_enable_caching(3);

  list = ov_frame_data_list_create(12);
  testrun(0 != list);

  ov_frame_data_list *cached = list;

  list = ov_frame_data_list_free(list);
  testrun(0 == list);

  // should have been cached
  list = ov_frame_data_list_create(12);
  testrun(list == cached);
  cached = 0;

  list = ov_frame_data_list_free(list);

  // Dealing with more lists than cache buckets available

  const size_t CACHE_SIZE = 20;

  for (size_t i = 0; i < CACHE_SIZE; ++i) {
    list = ov_frame_data_list_create(1 + i);
    testrun(0 != list);

    testrun(i <= list->capacity);
    testrun(list_is_empty(list));

    list = ov_frame_data_list_free(list);
    testrun(0 == list);
  }

  // Put non-empty lists into cache

  list = ov_frame_data_list_create(3);
  testrun(0 != list);

  testrun(3 <= list->capacity);
  testrun(list_is_empty(list));

  for (size_t i = 0; i < list->capacity; ++i) {

    ov_frame_data *data = ov_frame_data_create();
    testrun(0 != data);
    data->ssid = i;
    data = ov_frame_data_list_push_data(list, data);
    testrun(0 == data);
  }

  list = ov_frame_data_list_free(list);
  testrun(0 == list);

  ov_frame_data_list **lists =
      calloc(CACHE_SIZE + 1, sizeof(ov_frame_data_list *));

  // Check that there are really only empty frames in cache
  // We need to buffer all the lists, otherwise they are put back into the
  // cache and we will not see all cached lists
  for (size_t i = 0; i < CACHE_SIZE + 1; ++i) {

    list = ov_frame_data_list_create(1);

    testrun(1 <= list->capacity);
    testrun(list_is_empty(list));
    lists[i] = list;

    list = 0;
  }

  // and free the lists
  for (size_t i = 0; i < CACHE_SIZE + 1; ++i) {

    lists[i] = ov_frame_data_list_free(lists[i]);
    testrun(0 == lists[i]);
  }

  free(lists);
  lists = 0;

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_frame_data_list_create() {

  testrun(0 == ov_frame_data_list_create(0));

  ov_frame_data_list *list = ov_frame_data_list_create(12);

  testrun(0 != list);
  testrun(FRAME_DATA_LIST_MAGIC_BYTES == list->magic_bytes);
  testrun(12 <= list->capacity);
  testrun(0 != list->frames);

  list = ov_frame_data_list_free(list);
  testrun(0 == list);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_frame_data_list_free() {

  testrun(0 == ov_frame_data_list_free(0));

  ov_frame_data_list *list = ov_frame_data_list_create(18);
  testrun(0 != list);

  list = ov_frame_data_list_free(list);
  testrun(0 == list);

  list = ov_frame_data_list_create(18);
  testrun(0 != list);

  ov_frame_data *data = ov_frame_data_create();
  testrun(0 != data);
  data->ssid = 11;

  testrun(0 == ov_frame_data_list_push_data(list, data));

  data = ov_frame_data_create();
  testrun(0 != data);
  data->ssid = 8;

  testrun(0 == ov_frame_data_list_push_data(list, data));

  list = ov_frame_data_list_free(list);
  testrun(0 == list);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_frame_data_list *
get_frame_data_list_with_exactly_num_entries(size_t n) {

  ov_frame_data_list *list = ov_frame_data_list_create(n);

  while (n != list->capacity) {

    free_frame_data_list(list);
    list = ov_frame_data_list_create(n);
  };

  return list;
}

/*----------------------------------------------------------------------------*/

static int test_ov_frame_data_list_push_data() {

  testrun(0 == ov_frame_data_list_push_data(0, 0));

  ov_frame_data *data = ov_frame_data_create();
  data->ssid = 13;

  testrun(data == ov_frame_data_list_push_data(0, data));

  ov_frame_data_list *list = get_frame_data_list_with_exactly_num_entries(3);
  testrun(3 == list->capacity);

  testrun(0 == ov_frame_data_list_push_data(list, data));

  data = ov_frame_data_create();
  testrun(0 != data);

  data->ssid = 13;

  ov_frame_data *old = ov_frame_data_list_push_data(list, data);

  testrun(0 != old);
  testrun(13 == old->ssid);
  testrun(old != data);
  data = 0;

  old->ssid = 14;
  testrun(0 == ov_frame_data_list_push_data(list, old));

  old = 0;

  data = ov_frame_data_create();
  data->ssid = 15;
  testrun(0 == ov_frame_data_list_push_data(list, data));

  // List exhausted
  data = ov_frame_data_create();
  data->ssid = 16;
  testrun(data == ov_frame_data_list_push_data(list, data));

  // Overwrite another entry
  data->ssid = 14;

  old = ov_frame_data_list_push_data(list, data);

  testrun(0 != old);
  testrun(14 == old->ssid);
  old = ov_frame_data_free(old);
  testrun(0 == old);

  list = ov_frame_data_list_free(list);
  testrun(0 == list);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_frame_data_list_pop_data() {

  testrun(0 == ov_frame_data_list_pop_data(0, 0));

  ov_frame_data_list *list = ov_frame_data_list_create(2);
  testrun(0 != list);

  testrun(0 == ov_frame_data_list_pop_data(list, 0));
  testrun(0 == ov_frame_data_list_pop_data(list, 1));

  ov_frame_data *data_1 = ov_frame_data_create();
  data_1->ssid = 1;

  testrun(0 == ov_frame_data_list_push_data(list, data_1));

  ov_frame_data *data_2 = ov_frame_data_create();
  data_2->ssid = 2;

  testrun(0 == ov_frame_data_list_push_data(list, data_2));

  testrun(0 == ov_frame_data_list_pop_data(list, 3));
  testrun(data_2 == ov_frame_data_list_pop_data(list, 2));
  testrun(data_1 == ov_frame_data_list_pop_data(list, 1));

  testrun(0 == ov_frame_data_list_push_data(list, data_2));
  testrun(0 == ov_frame_data_list_push_data(list, data_1));

  data_2 = 0;
  data_1 = 0;

  list = ov_frame_data_list_free(list);

  testrun(0 == list);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int tear_down() {

  ov_registered_cache_free_all();

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_frame_data_list", test_ov_frame_data_list_enable_caching,
            test_ov_frame_data_list_create, test_ov_frame_data_list_free,
            test_ov_frame_data_list_push_data, test_ov_frame_data_list_pop_data,
            tear_down);

/*----------------------------------------------------------------------------*/
