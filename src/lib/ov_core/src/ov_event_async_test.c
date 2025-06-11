/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_event_async_test.c
        @author         Markus Töpfer

        @date           2021-06-16


        ------------------------------------------------------------------------
*/
#include "ov_event_async.c"
#include <ov_base/ov_id.h>
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_event_async_store_create() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_event_async_store_config config = (ov_event_async_store_config){0};
    testrun(NULL == ov_event_async_store_create(config));
    config.loop = loop;

    ov_event_async_store *store = ov_event_async_store_create(config);
    testrun(store);
    testrun(store->dict);
    testrun(OV_TIMER_INVALID != store->invalidate_timer);
    testrun(store->config.threadlock_timeout_usec ==
            IMPL_THREADLOCK_TIMEOUT_USEC);
    testrun(store->config.invalidate_check_interval_usec ==
            IMPL_INVALIDATE_TIMEOUT_USEC);

    testrun(ov_thread_lock_try_lock(&store->lock));
    testrun(ov_thread_lock_unlock(&store->lock));

    testrun(NULL == ov_event_async_store_free(store));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_async_store_free() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_event_async_store_config config =
        (ov_event_async_store_config){.loop = loop};

    ov_event_async_store *store = ov_event_async_store_create(config);
    testrun(store);
    testrun(NULL == ov_event_async_store_free(NULL));
    testrun(NULL == ov_event_async_store_free(store));

    /* free with data */

    store = ov_event_async_store_create(config);

    testrun(ov_event_async_set(
        store,
        "1",
        (ov_event_async_data){.socket = 1, .value = ov_json_object()},
        1000));

    testrun(ov_event_async_set(
        store,
        "2",
        (ov_event_async_data){.socket = 1, .value = ov_json_object()},
        1000));

    testrun(ov_event_async_set(
        store,
        "3",
        (ov_event_async_data){.socket = 1, .value = ov_json_object()},
        1000));

    testrun(NULL == ov_event_async_store_free(store));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_async_set() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_event_async_store *store = ov_event_async_store_create(
        (ov_event_async_store_config){.loop = loop});

    testrun(store);

    ov_json_value *val1 = ov_json_object();
    ov_json_value *val2 = ov_json_object();
    ov_json_value *val3 = ov_json_object();

    ov_event_async_data data1 =
        (ov_event_async_data){.socket = 1, .value = val1};

    ov_event_async_data data2 =
        (ov_event_async_data){.socket = 2, .value = val2};

    ov_event_async_data data3 =
        (ov_event_async_data){.socket = 3, .value = val3};

    testrun(!ov_event_async_set(NULL, NULL, (ov_event_async_data){0}, 0));
    testrun(!ov_event_async_set(store, NULL, (ov_event_async_data){0}, 0));
    testrun(!ov_event_async_set(NULL, "1", (ov_event_async_data){0}, 0));

    testrun(!ov_dict_get(store->dict, "1"));
    testrun(ov_event_async_set(store, "1", (ov_event_async_data){0}, 0));
    testrun(ov_dict_get(store->dict, "1"));

    uint64_t start = ov_time_get_current_time_usecs();
    usleep(1000);

    // override
    testrun(ov_event_async_set(store, "1", data1, 0));
    testrun(ov_dict_get(store->dict, "1"));
    testrun(1 == ov_dict_count(store->dict));

    testrun(ov_event_async_set(store, "2", data2, 2));
    testrun(2 == ov_dict_count(store->dict));

    testrun(ov_event_async_set(store, "3", data3, 1000));
    testrun(3 == ov_dict_count(store->dict));

    usleep(1000);

    // check content
    uint64_t now = ov_time_get_current_time_usecs();
    internal_session_data *internal =
        (internal_session_data *)ov_dict_get(store->dict, "1");
    testrun(internal);
    testrun(internal->created_usec > start);
    testrun(internal->created_usec < now);
    testrun(internal->max_lifetime_usec == 0);
    testrun(internal->data.socket == 1);
    testrun(internal->data.value == val1);

    internal = (internal_session_data *)ov_dict_get(store->dict, "2");
    testrun(internal);
    testrun(internal->created_usec > start);
    testrun(internal->created_usec < now);
    testrun(internal->max_lifetime_usec == 2);
    testrun(internal->data.socket == 2);
    testrun(internal->data.value == val2);

    internal = (internal_session_data *)ov_dict_get(store->dict, "3");
    testrun(internal);
    testrun(internal->created_usec > start);
    testrun(internal->created_usec < now);
    testrun(internal->max_lifetime_usec == 1000);
    testrun(internal->data.socket == 3);
    testrun(internal->data.value == val3);

    // remove some item
    ov_event_async_data out = ov_event_async_unset(store, "2");
    testrun(2 == ov_dict_count(store->dict));
    testrun(out.socket == 2);
    testrun(out.value == val2);

    // try to add locked
    testrun(2 == ov_dict_count(store->dict));
    testrun(ov_thread_lock_try_lock(&store->lock));
    testrun(!ov_event_async_set(store, "2", data2, 0));
    testrun(2 == ov_dict_count(store->dict));
    testrun(ov_thread_lock_unlock(&store->lock));
    testrun(ov_event_async_set(store, "2", data2, 0));
    testrun(3 == ov_dict_count(store->dict));

    // remove 2 again
    out = ov_event_async_unset(store, "2");
    testrun(2 == ov_dict_count(store->dict));
    testrun(out.socket == 2);
    testrun(out.value == val2);
    out.value = ov_json_value_free(out.value);

    // remove 3
    out = ov_event_async_unset(store, "3");
    testrun(1 == ov_dict_count(store->dict));
    testrun(out.socket == 3);
    testrun(out.value == val3);
    out.value = ov_json_value_free(out.value);

    testrun(NULL == ov_event_async_store_free(store));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_async_unset() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_event_async_store *store = ov_event_async_store_create(
        (ov_event_async_store_config){.loop = loop});

    testrun(store);

    testrun(ov_event_async_set(
        store,
        "1",
        (ov_event_async_data){.socket = 1, .value = ov_json_object()},
        1000));

    testrun(ov_event_async_set(
        store,
        "2",
        (ov_event_async_data){.socket = 2, .value = ov_json_object()},
        1000));

    testrun(ov_event_async_set(
        store,
        "3",
        (ov_event_async_data){.socket = 3, .value = ov_json_object()},
        1000));

    testrun(3 == ov_dict_count(store->dict));

    ov_event_async_data out = ov_event_async_unset(NULL, NULL);
    testrun(out.socket == 0);
    testrun(out.value == NULL);

    out = ov_event_async_unset(store, NULL);
    testrun(out.socket == 0);
    testrun(out.value == NULL);

    out = ov_event_async_unset(NULL, "1");
    testrun(out.socket == 0);
    testrun(out.value == NULL);

    testrun(3 == ov_dict_count(store->dict));

    out = ov_event_async_unset(store, "1");
    testrun(out.socket == 1);
    testrun(out.value != NULL);
    out.value = ov_json_value_free(out.value);

    testrun(2 == ov_dict_count(store->dict));

    // try to unset non set
    out = ov_event_async_unset(store, "1");
    testrun(out.socket == 0);
    testrun(out.value == NULL);

    // try to unset locked
    testrun(ov_thread_lock_try_lock(&store->lock));
    out = ov_event_async_unset(store, "2");
    testrun(out.socket == 0);
    testrun(out.value == NULL);

    testrun(2 == ov_dict_count(store->dict));

    // unlock and unset
    testrun(ov_thread_lock_unlock(&store->lock));
    out = ov_event_async_unset(store, "2");
    testrun(out.socket == 2);
    testrun(out.value != NULL);
    out.value = ov_json_value_free(out.value);

    testrun(1 == ov_dict_count(store->dict));

    testrun(NULL == ov_event_async_store_free(store));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

struct dummy_userdata {

    int socket;
    ov_json_value *value;
};

/*----------------------------------------------------------------------------*/

static void dummy_timedout_callback(void *userdata, ov_event_async_data data) {

    struct dummy_userdata *x = (struct dummy_userdata *)userdata;
    x->socket = data.socket;
    x->value = data.value;

    UNUSED(data);
    UNUSED(send);

    return;
};

/*----------------------------------------------------------------------------*/

int check_ov_event_async_invalidation() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_event_async_store *store =
        ov_event_async_store_create((ov_event_async_store_config){
            .loop = loop, .invalidate_check_interval_usec = 20000});

    testrun(store);
    testrun(store->config.invalidate_check_interval_usec == 20000);

    testrun(ov_event_async_set(
        store,
        "1",
        (ov_event_async_data){.socket = 1, .value = ov_json_object()},
        500000));

    testrun(ov_event_async_set(
        store,
        "2",
        (ov_event_async_data){.socket = 2, .value = ov_json_object()},
        1000000));

    testrun(ov_event_async_set(
        store,
        "3",
        (ov_event_async_data){.socket = 3, .value = ov_json_object()},
        200000));

    testrun(3 == ov_dict_count(store->dict));

    /* check timer not fired */

    testrun(loop->run(loop, OV_RUN_ONCE));
    testrun(3 == ov_dict_count(store->dict));

    /* expect first invalidation */
    testrun(loop->run(loop, 220000));

    testrun(2 == ov_dict_count(store->dict));
    testrun(ov_dict_get(store->dict, "1"));
    testrun(ov_dict_get(store->dict, "2"));

    /* expect next invalidation */

    testrun(loop->run(loop, 300000));

    testrun(1 == ov_dict_count(store->dict));
    testrun(ov_dict_get(store->dict, "2"));

    /* expect last invalidation */

    testrun(loop->run(loop, 500000));

    testrun(0 == ov_dict_count(store->dict));

    /* set same values again */

    testrun(ov_event_async_set(
        store,
        "1",
        (ov_event_async_data){.socket = 1, .value = ov_json_object()},
        50000));

    testrun(ov_event_async_set(
        store,
        "2",
        (ov_event_async_data){.socket = 2, .value = ov_json_object()},
        500000));

    testrun(ov_event_async_set(
        store,
        "3",
        (ov_event_async_data){.socket = 3, .value = ov_json_object()},
        10000));

    /* lock store */
    testrun(ov_thread_lock_try_lock(&store->lock));

    testrun(3 == ov_dict_count(store->dict));
    testrun(loop->run(loop, 50000));
    testrun(3 == ov_dict_count(store->dict));

    /* unlock and expect deletion of first two timed out items */
    testrun(ov_thread_lock_unlock(&store->lock));

    testrun(loop->run(loop, 50000));
    testrun(1 == ov_dict_count(store->dict));

    testrun(loop->run(loop, 500000));
    testrun(0 == ov_dict_count(store->dict));

    /* check timeout callback */

    ov_json_value *input1 = ov_json_object();
    ov_json_value *input2 = ov_json_object();

    struct dummy_userdata userdata = (struct dummy_userdata){0};

    testrun(ov_event_async_set(
        store,
        "1",
        (ov_event_async_data){.socket = 1,
                              .value = input1,
                              .timedout.userdata = &userdata,
                              .timedout.callback = dummy_timedout_callback},
        50000));

    testrun(ov_event_async_set(
        store,
        "2",
        (ov_event_async_data){.socket = 2,
                              .value = input2,
                              .timedout.userdata = &userdata,
                              .timedout.callback = dummy_timedout_callback},
        500000));

    testrun(0 == userdata.socket);
    testrun(NULL == userdata.value);

    testrun(loop->run(loop, 100000));

    testrun(1 == userdata.socket);
    testrun(input1 == userdata.value);
    input1 = ov_json_value_free(input1);

    testrun(loop->run(loop, 500000));
    testrun(2 == userdata.socket);
    testrun(input2 == userdata.value);
    input2 = ov_json_value_free(input2);

    testrun(NULL == ov_event_async_store_free(store));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_timing() {

    uint64_t start = 0;
    uint64_t end = 0;

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_event_async_store *store = ov_event_async_store_create(
        (ov_event_async_store_config){.loop = loop,
                                      .invalidate_check_interval_usec = 20000,
                                      .cache = 100000});

    testrun(store);
    testrun(store->config.invalidate_check_interval_usec == 20000);

    ov_event_async_data out = {0};

    ov_list *list =
        ov_list_create((ov_list_config){.item.free = ov_data_pointer_free});

    ov_id uuid = {0};
    char *str = NULL;

    /* NOTE this will overload the internal dict by intention to
     * check for long runtimes */

    size_t items = 100000;

    for (size_t i = 0; i < items; i++) {

        ov_id_fill_with_uuid(uuid);
        str = strdup(uuid);
        testrun(ov_list_push(list, str));
    }

    ov_json_value *input = ov_json_object();

    fprintf(stdout, "\nChecking with dict overload and %zu items\n", items);

    double average = 0;
    uint64_t max = 0;
    uint64_t runtime = 0;
    for (size_t i = 1; i < items; i++) {

        str = ov_list_get(list, i);

        start = ov_time_get_current_time_usecs();
        testrun(ov_event_async_set(
            store,
            str,
            (ov_event_async_data){.socket = i,
                                  .value = input,
                                  .timedout.userdata = &userdata,
                                  .timedout.callback = dummy_timedout_callback},
            500000));
        end = ov_time_get_current_time_usecs();
        // fprintf(stdout, "%zu set %"PRIu64" usec\n", i, end - start);
        runtime = end - start;
        average += runtime;
        if (runtime > max) max = runtime;
    }
    fprintf(stdout,
            "average set %f usec max %" PRIu64 " \n",
            (double)average / items,
            max);

    average = 0;
    max = 0;
    for (size_t i = 1; i < items; i++) {

        str = ov_list_get(list, i);

        start = ov_time_get_current_time_usecs();
        out = ov_event_async_unset(store, str);
        end = ov_time_get_current_time_usecs();
        testrun(out.value == input);
        testrun(out.socket == (int)i);
        testrun(out.timedout.userdata == &userdata);
        testrun(out.timedout.callback == dummy_timedout_callback);
        // fprintf(stdout, "%zu unset %"PRIu64" usec\n", i, end - start);

        runtime = end - start;
        average += runtime;
        if (runtime > max) max = runtime;
    }
    fprintf(stdout,
            "average unset %f usec max %" PRIu64 " \n",
            (double)average / items,
            max);

    // rerun with now cached internal structures

    average = 0;
    max = 0;
    for (size_t i = 1; i < items; i++) {

        str = ov_list_get(list, i);

        start = ov_time_get_current_time_usecs();
        testrun(ov_event_async_set(
            store,
            str,
            (ov_event_async_data){.socket = i,
                                  .value = input,
                                  .timedout.userdata = &userdata,
                                  .timedout.callback = dummy_timedout_callback},
            500000));
        end = ov_time_get_current_time_usecs();
        runtime = end - start;
        average += runtime;
        if (runtime > max) max = runtime;
    }
    fprintf(stdout,
            "average set cached %f usec max %" PRIu64 " \n",
            (double)average / items,
            max);

    average = 0;
    max = 0;
    for (size_t i = 1; i < items; i++) {

        str = ov_list_get(list, i);

        start = ov_time_get_current_time_usecs();
        out = ov_event_async_unset(store, str);
        end = ov_time_get_current_time_usecs();
        testrun(out.value == input);
        testrun(out.socket == (int)i);
        testrun(out.timedout.userdata == &userdata);
        testrun(out.timedout.callback == dummy_timedout_callback);

        runtime = end - start;
        average += runtime;
        if (runtime > max) max = runtime;
    }
    fprintf(stdout,
            "average unset %f usec max %" PRIu64 " \n",
            (double)average / items,
            max);

    items = 255;

    fprintf(stdout,
            "\nChecking with dict optimum max distribution "
            "and %zu items\n",
            items);

    average = 0;
    max = 0;
    for (size_t i = 1; i < items; i++) {

        str = ov_list_get(list, i);

        start = ov_time_get_current_time_usecs();
        testrun(ov_event_async_set(
            store,
            str,
            (ov_event_async_data){.socket = i,
                                  .value = input,
                                  .timedout.userdata = &userdata,
                                  .timedout.callback = dummy_timedout_callback},
            500000));
        end = ov_time_get_current_time_usecs();
        runtime = end - start;
        average += runtime;
        if (runtime > max) max = runtime;
    }
    fprintf(stdout,
            "average set %f usec max %" PRIu64 " \n",
            (double)average / items,
            max);

    average = 0;
    max = 0;
    for (size_t i = 1; i < items; i++) {

        str = ov_list_get(list, i);

        start = ov_time_get_current_time_usecs();
        out = ov_event_async_unset(store, str);
        end = ov_time_get_current_time_usecs();
        testrun(out.value == input);
        testrun(out.socket == (int)i);
        testrun(out.timedout.userdata == &userdata);
        testrun(out.timedout.callback == dummy_timedout_callback);

        runtime = end - start;
        average += runtime;
        if (runtime > max) max = runtime;
    }
    fprintf(stdout,
            "average unset %f usec max %" PRIu64 " \n",
            (double)average / items,
            max);

    items = 5000;
    fprintf(stdout, "\nChecking with %zu items\n", items);

    average = 0;
    max = 0;
    for (size_t i = 1; i < items; i++) {

        str = ov_list_get(list, i);

        start = ov_time_get_current_time_usecs();
        testrun(ov_event_async_set(
            store,
            str,
            (ov_event_async_data){.socket = i,
                                  .value = input,
                                  .timedout.userdata = &userdata,
                                  .timedout.callback = dummy_timedout_callback},
            500000));
        end = ov_time_get_current_time_usecs();
        runtime = end - start;
        average += runtime;
        if (runtime > max) max = runtime;
    }
    fprintf(stdout,
            "average set %f usec max %" PRIu64 " \n",
            (double)average / items,
            max);

    average = 0;
    max = 0;
    for (size_t i = 1; i < items; i++) {

        str = ov_list_get(list, i);

        start = ov_time_get_current_time_usecs();
        out = ov_event_async_unset(store, str);
        end = ov_time_get_current_time_usecs();
        testrun(out.value == input);
        testrun(out.socket == (int)i);
        testrun(out.timedout.userdata == &userdata);
        testrun(out.timedout.callback == dummy_timedout_callback);

        runtime = end - start;
        average += runtime;
        if (runtime > max) max = runtime;
    }
    fprintf(stdout,
            "average unset %f usec max %" PRIu64 " \n",
            (double)average / items,
            max);

    list = ov_list_free(list);
    input = ov_json_value_free(input);
    testrun(NULL == ov_event_async_store_free(store));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_async_drop() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_event_async_store *store = ov_event_async_store_create(
        (ov_event_async_store_config){.loop = loop,
                                      .invalidate_check_interval_usec = 20000,
                                      .cache = 100000});

    testrun(store);
    testrun(store->config.invalidate_check_interval_usec == 20000);

    testrun(ov_event_async_drop(store, 1));

    ov_json_value *val1 = ov_json_object();
    ov_json_value *val2 = ov_json_object();
    ov_json_value *val3 = ov_json_object();

    ov_event_async_data data1 =
        (ov_event_async_data){.socket = 1, .value = val1};

    ov_event_async_data data2 =
        (ov_event_async_data){.socket = 2, .value = val2};

    ov_event_async_data data3 =
        (ov_event_async_data){.socket = 2, .value = val3};

    testrun(ov_event_async_set(store, "1", data1, 0));
    testrun(ov_event_async_set(store, "2", data2, 0));
    testrun(ov_event_async_set(store, "3", data3, 0));

    testrun(3 == ov_dict_count(store->dict));
    testrun(ov_event_async_drop(store, 1));
    testrun(2 == ov_dict_count(store->dict));

    testrun(ov_event_async_drop(store, 2));
    testrun(0 == ov_dict_count(store->dict));

    testrun(NULL == ov_event_async_store_free(store));
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
    testrun_test(test_ov_event_async_store_create);
    testrun_test(test_ov_event_async_store_free);
    testrun_test(test_ov_event_async_set);
    testrun_test(test_ov_event_async_unset);

    testrun_test(test_ov_event_async_drop);

    testrun_test(check_ov_event_async_invalidation);
    testrun_test(check_timing);

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
