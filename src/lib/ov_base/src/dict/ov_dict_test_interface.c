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
        @file           ov_dict_test_interface.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-08-15

        @ingroup        ov_basics

        @brief          Implementation of Interface tests for
                        ov_dict implementations.

        ------------------------------------------------------------------------
*/
#include "../../include/ov_dict_test_interface.h"
#include "../../include/ov_utils.h"

ov_dict *(*dict_creator)(ov_dict_config);

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_impl_dict_is_empty() {

  ov_dict *dict = dict_creator(ov_dict_string_key_config(1));
  testrun(dict);
  testrun(ov_dict_is_valid(dict));
  testrun(!dict->is_empty(NULL));
  testrun(dict->is_empty(dict));

  char *key = strdup("key");
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(!dict->is_empty(dict));
  testrun(dict->del(dict, key));
  testrun(dict->is_empty(dict));

  key = strdup("key1");
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(!dict->is_empty(dict));
  key = strdup("key2");
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(!dict->is_empty(dict));
  key = strdup("key3");
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(!dict->is_empty(dict));

  testrun(dict->del(dict, "key1"));
  testrun(!dict->is_empty(dict));
  testrun(dict->del(dict, "key2"));
  testrun(!dict->is_empty(dict));
  testrun(dict->del(dict, "key3"));
  testrun(dict->is_empty(dict));

  dict->free(dict);

  // check with same with multiple slots
  dict = dict_creator(ov_dict_string_key_config(100));
  testrun(dict);
  testrun(ov_dict_is_valid(dict));
  testrun(!dict->is_empty(NULL));
  testrun(dict->is_empty(dict));

  key = strdup("key1");
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(!dict->is_empty(dict));
  key = strdup("key2");
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(!dict->is_empty(dict));
  key = strdup("key3");
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(!dict->is_empty(dict));

  testrun(dict->del(dict, "key1"));
  testrun(!dict->is_empty(dict));
  testrun(dict->del(dict, "key2"));
  testrun(!dict->is_empty(dict));
  testrun(dict->del(dict, "key3"));
  testrun(dict->is_empty(dict));

  dict->free(dict);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_dict_clear() {

  char *key = NULL;
  ov_dict *dict = dict_creator(ov_dict_string_key_config(1));

  testrun(dict);
  testrun(ov_dict_is_valid(dict));
  testrun(dict->is_empty(dict));

  testrun(dict->clear(dict));
  testrun(dict->is_empty(dict));

  key = strdup("key1");
  testrun(dict->set(dict, key, NULL, NULL));
  key = strdup("key2");
  testrun(dict->set(dict, key, NULL, NULL));
  key = strdup("key3");
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(!dict->is_empty(dict));
  testrun(dict->clear(dict));
  testrun(dict->is_empty(dict));

  dict->free(dict);

  // check with same with multiple slots
  dict = dict_creator(ov_dict_string_key_config(100));
  testrun(dict);
  testrun(ov_dict_is_valid(dict));
  testrun(dict->is_empty(dict));

  testrun(dict->clear(dict));
  testrun(dict->is_empty(dict));

  key = strdup("key1");
  testrun(dict->set(dict, key, NULL, NULL));
  key = strdup("key2");
  testrun(dict->set(dict, key, NULL, NULL));
  key = strdup("key3");
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(!dict->is_empty(dict));
  testrun(dict->clear(dict));
  testrun(dict->is_empty(dict));

  dict->free(dict);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_dict_free() {

  char *key = NULL;
  ov_dict *dict = dict_creator(ov_dict_string_key_config(1));

  testrun(dict);
  testrun(ov_dict_is_valid(dict));
  testrun(dict->is_empty(dict));
  testrun(NULL == dict->free(dict));

  dict = dict_creator(ov_dict_string_key_config(1));
  testrun(dict);
  key = strdup("key1");
  testrun(dict->set(dict, key, NULL, NULL));
  key = strdup("key2");
  testrun(dict->set(dict, key, NULL, NULL));
  key = strdup("key3");
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(!dict->is_empty(dict));
  testrun(NULL == dict->free(dict));

  // check with same with multiple slots
  dict = dict_creator(ov_dict_string_key_config(100));
  testrun(dict);
  key = strdup("key1");
  testrun(dict->set(dict, key, NULL, NULL));
  key = strdup("key2");
  testrun(dict->set(dict, key, NULL, NULL));
  key = strdup("key3");
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(!dict->is_empty(dict));
  testrun(NULL == dict->free(dict));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_dict_get_keys() {

  ov_list *list = NULL;
  ov_dict *dict = dict_creator(ov_dict_string_key_config(1));

  char *key1 = strdup("key1");
  char *key2 = strdup("key2");
  char *key3 = strdup("key3");
  char *key4 = strdup("key4");
  char *key5 = strdup("key5");

  char *val1 = "val1";
  char *val2 = "val2";
  char *val3 = "val3";

  testrun(dict->get_keys);

  testrun(NULL == dict->get_keys(NULL, NULL));
  testrun(NULL == dict->get_keys(NULL, val1));

  list = dict->get_keys(dict, 0);

  testrun(0 != list);
  testrun(0 == list->count(list));

  list = list->free(list);
  testrun(0 == list);

  list = dict->get_keys(dict, val1);
  testrun(list);
  testrun(0 == list->count(list));
  list = list->free(list);

  testrun(dict->set(dict, key1, val1, NULL));
  testrun(dict->set(dict, key2, val2, NULL));
  testrun(dict->set(dict, key3, val3, NULL));

  list = dict->get_keys(dict, 0);

  testrun(3 == list->count(list));

  bool contained[3] = {0};

  for (size_t i = 0; i < list->count(list); ++i) {

    if (0 == strcmp(key1, list->get(list, i + 1))) {
      contained[0] = true;
    }
    if (0 == strcmp(key2, list->get(list, i + 1))) {
      contained[1] = true;
    }
    if (0 == strcmp(key3, list->get(list, i + 1))) {
      contained[2] = true;
    }
  }

  testrun(contained[0] && contained[1] && contained[2]);
  list = list->free(list);

  list = dict->get_keys(dict, val1);
  testrun(list);
  testrun(1 == list->count(list));
  testrun(key1 == list->get(list, 1));
  list = list->free(list);

  list = dict->get_keys(dict, val2);
  testrun(list);
  testrun(1 == list->count(list));
  testrun(key2 == list->get(list, 1));
  list = list->free(list);

  list = dict->get_keys(dict, val3);
  testrun(list);
  testrun(1 == list->count(list));
  testrun(key3 == list->get(list, 1));
  list = list->free(list);

  testrun(dict->set(dict, key4, val1, NULL));
  testrun(dict->set(dict, key5, val1, NULL));

  list = dict->get_keys(dict, val1);
  testrun(list);
  testrun(3 == list->count(list));
  testrun(0 < list->get_pos(list, key1));
  testrun(0 < list->get_pos(list, key4));
  testrun(0 < list->get_pos(list, key5));
  list = list->free(list);

  dict->free(dict);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_dict_get() {

  char *key1 = strdup("key1");
  char *key2 = strdup("key2");
  char *key3 = strdup("key3");
  char *key4 = strdup("key4");
  char *key5 = strdup("key5");

  char *val1 = "val1";
  char *val2 = "val2";
  char *val3 = "val3";
  char *val4 = "val4";
  char *val5 = "val5";

  ov_dict *dict = dict_creator(ov_dict_string_key_config(1));

  testrun(dict);
  testrun(ov_dict_is_valid(dict));
  testrun(dict->is_empty(dict));

  testrun(NULL == dict->get(NULL, NULL));
  testrun(NULL == dict->get(dict, NULL));
  testrun(NULL == dict->get(NULL, key1));
  testrun(NULL == dict->get(dict, key1));

  testrun(dict->set(dict, key5, val5, NULL));
  testrun(dict->set(dict, key1, val1, NULL));
  testrun(dict->set(dict, key3, val3, NULL));
  testrun(dict->set(dict, key4, val4, NULL));
  testrun(dict->set(dict, key2, val2, NULL));
  testrun(5 == ov_dict_count(dict));

  testrun(val1 == dict->get(dict, key1));
  testrun(val2 == dict->get(dict, key2));
  testrun(val3 == dict->get(dict, key3));
  testrun(val4 == dict->get(dict, key4));
  testrun(val5 == dict->get(dict, key5));
  testrun(NULL == dict->get(dict, NULL));

  // check override and input destroy on override!
  testrun(dict->set(dict, strdup("key2"), NULL, NULL));
  testrun(NULL == dict->get(dict, "key2"));
  testrun(ov_dict_is_set(dict, "key2"));

  dict->free(dict);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_dict_set() {

  char *key1 = strdup("key1");
  char *key2 = strdup("key2");
  char *key3 = strdup("key3");
  char *key4 = strdup("key4");
  char *key5 = strdup("key5");

  char *val1 = "val1";
  char *val2 = "val2";
  char *val3 = "val3";
  char *val4 = "val4";
  char *val5 = "val5";

  void *old = NULL;

  ov_dict *dict = dict_creator(ov_dict_string_key_config(1));

  testrun(dict);
  testrun(ov_dict_is_valid(dict));
  testrun(dict->is_empty(dict));

  testrun(!dict->set(NULL, NULL, NULL, NULL));
  testrun(!dict->set(NULL, key1, NULL, NULL));
  testrun(dict->set(dict, NULL, NULL, NULL));

  testrun(ov_dict_is_set(dict, NULL));

  testrun(dict->set(dict, key1, NULL, NULL));
  testrun(ov_dict_is_set(dict, key1));

  testrun(dict->set(dict, NULL, NULL, NULL));
  testrun(ov_dict_is_set(dict, NULL));

  // check override and input destroy on override!
  testrun(dict->set(dict, strdup("key1"), NULL, NULL));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(NULL == dict->get(dict, "key1"));
  testrun(dict->set(dict, strdup("key1"), val1, NULL));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(val1 == dict->get(dict, "key1"));

  // override key and value
  testrun(dict->set(dict, strdup("key1"), val2, &old));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(val1 == old);
  testrun(val2 == dict->get(dict, "key1"));

  // override key and value
  testrun(dict->set(dict, strdup("key1"), val1, &old));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(val2 == old);
  testrun(val1 == dict->get(dict, "key1"));

  // set multiple values
  testrun(dict->set(dict, key2, val2, NULL));
  testrun(dict->set(dict, key3, val3, NULL));
  testrun(dict->set(dict, key4, val4, NULL));
  testrun(dict->set(dict, key5, val5, NULL));
  testrun(val1 == dict->get(dict, "key1"));
  testrun(val2 == dict->get(dict, "key2"));
  testrun(val3 == dict->get(dict, "key3"));
  testrun(val4 == dict->get(dict, "key4"));
  testrun(val5 == dict->get(dict, "key5"));
  dict->free(dict);

  // check with multiple slot dict
  key1 = strdup("key1");
  key2 = strdup("key2");
  key3 = strdup("key3");
  key4 = strdup("key4");
  key5 = strdup("key5");

  dict = dict_creator(ov_dict_string_key_config(100));
  testrun(dict->set(dict, key1, NULL, NULL));
  testrun(ov_dict_is_set(dict, key1));

  // check override and input destroy on override!
  testrun(dict->set(dict, strdup("key1"), NULL, NULL));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(NULL == dict->get(dict, "key1"));
  testrun(dict->set(dict, strdup("key1"), val1, NULL));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(val1 == dict->get(dict, "key1"));

  // override key and value
  testrun(dict->set(dict, strdup("key1"), val2, &old));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(val1 == old);
  testrun(val2 == dict->get(dict, "key1"));

  // override key and value
  testrun(dict->set(dict, strdup("key1"), val1, &old));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(val2 == old);
  testrun(val1 == dict->get(dict, "key1"));

  // set multiple values
  testrun(dict->set(dict, key2, val2, NULL));
  testrun(dict->set(dict, key3, val3, NULL));
  testrun(dict->set(dict, key4, val4, NULL));
  testrun(dict->set(dict, key5, val5, NULL));
  testrun(val1 == dict->get(dict, "key1"));
  testrun(val2 == dict->get(dict, key2));
  testrun(val3 == dict->get(dict, key3));
  testrun(val4 == dict->get(dict, key4));
  testrun(val5 == dict->get(dict, key5));
  dict->free(dict);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_dict_del() {

  char *key1 = strdup("key1");
  char *key2 = strdup("key2");
  char *key3 = strdup("key3");

  char *val1 = strdup("val1");

  ov_dict *dict = dict_creator(ov_dict_string_key_config(1));

  testrun(dict);
  testrun(ov_dict_is_valid(dict));
  testrun(dict->is_empty(dict));

  testrun(!dict->del(NULL, NULL));
  testrun(dict->del(dict, NULL));
  testrun(!dict->del(NULL, key1));

  // key not contained
  testrun(!ov_dict_is_set(dict, key1));
  testrun(dict->del(dict, key1));
  testrun(!ov_dict_is_set(dict, key1));

  // key contained
  testrun(dict->set(dict, key1, NULL, NULL));
  testrun(ov_dict_is_set(dict, key1));
  testrun(dict->del(dict, key1));
  // string key1 MUST be freed!
  testrun(!ov_dict_is_set(dict, "key1"));

  // key with value
  testrun(dict->set(dict, key2, val1, NULL));
  testrun(ov_dict_is_set(dict, key2));
  testrun(dict->del(dict, key2));
  // string key2 MUST be freed!
  testrun(!ov_dict_is_set(dict, "key2"));
  // string val1 MUST be untouched (no value free configured)
  testrun(0 == strcmp(val1, "val1"));

  dict->config.value.data_function.free = ov_data_string_free;
  testrun(dict->set(dict, key3, val1, NULL));
  testrun(ov_dict_is_set(dict, key3));
  testrun(dict->del(dict, key3));
  // string key2 MUST be freed!
  testrun(!ov_dict_is_set(dict, "key2"));
  // string val1 MUST be freed (check valgrind run)

  dict->free(dict);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_dict_remove() {

  char *key1 = strdup("key1");
  char *key2 = strdup("key2");
  char *key3 = strdup("key3");

  char *val1 = strdup("val1");

  ov_dict *dict = dict_creator(ov_dict_string_key_config(1));

  testrun(dict);
  testrun(ov_dict_is_valid(dict));
  testrun(dict->is_empty(dict));

  testrun(!dict->remove(NULL, NULL));
  testrun(!dict->remove(dict, NULL));
  testrun(!dict->remove(NULL, key1));

  // key not contained
  testrun(!ov_dict_is_set(dict, key1));
  testrun(NULL == dict->remove(dict, key1));

  // key without value (MUST unset key)
  testrun(dict->set(dict, key1, NULL, NULL));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(NULL == dict->remove(dict, key1));
  testrun(!ov_dict_is_set(dict, "key1"));

  key1 = strdup("key1");
  // set value to 1 and 3
  testrun(dict->set(dict, key1, val1, NULL));
  testrun(dict->set(dict, key2, NULL, NULL));
  testrun(dict->set(dict, key3, val1, NULL));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(ov_dict_is_set(dict, "key2"));
  testrun(ov_dict_is_set(dict, "key3"));
  testrun(val1 == dict->remove(dict, key1));
  testrun(!ov_dict_is_set(dict, "key1"));
  testrun(ov_dict_is_set(dict, "key2"));
  testrun(ov_dict_is_set(dict, "key3"));
  testrun(NULL == dict->remove(dict, key2));
  testrun(!ov_dict_is_set(dict, "key1"));
  testrun(!ov_dict_is_set(dict, "key2"));
  testrun(ov_dict_is_set(dict, "key3"));
  testrun(val1 == dict->remove(dict, key3));
  testrun(!ov_dict_is_set(dict, "key1"));
  testrun(!ov_dict_is_set(dict, "key2"));
  testrun(!ov_dict_is_set(dict, "key3"));
  testrun(dict->is_empty(dict));
  dict->free(dict);

  // non overloaded buckets
  dict = dict_creator(ov_dict_string_key_config(100));
  key1 = strdup("key1");
  key2 = strdup("key2");
  key3 = strdup("key3");

  testrun(dict);
  testrun(ov_dict_is_valid(dict));
  testrun(dict->is_empty(dict));

  testrun(!dict->remove(NULL, NULL));
  testrun(!dict->remove(dict, NULL));
  testrun(!dict->remove(NULL, key1));

  // key not contained
  testrun(!ov_dict_is_set(dict, key1));
  testrun(NULL == dict->remove(dict, key1));

  // key without value (MUST unset key)
  testrun(dict->set(dict, key1, NULL, NULL));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(NULL == dict->remove(dict, key1));
  testrun(!ov_dict_is_set(dict, "key1"));

  key1 = strdup("key1");
  // set value to 1 and 3
  testrun(dict->set(dict, key1, val1, NULL));
  testrun(dict->set(dict, key2, NULL, NULL));
  testrun(dict->set(dict, key3, val1, NULL));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(ov_dict_is_set(dict, "key2"));
  testrun(ov_dict_is_set(dict, "key3"));
  testrun(val1 == dict->remove(dict, key1));
  testrun(!ov_dict_is_set(dict, "key1"));
  testrun(ov_dict_is_set(dict, "key2"));
  testrun(ov_dict_is_set(dict, "key3"));
  testrun(NULL == dict->remove(dict, key2));
  testrun(!ov_dict_is_set(dict, "key1"));
  testrun(!ov_dict_is_set(dict, "key2"));
  testrun(ov_dict_is_set(dict, "key3"));
  testrun(val1 == dict->remove(dict, key3));
  testrun(!ov_dict_is_set(dict, "key1"));
  testrun(!ov_dict_is_set(dict, "key2"));
  testrun(!ov_dict_is_set(dict, "key3"));
  testrun(dict->is_empty(dict));
  dict->free(dict);

  free(val1);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool for_each_set_x(const void *key, void *value, void *data) {

  if (!key)
    return true;

  if (!value)
    return false;

  if (data) { /* unused */
  };

  char *v = (char *)value;

  v[1] = 'X';

  return true;
}

/*----------------------------------------------------------------------------*/

#define PAIR_CHECKER_MAGIC_BYTES 0x2d1f4aaa

struct pair_checker_arg {

  uint32_t magic_bytes;

  size_t entries;
  char const **keys_expected;
  char const **values_expected;

  /* Results */

  bool key_found[10];
  bool value_found[10];

  bool unexpected_key_found;
  bool key_not_unique;
};

static bool pair_checker(const void *key, void *value, void *data) {

  UNUSED(value);

  struct pair_checker_arg *arg = data;
  OV_ASSERT(0 != arg);
  OV_ASSERT(PAIR_CHECKER_MAGIC_BYTES == arg->magic_bytes);

  ssize_t key_index = -1;

  for (size_t i = 0; i < arg->entries; ++i) {

    if (0 == strcmp(arg->keys_expected[i], key)) {

      key_index = i;
      break;
    }
  }

  if (0 > key_index) {

    testrun_log_error("Found unexpected key %s", (char const *)key);
    arg->unexpected_key_found = true;
    return false;
  }

  if (0 != strcmp(arg->values_expected[key_index], (char const *)value)) {

    testrun_log_error("Unexpected value found for key %s", (char const *)key);
    return false;
  }

  if (arg->key_found[key_index]) {

    testrun_log_error("Key %s found twice", (char const *)key);
    arg->value_found[key_index] = true;
    arg->key_not_unique = true;
    return false;
  }

  arg->key_found[key_index] = true;
  arg->value_found[key_index] = true;

  return true;
}

/*----------------------------------------------------------------------------*/

int test_impl_dict_for_each() {

  char const *keys[] = {"key1", "key2", "key3"};
  char const *values[] = {"val1", "val2", "val3"};

  char *key1 = strdup(keys[0]);
  char *key2 = strdup(keys[1]);
  char *key3 = strdup(keys[2]);

  char *val1 = strdup(values[0]);
  char *val2 = strdup(values[1]);
  char *val3 = strdup(values[2]);

  ov_dict *dict = dict_creator(ov_dict_string_key_config(1));

  testrun(!dict->for_each(NULL, NULL, NULL));
  testrun(!dict->for_each(dict, NULL, NULL));
  testrun(!dict->for_each(NULL, NULL, for_each_set_x));

  // dict without values

  struct pair_checker_arg arg = {

      .magic_bytes = PAIR_CHECKER_MAGIC_BYTES,
      .entries = 0,
      .keys_expected = keys,
      .values_expected = values,

  };

  testrun(ov_dict_for_each(dict, &arg, pair_checker));
  testrun(!arg.unexpected_key_found);
  testrun(!arg.key_not_unique);

  testrun(dict->for_each(dict, NULL, for_each_set_x));

  testrun(dict->set(dict, key1, val1, NULL));
  testrun(dict->set(dict, key2, val2, NULL));
  testrun(dict->set(dict, key3, val3, NULL));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(ov_dict_is_set(dict, "key2"));
  testrun(ov_dict_is_set(dict, "key3"));

  arg.entries = 3;

  testrun(ov_dict_for_each(dict, &arg, pair_checker));
  testrun(!arg.unexpected_key_found);
  testrun(!arg.key_not_unique);

  for (size_t i = 0; i < arg.entries; ++i) {

    testrun(arg.key_found[i]);
    testrun(arg.value_found[i]);
  }

  testrun(dict->for_each(dict, NULL, for_each_set_x));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(ov_dict_is_set(dict, "key2"));
  testrun(ov_dict_is_set(dict, "key3"));
  testrun(0 == strcmp("vXl1", dict->get(dict, "key1")));
  testrun(0 == strcmp("vXl2", dict->get(dict, "key2")));
  testrun(0 == strcmp("vXl3", dict->get(dict, "key3")));

  testrun(dict->for_each(dict, NULL, for_each_set_x));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(ov_dict_is_set(dict, "key2"));
  testrun(ov_dict_is_set(dict, "key3"));
  testrun(0 == strcmp("vXl1", dict->get(dict, "key1")));
  testrun(0 == strcmp("vXl2", dict->get(dict, "key2")));
  testrun(0 == strcmp("vXl3", dict->get(dict, "key3")));

  // failure from function
  testrun(dict->set(dict, strdup("key2"), NULL, NULL));
  testrun(!dict->for_each(dict, NULL, for_each_set_x));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(ov_dict_is_set(dict, "key2"));
  testrun(ov_dict_is_set(dict, "key3"));
  dict->free(dict);
  free(val1);
  free(val2);
  free(val3);

  // check with non overloaded

  dict = dict_creator(ov_dict_string_key_config(100));
  key1 = strdup("key1");
  key2 = strdup("key2");
  key3 = strdup("key3");
  val1 = strdup("val1");
  val2 = strdup("val2");
  val3 = strdup("val3");

  // dict without values
  testrun(dict->for_each(dict, NULL, for_each_set_x));

  testrun(dict->set(dict, key1, val1, NULL));
  testrun(dict->set(dict, key2, val2, NULL));
  testrun(dict->set(dict, key3, val3, NULL));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(ov_dict_is_set(dict, "key2"));
  testrun(ov_dict_is_set(dict, "key3"));
  testrun(dict->for_each(dict, NULL, for_each_set_x));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(ov_dict_is_set(dict, "key2"));
  testrun(ov_dict_is_set(dict, "key3"));
  testrun(0 == strcmp("vXl1", dict->get(dict, "key1")));
  testrun(0 == strcmp("vXl2", dict->get(dict, "key2")));
  testrun(0 == strcmp("vXl3", dict->get(dict, "key3")));

  testrun(dict->for_each(dict, NULL, for_each_set_x));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(ov_dict_is_set(dict, "key2"));
  testrun(ov_dict_is_set(dict, "key3"));
  testrun(0 == strcmp("vXl1", dict->get(dict, "key1")));
  testrun(0 == strcmp("vXl2", dict->get(dict, "key2")));
  testrun(0 == strcmp("vXl3", dict->get(dict, "key3")));

  // failure from function
  testrun(dict->set(dict, strdup("key2"), NULL, NULL));
  testrun(!dict->for_each(dict, NULL, for_each_set_x));
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(ov_dict_is_set(dict, "key2"));
  testrun(ov_dict_is_set(dict, "key3"));
  dict->free(dict);
  free(val1);
  free(val2);
  free(val3);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/
