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
        @file           ov_mc_backend_test.c
        @author         Markus Töpfer

        @date           2023-01-21


        ------------------------------------------------------------------------
*/
#include "ov_mc_backend.c"
#include <ov_test/testrun.h>

#include <ov_base/ov_registered_cache.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_string.h>

struct userdata {

  ov_id uuid;
  ov_id session_id;
  uint64_t error_code;
  char *error_desc;
  char *loopname;
  uint8_t volume;
  char *function;
};

/*----------------------------------------------------------------------------*/

static bool userdata_clear(struct userdata *userdata) {

  if (!userdata)
    goto error;

  userdata->error_desc = ov_data_pointer_free(userdata->error_desc);
  userdata->loopname = ov_data_pointer_free(userdata->loopname);
  userdata->function = ov_data_pointer_free(userdata->function);
  *userdata = (struct userdata){0};

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static void cb_mixer_lost(void *userdata, const char *uuid) {

  struct userdata *data = (struct userdata *)userdata;
  userdata_clear(data);

  if (uuid)
    strncpy(data->uuid, uuid, sizeof(ov_id));
  data->function = strdup("cb_mixer_lost");
  return;
}

/*----------------------------------------------------------------------------*/

static void cb_mixer_acquired(void *userdata, const char *uuid,
                              const char *user_uuid, uint64_t error_code,
                              const char *error_desc) {

  struct userdata *data = (struct userdata *)userdata;
  userdata_clear(data);

  if (uuid)
    strncpy(data->uuid, uuid, sizeof(ov_id));
  if (user_uuid)
    strncpy(data->session_id, user_uuid, sizeof(ov_id));
  data->error_code = error_code;
  data->error_desc = ov_string_dup(error_desc);
  data->function = strdup("cb_mixer_acquired");
  return;
}

/*----------------------------------------------------------------------------*/

static void cb_mixer_released(void *userdata, const char *uuid,
                              const char *user_uuid, uint64_t error_code,
                              const char *error_desc) {

  struct userdata *data = (struct userdata *)userdata;
  userdata_clear(data);

  if (uuid)
    strncpy(data->uuid, uuid, sizeof(ov_id));
  if (user_uuid)
    strncpy(data->session_id, user_uuid, sizeof(ov_id));
  data->error_code = error_code;
  data->error_desc = ov_string_dup(error_desc);
  data->function = strdup("cb_mixer_released");
  return;
}

/*----------------------------------------------------------------------------*/

static void cb_mixer_join(void *userdata, const char *uuid,
                          const char *user_uuid, const char *loopname,
                          uint64_t error_code, const char *error_desc) {

  struct userdata *data = (struct userdata *)userdata;
  userdata_clear(data);

  if (uuid)
    strncpy(data->uuid, uuid, sizeof(ov_id));
  if (user_uuid)
    strncpy(data->session_id, user_uuid, sizeof(ov_id));
  data->error_code = error_code;
  data->error_desc = ov_string_dup(error_desc);
  data->loopname = ov_string_dup(loopname);
  data->function = strdup("cb_mixer_join");

  return;
}

/*----------------------------------------------------------------------------*/

static void cb_mixer_state(void *userdata, const char *uuid,
                           const ov_json_value *state) {

  struct userdata *data = (struct userdata *)userdata;
  userdata_clear(data);

  UNUSED(state);

  if (uuid)
    strncpy(data->uuid, uuid, sizeof(ov_id));
  data->function = strdup("cb_mixer_state");

  return;
}

/*----------------------------------------------------------------------------*/

static void cb_mixer_leave(void *userdata, const char *uuid,
                           const char *user_uuid, const char *loopname,
                           uint64_t error_code, const char *error_desc) {

  struct userdata *data = (struct userdata *)userdata;
  userdata_clear(data);

  if (uuid)
    strncpy(data->uuid, uuid, sizeof(ov_id));
  if (user_uuid)
    strncpy(data->session_id, user_uuid, sizeof(ov_id));
  data->error_code = error_code;
  data->error_desc = ov_string_dup(error_desc);
  data->loopname = ov_string_dup(loopname);
  data->function = strdup("cb_mixer_leave");

  return;
}

/*----------------------------------------------------------------------------*/

static void cb_mixer_volume(void *userdata, const char *uuid,
                            const char *user_uuid, const char *loopname,
                            uint8_t volume, uint64_t error_code,
                            const char *error_desc) {

  struct userdata *data = (struct userdata *)userdata;
  userdata_clear(data);

  if (uuid)
    strncpy(data->uuid, uuid, sizeof(ov_id));
  if (user_uuid)
    strncpy(data->session_id, user_uuid, sizeof(ov_id));
  data->volume = volume;
  data->error_code = error_code;
  data->error_desc = ov_string_dup(error_desc);
  data->loopname = ov_string_dup(loopname);
  data->function = strdup("cb_mixer_volume");

  return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_mc_backend_create() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_backend_config config = (ov_mc_backend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.mixer.lost = cb_mixer_lost

  };

  ov_mc_backend *backend = ov_mc_backend_create(config);
  testrun(backend);
  testrun(ov_mc_backend_cast(backend));
  testrun(backend->app);
  testrun(-1 != backend->socket.manager);
  testrun(backend->registry);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_backend_free(backend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_mixer_register() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_backend_config config = (ov_mc_backend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.mixer.lost = cb_mixer_lost

  };

  ov_mc_backend *backend = ov_mc_backend_create(config);
  testrun(backend);

  // register a mixer

  int mixer = ov_socket_create(manager, true, NULL);
  testrun(mixer > 0);
  testrun(ov_socket_ensure_nonblocking(mixer));

  ov_json_value *msg = ov_mc_mixer_msg_register("1-2-3-4", "audio");
  char *str = ov_json_value_to_string(msg);
  msg = ov_json_value_free(msg);

  ssize_t bytes = -1;
  while (-1 == bytes) {

    bytes = send(mixer, str, strlen(str), 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  str = ov_data_pointer_free(str);

  // expect configure response

  char buf[1024] = {0};

  bytes = -1;
  while (-1 == bytes) {

    bytes = recv(mixer, buf, 1024, 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  msg = ov_json_value_from_string(buf, bytes);
  testrun(msg);
  testrun(ov_event_api_event_is(msg, OV_KEY_CONFIGURE));
  msg = ov_json_value_free(msg);

  // check registered mixer

  ov_mc_backend_registry_count count =
      ov_mc_backend_registry_count_mixers(backend->registry);

  testrun(count.mixers == 1);
  testrun(count.used == 0);

  ov_mc_mixer_data data =
      ov_mc_backend_registry_acquire_user(backend->registry, "2-2-2-2");

  count = ov_mc_backend_registry_count_mixers(backend->registry);

  testrun(count.mixers == 1);
  testrun(count.used == 1);

  testrun(0 == strcmp(data.uuid, "1-2-3-4"));
  testrun(0 == strcmp(data.user, "2-2-2-2"));

  // register a mixer with failing data

  int mixer2 = ov_socket_create(manager, true, NULL);
  testrun(mixer2 > 0);
  testrun(ov_socket_ensure_nonblocking(mixer2));

  msg = ov_mc_mixer_msg_register("1-2-3-4", "video");
  str = ov_json_value_to_string(msg);
  msg = ov_json_value_free(msg);

  bytes = -1;
  while (-1 == bytes) {

    bytes = send(mixer2, str, strlen(str), 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  str = ov_data_pointer_free(str);

  // expect socket close

  bytes = -1;
  while (-1 == bytes) {

    bytes = recv(mixer2, buf, 1024, 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  testrun(0 == bytes);

  // expect no impact to mixer 1

  count = ov_mc_backend_registry_count_mixers(backend->registry);

  testrun(count.mixers == 1);
  testrun(count.used == 1);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_backend_free(backend));
  userdata_clear(&userdata);
  testrun(NULL == ov_event_loop_free(loop));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int register_mixer(ov_event_loop *loop, ov_socket_configuration manager,
                          ov_mc_backend *backend, const char *uuid) {

  if (!loop || !backend || !uuid)
    goto error;

  // register a mixer

  int mixer = ov_socket_create(manager, true, NULL);
  if (mixer < 0)
    goto error;
  ov_socket_ensure_nonblocking(mixer);

  ov_json_value *msg = ov_mc_mixer_msg_register(uuid, "audio");
  char *str = ov_json_value_to_string(msg);
  msg = ov_json_value_free(msg);

  ssize_t bytes = -1;
  while (-1 == bytes) {

    bytes = send(mixer, str, strlen(str), 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  str = ov_data_pointer_free(str);

  // expect configure response

  char buf[1024] = {0};

  bytes = -1;
  while (-1 == bytes) {

    bytes = recv(mixer, buf, 1024, 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  msg = ov_json_value_from_string(buf, bytes);
  if (!msg)
    goto error;
  msg = ov_json_value_free(msg);

  return mixer;
error:
  return -1;
}

/*----------------------------------------------------------------------------*/

int check_mixer_acquire_response() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_backend_config config = (ov_mc_backend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.mixer.lost = cb_mixer_lost

  };

  ov_mc_backend *backend = ov_mc_backend_create(config);
  testrun(backend);

  // register some mixer and aquire_user to get mixer_data

  int mixer1 = register_mixer(loop, manager, backend, "1-1-1-1");
  testrun(mixer1 > 0);

  ov_mc_mixer_data data1 =
      ov_mc_backend_registry_acquire_user(backend->registry, "1-1-1-2");

  int mixer2 = register_mixer(loop, manager, backend, "2-1-1-1");
  testrun(mixer2 > 0);

  ov_mc_mixer_data data2 =
      ov_mc_backend_registry_acquire_user(backend->registry, "2-1-1-2");

  // check success

  ov_json_value *out = NULL;
  ov_json_value *msg = ov_mc_mixer_msg_acquire(
      "1-1-1-2", (ov_mc_mixer_core_forward){.socket.host = "127.0.0.1",
                                            .socket.port = 12345,
                                            .socket.type = UDP,
                                            .ssrc = 123});

  testrun(msg);
  out = ov_event_api_create_success_response(msg);
  testrun(out);

  testrun(ov_callback_registry_register(
      backend->callbacks, ov_event_api_get_uuid(out),
      (ov_callback){.userdata = &userdata, .function = cb_mixer_acquired},
      OV_MC_BACKEND_DEFAULT_TIMEOUT));

  cb_event_acquire_response(backend, data1.socket, out);

  // check callback success
  testrun(0 == strcmp("1-1-1-2", userdata.session_id));
  testrun(0 == userdata.error_code);
  userdata_clear(&userdata);

  // check user registered
  ov_mc_mixer_data data3 =
      ov_mc_backend_registry_get_user(backend->registry, "1-1-1-2");
  testrun(data3.socket == data1.socket);

  out = ov_event_api_create_error_response(msg, 1, "some error");
  testrun(out);

  testrun(ov_callback_registry_register(
      backend->callbacks, ov_event_api_get_uuid(out),
      (ov_callback){.userdata = &userdata, .function = cb_mixer_acquired},
      OV_MC_BACKEND_DEFAULT_TIMEOUT));

  cb_event_acquire_response(backend, data1.socket, out);

  // check callback error
  testrun(0 == strcmp("1-1-1-2", userdata.session_id));
  testrun(0 == strcmp("some error", userdata.error_desc));
  testrun(1 == userdata.error_code);
  userdata_clear(&userdata);

  // check user unregistered
  data3 = ov_mc_backend_registry_get_user(backend->registry, "1-1-1-2");
  testrun(data3.socket == -1);
  testrun(0 == data3.uuid[0]);
  testrun(0 == data3.user[0]);

  // check mixer still registered, but unassigend
  data3 = ov_mc_backend_registry_get_socket(backend->registry, data1.socket);
  testrun(data3.socket == data1.socket);
  testrun(0 != data3.uuid[0]);
  testrun(0 == data3.user[0]);
  testrun(0 == strcmp(data1.uuid, data3.uuid));

  // check kick mixer on failure input

  ov_json_value *par = ov_event_api_get_parameter(msg);
  testrun(ov_json_object_del(par, OV_KEY_NAME));
  out = ov_event_api_create_success_response(msg);

  testrun(ov_callback_registry_register(
      backend->callbacks, ov_event_api_get_uuid(out),
      (ov_callback){.userdata = &userdata, .function = cb_mixer_acquired},
      OV_MC_BACKEND_DEFAULT_TIMEOUT));

  cb_event_acquire_response(backend, data2.socket, out);
  msg = ov_json_value_free(msg);

  // check mixer no longer registered
  data3 = ov_mc_backend_registry_get_socket(backend->registry, data2.socket);
  testrun(data3.socket == -1);
  testrun(0 == data3.uuid[0]);
  testrun(0 == data3.user[0]);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_backend_free(backend));
  userdata_clear(&userdata);
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_mixer_release_response() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_backend_config config = (ov_mc_backend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.mixer.lost = cb_mixer_lost

  };

  ov_mc_backend *backend = ov_mc_backend_create(config);
  testrun(backend);

  // register some mixer and aquire_user to get mixer_data

  int mixer1 = register_mixer(loop, manager, backend, "1-1-1-1");
  testrun(mixer1 > 0);

  ov_mc_mixer_data data1 =
      ov_mc_backend_registry_acquire_user(backend->registry, "1-1-1-2");

  int mixer2 = register_mixer(loop, manager, backend, "2-1-1-1");
  testrun(mixer2 > 0);

  ov_mc_mixer_data data2 =
      ov_mc_backend_registry_acquire_user(backend->registry, "2-1-1-2");

  int mixer3 = register_mixer(loop, manager, backend, "3-1-1-1");
  testrun(mixer3 > 0);

  ov_mc_mixer_data data3 =
      ov_mc_backend_registry_acquire_user(backend->registry, "3-1-1-2");

  // check success

  ov_json_value *out = NULL;
  ov_json_value *msg = ov_mc_mixer_msg_release("1-1-1-2");

  testrun(msg);
  out = ov_event_api_create_success_response(msg);
  testrun(out);

  testrun(ov_callback_registry_register(
      backend->callbacks, ov_event_api_get_uuid(out),
      (ov_callback){.userdata = &userdata, .function = cb_mixer_released},
      OV_MC_BACKEND_DEFAULT_TIMEOUT));

  cb_event_release_response(backend, data1.socket, out);

  // check callback success
  testrun(0 == strcmp("1-1-1-2", userdata.session_id));
  testrun(0 == userdata.error_code);
  userdata_clear(&userdata);

  // check user unregistered
  ov_mc_mixer_data data =
      ov_mc_backend_registry_get_user(backend->registry, "1-1-1-2");
  testrun(data.socket == -1);

  msg = ov_json_value_free(msg);
  msg = ov_mc_mixer_msg_release("2-1-1-2");
  out = ov_event_api_create_error_response(msg, 1, "some error");
  testrun(out);

  testrun(ov_callback_registry_register(
      backend->callbacks, ov_event_api_get_uuid(out),
      (ov_callback){.userdata = &userdata, .function = cb_mixer_released},
      OV_MC_BACKEND_DEFAULT_TIMEOUT));

  cb_event_release_response(backend, data2.socket, out);

  // check callback error
  testrun(0 == strcmp("2-1-1-2", userdata.session_id));
  testrun(0 == strcmp("some error", userdata.error_desc));
  testrun(1 == userdata.error_code);
  userdata_clear(&userdata);

  // check user unregistered
  data = ov_mc_backend_registry_get_user(backend->registry, "2-1-1-2");
  testrun(data.socket == -1);
  testrun(0 == data.uuid[0]);
  testrun(0 == data.user[0]);

  // check mixer still registered, but unassigend
  data = ov_mc_backend_registry_get_socket(backend->registry, data1.socket);
  testrun(data.socket == data1.socket);
  testrun(0 != data.uuid[0]);
  testrun(0 == data.user[0]);
  testrun(0 == strcmp(data1.uuid, data.uuid));

  // check mixer kicked due to error
  data = ov_mc_backend_registry_get_socket(backend->registry, data2.socket);
  testrun(data.socket == -1);
  testrun(0 == data.uuid[0]);
  testrun(0 == data.user[0]);

  // check kick mixer on failure input

  msg = ov_json_value_free(msg);
  msg = ov_mc_mixer_msg_release("3-1-1-2");
  ov_json_value *par = ov_event_api_get_parameter(msg);
  testrun(ov_json_object_del(par, OV_KEY_NAME));
  out = ov_event_api_create_success_response(msg);

  testrun(ov_callback_registry_register(
      backend->callbacks, ov_event_api_get_uuid(out),
      (ov_callback){.userdata = &userdata, .function = cb_mixer_released},
      OV_MC_BACKEND_DEFAULT_TIMEOUT));

  cb_event_release_response(backend, data3.socket, out);
  msg = ov_json_value_free(msg);

  // check mixer no longer registered
  data = ov_mc_backend_registry_get_socket(backend->registry, data3.socket);
  testrun(data.socket == -1);
  testrun(0 == data.uuid[0]);
  testrun(0 == data.user[0]);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_backend_free(backend));
  userdata_clear(&userdata);
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_mixer_join_response() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_backend_config config = (ov_mc_backend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.mixer.lost = cb_mixer_lost

  };

  ov_mc_backend *backend = ov_mc_backend_create(config);
  testrun(backend);

  // register some mixer and aquire_user to get mixer_data

  int mixer1 = register_mixer(loop, manager, backend, "1-1-1-1");
  testrun(mixer1 > 0);

  ov_mc_mixer_data data1 =
      ov_mc_backend_registry_acquire_user(backend->registry, "1-1-1-2");

  // check success

  ov_json_value *out = NULL;
  ov_json_value *msg =
      ov_mc_mixer_msg_join((ov_mc_loop_data){.socket.host = "224.0.0.1",
                                             .socket.port = 12345,
                                             .socket.type = UDP,
                                             .name = "loop1",
                                             .volume = 50});

  testrun(msg);
  out = ov_event_api_create_success_response(msg);
  testrun(out);

  testrun(ov_callback_registry_register(
      backend->callbacks, ov_event_api_get_uuid(out),
      (ov_callback){.userdata = &userdata, .function = cb_mixer_join},
      OV_MC_BACKEND_DEFAULT_TIMEOUT));

  cb_event_join_response(backend, data1.socket, out);

  // check callback success
  testrun(0 == strcmp("1-1-1-2", userdata.session_id));
  testrun(0 == strcmp("loop1", userdata.loopname));
  testrun(0 == userdata.error_code);
  userdata_clear(&userdata);

  // check error

  testrun(msg);
  out = ov_event_api_create_error_response(msg, 1, "some error");
  testrun(out);

  testrun(ov_callback_registry_register(
      backend->callbacks, ov_event_api_get_uuid(out),
      (ov_callback){.userdata = &userdata, .function = cb_mixer_join},
      OV_MC_BACKEND_DEFAULT_TIMEOUT));

  cb_event_join_response(backend, data1.socket, out);

  // check callback success
  testrun(0 == strcmp("1-1-1-2", userdata.session_id));
  testrun(0 == strcmp("loop1", userdata.loopname));
  testrun(1 == userdata.error_code);
  testrun(0 == strcmp("some error", userdata.error_desc));
  userdata_clear(&userdata);

  msg = ov_json_value_free(msg);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_backend_free(backend));
  userdata_clear(&userdata);
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_mixer_leave_response() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_backend_config config = (ov_mc_backend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.mixer.lost = cb_mixer_lost

  };

  ov_mc_backend *backend = ov_mc_backend_create(config);
  testrun(backend);

  // register some mixer and aquire_user to get mixer_data

  int mixer1 = register_mixer(loop, manager, backend, "1-1-1-1");
  testrun(mixer1 > 0);

  ov_mc_mixer_data data1 =
      ov_mc_backend_registry_acquire_user(backend->registry, "1-1-1-2");

  // check success

  ov_json_value *out = NULL;
  ov_json_value *msg = ov_mc_mixer_msg_leave("loop1");

  testrun(msg);
  out = ov_event_api_create_success_response(msg);
  testrun(out);

  testrun(ov_callback_registry_register(
      backend->callbacks, ov_event_api_get_uuid(out),
      (ov_callback){.userdata = &userdata, .function = cb_mixer_leave},
      OV_MC_BACKEND_DEFAULT_TIMEOUT));

  cb_event_leave_response(backend, data1.socket, out);

  // check callback success
  testrun(0 == strcmp("1-1-1-2", userdata.session_id));
  testrun(0 == strcmp("loop1", userdata.loopname));
  testrun(0 == userdata.error_code);
  userdata_clear(&userdata);

  // check error

  testrun(msg);
  out = ov_event_api_create_error_response(msg, 1, "some error");
  testrun(out);

  testrun(ov_callback_registry_register(
      backend->callbacks, ov_event_api_get_uuid(out),
      (ov_callback){.userdata = &userdata, .function = cb_mixer_leave},
      OV_MC_BACKEND_DEFAULT_TIMEOUT));

  cb_event_leave_response(backend, data1.socket, out);

  // check callback success
  testrun(0 == strcmp("1-1-1-2", userdata.session_id));
  testrun(0 == strcmp("loop1", userdata.loopname));
  testrun(1 == userdata.error_code);
  testrun(0 == strcmp("some error", userdata.error_desc));
  userdata_clear(&userdata);

  msg = ov_json_value_free(msg);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_backend_free(backend));
  userdata_clear(&userdata);
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_mixer_volume_response() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_backend_config config = (ov_mc_backend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.mixer.lost = cb_mixer_lost

  };

  ov_mc_backend *backend = ov_mc_backend_create(config);
  testrun(backend);

  // register some mixer and aquire_user to get mixer_data

  int mixer1 = register_mixer(loop, manager, backend, "1-1-1-1");
  testrun(mixer1 > 0);

  ov_mc_mixer_data data1 =
      ov_mc_backend_registry_acquire_user(backend->registry, "1-1-1-2");

  // check success

  ov_json_value *out = NULL;
  ov_json_value *msg = ov_mc_mixer_msg_volume("loop1", 50);

  testrun(msg);
  out = ov_event_api_create_success_response(msg);
  testrun(out);

  testrun(ov_callback_registry_register(
      backend->callbacks, ov_event_api_get_uuid(out),
      (ov_callback){.userdata = &userdata, .function = cb_mixer_volume},
      OV_MC_BACKEND_DEFAULT_TIMEOUT));

  cb_event_volume_response(backend, data1.socket, out);

  // check callback success
  testrun(0 == strcmp("1-1-1-2", userdata.session_id));
  testrun(0 == strcmp("loop1", userdata.loopname));
  testrun(50 == userdata.volume);
  testrun(0 == userdata.error_code);
  userdata_clear(&userdata);

  // check error

  testrun(msg);
  out = ov_event_api_create_error_response(msg, 1, "some error");
  testrun(out);

  testrun(ov_callback_registry_register(
      backend->callbacks, ov_event_api_get_uuid(out),
      (ov_callback){.userdata = &userdata, .function = cb_mixer_volume},
      OV_MC_BACKEND_DEFAULT_TIMEOUT));

  cb_event_volume_response(backend, data1.socket, out);

  // check callback success
  testrun(0 == strcmp("1-1-1-2", userdata.session_id));
  testrun(0 == strcmp("loop1", userdata.loopname));
  testrun(1 == userdata.error_code);
  testrun(0 == strcmp("some error", userdata.error_desc));
  userdata_clear(&userdata);

  msg = ov_json_value_free(msg);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_backend_free(backend));
  userdata_clear(&userdata);
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_free() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_backend_config config = (ov_mc_backend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.mixer.lost = cb_mixer_lost

  };

  ov_mc_backend *backend = ov_mc_backend_create(config);
  testrun(backend);

  // register some mixer and aquire_user to get mixer_data

  int mixer1 = register_mixer(loop, manager, backend, "1-1-1-1");
  testrun(mixer1 > 0);

  ov_mc_mixer_data data1 =
      ov_mc_backend_registry_acquire_user(backend->registry, "1-1-1-2");

  testrun(0 == strcmp("1-1-1-1", data1.uuid));

  int mixer2 = register_mixer(loop, manager, backend, "2-1-1-1");
  testrun(mixer2 > 0);

  ov_mc_mixer_data data2 =
      ov_mc_backend_registry_acquire_user(backend->registry, "2-1-1-2");

  testrun(0 == strcmp("2-1-1-1", data2.uuid));

  int mixer3 = register_mixer(loop, manager, backend, "3-1-1-1");
  testrun(mixer3 > 0);

  ov_mc_mixer_data data3 =
      ov_mc_backend_registry_acquire_user(backend->registry, "3-1-1-2");

  testrun(0 == strcmp("3-1-1-1", data3.uuid));

  testrun(NULL == ov_mc_backend_free(backend));
  userdata_clear(&userdata);
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_cast() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_backend_config config = (ov_mc_backend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.mixer.lost = cb_mixer_lost

  };

  ov_mc_backend *backend = ov_mc_backend_create(config);
  testrun(backend);

  testrun(ov_mc_backend_cast(backend));

  for (size_t i = 0; i < 0xffff; i++) {

    backend->magic_bytes = i;

    if (i == OV_MC_BACKEND_MAGIC_BYTES) {
      testrun(ov_mc_backend_cast(backend));
    } else {
      testrun(!ov_mc_backend_cast(backend));
    }
  }

  backend->magic_bytes = OV_MC_BACKEND_MAGIC_BYTES;

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_backend_free(backend));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_acquire_mixer() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_backend_config config = (ov_mc_backend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.mixer.lost = cb_mixer_lost

  };

  ov_mc_backend *backend = ov_mc_backend_create(config);
  testrun(backend);

  // no mixer registered

  testrun(ov_mc_backend_acquire_mixer(
      backend, "1-1-1-1", "2-2-2-2",
      (ov_mc_mixer_core_forward){.socket.host = "127.0.0.1",
                                 .socket.port = 12345,
                                 .socket.type = UDP,
                                 .ssrc = 12345},
      &userdata, cb_mixer_acquired));

  testrun(0 == strcmp("1-1-1-1", userdata.uuid));
  testrun(0 == strcmp("2-2-2-2", userdata.session_id));
  testrun(OV_ERROR_NO_RESOURCE == userdata.error_code);
  userdata_clear(&userdata);

  // register some mixer

  int mixer1 = register_mixer(loop, manager, backend, "m-1");
  testrun(mixer1 > 0);

  testrun(ov_mc_backend_acquire_mixer(
      backend, "u-1", "s-1",
      (ov_mc_mixer_core_forward){.socket.host = "127.0.0.1",
                                 .socket.port = 12345,
                                 .socket.type = UDP,
                                 .ssrc = 12345},
      &userdata, cb_mixer_acquired));

  ssize_t bytes = -1;
  char buf[1024] = {0};

  while (-1 == bytes) {

    bytes = recv(mixer1, buf, 1024, 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  ov_json_value *msg = ov_json_value_from_string(buf, bytes);
  testrun(msg);
  testrun(ov_event_api_event_is(msg, OV_KEY_ACQUIRE));
  testrun(0 == strcmp("u-1", ov_event_api_get_uuid(msg)));
  const char *str = ov_json_string_get(
      ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_NAME));
  testrun(str);
  testrun(0 == strcmp(str, "s-1"));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_SOCKET));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_SSRC));
  msg = ov_json_value_free(msg);

  // check no callback done
  testrun(0 == userdata.uuid[0]);
  testrun(0 == userdata.session_id[0]);
  testrun(0 == userdata.error_code);
  testrun(0 == userdata.function);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_backend_free(backend));
  userdata_clear(&userdata);
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_release_mixer() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_backend_config config = (ov_mc_backend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.mixer.lost = cb_mixer_lost

  };

  ov_mc_backend *backend = ov_mc_backend_create(config);
  testrun(backend);

  // no mixer registered

  testrun(ov_mc_backend_release_mixer(backend, "1-1-1-1", "2-2-2-2", &userdata,
                                      cb_mixer_released));

  testrun(0 == strcmp("1-1-1-1", userdata.uuid));
  testrun(0 == strcmp("2-2-2-2", userdata.session_id));
  testrun(0 == userdata.error_code);
  userdata_clear(&userdata);

  // register some mixer

  int mixer1 = register_mixer(loop, manager, backend, "m-1");
  testrun(mixer1 > 0);

  testrun(ov_mc_backend_release_mixer(backend, "1-1-1-1", "2-2-2-2", &userdata,
                                      cb_mixer_released));

  testrun(0 == strcmp("1-1-1-1", userdata.uuid));
  testrun(0 == strcmp("2-2-2-2", userdata.session_id));
  testrun(0 == userdata.error_code);
  userdata_clear(&userdata);

  // acquire user

  ov_mc_mixer_data data =
      ov_mc_backend_registry_acquire_user(backend->registry, "2-2-2-2");

  UNUSED(data);

  testrun(ov_mc_backend_release_mixer(backend, "1-1-1-1", "2-2-2-2", &userdata,
                                      cb_mixer_released));

  // check no callback done
  testrun(0 == userdata.uuid[0]);
  testrun(0 == userdata.session_id[0]);
  testrun(0 == userdata.error_code);
  testrun(0 == userdata.function);

  ssize_t bytes = -1;
  char buf[1024] = {0};

  while (-1 == bytes) {

    bytes = recv(mixer1, buf, 1024, 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  ov_json_value *msg = ov_json_value_from_string(buf, bytes);
  testrun(msg);
  testrun(ov_event_api_event_is(msg, OV_KEY_RELEASE));
  testrun(0 == strcmp("1-1-1-1", ov_event_api_get_uuid(msg)));
  const char *str = ov_json_string_get(
      ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_NAME));
  testrun(str);
  testrun(0 == strcmp(str, "2-2-2-2"));
  msg = ov_json_value_free(msg);

  // check mixer released user but still registered

  data = ov_mc_backend_registry_get_socket(backend->registry, data.socket);

  testrun(0 != data.uuid[0]);
  testrun(0 == data.user[0]);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_backend_free(backend));
  userdata_clear(&userdata);
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_join_loop() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_backend_config config = (ov_mc_backend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.mixer.lost = cb_mixer_lost

  };

  ov_mc_backend *backend = ov_mc_backend_create(config);
  testrun(backend);

  // user not registered

  testrun(!ov_mc_backend_join_loop(backend, "1-1-1-1", "2-2-2-2",
                                   (ov_mc_loop_data){.socket.host = "229.0.0."
                                                                    "1",
                                                     .socket.port = 12345,
                                                     .socket.type = UDP,
                                                     .name = "loop1",
                                                     .volume = 50},
                                   &userdata, cb_mixer_join));

  // register some mixer

  int mixer1 = register_mixer(loop, manager, backend, "m-1");
  testrun(mixer1 > 0);

  testrun(!ov_mc_backend_join_loop(backend, "1-1-1-1", "2-2-2-2",
                                   (ov_mc_loop_data){.socket.host = "229.0.0."
                                                                    "1",
                                                     .socket.port = 12345,
                                                     .socket.type = UDP,
                                                     .name = "loop1",
                                                     .volume = 50},
                                   &userdata, cb_mixer_join));

  // acquire user

  ov_mc_mixer_data data =
      ov_mc_backend_registry_acquire_user(backend->registry, "2-2-2-2");

  UNUSED(data);

  testrun(ov_mc_backend_join_loop(backend, "1-1-1-1", "2-2-2-2",
                                  (ov_mc_loop_data){.socket.host = "229.0.0."
                                                                   "1",
                                                    .socket.port = 12345,
                                                    .socket.type = UDP,
                                                    .name = "loop1",
                                                    .volume = 50},
                                  &userdata, cb_mixer_join));

  ssize_t bytes = -1;
  char buf[1024] = {0};

  while (-1 == bytes) {

    bytes = recv(mixer1, buf, 1024, 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  // fprintf(stdout, "%s", buf);

  ov_json_value *msg = ov_json_value_from_string(buf, bytes);
  testrun(msg);
  testrun(ov_event_api_event_is(msg, OV_KEY_JOIN));
  testrun(0 == strcmp("1-1-1-1", ov_event_api_get_uuid(msg)));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_HOST));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
  msg = ov_json_value_free(msg);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_backend_free(backend));
  userdata_clear(&userdata);
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_get_session_state() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_backend_config config = (ov_mc_backend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.mixer.lost = cb_mixer_lost

  };

  ov_mc_backend *backend = ov_mc_backend_create(config);
  testrun(backend);

  // user not registered

  testrun(!ov_mc_backend_get_session_state(backend, "1-1-1-1", "2-2-2-2",
                                           &userdata, cb_mixer_state));

  // register some mixer

  int mixer1 = register_mixer(loop, manager, backend, "m-1");
  testrun(mixer1 > 0);

  testrun(!ov_mc_backend_get_session_state(backend, "1-1-1-1", "2-2-2-2",
                                           &userdata, cb_mixer_state));

  // acquire user

  ov_mc_mixer_data data =
      ov_mc_backend_registry_acquire_user(backend->registry, "2-2-2-2");

  UNUSED(data);

  testrun(ov_mc_backend_get_session_state(backend, "1-1-1-1", "2-2-2-2",
                                          &userdata, cb_mixer_state));

  ssize_t bytes = -1;
  char buf[1024] = {0};

  while (-1 == bytes) {

    bytes = recv(mixer1, buf, 1024, 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  // fprintf(stdout, "%s", buf);

  ov_json_value *msg = ov_json_value_from_string(buf, bytes);
  testrun(msg);
  testrun(ov_event_api_event_is(msg, OV_KEY_STATE));
  testrun(0 == strcmp("1-1-1-1", ov_event_api_get_uuid(msg)));
  msg = ov_json_value_free(msg);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_backend_free(backend));
  userdata_clear(&userdata);
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_leave_loop() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_backend_config config = (ov_mc_backend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.mixer.lost = cb_mixer_lost

  };

  ov_mc_backend *backend = ov_mc_backend_create(config);
  testrun(backend);

  // user not registered

  testrun(!ov_mc_backend_leave_loop(backend, "1-1-1-1", "2-2-2-2", "loop1",
                                    &userdata, cb_mixer_leave));

  // register some mixer

  int mixer1 = register_mixer(loop, manager, backend, "m-1");
  testrun(mixer1 > 0);

  testrun(!ov_mc_backend_leave_loop(backend, "1-1-1-1", "2-2-2-2", "loop1",
                                    &userdata, cb_mixer_leave));

  // acquire user

  ov_mc_mixer_data data =
      ov_mc_backend_registry_acquire_user(backend->registry, "2-2-2-2");

  UNUSED(data);

  testrun(ov_mc_backend_leave_loop(backend, "1-1-1-1", "2-2-2-2", "loop1",
                                   &userdata, cb_mixer_leave));

  ssize_t bytes = -1;
  char buf[1024] = {0};

  while (-1 == bytes) {

    bytes = recv(mixer1, buf, 1024, 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  // fprintf(stdout, "%s", buf);

  ov_json_value *msg = ov_json_value_from_string(buf, bytes);
  testrun(msg);
  testrun(ov_event_api_event_is(msg, OV_KEY_LEAVE));
  testrun(0 == strcmp("1-1-1-1", ov_event_api_get_uuid(msg)));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
  msg = ov_json_value_free(msg);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_backend_free(backend));
  userdata_clear(&userdata);
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_set_loop_volume() {

  struct userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration manager = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

  ov_mc_backend_config config = (ov_mc_backend_config){

      .loop = loop,
      .socket.manager = manager,

      .callback.userdata = &userdata,
      .callback.mixer.lost = cb_mixer_lost

  };

  ov_mc_backend *backend = ov_mc_backend_create(config);
  testrun(backend);

  // user not registered

  testrun(!ov_mc_backend_set_loop_volume(backend, "1-1-1-1", "2-2-2-2", "loop1",
                                         50, &userdata, cb_mixer_volume));

  // register some mixer

  int mixer1 = register_mixer(loop, manager, backend, "m-1");
  testrun(mixer1 > 0);

  testrun(!ov_mc_backend_set_loop_volume(backend, "1-1-1-1", "2-2-2-2", "loop1",
                                         50, &userdata, cb_mixer_volume));

  // acquire user

  ov_mc_mixer_data data =
      ov_mc_backend_registry_acquire_user(backend->registry, "2-2-2-2");

  UNUSED(data);

  testrun(ov_mc_backend_set_loop_volume(backend, "1-1-1-1", "2-2-2-2", "loop1",
                                        50, &userdata, cb_mixer_volume));

  ssize_t bytes = -1;
  char buf[1024] = {0};

  while (-1 == bytes) {

    bytes = recv(mixer1, buf, 1024, 0);
    loop->run(loop, OV_RUN_ONCE);
  }

  // fprintf(stdout, "%s", buf);

  ov_json_value *msg = ov_json_value_from_string(buf, bytes);
  testrun(msg);
  testrun(ov_event_api_event_is(msg, OV_KEY_VOLUME));
  testrun(0 == strcmp("1-1-1-1", ov_event_api_get_uuid(msg)));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
  testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_VOLUME));
  msg = ov_json_value_free(msg);

  userdata_clear(&userdata);
  testrun(NULL == ov_mc_backend_free(backend));
  userdata_clear(&userdata);
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_config_from_json() {

  char *str = "{"
              "\"backend\" :"
              "{"
              "\"socket\" :"
              "{"
              "\"manager\" :"
              "{"
              "\"host\" : \"127.0.0.1\","
              "\"type\" : \"TCP\","
              "\"port\" : 12346"
              "}"
              "},"

              "\"mixer\" :"
              "{"
              "\"vad\":"
              "{"
              "\"zero_crossings_rate_hertz\" : 50000,"
              "\"powerlevel_density_dbfs\" : -500,"
              "\"enabled\" : true"
              "},"
              "\"sample_rate_hz\" : 48000,"
              "\"noise\" : -70,"
              "\"max_num_frames\" : 100,"
              "\"frame_buffer\": 1024,"
              "\"normalize_input\" : false,"
              "\"rtp_keepalive\" : true,"
              "\"normalize_mixed_by_root\" : false"
              "}"
              "}"
              "}";

  ov_json_value *input = ov_json_value_from_string(str, strlen(str));
  testrun(input);

  ov_mc_backend_config config = ov_mc_backend_config_from_json(input);
  input = ov_json_value_free(input);

  testrun(50000 == config.mixer.config.vad.zero_crossings_rate_threshold_hertz);
  testrun(-500 == config.mixer.config.vad.powerlevel_density_threshold_db);
  testrun(true == config.mixer.config.incoming_vad);
  testrun(48000 == config.mixer.config.samplerate_hz);
  testrun(-70 == config.mixer.config.comfort_noise_max_amplitude);
  testrun(false == config.mixer.config.normalize_input);
  testrun(true == config.mixer.config.rtp_keepalive);
  testrun(false == config.mixer.config.normalize_mixing_result_by_square_root);
  testrun(100 == config.mixer.config.max_num_frames_to_mix);

  testrun(0 == strcmp("127.0.0.1", config.socket.manager.host));
  testrun(12346 == config.socket.manager.port);

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
  testrun_test(test_ov_mc_backend_create);
  testrun_test(test_ov_mc_backend_free);
  testrun_test(test_ov_mc_backend_cast);

  testrun_test(check_mixer_register);
  testrun_test(check_mixer_acquire_response);
  testrun_test(check_mixer_release_response);
  testrun_test(check_mixer_join_response);
  testrun_test(check_mixer_leave_response);
  testrun_test(check_mixer_volume_response);

  testrun_test(test_ov_mc_backend_acquire_mixer);
  testrun_test(test_ov_mc_backend_release_mixer);

  testrun_test(test_ov_mc_backend_join_loop);
  testrun_test(test_ov_mc_backend_leave_loop);
  testrun_test(test_ov_mc_backend_get_session_state);
  testrun_test(test_ov_mc_backend_set_loop_volume);

  testrun_test(test_ov_mc_backend_config_from_json);

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
