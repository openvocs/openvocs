/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_event_loop_test_interface.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2019-04-05

        @ingroup        ov_event_loop_test_interface

        @brief          Implementation of


        ------------------------------------------------------------------------
*/

#ifndef OV_TEST_RESOURCE_DIR
#error "Must provide -D OV_TEST_RESOURCE_DIR=value while compiling this file."
#endif

#include "../../include/ov_event_loop_test_interface.h"
#include "../../include/ov_utils.h"

#include "../../include/ov_time.h"

#define TEST_DEFAULT_WAIT_USEC 20000

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST HELPER
 *
 *      ------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------*/

struct container1 {

    ov_event_loop *loop;
    uint64_t timemax_usec;
};

/*---------------------------------------------------------------------------*/

struct container2 {

    int counter;
    uint64_t start;
    uint64_t timestamp;
    uint64_t timeout;
    ov_event_loop *loop;
};

static void clear_container2(struct container2 *container, size_t num) {
    OV_ASSERT(0 != container);

    for (size_t i = 0; i < num; ++i) {
        container[i] = (struct container2){0};
    }
}

/*---------------------------------------------------------------------------*/

static void *run_the_loop(void *arg) {

    /*
     *      Run the loop within a thread
     */

    ov_event_loop *loop = ov_event_loop_cast(arg);
    if (!loop) goto error;

    if (!loop->run(loop, OV_RUN_MAX)) goto error;

    // DONE exit anyway
error:
    pthread_exit(NULL);
}

/*---------------------------------------------------------------------------*/

static void *run_the_loop_with_timeout(void *arg) {

    struct container1 *container = (struct container1 *)arg;

    ov_event_loop *loop = ov_event_loop_cast(container->loop);
    if (!loop) {
        OV_ASSERT(1 == 0);
        goto error;
    }

    if (!loop->run(loop, container->timemax_usec)) goto error;

    // DONE exit anyway
error:
    pthread_exit(NULL);
}

/*---------------------------------------------------------------------------*/

static bool dummy_socket_callback(int socket, uint8_t events, void *data) {

    if (socket > 0) { /* unused */
    };
    if (events > 0) { /* unused */
    };
    if (data) { /* unused */
    };
    return true;
}

/*---------------------------------------------------------------------------*/

static bool dummy_timer_callback(uint32_t id, void *data) {

    if (id > 0) { /* unused */
    };
    if (data) { /* unused */
    };
    return true;
}

/*---------------------------------------------------------------------------*/

static bool dummy_callback(int socket, uint8_t events, void *data) {

    if (data || (socket == 0) || (events == 0)) { /* whatever */
    };
    return true;
}

/*---------------------------------------------------------------------------*/

static bool counting_called_callback(int socket, uint8_t events, void *data) {

    if (!data) return false;

    errno = 0;

    int i = *(int *)data;

    size_t size = 5;
    char buffer[size];
    memset(buffer, 0, size);

    /*  @DEV NOTE
     *
     *  recv with MSG_WAIT_ALL was not reading all data on macos,
     *  read is working as expected, to empty the sockets read buffer.
     */
    int r = read(socket, buffer, size);
    usleep(100);

    *(int *)data = i + 1;

    fprintf(stdout,
            "read %i bytes at socket %i - counter is at %i\n",
            r,
            socket,
            i + 1);

    if ((socket == 0) || (events == 0)) { /* whatever */
    };
    return true;
}

/*---------------------------------------------------------------------------*/

static bool counting_timer_cb(uint32_t id, void *data) {

    /*
     *      Increase int data counter
     */

    if (!data) return false;

    if (id == 0) { /* unused */
    };

    int counter = *(int *)data;
    counter++;
    *(int *)data = counter;

    // testrun_log("counting timer callback with ID %i counter %i", id,
    // counter);

    return true;
}

/*----------------------------------------------------------------------------*/

static bool counting_timestamped_cb(uint32_t id, void *data) {

    if (!data || (id == 0)) return false;

    struct container2 *container = data;

    container->counter++;
    container->timestamp = ov_time_get_current_time_usecs();
    /*
            testrun_log("counting timestamped counter %i | %"PRIu64,
                    container->counter,
                    container->timestamp);
    */
    return true;
}

/*---------------------------------------------------------------------------*/

static bool counting_timestamped_reset_cb(uint32_t id, void *data) {

    if (!data || (id == 0)) return false;

    struct container2 *container = data;

    if (!container->loop || !container->loop->timer.set) return false;

    container->counter++;
    container->timestamp = ov_time_get_current_time_usecs();

    container->loop->timer.set(container->loop,
                               container->timeout,
                               container,
                               counting_timestamped_reset_cb);

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TESTS
 *
 *      ------------------------------------------------------------------------
 */

ov_event_loop *(*event_loop_creator)(ov_event_loop_config config);

/*---------------------------------------------------------------------------*/

int test_impl_event_loop_free() {

    ov_event_loop_config config = {

        .max.sockets = 10, .max.timers = 10};

    ov_event_loop *loop = event_loop_creator(config);
    testrun(loop);

    // default empty test
    testrun(NULL == loop->free(NULL));
    testrun(NULL == loop->free(loop));

    // check event_loop verification
    loop = event_loop_creator(ov_event_loop_config_default());
    loop->magic_byte = 0;
    testrun(loop == loop->free(loop));
    loop->magic_byte = OV_EVENT_LOOP_MAGIC_BYTE;
    testrun(NULL == loop->free(loop));

    // check with content
    loop = event_loop_creator(ov_event_loop_config_default());

    ov_socket_configuration socket_config = {.host = "localhost", .type = TCP};

    int socket = ov_socket_create(socket_config, false, NULL);
    testrun(socket >= 0);
    testrun(ov_socket_get_config(socket, &socket_config, NULL, NULL));

    // open a client connection
    int client = ov_socket_create(socket_config, true, NULL);
    testrun(client >= 0);

    int opt = 0;
    socklen_t len = sizeof(opt);
    testrun(0 == getsockopt(socket, SOL_SOCKET, SO_ERROR, &opt, &len));
    testrun(0 == opt);
    testrun(0 == getsockopt(client, SOL_SOCKET, SO_ERROR, &opt, &len));
    testrun(0 == opt);

    testrun(loop->callback.set(
        loop, socket, OV_EVENT_IO_IN, NULL, dummy_socket_callback));

    uint32_t timer_id =
        loop->timer.set(loop, 1000000, NULL, dummy_timer_callback);

    testrun(timer_id > 0);
    testrun(NULL == loop->free(loop));

    // check socket closed
    testrun(0 != getsockopt(socket, SOL_SOCKET, SO_ERROR, &opt, &len));
    testrun(0 == getsockopt(client, SOL_SOCKET, SO_ERROR, &opt, &len));
    // testrun(0 != opt);
    close(client);
    close(socket);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_event_loop_is_running() {

    pthread_t thread;

    ov_event_loop_config config = {

        .max.sockets = 10, .max.timers = 10};

    ov_event_loop *loop = event_loop_creator(config);
    testrun(loop);
    testrun(loop->is_running(loop) == false);
    testrun(0 == pthread_create(&thread, NULL, run_the_loop, loop));

    int wait = 10;
    size_t i = 0;

    // allow some dynamic setup time for the runner
    for (i = 0; i < 1000; i++) {

        usleep(wait);
        if (loop->is_running(loop)) break;
    }

    testrun_log("Run time setup < %zu nano seconds", (i + 1) * wait);

    testrun(loop->is_running(loop) == true);
    testrun(loop->stop(loop));
    testrun(loop->is_running(loop) == false);
    testrun(0 == pthread_join(thread, NULL));

    testrun(loop->is_running(loop) == false);
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_event_loop_stop() {

    pthread_t thread = 0;

    ov_event_loop_config config = {

        .max.sockets = 10, .max.timers = 10};

    ov_event_loop *loop = event_loop_creator(config);
    testrun(loop);
    testrun(loop->is_running(loop) == false);

    testrun(!loop->stop(NULL));
    testrun(loop->stop(loop));
    testrun(loop->is_running(loop) == false);

    // stop with content

    ov_socket_configuration socket_config = {.host = "localhost", .type = TCP};

    int socket = ov_socket_create(socket_config, false, NULL);
    testrun(socket >= 0);

    // reread the socket config
    testrun(ov_socket_get_config(socket, &socket_config, NULL, NULL));
    int client = ov_socket_create(socket_config, true, NULL);
    testrun(client > 0);

    testrun(loop->callback.set(
        loop, socket, OV_EVENT_IO_IN, NULL, dummy_socket_callback));

    uint32_t timer_id =
        loop->timer.set(loop, 1000000, NULL, dummy_timer_callback);

    testrun(timer_id > 0);

    testrun(loop->is_running(loop) == false);
    testrun(0 == pthread_create(&thread, NULL, run_the_loop, loop));

    int wait = 10;
    size_t i = 0;

    // allow some dynamic setup time for the runner
    for (i = 0; i < 1000; i++) {

        usleep(wait);
        if (loop->is_running(loop)) break;
    }

    testrun(loop->is_running(loop) == true);
    testrun(loop->stop(loop));
    testrun(loop->is_running(loop) == false);

    // trigger io loop over client
    testrun(0 != send(client, "test", 4, 0));
    testrun(loop->is_running(loop) == false);

    close(client);
    close(socket);
    testrun(0 == pthread_join(thread, NULL));
    testrun(loop->is_running(loop) == false);
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_event_loop_run() {

    pthread_t thread;

    ov_event_loop_config config = {

        .max.sockets = 10, .max.timers = 10};

    ov_socket_configuration socket_config = {

        .host = "localhost", .type = UDP

    };

    int tcount = 0;
    int scount = 0;

    struct container1 container = {0};

    ov_event_loop *loop = event_loop_creator(config);
    testrun(loop);
    testrun(loop->is_running(loop) == false);

    container.loop = loop;
    container.timemax_usec = 1 * 1000 * 1000;

    testrun(0 == pthread_create(
                     &thread, NULL, run_the_loop_with_timeout, &container));

    int wait = 10;
    size_t i = 0;

    // allow some dynamic setup time for the runner
    for (i = 0; i < 1000; i++) {

        usleep(wait);
        if (loop->is_running(loop)) break;
    }

    testrun(loop->is_running(loop) == true);
    testrun_log("testloop is running in thread");

    usleep(container.timemax_usec);
    testrun(0 == pthread_join(thread, NULL));
    testrun(loop->is_running(loop) == false);

    testrun(!loop->run(NULL, 0));

    // run once
    testrun(loop->run(loop, 0));
    testrun(loop->run(loop, OV_RUN_ONCE));
    testrun(loop->is_running(loop) == false);

    // run 1 usec
    testrun(loop->run(loop, 1));
    usleep(100);
    testrun(loop->is_running(loop) == false);

    // run max
    container.timemax_usec = OV_RUN_MAX;
    testrun(0 == pthread_create(
                     &thread, NULL, run_the_loop_with_timeout, &container));

    usleep(10000);
    testrun(loop->is_running(loop) == true);

    // open a timer
    int id = loop->timer.set(loop, 10000, &tcount, counting_timer_cb);

    testrun(id > 0);

    // open a socket
    int socket = ov_socket_create(socket_config, false, NULL);
    testrun(socket > 0);

    // open a socket callback
    testrun(loop->callback.set(
        loop, socket, OV_EVENT_IO_IN, &scount, counting_called_callback));

    // stop running
    loop->stop(loop);
    testrun(0 == pthread_join(thread, NULL));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_event_loop_callback_set() {

    pthread_t thread;

    int max_sockets = 100;
    int max_timers = 100;
    int i = 0;

    int sockets[max_sockets];
    memset(sockets, 0, sizeof(sockets));

    int opt = 0;
    socklen_t len = sizeof(opt);

    ov_event_loop_config config = {

        .max.sockets = max_sockets, .max.timers = max_timers};

    ov_socket_configuration socket_config = {

        .host = "127.0.0.1", .type = TCP

    };

    ov_event_loop *loop = event_loop_creator(config);
    testrun(loop);
    testrun(loop->is_running(loop) == false);

    // open and check max_sockets
    int first = -1, last = 0;
    for (i = 0; i < 10; i++) {

        sockets[i] = ov_socket_create(socket_config, false, NULL);
        if (first == -1) first = sockets[i];

        testrun(sockets[i] >= 0);
        testrun(0 == getsockopt(sockets[i], SOL_SOCKET, SO_ERROR, &opt, &len));

        testrun(0 == opt);
        last = sockets[i];
    }

    // check behaviour
    testrun(!loop->callback.set(NULL, 0, 0, NULL, NULL));
    testrun(
        !loop->callback.set(NULL, first, OV_EVENT_IO_IN, NULL, dummy_callback));
    testrun(!loop->callback.set(loop, 0, OV_EVENT_IO_IN, NULL, dummy_callback));
    testrun(!loop->callback.set(loop, first, 0, NULL, dummy_callback));
    testrun(!loop->callback.set(loop, first, OV_EVENT_IO_IN, NULL, NULL));

    int counter = 0;

    testrun(loop->callback.set(
        loop, first, OV_EVENT_IO_IN, &counter, counting_called_callback));

    /*
     *      Performing a FULL functional callback test with set.
     *      This verifies the whole event loop
     *      callback IO related functionality.
     *
     *      TBD Discussion remove here and move somewhere else?
     *      This test is a bit more than testing set, this test
     *      is actually testing and verifying set.
     *
     *      START of Full functional verification test
     */

    // add auto accept server socket callback
    testrun(ov_event_add_default_connection_accept(
        loop, last, OV_EVENT_IO_IN, &counter, counting_called_callback));

    // create client
    testrun(ov_socket_get_config(last, &socket_config, NULL, NULL));
    int client = ov_socket_create(socket_config, true, NULL);
    testrun(client > 0);
    testrun(ov_socket_ensure_nonblocking(client));

    // run server functionality in thread
    struct container1 container = {
        .loop = loop,
        .timemax_usec = 3000 * 1000,
    };

    testrun(0 == pthread_create(
                     &thread, NULL, run_the_loop_with_timeout, &container));

    while (!loop->is_running(loop)) {
        sleep(1);
    };

    // verify callback
    testrun(0 == counter);
    // send some message to check the callback chain
    testrun(-1 != send(client, "test", 4, 0));
    // sleep for 1 second to let the thread work
    fprintf(stdout, "counter %i\n", counter);
    sleep(1);
    fprintf(stdout, "counter %i\n", counter);
    // callback was called
    testrun(1 == counter);
    // send some message to check the callback chain
    testrun(-1 != send(client, "test", 4, 0));
    // sleep for 1 second to let the thread work
    sleep(1);
    // callback was called
    testrun(2 == counter);

    // trigger stop
    loop->stop(loop);
    testrun(0 == pthread_join(thread, NULL));
    // clear all auto accept data
    testrun(ov_event_remove_default_connection_accept(loop, last));

    // <--- end of FULL functional verification test

    // override callback
    testrun(loop->callback.set(
        loop, last, OV_EVENT_IO_IN, &counter, dummy_callback));

    testrun(loop->callback.set(
        loop, last, OV_EVENT_IO_IN, &counter, counting_called_callback));

    testrun(loop->callback.set(
        loop, last, OV_EVENT_IO_IN, &counter, dummy_callback));

    testrun(loop->callback.set(
        loop, last, OV_EVENT_IO_IN, &counter, dummy_callback));

    // try to add without event
    testrun(!loop->callback.set(loop, last, 0, &counter, dummy_callback));

    testrun(loop->callback.set(
        loop, last, OV_EVENT_IO_IN, &counter, dummy_callback));

    // try to add with closed socket
    close(last);
    testrun(!loop->callback.set(
        loop, last, OV_EVENT_IO_IN, &counter, dummy_callback));

    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_event_loop_callback_unset() {

    pthread_t thread;

    int max_sockets = 100;
    int max_timers = 100;

    int sockets[max_sockets];
    memset(sockets, 0, sizeof(sockets));

    ov_event_loop_config config = {

        .max.sockets = max_sockets, .max.timers = max_timers};

    ov_socket_configuration socket_config = {

        .host = "127.0.0.1", .type = UDP

    };

    ov_event_loop *loop = event_loop_creator(config);
    testrun(loop);
    testrun(loop->is_running(loop) == false);

    int socket = ov_socket_create(socket_config, false, NULL);
    testrun(socket > 0);

    // set a callback
    int counter = 0;

    testrun(loop->callback.set(
        loop, socket, OV_EVENT_IO_IN, &counter, counting_called_callback));

    // check behaviour
    testrun(!loop->callback.unset(NULL, 0, NULL));
    testrun(!loop->callback.unset(NULL, socket, NULL));

    testrun(loop->callback.unset(loop, 0, NULL));
    testrun(loop->callback.unset(loop, socket, NULL));

    // create client
    testrun(ov_socket_get_config(socket, &socket_config, NULL, NULL));

    int client = ov_socket_create(socket_config, true, NULL);
    testrun(client > 0);
    testrun(ov_socket_ensure_nonblocking(client));

    // run server functionality in thread
    struct container1 container = {
        .loop = loop,
        .timemax_usec = 10 * 1000 * 1000,
    };

    // set callback
    testrun(loop->callback.set(
        loop, socket, OV_EVENT_IO_IN, &counter, counting_called_callback));

    testrun(0 == pthread_create(
                     &thread, NULL, run_the_loop_with_timeout, &container));

    usleep(TEST_DEFAULT_WAIT_USEC);
    while (!loop->is_running(loop)) {
    };

    // verify callback is set
    testrun(0 == counter);
    // send some message to check the callback chain
    testrun(0 < send(client, "test1", 5, 0));
    // sleep to let the thread work
    while (0 == counter) {
        usleep(TEST_DEFAULT_WAIT_USEC);
    }
    // callback was called
    testrun(1 == counter);
    // another run
    testrun(0 < send(client, "test2", 5, 0));
    // sleep to let the thread work
    while (1 == counter) {
        usleep(TEST_DEFAULT_WAIT_USEC);
    }
    // callback was called
    testrun(2 == counter);

    // Unsetting is undefined if the loop still runs in another thread ->
    // stop thread first, then unset callback, then start thread again
    loop->stop(loop);
    testrun(0 == pthread_join(thread, NULL));

    testrun(loop->callback.unset(loop, socket, NULL));

    testrun(0 == pthread_create(
                     &thread, NULL, run_the_loop_with_timeout, &container));

    // another run
    testrun(0 < send(client, "test3", 5, 0));
    // sleep for 1 second to let the thread work
    sleep(1);
    // callback was NOT called!
    testrun(2 == counter);
    // another run
    testrun(0 < send(client, "test4", 5, 0));
    // sleep for 1 second to let the thread work
    sleep(1);
    // callback was NOT called
    testrun(2 == counter);

    usleep(TEST_DEFAULT_WAIT_USEC);

    // reset callback
    testrun(loop->callback.set(
        loop, socket, OV_EVENT_IO_IN, &counter, counting_called_callback));

    // another run
    testrun(0 < send(client, "test5", 5, 0));
    while (2 == counter) {
        usleep(TEST_DEFAULT_WAIT_USEC);
    }

    // callback was called
    // it is not guaranteed that the events in between are stuck within some
    // socket buffer and delivered after re-registering the fd with the loop
    // thus the counter might be anything between 3 and 5
    testrun(2 <= counter);

    // trigger stop
    loop->stop(loop);
    testrun(0 == pthread_join(thread, NULL));

    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_event_loop_timer_set() {

    size_t max_sockets = 100;
    size_t max_timers = 100;

    ov_event_loop_config config = {

        .max.sockets = max_sockets, .max.timers = max_timers};

    ov_event_loop *loop = event_loop_creator(config);
    testrun(loop);
    testrun(loop->is_running(loop) == false);

    uint32_t id = 0;
    uint64_t start = 0;
    uint64_t idle_usec = 1000 * 500; // 500 msec

    testrun(0 == loop->timer.set(NULL, 0, NULL, NULL));
    testrun(0 == loop->timer.set(loop, 0, NULL, NULL));
    testrun(0 == loop->timer.set(NULL, 0, NULL, counting_timer_cb));

    /* It is NOT GUARANTEED that the first timer will receive ID 1 ! -> See
     * specs */
    id = loop->timer.set(loop, 0, NULL, counting_timer_cb);
    testrun(OV_TIMER_INVALID != id);

    testrun(loop->timer.unset(loop, id, NULL));

    /*
     *      Test to log the timing of the
     *      timer implementation.
     *
     */

    struct container2 container = {0};

    testrun_log("Check serial timers with different timespans");

    for (size_t i = 1; i < 20; i++) {

        container.timeout = 1000 * i; // i ms
        container.counter = 0;
        container.timestamp = 0;
        id = loop->timer.set(
            loop, container.timeout, &container, counting_timestamped_cb);
        start = ov_time_get_current_time_usecs();
        testrun(OV_TIMER_INVALID != id);
        while (0 == container.counter) {
            loop->run(loop, idle_usec);
        }
        testrun(1 == container.counter);
        testrun_log("Timeout %10" PRIu64
                    " usec "
                    "called after %10" PRIu64
                    " usec "
                    "offset %10" PRIi64 " usec ",
                    container.timeout,
                    container.timestamp - start,
                    container.timestamp - start - container.timeout);
        testrun(loop->timer.unset(loop, id, NULL));
    }

    for (size_t i = 100; i < 1000; i += 100) {

        container.timeout = 1000 * i; // i ms
        container.counter = 0;
        container.timestamp = 0;
        id = loop->timer.set(
            loop, container.timeout, &container, counting_timestamped_cb);
        testrun(OV_TIMER_INVALID != id);
        start = ov_time_get_current_time_usecs();
        while (0 == container.counter) {
            loop->run(loop, idle_usec);
        }
        testrun(1 == container.counter);
        testrun_log("Timeout %10" PRIu64
                    " usec "
                    "called after %10" PRIu64
                    " usec "
                    "offset %10" PRIi64 " usec ",
                    container.timeout,
                    container.timestamp - start,
                    container.timestamp - start - container.timeout);
        testrun(loop->timer.unset(loop, id, NULL));
    }

    /*
     *      Add multiple timers and check timing.
     *
     */

    struct container2 data[max_timers];
    clear_container2(data, sizeof(data) / sizeof(data[0]));

    uint32_t timer_id[max_timers];
    memset(timer_id, 0, sizeof(timer_id));

    for (size_t i = 1; i < max_timers; i++) {

        data[i].timeout = 1000 * 100 * i; // i ms
        data[i].counter = 0;
        data[i].timestamp = 0;
        timer_id[i] = loop->timer.set(
            loop, data[i].timeout, &data[i], counting_timestamped_cb);

        testrun(OV_TIMER_INVALID != timer_id);
    }

    testrun_log(
        "Check %zd parallel timers with different "
        "timespans",
        max_timers);

    start = ov_time_get_current_time_usecs();
    while (0 == data[max_timers - 1].counter) {
        loop->run(loop, idle_usec);
    }

    for (size_t i = 1; i < max_timers; i++) {

        testrun_log("Timeout %10" PRIu64
                    " usec "
                    "called after %10" PRIu64
                    " usec "
                    "offset %10" PRIi64 " usec ",
                    data[i].timeout,
                    data[i].timestamp - start,
                    data[i].timestamp - start - data[i].timeout);

        testrun(loop->timer.unset(loop, timer_id[i], NULL));
    }

    /*
     *      Check independent counting timers.
     *
     *      Counting should be
     *
     *      data[1] ++ (each run)
     *      data[2] ++ (each second run)
     *      data[3] ++ (each third run)
     */

    clear_container2(data, sizeof(data) / sizeof(data[0]));
    memset(timer_id, 0, sizeof(timer_id));

    for (size_t i = 1; i < 4; i++) {

        data[i].timeout = (idle_usec * i);
        data[i].counter = 0;
        data[i].timestamp = 0;
        data[i].loop = loop;
        timer_id[i] = loop->timer.set(
            loop, data[i].timeout, &data[i], counting_timestamped_reset_cb);
        testrun(OV_TIMER_INVALID != timer_id[i]);
    }

    idle_usec = data[1].timeout;

    int a = 0;
    int b = 0;

    for (size_t i = 1; i < 10; i++) {

        if (i % 2 == 0) a++;
        if (i % 3 == 0) b++;

        testrun(loop->run(loop, idle_usec));
    }

    testrun(a > 0);
    testrun(b > 0);

    // we let some room for incorrect timing resolution

    testrun((int)data[1].counter - 9 <= 2);
    testrun((int)data[2].counter - 4 <= 2);
    testrun((int)data[3].counter - 3 <= 2);

    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_event_loop_timer_unset() {

    int max_sockets = 100;
    int max_timers = 100;

    /* 1 sec */
    uint64_t rel_timeout_usecs = 1000 * 1000;

    int counter = 0;

    ov_event_loop_config config = {

        .max.sockets = max_sockets, .max.timers = max_timers};

    ov_event_loop *loop = event_loop_creator(config);
    testrun(loop);
    testrun(loop->is_running(loop) == false);

    uint32_t id = OV_TIMER_INVALID;

    id = loop->timer.set(loop, rel_timeout_usecs, &counter, counting_timer_cb);
    testrun(id != OV_TIMER_INVALID);

    testrun(loop->run(loop, 3 * rel_timeout_usecs / 2));
    testrun(1 == counter);

    testrun(!loop->timer.unset(NULL, 0, NULL));
    testrun(!loop->timer.unset(loop, OV_TIMER_INVALID, NULL));
    testrun(!loop->timer.unset(NULL, id, NULL));

    // unset timer id which is set
    testrun(loop->timer.unset(loop, id, NULL));

    counter = 0;

    testrun(loop->run(loop, 3 * rel_timeout_usecs / 2));
    testrun(0 == counter)

        // unset timer id which is not set
        testrun(loop->timer.unset(loop, id, NULL));

    // Set and immediately unset timer again to check that it won't be triggered
    id = loop->timer.set(loop, rel_timeout_usecs, &counter, counting_timer_cb);
    testrun(id != OV_TIMER_INVALID);

    testrun(loop->timer.unset(loop, id, 0));

    testrun(loop->run(loop, 3 * rel_timeout_usecs / 2));
    testrun(0 == counter);

    // Clean up
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}
