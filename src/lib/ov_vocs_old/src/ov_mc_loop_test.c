/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_mc_loop_test.c
        @author         Markus

        @date           2022-11-10


        ------------------------------------------------------------------------
*/
#include "ov_mc_loop.c"
#include <ov_test/testrun.h>

struct userdata {

    ov_mc_loop_data data;
    char buffer[2048];
    size_t bytes;
    ov_socket_data remote;
};

/*----------------------------------------------------------------------------*/

static bool userdata_clear(struct userdata *data) {

    memset(data->buffer, 0, 2048);
    data->bytes = 0;
    data->data = (ov_mc_loop_data){0};
    data->remote = (ov_socket_data){0};
    return true;
}

/*----------------------------------------------------------------------------*/

static void cb_io(void *userdata,
                  const ov_mc_loop_data *data,
                  const uint8_t *buffer,
                  size_t bytes,
                  const ov_socket_data *remote) {

    struct userdata *d = (struct userdata *)userdata;
    userdata_clear(d);
    d->bytes = bytes;
    d->data = *data;
    d->remote = *remote;
    memcpy(d->buffer, buffer, bytes);
    return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_mc_loop_create() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_mc_loop_config config =
        (ov_mc_loop_config){.loop = loop,
                            .data = (ov_mc_loop_data){.socket.host = "229.0.0."
                                                                     "1",
                                                      .socket.port = 12345,
                                                      .socket.type = UDP,
                                                      .name = "loop1",
                                                      .volume = 50},
                            .callback.userdata = &userdata,
                            .callback.io = cb_io};

    ov_mc_loop *l = ov_mc_loop_create(config);
    testrun(l);
    testrun(ov_mc_loop_cast(l));
    testrun(l->rtp_fhd > 0);

    testrun(NULL == ov_mc_loop_free(l));

    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_loop_free() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_mc_loop_config config =
        (ov_mc_loop_config){.loop = loop,
                            .data = (ov_mc_loop_data){.socket.host = "229.0.0."
                                                                     "1",
                                                      .socket.port = 12345,
                                                      .socket.type = UDP,
                                                      .name = "loop1",
                                                      .volume = 50},
                            .callback.userdata = &userdata,
                            .callback.io = cb_io};

    ov_mc_loop *l = ov_mc_loop_create(config);
    testrun(l);
    testrun(ov_mc_loop_cast(l));

    testrun(NULL == ov_mc_loop_free(l));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_loop_cast() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_mc_loop_config config =
        (ov_mc_loop_config){.loop = loop,
                            .data = (ov_mc_loop_data){.socket.host = "229.0.0."
                                                                     "1",
                                                      .socket.port = 12345,
                                                      .socket.type = UDP,
                                                      .name = "loop1",
                                                      .volume = 50},
                            .callback.userdata = &userdata,
                            .callback.io = cb_io};

    ov_mc_loop *l = ov_mc_loop_create(config);
    testrun(l);
    testrun(ov_mc_loop_cast(l));

    for (size_t i = 0; i < 0xffff; i++) {

        l->magic_byte = i;

        if (i == OV_MC_LOOP_MAGIC_BYTES) {
            testrun(ov_mc_loop_cast(l));
        } else {
            testrun(!ov_mc_loop_cast(l));
        }
    }

    l->magic_byte = OV_MC_LOOP_MAGIC_BYTES;

    testrun(NULL == ov_mc_loop_free(l));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_loop_set_volume() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_mc_loop_config config =
        (ov_mc_loop_config){.loop = loop,
                            .data = (ov_mc_loop_data){.socket.host = "229.0.0."
                                                                     "1",
                                                      .socket.port = 12345,
                                                      .socket.type = UDP,
                                                      .name = "loop1",
                                                      .volume = 50},
                            .callback.userdata = &userdata,
                            .callback.io = cb_io};

    ov_mc_loop *l = ov_mc_loop_create(config);
    testrun(l);

    testrun(ov_mc_loop_set_volume(l, 10));
    testrun(l->config.data.volume == 10);
    testrun(ov_mc_loop_set_volume(l, 20));
    testrun(l->config.data.volume == 20);
    testrun(ov_mc_loop_set_volume(l, 30));
    testrun(l->config.data.volume == 30);

    testrun(NULL == ov_mc_loop_free(l));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_loop_get_volume() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_mc_loop_config config =
        (ov_mc_loop_config){.loop = loop,
                            .data = (ov_mc_loop_data){.socket.host = "229.0.0."
                                                                     "1",
                                                      .socket.port = 12345,
                                                      .socket.type = UDP,
                                                      .name = "loop1",
                                                      .volume = 50},
                            .callback.userdata = &userdata,
                            .callback.io = cb_io};

    ov_mc_loop *l = ov_mc_loop_create(config);
    testrun(l);
    testrun(ov_mc_loop_cast(l));

    testrun(ov_mc_loop_set_volume(l, 10));
    testrun(ov_mc_loop_get_volume(l) == 10);
    testrun(ov_mc_loop_set_volume(l, 20));
    testrun(ov_mc_loop_get_volume(l) == 20);
    testrun(ov_mc_loop_set_volume(l, 30));
    testrun(ov_mc_loop_get_volume(l) == 30);

    testrun(NULL == ov_mc_loop_free(l));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_loop_to_json() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_mc_loop_config config =
        (ov_mc_loop_config){.loop = loop,
                            .data = (ov_mc_loop_data){.socket.host = "229.0.0."
                                                                     "1",
                                                      .socket.port = 12345,
                                                      .socket.type = UDP,
                                                      .name = "loop1",
                                                      .volume = 50},
                            .callback.userdata = &userdata,
                            .callback.io = cb_io};

    ov_mc_loop *l = ov_mc_loop_create(config);
    testrun(l);
    testrun(ov_mc_loop_cast(l));

    testrun(ov_mc_loop_set_volume(l, 10));

    ov_json_value *val = ov_mc_loop_to_json(l);
    testrun(val);
    // ov_json_value_dump(stdout, val);
    val = ov_json_value_free(val);

    testrun(NULL == ov_mc_loop_free(l));
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
    testrun_test(test_ov_mc_loop_create);
    testrun_test(test_ov_mc_loop_free);
    testrun_test(test_ov_mc_loop_cast);

    testrun_test(test_ov_mc_loop_set_volume);
    testrun_test(test_ov_mc_loop_get_volume);

    testrun_test(test_ov_mc_loop_to_json);

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
