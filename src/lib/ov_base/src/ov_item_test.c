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
        @file           ov_item_test.c
        @author         Töpfer, Markus

        @date           2025-11-28


        ------------------------------------------------------------------------
*/
#include "ov_item.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

int test_ov_item_cast() {

  uint16_t magic_byte = 0;

  for (uint16_t i = 0; i < UINT16_MAX; i++) {

    magic_byte = i;

    if (i == OV_ITEM_MAGIC_BYTES) {
      testrun(ov_item_cast(&magic_byte));
    } else {
      testrun(!ov_item_cast(&magic_byte))
    }
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_object() {

  ov_item *item = ov_item_object();
  testrun(ov_item_is_object(item));
  testrun(ov_dict_cast(item->config.data));
  testrun(NULL == ov_item_free(item));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_array() {

  ov_item *item = ov_item_array();
  testrun(ov_item_is_array(item));
  testrun(ov_list_cast(item->config.data));
  testrun(NULL == ov_item_free(item));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_string() {

  ov_item *item = ov_item_string("test");
  testrun(ov_item_is_string(item));
  testrun(0 == ov_string_compare(item->config.data, "test"));
  testrun(NULL == ov_item_free(item));

  testrun(NULL == ov_item_string(NULL));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_number() {

  ov_item *item = ov_item_number(0);
  testrun(ov_item_is_number(item));
  testrun(0 == item->config.number);
  testrun(NULL == ov_item_free(item));

  for (size_t i = 0; i < 100; i++) {

    ov_item *item = ov_item_number(i);
    testrun(item);
    testrun(ov_item_is_number(item));
    testrun(i == item->config.number);
    testrun(NULL == ov_item_free(item));
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_true() {

  ov_item *item = ov_item_true();
  testrun(ov_item_is_true(item));
  testrun(NULL == ov_item_free(item));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_false() {

  ov_item *item = ov_item_false();
  testrun(ov_item_is_false(item));
  testrun(NULL == ov_item_free(item));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_null() {

  ov_item *item = ov_item_null();
  testrun(ov_item_is_null(item));
  testrun(NULL == ov_item_free(item));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_free() {

  ov_item *item = ov_item_null();
  ov_list *list = ov_linked_list_create((ov_list_config){0});

  testrun(list == ov_item_free(list));
  testrun(NULL == ov_list_free(list));

  testrun(ov_item_is_null(item));
  testrun(NULL == ov_item_free(item));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_clear() {

  ov_item *item = ov_item_null();
  ov_list *list = ov_linked_list_create((ov_list_config){0});

  testrun(!ov_item_clear(list));
  testrun(NULL == ov_list_free(list));

  testrun(ov_item_clear(item));
  testrun(ov_item_is_null(item));
  testrun(NULL == item->config.data);
  testrun(NULL == ov_item_free(item));

  item = ov_item_true();
  testrun(ov_item_clear(item));
  testrun(ov_item_is_null(item));
  testrun(NULL == item->config.data);
  testrun(NULL == ov_item_free(item));

  item = ov_item_false();
  testrun(ov_item_clear(item));
  testrun(ov_item_is_null(item));
  testrun(NULL == item->config.data);
  testrun(NULL == ov_item_free(item));

  item = ov_item_number(100);
  testrun(ov_item_clear(item));
  testrun(ov_item_is_null(item));
  testrun(NULL == item->config.data);
  testrun(NULL == ov_item_free(item));

  item = ov_item_string("test");
  testrun(ov_item_clear(item));
  testrun(ov_item_is_null(item));
  testrun(NULL == item->config.data);
  testrun(NULL == ov_item_free(item));

  item = ov_item_object();
  testrun(ov_item_clear(item));
  testrun(ov_item_is_null(item));
  testrun(NULL == item->config.data);
  testrun(NULL == ov_item_free(item));

  item = ov_item_array();
  testrun(ov_item_clear(item));
  testrun(ov_item_is_null(item));
  testrun(NULL == item->config.data);
  testrun(NULL == ov_item_free(item));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_copy() {

  ov_item *item = ov_item_object();
  ov_item *copy = NULL;

  testrun(ov_item_object_set(item, "1", ov_item_null()));
  testrun(ov_item_object_set(item, "2", ov_item_true()));
  testrun(ov_item_object_set(item, "3", ov_item_false()));
  testrun(ov_item_object_set(item, "4", ov_item_string("test")));
  testrun(ov_item_object_set(item, "5", ov_item_array()));
  testrun(ov_item_object_set(item, "6", ov_item_object()));
  testrun(ov_item_object_set(item, "7", ov_item_number(7)));
  testrun(7 == ov_item_count(item));

  testrun(!ov_item_copy((void **)&copy, NULL));

  testrun(ov_item_copy((void **)&copy, item));
  testrun(copy);
  testrun(7 == ov_item_count(copy));
  testrun(copy != item);

  testrun(ov_item_is_null((ov_item *)ov_item_object_get(copy, "1")));
  testrun(ov_item_is_true((ov_item *)ov_item_object_get(copy, "2")));
  testrun(ov_item_is_false((ov_item *)ov_item_object_get(copy, "3")));
  testrun(ov_item_is_string((ov_item *)ov_item_object_get(copy, "4")));
  testrun(ov_item_is_array((ov_item *)ov_item_object_get(copy, "5")));
  testrun(ov_item_is_object((ov_item *)ov_item_object_get(copy, "6")));
  testrun(ov_item_is_number((ov_item *)ov_item_object_get(copy, "7")));

  testrun(NULL == ov_item_free(item));
  testrun(NULL == ov_item_free(copy));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_dump() {

  ov_item *item = ov_item_object();

  testrun(ov_item_object_set(item, "1", ov_item_null()));
  testrun(ov_item_object_set(item, "2", ov_item_true()));
  testrun(ov_item_object_set(item, "3", ov_item_false()));
  testrun(ov_item_object_set(item, "4", ov_item_string("test")));
  testrun(ov_item_object_set(item, "5", ov_item_array()));
  testrun(ov_item_object_set(item, "6", ov_item_object()));
  testrun(ov_item_object_set(item, "7", ov_item_number(7)));
  testrun(7 == ov_item_count(item));

  testrun(ov_item_dump(stdout, item));
  testrun(NULL == ov_item_free(item));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_count() {

  ov_item *item = ov_item_null();
  testrun(ov_item_is_null(item));
  testrun(1 == ov_item_count(item));
  testrun(NULL == ov_item_free(item));

  item = ov_item_true();
  testrun(1 == ov_item_count(item));
  testrun(NULL == ov_item_free(item));

  item = ov_item_false();
  testrun(1 == ov_item_count(item));
  testrun(NULL == ov_item_free(item));

  item = ov_item_number(100);
  testrun(1 == ov_item_count(item));
  testrun(NULL == ov_item_free(item));

  item = ov_item_string("test");
  testrun(1 == ov_item_count(item));
  testrun(NULL == ov_item_free(item));

  item = ov_item_object();
  testrun(0 == ov_item_count(item));
  testrun(ov_item_object_set(item, "1", ov_item_null()));
  testrun(ov_item_object_set(item, "2", ov_item_true()));
  testrun(ov_item_object_set(item, "3", ov_item_false()));
  testrun(ov_item_object_set(item, "4", ov_item_string("test")));
  testrun(ov_item_object_set(item, "5", ov_item_array()));
  testrun(ov_item_object_set(item, "6", ov_item_object()));
  testrun(ov_item_object_set(item, "7", ov_item_number(7)));
  testrun(7 == ov_item_count(item));
  testrun(NULL == ov_item_free(item));

  item = ov_item_array();
  testrun(0 == ov_item_count(item));
  testrun(ov_item_array_push(item, ov_item_null()));
  testrun(ov_item_array_push(item, ov_item_true()));
  testrun(ov_item_array_push(item, ov_item_false()));
  testrun(ov_item_array_push(item, ov_item_string("test")));
  testrun(ov_item_array_push(item, ov_item_array()));
  testrun(ov_item_array_push(item, ov_item_object()));
  testrun(ov_item_array_push(item, ov_item_number(7)));
  testrun(7 == ov_item_count(item));
  testrun(NULL == ov_item_free(item));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_is_object() {

  ov_item *n = ov_item_null();
  ov_item *f = ov_item_false();
  ov_item *t = ov_item_true();
  ov_item *nbr = ov_item_number(0);
  ov_item *str = ov_item_string("test");
  ov_item *arr = ov_item_array();
  ov_item *obj = ov_item_object();

  testrun(!ov_item_is_object(n));
  testrun(!ov_item_is_object(f));
  testrun(!ov_item_is_object(t));
  testrun(!ov_item_is_object(nbr));
  testrun(!ov_item_is_object(str));
  testrun(!ov_item_is_object(arr));
  testrun(ov_item_is_object(obj));

  testrun(NULL == ov_item_free(n));
  testrun(NULL == ov_item_free(f));
  testrun(NULL == ov_item_free(t));
  testrun(NULL == ov_item_free(nbr));
  testrun(NULL == ov_item_free(str));
  testrun(NULL == ov_item_free(arr));
  testrun(NULL == ov_item_free(obj));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_is_array() {

  ov_item *n = ov_item_null();
  ov_item *f = ov_item_false();
  ov_item *t = ov_item_true();
  ov_item *nbr = ov_item_number(0);
  ov_item *str = ov_item_string("test");
  ov_item *arr = ov_item_array();
  ov_item *obj = ov_item_object();

  testrun(!ov_item_is_array(n));
  testrun(!ov_item_is_array(f));
  testrun(!ov_item_is_array(t));
  testrun(!ov_item_is_array(nbr));
  testrun(!ov_item_is_array(str));
  testrun(ov_item_is_array(arr));
  testrun(!ov_item_is_array(obj));

  testrun(NULL == ov_item_free(n));
  testrun(NULL == ov_item_free(f));
  testrun(NULL == ov_item_free(t));
  testrun(NULL == ov_item_free(nbr));
  testrun(NULL == ov_item_free(str));
  testrun(NULL == ov_item_free(arr));
  testrun(NULL == ov_item_free(obj));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_is_string() {

  ov_item *n = ov_item_null();
  ov_item *f = ov_item_false();
  ov_item *t = ov_item_true();
  ov_item *nbr = ov_item_number(0);
  ov_item *str = ov_item_string("test");
  ov_item *arr = ov_item_array();
  ov_item *obj = ov_item_object();

  testrun(!ov_item_is_string(n));
  testrun(!ov_item_is_string(f));
  testrun(!ov_item_is_string(t));
  testrun(!ov_item_is_string(nbr));
  testrun(ov_item_is_string(str));
  testrun(!ov_item_is_string(arr));
  testrun(!ov_item_is_string(obj));

  testrun(NULL == ov_item_free(n));
  testrun(NULL == ov_item_free(f));
  testrun(NULL == ov_item_free(t));
  testrun(NULL == ov_item_free(nbr));
  testrun(NULL == ov_item_free(str));
  testrun(NULL == ov_item_free(arr));
  testrun(NULL == ov_item_free(obj));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_is_number() {

  ov_item *n = ov_item_null();
  ov_item *f = ov_item_false();
  ov_item *t = ov_item_true();
  ov_item *nbr = ov_item_number(0);
  ov_item *str = ov_item_string("test");
  ov_item *arr = ov_item_array();
  ov_item *obj = ov_item_object();

  testrun(!ov_item_is_number(n));
  testrun(!ov_item_is_number(f));
  testrun(!ov_item_is_number(t));
  testrun(ov_item_is_number(nbr));
  testrun(!ov_item_is_number(str));
  testrun(!ov_item_is_number(arr));
  testrun(!ov_item_is_number(obj));

  testrun(NULL == ov_item_free(n));
  testrun(NULL == ov_item_free(f));
  testrun(NULL == ov_item_free(t));
  testrun(NULL == ov_item_free(nbr));
  testrun(NULL == ov_item_free(str));
  testrun(NULL == ov_item_free(arr));
  testrun(NULL == ov_item_free(obj));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_is_null() {

  ov_item *n = ov_item_null();
  ov_item *f = ov_item_false();
  ov_item *t = ov_item_true();
  ov_item *nbr = ov_item_number(0);
  ov_item *str = ov_item_string("test");
  ov_item *arr = ov_item_array();
  ov_item *obj = ov_item_object();

  testrun(ov_item_is_null(n));
  testrun(!ov_item_is_null(f));
  testrun(!ov_item_is_null(t));
  testrun(!ov_item_is_null(nbr));
  testrun(!ov_item_is_null(str));
  testrun(!ov_item_is_null(arr));
  testrun(!ov_item_is_null(obj));

  testrun(NULL == ov_item_free(n));
  testrun(NULL == ov_item_free(f));
  testrun(NULL == ov_item_free(t));
  testrun(NULL == ov_item_free(nbr));
  testrun(NULL == ov_item_free(str));
  testrun(NULL == ov_item_free(arr));
  testrun(NULL == ov_item_free(obj));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_is_false() {

  ov_item *n = ov_item_null();
  ov_item *f = ov_item_false();
  ov_item *t = ov_item_true();
  ov_item *nbr = ov_item_number(0);
  ov_item *str = ov_item_string("test");
  ov_item *arr = ov_item_array();
  ov_item *obj = ov_item_object();

  testrun(!ov_item_is_false(n));
  testrun(ov_item_is_false(f));
  testrun(!ov_item_is_false(t));
  testrun(!ov_item_is_false(nbr));
  testrun(!ov_item_is_false(str));
  testrun(!ov_item_is_false(arr));
  testrun(!ov_item_is_false(obj));

  testrun(NULL == ov_item_free(n));
  testrun(NULL == ov_item_free(f));
  testrun(NULL == ov_item_free(t));
  testrun(NULL == ov_item_free(nbr));
  testrun(NULL == ov_item_free(str));
  testrun(NULL == ov_item_free(arr));
  testrun(NULL == ov_item_free(obj));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_is_true() {

  ov_item *n = ov_item_null();
  ov_item *f = ov_item_false();
  ov_item *t = ov_item_true();
  ov_item *nbr = ov_item_number(0);
  ov_item *str = ov_item_string("test");
  ov_item *arr = ov_item_array();
  ov_item *obj = ov_item_object();

  testrun(!ov_item_is_true(n));
  testrun(!ov_item_is_true(f));
  testrun(ov_item_is_true(t));
  testrun(!ov_item_is_true(nbr));
  testrun(!ov_item_is_true(str));
  testrun(!ov_item_is_true(arr));
  testrun(!ov_item_is_true(obj));

  testrun(NULL == ov_item_free(n));
  testrun(NULL == ov_item_free(f));
  testrun(NULL == ov_item_free(t));
  testrun(NULL == ov_item_free(nbr));
  testrun(NULL == ov_item_free(str));
  testrun(NULL == ov_item_free(arr));
  testrun(NULL == ov_item_free(obj));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_object_set() {

  ov_item *n = ov_item_null();
  ov_item *f = ov_item_false();
  ov_item *t = ov_item_true();
  ov_item *nbr = ov_item_number(0);
  ov_item *str = ov_item_string("test");
  ov_item *arr = ov_item_array();
  ov_item *obj = ov_item_object();

  ov_item *val = ov_item_number(1234);

  testrun(!ov_item_object_set(n, "key", val));
  testrun(!ov_item_object_set(f, "key", val));
  testrun(!ov_item_object_set(t, "key", val));
  testrun(!ov_item_object_set(str, "key", val));
  testrun(!ov_item_object_set(nbr, "key", val));
  testrun(!ov_item_object_set(arr, "key", val));
  testrun(ov_item_object_set(obj, "key", val));
  testrun(1 == ov_item_count(obj));
  testrun(1234 ==
          ov_item_get_number((ov_item *)ov_item_object_get(obj, "key")));

  testrun(NULL == ov_item_free(n));
  testrun(NULL == ov_item_free(f));
  testrun(NULL == ov_item_free(t));
  testrun(NULL == ov_item_free(nbr));
  testrun(NULL == ov_item_free(str));
  testrun(NULL == ov_item_free(arr));
  testrun(NULL == ov_item_free(obj));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_object_delete() {

  ov_item *item = ov_item_object();
  ov_item *obj = ov_item_object();

  ov_item *val = ov_item_number(1234);

  testrun(ov_item_object_set(item, "1", val));
  testrun(ov_item_object_set(item, "2", obj));
  testrun(2 == ov_item_count(item));

  testrun(ov_item_object_delete(item, "2"));
  testrun(1 == ov_item_count(item));

  testrun(ov_item_object_delete(item, "1"));
  testrun(0 == ov_item_count(item));

  testrun(NULL == ov_item_free(item));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_object_remove() {

  ov_item *item = ov_item_object();
  ov_item *obj = ov_item_object();

  ov_item *val = ov_item_number(1234);

  testrun(ov_item_object_set(item, "1", val));
  testrun(ov_item_object_set(item, "2", obj));
  testrun(2 == ov_item_count(item));

  testrun(obj == ov_item_object_remove(item, "2"));
  testrun(1 == ov_item_count(item));

  testrun(val == ov_item_object_remove(item, "1"));
  testrun(0 == ov_item_count(item));

  testrun(NULL == ov_item_free(item));
  testrun(NULL == ov_item_free(val));
  testrun(NULL == ov_item_free(obj));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_object_get() {

  ov_item *item = ov_item_object();
  ov_item *obj = ov_item_object();

  ov_item *val = ov_item_number(1234);

  testrun(ov_item_object_set(item, "1", val));
  testrun(ov_item_object_set(item, "2", obj));
  testrun(2 == ov_item_count(item));

  testrun(obj == ov_item_object_get(item, "2"));
  testrun(2 == ov_item_count(item));

  testrun(val == ov_item_object_get(item, "1"));
  testrun(2 == ov_item_count(item));

  testrun(NULL == ov_item_free(item));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool clear_subitem(const char *key, ov_item const *val, void *userdata) {

  if (!key)
    return true;
  ov_item_clear((ov_item *)val);
  UNUSED(userdata);
  return true;
}

/*----------------------------------------------------------------------------*/

int test_ov_item_object_for_each() {

  ov_item *item = ov_item_object();
  ov_item *obj = ov_item_object();
  ov_item *val = ov_item_number(1234);

  testrun(ov_item_object_set(item, "1", val));
  testrun(ov_item_object_set(item, "2", obj));

  testrun(2 == ov_item_count(item));

  testrun(ov_item_object_for_each(item, clear_subitem, item));

  testrun(2 == ov_item_count(item));

  testrun(obj == ov_item_object_get(item, "2"));
  testrun(2 == ov_item_count(item));
  testrun(ov_item_is_null(obj));

  testrun(val == ov_item_object_get(item, "1"));
  testrun(2 == ov_item_count(item));
  testrun(ov_item_is_null(val));

  testrun(NULL == ov_item_free(item));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_array_get() {

  ov_item *item = ov_item_array();
  ov_item *obj = ov_item_object();
  ov_item *val = ov_item_number(1234);

  testrun(ov_item_array_push(item, obj));
  testrun(ov_item_array_push(item, val));

  testrun(obj == ov_item_array_get(item, 1));
  testrun(val == ov_item_array_get(item, 2));
  testrun(NULL == ov_item_array_get(item, 3));

  testrun(NULL == ov_item_free(item));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_array_set() {

  ov_item *item = ov_item_array();
  ov_item *obj = ov_item_object();
  ov_item *val = ov_item_number(1234);

  testrun(ov_item_array_push(item, obj));
  testrun(ov_item_array_push(item, val));

  testrun(ov_item_array_set(item, 1, ov_item_null()));
  testrun(ov_item_is_null(ov_item_array_get(item, 1)));
  testrun(val == ov_item_array_get(item, 2));
  testrun(NULL == ov_item_array_get(item, 3));
  testrun(NULL == ov_item_array_get(item, 4));

  testrun(ov_item_array_set(item, 3, ov_item_null()));
  testrun(ov_item_is_null(ov_item_array_get(item, 1)));
  testrun(val == ov_item_array_get(item, 2));
  testrun(ov_item_is_null(ov_item_array_get(item, 3)));
  testrun(NULL == ov_item_array_get(item, 4));
  testrun(NULL == ov_item_array_get(item, 5));
  testrun(NULL == ov_item_array_get(item, 6));
  testrun(NULL == ov_item_array_get(item, 7));
  testrun(ov_item_array_set(item, 6, ov_item_true()));
  testrun(ov_item_is_null(ov_item_array_get(item, 1)));
  testrun(val == ov_item_array_get(item, 2));
  testrun(ov_item_is_null(ov_item_array_get(item, 3)));
  testrun(NULL == ov_item_array_get(item, 4));
  testrun(NULL == ov_item_array_get(item, 5));
  testrun(ov_item_is_true(ov_item_array_get(item, 6)));
  testrun(NULL == ov_item_array_get(item, 7));

  testrun(NULL == ov_item_free(item));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_array_push() {

  ov_item *item = ov_item_array();
  ov_item *obj = ov_item_object();
  ov_item *val = ov_item_number(1234);

  testrun(ov_item_array_push(item, obj));
  testrun(ov_item_array_push(item, val));

  testrun(obj == ov_item_array_get(item, 1));
  testrun(val == ov_item_array_get(item, 2));
  testrun(NULL == ov_item_array_get(item, 3));

  testrun(NULL == ov_item_free(item));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_array_stack_pop() {

  ov_item *item = ov_item_array();
  ov_item *obj = ov_item_object();
  ov_item *val = ov_item_number(1234);

  testrun(ov_item_array_push(item, obj));
  testrun(ov_item_array_push(item, val));

  testrun(val == ov_item_array_stack_pop(item));
  testrun(obj == ov_item_array_stack_pop(item));
  testrun(NULL == ov_item_array_stack_pop(item));

  testrun(NULL == ov_item_free(item));
  testrun(NULL == ov_item_free(val));
  testrun(NULL == ov_item_free(obj));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_array_queue_pop() {

  ov_item *item = ov_item_array();
  ov_item *obj = ov_item_object();
  ov_item *val = ov_item_number(1234);

  testrun(ov_item_array_push(item, obj));
  testrun(ov_item_array_push(item, val));

  testrun(obj == ov_item_array_queue_pop(item));
  testrun(val == ov_item_array_queue_pop(item));
  testrun(NULL == ov_item_array_queue_pop(item));

  testrun(NULL == ov_item_free(item));
  testrun(NULL == ov_item_free(val));
  testrun(NULL == ov_item_free(obj));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_get_string() {

  ov_item *item = ov_item_string("test");
  testrun(0 == ov_string_compare("test", ov_item_get_string(item)));
  testrun(NULL == ov_item_free(item));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_get_number() {

  ov_item *item = ov_item_number(1.001);
  testrun(1.001 == ov_item_get_number(item));
  testrun(NULL == ov_item_free(item));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_get_int() {

  ov_item *item = ov_item_number(1.001);
  testrun(1 == ov_item_get_int(item));
  testrun(NULL == ov_item_free(item));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_pointer_parse_token() {

  uint64_t size = 100;
  char buffer[size];
  memset(buffer, '\0', size);

  memcpy(buffer, "/abc", 4);

  char *token = NULL;
  size_t length = 0;

  testrun(!pointer_parse_token(NULL, 0, NULL, NULL));
  testrun(!pointer_parse_token(NULL, size, &token, &length));
  testrun(!pointer_parse_token(buffer, 0, &token, &length));
  testrun(!pointer_parse_token(buffer, size, NULL, &length));
  testrun(!pointer_parse_token(buffer, size, &token, NULL));
  testrun(pointer_parse_token(buffer, 1, &token, &length));

  // empty token
  testrun(pointer_parse_token(buffer, 1, &token, &length));
  testrun(length == 0);
  testrun(token[0] == '/');

  testrun(pointer_parse_token(buffer, 2, &token, &length));
  testrun(length == 1);
  testrun(token[0] == 'a');
  testrun(token[length - 1] == 'a');

  testrun(pointer_parse_token(buffer, 3, &token, &length));
  testrun(length == 2);
  testrun(token[0] == 'a');
  testrun(token[length - 1] == 'b');

  testrun(pointer_parse_token(buffer, 4, &token, &length));
  testrun(length == 3);
  testrun(token[0] == 'a');
  testrun(token[length - 1] == 'c');

  testrun(pointer_parse_token(buffer, 5, &token, &length));
  testrun(length == 3);
  testrun(token[0] == 'a');
  testrun(token[length - 1] == 'c');

  testrun(pointer_parse_token(buffer, 100, &token, &length));
  testrun(length == 3);
  testrun(token[0] == 'a');
  testrun(token[length - 1] == 'c');

  //              0123456789
  memcpy(buffer, "/123/45/6/", 10);

  testrun(pointer_parse_token(buffer, 2, &token, &length));
  testrun(length == 1);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '1');

  testrun(pointer_parse_token(buffer, 3, &token, &length));
  testrun(length == 2);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '2');

  testrun(pointer_parse_token(buffer, 4, &token, &length));
  testrun(length == 3);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '3');

  testrun(pointer_parse_token(buffer, 5, &token, &length));
  testrun(length == 3);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '3');

  testrun(pointer_parse_token(buffer, 100, &token, &length));
  testrun(length == 3);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '3');

  // not starting with '/'
  testrun(!pointer_parse_token(buffer + 1, 2, &token, &length));
  testrun(!pointer_parse_token(buffer + 2, 2, &token, &length));
  testrun(!pointer_parse_token(buffer + 3, 2, &token, &length));

  // check second pointer
  testrun(pointer_parse_token(buffer + 4, 2, &token, &length));
  testrun(length == 1);
  testrun(token[0] == '4');
  testrun(token[length - 1] == '4');
  testrun(pointer_parse_token(buffer + 4, 3, &token, &length));
  testrun(length == 2);
  testrun(token[0] == '4');
  testrun(token[length - 1] == '5');
  testrun(pointer_parse_token(buffer + 4, 1000, &token, &length));
  testrun(length == 2);
  testrun(token[0] == '4');
  testrun(token[length - 1] == '5');

  // check third pointer
  testrun(!pointer_parse_token(buffer + 5, 2, &token, &length));
  testrun(!pointer_parse_token(buffer + 6, 2, &token, &length));
  testrun(pointer_parse_token(buffer + 7, 2, &token, &length));
  testrun(length == 1);
  testrun(token[0] == '6');
  testrun(token[length - 1] == '6');
  testrun(pointer_parse_token(buffer + 7, 3, &token, &length));
  testrun(length == 1);
  testrun(token[0] == '6');
  testrun(token[length - 1] == '6');
  testrun(pointer_parse_token(buffer + 7, 1000, &token, &length));
  testrun(length == 1);
  testrun(token[0] == '6');
  testrun(token[length - 1] == '6');

  // check empty followed by null pointer
  testrun(!pointer_parse_token(buffer + 10, 2, &token, &length));
  testrun(!pointer_parse_token(buffer + 11, 2, &token, &length));
  testrun(!pointer_parse_token(buffer + 12, 2, &token, &length));

  //              0123456789
  memcpy(buffer, "/1~0/45/6/", 10);
  testrun(pointer_parse_token(buffer, 2, &token, &length));
  testrun(length == 1);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '1');

  testrun(pointer_parse_token(buffer, 3, &token, &length));
  testrun(length == 2);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '~');

  testrun(pointer_parse_token(buffer, 4, &token, &length));
  testrun(length == 3);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '0');

  testrun(pointer_parse_token(buffer, 5, &token, &length));
  testrun(length == 3);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '0');

  testrun(pointer_parse_token(buffer, 100, &token, &length));
  testrun(length == 3);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '0');

  //              0123456789
  memcpy(buffer, "/1~1/45/6/", 10);
  testrun(pointer_parse_token(buffer, 2, &token, &length));
  testrun(length == 1);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '1');

  testrun(pointer_parse_token(buffer, 3, &token, &length));
  testrun(length == 2);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '~');

  testrun(pointer_parse_token(buffer, 4, &token, &length));
  testrun(length == 3);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '1');

  testrun(pointer_parse_token(buffer, 5, &token, &length));
  testrun(length == 3);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '1');

  testrun(pointer_parse_token(buffer, 100, &token, &length));
  testrun(length == 3);
  testrun(token[0] == '1');
  testrun(token[length - 1] == '1');

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_pointer_replace_special_encoding() {

  uint64_t size = 100;
  char buffer1[size];
  char buffer2[size];
  memset(buffer1, '\0', size);
  memset(buffer2, '\0', size);

  memcpy(buffer1, "123abcdefg", 10);

  char *result = buffer2;

  testrun(!pointer_replace_special_encoding(NULL, size, buffer1, "abc", "xyz"));

  testrun(!pointer_replace_special_encoding(result, size, NULL, "abc", "xyz"));

  testrun(
      !pointer_replace_special_encoding(result, size, buffer1, NULL, "xyz"));

  testrun(
      !pointer_replace_special_encoding(result, size, buffer1, "abc", NULL));

  testrun(!pointer_replace_special_encoding(result, 0, buffer1, "abc", "xyz"));

  testrun(strlen(buffer2) == 0);

  // replace with same length
  testrun(
      pointer_replace_special_encoding(result, size, buffer1, "abc", "xyz"));

  testrun(strlen(buffer2) == strlen(buffer1));
  testrun(strncmp("123xyzdefg", buffer2, strlen(buffer2)) == 0);
  memset(buffer2, '\0', size);

  // replace with shorter length
  memset(buffer2, '\0', size);
  testrun(pointer_replace_special_encoding(result, size, buffer1, "abc", "x"));

  testrun(strlen(buffer2) == strlen(buffer1) - 2);
  testrun(strncmp("123xdefg", buffer2, strlen(buffer2)) == 0);

  // replace first
  memset(buffer2, '\0', size);
  testrun(pointer_replace_special_encoding(result, size, buffer1, "1", "x"));

  testrun(strlen(buffer2) == strlen(buffer1));
  testrun(strncmp("x23abcdefg", buffer2, strlen(buffer2)) == 0);

  // replace last
  memset(buffer2, '\0', size);
  testrun(pointer_replace_special_encoding(result, size, buffer1, "g", "x"));

  testrun(strlen(buffer2) == strlen(buffer1));
  testrun(strncmp("123abcdefx", buffer2, strlen(buffer2)) == 0);

  // replace (not included)
  memset(buffer2, '\0', size);
  testrun(pointer_replace_special_encoding(result, size, buffer1, "vvv", "x"));

  testrun(strlen(buffer2) == strlen(buffer1));
  testrun(strncmp("123abcdefg", buffer2, strlen(buffer2)) == 0);

  // replace all
  memset(buffer2, '\0', size);
  testrun(pointer_replace_special_encoding(result, size, buffer1, "123abcdefg",
                                           "x"));

  testrun(strlen(buffer2) == 1);
  testrun(strncmp("x", buffer2, strlen(buffer2)) == 0);

  // replace up to length
  memset(buffer2, '\0', size);
  testrun(pointer_replace_special_encoding(result, 9, buffer1, "abc", "xyz"));
  testrun(strlen(buffer2) == 9);
  testrun(strncmp("123xyzdef", buffer2, strlen(buffer2)) == 0);

  memset(buffer2, '\0', size);
  testrun(pointer_replace_special_encoding(result, 8, buffer1, "abc", "xyz"));
  testrun(strlen(buffer2) == 8);
  testrun(strncmp("123xyzde", buffer2, strlen(buffer2)) == 0);

  memset(buffer2, '\0', size);
  testrun(pointer_replace_special_encoding(result, 7, buffer1, "abc", "xyz"));
  testrun(strlen(buffer2) == 7);
  testrun(strncmp("123xyzd", buffer2, strlen(buffer2)) == 0);

  memset(buffer2, '\0', size);
  testrun(pointer_replace_special_encoding(result, 6, buffer1, "abc", "xyz"));
  testrun(strlen(buffer2) == 6);
  testrun(strncmp("123xyz", buffer2, strlen(buffer2)) == 0);

  memset(buffer2, '\0', size);
  testrun(!pointer_replace_special_encoding(result, 5, buffer1, "abc", "xyz"));
  testrun(strlen(buffer2) == 0);

  memcpy(buffer1, "/123/abc/x", 10);

  memset(buffer2, '\0', size);
  testrun(pointer_replace_special_encoding(result, 10, buffer1, "~1", "/"));
  testrun(strlen(buffer2) == 10);
  testrun(strncmp("/123/abc/x", buffer2, strlen(buffer2)) == 0);

  memset(buffer2, '\0', size);
  testrun(pointer_replace_special_encoding(result, 4, buffer1, "~1", "/"));
  testrun(strlen(buffer2) == 4);
  testrun(strncmp("/123", buffer2, strlen(buffer2)) == 0);

  memcpy(buffer1, "12~1abc", 7);
  memset(buffer2, '\0', size);
  testrun(pointer_replace_special_encoding(result, 7, buffer1, "~1", "/"));
  testrun(strlen(buffer2) == 6);
  testrun(strncmp("12/abc", buffer2, strlen(buffer2)) == 0);

  memcpy(buffer1, "12~0abc", 7);
  memset(buffer2, '\0', size);
  testrun(pointer_replace_special_encoding(result, 7, buffer1, "~0", "~"));
  testrun(strlen(buffer2) == 6);
  testrun(strncmp("12~abc", buffer2, strlen(buffer2)) == 0);

  memcpy(buffer1, "12~3abc", 7);
  memset(buffer2, '\0', size);
  testrun(pointer_replace_special_encoding(result, 7, buffer1, "~0", "~"));
  testrun(strlen(buffer2) == 7);
  testrun(strncmp("12~3abc", buffer2, strlen(buffer2)) == 0);

  memcpy(buffer1, "12~~0ab", 7);
  memset(buffer2, '\0', size);
  testrun(pointer_replace_special_encoding(result, 7, buffer1, "~0", "~"));
  testrun(strlen(buffer2) == 6);
  testrun(strncmp("12~~ab", buffer2, strlen(buffer2)) == 0);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_pointer_escape_token() {

  uint64_t size = 100;
  char buffer1[size];
  char buffer2[size];
  memset(buffer1, '\0', size);
  memset(buffer2, '\0', size);

  memcpy(buffer1, "123abcdefg", 10);

  char *result = buffer2;

  testrun(!pointer_escape_token(NULL, size, result, size));

  testrun(!pointer_escape_token(buffer1, 0, result, size));

  testrun(!pointer_escape_token(buffer1, size, NULL, size));

  testrun(!pointer_escape_token(buffer1, size, result, size - 1));

  // tokens not contained
  testrun(pointer_escape_token(buffer1, size, result, size));

  // log("buffer1 |%s|\nbuffer2 |%s|", buffer1, buffer2);
  testrun(strlen(buffer2) == strlen(buffer1));
  testrun(strncmp("123abcdefg", buffer2, strlen(buffer2)) == 0);

  // ~0 contained
  memcpy(buffer1, "123~0cdefg", 10);
  memset(buffer2, '\0', size);

  testrun(pointer_escape_token(buffer1, 10, result, size));
  testrun(strlen(buffer2) == strlen(buffer1) - 1);
  testrun(strncmp("123~cdefg", buffer2, strlen(buffer2)) == 0);

  // ~1 contained
  memcpy(buffer1, "123~1cdefg", 10);
  memset(buffer2, '\0', size);

  testrun(pointer_escape_token(buffer1, size, result, size));
  testrun(strlen(buffer2) == strlen(buffer1) - 1);
  testrun(strncmp("123\\cdefg", buffer2, strlen(buffer2)) == 0);

  // replacement order
  memcpy(buffer1, "123~01defg", 10);
  memset(buffer2, '\0', size);
  testrun(pointer_escape_token(buffer1, size, result, size));
  testrun(strlen(buffer2) == strlen(buffer1) - 1);
  testrun(strncmp("123~1defg", buffer2, strlen(buffer2)) == 0);

  // multiple replacements
  memcpy(buffer1, "~0~0~01d~0g", 11);
  memset(buffer2, '\0', size);
  testrun(pointer_escape_token(buffer1, 11, result, size));
  testrun(strlen(buffer2) == strlen(buffer1) - 4);
  testrun(strncmp("~~~1d~g", buffer2, strlen(buffer2)) == 0);

  // multiple replacements
  memcpy(buffer1, "~0~1~01d~1g", 11);
  memset(buffer2, '\0', size);
  testrun(pointer_escape_token(buffer1, size, result, size));
  testrun(strlen(buffer2) == strlen(buffer1) - 4);
  testrun(strncmp("~\\~1d\\g", buffer2, strlen(buffer2)) == 0);

  // multiple replacements
  memcpy(buffer1, "array/5", 7);
  memset(buffer2, '\0', size);
  testrun(pointer_escape_token(buffer1, 5, result, size));
  testrun(strlen(buffer2) == 5);
  testrun(strncmp("array", buffer2, strlen(buffer2)) == 0);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_pointer_get_token_in_parent() {

  ov_item *v1 = NULL;
  ov_item *v2 = NULL;
  ov_item *v3 = NULL;
  ov_item *v4 = NULL;
  ov_item *v0 = NULL;
  ov_item *v5 = NULL;
  ov_item *v6 = NULL;
  ov_item *v7 = NULL;
  ov_item *v8 = NULL;
  ov_item *v9 = NULL;

  ov_item *result = NULL;
  ov_item *val = NULL;
  ov_item *value = ov_item_object();

  v1 = ov_item_true();
  ov_item_object_set(value, "key", v1);

  uint64_t size = 100;
  char buffer1[size];
  char buffer2[size];
  memset(buffer1, '\0', size);
  memset(buffer2, '\0', size);

  strcpy(buffer1, "key");

  testrun(!pointer_get_token_in_parent(NULL, NULL, 0));
  testrun(!pointer_get_token_in_parent(value, NULL, 3));

  // empty key is value
  testrun(value == pointer_get_token_in_parent(value, "key", 0));

  val = ov_item_object_get(value, "key");
  result = pointer_get_token_in_parent(value, "key", 3);
  testrun(result);
  testrun(result->parent == value);
  testrun(result == val);

  // test empty object
  value = ov_item_free(value);
  value = ov_item_object();
  testrun(!pointer_get_token_in_parent(value, "key", 3));
  ov_item_object_set(value, "key", ov_item_true());
  val = ov_item_object_get(value, "key");
  result = pointer_get_token_in_parent(value, "key", 3);
  testrun(result);
  testrun(result->parent == value);
  testrun(result == val);

  // check key match
  testrun(!pointer_get_token_in_parent(value, "key", 2));
  testrun(!pointer_get_token_in_parent(value, "Key ", 3));
  testrun(!pointer_get_token_in_parent(value, "key ", 4));

  // add some members
  v1 = ov_item_true();
  testrun(ov_item_object_set(value, "1", v1));
  v2 = ov_item_true();
  testrun(ov_item_object_set(value, "2", v2));
  v3 = ov_item_true();
  testrun(ov_item_object_set(value, "3", v3));
  v4 = ov_item_true();
  testrun(ov_item_object_set(value, "4", v4));

  result = pointer_get_token_in_parent(value, "1", 1);
  testrun(result);
  testrun(result->parent == value);
  testrun(result == v1);

  result = pointer_get_token_in_parent(value, "2", 1);
  testrun(result);
  testrun(result->parent == value);
  testrun(result == v2);

  result = pointer_get_token_in_parent(value, "3", 1);
  testrun(result);
  testrun(result->parent == value);
  testrun(result == v3);

  result = pointer_get_token_in_parent(value, "4", 1);
  testrun(result);
  testrun(result->parent == value);
  testrun(result == v4);
  value = ov_item_free(value);

  value = ov_item_array();
  testrun(0 == ov_item_count(value));
  testrun(!pointer_get_token_in_parent(value, "1", 1));
  testrun(!pointer_get_token_in_parent(value, "2", 1));
  testrun(!pointer_get_token_in_parent(value, "3", 1));
  v0 = pointer_get_token_in_parent(value, "-", 1);
  testrun(1 == ov_item_count(value));

  v1 = ov_item_true();
  testrun(ov_item_array_push(value, v1)) v2 = ov_item_true();
  testrun(ov_item_array_push(value, v2)) v3 = ov_item_true();
  testrun(ov_item_array_push(value, v3)) v4 = ov_item_true();
  testrun(ov_item_array_push(value, v4));

  result = pointer_get_token_in_parent(value, "0", 1);
  testrun(result == v0);
  result = pointer_get_token_in_parent(value, "1", 1);
  testrun(result == v1);
  result = pointer_get_token_in_parent(value, "2", 1);
  testrun(result == v2);
  result = pointer_get_token_in_parent(value, "3", 1);
  testrun(result == v3);
  result = pointer_get_token_in_parent(value, "4", 1);
  testrun(result == v4);
  testrun(5 == ov_item_count(value));

  // add a new array item
  result = pointer_get_token_in_parent(value, "-", 1);
  testrun(6 == ov_item_count(value));
  testrun(result->parent == value);

  // get with leading zero
  result = pointer_get_token_in_parent(value, "00", 2);
  testrun(result == v0);
  result = pointer_get_token_in_parent(value, "01", 2);
  testrun(result == v1);
  result = pointer_get_token_in_parent(value, "02", 2);
  testrun(result == v2);
  result = pointer_get_token_in_parent(value, "03", 2);
  testrun(result == v3);
  result = pointer_get_token_in_parent(value, "04", 2);
  testrun(result == v4);

  // not array or object
  testrun(!pointer_get_token_in_parent(v1, "3", 1));
  value = ov_item_free(value);

  value = ov_item_object();
  testrun(!pointer_get_token_in_parent(value, "foo", 3));
  testrun(ov_item_object_set(value, "foo", ov_item_true()));
  result = pointer_get_token_in_parent(value, "foo", 3);
  testrun(result);
  testrun(result->parent == value);
  testrun(result == ov_item_object_get(value, "foo"));

  v1 = ov_item_true();
  v2 = ov_item_true();
  v3 = ov_item_true();
  v4 = ov_item_true();
  v5 = ov_item_true();
  v6 = ov_item_true();
  v7 = ov_item_true();
  v8 = ov_item_true();
  v9 = ov_item_true();
  testrun(ov_item_object_set(value, "0", v1));
  testrun(ov_item_object_set(value, "a/b", v2));
  testrun(ov_item_object_set(value, "c%d", v3));
  testrun(ov_item_object_set(value, "e^f", v4));
  testrun(ov_item_object_set(value, "g|h", v5));
  testrun(ov_item_object_set(value, "i\\\\j", v6));
  testrun(ov_item_object_set(value, "k\"l", v7));
  testrun(ov_item_object_set(value, " ", v8));
  testrun(ov_item_object_set(value, "m~n", v9));

  result = pointer_get_token_in_parent(value, "", 0);
  testrun(result == value);
  result = pointer_get_token_in_parent(value, "0", 1);
  testrun(result == v1);
  result = pointer_get_token_in_parent(value, "a/b", 3);
  testrun(result == v2);
  result = pointer_get_token_in_parent(value, "c%d", 3);
  testrun(result == v3);
  result = pointer_get_token_in_parent(value, "e^f", 3);
  testrun(result == v4);
  result = pointer_get_token_in_parent(value, "g|h", 3);
  testrun(result == v5);
  result = pointer_get_token_in_parent(value, "i\\\\j", 4);
  testrun(result == v6);
  result = pointer_get_token_in_parent(value, "k\"l", 3);
  testrun(result == v7);
  result = pointer_get_token_in_parent(value, " ", 1);
  testrun(result == v8);
  result = pointer_get_token_in_parent(value, "m~n", 3);
  testrun(result == v9);

  value = ov_item_free(value);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_get() {

  ov_item *array = NULL;
  ov_item *object = NULL;
  ov_item *v0 = NULL;
  ov_item *v1 = NULL;
  ov_item *v2 = NULL;
  ov_item *v3 = NULL;
  ov_item *v4 = NULL;
  ov_item *v5 = NULL;

  ov_item const *result = NULL;
  ov_item *value = ov_item_object();
  v0 = ov_item_true();
  ov_item_object_set(value, "key", v0);

  uint64_t size = 100;
  char buffer1[size];
  memset(buffer1, '\0', size);

  strcpy(buffer1, "key");

  testrun(!ov_item_get(NULL, NULL));
  testrun(value == ov_item_get(value, NULL));
  testrun(!ov_item_get(NULL, "key"));

  // empty key is value
  testrun(value == ov_item_get(value, ""));
  testrun(!ov_item_get(value, "/"));
  testrun(!ov_item_get(value, "/k"));
  testrun(!ov_item_get(value, "/ke"));
  testrun(v0 == ov_item_get(value, "/key"));

  // key not contained
  testrun(!ov_item_get(value, "1"));

  // add some childs
  array = ov_item_array();
  ov_item_object_set(value, "array", array);
  object = ov_item_object();
  ov_item_object_set(value, "object", object);
  v1 = ov_item_true();
  ov_item_object_set(value, "1", v1);
  v2 = ov_item_false();
  ov_item_object_set(value, "2", v2);

  // add some child to the array
  v3 = ov_item_true();
  v4 = ov_item_true();
  v5 = ov_item_object();
  ov_item_array_push(array, v3);
  ov_item_array_push(array, v4);
  ov_item_array_push(array, v5);

  // add child to object in array
  ov_item_object_set(v5, "5", ov_item_array());
  ov_item_object_set(v5, "key6", ov_item_object());
  ov_item_object_set(v5, "key7", ov_item_object());

  // add child to object
  ov_item_object_set(object, "8", ov_item_object());

  // positive testing

  testrun(value == ov_item_get(value, ""));
  testrun(v0 == ov_item_get(value, "/key"));
  testrun(array == ov_item_get(value, "/array"));
  testrun(object == ov_item_get(value, "/object"));

  testrun(ov_item_get(value, "/key"));
  testrun(ov_item_get(value, "/array"));
  testrun(ov_item_get(value, "/object"));
  testrun(!ov_item_get(value, "/key "));
  testrun(!ov_item_get(value, "/array "));
  testrun(!ov_item_get(value, "/object "));

  testrun(v3 == ov_item_get(value, "/array/0"));
  testrun(v4 == ov_item_get(value, "/array/1"));
  testrun(v5 == ov_item_get(value, "/array/2"));

  testrun(ov_item_get(value, "/array/2/5"));
  testrun(ov_item_get(value, "/array/2/key6"));
  testrun(ov_item_get(value, "/array/2/key7"));

  testrun(object == ov_item_get(value, "/object"));
  testrun(ov_item_get(value, "/object/8"));

  testrun(v1 == ov_item_get(value, "/1"));
  testrun(v2 == ov_item_get(value, "/2"));

  // add new array member
  testrun(3 == ov_item_count(array));
  result = ov_item_get(value, "/array/-");
  testrun(result);
  testrun(4 == ov_item_count(array));

  // try to add new object member
  testrun(!ov_item_get(value, "/-"));

  // key not contained
  testrun(!ov_item_get(value, "/objectX"));
  testrun(!ov_item_get(value, "/obj"));
  testrun(!ov_item_get(value, "/array/2/key1"));
  testrun(!ov_item_get(value, "/array/2/k"));

  // out of index
  testrun(4 == ov_item_count(array));
  testrun(ov_item_get(value, "/array/0"));
  testrun(ov_item_get(value, "/array/1"));
  testrun(ov_item_get(value, "/array/2"));
  testrun(ov_item_get(value, "/array/3"));
  testrun(!ov_item_get(value, "/array/4"));
  testrun(!ov_item_get(value, "/array/5"));
  testrun(!ov_item_get(value, "/array/6"));
  testrun(!ov_item_get(value, "/array/7"));

  // not an index
  testrun(array == ov_item_get(value, "/array/"));
  testrun(array == ov_item_get(value, "/array/"));
  testrun(array == ov_item_get(value, "/array/"));

  // not an object
  testrun(object == ov_item_get(value, "/object"));
  testrun(object == ov_item_get(value, "/object/"));
  testrun(!ov_item_get(value, "/object/ "));
  testrun(!ov_item_get(value, "/objec "));

  // no value to return
  testrun(!ov_item_get(value, "/"));

  // no starting pointer slash
  testrun(!ov_item_get(value, "object"));
  testrun(!ov_item_get(value, "object/"));

  ov_item_free(value);

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
  testrun_test(test_ov_item_cast);

  testrun_test(test_ov_item_object);
  testrun_test(test_ov_item_array);
  testrun_test(test_ov_item_string);
  testrun_test(test_ov_item_number);
  testrun_test(test_ov_item_true);
  testrun_test(test_ov_item_false);
  testrun_test(test_ov_item_null);

  testrun_test(test_ov_item_free);
  testrun_test(test_ov_item_clear);
  testrun_test(test_ov_item_copy);
  testrun_test(test_ov_item_dump);

  testrun_test(test_ov_item_count);

  testrun_test(test_ov_item_is_object);
  testrun_test(test_ov_item_is_array);
  testrun_test(test_ov_item_is_string);
  testrun_test(test_ov_item_is_number);
  testrun_test(test_ov_item_is_null);
  testrun_test(test_ov_item_is_true);
  testrun_test(test_ov_item_is_false);

  testrun_test(test_ov_item_object_set);
  testrun_test(test_ov_item_object_delete);
  testrun_test(test_ov_item_object_remove);
  testrun_test(test_ov_item_object_get);
  testrun_test(test_ov_item_object_for_each);

  testrun_test(test_ov_item_array_get);
  testrun_test(test_ov_item_array_set);
  testrun_test(test_ov_item_array_push);
  testrun_test(test_ov_item_array_stack_pop);
  testrun_test(test_ov_item_array_queue_pop);

  testrun_test(test_ov_item_get_string);
  testrun_test(test_ov_item_get_number);
  testrun_test(test_ov_item_get_int);

  testrun_test(check_pointer_parse_token);
  testrun_test(check_pointer_replace_special_encoding);
  testrun_test(check_pointer_escape_token);
  testrun_test(check_pointer_get_token_in_parent);

  testrun_test(test_ov_item_get);

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
