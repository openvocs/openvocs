/***

  Copyright   2018       German Aerospace Center DLR e.V.,
  German Space Operations Center (GSOC)

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by processlicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

This file is part of the openvocs project. http://openvocs.org

 ***/ /**

      \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
      \date               2018-07-12

     **/
/*---------------------------------------------------------------------------*/

#include "ov_thread_loop.c"
#include <ov_test/ov_test.h>
#include <pthread.h>

#include <unistd.h>

/*---------------------------------------------------------------------------*/

#define MAX_STRLEN 50000

/******************************************************************************
 *                               HELPER METHODS
 ******************************************************************************/

static bool configs_equal(ov_thread_loop_config c1, ov_thread_loop_config c2) {

    if (c1.message_queue_capacity != c2.message_queue_capacity) return false;
    if (c1.lock_timeout_usecs != c2.lock_timeout_usecs) return false;
    if (c1.num_threads != c2.num_threads) return false;
    if (c1.disable_to_loop_queue != c2.disable_to_loop_queue) return false;

    return true;
}

/******************************************************************************
 *                                   TESTS
 ******************************************************************************/

static bool test_handler(ov_thread_loop *p, ov_thread_message *msg) {

    UNUSED(p);

    return 0 != msg;
}

/*----------------------------------------------------------------------------*/

int test_ov_thread_loop_create() {

    ov_thread_loop_callbacks callbacks = {0};

    ov_thread_loop *process = ov_thread_loop_create(0, callbacks, 0);

    testrun(0 == process);

    callbacks.handle_message_in_loop = test_handler;

    process = ov_thread_loop_create(0, callbacks, 0);

    testrun(0 == process);

    ov_event_loop *loop = ov_event_loop_default(ov_event_loop_config_default());
    testrun(0 != loop);

    process = ov_thread_loop_create(loop, callbacks, 0);
    testrun(0 == process);

    callbacks.handle_message_in_thread = test_handler;

    process = ov_thread_loop_create(loop, callbacks, 0);
    testrun(0 != process);

    testrun(0 == ov_thread_loop_free(process));

    int test_data = 42;

    process = ov_thread_loop_create(loop, callbacks, &test_data);

    testrun(0 != process);

    void *data_ptr = ov_thread_loop_get_data(process);

    testrun(0 != data_ptr);
    testrun(42 == *(int *)data_ptr);

    process = ov_thread_loop_free(process);
    testrun(0 == process);

    loop = ov_event_loop_free(loop);
    testrun(0 == loop);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_thread_loop_free() {

    testrun(0 == ov_thread_loop_free(0));

    ov_thread_loop_callbacks callbacks = {

        .handle_message_in_loop = test_handler,
        .handle_message_in_thread = test_handler,

    };

    ov_event_loop *loop = ov_event_loop_default(ov_event_loop_config_default());

    ov_thread_loop *tpp = ov_thread_loop_create(loop, callbacks, 0);

    tpp = ov_thread_loop_free(tpp);

    testrun(0 == tpp);

    loop = ov_event_loop_free(loop);

    testrun(0 == loop);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_thread_loop_get_data() {

    testrun(0 == ov_thread_loop_get_data(0));

    ov_thread_loop_callbacks callbacks = {

        .handle_message_in_loop = test_handler,
        .handle_message_in_thread = test_handler,

    };

    ov_event_loop *loop = ov_event_loop_default(ov_event_loop_config_default());

    ov_thread_loop *tpp = ov_thread_loop_create(loop, callbacks, 0);

    testrun(0 == ov_thread_loop_get_data(tpp));

    tpp = ov_thread_loop_free(tpp);

    testrun(0 == tpp);

    int test_data = 42;

    tpp = ov_thread_loop_create(loop, callbacks, &test_data);

    int *received_data = ov_thread_loop_get_data(tpp);

    testrun(&test_data == received_data);
    testrun(test_data == *received_data);

    tpp = ov_thread_loop_free(tpp);

    testrun(0 == tpp);

    loop = ov_event_loop_free(loop);

    testrun(0 == loop);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_thread_loop_reconfigure() {

    ov_thread_loop_config config = {0};

    testrun(!ov_thread_loop_reconfigure(0, config));

    ov_thread_loop_callbacks callbacks = {

        .handle_message_in_loop = test_handler,
        .handle_message_in_thread = test_handler,

    };

    ov_event_loop *loop = ov_event_loop_default(ov_event_loop_config_default());
    testrun(0 != loop);

    ov_thread_loop *tpp = ov_thread_loop_create(loop, callbacks, 0);

    testrun(ov_thread_loop_reconfigure(tpp, config));

    config.message_queue_capacity = 10;

    testrun(ov_thread_loop_reconfigure(tpp, config));

    config.lock_timeout_usecs = 12345;

    testrun(ov_thread_loop_reconfigure(tpp, config));

    tpp = ov_thread_loop_free(tpp);

    testrun(0 == tpp);

    loop = ov_event_loop_free(loop);

    testrun(0 == loop);

    return testrun_log_success();
}

/******************************************************************************
 *                                send_message
 ******************************************************************************/

const ov_thread_message_type NUM_MESSAGE_TYPE =
    OV_THREAD_MESSAGE_TYPE_ENSURE_SIGNED_INT_TYPE;

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_thread_message public;
    size_t msg_number;

} num_message;

/*----------------------------------------------------------------------------*/

ov_thread_message *free_num_message(ov_thread_message *msg) {

    if (0 != msg) free(msg);

    msg = 0;

    return msg;
}

/*---------------------------------------------------------------------------*/

ov_thread_message *create_num_message(size_t num) {

    num_message *msg = calloc(1, sizeof(num_message));

    msg->public = (ov_thread_message){
        .magic_bytes = OV_THREAD_MESSAGE_MAGIC_BYTES,
        .type = NUM_MESSAGE_TYPE,
        .free = free_num_message,
    };

    msg->msg_number = num;

    return (ov_thread_message *)msg;
}

/*----------------------------------------------------------------------------*/

ssize_t msg_get_num(ov_thread_message *msg) {

    if (msg->magic_bytes != OV_THREAD_MESSAGE_MAGIC_BYTES) goto error;

    if (msg->type != NUM_MESSAGE_TYPE) goto error;

    num_message *nmsg = (num_message *)msg;

    return nmsg->msg_number;

error:

    return -1;
}

/*----------------------------------------------------------------------------*/

static ov_event_loop *loop = 0;

static ov_thread_loop *tpp = 0;

/*----------------------------------------------------------------------------*/

static size_t in_loop_msg_received[50] = {0};

static size_t in_loop_invalid_msgs = 0;

static bool test_in_loop_handler(ov_thread_loop *self,
                                 ov_thread_message *message) {

    UNUSED(self);

    ssize_t num = msg_get_num(message);

    if (0 > num) goto error;

    in_loop_msg_received[num] = true;

    message = message->free(message);

    return true;

error:

    ++in_loop_invalid_msgs;
    return false;
}

/*----------------------------------------------------------------------------*/

static bool in_thread_msg_received[50] = {0};

static size_t in_thread_invalid_msgs = 0;

static bool test_in_thread_handler(ov_thread_loop *self,
                                   ov_thread_message *message) {

    UNUSED(self);

    ssize_t num = msg_get_num(message);

    if (0 > num) goto error;

    in_thread_msg_received[num] = true;

    if (!ov_thread_loop_send_message(tpp, message, OV_RECEIVER_EVENT_LOOP))
        goto error;

    return true;

error:

    ++in_thread_invalid_msgs;
    return false;
}

/*----------------------------------------------------------------------------*/

static const useconds_t ONE_SECOND = 1000 * 1000;
static size_t num_messages_to_send = 0;

static void *feeder_thread(void *arg) {

    UNUSED(arg);

    OV_ASSERT(0 != loop);
    OV_ASSERT(0 != tpp);

    for (size_t i = 0; num_messages_to_send > i; ++i) {

        ov_thread_loop_send_message(
            tpp, create_num_message(i), OV_RECEIVER_THREAD);
    }

    /* Wait for loop to process ... */
    usleep(3 * ONE_SECOND);

    loop->stop(loop);

    return 0;
}

/*----------------------------------------------------------------------------*/

pthread_t spawn_send_thread_for_msgs(size_t num_messages) {

    pthread_t pt;

    num_messages_to_send = num_messages;

    pthread_create(&pt, 0, feeder_thread, 0);

    return pt;
}

/*----------------------------------------------------------------------------*/

int test_ov_thread_loop_send_message() {

    size_t msg_queue_len = 20;

    testrun(!ov_thread_loop_send_message(0, 0, OV_RECEIVER_THREAD));

    testrun(!ov_thread_loop_send_message(0, 0, OV_RECEIVER_EVENT_LOOP));

    loop = ov_event_loop_default(ov_event_loop_config_default());

    testrun(0 != loop);

    ov_thread_loop_callbacks callbacks = {
        .handle_message_in_thread = test_in_thread_handler,
        .handle_message_in_loop = test_in_loop_handler,
    };

    int testdata = 42;

    tpp = ov_thread_loop_create(loop, callbacks, &testdata);

    testrun(ov_thread_loop_start_threads(tpp));

    pthread_t pt = spawn_send_thread_for_msgs(10);

    loop->run(loop, 20 * ONE_SECOND);

    void *retval;

    pthread_join(pt, &retval);

    const size_t msg_array_len =
        sizeof(in_thread_msg_received) / sizeof(in_thread_msg_received[0]);

    testrun(0 == in_thread_invalid_msgs);
    testrun(0 == in_loop_invalid_msgs);

    for (size_t i = 0; i < 10; ++i) {

        testrun(in_thread_msg_received[i]);
        testrun(in_loop_msg_received[i]);
    }

    for (size_t i = 10; i < msg_array_len; ++i) {

        testrun(!in_thread_msg_received[i]);
        testrun(!in_loop_msg_received[i]);
    }

    for (size_t i = 0; i < msg_array_len; ++i) {

        in_thread_msg_received[i] = false;
        in_loop_msg_received[i] = false;
    }

    ov_thread_loop_config cfg = {

        .message_queue_capacity = msg_queue_len,
        .num_threads = 4,

    };

    testrun(ov_thread_loop_reconfigure(tpp, cfg));

    testrun(ov_thread_loop_start_threads(tpp));

    pt = spawn_send_thread_for_msgs(50);

    loop->run(loop, 10 * ONE_SECOND);

    pthread_join(pt, &retval);

    testrun(0 == in_thread_invalid_msgs);
    testrun(0 == in_loop_invalid_msgs);

    size_t num_in_thread_msgs_received = 0;
    size_t num_in_loop_msgs_received = 0;

    for (size_t i = 0; i < msg_array_len; ++i) {

        if (in_thread_msg_received[i]) ++num_in_thread_msgs_received;
        if (in_loop_msg_received[i]) ++num_in_loop_msgs_received;
    }

    testrun(msg_queue_len - 2 < num_in_thread_msgs_received);
    testrun(msg_queue_len - 2 < num_in_loop_msgs_received);

    tpp = ov_thread_loop_free(tpp);

    testrun(0 == tpp);

    loop = ov_event_loop_free(loop);

    testrun(0 == loop);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_thread_pool_process_start() {

    testrun(!ov_thread_loop_start_threads(0));

    ov_event_loop *loop = ov_event_loop_default(ov_event_loop_config_default());

    testrun(0 != loop);

    ov_thread_loop_callbacks callbacks = {

        .handle_message_in_loop = test_handler,
        .handle_message_in_thread = test_handler,

    };

    ov_thread_loop *tpp = ov_thread_loop_create(loop, callbacks, 0);

    testrun(0 != tpp);

    testrun(ov_thread_loop_start_threads(tpp));

    testrun(0 == ov_thread_loop_free(tpp));

    tpp = 0;

    testrun(0 == ov_event_loop_free(loop));

    loop = 0;

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_thread_pool_process_stop() {

    testrun(!ov_thread_loop_stop_threads(0));

    ov_event_loop *loop = ov_event_loop_default(ov_event_loop_config_default());

    testrun(0 != loop);

    ov_thread_loop_callbacks callbacks = {

        .handle_message_in_loop = test_handler,
        .handle_message_in_thread = test_handler,

    };

    ov_thread_loop *tpp = ov_thread_loop_create(loop, callbacks, 0);

    testrun(0 != tpp);

    testrun(!ov_thread_loop_stop_threads(tpp));

    testrun(ov_thread_loop_start_threads(tpp));

    testrun(ov_thread_loop_stop_threads(tpp));

    testrun(0 == ov_thread_loop_free(tpp));

    tpp = 0;

    testrun(0 == ov_event_loop_free(loop));

    loop = 0;
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_thread_loop_config_to_json() {

    ov_thread_loop_config config = {0};

    ov_thread_loop_config read_config;

    ov_json_value *json = ov_thread_loop_config_to_json(config, 0);

    /* As we set everything to zero, we requested the default values
     * - set them as desired */
    config = (ov_thread_loop_config){
        .message_queue_capacity = MESSAGE_QUEUE_CAPACITY_DEFAULT,
        .lock_timeout_usecs = LOCK_TIMEOUT_USECS_DEFAULT,
        .num_threads = NUM_THREADS_DEFAULT,
        .disable_to_loop_queue = false,
    };

    read_config = ov_thread_loop_config_from_json(json);
    json = json->free(json);

    testrun(configs_equal(config, read_config));

    config.num_threads = 15;

    json = ov_thread_loop_config_to_json(config, 0);
    read_config = ov_thread_loop_config_from_json(json);
    json = json->free(json);
    testrun(configs_equal(config, read_config));

    config.disable_to_loop_queue = true;

    json = ov_thread_loop_config_to_json(config, 0);
    read_config = ov_thread_loop_config_from_json(json);
    json = json->free(json);
    testrun(configs_equal(config, read_config));

    config.lock_timeout_usecs = 41221;

    json = ov_thread_loop_config_to_json(config, 0);
    read_config = ov_thread_loop_config_from_json(json);
    json = json->free(json);
    testrun(configs_equal(config, read_config));

    config.message_queue_capacity = 41;

    json = ov_thread_loop_config_to_json(config, 0);
    read_config = ov_thread_loop_config_from_json(json);
    json = json->free(json);
    testrun(configs_equal(config, read_config));

    ++config.num_threads;

    json = ov_thread_loop_config_to_json(config, 0);
    read_config = ov_thread_loop_config_from_json(json);
    json = json->free(json);
    testrun(configs_equal(config, read_config));

    /* And a last time with an existing JSON object */
    json = ov_json_object();
    testrun(json == ov_thread_loop_config_to_json(config, json));
    read_config = ov_thread_loop_config_from_json(json);
    json = json->free(json);
    testrun(configs_equal(config, read_config));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_thread_loop_config_from_json() {

    ov_thread_loop_config default_cfg = {
        .message_queue_capacity = MESSAGE_QUEUE_CAPACITY_DEFAULT,
        .lock_timeout_usecs = LOCK_TIMEOUT_USECS_DEFAULT,
        .num_threads = NUM_THREADS_DEFAULT,
        .disable_to_loop_queue = false,
    };

    ov_thread_loop_config cfg_empty = {0};

    ov_thread_loop_config expected = default_cfg;

    testrun(configs_equal(cfg_empty, ov_thread_loop_config_from_json(0)));

    ov_json_value *jval = ov_json_decode("{}");

    testrun(configs_equal(expected, ov_thread_loop_config_from_json(jval)));

    jval = jval->free(jval);

    jval = ov_json_decode("{\"" CONFIG_KEY_MESSAGE_QUEUE_CAPACITY
                          "\" : "
                          "13}");

    expected.message_queue_capacity = 13;

    testrun(configs_equal(expected, ov_thread_loop_config_from_json(jval)));

    jval = jval->free(jval);

    jval = ov_json_decode("{\"" CONFIG_KEY_LOCK_TIMEOUT_USECS "\" :24}");

    expected.message_queue_capacity = MESSAGE_QUEUE_CAPACITY_DEFAULT;
    expected.lock_timeout_usecs = 24;

    testrun(configs_equal(expected, ov_thread_loop_config_from_json(jval)));

    jval = jval->free(jval);

    jval = ov_json_decode("{\"" CONFIG_KEY_NUM_THREADS "\":35}");

    expected.lock_timeout_usecs = LOCK_TIMEOUT_USECS_DEFAULT;
    expected.num_threads = 35;

    testrun(configs_equal(expected, ov_thread_loop_config_from_json(jval)));

    jval = jval->free(jval);

    jval = ov_json_decode("{\"" CONFIG_KEY_DISABLE_TO_LOOP_QUEUE
                          "\":"
                          "true}");

    expected = default_cfg;
    expected.disable_to_loop_queue = true;

    testrun(configs_equal(expected, ov_thread_loop_config_from_json(jval)));

    jval = jval->free(jval);

    jval = ov_json_decode("{\"" CONFIG_KEY_DISABLE_TO_LOOP_QUEUE
                          "\":true,"
                          "\"" CONFIG_KEY_NUM_THREADS
                          "\":9,"
                          "\"" CONFIG_KEY_MESSAGE_QUEUE_CAPACITY
                          "\":8,"
                          "\"" CONFIG_KEY_LOCK_TIMEOUT_USECS
                          "\":7"
                          "}");

    expected = (ov_thread_loop_config){.message_queue_capacity = 8,
                                       .lock_timeout_usecs = 7,
                                       .num_threads = 9,
                                       .disable_to_loop_queue = true};

    testrun(configs_equal(expected, ov_thread_loop_config_from_json(jval)));

    jval = jval->free(jval);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

OV_TEST_RUN("ov_thread_loop",
            test_ov_thread_loop_create,
            test_ov_thread_loop_free,
            test_ov_thread_loop_get_data,
            test_ov_thread_loop_reconfigure,
            test_ov_thread_loop_send_message,
            test_ov_thread_pool_process_start,
            test_ov_thread_pool_process_stop,
            test_ov_thread_loop_config_to_json,
            test_ov_thread_loop_config_from_json);

/*----------------------------------------------------------------------------*/
