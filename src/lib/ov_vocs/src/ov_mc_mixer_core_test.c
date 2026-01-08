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
        @file           ov_mc_mixer_core_test.c
        @author         Markus Töpfer

        @date           2023-01-21


        ------------------------------------------------------------------------
*/
#include "ov_mc_mixer_core.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_mc_mixer_core_create() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_mc_mixer_core_config config = (ov_mc_mixer_core_config){.loop = loop};

  ov_mc_mixer_core *core = ov_mc_mixer_core_create(config);
  testrun(core);
  testrun(ov_mc_mixer_core_cast(core));
  testrun(core->loops);
  testrun(core->frame_buffer);
  testrun(core->codec.factory);
  testrun(core->codec.codecs);
  testrun(core->comfort_noise_32bit);
  testrun(-1 == core->socket);

  testrun(NULL == ov_mc_mixer_core_free(core));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_mixer_core_free() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_mc_mixer_core_config config = (ov_mc_mixer_core_config){.loop = loop};

  ov_mc_mixer_core *core = ov_mc_mixer_core_create(config);
  testrun(core);
  testrun(ov_mc_mixer_core_cast(core));

  testrun(ov_mc_mixer_core_set_name(core, "username"));
  testrun(ov_mc_mixer_core_set_forward(
      core, (ov_mc_mixer_core_forward){.ssrc = 12345,
                                       .payload_type = 1,
                                       .socket.host = "127.0.0.1",
                                       .socket.port = 12345,
                                       .socket.type = UDP}));

  testrun(
      ov_mc_mixer_core_join(core, (ov_mc_loop_data){.socket.host = "229.0.0.1",
                                                    .socket.port = 12345,
                                                    .socket.type = UDP,
                                                    .name = "loop1",
                                                    .volume = 50}));

  testrun(ov_mc_mixer_core_set_volume(core, "loop1", 100));

  testrun(NULL == ov_mc_mixer_core_free(core));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_mixer_core_cast() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_mc_mixer_core_config config = (ov_mc_mixer_core_config){.loop = loop};

  ov_mc_mixer_core *core = ov_mc_mixer_core_create(config);
  testrun(core);
  testrun(ov_mc_mixer_core_cast(core));

  for (size_t i = 0; i < 0xffff; i++) {

    core->magic_bytes = i;

    if (i == OV_MC_MIXER_CORE_MAGIC_BYTES) {
      testrun(ov_mc_mixer_core_cast(core));
    } else {
      testrun(!ov_mc_mixer_core_cast(core));
    }
  }

  core->magic_bytes = OV_MC_MIXER_CORE_MAGIC_BYTES;

  testrun(NULL == ov_mc_mixer_core_free(core));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_mixer_core_reconfigure() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_mc_mixer_core_config config = (ov_mc_mixer_core_config){.loop = loop};

  ov_mc_mixer_core *core = ov_mc_mixer_core_create(config);
  testrun(core);
  testrun(ov_mc_mixer_core_cast(core));

  testrun(ov_mc_mixer_core_reconfigure(
      core, (ov_mc_mixer_core_config){.loop = loop}));

  testrun(0 != core->mix_timer);
  testrun(NULL != core->comfort_noise_32bit);

  testrun(NULL == ov_mc_mixer_core_free(core));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_mixer_core_set_name() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_mc_mixer_core_config config = (ov_mc_mixer_core_config){.loop = loop};

  ov_mc_mixer_core *core = ov_mc_mixer_core_create(config);
  testrun(core);
  testrun(ov_mc_mixer_core_cast(core));

  testrun(ov_mc_mixer_core_set_name(core, "username"));
  testrun(0 == strcmp("username", ov_mc_mixer_core_get_name(core)));
  testrun(0 == strcmp("username", core->name));

  // check override
  testrun(ov_mc_mixer_core_set_name(core, "name"));
  testrun(0 == strcmp("name", ov_mc_mixer_core_get_name(core)));
  testrun(0 == strcmp("name", core->name));

  testrun(NULL == ov_mc_mixer_core_free(core));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_mixer_core_get_name() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_mc_mixer_core_config config = (ov_mc_mixer_core_config){.loop = loop};

  ov_mc_mixer_core *core = ov_mc_mixer_core_create(config);
  testrun(core);
  testrun(ov_mc_mixer_core_cast(core));

  testrun(NULL == ov_mc_mixer_core_get_name(core));

  testrun(ov_mc_mixer_core_set_name(core, "username"));
  testrun(0 == strcmp("username", ov_mc_mixer_core_get_name(core)));

  testrun(NULL == ov_mc_mixer_core_free(core));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_mixer_core_forward_data_is_valid() {

  ov_mc_mixer_core_forward data = {0};

  testrun(!ov_mc_mixer_core_forward_data_is_valid(&data));

  data = (ov_mc_mixer_core_forward){.ssrc = 12345,
                                    .payload_type = 1,
                                    .socket.host = "127.0.0.1",
                                    .socket.port = 12345,
                                    .socket.type = UDP};

  testrun(ov_mc_mixer_core_forward_data_is_valid(&data));

  data = (ov_mc_mixer_core_forward){.ssrc = 0,
                                    .payload_type = 1,
                                    .socket.host = "127.0.0.1",
                                    .socket.port = 12345,
                                    .socket.type = UDP};

  testrun(!ov_mc_mixer_core_forward_data_is_valid(&data));

  data = (ov_mc_mixer_core_forward){
      .ssrc = 12345, .socket.port = 12345, .socket.type = UDP};

  testrun(!ov_mc_mixer_core_forward_data_is_valid(&data));

  data = (ov_mc_mixer_core_forward){.ssrc = 12345,
                                    .payload_type = 1,
                                    .socket.host = "127.0.0.1",
                                    .socket.port = 0,
                                    .socket.type = UDP};

  testrun(!ov_mc_mixer_core_forward_data_is_valid(&data));

  data = (ov_mc_mixer_core_forward){.ssrc = 12345,
                                    .payload_type = 1,
                                    .socket.host = "127.0.0.1",
                                    .socket.port = 12345,
                                    .socket.type = TCP};

  testrun(!ov_mc_mixer_core_forward_data_is_valid(&data));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_mixer_core_set_forward() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_mc_mixer_core_config config = (ov_mc_mixer_core_config){.loop = loop};

  ov_mc_mixer_core *core = ov_mc_mixer_core_create(config);
  testrun(core);
  testrun(ov_mc_mixer_core_cast(core));

  ov_mc_mixer_core_forward data = {0};

  testrun(!ov_mc_mixer_core_set_forward(core, data));

  data = (ov_mc_mixer_core_forward){.ssrc = 12345,
                                    .payload_type = 1,
                                    .socket.host = "127.0.0.1",
                                    .socket.port = 12345,
                                    .socket.type = UDP};

  testrun(ov_mc_mixer_core_set_forward(core, data));

  testrun(12345 == core->forward.ssrc);
  testrun(12345 == core->forward.socket.port);
  testrun(0 == strcmp(core->forward.socket.host, "127.0.0.1"));
  testrun(-1 != core->socket);

  testrun(NULL == ov_mc_mixer_core_free(core));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_mixer_core_get_forward() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_mc_mixer_core_config config = (ov_mc_mixer_core_config){.loop = loop};

  ov_mc_mixer_core *core = ov_mc_mixer_core_create(config);
  testrun(core);
  testrun(ov_mc_mixer_core_cast(core));

  ov_mc_mixer_core_forward data =
      (ov_mc_mixer_core_forward){.ssrc = 12345,
                                 .payload_type = 1,
                                 .socket.host = "127.0.0.1",
                                 .socket.port = 12345,
                                 .socket.type = UDP};

  testrun(ov_mc_mixer_core_set_forward(core, data));

  data = ov_mc_mixer_core_get_forward(core);

  testrun(12345 == data.ssrc);
  testrun(12345 == data.socket.port);
  testrun(0 == strcmp(data.socket.host, "127.0.0.1"));

  testrun(NULL == ov_mc_mixer_core_free(core));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_mixer_core_join() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_mc_mixer_core_config config = (ov_mc_mixer_core_config){.loop = loop};

  ov_mc_mixer_core *core = ov_mc_mixer_core_create(config);
  testrun(core);
  testrun(ov_mc_mixer_core_cast(core));

  testrun(!ov_mc_mixer_core_join(
      core, (ov_mc_loop_data){.socket.host = "229.0.0.1"}));

  testrun(!ov_mc_mixer_core_join(core, (ov_mc_loop_data){.name = "loop1"}));

  testrun(
      ov_mc_mixer_core_join(core, (ov_mc_loop_data){.socket.host = "229.0.0.1",
                                                    .socket.port = 12345,
                                                    .socket.type = UDP,
                                                    .name = "loop1"}));

  testrun(1 == ov_dict_count(core->loops));
  testrun(ov_dict_get(core->loops, "loop1"));

  testrun(
      ov_mc_mixer_core_join(core, (ov_mc_loop_data){.socket.host = "229.0.0.2",
                                                    .socket.port = 12345,
                                                    .socket.type = UDP,
                                                    .name = "loop2"}));

  testrun(2 == ov_dict_count(core->loops));
  testrun(ov_dict_get(core->loops, "loop1"));
  testrun(ov_dict_get(core->loops, "loop2"));

  testrun(
      ov_mc_mixer_core_join(core, (ov_mc_loop_data){.socket.host = "229.0.0.3",
                                                    .socket.port = 12345,
                                                    .socket.type = UDP,
                                                    .name = "loop3"}));

  testrun(3 == ov_dict_count(core->loops));
  testrun(ov_dict_get(core->loops, "loop1"));
  testrun(ov_dict_get(core->loops, "loop2"));
  testrun(ov_dict_get(core->loops, "loop3"));

  testrun(NULL == ov_mc_mixer_core_free(core));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_mixer_core_leave() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_mc_mixer_core_config config = (ov_mc_mixer_core_config){.loop = loop};

  ov_mc_mixer_core *core = ov_mc_mixer_core_create(config);
  testrun(core);
  testrun(ov_mc_mixer_core_cast(core));

  testrun(
      ov_mc_mixer_core_join(core, (ov_mc_loop_data){.socket.host = "229.0.0.1",
                                                    .socket.port = 12345,
                                                    .socket.type = UDP,
                                                    .name = "loop1"}));

  testrun(
      ov_mc_mixer_core_join(core, (ov_mc_loop_data){.socket.host = "229.0.0.2",
                                                    .socket.port = 12345,
                                                    .socket.type = UDP,
                                                    .name = "loop2"}));

  testrun(
      ov_mc_mixer_core_join(core, (ov_mc_loop_data){.socket.host = "229.0.0.3",
                                                    .socket.port = 12345,
                                                    .socket.type = UDP,
                                                    .name = "loop3"}));

  testrun(3 == ov_dict_count(core->loops));
  testrun(ov_dict_get(core->loops, "loop1"));
  testrun(ov_dict_get(core->loops, "loop2"));
  testrun(ov_dict_get(core->loops, "loop3"));

  testrun(ov_mc_mixer_core_leave(core, "loop4"));
  testrun(ov_mc_mixer_core_leave(core, "unknown"));

  testrun(3 == ov_dict_count(core->loops));
  testrun(ov_dict_get(core->loops, "loop1"));
  testrun(ov_dict_get(core->loops, "loop2"));
  testrun(ov_dict_get(core->loops, "loop3"));

  testrun(ov_mc_mixer_core_leave(core, "loop2"));

  testrun(2 == ov_dict_count(core->loops));
  testrun(ov_dict_get(core->loops, "loop1"));
  testrun(!ov_dict_get(core->loops, "loop2"));
  testrun(ov_dict_get(core->loops, "loop3"));

  testrun(ov_mc_mixer_core_leave(core, "loop3"));

  testrun(1 == ov_dict_count(core->loops));
  testrun(ov_dict_get(core->loops, "loop1"));
  testrun(!ov_dict_get(core->loops, "loop2"));
  testrun(!ov_dict_get(core->loops, "loop3"));

  testrun(ov_mc_mixer_core_leave(core, "loop1"));
  testrun(0 == ov_dict_count(core->loops));
  testrun(!ov_dict_get(core->loops, "loop1"));
  testrun(!ov_dict_get(core->loops, "loop2"));
  testrun(!ov_dict_get(core->loops, "loop3"));

  testrun(NULL == ov_mc_mixer_core_free(core));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_mixer_core_release() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_mc_mixer_core_config config = (ov_mc_mixer_core_config){.loop = loop};

  ov_mc_mixer_core *core = ov_mc_mixer_core_create(config);
  testrun(core);
  testrun(ov_mc_mixer_core_cast(core));

  testrun(ov_mc_mixer_core_release(core));
  testrun(0 == ov_dict_count(core->loops));
  testrun(NULL == core->name);

  ov_mc_mixer_core_forward data = ov_mc_mixer_core_get_forward(core);

  testrun(0 == data.ssrc);
  testrun(0 == data.socket.port);
  testrun(0 == data.socket.host[0]);

  testrun(ov_mc_mixer_core_set_name(core, "username"));

  data = (ov_mc_mixer_core_forward){.ssrc = 12345,
                                    .payload_type = 1,
                                    .socket.host = "127.0.0.1",
                                    .socket.port = 12345,
                                    .socket.type = UDP};

  testrun(ov_mc_mixer_core_set_forward(core, data));

  testrun(
      ov_mc_mixer_core_join(core, (ov_mc_loop_data){.socket.host = "229.0.0.1",
                                                    .socket.port = 12345,
                                                    .socket.type = UDP,
                                                    .name = "loop1"}));

  testrun(
      ov_mc_mixer_core_join(core, (ov_mc_loop_data){.socket.host = "229.0.0.2",
                                                    .socket.port = 12345,
                                                    .socket.type = UDP,
                                                    .name = "loop2"}));

  testrun(
      ov_mc_mixer_core_join(core, (ov_mc_loop_data){.socket.host = "229.0.0.3",
                                                    .socket.port = 12345,
                                                    .socket.type = UDP,
                                                    .name = "loop3"}));

  testrun(3 == ov_dict_count(core->loops));
  testrun(ov_dict_get(core->loops, "loop1"));
  testrun(ov_dict_get(core->loops, "loop2"));
  testrun(ov_dict_get(core->loops, "loop3"));

  testrun(ov_mc_mixer_core_release(core));
  testrun(0 == ov_dict_count(core->loops));
  testrun(NULL == core->name);

  data = ov_mc_mixer_core_get_forward(core);

  testrun(0 == data.ssrc);
  testrun(0 == data.socket.port);
  testrun(0 == data.socket.host[0]);

  testrun(NULL == ov_mc_mixer_core_free(core));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_mixer_core_set_volume() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_mc_mixer_core_config config = (ov_mc_mixer_core_config){.loop = loop};

  ov_mc_mixer_core *core = ov_mc_mixer_core_create(config);
  testrun(core);
  testrun(ov_mc_mixer_core_cast(core));

  testrun(!ov_mc_mixer_core_set_volume(core, "loop1", 20));

  testrun(
      ov_mc_mixer_core_join(core, (ov_mc_loop_data){.socket.host = "229.0.0.1",
                                                    .socket.port = 12345,
                                                    .socket.type = UDP,
                                                    .name = "loop1"}));

  testrun(ov_mc_mixer_core_set_volume(core, "loop1", 20));
  ov_mc_loop *l = ov_dict_get(core->loops, "loop1");
  testrun(20 == ov_mc_loop_get_volume(l));

  testrun(ov_mc_mixer_core_set_volume(core, "loop1", 30));
  testrun(30 == ov_mc_loop_get_volume(l));

  testrun(ov_mc_mixer_core_set_volume(core, "loop1", 101));
  testrun(100 == ov_mc_loop_get_volume(l));

  testrun(ov_mc_mixer_core_set_volume(core, "loop1", 255));
  testrun(100 == ov_mc_loop_get_volume(l));

  testrun(ov_mc_mixer_core_set_volume(core, "loop1", 0));
  testrun(0 == ov_mc_loop_get_volume(l));

  testrun(NULL == ov_mc_mixer_core_free(core));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_mixer_core_get_volume() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_mc_mixer_core_config config = (ov_mc_mixer_core_config){.loop = loop};

  ov_mc_mixer_core *core = ov_mc_mixer_core_create(config);
  testrun(core);
  testrun(ov_mc_mixer_core_cast(core));

  testrun(0 == ov_mc_mixer_core_get_volume(core, "loop1"));
  testrun(!ov_mc_mixer_core_set_volume(core, "loop1", 20));

  testrun(
      ov_mc_mixer_core_join(core, (ov_mc_loop_data){.socket.host = "229.0.0.1",
                                                    .socket.port = 12345,
                                                    .socket.type = UDP,
                                                    .name = "loop1"}));

  testrun(ov_mc_mixer_core_set_volume(core, "loop1", 20));
  ov_mc_loop *l = ov_dict_get(core->loops, "loop1");
  testrun(20 == ov_mc_loop_get_volume(l));
  testrun(20 == ov_mc_mixer_core_get_volume(core, "loop1"));

  testrun(ov_mc_mixer_core_set_volume(core, "loop1", 30));
  testrun(30 == ov_mc_loop_get_volume(l));
  testrun(30 == ov_mc_mixer_core_get_volume(core, "loop1"));

  testrun(ov_mc_mixer_core_set_volume(core, "loop1", 101));
  testrun(100 == ov_mc_loop_get_volume(l));
  testrun(100 == ov_mc_mixer_core_get_volume(core, "loop1"));

  testrun(ov_mc_mixer_core_set_volume(core, "loop1", 255));
  testrun(100 == ov_mc_loop_get_volume(l));
  testrun(100 == ov_mc_mixer_core_get_volume(core, "loop1"));

  testrun(ov_mc_mixer_core_set_volume(core, "loop1", 0));
  testrun(0 == ov_mc_loop_get_volume(l));
  testrun(0 == ov_mc_mixer_core_get_volume(core, "loop1"));

  testrun(NULL == ov_mc_mixer_core_free(core));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_mixer_state() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_mc_mixer_core_config config = (ov_mc_mixer_core_config){.loop = loop};

  ov_mc_mixer_core *core = ov_mc_mixer_core_create(config);
  testrun(core);
  testrun(ov_mc_mixer_core_cast(core));

  ov_json_value *state = ov_mc_mixer_state(core);
  ov_json_value_dump(stdout, state);
  state = ov_json_value_free(state);

  testrun(
      ov_mc_mixer_core_join(core, (ov_mc_loop_data){.socket.host = "229.0.0.1",
                                                    .socket.port = 12345,
                                                    .socket.type = UDP,
                                                    .name = "loop1"}));

  testrun(
      ov_mc_mixer_core_join(core, (ov_mc_loop_data){.socket.host = "229.0.0.2",
                                                    .socket.port = 12345,
                                                    .socket.type = UDP,
                                                    .name = "loop3"}));

  testrun(ov_mc_mixer_core_set_volume(core, "loop1", 20));

  state = ov_mc_mixer_state(core);
  ov_json_value_dump(stdout, state);
  state = ov_json_value_free(state);

  testrun(NULL == ov_mc_mixer_core_free(core));
  testrun(NULL == ov_event_loop_free(loop));

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
  testrun_test(test_ov_mc_mixer_core_create);
  testrun_test(test_ov_mc_mixer_core_free);
  testrun_test(test_ov_mc_mixer_core_cast);

  testrun_test(test_ov_mc_mixer_core_reconfigure);

  testrun_test(test_ov_mc_mixer_core_set_name);
  testrun_test(test_ov_mc_mixer_core_get_name);

  testrun_test(test_ov_mc_mixer_core_forward_data_is_valid);
  testrun_test(test_ov_mc_mixer_core_get_forward);
  testrun_test(test_ov_mc_mixer_core_set_forward);

  testrun_test(test_ov_mc_mixer_core_join);
  testrun_test(test_ov_mc_mixer_core_leave);

  testrun_test(test_ov_mc_mixer_core_release);

  testrun_test(test_ov_mc_mixer_core_set_volume);
  testrun_test(test_ov_mc_mixer_core_get_volume);

  testrun_test(test_ov_mc_mixer_state);

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
