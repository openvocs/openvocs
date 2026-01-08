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
        @file           ov_mc_frontend_registry_test.c
        @author         Markus Töpfer

        @date           2023-01-23


        ------------------------------------------------------------------------
*/
#include "ov_mc_frontend_registry.c"
#include <ov_test/testrun.h>

struct userdata {

  ov_id uuid;
};

/*----------------------------------------------------------------------------*/

static bool userdata_clear(struct userdata *data) {

  memset(data->uuid, 0, sizeof(ov_id));
  return true;
};

/*----------------------------------------------------------------------------*/

static void cb_drop(void *userdata, const char *uuid) {

  struct userdata *data = (struct userdata *)userdata;
  if (uuid)
    memcpy(data->uuid, uuid, sizeof(ov_id));
  return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_mc_frontend_registry_create() {

  struct userdata userdata = {0};

  ov_mc_frontend_registry *reg =
      ov_mc_frontend_registry_create((ov_mc_frontend_registry_config){
          .callback.userdata = &userdata, .callback.session.drop = cb_drop});

  testrun(reg);
  testrun(ov_mc_frontend_registry_cast(reg));
  testrun(reg->sessions);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_registry_free(reg));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_frontend_registry_free() {

  struct userdata userdata = {0};

  ov_mc_frontend_registry *reg =
      ov_mc_frontend_registry_create((ov_mc_frontend_registry_config){
          .callback.userdata = &userdata, .callback.session.drop = cb_drop});

  testrun(reg);
  testrun(ov_mc_frontend_registry_cast(reg));
  testrun(NULL == ov_mc_frontend_registry_free(reg));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_frontend_registry_cast() {

  struct userdata userdata = {0};

  ov_mc_frontend_registry *reg =
      ov_mc_frontend_registry_create((ov_mc_frontend_registry_config){
          .callback.userdata = &userdata, .callback.session.drop = cb_drop});

  testrun(reg);
  testrun(ov_mc_frontend_registry_cast(reg));

  for (size_t i = 0; i < 0xffff; i++) {

    reg->magic_bytes = i;
    if (i == OV_MC_FRONTEND_REGISTRY_MAGIC_BYTES) {
      testrun(ov_mc_frontend_registry_cast(reg));
    } else {
      testrun(!ov_mc_frontend_registry_cast(reg));
    }
  }

  reg->magic_bytes = OV_MC_FRONTEND_REGISTRY_MAGIC_BYTES;
  testrun(NULL == ov_mc_frontend_registry_free(reg));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_frontend_registry_register_proxy() {

  struct userdata userdata = {0};

  ov_mc_frontend_registry *reg =
      ov_mc_frontend_registry_create((ov_mc_frontend_registry_config){
          .callback.userdata = &userdata, .callback.session.drop = cb_drop});

  testrun(reg);
  testrun(ov_mc_frontend_registry_cast(reg));

  testrun(!ov_mc_frontend_registry_register_proxy(NULL, 0, NULL));
  testrun(!ov_mc_frontend_registry_register_proxy(reg, 0, NULL));
  testrun(!ov_mc_frontend_registry_register_proxy(NULL, 0, "1-1-1-1"));
  testrun(!ov_mc_frontend_registry_register_proxy(reg, 0, "1-1-1-1"));

  testrun(ov_mc_frontend_registry_register_proxy(reg, 1, "1-1-1-1"));
  testrun(1 == ov_mc_frontend_registry_get_proxy_socket(reg));

  IceProxyConnection *conn = &reg->socket[1];
  testrun(conn->socket == 1);
  testrun(0 == strcmp(conn->uuid, "1-1-1-1"));
  testrun(conn->sessions);

  testrun(!ov_mc_frontend_registry_register_proxy(reg, 1, "2-2-2-2"));

  testrun(NULL == ov_mc_frontend_registry_free(reg));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_frontend_registry_unregister_proxy() {

  struct userdata userdata = {0};

  ov_mc_frontend_registry *reg =
      ov_mc_frontend_registry_create((ov_mc_frontend_registry_config){
          .callback.userdata = &userdata, .callback.session.drop = cb_drop});

  testrun(reg);
  testrun(ov_mc_frontend_registry_cast(reg));

  testrun(!ov_mc_frontend_registry_unregister_proxy(NULL, 0));
  testrun(!ov_mc_frontend_registry_unregister_proxy(reg, 0));
  testrun(!ov_mc_frontend_registry_unregister_proxy(NULL, 1));

  IceProxyConnection *conn = &reg->socket[1];

  // check no proxy registered
  testrun(ov_mc_frontend_registry_unregister_proxy(reg, 1));
  testrun(-1 == conn->socket);
  testrun(0 == conn->uuid[0]);
  testrun(NULL == conn->sessions);

  testrun(ov_mc_frontend_registry_register_proxy(reg, 1, "1-1-1-1"));
  testrun(1 == ov_mc_frontend_registry_get_proxy_socket(reg));
  testrun(conn->socket == 1);
  testrun(0 == strcmp(conn->uuid, "1-1-1-1"));
  testrun(conn->sessions);

  // check proxy registered
  testrun(ov_mc_frontend_registry_unregister_proxy(reg, 1));
  testrun(-1 == conn->socket);
  testrun(0 == conn->uuid[0]);
  testrun(NULL == conn->sessions);

  testrun(NULL == ov_mc_frontend_registry_free(reg));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_frontend_registry_get_proxy_socket() {

  struct userdata userdata = {0};

  ov_mc_frontend_registry *reg =
      ov_mc_frontend_registry_create((ov_mc_frontend_registry_config){
          .callback.userdata = &userdata, .callback.session.drop = cb_drop});

  testrun(reg);
  testrun(ov_mc_frontend_registry_cast(reg));

  testrun(-1 == ov_mc_frontend_registry_get_proxy_socket(NULL));
  testrun(-1 == ov_mc_frontend_registry_get_proxy_socket(reg));

  testrun(ov_mc_frontend_registry_register_proxy(reg, 1, "1-1-1-1"));
  testrun(ov_mc_frontend_registry_register_proxy(reg, 2, "2-1-1-1"));
  testrun(ov_mc_frontend_registry_register_proxy(reg, 3, "3-1-1-1"));

  testrun(1 == ov_mc_frontend_registry_get_proxy_socket(reg));

  testrun(ov_mc_frontend_registry_register_session(reg, 1, "session1"));
  testrun(2 == ov_mc_frontend_registry_get_proxy_socket(reg));

  testrun(ov_mc_frontend_registry_register_session(reg, 2, "session2"));
  testrun(3 == ov_mc_frontend_registry_get_proxy_socket(reg));

  testrun(ov_mc_frontend_registry_register_session(reg, 3, "session3"));
  testrun(1 == ov_mc_frontend_registry_get_proxy_socket(reg));

  testrun(ov_mc_frontend_registry_register_session(reg, 1, "session4"));
  testrun(2 == ov_mc_frontend_registry_get_proxy_socket(reg));

  testrun(ov_mc_frontend_registry_register_session(reg, 1, "session5"));
  testrun(2 == ov_mc_frontend_registry_get_proxy_socket(reg));

  testrun(ov_mc_frontend_registry_register_session(reg, 2, "session5"));
  testrun(3 == ov_mc_frontend_registry_get_proxy_socket(reg));

  testrun(ov_mc_frontend_registry_register_session(reg, 3, "session6"));
  testrun(2 == ov_mc_frontend_registry_get_proxy_socket(reg));

  testrun(NULL == ov_mc_frontend_registry_free(reg));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_frontend_registry_register_session() {

  struct userdata userdata = {0};

  ov_mc_frontend_registry *reg =
      ov_mc_frontend_registry_create((ov_mc_frontend_registry_config){
          .callback.userdata = &userdata, .callback.session.drop = cb_drop});

  testrun(reg);
  testrun(ov_mc_frontend_registry_cast(reg));

  testrun(!ov_mc_frontend_registry_register_session(NULL, 1, "session1"));
  testrun(!ov_mc_frontend_registry_register_session(reg, 0, "session1"));
  testrun(!ov_mc_frontend_registry_register_session(reg, 1, NULL));

  // proxy not registered
  testrun(!ov_mc_frontend_registry_register_session(reg, 1, "session1"));

  testrun(ov_mc_frontend_registry_register_proxy(reg, 1, "1-1-1-1"));
  testrun(ov_mc_frontend_registry_register_proxy(reg, 2, "2-1-1-1"));
  testrun(ov_mc_frontend_registry_register_proxy(reg, 3, "3-1-1-1"));

  testrun(ov_mc_frontend_registry_register_session(reg, 1, "session1"));

  IceProxyConnection *conn = &reg->socket[1];
  testrun(1 == (intptr_t)ov_dict_get(reg->sessions, "session1"));
  IceSession *sess = ov_dict_get(conn->sessions, "session1");
  testrun(sess);
  testrun(0 == strcmp(sess->uuid, "session1"));
  testrun(reg = sess->registry);

  testrun(ov_mc_frontend_registry_register_session(reg, 1, "session2"));

  testrun(1 == (intptr_t)ov_dict_get(reg->sessions, "session1"));
  testrun(1 == (intptr_t)ov_dict_get(reg->sessions, "session2"));
  testrun(ov_dict_get(conn->sessions, "session1"));
  testrun(ov_dict_get(conn->sessions, "session2"));

  testrun(NULL == ov_mc_frontend_registry_free(reg));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_frontend_registry_unregister_session() {

  struct userdata userdata = {0};

  ov_mc_frontend_registry *reg =
      ov_mc_frontend_registry_create((ov_mc_frontend_registry_config){
          .callback.userdata = &userdata, .callback.session.drop = cb_drop});

  testrun(reg);
  testrun(ov_mc_frontend_registry_cast(reg));

  testrun(!ov_mc_frontend_registry_unregister_session(NULL, "session1"));
  testrun(!ov_mc_frontend_registry_unregister_session(reg, NULL));

  // session not registered
  testrun(ov_mc_frontend_registry_unregister_session(reg, "session1"));

  testrun(ov_mc_frontend_registry_register_proxy(reg, 1, "1-1-1-1"));
  testrun(ov_mc_frontend_registry_register_proxy(reg, 2, "2-1-1-1"));
  testrun(ov_mc_frontend_registry_register_proxy(reg, 3, "3-1-1-1"));

  testrun(ov_mc_frontend_registry_register_session(reg, 1, "session1"));
  testrun(ov_mc_frontend_registry_register_session(reg, 1, "session2"));
  testrun(ov_mc_frontend_registry_register_session(reg, 2, "session3"));

  testrun(1 == (intptr_t)ov_dict_get(reg->sessions, "session1"));
  testrun(1 == (intptr_t)ov_dict_get(reg->sessions, "session2"));
  testrun(2 == (intptr_t)ov_dict_get(reg->sessions, "session3"));
  testrun(ov_dict_get(reg->socket[1].sessions, "session1"));
  testrun(ov_dict_get(reg->socket[1].sessions, "session2"));
  testrun(ov_dict_get(reg->socket[2].sessions, "session3"));

  testrun(ov_mc_frontend_registry_unregister_session(reg, "session1"));

  testrun(0 == (intptr_t)ov_dict_get(reg->sessions, "session1"));
  testrun(1 == (intptr_t)ov_dict_get(reg->sessions, "session2"));
  testrun(2 == (intptr_t)ov_dict_get(reg->sessions, "session3"));
  testrun(!ov_dict_get(reg->socket[1].sessions, "session1"));
  testrun(ov_dict_get(reg->socket[1].sessions, "session2"));
  testrun(ov_dict_get(reg->socket[2].sessions, "session3"));

  testrun(ov_mc_frontend_registry_unregister_session(reg, "session2"));

  testrun(0 == (intptr_t)ov_dict_get(reg->sessions, "session1"));
  testrun(0 == (intptr_t)ov_dict_get(reg->sessions, "session2"));
  testrun(2 == (intptr_t)ov_dict_get(reg->sessions, "session3"));
  testrun(!ov_dict_get(reg->socket[1].sessions, "session1"));
  testrun(!ov_dict_get(reg->socket[1].sessions, "session2"));
  testrun(ov_dict_get(reg->socket[2].sessions, "session3"));

  testrun(ov_mc_frontend_registry_unregister_session(reg, "session3"));

  testrun(0 == (intptr_t)ov_dict_get(reg->sessions, "session1"));
  testrun(0 == (intptr_t)ov_dict_get(reg->sessions, "session2"));
  testrun(0 == (intptr_t)ov_dict_get(reg->sessions, "session3"));
  testrun(!ov_dict_get(reg->socket[1].sessions, "session1"));
  testrun(!ov_dict_get(reg->socket[1].sessions, "session2"));
  testrun(!ov_dict_get(reg->socket[2].sessions, "session3"));

  testrun(NULL == ov_mc_frontend_registry_free(reg));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_frontend_registry_get_session_socket() {

  struct userdata userdata = {0};

  ov_mc_frontend_registry *reg =
      ov_mc_frontend_registry_create((ov_mc_frontend_registry_config){
          .callback.userdata = &userdata, .callback.session.drop = cb_drop});

  testrun(reg);
  testrun(ov_mc_frontend_registry_cast(reg));

  // session not registered
  testrun(0 == ov_mc_frontend_registry_get_session_socket(reg, "session1"));

  testrun(ov_mc_frontend_registry_register_proxy(reg, 1, "1-1-1-1"));
  testrun(ov_mc_frontend_registry_register_proxy(reg, 2, "2-1-1-1"));
  testrun(ov_mc_frontend_registry_register_proxy(reg, 3, "3-1-1-1"));

  testrun(ov_mc_frontend_registry_register_session(reg, 1, "session1"));
  testrun(ov_mc_frontend_registry_register_session(reg, 1, "session2"));
  testrun(ov_mc_frontend_registry_register_session(reg, 2, "session3"));

  testrun(1 == ov_mc_frontend_registry_get_session_socket(reg, "session1"));
  testrun(1 == ov_mc_frontend_registry_get_session_socket(reg, "session2"));
  testrun(2 == ov_mc_frontend_registry_get_session_socket(reg, "session3"));

  testrun(NULL == ov_mc_frontend_registry_free(reg));

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
  testrun_test(test_ov_mc_frontend_registry_create);
  testrun_test(test_ov_mc_frontend_registry_free);
  testrun_test(test_ov_mc_frontend_registry_cast);

  testrun_test(test_ov_mc_frontend_registry_register_proxy);
  testrun_test(test_ov_mc_frontend_registry_unregister_proxy);

  testrun_test(test_ov_mc_frontend_registry_get_proxy_socket);

  testrun_test(test_ov_mc_frontend_registry_register_session);
  testrun_test(test_ov_mc_frontend_registry_unregister_session);
  testrun_test(test_ov_mc_frontend_registry_get_session_socket);

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
