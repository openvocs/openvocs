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
        @file           ov_event_loop_test.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-12-17

        @ingroup        ov_event_loop

        @brief          Unit tests of ov_event_loop implementation.


        ------------------------------------------------------------------------
*/
#include "../../include/ov_event_loop_test_interface.h"
#include "ov_event_loop.c"

#define TEST_SLEEP_USEC 20000

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------*/

int test_ov_event_loop_config_default() {

    ov_event_loop_config config = ov_event_loop_config_default();

    testrun(config.max.sockets == OV_EVENT_LOOP_MAX_SOCKETS_DEFAULT);
    testrun(config.max.timers == OV_EVENT_LOOP_MAX_TIMERS_DEFAULT);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_ov_event_loop_config_adapt_to_runtime() {

    ov_event_loop_config config = {0};

    struct rlimit file_limit = {0};

    testrun(0 == getrlimit(RLIMIT_NOFILE, &file_limit));

    /*
     *  NOTE on macOS we get RLIM_INFINITY
     */

    // check we got a limit range
    testrun(file_limit.rlim_max > 1);
    // testrun(file_limit.rlim_max != RLIM_INFINITY);

    config = ov_event_loop_config_adapt_to_runtime(config);
    testrun(config.max.sockets == OV_EVENT_LOOP_SOCKETS_MIN);
    testrun(config.max.timers == OV_EVENT_LOOP_TIMERS_MIN);

    config.max.sockets = file_limit.rlim_max + 1;

    config = ov_event_loop_config_adapt_to_runtime(config);
    // testrun(config.max.sockets == file_limit.rlim_max);
    testrun(config.max.timers == OV_EVENT_LOOP_TIMERS_MIN);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_ov_event_loop_cast() {

    ov_event_loop loop = {0};

    testrun(!ov_event_loop_cast(&loop));
    testrun(ov_event_loop_set_type(&loop, 0));
    testrun(ov_event_loop_cast(&loop));

    for (uint32_t i = 0; i <= 0xffff; i++) {

        loop.magic_byte = i;
        if (i == OV_EVENT_LOOP_MAGIC_BYTE) {
            testrun(ov_event_loop_cast(&loop));
        } else {
            testrun(!ov_event_loop_cast(&loop));
        }

        loop.magic_byte = OV_EVENT_LOOP_MAGIC_BYTE;
        loop.type = i;
        testrun(ov_event_loop_cast(&loop));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int g_signum = -1;

static void sighandler_sigpipe(int signum) { g_signum = signum; }

/*----------------------------------------------------------------------------*/

bool g_stopped = false;

static bool event_loop_stop_mockup(ov_event_loop *loop) {

    UNUSED(loop);
    g_stopped = true;

    return true;
}

/*----------------------------------------------------------------------------*/

int test_ov_event_loop_setup_signals() {

    testrun(!ov_event_loop_setup_signals(0));

    testrun(SIGPIPE != g_signum);

    signal(SIGPIPE, sighandler_sigpipe);

    // process to send itself SIGPIPE...
    kill(getpid(), SIGPIPE);
    sleep(1);

    testrun(SIGPIPE == g_signum);
    g_signum = -1;

    ov_event_loop loop = {

        .stop = event_loop_stop_mockup,
    };

    testrun(ov_event_loop_set_type(&loop, 0x1234));

    testrun(ov_event_loop_setup_signals(&loop));
    kill(getpid(), SIGINT);
    sleep(1);

    testrun(g_signum == -1);

    kill(getpid(), SIGINT);
    sleep(1);

    testrun(g_signum == -1);
    testrun(g_stopped == true);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_ov_event_loop_set_type() {

    ov_event_loop loop = {0};

    testrun(!ov_event_loop_set_type(NULL, 0));

    for (uint32_t i = 0; i <= 0xffff; i++) {

        loop.magic_byte = i;
        testrun(ov_event_loop_set_type(&loop, i));
        testrun(loop.magic_byte == OV_EVENT_LOOP_MAGIC_BYTE);
        testrun(loop.type == i);
    }

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_ov_event_loop_free() {

    ov_event_loop *loop = ov_event_loop_default(ov_event_loop_config_default());

    testrun(loop);
    testrun(loop->free);

    ov_event_loop *(*free_loop)(ov_event_loop *) = NULL;

    testrun(NULL == ov_event_loop_free(NULL));

    loop->magic_byte = 1;
    testrun(loop == ov_event_loop_free(loop));
    loop->magic_byte = OV_EVENT_LOOP_MAGIC_BYTE;
    testrun(NULL == ov_event_loop_free(loop));

    loop = ov_event_loop_default(ov_event_loop_config_default());

    free_loop = loop->free;
    loop->free = NULL;
    testrun(loop == ov_event_loop_free(loop));

    loop->free = free_loop;
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_loop_default() {

    /* just check we have a default loop providing all functionatily
     * actual functionality is tested over interface tests */

    ov_event_loop *loop = ov_event_loop_default(ov_event_loop_config_default());

    testrun(loop);
    testrun(loop->free);

    testrun(loop->is_running);
    testrun(loop->stop);
    testrun(loop->run);

    testrun(loop->callback.set);
    testrun(loop->callback.unset);

    testrun(loop->timer.set);
    testrun(loop->timer.unset);

    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool cb(int socket, uint8_t events, void *data) {

    if (socket == 0) { /* unused */
    };
    if (events == 0) { /* unused */
    };

    char *buffer = (char *)data;

    if (buffer) recv(socket, buffer, 512, 0);

    return true;
}

/*----------------------------------------------------------------------------*/

int test_ov_event_add_default_connection_accept() {

    char buffer[512] = {0};

    int max_sockets = 100;
    int max_timers = 100;

    int sockets[max_sockets];
    memset(sockets, 0, sizeof(sockets));

    ov_event_loop_config config = {

        .max.sockets = max_sockets, .max.timers = max_timers};

    ov_socket_configuration socket_config = {

        .host = "localhost", .type = TCP

    };

    ov_event_loop *loop = ov_event_loop_default(config);
    testrun(loop);
    testrun(loop->is_running(loop) == false);

    socket_config.type = TCP;
    int socketTCP = ov_socket_create(socket_config, false, NULL);
    socket_config.type = UDP;
    int socketUDP = ov_socket_create(socket_config, false, NULL);
    socket_config.type = LOCAL;
    int socketLOCAL = ov_socket_create(socket_config, false, NULL);

    testrun(0 < socketTCP);
    testrun(0 < socketUDP);
    testrun(0 < socketLOCAL);

    testrun(!ov_event_add_default_connection_accept(NULL, 0, 0, NULL, NULL));
    testrun(!ov_event_add_default_connection_accept(
        NULL, socketTCP, OV_EVENT_IO_IN, &buffer, cb));
    testrun(!ov_event_add_default_connection_accept(
        loop, 0, OV_EVENT_IO_IN, &buffer, cb));
    testrun(!ov_event_add_default_connection_accept(
        loop, socketTCP, 0, &buffer, cb));
    testrun(!ov_event_add_default_connection_accept(
        loop, socketTCP, OV_EVENT_IO_IN, NULL, NULL));

    // TCP OK
    testrun(ov_event_add_default_connection_accept(
        loop, socketTCP, OV_EVENT_IO_IN, &buffer, cb));

    // LOCAL OK
    testrun(ov_event_add_default_connection_accept(
        loop, socketLOCAL, OV_EVENT_IO_IN, &buffer, cb));

    // UDP NOK
    testrun(!ov_event_add_default_connection_accept(
        loop, socketUDP, OV_EVENT_IO_IN, &buffer, cb));

    testrun(ov_socket_get_config(socketTCP, &socket_config, NULL, NULL));
    int client = ov_socket_create(socket_config, true, NULL);
    testrun(client > 0);
    testrun(strlen(buffer) == 0);
    loop->run(loop, OV_RUN_ONCE);
    testrun(0 < send(client, "1234", 4, 0));
    usleep(10000);
    loop->run(loop, OV_RUN_ONCE);

    fprintf(stdout, "buffer %s\n", buffer);

    // check data from buffer (MUST be accepted, as data was send)
    testrun(0 == strncmp(buffer, "1234", 4));

    testrun(ov_event_remove_default_connection_accept(loop, socketTCP));
    testrun(ov_event_remove_default_connection_accept(loop, socketLOCAL));

    // remove allready removed
    testrun(ov_event_remove_default_connection_accept(loop, socketTCP));
    testrun(ov_event_remove_default_connection_accept(loop, socketLOCAL));

    // remove on non accept socket
    testrun(ov_event_remove_default_connection_accept(loop, socketUDP));

    testrun(NULL == ov_event_loop_free(loop));
    unlink("localhost");

    close(socketTCP);
    close(socketUDP);
    close(socketLOCAL);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_remove_default_connection_accept() {

    // tested with @see test_ov_event_add_default_connection_accept
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_loop_config_from_json() {

    ov_json_value *obj = NULL;
    ov_json_value *val = NULL;
    ov_json_value *input = ov_json_object();

    ov_event_loop_config config = ov_event_loop_config_from_json(NULL);
    testrun(0 == config.max.sockets);
    testrun(0 == config.max.timers);

    config = ov_event_loop_config_from_json(input);
    testrun(0 == config.max.sockets);
    testrun(0 == config.max.timers);

    val = ov_json_number(1);
    testrun(ov_json_object_set(input, OV_EVENT_LOOP_KEY_MAX_SOCKETS, val));

    config = ov_event_loop_config_from_json(input);
    testrun(1 == config.max.sockets);
    testrun(0 == config.max.timers);

    val = ov_json_number(2);
    testrun(ov_json_object_set(input, OV_EVENT_LOOP_KEY_MAX_TIMERS, val));

    config = ov_event_loop_config_from_json(input);
    testrun(1 == config.max.sockets);
    testrun(2 == config.max.timers);

    /*
     *      Check outer object with KEY OV_EVENT_LOOP_KEY
     */

    obj = ov_json_object();

    config = ov_event_loop_config_from_json(obj);
    testrun(0 == config.max.sockets);
    testrun(0 == config.max.timers);

    testrun(ov_json_object_set(obj, OV_EVENT_LOOP_KEY, input));

    config = ov_event_loop_config_from_json(obj);
    testrun(1 == config.max.sockets);
    testrun(2 == config.max.timers);

    testrun(NULL == ov_json_value_free(obj));

    /*
     *      Check out of range
     */

    input = ov_json_object();

    val = ov_json_number(UINT32_MAX + 1);
    testrun(ov_json_object_set(input, OV_EVENT_LOOP_KEY_MAX_SOCKETS, val));

    config = ov_event_loop_config_from_json(input);
    testrun(0 == config.max.sockets);
    testrun(0 == config.max.timers);

    testrun(ov_json_object_del(input, OV_EVENT_LOOP_KEY_MAX_SOCKETS));

    val = ov_json_number(UINT32_MAX);
    testrun(ov_json_object_set(input, OV_EVENT_LOOP_KEY_MAX_SOCKETS, val));

    config = ov_event_loop_config_from_json(input);
    testrun(UINT32_MAX == config.max.sockets);
    testrun(0 == config.max.timers);

    testrun(ov_json_object_del(input, OV_EVENT_LOOP_KEY_MAX_SOCKETS));

    val = ov_json_number(UINT32_MAX + 1);
    testrun(ov_json_object_set(input, OV_EVENT_LOOP_KEY_MAX_SOCKETS, val));

    config = ov_event_loop_config_from_json(input);
    testrun(0 == config.max.sockets);
    testrun(0 == config.max.timers);

    testrun(ov_json_object_del(input, OV_EVENT_LOOP_KEY_MAX_TIMERS));

    val = ov_json_number(UINT32_MAX);
    testrun(ov_json_object_set(input, OV_EVENT_LOOP_KEY_MAX_TIMERS, val));

    config = ov_event_loop_config_from_json(input);
    testrun(0 == config.max.sockets);
    testrun(UINT32_MAX == config.max.timers);

    testrun(NULL == ov_json_value_free(input));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_loop_config_to_json() {

    ov_json_value *val = NULL;
    ov_event_loop_config config = {0};

    val = ov_event_loop_config_to_json(config);
    testrun(val);
    testrun(0 == ov_json_number_get(
                     ov_json_object_get(val, OV_EVENT_LOOP_KEY_MAX_TIMERS)));
    testrun(0 == ov_json_number_get(
                     ov_json_object_get(val, OV_EVENT_LOOP_KEY_MAX_SOCKETS)));
    val = ov_json_value_free(val);

    val = ov_event_loop_config_to_json(config);
    testrun(val);
    testrun(0 == ov_json_number_get(
                     ov_json_object_get(val, OV_EVENT_LOOP_KEY_MAX_TIMERS)));
    testrun(0 == ov_json_number_get(
                     ov_json_object_get(val, OV_EVENT_LOOP_KEY_MAX_SOCKETS)));
    val = ov_json_value_free(val);

    config.max.sockets = 15;
    config.max.timers = 1;

    val = ov_event_loop_config_to_json(config);
    testrun(val);
    testrun(1 == ov_json_number_get(
                     ov_json_object_get(val, OV_EVENT_LOOP_KEY_MAX_TIMERS)));
    testrun(15 == ov_json_number_get(
                      ov_json_object_get(val, OV_EVENT_LOOP_KEY_MAX_SOCKETS)));
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

    testrun_test(test_ov_event_loop_config_from_json);
    testrun_test(test_ov_event_loop_config_to_json);

    testrun_test(test_ov_event_loop_config_default);
    testrun_test(test_ov_event_loop_config_adapt_to_runtime);

    testrun_test(test_ov_event_loop_cast);
    testrun_test(test_ov_event_loop_setup_signals);
    testrun_test(test_ov_event_loop_set_type);
    testrun_test(test_ov_event_loop_free);

    testrun_test(test_ov_event_add_default_connection_accept);
    testrun_test(test_ov_event_remove_default_connection_accept);

    OV_EVENT_LOOP_PERFORM_INTERFACE_TESTS(ov_event_loop_default);

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
