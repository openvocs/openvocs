/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_event_broadcast_test.c
        @author         Markus Töpfer

        @date           2021-01-23


        ------------------------------------------------------------------------
*/
#include "ov_event_broadcast.c"
#include <ov_test/testrun.h>

#include "../include/ov_event_api.h"

#include <ov_base/ov_dict.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_time.h>
#include <stdint.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_event_broadcast_create() {

  ov_event_broadcast *b =
      ov_event_broadcast_create((ov_event_broadcast_config){0});
  testrun(NULL != b);
  b = ov_event_broadcast_free(b);

  b = ov_event_broadcast_create(
      (ov_event_broadcast_config){.max_sockets = 1000, .lock_timeout_usec = 1});
  testrun(b);
  testrun(broadcast_cast(b));
  testrun(OV_EVENT_BROADCAST_MAGIC_BYTE == b->magic_byte);
  testrun(0 == b->last);
  testrun(1000 == b->config.max_sockets);
  testrun(1 == b->config.lock_timeout_usec);

  for (size_t i = 0; i < b->config.max_sockets; i++) {
    testrun(b->connections[i].type == 0);
  }

  testrun(NULL == ov_event_broadcast_free(b));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_broadcast_free() {

  ov_event_broadcast *b =
      ov_event_broadcast_create((ov_event_broadcast_config){0});
  testrun(b);

  testrun(NULL == ov_event_broadcast_free(b));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_broadcast_configure_uri_event_io() {

  ov_event_broadcast *b =
      ov_event_broadcast_create((ov_event_broadcast_config){0});
  testrun(b);

  ov_event_io_config config =
      ov_event_broadcast_configure_uri_event_io(NULL, NULL);
  testrun(config.userdata == NULL);
  testrun(config.callback.close == NULL);
  testrun(config.callback.process == NULL);
  testrun(0 == config.name[0]);

  config = ov_event_broadcast_configure_uri_event_io(b, NULL);
  testrun(config.userdata == NULL);
  testrun(config.callback.close == NULL);
  testrun(config.callback.process == NULL);
  testrun(0 == config.name[0]);

  config = ov_event_broadcast_configure_uri_event_io(NULL, "name");
  testrun(config.userdata == NULL);
  testrun(config.callback.close == NULL);
  testrun(config.callback.process == NULL);
  testrun(0 == config.name[0]);

  config = ov_event_broadcast_configure_uri_event_io(b, "name");
  testrun(config.userdata == b);
  testrun(config.callback.close == cb_close);
  testrun(config.callback.process == cb_process);
  testrun(0 == strcmp("name", config.name));

  testrun(NULL == ov_event_broadcast_free(b));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_broadcast_set() {

  ov_event_broadcast *b = ov_event_broadcast_create((ov_event_broadcast_config){
      .max_sockets = ov_socket_get_max_supported_runtime_sockets(0)});
  testrun(b);

  testrun(!ov_event_broadcast_set(NULL, 0, 0));
  testrun(!ov_event_broadcast_set(NULL, 1, 0));

  testrun(ov_event_broadcast_set(b, 0, 0));

  testrun(0 == b->last);
  testrun(0 == b->connections[0].type);

  for (uint8_t i = 0; i < 0xff; i++) {

    testrun(ov_event_broadcast_set(b, 0, i));
    testrun(0 == b->last);
    testrun(i == b->connections[0].type);
  }

  testrun(ov_event_broadcast_set(b, 0, 2));
  testrun(0 == b->last);
  testrun(2 == b->connections[0].type);

  testrun(ov_event_broadcast_set(b, 1, 4));
  testrun(1 == b->last);
  testrun(2 == b->connections[0].type);
  testrun(4 == b->connections[1].type);

  testrun(ov_event_broadcast_set(b, 20, 1));
  testrun(20 == b->last);

  for (intptr_t i = 0; i < 100; i++) {

    switch (i) {

    case 0:
      testrun(2 == b->connections[i].type);
      break;
    case 1:
      testrun(4 == b->connections[i].type);
      break;
    case 20:
      testrun(1 == b->connections[i].type);
      break;
    default:
      testrun(OV_BROADCAST_UNSET == b->connections[i].type);
    }
  }

  size_t max_sockets = ov_socket_get_max_supported_runtime_sockets(0);

  for (size_t i = 0; i < max_sockets; i++) {

    testrun(ov_event_broadcast_set(b, i, OV_USER_BROADCAST));
    testrun(OV_USER_BROADCAST == b->connections[i].type);
  }

  testrun(ov_event_broadcast_set(b, 20, 0));

  for (size_t i = 0; i < max_sockets; i++) {

    switch (i) {

    case 20:
      testrun(OV_BROADCAST_UNSET == b->connections[i].type);
      break;
    default:
      testrun(OV_USER_BROADCAST == b->connections[i].type);
    }
  }

  testrun(
      ov_event_broadcast_set(b, 11, (OV_LOOP_BROADCAST | OV_ROLE_BROADCAST)));

  for (size_t i = 0; i < max_sockets; i++) {

    switch (i) {

    case 11:
      testrun((OV_LOOP_BROADCAST | OV_ROLE_BROADCAST) ==
              b->connections[i].type);
      break;
    case 20:
      testrun(OV_BROADCAST_UNSET == b->connections[i].type);
      break;
    default:
      testrun(OV_USER_BROADCAST == b->connections[i].type);
    }
  }

  testrun(ov_event_broadcast_set(
      b, 11, (OV_USER_BROADCAST | OV_LOOP_BROADCAST | OV_ROLE_BROADCAST)));

  for (size_t i = 0; i < max_sockets; i++) {

    switch (i) {

    case 11:
      testrun(0x0E == b->connections[i].type);
      break;
    case 20:
      testrun(OV_BROADCAST_UNSET == b->connections[i].type);
      break;
    default:
      testrun(OV_USER_BROADCAST == b->connections[i].type);
    }
  }

  testrun(NULL == ov_event_broadcast_free(b));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_broadcast_get() {

  ov_event_broadcast *b =
      ov_event_broadcast_create((ov_event_broadcast_config){0});
  testrun(b);

  testrun(0 == ov_event_broadcast_get(NULL, 0));
  testrun(0 == ov_event_broadcast_get(NULL, 1));
  testrun(0 == ov_event_broadcast_get(b, 1));

  testrun(ov_event_broadcast_set(b, 0, 0));
  testrun(0 == ov_event_broadcast_get(b, 0));

  testrun(ov_event_broadcast_set(b, 0, 1));
  testrun(1 == ov_event_broadcast_get(b, 0));

  for (uint8_t i = 0; i < 0xff; i++) {

    testrun(ov_event_broadcast_set(b, 0, i));
    testrun(i == ov_event_broadcast_get(b, 0));
  }

  testrun(NULL == ov_event_broadcast_free(b));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool dummy_send(void *instance, int socket, const ov_json_value *val) {

  if (!instance)
    return false;

  ov_dict *dict = ov_dict_cast(instance);
  intptr_t key = socket;
  return ov_dict_set(dict, (void *)key, (void *)val, NULL);
}

/*----------------------------------------------------------------------------*/

int test_ov_event_broadcast_send_params() {

  ov_dict *dict = ov_dict_create(ov_dict_intptr_key_config(255));

  ov_event_broadcast *b =
      ov_event_broadcast_create((ov_event_broadcast_config){0});
  testrun(b);

  ov_json_value *v = ov_json_object();

  ov_event_parameter p = {

      .send.instance = dict, .send.send = dummy_send};

  testrun(!ov_event_broadcast_send_params(NULL, NULL, NULL, 0));
  testrun(!ov_event_broadcast_send_params(NULL, &p, v, 1));
  testrun(!ov_event_broadcast_send_params(b, NULL, v, 1));
  testrun(!ov_event_broadcast_send_params(b, &p, NULL, 1));
  testrun(!ov_event_broadcast_send_params(b, &p, v, 0));

  // no listener
  testrun(ov_event_broadcast_send_params(b, &p, v, 1));
  testrun(ov_dict_is_empty(dict));

  testrun(ov_event_broadcast_set(b, 10, 1));
  testrun(ov_event_broadcast_set(b, 20, 1));
  testrun(ov_event_broadcast_set(b, 30, 1));
  testrun(ov_event_broadcast_set(b, 21, 2));
  testrun(ov_event_broadcast_set(b, 22, 2));
  testrun(ov_event_broadcast_set(b, 23, 2));
  testrun(ov_event_broadcast_set(b, 31, 3));
  testrun(ov_event_broadcast_set(b, 32, 3));
  testrun(ov_event_broadcast_set(b, 33, 3));

  testrun(ov_event_broadcast_send_params(b, &p, v, 1));

  for (intptr_t i = 0; i < 100; i++) {

    switch (i) {

    case 10:
    case 20:
    case 30:
    case 31:
    case 32:
    case 33:
      testrun(v == ov_dict_get(dict, (void *)i));
      break;

    default:
      testrun(ov_dict_get(dict, (void *)i) == NULL);
    }
  }

  ov_dict_clear(dict);
  testrun(ov_event_broadcast_send_params(b, &p, v, 2));

  for (intptr_t i = 0; i < 100; i++) {

    switch (i) {

    case 21:
    case 22:
    case 23:
    case 31:
    case 32:
    case 33:
      testrun(v == ov_dict_get(dict, (void *)i));
      break;

    default:
      testrun(ov_dict_get(dict, (void *)i) == NULL);
    }
  }

  ov_dict_clear(dict);
  testrun(ov_event_broadcast_send_params(b, &p, v, 3));

  for (intptr_t i = 0; i < 200; i++) {

    switch (i) {

    case 10:
    case 20:
    case 30:
    case 21:
    case 22:
    case 23:
    case 31:
    case 32:
    case 33:
      testrun(v == ov_dict_get(dict, (void *)i));
      break;

    default:
      testrun(ov_dict_get(dict, (void *)i) == NULL);
    }
  }

  ov_dict_clear(dict);
  testrun(ov_event_broadcast_send_params(b, &p, v, 4));
  intptr_t key = 4;
  testrun(0 == ov_dict_get(dict, (void *)key));

  testrun(ov_dict_is_empty(dict));

  testrun(ov_event_broadcast_set(b, 40, 4));
  testrun(ov_event_broadcast_set(b, 80, 8));
  testrun(ov_event_broadcast_set(b, 100, 0xff));

  ov_dict_clear(dict);
  testrun(ov_event_broadcast_send_params(b, &p, v, 4));

  for (intptr_t i = 0; i < 200; i++) {

    switch (i) {

    case 40:
    case 100:
      testrun(v == ov_dict_get(dict, (void *)i));
      break;

    default:
      testrun(ov_dict_get(dict, (void *)i) == NULL);
    }
  }

  ov_dict_clear(dict);
  testrun(ov_event_broadcast_send_params(b, &p, v, 8));

  for (intptr_t i = 0; i < 200; i++) {

    switch (i) {

    case 80:
    case 100:
      testrun(v == ov_dict_get(dict, (void *)i));
      break;

    default:
      testrun(ov_dict_get(dict, (void *)i) == NULL);
    }
  }

  // check OR based again

  ov_dict_clear(dict);
  testrun(ov_event_broadcast_send_params(b, &p, v, 3));

  for (intptr_t i = 0; i < 200; i++) {

    switch (i) {

    case 10:
    case 20:
    case 30:
    case 21:
    case 22:
    case 23:
    case 31:
    case 32:
    case 33:
    case 100:
      testrun(v == ov_dict_get(dict, (void *)i));
      break;

    default:
      testrun(ov_dict_get(dict, (void *)i) == NULL);
    }
  }

  testrun(NULL == ov_dict_free(dict));
  testrun(NULL == ov_json_value_free(v));
  testrun(NULL == ov_event_broadcast_free(b));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_broadcast_is_empty() {

  ov_event_broadcast *b =
      ov_event_broadcast_create((ov_event_broadcast_config){0});
  testrun(b);

  testrun(ov_event_broadcast_is_empty(b));
  testrun(!ov_event_broadcast_is_empty(NULL));

  testrun(ov_thread_lock_try_lock(&b->lock));
  testrun(!ov_event_broadcast_is_empty(b));
  testrun(ov_thread_lock_unlock(&b->lock));
  testrun(ov_event_broadcast_is_empty(b));

  testrun(ov_event_broadcast_set(b, 1, OV_BROADCAST));
  testrun(!ov_event_broadcast_is_empty(b));
  testrun(ov_event_broadcast_set(b, 1, OV_USER_BROADCAST));
  testrun(!ov_event_broadcast_is_empty(b));
  testrun(ov_event_broadcast_set(b, 1, OV_BROADCAST_UNSET));
  testrun(ov_event_broadcast_is_empty(b));

  testrun(NULL == ov_event_broadcast_free(b));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_broadcast_count() {

  ov_event_broadcast *b =
      ov_event_broadcast_create((ov_event_broadcast_config){0});
  testrun(b);

  testrun(ov_event_broadcast_is_empty(b));
  testrun(0 == ov_event_broadcast_count(b, 0));
  testrun(0 == ov_event_broadcast_count(b, 1));

  testrun(ov_event_broadcast_set(b, 1, OV_BROADCAST));
  testrun(1 == ov_event_broadcast_count(b, OV_BROADCAST));
  testrun(0 == ov_event_broadcast_count(b, OV_LOOP_BROADCAST));

  testrun(ov_event_broadcast_set(b, 1, OV_BROADCAST | OV_LOOP_BROADCAST));
  testrun(1 == ov_event_broadcast_count(b, OV_BROADCAST));
  testrun(1 == ov_event_broadcast_count(b, OV_LOOP_BROADCAST));
  testrun(0 == ov_event_broadcast_count(b, OV_ROLE_BROADCAST));

  testrun(ov_event_broadcast_set(b, 2, OV_BROADCAST | OV_LOOP_BROADCAST));
  testrun(2 == ov_event_broadcast_count(b, OV_BROADCAST));
  testrun(2 == ov_event_broadcast_count(b, OV_LOOP_BROADCAST));
  testrun(0 == ov_event_broadcast_count(b, OV_ROLE_BROADCAST));

  testrun(ov_event_broadcast_set(
      b, 2, OV_BROADCAST | OV_LOOP_BROADCAST | OV_ROLE_BROADCAST));
  testrun(2 == ov_event_broadcast_count(b, OV_BROADCAST));
  testrun(2 == ov_event_broadcast_count(b, OV_LOOP_BROADCAST));
  testrun(1 == ov_event_broadcast_count(b, OV_ROLE_BROADCAST));

  testrun(NULL == ov_event_broadcast_free(b));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_broadcast_state() {

  ov_event_broadcast *b =
      ov_event_broadcast_create((ov_event_broadcast_config){0});
  testrun(b);

  ov_json_value *val = NULL;
  ov_json_value *out = ov_event_broadcast_state(b);
  testrun(out);
  testrun(0 == ov_json_object_count(out));
  out = ov_json_value_free(out);

  testrun(ov_event_broadcast_set(b, 1, OV_BROADCAST));
  out = ov_event_broadcast_state(b);
  testrun(out);
  testrun(1 == ov_json_object_count(out));
  val = ov_json_object_get(out, "1");
  testrun(val);
  testrun(1 == ov_json_object_count(val));
  testrun(ov_json_is_true(ov_json_object_get(val, OV_BROADCAST_KEY_BROADCAST)));
  out = ov_json_value_free(out);

  testrun(ov_event_broadcast_set(b, 1, OV_BROADCAST | OV_LOOP_BROADCAST));
  out = ov_event_broadcast_state(b);
  testrun(out);
  testrun(1 == ov_json_object_count(out));
  val = ov_json_object_get(out, "1");
  testrun(val);
  testrun(2 == ov_json_object_count(val));
  testrun(ov_json_is_true(ov_json_object_get(val, OV_BROADCAST_KEY_BROADCAST)));
  testrun(ov_json_is_true(
      ov_json_object_get(val, OV_BROADCAST_KEY_LOOP_BROADCAST)));
  out = ov_json_value_free(out);

  testrun(ov_event_broadcast_set(
      b, 1, OV_BROADCAST | OV_LOOP_BROADCAST | OV_SYSTEM_BROADCAST));
  out = ov_event_broadcast_state(b);
  testrun(out);
  testrun(1 == ov_json_object_count(out));
  val = ov_json_object_get(out, "1");
  testrun(val);
  testrun(3 == ov_json_object_count(val));
  testrun(ov_json_is_true(ov_json_object_get(val, OV_BROADCAST_KEY_BROADCAST)));
  testrun(ov_json_is_true(
      ov_json_object_get(val, OV_BROADCAST_KEY_LOOP_BROADCAST)));
  testrun(ov_json_is_true(
      ov_json_object_get(val, OV_BROADCAST_KEY_SYSTEM_BROADCAST)));
  out = ov_json_value_free(out);

  // set all
  testrun(ov_event_broadcast_set(b, 1, 0xff));
  out = ov_event_broadcast_state(b);
  testrun(out);
  testrun(1 == ov_json_object_count(out));
  val = ov_json_object_get(out, "1");
  testrun(val);
  testrun(8 == ov_json_object_count(val));
  testrun(ov_json_is_true(ov_json_object_get(val, OV_BROADCAST_KEY_BROADCAST)));
  testrun(ov_json_is_true(
      ov_json_object_get(val, OV_BROADCAST_KEY_USER_BROADCAST)));
  testrun(ov_json_is_true(
      ov_json_object_get(val, OV_BROADCAST_KEY_ROLE_BROADCAST)));
  testrun(ov_json_is_true(
      ov_json_object_get(val, OV_BROADCAST_KEY_LOOP_BROADCAST)));
  testrun(ov_json_is_true(
      ov_json_object_get(val, OV_BROADCAST_KEY_LOOP_SENDER_BROADCAST)));
  testrun(ov_json_is_true(
      ov_json_object_get(val, OV_BROADCAST_KEY_PROJECT_BROADCAST)));
  testrun(ov_json_is_true(
      ov_json_object_get(val, OV_BROADCAST_KEY_DOMAIN_BROADCAST)));
  testrun(ov_json_is_true(
      ov_json_object_get(val, OV_BROADCAST_KEY_SYSTEM_BROADCAST)));
  out = ov_json_value_free(out);

  // unset all
  testrun(ov_event_broadcast_set(b, 1, 0x00));
  out = ov_event_broadcast_state(b);
  testrun(out);
  testrun(0 == ov_json_object_count(out));
  out = ov_json_value_free(out);

  // set several broadcasts
  testrun(ov_event_broadcast_set(b, 1, OV_BROADCAST | OV_LOOP_BROADCAST));
  testrun(ov_event_broadcast_set(b, 2, OV_LOOP_BROADCAST));
  testrun(ov_event_broadcast_set(b, 3, OV_ROLE_BROADCAST));
  testrun(ov_event_broadcast_set(b, 4, OV_SYSTEM_BROADCAST));

  out = ov_event_broadcast_state(b);
  testrun(out);
  testrun(4 == ov_json_object_count(out));
  val = ov_json_object_get(out, "1");
  testrun(val);
  testrun(2 == ov_json_object_count(val));
  testrun(ov_json_is_true(ov_json_object_get(val, OV_BROADCAST_KEY_BROADCAST)));
  testrun(ov_json_is_true(
      ov_json_object_get(val, OV_BROADCAST_KEY_LOOP_BROADCAST)));
  val = ov_json_object_get(out, "2");
  testrun(val);
  testrun(1 == ov_json_object_count(val));
  testrun(ov_json_is_true(
      ov_json_object_get(val, OV_BROADCAST_KEY_LOOP_BROADCAST)));
  val = ov_json_object_get(out, "3");
  testrun(val);
  testrun(1 == ov_json_object_count(val));
  testrun(ov_json_is_true(
      ov_json_object_get(val, OV_BROADCAST_KEY_ROLE_BROADCAST)));
  val = ov_json_object_get(out, "4");
  testrun(val);
  testrun(1 == ov_json_object_count(val));
  testrun(ov_json_is_true(
      ov_json_object_get(val, OV_BROADCAST_KEY_SYSTEM_BROADCAST)));
  out = ov_json_value_free(out);

  testrun(NULL == ov_event_broadcast_free(b));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int time_ov_event_broadcast_create() {

  uint64_t start = 0;
  uint64_t end = 0;

  start = ov_time_get_current_time_usecs();
  ov_event_broadcast *b =
      ov_event_broadcast_create((ov_event_broadcast_config){0});
  end = ov_time_get_current_time_usecs();
  testrun(b);
  fprintf(stdout, "default %" PRIu64 " usec\n", end - start);
  b = ov_event_broadcast_free(b);

  start = ov_time_get_current_time_usecs();
  b = ov_event_broadcast_create(
      (ov_event_broadcast_config){.max_sockets = 1000});
  end = ov_time_get_current_time_usecs();
  testrun(b);
  fprintf(stdout, "1K %" PRIu64 " usec\n", end - start);
  b = ov_event_broadcast_free(b);

  start = ov_time_get_current_time_usecs();
  b = ov_event_broadcast_create(
      (ov_event_broadcast_config){.max_sockets = 10000});
  end = ov_time_get_current_time_usecs();
  testrun(b);
  fprintf(stdout, "10K %" PRIu64 " usec\n", end - start);
  b = ov_event_broadcast_free(b);

  start = ov_time_get_current_time_usecs();
  b = ov_event_broadcast_create(
      (ov_event_broadcast_config){.max_sockets = 100000});
  end = ov_time_get_current_time_usecs();
  testrun(b);
  fprintf(stdout, "100K %" PRIu64 " usec\n", end - start);
  b = ov_event_broadcast_free(b);

  start = ov_time_get_current_time_usecs();
  b = ov_event_broadcast_create(
      (ov_event_broadcast_config){.max_sockets = 1000000});
  end = ov_time_get_current_time_usecs();
  testrun(b);
  fprintf(stdout, "1M %" PRIu64 " usec\n", end - start);
  b = ov_event_broadcast_free(b);

  /* NOTE 10M slots will allocate 10M * sizeof(uint8_t) bytes
   * which is already >10MB for this test */
  start = ov_time_get_current_time_usecs();
  b = ov_event_broadcast_create(
      (ov_event_broadcast_config){.max_sockets = 10000000});
  end = ov_time_get_current_time_usecs();
  testrun(b);
  fprintf(stdout, "10M %" PRIu64 " usec\n", end - start);
  b = ov_event_broadcast_free(b);

  start = ov_time_get_current_time_usecs();
  b = ov_event_broadcast_create((ov_event_broadcast_config){
      .max_sockets = ov_socket_get_max_supported_runtime_sockets(UINT32_MAX)});
  end = ov_time_get_current_time_usecs();

  if (!b) {

    fprintf(stdout,
            "THIS TEST MAY FAIL WITH OUT OF MEMORY "
            " ON LIMITED TEST RUNTIME ENVIRONMENTS AND IS NO ERROR HERE.");

  } else {

    fprintf(stdout, "MAX RUNTIME %" PRIu32 " sockets in %" PRIu64 " usec\n",
            b->config.max_sockets, end - start);
  }

  b = ov_event_broadcast_free(b);

  /*
      start = ov_time_get_current_time_usecs();
      b = ov_event_broadcast_create(
          (ov_event_broadcast_config){
              .max_sockets = 100000000
          });
      end = ov_time_get_current_time_usecs();
      testrun(b);
      fprintf(stdout, "100000000 %"PRIu64" usec\n", end - start);
      b = ov_event_broadcast_free(b);

      start = ov_time_get_current_time_usecs();
      b = ov_event_broadcast_create(
          (ov_event_broadcast_config){
              .max_sockets = UINT32_MAX
          });
      end = ov_time_get_current_time_usecs();
      testrun(b);
      fprintf(stdout, "max sockets %"PRIu64" usec\n", end - start);
      b = ov_event_broadcast_free(b);
  */

  testrun(NULL == ov_event_broadcast_free(b));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int time_ov_event_broadcast_set() {

  uint64_t start = 0;
  uint64_t end = 0;

  ov_event_broadcast *b = ov_event_broadcast_create(
      (ov_event_broadcast_config){.max_sockets = 1000001});
  testrun(b);

  start = ov_time_get_current_time_usecs();
  testrun(ov_event_broadcast_set(b, 1, 0xff));
  end = ov_time_get_current_time_usecs();
  fprintf(stdout, "SET 1 %" PRIu64 " usec\n", end - start);

  start = ov_time_get_current_time_usecs();
  testrun(ov_event_broadcast_set(b, 1000000, 0xff));
  end = ov_time_get_current_time_usecs();
  fprintf(stdout, "SET 1M %" PRIu64 " usec\n", end - start);

  start = ov_time_get_current_time_usecs();
  testrun(ov_event_broadcast_set(b, 999999, 0x00));
  end = ov_time_get_current_time_usecs();
  fprintf(stdout, "UNSET 999999 %" PRIu64 " usec\n", end - start);

  start = ov_time_get_current_time_usecs();
  testrun(ov_event_broadcast_set(b, 1, 0x00));
  end = ov_time_get_current_time_usecs();
  fprintf(stdout, "UNSET 1 %" PRIu64 " usec\n", end - start);

  // unset last, longest runtime expected
  start = ov_time_get_current_time_usecs();
  testrun(ov_event_broadcast_set(b, 1000000, 0x00));
  end = ov_time_get_current_time_usecs();
  fprintf(stdout, "UNSET 1M %" PRIu64 " usec\n", end - start);

  testrun(ov_event_broadcast_is_empty(b));
  testrun(NULL == ov_event_broadcast_free(b));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int time_ov_event_broadcast_get() {

  uint64_t start = 0;
  uint64_t end = 0;

  ov_event_broadcast *b = ov_event_broadcast_create(
      (ov_event_broadcast_config){.max_sockets = 1000001});
  testrun(b);

  testrun(ov_event_broadcast_set(b, 1, 0xff));
  testrun(ov_event_broadcast_set(b, 1000000, 0xff));

  uint32_t out = 0;

  start = ov_time_get_current_time_usecs();
  out = ov_event_broadcast_get(b, 1);
  end = ov_time_get_current_time_usecs();
  testrun(out == 0xFF);
  fprintf(stdout, "GET 1 %" PRIu64 " usec\n", end - start);

  start = ov_time_get_current_time_usecs();
  out = ov_event_broadcast_get(b, 1000000);
  end = ov_time_get_current_time_usecs();
  testrun(out == 0xFF);
  fprintf(stdout, "GET 1M %" PRIu64 " usec\n", end - start);

  start = ov_time_get_current_time_usecs();
  out = ov_event_broadcast_get(b, 999999);
  end = ov_time_get_current_time_usecs();
  testrun(out == 0x00);
  fprintf(stdout, "GET 999999 %" PRIu64 " usec\n", end - start);

  testrun(NULL == ov_event_broadcast_free(b));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

struct dummy_userdata {

  ov_list *messages;
};

/*----------------------------------------------------------------------------*/

static bool dummy_userdata_clear(struct dummy_userdata *data) {

  if (!data)
    return false;

  data->messages = ov_list_free(data->messages);
  return true;
}

/*----------------------------------------------------------------------------*/

static bool dummy_send_to_userdata(void *userdata, int socket,
                                   const ov_json_value *v) {

  struct dummy_userdata *d = (struct dummy_userdata *)userdata;

  /* This function is simulation a send process, by queing the incoming
   * JSON to some outgoing send list. This is pretty close to the
   * functionality of ov_webserver_multithreaded:mt_channel_send_at_socket */

  UNUSED(socket);
  if (!d || !v)
    goto error;

  if (!d->messages) {
    d->messages =
        ov_list_create((ov_list_config){.item.free = ov_json_value_free});
  }

  ov_json_value *copy = NULL;
  if (!ov_json_value_copy((void **)&copy, v))
    goto error;

  if (!ov_list_push(d->messages, copy)) {
    copy = ov_json_value_free(copy);
    goto error;
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

int time_ov_event_broadcast_send_params() {

  uint64_t start = 0;
  uint64_t end = 0;

  struct dummy_userdata userdata = {0};

  ov_event_parameter send = (ov_event_parameter){
      .send.instance = &userdata, .send.send = dummy_send_to_userdata};

  ov_event_broadcast *b = ov_event_broadcast_create(
      (ov_event_broadcast_config){.max_sockets = 1000001});
  testrun(b);

  const char *teststring = "Nothing set, nothing send";
  ov_json_value *msg = ov_event_api_message_create("dummy", NULL, 0);

  start = ov_time_get_current_time_usecs();
  testrun(ov_event_broadcast_send_params(b, &send, msg, 0xFF)) end =
      ov_time_get_current_time_usecs();
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);
  // verify nothing received
  testrun(0 == userdata.messages);

  teststring = "1 set no interest";
  testrun(ov_event_broadcast_set(b, 1, 0x10));
  start = ov_time_get_current_time_usecs();
  testrun(ov_event_broadcast_send_params(b, &send, msg, 0x01)) end =
      ov_time_get_current_time_usecs();
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);
  // verify nothing received
  testrun(0 == userdata.messages);

  teststring = "1M set no interest";
  testrun(ov_event_broadcast_set(b, 1000000, 0x10));
  start = ov_time_get_current_time_usecs();
  testrun(ov_event_broadcast_send_params(b, &send, msg, 0x01)) end =
      ov_time_get_current_time_usecs();
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);
  // verify nothing received
  testrun(0 == userdata.messages);

  teststring = "1 & 1M with interest";
  start = ov_time_get_current_time_usecs();
  testrun(ov_event_broadcast_send_params(b, &send, msg, 0x10)) end =
      ov_time_get_current_time_usecs();
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);
  testrun(0 != userdata.messages);
  testrun(2 == ov_list_count(userdata.messages));
  testrun(dummy_userdata_clear(&userdata));

  for (int i = 1; i < 1000; i++) {
    testrun(ov_event_broadcast_set(b, i, 0x20));
  }

  teststring = "1 to 1K with interest, 1M without interest";
  start = ov_time_get_current_time_usecs();
  testrun(ov_event_broadcast_send_params(b, &send, msg, 0x20)) end =
      ov_time_get_current_time_usecs();
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);
  testrun(0 != userdata.messages);
  testrun(999 == ov_list_count(userdata.messages));
  testrun(dummy_userdata_clear(&userdata));

  for (int i = 1; i < 10000; i++) {
    testrun(ov_event_broadcast_set(b, i, 0x20));
  }

  teststring = "1 to 10K with interest";
  start = ov_time_get_current_time_usecs();
  testrun(ov_event_broadcast_send_params(b, &send, msg, 0x20)) end =
      ov_time_get_current_time_usecs();
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);
  testrun(0 != userdata.messages);
  testrun(9999 == ov_list_count(userdata.messages));
  testrun(dummy_userdata_clear(&userdata));

  for (int i = 1; i < 100000; i++) {
    testrun(ov_event_broadcast_set(b, i, 0x20));
  }

  teststring = "1 to 100K with interest";
  start = ov_time_get_current_time_usecs();
  testrun(ov_event_broadcast_send_params(b, &send, msg, 0x20)) end =
      ov_time_get_current_time_usecs();
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);
  testrun(0 != userdata.messages);
  testrun(99999 == ov_list_count(userdata.messages));
  testrun(dummy_userdata_clear(&userdata));

  for (int i = 1; i < 100000; i++) {
    testrun(ov_event_broadcast_set(b, i, 0x00));
    if (0 == i % 2)
      testrun(ov_event_broadcast_set(b, i, 0x30));
  }

  teststring = "1 to 100K every second with interest";
  start = ov_time_get_current_time_usecs();
  testrun(ov_event_broadcast_send_params(b, &send, msg, 0x30)) end =
      ov_time_get_current_time_usecs();
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);
  testrun(0 != userdata.messages);
  testrun(50000 == ov_list_count(userdata.messages));
  testrun(dummy_userdata_clear(&userdata));

  for (int i = 1; i < 100000; i++) {
    testrun(ov_event_broadcast_set(b, i, 0x00));
    if (0 == i % 4)
      testrun(ov_event_broadcast_set(b, i, 0x30));
  }

  teststring = "1 to 100K every forth with interest";
  start = ov_time_get_current_time_usecs();
  testrun(ov_event_broadcast_send_params(b, &send, msg, 0x30)) end =
      ov_time_get_current_time_usecs();
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);
  testrun(0 != userdata.messages);
  testrun(25000 == ov_list_count(userdata.messages));
  testrun(dummy_userdata_clear(&userdata));
  /*
      for (int i = 1; i < 1000000; i++){
          testrun(ov_event_broadcast_set(b, i, 0x20));
      }

      teststring = "1 to 1M with interest";
      start = ov_time_get_current_time_usecs();
      testrun(ov_event_broadcast_send_params(b, &send, msg, 0x20))
      end = ov_time_get_current_time_usecs();
      fprintf(stdout, "%s %"PRIu64" usec\n", teststring, end - start);
      testrun(0 != userdata.messages);
      testrun(999999 == ov_list_count(userdata.messages));
      testrun(dummy_userdata_clear(&userdata));
  */

  msg = ov_json_value_free(msg);
  testrun(NULL == ov_event_broadcast_free(b));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_ov_event_broadcast_send_params_bit_masking() {

  struct dummy_userdata userdata = {0};

  ov_event_parameter send = (ov_event_parameter){
      .send.instance = &userdata, .send.send = dummy_send_to_userdata};

  ov_event_broadcast *b = ov_event_broadcast_create(
      (ov_event_broadcast_config){.max_sockets = 1000001});
  testrun(b);

  ov_json_value *msg = ov_event_api_message_create("dummy", NULL, 0);

  testrun(ov_event_broadcast_send_params(b, &send, msg, 0x30))
      testrun(0 == ov_list_count(userdata.messages));

  testrun(ov_event_broadcast_set(b, 1, 0x30));
  testrun(ov_event_broadcast_send_params(b, &send, msg, 0x30))
      testrun(1 == ov_list_count(userdata.messages));
  testrun(dummy_userdata_clear(&userdata));

  for (uint8_t i = 1; i < 0xff; i++) {

    testrun(ov_event_broadcast_send_params(b, &send, msg, i));

    if (i & 0x30) {
      testrun(1 == ov_list_count(userdata.messages));
    } else {
      testrun(0 == ov_list_count(userdata.messages));
    }
    testrun(dummy_userdata_clear(&userdata));
  }

  testrun(ov_event_broadcast_set(b, 1, 0x11));

  for (uint8_t i = 1; i < 0xff; i++) {

    testrun(ov_event_broadcast_send_params(b, &send, msg, i));

    if ((i & 0x01) || (i & 0x10)) {
      testrun(1 == ov_list_count(userdata.messages));
    } else {
      testrun(0 == ov_list_count(userdata.messages));
    }
    testrun(dummy_userdata_clear(&userdata));
  }

  testrun(ov_event_broadcast_set(b, 2, 0x01));

  for (uint8_t i = 1; i < 0xff; i++) {

    testrun(ov_event_broadcast_send_params(b, &send, msg, i));

    if (i & 0x01) {

      testrun(2 == ov_list_count(userdata.messages));

    } else if (i & 0x10) {

      testrun(1 == ov_list_count(userdata.messages));

    } else {

      testrun(0 == ov_list_count(userdata.messages));
    }

    testrun(dummy_userdata_clear(&userdata));
  }

  testrun(ov_event_broadcast_set(b, 3, 0x02));

  for (uint8_t i = 1; i < 0xff; i++) {

    testrun(ov_event_broadcast_send_params(b, &send, msg, i));

    if (i & 0x01) {

      if (i & 0x02) {
        testrun(3 == ov_list_count(userdata.messages));
      } else {
        testrun(2 == ov_list_count(userdata.messages));
      }

    } else if (i & 0x10) {

      if (i & 0x02) {
        testrun(2 == ov_list_count(userdata.messages));
      } else {
        testrun(1 == ov_list_count(userdata.messages));
      }

    } else if (i & 0x02) {

      if (i & 0x10) {
        testrun(2 == ov_list_count(userdata.messages));
      } else {
        testrun(1 == ov_list_count(userdata.messages));
      }

    } else {

      testrun(0 == ov_list_count(userdata.messages));
    }

    testrun(dummy_userdata_clear(&userdata));
  }

  testrun(ov_event_broadcast_send_params(b, &send, msg, 0xff));
  testrun(3 == ov_list_count(userdata.messages));
  testrun(dummy_userdata_clear(&userdata));

  testrun(ov_event_broadcast_send_params(b, &send, msg, 0x20));
  testrun(0 == ov_list_count(userdata.messages));
  testrun(dummy_userdata_clear(&userdata));

  msg = ov_json_value_free(msg);
  testrun(NULL == ov_event_broadcast_free(b));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int time_ov_event_broadcast_is_empty() {

  uint64_t start = 0;
  uint64_t end = 0;

  ov_event_broadcast *b = ov_event_broadcast_create(
      (ov_event_broadcast_config){.max_sockets = 1000001});
  testrun(b);

  const char *teststring = "Nothing set";

  start = ov_time_get_current_time_usecs();
  testrun(ov_event_broadcast_is_empty(b)) end =
      ov_time_get_current_time_usecs();
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);

  testrun(ov_event_broadcast_set(b, 1, 0x01));
  teststring = "1 set";
  start = ov_time_get_current_time_usecs();
  testrun(!ov_event_broadcast_is_empty(b)) end =
      ov_time_get_current_time_usecs();
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);

  testrun(ov_event_broadcast_set(b, 1000000, 0x01));
  teststring = "1M set";
  start = ov_time_get_current_time_usecs();
  testrun(!ov_event_broadcast_is_empty(b)) end =
      ov_time_get_current_time_usecs();
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);

  /*
      for (int i = 1; i < 1000000; i++){
          testrun(ov_event_broadcast_set(b, i, 0x20));
      }

      teststring = "1to 1M set";
      start = ov_time_get_current_time_usecs();
      testrun(!ov_event_broadcast_is_empty(b))
      end = ov_time_get_current_time_usecs();
      fprintf(stdout, "%s %"PRIu64" usec\n", teststring, end - start);

      for (int i = 1; i < 1000000; i++){
          testrun(ov_event_broadcast_set(b, i, 0x00));
      }

      testrun(ov_event_broadcast_set(b, 1000000, 0x01));
      teststring = "Only 1M set";
      start = ov_time_get_current_time_usecs();
      testrun(!ov_event_broadcast_is_empty(b))
      end = ov_time_get_current_time_usecs();
      fprintf(stdout, "%s %"PRIu64" usec\n", teststring, end - start);
  */

  testrun(NULL == ov_event_broadcast_free(b));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int time_ov_event_broadcast_count() {

  uint64_t start = 0;
  uint64_t end = 0;

  ov_event_broadcast *b = ov_event_broadcast_create(
      (ov_event_broadcast_config){.max_sockets = 1000001});
  testrun(b);

  const char *teststring = "Nothing set";
  int64_t result = 0;

  start = ov_time_get_current_time_usecs();
  result = ov_event_broadcast_count(b, 0x01);
  end = ov_time_get_current_time_usecs();
  testrun(0 == result);
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);

  testrun(ov_event_broadcast_set(b, 1, 0x01));
  testrun(ov_event_broadcast_set(b, 2, 0x02));
  testrun(ov_event_broadcast_set(b, 3, 0x03));
  testrun(ov_event_broadcast_set(b, 4, 0x04));

  teststring = "0x01 twice";
  start = ov_time_get_current_time_usecs();
  result = ov_event_broadcast_count(b, 0x01);
  end = ov_time_get_current_time_usecs();
  testrun(2 == result);
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);

  teststring = "0x02 twice";
  start = ov_time_get_current_time_usecs();
  result = ov_event_broadcast_count(b, 0x02);
  end = ov_time_get_current_time_usecs();
  testrun(2 == result);
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);

  teststring = "0x03 three";
  start = ov_time_get_current_time_usecs();
  result = ov_event_broadcast_count(b, 0x03);
  end = ov_time_get_current_time_usecs();
  testrun(3 == result);
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);

  teststring = "0x04 one";
  start = ov_time_get_current_time_usecs();
  result = ov_event_broadcast_count(b, 0x04);
  end = ov_time_get_current_time_usecs();
  testrun(1 == result);
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);

  testrun(ov_event_broadcast_set(b, 1000000, 0x01));
  testrun(ov_event_broadcast_set(b, 1000, 0x02));
  testrun(ov_event_broadcast_set(b, 50, 0x03));
  testrun(ov_event_broadcast_set(b, 51, 0x04));

  teststring = "0x01 four";
  start = ov_time_get_current_time_usecs();
  result = ov_event_broadcast_count(b, 0x01);
  end = ov_time_get_current_time_usecs();
  testrun(4 == result);
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);

  teststring = "0x02 four";
  start = ov_time_get_current_time_usecs();
  result = ov_event_broadcast_count(b, 0x02);
  end = ov_time_get_current_time_usecs();
  testrun(4 == result);
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);

  teststring = "0x03 six";
  start = ov_time_get_current_time_usecs();
  result = ov_event_broadcast_count(b, 0x03);
  end = ov_time_get_current_time_usecs();
  testrun(6 == result);
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);

  teststring = "0x04 twice";
  start = ov_time_get_current_time_usecs();
  result = ov_event_broadcast_count(b, 0x04);
  end = ov_time_get_current_time_usecs();
  testrun(2 == result);
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);

  testrun(NULL == ov_event_broadcast_free(b));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int time_ov_event_broadcast_state() {

  uint64_t start = 0;
  uint64_t end = 0;

  ov_event_broadcast *b = ov_event_broadcast_create(
      (ov_event_broadcast_config){.max_sockets = 1000001});
  testrun(b);

  ov_json_value *out = NULL;
  const char *teststring = "Nothing set";

  start = ov_time_get_current_time_usecs();
  out = ov_event_broadcast_state(b);
  end = ov_time_get_current_time_usecs();
  testrun(out);
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);
  out = ov_json_value_free(out);

  testrun(ov_event_broadcast_set(b, 1, 0x01));
  teststring = "One set";
  start = ov_time_get_current_time_usecs();
  out = ov_event_broadcast_state(b);
  end = ov_time_get_current_time_usecs();
  testrun(out);
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);
  out = ov_json_value_free(out);

  for (size_t i = 1; i < 10; i++) {

    testrun(ov_event_broadcast_set(b, i, 0x01));
  }

  teststring = "One to 10 set";
  start = ov_time_get_current_time_usecs();
  out = ov_event_broadcast_state(b);
  end = ov_time_get_current_time_usecs();
  testrun(out);
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);
  out = ov_json_value_free(out);

  for (size_t i = 1; i < 10; i++) {

    testrun(ov_event_broadcast_set(b, i, 0xFF));
  }

  teststring = "One to 10 - all set";
  start = ov_time_get_current_time_usecs();
  out = ov_event_broadcast_state(b);
  end = ov_time_get_current_time_usecs();
  testrun(out);
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);
  out = ov_json_value_free(out);

  for (size_t i = 1; i < 10000; i++) {

    testrun(ov_event_broadcast_set(b, i, 0xFF));
  }

  teststring = "One to 10K - all set";
  start = ov_time_get_current_time_usecs();
  out = ov_event_broadcast_state(b);
  end = ov_time_get_current_time_usecs();
  testrun(out);
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);
  out = ov_json_value_free(out);

  /*
      for (size_t i = 1; i < 100000; i++){

          testrun(ov_event_broadcast_set(b, i, 0xFF));
      }

      teststring = "One to 100K - all set";
      start = ov_time_get_current_time_usecs();
      out = ov_event_broadcast_state(b);
      end = ov_time_get_current_time_usecs();
      testrun(out);
      fprintf(stdout, "%s %"PRIu64" usec\n", teststring, end - start);
      out = ov_json_value_free(out);

      for (size_t i = 1; i < 1000000; i++){

          testrun(ov_event_broadcast_set(b, i, 0xFF));
      }

      teststring = "One to 1M - all set";
      start = ov_time_get_current_time_usecs();
      out = ov_event_broadcast_state(b);
      end = ov_time_get_current_time_usecs();
      testrun(out);
      fprintf(stdout, "%s %"PRIu64" usec\n", teststring, end - start);
      out = ov_json_value_free(out);

  */
  testrun(NULL == ov_event_broadcast_free(b));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_broadcast_get_sockets() {

  uint64_t start = 0;
  uint64_t end = 0;

  ov_event_broadcast *b = ov_event_broadcast_create(
      (ov_event_broadcast_config){.max_sockets = 1000001});
  testrun(b);

  ov_list *list = NULL;

  testrun(!ov_event_broadcast_get_sockets(NULL, 0));
  testrun(!ov_event_broadcast_get_sockets(b, OV_BROADCAST_UNSET));
  testrun(!ov_event_broadcast_get_sockets(NULL, 0x01));

  char *teststring = "Empty";

  start = ov_time_get_current_time_usecs();
  list = ov_event_broadcast_get_sockets(b, 0x01);
  end = ov_time_get_current_time_usecs();
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);
  testrun(list);
  testrun(ov_list_is_empty(list));
  list = ov_list_free(list);

  teststring = "1 0xFF";
  testrun(ov_event_broadcast_set(b, 1, 0xFF));

  for (uint8_t i = 1; i < 0xff; i++) {

    start = ov_time_get_current_time_usecs();
    list = ov_event_broadcast_get_sockets(b, i);
    end = ov_time_get_current_time_usecs();
    // fprintf(stdout, "%s %i %"PRIu64" usec\n", teststring,i, end - start);
    testrun(list);
    testrun(1 == ov_list_count(list));
    testrun(1 == (intptr_t)ov_list_pop(list));
    list = ov_list_free(list);
  }

  teststring = "1 0xFF 1M 0x01";
  testrun(ov_event_broadcast_set(b, 1000000, 0x01));

  for (uint8_t i = 1; i < 0xff; i++) {

    start = ov_time_get_current_time_usecs();
    list = ov_event_broadcast_get_sockets(b, i);
    end = ov_time_get_current_time_usecs();
    // fprintf(stdout, "%s %i %"PRIu64" usec\n", teststring,i, end - start);
    testrun(list);

    if (i & 0x01) {

      testrun(2 == ov_list_count(list));
      testrun(1000000 == (intptr_t)ov_list_pop(list));
      testrun(1 == (intptr_t)ov_list_pop(list));

    } else {

      testrun(1 == ov_list_count(list));
      testrun(1 == (intptr_t)ov_list_pop(list));
    }

    list = ov_list_free(list);
  }

  testrun(ov_event_broadcast_set(b, 1000000, 0x00));

  for (int i = 0; i < 1000000; i++) {
    testrun(ov_event_broadcast_set(b, i, 0x01));
  }

  teststring = "1M set";
  start = ov_time_get_current_time_usecs();
  list = ov_event_broadcast_get_sockets(b, 0x01);
  end = ov_time_get_current_time_usecs();
  fprintf(stdout, "%s %" PRIu64 " usec\n", teststring, end - start);
  testrun(list);
  testrun(1000000 == ov_list_count(list));
  testrun(0 == (intptr_t)ov_list_get(list, 1));
  testrun(10 == (intptr_t)ov_list_get(list, 11));
  testrun(36 == (intptr_t)ov_list_get(list, 37));
  testrun(999999 == (intptr_t)ov_list_pop(list));
  list = ov_list_free(list);

  testrun(NULL == ov_event_broadcast_free(b));
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

  testrun_test(check_ov_event_broadcast_send_params_bit_masking);

  testrun_test(test_ov_event_broadcast_create);
  testrun_test(test_ov_event_broadcast_free);
  testrun_test(test_ov_event_broadcast_configure_uri_event_io);
  testrun_test(test_ov_event_broadcast_set);
  testrun_test(test_ov_event_broadcast_get);
  testrun_test(test_ov_event_broadcast_send_params);
  testrun_test(test_ov_event_broadcast_is_empty);
  testrun_test(test_ov_event_broadcast_count);
  testrun_test(test_ov_event_broadcast_state);

  testrun_test(time_ov_event_broadcast_create);
  testrun_test(time_ov_event_broadcast_set);
  testrun_test(time_ov_event_broadcast_get);
  testrun_test(time_ov_event_broadcast_is_empty);
  testrun_test(time_ov_event_broadcast_count);

  testrun_test(time_ov_event_broadcast_send_params);
  testrun_test(time_ov_event_broadcast_state);

  testrun_test(test_ov_event_broadcast_get_sockets);
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
