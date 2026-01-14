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
        @file           ov_mixer_msg_test.c
        @author         Markus Töpfer

        @date           2022-11-11


        ------------------------------------------------------------------------
*/
#include "ov_mixer_msg.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_mixer_msg_acquire() {

    ov_json_value *par = NULL;
    ov_json_value *val = NULL;

    testrun(!ov_mixer_msg_acquire(NULL, (ov_mixer_forward){0}));
    testrun(!ov_mixer_msg_acquire("user", (ov_mixer_forward){0}));

    val = ov_mixer_msg_acquire(
        "user", (ov_mixer_forward){.ssrc = 1,
                                           .socket.port = 1234,
                                           .socket.host = "127.0.0.1",
                                           .socket.type = UDP});

    testrun(val);
    testrun(ov_event_api_event_is(val, OV_KEY_ACQUIRE));
    testrun(ov_event_api_get_uuid(val));
    par = ov_event_api_get_parameter(val);
    testrun(par);
    testrun(1 == ov_json_number_get(ov_json_get(par, "/" OV_KEY_SSRC)));
    testrun(ov_json_get(par, "/" OV_KEY_SOCKET));
    testrun(ov_json_get(par, "/" OV_KEY_NAME));
    testrun(1234 == ov_json_number_get(
                        ov_json_get(par, "/" OV_KEY_SOCKET "/" OV_KEY_PORT)));

    testrun(NULL == ov_json_value_free(val));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mixer_msg_aquire_get_username() {

    ov_json_value *val = val = ov_mixer_msg_acquire(
        "user", (ov_mixer_forward){.ssrc = 1,
                                           .socket.port = 1234,
                                           .socket.host = "127.0.0.1",
                                           .socket.type = UDP});

    const char *str = ov_mixer_msg_aquire_get_username(val);
    testrun(0 == strcmp(str, "user"));

    str = ov_mixer_msg_aquire_get_username(ov_event_api_get_parameter(val));
    testrun(0 == strcmp(str, "user"));

    val = ov_json_value_free(val);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mixer_msg_acquire_get_forward() {

    ov_json_value *val = ov_mixer_msg_acquire(
        "user", (ov_mixer_forward){.ssrc = 1,
                                           .socket.port = 1234,
                                           .socket.host = "127.0.0.1",
                                           .socket.type = UDP});
    testrun(val);

    ov_mixer_forward data = ov_mixer_msg_acquire_get_forward(val);
    testrun(1 == data.ssrc);
    testrun(1234 == data.socket.port);
    testrun(UDP == data.socket.type);
    testrun(0 == strcmp("127.0.0.1", data.socket.host));

    data = (ov_mixer_forward){0};

    data = ov_mixer_msg_acquire_get_forward(ov_event_api_get_parameter(val));
    testrun(1 == data.ssrc);
    testrun(1234 == data.socket.port);
    testrun(UDP == data.socket.type);
    testrun(0 == strcmp("127.0.0.1", data.socket.host));

    val = ov_json_value_free(val);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mixer_msg_join() {

    ov_json_value *par = NULL;
    ov_json_value *val = ov_mixer_msg_join((ov_mc_loop_data){0});
    testrun(!val);

    val = ov_mixer_msg_join((ov_mc_loop_data){.socket.host = "224.0.0.0",
                                                 .socket.port = 12345,
                                                 .socket.type = UDP,
                                                 .name = "loop1"});

    testrun(val);
    testrun(ov_event_api_event_is(val, OV_KEY_JOIN));
    testrun(ov_event_api_get_uuid(val));
    par = ov_event_api_get_parameter(val);
    testrun(par);

    testrun(ov_json_get(par, "/" OV_KEY_HOST));
    testrun(ov_json_get(par, "/" OV_KEY_LOOP));

    val = ov_json_value_free(val);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mixer_msg_join_from_json() {

    ov_json_value *val =
        ov_mixer_msg_join((ov_mc_loop_data){.socket.host = "224.0.0.0",
                                               .socket.port = 12345,
                                               .socket.type = UDP,
                                               .name = "loop1"});

    testrun(val);

    ov_mc_loop_data data = ov_mixer_msg_join_from_json(val);
    testrun(0 == strcmp(data.socket.host, "224.0.0.0"));
    testrun(12345 == data.socket.port);
    testrun(UDP == data.socket.type);
    testrun(0 == strcmp(data.name, "loop1"));

    val = ov_json_value_free(val);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mixer_msg_leave() {

    ov_json_value *par = NULL;
    ov_json_value *val = ov_mixer_msg_leave(NULL);
    testrun(!val);

    val = ov_mixer_msg_leave("loop1");
    testrun(val);
    testrun(ov_event_api_event_is(val, OV_KEY_LEAVE));
    testrun(ov_event_api_get_uuid(val));
    par = ov_event_api_get_parameter(val);
    testrun(par);

    testrun(ov_json_get(par, "/" OV_KEY_LOOP));

    val = ov_json_value_free(val);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mixer_msg_leave_from_json() {

    ov_json_value *val = ov_mixer_msg_leave("loop1");
    testrun(val);

    const char *str = ov_mixer_msg_leave_from_json(val);
    testrun(0 == strcmp(str, "loop1"));

    str = ov_mixer_msg_leave_from_json(ov_event_api_get_parameter(val));
    testrun(0 == strcmp(str, "loop1"));

    val = ov_json_value_free(val);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mixer_msg_shutdown() {

    ov_json_value *val = ov_mixer_msg_shutdown();
    testrun(val);
    testrun(ov_event_api_event_is(val, OV_KEY_SHUTDOWN));
    testrun(ov_event_api_get_uuid(val));
    testrun(!ov_event_api_get_parameter(val));
    val = ov_json_value_free(val);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mixer_msg_release() {

    ov_json_value *val = ov_mixer_msg_release("name");
    testrun(val);
    testrun(ov_event_api_event_is(val, OV_KEY_RELEASE));
    testrun(ov_event_api_get_uuid(val));
    testrun(ov_event_api_get_parameter(val));
    val = ov_json_value_free(val);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mixer_msg_state() {

    ov_json_value *val = ov_mixer_msg_state();
    testrun(val);
    testrun(ov_event_api_event_is(val, OV_KEY_STATE));
    testrun(ov_event_api_get_uuid(val));
    testrun(!ov_event_api_get_parameter(val));
    val = ov_json_value_free(val);

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

    testrun_test(test_ov_mixer_msg_acquire);
    testrun_test(test_ov_mixer_msg_aquire_get_username);
    testrun_test(test_ov_mixer_msg_acquire_get_forward);

    testrun_test(test_ov_mixer_msg_join);
    testrun_test(test_ov_mixer_msg_join_from_json);

    testrun_test(test_ov_mixer_msg_leave);
    testrun_test(test_ov_mixer_msg_leave_from_json);

    testrun_test(test_ov_mixer_msg_shutdown);
    testrun_test(test_ov_mixer_msg_release);
    testrun_test(test_ov_mixer_msg_state);

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
