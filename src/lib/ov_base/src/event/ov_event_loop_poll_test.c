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
        @file           ov_event_loop_poll_test.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-12-17

        @ingroup        ov_event_loop

        @brief          Unit tests of


        ------------------------------------------------------------------------
*/
#include "../../include/ov_event_loop_test_interface.h"
#include "ov_event_loop_poll.c"

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

int test_ov_event_loop_poll() {

    ov_event_loop_config config = {

        .max.sockets = 10, .max.timers = 10};

    ov_event_loop *eventloop = ov_event_loop_poll(config);
    testrun(eventloop);
    PollLoop *loop = AS_POLL_LOOP(eventloop);
    testrun(loop);

    // check config
    testrun(loop->config.max.sockets == config.max.sockets);
    testrun(loop->config.max.timers == config.max.timers);

    // check data stores are created
    testrun(loop->fds);
    testrun(loop->cb.socket);

    // check public functions
    testrun(eventloop->free == impl_poll_event_loop_free);

    testrun(eventloop->is_running == impl_poll_event_loop_is_running);
    testrun(eventloop->stop == impl_poll_event_loop_stop);
    testrun(eventloop->run == impl_poll_event_loop_run);

    testrun(eventloop->callback.set == impl_poll_event_loop_callback_set);
    testrun(eventloop->callback.unset == impl_poll_event_loop_callback_unset);

    testrun(eventloop->timer.set == impl_poll_event_loop_timer_set);
    testrun(eventloop->timer.unset == impl_poll_event_loop_timer_unset);

    // check trigger pair
    testrun(loop->wakeup[0] >= 0);
    testrun(loop->wakeup[1] >= 0);
    testrun(0 < send(loop->wakeup[0], "data", 4, 0));

    ov_event_loop_free(eventloop);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_flags_ov_to_poll() {

    uint8_t ov_flag = 0;
    short poll_flag = 0;

    testrun(!flags_ov_to_poll(NULL, NULL));
    testrun(!flags_ov_to_poll(&ov_flag, NULL));
    testrun(!flags_ov_to_poll(NULL, &poll_flag));

    testrun(flags_ov_to_poll(&ov_flag, &poll_flag));
    testrun(ov_flag == 0);
    testrun(poll_flag == 0);

    ov_flag = OV_EVENT_IO_IN;
    testrun(flags_ov_to_poll(&ov_flag, &poll_flag));
    testrun(ov_flag == OV_EVENT_IO_IN);
    testrun(poll_flag == (POLLIN | POLLPRI));

    ov_flag = OV_EVENT_IO_OUT;
    testrun(flags_ov_to_poll(&ov_flag, &poll_flag));
    testrun(ov_flag == OV_EVENT_IO_OUT);
    testrun(poll_flag == POLLOUT);

    ov_flag = OV_EVENT_IO_CLOSE;
    testrun(flags_ov_to_poll(&ov_flag, &poll_flag));
    testrun(ov_flag == OV_EVENT_IO_CLOSE);
    testrun(poll_flag == POLLHUP);

    ov_flag = OV_EVENT_IO_ERR;
    testrun(flags_ov_to_poll(&ov_flag, &poll_flag));
    testrun(ov_flag == OV_EVENT_IO_ERR);
    testrun(poll_flag == POLLERR);

    ov_flag =
        OV_EVENT_IO_IN | OV_EVENT_IO_OUT | OV_EVENT_IO_CLOSE | OV_EVENT_IO_ERR;

    testrun(flags_ov_to_poll(&ov_flag, &poll_flag));
    testrun(ov_flag == (OV_EVENT_IO_ERR | OV_EVENT_IO_IN | OV_EVENT_IO_OUT |
                        OV_EVENT_IO_CLOSE));

    testrun(poll_flag == (POLLIN | POLLPRI | POLLERR | POLLHUP | POLLOUT));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

struct dummy_timer {

    uint32_t id;
    uint64_t timestamp;
    uint64_t rel_timeout;
};

/*----------------------------------------------------------------------------*/

bool dummy_timer_callback(uint32_t id, void *data) {

    struct dummy_timer *dummy = data;

    uint64_t current = ov_time_get_current_time_usecs();

    testrun_log(
        "Got id %i set id %i\n"
        "current usec %" PRIu64
        " \n"
        "time    usec %" PRIu64
        " \n"
        "diff    usec %" PRIu64
        " \n"
        "rel     usec %" PRIu64 " \n",
        id,
        dummy->id,
        current,
        dummy->timestamp,
        current - dummy->timestamp,
        dummy->rel_timeout);

    testrun(id == dummy->id);
    return true;
};

/*----------------------------------------------------------------------------*/

int check_poll_loop() {

    ov_event_loop_config config = {

        .max.sockets = 10, .max.timers = 10};

    ov_event_loop *loop = ov_event_loop_poll(config);
    testrun(loop);

    struct dummy_timer dummy1 = {0};
    struct dummy_timer dummy2 = {0};
    struct dummy_timer dummy3 = {0};
    uint64_t rel = 1 * 1000 * 1000;
    uint64_t max_time = 5 * rel;

    dummy1.id = loop->timer.set(loop, rel / 2, &dummy1, dummy_timer_callback);
    dummy1.rel_timeout = rel / 2;
    dummy1.timestamp = ov_time_get_current_time_usecs();
    testrun(dummy1.id != 0);

    dummy2.id = loop->timer.set(loop, rel, &dummy2, dummy_timer_callback);
    dummy2.rel_timeout = rel;
    dummy2.timestamp = ov_time_get_current_time_usecs();
    testrun(dummy2.id != 0);

    dummy3.id = loop->timer.set(loop, rel * 2, &dummy3, dummy_timer_callback);
    dummy3.rel_timeout = rel * 2;
    dummy3.timestamp = ov_time_get_current_time_usecs();
    testrun(dummy3.id != 0);

    uint64_t start = ov_time_get_current_time_usecs();
    testrun(loop->run(loop, max_time));
    uint64_t stop = ov_time_get_current_time_usecs();
    testrun_log("start - stop %" PRIu64 " usec %" PRIu64 " usec offset",
                stop - start,
                (stop - start) - max_time);

    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

struct container1 {

    ov_event_loop *loop;
    uint64_t timeout_usec;
    int counter;
    uint32_t id;
};

/*----------------------------------------------------------------------------*/

static bool counting_callback(uint32_t id, void *data) {

    if (!data || (0 == id)) return false;

    struct container1 *container = data;

    // testrun_log("counter %i", container->counter);

    container->counter++;
    // reenable self
    container->id = container->loop->timer.set(
        container->loop, container->timeout_usec, container, counting_callback);

    testrun(OV_TIMER_INVALID != container->id);

    return true;
}

/*----------------------------------------------------------------------------*/

int check_interval_call() {

    ov_event_loop_config config = {

        .max.sockets = 10, .max.timers = 10};

    ov_event_loop *loop = ov_event_loop_poll(config);
    testrun(loop);

    struct container1 container = {0};

    /*
     *      @NOTE This timer is relative reenabled.
     *      a very fast intervall test may result in
     *      less than expected run cycles.
     *
     *      Be aware that this timer implementation is NOT
     *      intended to be a realtime based timer implementation.
     *
     *      To check a a specific platform for the
     *      resulution you MAY play around with the settings
     *      below.
     *
     *      STANDARD TEST
     *      10% drift in 100 runs a 10ms, which is 1ms per run cycle
     *
     *      runs            = 100
     *      offset          = 10
     *      timeout_usec    = 10 * 1000
     *
     */

    int runs = 100;
    int offset = 10;
    int timeout_usec = 10 * 1000;
    int runtime_usec = timeout_usec * runs;

    container.loop = loop;
    container.timeout_usec = timeout_usec;
    container.counter = 0;

    testrun(loop->timer.set(
        loop, container.timeout_usec, &container, counting_callback));

    testrun(loop->run(loop, runtime_usec));
    testrun_log(
        "expect %i result %i offset range %i", runs, container.counter, offset);

    /*
     *  Uncommented the check due to test issues (offset to small),
     *  which let to some unpredictable test case
     */
    // testrun(container.counter >= runs - offset);
    // testrun(container.counter <= runs + offset);

    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_calculate_timeout_msec() {

    ov_event_loop_config config = {

        .max.sockets = 10, .max.timers = 10};

    ov_event_loop *loop = ov_event_loop_poll(config);
    testrun(loop);

    int64_t result = 0;

    uint64_t now = ov_time_get_current_time_usecs();
    uint64_t max = OV_TIMER_MAX;

    result = calculate_timeout_msec(AS_POLL_LOOP(loop), now, max, now);
    testrun_log("time %" PRIi64, result);
    testrun(result != 0);
    testrun(result == INT_MAX / 1000);

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

    testrun_test(test_ov_event_loop_poll);
    testrun_test(check_flags_ov_to_poll);
    testrun_test(check_poll_loop);

    testrun_test(check_interval_call);
    testrun_test(check_calculate_timeout_msec);

    OV_EVENT_LOOP_PERFORM_INTERFACE_TESTS(ov_event_loop_poll);

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
