/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_callback_test.c
        @author         Markus Töpfer

        @date           2023-03-29


        ------------------------------------------------------------------------
*/
#include "ov_callback.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_callback_registry_create() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  ov_callback_registry_config config = {0};

  testrun(!ov_callback_registry_create(config));
  config.loop = loop;

  ov_callback_registry *reg = ov_callback_registry_create(config);
  testrun(reg);
  testrun(reg->data);
  testrun(OV_CALLBACK_TIMEOUT_DEFAULT_USEC == reg->config.timeout_usec);
  testrun(OV_CALLBACK_DEFAULT_CACHE_SIZE == reg->config.cache_size);
  testrun(reg->timer_id != OV_TIMER_INVALID);

  testrun(NULL == ov_callback_registry_free(reg));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_callback_registry_free() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  ov_callback_registry_config config =
      (ov_callback_registry_config){.loop = loop};

  ov_callback_registry *reg = ov_callback_registry_create(config);
  testrun(reg);
  testrun(NULL == ov_callback_registry_free(reg));

  reg = ov_callback_registry_create(config);
  testrun(reg);

  void *data = calloc(1, 1000);

  testrun(ov_callback_registry_register(
      reg, "a",
      (ov_callback){.userdata = data,
                    .function = ov_callback_registry_register},
      OV_CALLBACK_TIMEOUT_DEFAULT_USEC));

  testrun(ov_callback_registry_register(
      reg, "a-b",
      (ov_callback){.userdata = data,
                    .function = ov_callback_registry_register},
      OV_CALLBACK_TIMEOUT_DEFAULT_USEC));

  testrun(ov_callback_registry_register(
      reg, "a-b-c",
      (ov_callback){.userdata = data,
                    .function = ov_callback_registry_register},
      OV_CALLBACK_TIMEOUT_DEFAULT_USEC));

  testrun(NULL == ov_callback_registry_free(reg));
  testrun(NULL == ov_event_loop_free(loop));

  data = ov_data_pointer_free(data);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_callback_registry_cast() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  ov_callback_registry_config config =
      (ov_callback_registry_config){.loop = loop};

  ov_callback_registry *reg = ov_callback_registry_create(config);
  testrun(reg);
  testrun(ov_callback_registry_cast(reg));
  testrun(reg->magic_bytes == OV_CALLBACK_REGISTRY_MAGIC_BYTES);

  testrun(NULL == ov_callback_registry_free(reg));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_callback_registry_register() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  ov_callback_registry_config config =
      (ov_callback_registry_config){.loop = loop};

  ov_callback_registry *reg = ov_callback_registry_create(config);
  testrun(reg);

  void *data = calloc(1, 1000);

  testrun(!ov_callback_registry_register(
      NULL, "a",
      (ov_callback){.userdata = data,
                    .function = ov_callback_registry_register},
      OV_CALLBACK_TIMEOUT_DEFAULT_USEC));

  testrun(!ov_callback_registry_register(
      reg, NULL,
      (ov_callback){.userdata = data,
                    .function = ov_callback_registry_register},
      OV_CALLBACK_TIMEOUT_DEFAULT_USEC));

  testrun(!ov_callback_registry_register(
      reg, "a",
      (ov_callback){.userdata = data,
                    .function = ov_callback_registry_register},
      0));

  testrun(ov_callback_registry_register(
      reg, "a",
      (ov_callback){.userdata = data,
                    .function = ov_callback_registry_register},
      OV_CALLBACK_TIMEOUT_DEFAULT_USEC));

  testrun(1 == ov_dict_count(reg->data));

  // check override

  testrun(ov_callback_registry_register(
      reg, "a",
      (ov_callback){.userdata = data,
                    .function = ov_callback_registry_register},
      OV_CALLBACK_TIMEOUT_DEFAULT_USEC));

  testrun(1 == ov_dict_count(reg->data));

  // check additional

  testrun(ov_callback_registry_register(
      reg, "b",
      (ov_callback){.userdata = data,
                    .function = ov_callback_registry_register},
      OV_CALLBACK_TIMEOUT_DEFAULT_USEC));

  testrun(2 == ov_dict_count(reg->data));

  testrun(NULL == ov_callback_registry_free(reg));
  testrun(NULL == ov_event_loop_free(loop));
  data = ov_data_pointer_free(data);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_callback_registry_unregister() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  ov_callback_registry_config config =
      (ov_callback_registry_config){.loop = loop};

  ov_callback_registry *reg = ov_callback_registry_create(config);
  testrun(reg);

  void *data = calloc(1, 1000);

  testrun(ov_callback_registry_register(
      reg, "a",
      (ov_callback){.userdata = data,
                    .function = ov_callback_registry_register},
      OV_CALLBACK_TIMEOUT_DEFAULT_USEC));

  testrun(1 == ov_dict_count(reg->data));

  // check unregister

  testrun(1 == ov_dict_count(reg->data));

  ov_callback cb = ov_callback_registry_unregister(reg, "a");

  testrun(0 == ov_dict_count(reg->data));
  testrun(cb.userdata == data);
  testrun(cb.function == ov_callback_registry_register);

  testrun(NULL == ov_callback_registry_free(reg));
  testrun(NULL == ov_event_loop_free(loop));
  data = ov_data_pointer_free(data);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_callback_registry_get() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  ov_callback_registry_config config =
      (ov_callback_registry_config){.loop = loop};

  ov_callback_registry *reg = ov_callback_registry_create(config);
  testrun(reg);

  void *data = calloc(1, 1000);

  testrun(ov_callback_registry_register(
      reg, "a",
      (ov_callback){.userdata = data,
                    .function = ov_callback_registry_register},
      OV_CALLBACK_TIMEOUT_DEFAULT_USEC));

  testrun(1 == ov_dict_count(reg->data));

  // check unregister

  testrun(1 == ov_dict_count(reg->data));

  ov_callback cb = ov_callback_registry_get(reg, "a");

  testrun(1 == ov_dict_count(reg->data));
  testrun(cb.userdata == data);
  testrun(cb.function == ov_callback_registry_register);

  cb = ov_callback_registry_get(reg, "a");

  testrun(1 == ov_dict_count(reg->data));
  testrun(cb.userdata == data);
  testrun(cb.function == ov_callback_registry_register);

  cb = ov_callback_registry_get(reg, "a");

  testrun(1 == ov_dict_count(reg->data));
  testrun(cb.userdata == data);
  testrun(cb.function == ov_callback_registry_register);

  cb = ov_callback_registry_get(reg, "a");

  testrun(1 == ov_dict_count(reg->data));
  testrun(cb.userdata == data);
  testrun(cb.function == ov_callback_registry_register);

  testrun(NULL == ov_callback_registry_free(reg));
  testrun(NULL == ov_event_loop_free(loop));
  data = ov_data_pointer_free(data);
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
  testrun_test(test_ov_callback_registry_create);
  testrun_test(test_ov_callback_registry_free);
  testrun_test(test_ov_callback_registry_cast);

  testrun_test(test_ov_callback_registry_register);
  testrun_test(test_ov_callback_registry_unregister);
  testrun_test(test_ov_callback_registry_get);

  ov_registered_cache_free_all();
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
