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
        @file           ov_mc_frontend_test.c
        @author         Markus Töpfer

        @date           2023-01-22


        ------------------------------------------------------------------------
*/
#include "ov_mc_frontend.c"
#include <ov_test/testrun.h>

#include "../include/ov_mc_mixer_core.h"

struct userdata {

  ov_id session_id;
  ov_id uuid;
  uint64_t error_code;
  char *error_desc;
  char *type;
  char *sdp;
  ov_ice_proxy_vocs_stream_forward_data forward;
  bool success;
  ov_json_value *info;
  ov_mc_loop_data data;
};

/*----------------------------------------------------------------------------*/

static bool userdata_clear(struct userdata *userdata) {

  if (!userdata)
    return false;

  memset(userdata->session_id, 0, sizeof(ov_id));
  memset(userdata->uuid, 0, sizeof(ov_id));
  userdata->error_code = 0;
  userdata->error_desc = ov_data_pointer_free(userdata->error_desc);
  userdata->type = ov_data_pointer_free(userdata->type);
  userdata->sdp = ov_data_pointer_free(userdata->sdp);
  userdata->forward = (ov_ice_proxy_vocs_stream_forward_data){0};
  userdata->success = false;
  userdata->info = ov_json_value_free(userdata->info);
  userdata->data = (ov_mc_loop_data){0};
  return true;
}

/*----------------------------------------------------------------------------*/

static void cb_session_dropped(void *userdata, const char *session_id) {

  struct userdata *data = (struct userdata *)userdata;
  if (session_id)
    strncpy(data->session_id, session_id, sizeof(ov_id));
  return;
}

/*----------------------------------------------------------------------------*/

static void
cb_session_created(void *userdata, const ov_response_state event,
                   const char *session_id, const char *type, const char *sdp,
                   size_t array_size,
                   const ov_ice_proxy_vocs_stream_forward_data *array) {

  UNUSED(array_size);

  struct userdata *data = (struct userdata *)userdata;
  if (session_id)
    strncpy(data->session_id, session_id, sizeof(ov_id));
  if (event.id)
    strncpy(data->uuid, event.id, sizeof(ov_id));
  data->error_code = event.result.error_code;
  data->error_desc = ov_string_dup(event.result.message);
  data->type = ov_string_dup(type);
  data->sdp = ov_string_dup(sdp);
  if (array)
    data->forward = array[0];
  return;
}

/*----------------------------------------------------------------------------*/

static void cb_session_completed(void *userdata, const char *session_id,
                                 bool success) {

  struct userdata *data = (struct userdata *)userdata;
  if (session_id)
    strncpy(data->session_id, session_id, sizeof(ov_id));
  data->success = success;
  return;
}

/*----------------------------------------------------------------------------*/

static void cb_session_update(void *userdata, const ov_response_state event,
                              const char *session_id) {

  struct userdata *data = (struct userdata *)userdata;
  if (session_id)
    strncpy(data->session_id, session_id, sizeof(ov_id));
  if (event.id)
    strncpy(data->uuid, event.id, sizeof(ov_id));
  data->error_code = event.result.error_code;
  data->error_desc = ov_string_dup(event.result.message);
  return;
}

/*----------------------------------------------------------------------------*/

static void cb_session_state(void *userdata, const ov_response_state event,
                             const char *session_id, const ov_json_value *msg) {

  UNUSED(msg);

  struct userdata *data = (struct userdata *)userdata;
  if (session_id)
    strncpy(data->session_id, session_id, sizeof(ov_id));
  if (event.id)
    strncpy(data->uuid, event.id, sizeof(ov_id));
  data->error_code = event.result.error_code;
  data->error_desc = ov_string_dup(event.result.message);
  return;
}

/*----------------------------------------------------------------------------*/

static void cb_candidate(void *userdata, const ov_response_state event,
                         const char *session_id,
                         const ov_ice_candidate_info *info) {

  struct userdata *data = (struct userdata *)userdata;
  if (session_id)
    strncpy(data->session_id, session_id, sizeof(ov_id));
  if (event.id)
    strncpy(data->uuid, event.id, sizeof(ov_id));
  data->error_code = event.result.error_code;
  data->error_desc = ov_string_dup(event.result.message);
  data->info = ov_ice_candidate_info_to_json(*info);
  return;
}

/*----------------------------------------------------------------------------*/

static void cb_end_of_candidates(void *userdata, const ov_response_state event,
                                 const char *session_id,
                                 const ov_ice_candidate_info *info) {

  struct userdata *data = (struct userdata *)userdata;
  if (session_id)
    strncpy(data->session_id, session_id, sizeof(ov_id));
  if (event.id)
    strncpy(data->uuid, event.id, sizeof(ov_id));
  data->error_code = event.result.error_code;
  data->error_desc = ov_string_dup(event.result.message);
  data->info = ov_ice_candidate_info_to_json(*info);
  return;
}

/*----------------------------------------------------------------------------*/

static void cb_talk(void *userdata, const ov_response_state event,
                    const char *session_id, const ov_mc_loop_data ldata,
                    bool on) {

  struct userdata *data = (struct userdata *)userdata;
  if (session_id)
    strncpy(data->session_id, session_id, sizeof(ov_id));
  if (event.id)
    strncpy(data->uuid, event.id, sizeof(ov_id));
  data->error_code = event.result.error_code;
  data->error_desc = ov_string_dup(event.result.message);
  data->success = on;
  data->data = ldata;
  return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_mc_frontend_create() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);
  testrun(ov_mc_frontend_cast(frontend));
  testrun(frontend->app);
  testrun(-1 != frontend->socket.manager);
  testrun(frontend->registry);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_frontend_free() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);

  TODO("... free with data set.");

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_frontend_cast() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);
  testrun(ov_mc_frontend_cast(frontend));

  for (size_t i = 0; i < 0xffff; i++) {

    frontend->magic_bytes = i;
    if (i == OV_MC_FRONTEND_MAGIC_BYTES) {
      testrun(ov_mc_frontend_cast(frontend));
    } else {
      testrun(!ov_mc_frontend_cast(frontend));
    }
  }

  frontend->magic_bytes = OV_MC_FRONTEND_MAGIC_BYTES;

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_register() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);

  ov_json_value *msg = ov_ice_proxy_vocs_msg_register("1-2-3-4");
  testrun(msg);

  testrun(-1 == ov_mc_frontend_registry_get_proxy_socket(frontend->registry));
  cb_event_register(frontend, OV_KEY_REGISTER, 1, msg);
  testrun(1 == ov_mc_frontend_registry_get_proxy_socket(frontend->registry));

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *
msg_ice_session_create_response(const char *uuid, const char *sdp,
                                const char *session_id, const char *type,
                                ov_mc_mixer_core_forward forward) {

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;
  ov_json_value *msg = NULL;
  ov_json_value *res = NULL;
  ov_json_value *arr = NULL;

  msg = ov_ice_proxy_vocs_msg_create_session_with_sdp(uuid, sdp);
  out = ov_event_api_create_success_response(msg);
  msg = ov_json_value_free(msg);
  res = ov_event_api_get_response(out);

  val = ov_json_string(session_id);
  if (!ov_json_object_set(res, OV_KEY_SESSION, val))
    goto error;

  val = ov_json_string(type);
  if (!ov_json_object_set(res, OV_KEY_TYPE, val))
    goto error;

  val = ov_json_string(sdp);
  if (!ov_json_object_set(res, OV_KEY_SDP, val))
    goto error;

  arr = ov_json_array();
  if (!ov_json_object_set(res, OV_KEY_PROXY, arr))
    goto error;

  msg = ov_json_object();
  if (!ov_json_array_push(arr, msg))
    goto error;

  val = ov_json_number(forward.ssrc);
  if (!ov_json_object_set(msg, OV_KEY_SSRC, val))
    goto error;

  val = NULL;
  if (!ov_socket_configuration_to_json(forward.socket, &val))
    goto error;
  if (!ov_json_object_set(msg, OV_KEY_SOCKET, val))
    goto error;

  return out;
error:
  ov_json_value_free(val);
  ov_json_value_free(out);
  return NULL;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_ice_session_create_response() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);

  // register a proxy

  int proxy = ov_socket_create(manager, true, NULL);
  testrun(proxy > 0);
  testrun(ov_socket_ensure_nonblocking(proxy));

  ov_json_value *msg = ov_ice_proxy_vocs_msg_register("1-2-3-4");
  testrun(msg);
  char *str = ov_json_value_to_string(msg);
  msg = ov_json_value_free(msg);

  ssize_t bytes = -1;
  while (-1 == bytes) {

    bytes = send(proxy, str, strlen(str), 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  usleep(10000);
  loop->run(loop, OV_RUN_ONCE);
  str = ov_data_pointer_free(str);

  // check success

  int conn = ov_mc_frontend_registry_get_proxy_socket(frontend->registry);
  testrun(conn > 0);

  msg = msg_ice_session_create_response(
      "1-1-1-1", "sdp", "2-2-2-2", "offer",
      (ov_mc_mixer_core_forward){.ssrc = 12345,
                                 .socket.host = "127.0.0.1",
                                 .socket.port = 12345,
                                 .socket.type = UDP});
  testrun(msg);

  cb_event_ice_session_create_response(frontend, conn, msg);

  testrun(conn == ov_mc_frontend_registry_get_session_socket(frontend->registry,
                                                             "2-2-2-2"));

  testrun(0 == strcmp("2-2-2-2", userdata.session_id));
  testrun(0 == userdata.error_code);
  testrun(0 == strcmp("offer", userdata.type));
  testrun(0 == strcmp("sdp", userdata.sdp));
  testrun(12345 == userdata.forward.ssrc);
  testrun(12345 == userdata.forward.socket.port);
  testrun(0 == strcmp("127.0.0.1", userdata.forward.socket.host));
  testrun(UDP == userdata.forward.socket.type);
  userdata_clear(&userdata);

  // check socket is responsive
  char buf[1024] = {0};
  bytes = -1;

  for (int i = 0; i < 10; i++) {
    bytes = recv(proxy, buf, 1024, 0);
    testrun(-1 == bytes);
    loop->run(loop, OV_RUN_ONCE);
  }

  // check with no proxy array

  msg = msg_ice_session_create_response(
      "1-1-1-1", "sdp", "2-2-2-2", "offer",
      (ov_mc_mixer_core_forward){.ssrc = 12345,
                                 .socket.host = "127.0.0.1",
                                 .socket.port = 12345,
                                 .socket.type = UDP});
  testrun(msg);
  ov_json_value *res = ov_event_api_get_response(msg);
  ov_json_object_del(res, OV_KEY_PROXY);

  cb_event_ice_session_create_response(frontend, conn, msg);

  testrun(0 == strcmp("2-2-2-2", userdata.session_id));
  testrun(OV_ERROR_CODE_COMMS_ERROR == userdata.error_code);
  testrun(0 == strcmp("offer", userdata.type));
  testrun(0 == strcmp("sdp", userdata.sdp));
  userdata_clear(&userdata);

  // check socket is closed

  for (int i = 0; i < 10; i++) {
    bytes = recv(proxy, buf, 1024, 0);
    if (bytes != -1)
      break;
    loop->run(loop, OV_RUN_ONCE);
  }

  testrun(bytes == 0);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_ice_session_completed() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);

  // register a proxy

  int proxy = ov_socket_create(manager, true, NULL);
  testrun(proxy > 0);
  testrun(ov_socket_ensure_nonblocking(proxy));

  ov_json_value *msg = ov_ice_proxy_vocs_msg_register("1-2-3-4");
  testrun(msg);
  char *str = ov_json_value_to_string(msg);
  msg = ov_json_value_free(msg);

  ssize_t bytes = -1;
  while (-1 == bytes) {

    bytes = send(proxy, str, strlen(str), 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  usleep(10000);
  loop->run(loop, OV_RUN_ONCE);
  str = ov_data_pointer_free(str);

  // check success

  int conn = ov_mc_frontend_registry_get_proxy_socket(frontend->registry);
  testrun(conn > 0);

  msg = ov_ice_proxy_vocs_msg_session_completed("2-2-2-2", OV_ICE_COMPLETED);
  testrun(msg);

  cb_event_ice_session_completed(frontend, "name", conn, msg);

  testrun(0 == strcmp("2-2-2-2", userdata.session_id));
  testrun(0 == userdata.error_code);
  testrun(true == userdata.success);
  userdata_clear(&userdata);

  msg = ov_ice_proxy_vocs_msg_session_completed("2-2-2-2", OV_ICE_FAILED);
  testrun(msg);

  cb_event_ice_session_completed(frontend, "name", conn, msg);

  testrun(0 == strcmp("2-2-2-2", userdata.session_id));
  testrun(0 == userdata.error_code);
  testrun(false == userdata.success);
  userdata_clear(&userdata);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_ice_session_drop() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);

  // register a proxy

  int proxy = ov_socket_create(manager, true, NULL);
  testrun(proxy > 0);
  testrun(ov_socket_ensure_nonblocking(proxy));

  ov_json_value *msg = ov_ice_proxy_vocs_msg_register("1-2-3-4");
  testrun(msg);
  char *str = ov_json_value_to_string(msg);
  msg = ov_json_value_free(msg);

  ssize_t bytes = -1;
  while (-1 == bytes) {

    bytes = send(proxy, str, strlen(str), 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  usleep(10000);
  loop->run(loop, OV_RUN_ONCE);
  str = ov_data_pointer_free(str);

  // register some sessions

  int conn = ov_mc_frontend_registry_get_proxy_socket(frontend->registry);
  testrun(conn > 0);

  testrun(ov_mc_frontend_registry_register_session(frontend->registry, conn,
                                                   "1-1-1-1"));

  testrun(ov_mc_frontend_registry_register_session(frontend->registry, conn,
                                                   "2-1-1-1"));

  testrun(ov_mc_frontend_registry_register_session(frontend->registry, conn,
                                                   "3-1-1-1"));

  testrun(conn == ov_mc_frontend_registry_get_session_socket(frontend->registry,
                                                             "1-1-1-1"));

  testrun(conn == ov_mc_frontend_registry_get_session_socket(frontend->registry,
                                                             "2-1-1-1"));

  testrun(conn == ov_mc_frontend_registry_get_session_socket(frontend->registry,
                                                             "3-1-1-1"));

  msg = ov_ice_proxy_vocs_msg_drop_session("2-1-1-1");
  testrun(msg);

  cb_event_ice_session_drop(frontend, "name", conn, msg);

  testrun(0 == strcmp("2-1-1-1", userdata.session_id));
  testrun(0 == userdata.error_code);
  userdata_clear(&userdata);

  testrun(conn == ov_mc_frontend_registry_get_session_socket(frontend->registry,
                                                             "1-1-1-1"));

  testrun(conn == ov_mc_frontend_registry_get_session_socket(frontend->registry,
                                                             "3-1-1-1"));

  testrun(0 == ov_mc_frontend_registry_get_session_socket(frontend->registry,
                                                          "2-1-1-1"));

  // check drop response

  msg = ov_ice_proxy_vocs_msg_drop_session("1-1-1-1");
  testrun(msg);
  ov_json_value *out = ov_event_api_create_success_response(msg);
  ov_json_value *val = ov_json_string("1-1-1-1");
  ov_json_value *res = ov_event_api_get_response(out);
  ov_json_object_set(res, OV_KEY_SESSION, val);
  msg = ov_json_value_free(msg);

  cb_event_ice_session_drop(frontend, "name", conn, out);

  testrun(0 == strcmp("1-1-1-1", userdata.session_id));
  testrun(0 == userdata.error_code);
  userdata_clear(&userdata);

  testrun(0 == ov_mc_frontend_registry_get_session_socket(frontend->registry,
                                                          "1-1-1-1"));

  testrun(conn == ov_mc_frontend_registry_get_session_socket(frontend->registry,
                                                             "3-1-1-1"));

  testrun(0 == ov_mc_frontend_registry_get_session_socket(frontend->registry,
                                                          "2-1-1-1"));

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_ice_session_update_response() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);

  // register a proxy

  int proxy = ov_socket_create(manager, true, NULL);
  testrun(proxy > 0);
  testrun(ov_socket_ensure_nonblocking(proxy));

  ov_json_value *msg = ov_ice_proxy_vocs_msg_register("1-2-3-4");
  testrun(msg);
  char *str = ov_json_value_to_string(msg);
  msg = ov_json_value_free(msg);

  ssize_t bytes = -1;
  while (-1 == bytes) {

    bytes = send(proxy, str, strlen(str), 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  usleep(10000);
  loop->run(loop, OV_RUN_ONCE);
  str = ov_data_pointer_free(str);

  int conn = ov_mc_frontend_registry_get_proxy_socket(frontend->registry);
  testrun(conn > 0);

  msg = ov_ice_proxy_vocs_msg_update_session("1-1-1-1", "2-2-2-2",
                                             OV_MEDIA_OFFER, "sdp");

  ov_json_value *out = ov_event_api_create_success_response(msg);
  ov_json_value *par = ov_event_api_get_response(out);
  ov_json_value *val = ov_json_string("2-2-2-2");
  ov_json_object_set(par, OV_KEY_SESSION, val);
  msg = ov_json_value_free(msg);

  cb_event_ice_session_update_response(frontend, conn, out);

  testrun(0 == strcmp("2-2-2-2", userdata.session_id));
  testrun(0 == userdata.error_code);
  userdata_clear(&userdata);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_state_response() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk,
      .callback.session.state = cb_session_state};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);

  // register a proxy

  int proxy = ov_socket_create(manager, true, NULL);
  testrun(proxy > 0);
  testrun(ov_socket_ensure_nonblocking(proxy));

  ov_json_value *msg = ov_ice_proxy_vocs_msg_register("1-2-3-4");
  testrun(msg);
  char *str = ov_json_value_to_string(msg);
  msg = ov_json_value_free(msg);

  ssize_t bytes = -1;
  while (-1 == bytes) {

    bytes = send(proxy, str, strlen(str), 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  usleep(10000);
  loop->run(loop, OV_RUN_ONCE);
  str = ov_data_pointer_free(str);

  int conn = ov_mc_frontend_registry_get_proxy_socket(frontend->registry);
  testrun(conn > 0);

  msg = ov_ice_proxy_vocs_msg_session_state("2-2-2-2");

  ov_json_value *out = ov_event_api_create_success_response(msg);
  ov_json_value *par = ov_event_api_get_response(out);
  ov_json_value *val = ov_json_string("2-2-2-2");
  ov_json_object_set(par, OV_KEY_SESSION, val);
  msg = ov_json_value_free(msg);

  cb_event_state_response(frontend, conn, out);

  testrun(0 == strcmp("2-2-2-2", userdata.session_id));
  testrun(0 == userdata.error_code);
  userdata_clear(&userdata);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_ice_candidate() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);

  // register a proxy

  int proxy = ov_socket_create(manager, true, NULL);
  testrun(proxy > 0);
  testrun(ov_socket_ensure_nonblocking(proxy));

  ov_json_value *msg = ov_ice_proxy_vocs_msg_register("1-2-3-4");
  testrun(msg);
  char *str = ov_json_value_to_string(msg);
  msg = ov_json_value_free(msg);

  ssize_t bytes = -1;
  while (-1 == bytes) {

    bytes = send(proxy, str, strlen(str), 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  usleep(10000);
  loop->run(loop, OV_RUN_ONCE);
  str = ov_data_pointer_free(str);

  int conn = ov_mc_frontend_registry_get_proxy_socket(frontend->registry);
  testrun(conn > 0);

  ov_ice_candidate_info info = (ov_ice_candidate_info){.candidate = "candidate",
                                                       .SDPMlineIndex = 0,
                                                       .SDPMid = 0,
                                                       .ufrag = "ufrag"};

  msg = ov_ice_proxy_vocs_msg_candidate("2-2-2-2", &info);

  testrun(msg);

  cb_event_ice_candidate(frontend, "candidate", conn, msg);
  testrun(0 == userdata.uuid[0]);
  testrun(0 == strcmp("2-2-2-2", userdata.session_id));
  testrun(OV_ERROR_CODE_NOT_A_RESPONSE == userdata.error_code);
  testrun(userdata.info);
  userdata_clear(&userdata);

  msg = ov_ice_proxy_vocs_msg_candidate("2-2-2-2", &info);

  testrun(msg);

  ov_json_value *out = ov_event_api_create_success_response(msg);
  ov_event_api_set_uuid(out, "3-3-3-3");
  ov_json_value *par = ov_event_api_get_response(out);
  ov_json_value *val = ov_json_string("2-2-2-2");
  ov_json_object_set(par, OV_KEY_SESSION, val);
  msg = ov_json_value_free(msg);

  cb_event_ice_candidate(frontend, "candidate", conn, out);

  testrun(0 == strcmp("3-3-3-3", userdata.uuid));
  testrun(0 == strcmp("2-2-2-2", userdata.session_id));
  testrun(0 == userdata.error_code);
  testrun(userdata.info);
  userdata_clear(&userdata);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_ice_end_of_candidates() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);

  // register a proxy

  int proxy = ov_socket_create(manager, true, NULL);
  testrun(proxy > 0);
  testrun(ov_socket_ensure_nonblocking(proxy));

  ov_json_value *msg = ov_ice_proxy_vocs_msg_register("1-2-3-4");
  testrun(msg);
  char *str = ov_json_value_to_string(msg);
  msg = ov_json_value_free(msg);

  ssize_t bytes = -1;
  while (-1 == bytes) {

    bytes = send(proxy, str, strlen(str), 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  usleep(10000);
  loop->run(loop, OV_RUN_ONCE);
  str = ov_data_pointer_free(str);

  int conn = ov_mc_frontend_registry_get_proxy_socket(frontend->registry);
  testrun(conn > 0);

  msg = ov_ice_proxy_vocs_msg_end_of_candidates("2-2-2-2");

  testrun(msg);

  cb_event_ice_end_of_candidates(frontend, "candidate", conn, msg);
  testrun(0 == userdata.uuid[0]);
  testrun(0 == strcmp("2-2-2-2", userdata.session_id));
  testrun(OV_ERROR_CODE_NOT_A_RESPONSE == userdata.error_code);
  testrun(NULL == userdata.info);
  userdata_clear(&userdata);

  msg = ov_ice_proxy_vocs_msg_end_of_candidates("2-2-2-2");

  testrun(msg);

  ov_json_value *out = ov_event_api_create_success_response(msg);
  ov_event_api_set_uuid(out, "3-3-3-3");
  ov_json_value *par = ov_event_api_get_response(out);
  ov_json_value *val = ov_json_string("2-2-2-2");
  ov_json_object_set(par, OV_KEY_SESSION, val);
  msg = ov_json_value_free(msg);

  cb_event_ice_end_of_candidates(frontend, "candidate", conn, out);

  testrun(0 == strcmp("3-3-3-3", userdata.uuid));
  testrun(0 == strcmp("2-2-2-2", userdata.session_id));
  testrun(0 == userdata.error_code);
  testrun(NULL == userdata.info);
  userdata_clear(&userdata);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_talk_response() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);

  // register a proxy

  int proxy = ov_socket_create(manager, true, NULL);
  testrun(proxy > 0);
  testrun(ov_socket_ensure_nonblocking(proxy));

  ov_json_value *msg = ov_ice_proxy_vocs_msg_register("1-2-3-4");
  testrun(msg);
  char *str = ov_json_value_to_string(msg);
  msg = ov_json_value_free(msg);

  ssize_t bytes = -1;
  while (-1 == bytes) {

    bytes = send(proxy, str, strlen(str), 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  usleep(10000);
  loop->run(loop, OV_RUN_ONCE);
  str = ov_data_pointer_free(str);

  int conn = ov_mc_frontend_registry_get_proxy_socket(frontend->registry);
  testrun(conn > 0);

  msg = ov_ice_proxy_vocs_msg_talk("1-1-1-1", "2-2-2-2", true,
                                   (ov_mc_loop_data){.socket.host = "229.0.0."
                                                                    "1",
                                                     .socket.port = 12345,
                                                     .socket.type = UDP,
                                                     .name = "loop1",
                                                     .volume = 50});

  testrun(msg);

  ov_json_value *out = ov_event_api_create_success_response(msg);
  ov_json_value *par = ov_event_api_get_response(out);
  ov_json_value *val = ov_json_string("2-2-2-2");
  ov_json_object_set(par, OV_KEY_SESSION, val);
  val = ov_json_true();
  ov_json_object_set(par, OV_KEY_ON, val);
  val = ov_mc_loop_data_to_json((ov_mc_loop_data){.socket.host = "229.0.0.1",
                                                  .socket.port = 12345,
                                                  .socket.type = UDP,
                                                  .name = "loop1",
                                                  .volume = 50});
  ov_json_object_set(par, OV_KEY_LOOP, val);
  msg = ov_json_value_free(msg);

  cb_event_talk_response(frontend, conn, out);

  testrun(0 == strcmp("1-1-1-1", userdata.uuid));
  testrun(0 == strcmp("2-2-2-2", userdata.session_id));
  testrun(0 == userdata.error_code);
  testrun(userdata.success == true);
  testrun(0 == strcmp(userdata.data.socket.host, "229.0.0.1"));
  testrun(0 == strcmp(userdata.data.name, "loop1"));
  testrun(NULL == userdata.info);
  userdata_clear(&userdata);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_frontend_create_session() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);

  // register a proxy

  int proxy = ov_socket_create(manager, true, NULL);
  testrun(proxy > 0);
  testrun(ov_socket_ensure_nonblocking(proxy));

  // no proxy registered
  testrun(!ov_mc_frontend_create_session(frontend, "1-1-1-1", "sdp"));

  ov_json_value *msg = ov_ice_proxy_vocs_msg_register("1-2-3-4");
  testrun(msg);
  char *str = ov_json_value_to_string(msg);
  msg = ov_json_value_free(msg);

  ssize_t bytes = -1;
  while (-1 == bytes) {

    bytes = send(proxy, str, strlen(str), 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  usleep(10000);
  loop->run(loop, OV_RUN_ONCE);
  str = ov_data_pointer_free(str);

  int conn = ov_mc_frontend_registry_get_proxy_socket(frontend->registry);
  testrun(conn > 0);

  testrun(ov_mc_frontend_create_session(frontend, "1-1-1-1", "sdp"));

  char buf[1024] = {0};
  bytes = -1;

  while (-1 == bytes) {

    bytes = recv(proxy, buf, 1024, 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  // fprintf(stdout, "%s", buf);

  msg = ov_json_value_from_string(buf, bytes);
  testrun(msg);
  testrun(ov_event_api_event_is(msg, OV_KEY_ICE_SESSION_CREATE));
  testrun(0 == strcmp("1-1-1-1", ov_event_api_get_uuid(msg)));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_SDP));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));
  msg = ov_json_value_free(msg);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_frontend_update_session() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);

  // register a proxy

  int proxy = ov_socket_create(manager, true, NULL);
  testrun(proxy > 0);
  testrun(ov_socket_ensure_nonblocking(proxy));

  ov_json_value *msg = ov_ice_proxy_vocs_msg_register("1-2-3-4");
  testrun(msg);
  char *str = ov_json_value_to_string(msg);
  msg = ov_json_value_free(msg);

  ssize_t bytes = -1;
  while (-1 == bytes) {

    bytes = send(proxy, str, strlen(str), 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  usleep(10000);
  loop->run(loop, OV_RUN_ONCE);
  str = ov_data_pointer_free(str);

  int conn = ov_mc_frontend_registry_get_proxy_socket(frontend->registry);
  testrun(conn > 0);

  // no session created

  testrun(!ov_mc_frontend_update_session(frontend, "1-1-1-1", "2-2-2-2",
                                         OV_MEDIA_OFFER, "sdp"));

  // create session
  testrun(ov_mc_frontend_registry_register_session(frontend->registry, conn,
                                                   "2-2-2-2"));

  testrun(ov_mc_frontend_update_session(frontend, "1-1-1-1", "2-2-2-2",
                                        OV_MEDIA_OFFER, "sdp"));

  char buf[1024] = {0};
  bytes = -1;

  while (-1 == bytes) {

    bytes = recv(proxy, buf, 1024, 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  // fprintf(stdout, "%s", buf);

  msg = ov_json_value_from_string(buf, bytes);
  testrun(msg);
  testrun(ov_event_api_event_is(msg, OV_KEY_ICE_SESSION_UPDATE));
  testrun(0 == strcmp("1-1-1-1", ov_event_api_get_uuid(msg)));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_SDP));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));
  msg = ov_json_value_free(msg);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_frontend_drop_session() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);

  // register a proxy

  int proxy = ov_socket_create(manager, true, NULL);
  testrun(proxy > 0);
  testrun(ov_socket_ensure_nonblocking(proxy));

  ov_json_value *msg = ov_ice_proxy_vocs_msg_register("1-2-3-4");
  testrun(msg);
  char *str = ov_json_value_to_string(msg);
  msg = ov_json_value_free(msg);

  ssize_t bytes = -1;
  while (-1 == bytes) {

    bytes = send(proxy, str, strlen(str), 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  usleep(10000);
  loop->run(loop, OV_RUN_ONCE);
  str = ov_data_pointer_free(str);

  int conn = ov_mc_frontend_registry_get_proxy_socket(frontend->registry);
  testrun(conn > 0);

  // no session created

  testrun(ov_mc_frontend_drop_session(frontend, "1-1-1-1", "2-2-2-2"));

  // create session
  testrun(ov_mc_frontend_registry_register_session(frontend->registry, conn,
                                                   "2-2-2-2"));

  testrun(ov_mc_frontend_drop_session(frontend, "1-1-1-1", "2-2-2-2"));

  char buf[1024] = {0};
  bytes = -1;

  while (-1 == bytes) {

    bytes = recv(proxy, buf, 1024, 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  // fprintf(stdout, "%s", buf);

  msg = ov_json_value_from_string(buf, bytes);
  testrun(msg);
  testrun(ov_event_api_event_is(msg, OV_KEY_ICE_SESSION_DROP));
  testrun(0 == strcmp("1-1-1-1", ov_event_api_get_uuid(msg)));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION));
  msg = ov_json_value_free(msg);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_frontend_candidate() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);

  // register a proxy

  int proxy = ov_socket_create(manager, true, NULL);
  testrun(proxy > 0);
  testrun(ov_socket_ensure_nonblocking(proxy));

  ov_json_value *msg = ov_ice_proxy_vocs_msg_register("1-2-3-4");
  testrun(msg);
  char *str = ov_json_value_to_string(msg);
  msg = ov_json_value_free(msg);

  ssize_t bytes = -1;
  while (-1 == bytes) {

    bytes = send(proxy, str, strlen(str), 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  usleep(10000);
  loop->run(loop, OV_RUN_ONCE);
  str = ov_data_pointer_free(str);

  int conn = ov_mc_frontend_registry_get_proxy_socket(frontend->registry);
  testrun(conn > 0);

  ov_ice_candidate_info info = (ov_ice_candidate_info){.candidate = "candidate",
                                                       .SDPMlineIndex = 0,
                                                       .SDPMid = 0,
                                                       .ufrag = "ufrag"};

  // no session created

  testrun(!ov_mc_frontend_candidate(frontend, "1-1-1-1", "2-2-2-2", &info));

  // create session
  testrun(ov_mc_frontend_registry_register_session(frontend->registry, conn,
                                                   "2-2-2-2"));

  testrun(ov_mc_frontend_candidate(frontend, "1-1-1-1", "2-2-2-2", &info));

  char buf[1024] = {0};
  bytes = -1;

  while (-1 == bytes) {

    bytes = recv(proxy, buf, 1024, 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  // fprintf(stdout, "%s", buf);

  msg = ov_json_value_from_string(buf, bytes);
  testrun(msg);
  testrun(ov_event_api_event_is(msg, OV_KEY_CANDIDATE));
  testrun(0 == strcmp("1-1-1-1", ov_event_api_get_uuid(msg)));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_ICE_STRING_CANDIDATE));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_ICE_STRING_UFRAG));
  testrun(
      ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_ICE_STRING_SDP_MLINEINDEX));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_ICE_STRING_SDP_MID));
  msg = ov_json_value_free(msg);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_frontend_end_of_candidates() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);

  // register a proxy

  int proxy = ov_socket_create(manager, true, NULL);
  testrun(proxy > 0);
  testrun(ov_socket_ensure_nonblocking(proxy));

  ov_json_value *msg = ov_ice_proxy_vocs_msg_register("1-2-3-4");
  testrun(msg);
  char *str = ov_json_value_to_string(msg);
  msg = ov_json_value_free(msg);

  ssize_t bytes = -1;
  while (-1 == bytes) {

    bytes = send(proxy, str, strlen(str), 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  usleep(10000);
  loop->run(loop, OV_RUN_ONCE);
  str = ov_data_pointer_free(str);

  int conn = ov_mc_frontend_registry_get_proxy_socket(frontend->registry);
  testrun(conn > 0);

  // no session created

  testrun(!ov_mc_frontend_end_of_candidates(frontend, "1-1-1-1", "2-2-2-2"));

  // create session
  testrun(ov_mc_frontend_registry_register_session(frontend->registry, conn,
                                                   "2-2-2-2"));

  testrun(ov_mc_frontend_end_of_candidates(frontend, "1-1-1-1", "2-2-2-2"));

  char buf[1024] = {0};
  bytes = -1;

  while (-1 == bytes) {

    bytes = recv(proxy, buf, 1024, 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  // fprintf(stdout, "%s", buf);

  msg = ov_json_value_from_string(buf, bytes);
  testrun(msg);
  testrun(ov_event_api_event_is(msg, OV_ICE_STRING_END_OF_CANDIDATES));
  testrun(0 == strcmp("1-1-1-1", ov_event_api_get_uuid(msg)));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION));
  msg = ov_json_value_free(msg);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_frontend_talk() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);

  // register a proxy

  int proxy = ov_socket_create(manager, true, NULL);
  testrun(proxy > 0);
  testrun(ov_socket_ensure_nonblocking(proxy));

  ov_json_value *msg = ov_ice_proxy_vocs_msg_register("1-2-3-4");
  testrun(msg);
  char *str = ov_json_value_to_string(msg);
  msg = ov_json_value_free(msg);

  ssize_t bytes = -1;
  while (-1 == bytes) {

    bytes = send(proxy, str, strlen(str), 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  usleep(10000);
  loop->run(loop, OV_RUN_ONCE);
  str = ov_data_pointer_free(str);

  int conn = ov_mc_frontend_registry_get_proxy_socket(frontend->registry);
  testrun(conn > 0);

  // no session created

  testrun(!ov_mc_frontend_talk(frontend, "1-1-1-1", "2-2-2-2", true,
                               (ov_mc_loop_data){.socket.host = "229.0.0.1",
                                                 .socket.port = 12345,
                                                 .socket.type = UDP,
                                                 .name = "loop1",
                                                 .volume = 50}));

  // create session
  testrun(ov_mc_frontend_registry_register_session(frontend->registry, conn,
                                                   "2-2-2-2"));

  testrun(ov_mc_frontend_talk(frontend, "1-1-1-1", "2-2-2-2", true,
                              (ov_mc_loop_data){.socket.host = "229.0.0.1",
                                                .socket.port = 12345,
                                                .socket.type = UDP,
                                                .name = "loop1",
                                                .volume = 50}));

  char buf[1024] = {0};
  bytes = -1;

  while (-1 == bytes) {

    bytes = recv(proxy, buf, 1024, 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  // fprintf(stdout, "%s", buf);

  msg = ov_json_value_from_string(buf, bytes);
  testrun(msg);
  testrun(ov_event_api_event_is(msg, OV_KEY_TALK));
  testrun(0 == strcmp("1-1-1-1", ov_event_api_get_uuid(msg)));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_ON));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
  msg = ov_json_value_free(msg);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_frontened_get_session_state() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_frontend_config config = (ov_mc_frontend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.session.dropped = cb_session_dropped,
      .callback.session.created = cb_session_created,
      .callback.session.completed = cb_session_completed,
      .callback.session.update = cb_session_update,
      .callback.candidate = cb_candidate,
      .callback.end_of_candidates = cb_end_of_candidates,
      .callback.talk = cb_talk};

  ov_mc_frontend *frontend = ov_mc_frontend_create(config);
  testrun(frontend);

  // register a proxy

  int proxy = ov_socket_create(manager, true, NULL);
  testrun(proxy > 0);
  testrun(ov_socket_ensure_nonblocking(proxy));

  ov_json_value *msg = ov_ice_proxy_vocs_msg_register("1-2-3-4");
  testrun(msg);
  char *str = ov_json_value_to_string(msg);
  msg = ov_json_value_free(msg);

  ssize_t bytes = -1;
  while (-1 == bytes) {

    bytes = send(proxy, str, strlen(str), 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  usleep(10000);
  loop->run(loop, OV_RUN_ONCE);
  str = ov_data_pointer_free(str);

  int conn = ov_mc_frontend_registry_get_proxy_socket(frontend->registry);
  testrun(conn > 0);

  // no session created

  testrun(!ov_mc_frontened_get_session_state(frontend, "1-1-1-1", "2-2-2-2"));

  // create session
  testrun(ov_mc_frontend_registry_register_session(frontend->registry, conn,
                                                   "2-2-2-2"));

  testrun(ov_mc_frontened_get_session_state(frontend, "1-1-1-1", "2-2-2-2"));

  char buf[1024] = {0};
  bytes = -1;

  while (-1 == bytes) {

    bytes = recv(proxy, buf, 1024, 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  // fprintf(stdout, "%s", buf);

  msg = ov_json_value_from_string(buf, bytes);
  testrun(msg);
  testrun(ov_event_api_event_is(msg, OV_KEY_ICE_SESSION_STATE));
  testrun(0 == strcmp("1-1-1-1", ov_event_api_get_uuid(msg)));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION));
  msg = ov_json_value_free(msg);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_frontend_free(frontend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_frontend_config_from_json() {

  char *str = "{"
              "\"frontend\" :"
              "{"
              "\"socket\" :"
              "{"
              "\"manager\" :"
              "{"
              "\"host\" : \"127.0.0.1\","
              "\"type\" : \"TCP\","
              "\"port\" : 12346"
              "}"
              "}"
              "}"
              "}";

  ov_json_value *input = ov_json_value_from_string(str, strlen(str));
  testrun(input);

  ov_mc_frontend_config config = ov_mc_frontend_config_from_json(input);

  testrun(0 == strcmp("127.0.0.1", config.socket.manager.host));
  testrun(12346 == config.socket.manager.port);

  input = ov_json_value_free(input);

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
  testrun_test(test_ov_mc_frontend_create);
  testrun_test(test_ov_mc_frontend_free);
  testrun_test(test_ov_mc_frontend_cast);

  testrun_test(check_cb_event_register);
  testrun_test(check_cb_event_ice_session_create_response);
  testrun_test(check_cb_event_ice_session_completed);
  testrun_test(check_cb_event_ice_session_drop);
  testrun_test(check_cb_event_ice_session_update_response);
  testrun_test(check_cb_event_state_response);
  testrun_test(check_cb_event_ice_candidate);
  testrun_test(check_cb_event_ice_end_of_candidates);
  testrun_test(check_cb_event_talk_response);

  testrun_test(test_ov_mc_frontend_create_session);
  testrun_test(test_ov_mc_frontend_update_session);
  testrun_test(test_ov_mc_frontend_drop_session);
  testrun_test(test_ov_mc_frontend_candidate);
  testrun_test(test_ov_mc_frontend_end_of_candidates);
  testrun_test(test_ov_mc_frontend_talk);
  testrun_test(test_ov_mc_frontened_get_session_state);

  testrun_test(test_ov_mc_frontend_config_from_json);

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
