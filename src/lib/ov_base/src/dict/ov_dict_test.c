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
        @file           ov_dict_test.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-08-15

        @ingroup        ov_data_structures

        @brief          Unit tests of


        ------------------------------------------------------------------------
*/
#include "../../include/ov_dict_test_interface.h"
#include "ov_dict.c"

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_dict_create() {

  size_t slots = 20;
  ov_dict_config config = ov_dict_string_key_config(slots);

  ov_dict *dict = ov_dict_create(config);

  // config general data
  testrun(dict);
  testrun(dict->magic_byte == OV_DICT_MAGIC_BYTE);
  testrun(dict->type == IMPL_DEFAULT_DICT_TYPE);

  // config check
  testrun(dict->config.slots == slots);
  testrun(dict->config.key.data_function.free == ov_data_string_free);
  testrun(dict->config.key.data_function.clear == ov_data_string_clear);
  testrun(dict->config.key.data_function.copy == ov_data_string_copy);
  testrun(dict->config.key.data_function.dump == ov_data_string_dump);
  testrun(dict->config.key.hash == ov_hash_pearson_c_string);
  testrun(dict->config.key.match == ov_match_c_string_strict);
  testrun(dict->config.value.data_function.free == NULL);
  testrun(dict->config.value.data_function.clear == NULL);
  testrun(dict->config.value.data_function.copy == NULL);
  testrun(dict->config.value.data_function.dump == NULL);

  // function check
  testrun(dict->is_empty == impl_dict_is_empty);
  testrun(dict->create == ov_dict_create);
  testrun(dict->clear == impl_dict_clear);
  testrun(dict->free == impl_dict_free);
  testrun(dict->get_keys == impl_dict_get_keys);
  testrun(dict->get == impl_dict_get);
  testrun(dict->set == impl_dict_set);
  testrun(dict->del == impl_dict_del);
  testrun(dict->remove == impl_dict_remove);
  testrun(dict->for_each == impl_dict_for_each);

  dict = dict->free(dict);

  // try to create with invalid config
  testrun(!ov_dict_create((ov_dict_config){0}));

  config.key.hash = NULL;
  testrun(!ov_dict_create(config));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_cast() {

  char data[10];
  memset(data, 0, 10);

  for (size_t i = 0; i <= 0xFF; i++) {

    data[0] = i;

    if (i == OV_DICT_MAGIC_BYTE) {
      testrun(ov_dict_cast(data));
    } else {
      testrun(!ov_dict_cast(data));
    }
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_config_is_valid() {

  ov_dict_config config = {0};

  testrun(!ov_dict_config_is_valid(NULL));
  testrun(!ov_dict_config_is_valid(&config));

  // prepare min valid
  config.slots = 1;
  config.key.hash = ov_hash_pearson_c_string;
  config.key.match = ov_match_c_string_strict;
  testrun(ov_dict_config_is_valid(&config));

  config.slots = 0;
  testrun(!ov_dict_config_is_valid(&config));
  config.slots = 100;
  testrun(ov_dict_config_is_valid(&config));

  config.key.hash = NULL;
  testrun(!ov_dict_config_is_valid(&config));
  config.key.hash = ov_hash_simple_c_string;
  testrun(ov_dict_config_is_valid(&config));

  config.key.match = NULL;
  testrun(!ov_dict_config_is_valid(&config));
  config.key.match = ov_match_strict;
  testrun(ov_dict_config_is_valid(&config));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_is_valid() {

  ov_dict *dict = ov_dict_create(ov_dict_string_key_config(1));

  testrun(!ov_dict_is_valid(NULL));
  testrun(ov_dict_is_valid(dict));

  // config invalid
  dict->config.key.hash = NULL;
  testrun(!ov_dict_is_valid(dict));
  dict->config.key.hash = ov_hash_simple_c_string;
  testrun(ov_dict_is_valid(dict));

  // wrong magic number
  dict->magic_byte = 0;
  testrun(!ov_dict_is_valid(dict));
  dict->magic_byte = OV_DICT_MAGIC_BYTE;
  testrun(ov_dict_is_valid(dict));

  // check different type bytes
  for (uint16_t i = 0; i < 0xFFFF; i++) {

    dict->type = i;
    testrun(ov_dict_is_valid(dict));
  }

  // check function pointers
  dict->is_empty = NULL;
  testrun(!ov_dict_is_valid(dict));
  dict->is_empty = impl_dict_is_empty;
  testrun(ov_dict_is_valid(dict));

  // check function pointers
  dict->create = NULL;
  testrun(!ov_dict_is_valid(dict));
  dict->create = ov_dict_create;
  testrun(ov_dict_is_valid(dict));

  dict->clear = NULL;
  testrun(!ov_dict_is_valid(dict));
  dict->clear = impl_dict_clear;
  testrun(ov_dict_is_valid(dict));

  dict->free = NULL;
  testrun(!ov_dict_is_valid(dict));
  dict->free = impl_dict_free;
  testrun(ov_dict_is_valid(dict));

  dict->get = NULL;
  testrun(!ov_dict_is_valid(dict));
  dict->get = impl_dict_get;
  testrun(ov_dict_is_valid(dict));

  dict->set = NULL;
  testrun(!ov_dict_is_valid(dict));
  dict->set = impl_dict_set;
  testrun(ov_dict_is_valid(dict));

  dict->del = NULL;
  testrun(!ov_dict_is_valid(dict));
  dict->del = impl_dict_del;
  testrun(ov_dict_is_valid(dict));

  dict->remove = NULL;
  testrun(!ov_dict_is_valid(dict));
  dict->remove = impl_dict_remove;
  testrun(ov_dict_is_valid(dict));

  dict->for_each = NULL;
  testrun(!ov_dict_is_valid(dict));
  dict->for_each = impl_dict_for_each;
  testrun(ov_dict_is_valid(dict));

  dict->type = IMPL_DEFAULT_DICT_TYPE;
  dict->free(dict);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_is_set() {

  char *key = NULL;
  ov_dict *dict = ov_dict_create(ov_dict_string_key_config(1));

  key = strdup("key1");
  testrun(!ov_dict_is_set(dict, key));
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(ov_dict_is_set(dict, key));
  key = strdup("key2");
  testrun(!ov_dict_is_set(dict, key));
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(ov_dict_is_set(dict, key));
  key = strdup("key3");
  testrun(!ov_dict_is_set(dict, key));
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(ov_dict_is_set(dict, key));

  // check all again
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(ov_dict_is_set(dict, "key2"));
  testrun(ov_dict_is_set(dict, "key3"));
  // some false checks
  // @NOTE case sensitive checks require case sensitive match!
  testrun(!ov_dict_is_set(dict, "Key1"));
  testrun(!ov_dict_is_set(dict, "keY2"));
  testrun(!ov_dict_is_set(dict, "key4"));

  dict->free(dict);

  // same checks buckets not overloaded
  dict = ov_dict_create(ov_dict_string_key_config(100));
  key = strdup("key1");
  testrun(!ov_dict_is_set(dict, key));
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(ov_dict_is_set(dict, key));
  key = strdup("key2");
  testrun(!ov_dict_is_set(dict, key));
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(ov_dict_is_set(dict, key));
  key = strdup("key3");
  testrun(!ov_dict_is_set(dict, key));
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(ov_dict_is_set(dict, key));

  // check all again
  testrun(ov_dict_is_set(dict, "key1"));
  testrun(ov_dict_is_set(dict, "key2"));
  testrun(ov_dict_is_set(dict, "key3"));

  dict->free(dict);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_set_magic_bytes() {

  ov_dict *dict = calloc(1, sizeof(ov_dict));

  testrun(!ov_dict_set_magic_bytes(NULL));

  testrun(dict->magic_byte == 0);
  testrun(ov_dict_set_magic_bytes(dict));
  testrun(dict->magic_byte == OV_DICT_MAGIC_BYTE);
  testrun(ov_dict_set_magic_bytes(dict));
  testrun(dict->magic_byte == OV_DICT_MAGIC_BYTE);

  // check override
  dict->magic_byte = 0xabcd;
  testrun(ov_dict_set_magic_bytes(dict));
  testrun(dict->magic_byte == OV_DICT_MAGIC_BYTE);
  dict->magic_byte = 0x0010;
  testrun(ov_dict_set_magic_bytes(dict));
  testrun(dict->magic_byte == OV_DICT_MAGIC_BYTE);

  free(dict);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_string_key_config() {

  ov_dict_config config = {0};

  for (size_t i = 0; i < 10; i++) {

    config = ov_dict_string_key_config(i);

    if (i == 0) {
      testrun(config.slots == IMPL_DEFAULT_SLOTS);
    } else {
      testrun(i == config.slots);
    }

    testrun(config.key.data_function.free == ov_data_string_free);
    testrun(config.key.data_function.clear == ov_data_string_clear);
    testrun(config.key.data_function.copy == ov_data_string_copy);
    testrun(config.key.data_function.dump == ov_data_string_dump);
    testrun(config.key.hash == ov_hash_pearson_c_string);
    testrun(config.key.match == ov_match_c_string_strict);
    testrun(config.value.data_function.free == NULL);
    testrun(config.value.data_function.clear == NULL);
    testrun(config.value.data_function.copy == NULL);
    testrun(config.value.data_function.dump == NULL);
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_intptr_key_config() {

  ov_dict_config config = {0};

  for (size_t i = 0; i < 10; i++) {

    config = ov_dict_intptr_key_config(i);

    if (i == 0) {
      testrun(config.slots == IMPL_DEFAULT_SLOTS);
    } else {
      testrun(i == config.slots);
    }

    testrun(config.key.data_function.free == NULL);
    testrun(config.key.data_function.clear == NULL);
    testrun(config.key.data_function.copy == NULL);
    testrun(config.key.data_function.dump == intptr_dump);
    testrun(config.key.hash == ov_hash_intptr);
    testrun(config.key.match == ov_match_intptr);
    testrun(config.value.data_function.free == NULL);
    testrun(config.value.data_function.clear == NULL);
    testrun(config.value.data_function.copy == NULL);
    testrun(config.value.data_function.dump == NULL);
  }

  ov_dict *dict = ov_dict_create(config);
  testrun(dict);
  ov_dict_free(dict);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_uint64_key_config() {

  ov_dict_config config = {0};

  for (size_t i = 0; i < 10; i++) {

    config = ov_dict_uint64_key_config(i);

    if (i == 0) {
      testrun(config.slots == IMPL_DEFAULT_SLOTS);
    } else {
      testrun(i == config.slots);
    }

    testrun(config.key.data_function.free == ov_data_uint64_free);
    testrun(config.key.data_function.clear == ov_data_uint64_clear);
    testrun(config.key.data_function.copy == ov_data_uint64_copy);
    testrun(config.key.data_function.dump == ov_data_uint64_dump);
    testrun(config.key.hash == ov_hash_uint64);
    testrun(config.key.match == ov_match_uint64);
    testrun(config.value.data_function.free == NULL);
    testrun(config.value.data_function.clear == NULL);
    testrun(config.value.data_function.copy == NULL);
    testrun(config.value.data_function.dump == NULL);
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_count() {

  char *key = NULL;
  ov_dict *dict = ov_dict_create(ov_dict_string_key_config(1));

  key = strdup("key1");
  testrun(!ov_dict_is_set(dict, key));
  testrun(-1 == ov_dict_count(NULL));
  testrun(0 == ov_dict_count(dict));
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(1 == ov_dict_count(dict));
  key = strdup("key2");
  testrun(!ov_dict_is_set(dict, key));
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(2 == ov_dict_count(dict));
  key = strdup("key3");
  testrun(!ov_dict_is_set(dict, key));
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(3 == ov_dict_count(dict));

  testrun(dict->del(dict, key));
  testrun(2 == ov_dict_count(dict));

  dict->free(dict);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_copy() {

  char *key = NULL;
  ov_dict *dict = ov_dict_create(ov_dict_string_key_config(1));
  ov_dict *copy = NULL;

  testrun(!ov_dict_copy(NULL, NULL));
  testrun(!ov_dict_copy((void **)&copy, NULL));
  testrun(!ov_dict_copy(NULL, dict));

  // no value copy
  testrun(!ov_dict_copy((void **)&copy, dict));

  // set string value functions
  dict->config.value.data_function = dict->config.key.data_function;

  // copy empty
  testrun(ov_dict_copy((void **)&copy, dict));
  testrun(copy);
  testrun(copy);
  testrun(copy->magic_byte == OV_DICT_MAGIC_BYTE);
  testrun(copy->type == IMPL_DEFAULT_DICT_TYPE);

  // config check
  testrun(copy->config.slots == 1);
  testrun(copy->config.key.data_function.free == ov_data_string_free);
  testrun(copy->config.key.data_function.clear == ov_data_string_clear);
  testrun(copy->config.key.data_function.copy == ov_data_string_copy);
  testrun(copy->config.key.data_function.dump == ov_data_string_dump);
  testrun(copy->config.key.hash == ov_hash_pearson_c_string);
  testrun(copy->config.key.match == ov_match_c_string_strict);
  testrun(copy->config.value.data_function.free == ov_data_string_free);
  testrun(copy->config.value.data_function.clear == ov_data_string_clear);
  testrun(copy->config.value.data_function.copy == ov_data_string_copy);
  testrun(copy->config.value.data_function.dump == ov_data_string_dump);

  // function check
  testrun(copy->is_empty == impl_dict_is_empty);
  testrun(copy->create == ov_dict_create);
  testrun(copy->clear == impl_dict_clear);
  testrun(copy->free == impl_dict_free);
  testrun(copy->get == impl_dict_get);
  testrun(copy->set == impl_dict_set);
  testrun(copy->del == impl_dict_del);
  testrun(copy->remove == impl_dict_remove);
  testrun(copy->for_each == impl_dict_for_each);

  testrun(0 == ov_dict_count(dict));
  testrun(0 == ov_dict_count(copy));

  // copy with content
  key = strdup("key1");
  testrun(dict->set(dict, key, NULL, NULL));
  key = strdup("key2");
  testrun(dict->set(dict, key, strdup("val2"), NULL));
  key = strdup("key3");
  testrun(dict->set(dict, key, strdup("val3"), NULL));
  testrun(3 == ov_dict_count(dict));
  testrun(0 == ov_dict_count(copy));

  testrun(ov_dict_copy((void **)&copy, dict));
  testrun(3 == ov_dict_count(dict));
  testrun(3 == ov_dict_count(copy));
  testrun(ov_dict_is_set(copy, "key1"));
  testrun(ov_dict_is_set(copy, "key2"));
  testrun(ov_dict_is_set(copy, "key3"));
  testrun(NULL == copy->get(copy, "key1"));
  testrun(0 == strcmp("val2", copy->get(copy, "key2")));
  testrun(0 == strcmp("val3", copy->get(copy, "key3")));
  dict->free(dict);

  // check empty existing content
  dict = ov_dict_create(ov_dict_string_key_config(100));
  dict->config.value.data_function = dict->config.key.data_function;

  testrun(ov_dict_copy((void **)&copy, dict));
  testrun(0 == ov_dict_count(dict));
  testrun(0 == ov_dict_count(copy));

  // check slots are as input (no automated override!)
  testrun(copy->config.slots == 1);
  testrun(dict->config.slots == 100);

  key = strdup("key1");
  testrun(dict->set(dict, key, NULL, NULL));
  key = strdup("key2");
  testrun(dict->set(dict, key, strdup("val2"), NULL));
  key = strdup("key3");
  testrun(dict->set(dict, key, strdup("val3"), NULL));
  testrun(3 == ov_dict_count(dict));
  testrun(0 == ov_dict_count(copy));
  testrun(ov_dict_copy((void **)&copy, dict));
  testrun(3 == ov_dict_count(dict));
  testrun(3 == ov_dict_count(copy));
  testrun(ov_dict_is_set(copy, "key1"));
  testrun(ov_dict_is_set(copy, "key2"));
  testrun(ov_dict_is_set(copy, "key3"));
  testrun(NULL == copy->get(copy, "key1"));
  testrun(0 == strcmp("val2", copy->get(copy, "key2")));
  testrun(0 == strcmp("val3", copy->get(copy, "key3")));
  dict->free(dict);
  copy->free(copy);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_clear() {

  char *key = NULL;
  ov_dict *dict = ov_dict_create(ov_dict_string_key_config(1));

  // clear empty

  testrun(!ov_dict_clear(NULL));
  testrun(ov_dict_clear(dict));

  key = strdup("key1");
  testrun(dict->set(dict, key, NULL, NULL));
  key = strdup("key2");
  testrun(dict->set(dict, key, NULL, NULL));
  key = strdup("key3");
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(3 == ov_dict_count(dict));
  testrun(ov_dict_clear(dict));
  testrun(0 == ov_dict_count(dict));

  dict->free(dict);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_free() {

  char *key = NULL;
  ov_dict *dict = ov_dict_create(ov_dict_string_key_config(1));

  // clear empty

  testrun(NULL == ov_dict_free(NULL));
  testrun(NULL == ov_dict_free(dict));

  dict = ov_dict_create(ov_dict_string_key_config(100));
  key = strdup("key1");
  testrun(dict->set(dict, key, NULL, NULL));
  key = strdup("key2");
  testrun(dict->set(dict, key, NULL, NULL));
  key = strdup("key3");
  testrun(dict->set(dict, key, NULL, NULL));
  testrun(3 == ov_dict_count(dict));
  testrun(NULL == ov_dict_free(dict));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_dump() {

  ov_dict *dict = ov_dict_create(ov_dict_string_key_config(1));

  // dump empty

  testrun(!ov_dict_dump(NULL, NULL));
  testrun(!ov_dict_dump(stdout, NULL));
  testrun(!ov_dict_dump(NULL, dict));

  testrun(ov_dict_dump(stdout, dict));

  testrun(dict->set(dict, strdup("key1"), NULL, NULL));
  testrun(ov_dict_dump(stdout, dict));

  // no value dump
  testrun(dict->set(dict, strdup("key2"), "value2", NULL));
  testrun(ov_dict_dump(stdout, dict));

  // with value dump
  dict->config.value.data_function.dump = ov_data_string_dump;
  testrun(ov_dict_dump(stdout, dict));

  dict->free(dict);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_data_functions() {

  ov_data_function f = ov_dict_data_functions();

  testrun(f.clear == ov_dict_clear);
  testrun(f.free == ov_dict_free);
  testrun(f.copy == ov_dict_copy);
  testrun(f.dump == ov_dict_dump);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_is_empty() {

  ov_dict *dict = ov_dict_create(ov_dict_string_key_config(100));
  void *function = dict->is_empty;

  void *value = "value";
  char *key = strdup("key");

  testrun(ov_dict_is_empty(dict));
  testrun(dict->set(dict, key, value, NULL));
  testrun(!ov_dict_is_empty(dict));

  testrun(ov_dict_del(dict, key));
  testrun(ov_dict_is_empty(dict));

  dict->is_empty = NULL;
  testrun(!ov_dict_is_empty(dict));
  dict->is_empty = function;
  testrun(ov_dict_is_empty(dict));

  ov_dict_free(dict);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_del() {

  ov_dict *dict = ov_dict_create(ov_dict_string_key_config(100));
  void *function = dict->del;

  void *value = "value";
  char *key = strdup("key");

  testrun(dict->set(dict, key, value, NULL));

  testrun(1 == ov_dict_count(dict));
  testrun(ov_dict_del(dict, key));
  testrun(0 == ov_dict_count(dict));

  key = strdup("key");
  testrun(dict->set(dict, key, value, NULL));

  dict->del = NULL;
  testrun(!ov_dict_del(dict, key));
  dict->del = function;
  testrun(1 == ov_dict_count(dict));
  testrun(ov_dict_del(dict, key));
  testrun(0 == ov_dict_count(dict));

  ov_dict_free(dict);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_get_keys() {

  ov_dict *dict = ov_dict_create(ov_dict_string_key_config(100));
  void *function = dict->get_keys;

  void *value = "value";
  char *key = strdup("key");

  testrun(dict->set(dict, key, value, NULL));
  ov_list *list = ov_dict_get_keys(dict, value);
  testrun(list);
  testrun(ov_list_count(list) == 1);
  list = ov_list_free(list);

  dict->get_keys = NULL;
  testrun(!ov_dict_get_keys(dict, value));
  dict->get_keys = function;
  list = ov_dict_get_keys(dict, value);
  testrun(list);
  testrun(ov_list_count(list) == 1);
  list = ov_list_free(list);

  testrun(ov_dict_del(dict, key));
  list = ov_dict_get_keys(dict, value);
  testrun(list);
  testrun(ov_list_count(list) == 0);
  list = ov_list_free(list);

  ov_dict_free(dict);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_get() {

  testrun(0 == ov_dict_get(0, 0));
  testrun(0 == ov_dict_get(0, "abc"));

  ov_dict *dict = ov_dict_create(ov_dict_string_key_config(100));
  void *function = dict->get;

  void *value = "value";
  char *key = strdup("key");

  testrun(dict->set(dict, key, value, NULL));

  testrun(value == ov_dict_get(dict, key));

  dict->get = NULL;
  testrun(!ov_dict_get(dict, key));
  dict->get = function;
  testrun(value == ov_dict_get(dict, key));

  ov_dict_free(dict);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_remove() {

  ov_dict *dict = ov_dict_create(ov_dict_string_key_config(100));
  void *function = dict->remove;

  void *value = "value";
  char *key = strdup("key");

  void *value2 = "value2";
  char *key2 = strdup("key2");

  testrun(dict->set(dict, key, value, NULL));
  testrun(dict->set(dict, key2, value2, NULL));

  testrun(value == ov_dict_remove(dict, key));

  dict->remove = NULL;
  testrun(!ov_dict_remove(dict, key2));
  dict->remove = function;
  testrun(value2 == ov_dict_remove(dict, key2));

  ov_dict_free(dict);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_set() {

  ov_dict *dict = ov_dict_create(ov_dict_string_key_config(100));
  void *function = dict->set;

  void *value = "value";
  char *key = strdup("key");

  void *value2 = "value2";
  char *key2 = strdup("key2");

  testrun(ov_dict_set(dict, key, value, NULL));

  testrun(value == ov_dict_get(dict, key));

  dict->set = NULL;
  testrun(!ov_dict_set(dict, key2, value2, NULL));
  dict->set = function;
  testrun(ov_dict_set(dict, key2, value2, NULL));
  testrun(value == ov_dict_get(dict, key));
  testrun(value2 == ov_dict_get(dict, key2));

  ov_dict_free(dict);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

bool dummy_for_each(const void *key, void *value, void *data) {

  if (!key && !value)
    return true;

  if (key || value || data)
    return true;

  return true;
}

/*----------------------------------------------------------------------------*/

int test_ov_dict_for_each() {

  ov_dict *dict = ov_dict_create(ov_dict_string_key_config(100));
  void *function = dict->for_each;

  void *value = "value";
  char *key = strdup("key");
  testrun(ov_dict_set(dict, key, value, NULL));

  testrun(ov_dict_for_each(dict, NULL, dummy_for_each));

  dict->for_each = NULL;
  testrun(!ov_dict_for_each(dict, NULL, dummy_for_each));
  dict->for_each = function;
  testrun(ov_dict_for_each(dict, NULL, dummy_for_each));

  ov_dict_free(dict);
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

int all_tests() {

  testrun_init();
  testrun_test(test_ov_dict_create);
  testrun_test(test_ov_dict_cast);
  testrun_test(test_ov_dict_config_is_valid);
  testrun_test(test_ov_dict_is_valid);
  testrun_test(test_ov_dict_is_set);
  testrun_test(test_ov_dict_count);
  testrun_test(test_ov_dict_set_magic_bytes);
  testrun_test(test_ov_dict_string_key_config);
  testrun_test(test_ov_dict_intptr_key_config);
  testrun_test(test_ov_dict_uint64_key_config);

  OV_DICT_PERFORM_PERFORM_INTERFACE_TESTS(ov_dict_create);

  testrun_test(test_ov_dict_copy);
  testrun_test(test_ov_dict_clear);
  testrun_test(test_ov_dict_free);
  testrun_test(test_ov_dict_dump);
  testrun_test(test_ov_dict_data_functions);

  testrun_test(test_ov_dict_is_empty);
  testrun_test(test_ov_dict_del);
  testrun_test(test_ov_dict_get_keys);
  testrun_test(test_ov_dict_get);
  testrun_test(test_ov_dict_remove);
  testrun_test(test_ov_dict_set);

  testrun_test(test_ov_dict_for_each);

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
