/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_stun_transaction_store_test.c
        @author         Markus

        @date           2024-04-24


        ------------------------------------------------------------------------
*/
#include "ov_stun_transaction_store.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_stun_transaction_store_create() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_stun_transaction_store_config config = {0};

    testrun(!ov_stun_transaction_store_create(config));

    config.loop = loop;

    ov_stun_transaction_store *store = ov_stun_transaction_store_create(config);
    testrun(store);
    testrun(ov_stun_transaction_store_cast(store));
    testrun(store->dict);
    testrun(OV_TIMER_INVALID != store->timer_invalidate);

    testrun(NULL == ov_stun_transaction_store_free(store));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_transaction_store_free() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_stun_transaction_store_config config = {0};
    config.loop = loop;

    ov_stun_transaction_store *store = ov_stun_transaction_store_create(config);
    testrun(store);
    testrun(loop == (ov_event_loop *)ov_stun_transaction_store_free(
                        (ov_stun_transaction_store *)loop));
    testrun(NULL == ov_stun_transaction_store_free(store));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_transaction_store_create_transaction() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_stun_transaction_store_config config = {0};
    config.loop = loop;

    ov_stun_transaction_store *store = ov_stun_transaction_store_create(config);
    testrun(store);

    char *data = "testdata";
    uint8_t buffer[100] = {0};
    uint8_t *ptr = buffer;

    testrun(ov_stun_transaction_store_create_transaction(store, ptr, data));
    testrun(1 == ov_dict_count(store->dict));
    testrun(data == ov_stun_transaction_store_unset(store, ptr));

    for (size_t i = 0; i < 20; i++) {

        if (i < 12) {
            testrun(0 != buffer[i]);
        } else {
            testrun(0 == buffer[i]);
        }
    }

    testrun(NULL == ov_stun_transaction_store_free(store));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_transaction_store_unset() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_stun_transaction_store_config config = {0};
    config.loop = loop;

    ov_stun_transaction_store *store = ov_stun_transaction_store_create(config);
    testrun(store);

    char *data = "testdata";
    uint8_t buffer[100] = {0};
    uint8_t *ptr = buffer;

    testrun(ov_stun_transaction_store_create_transaction(store, ptr, data));
    testrun(1 == ov_dict_count(store->dict));
    testrun(data == ov_stun_transaction_store_unset(store, ptr));

    for (size_t i = 0; i < 20; i++) {

        if (i < 12) {
            testrun(0 != buffer[i]);
        } else {
            testrun(0 == buffer[i]);
        }
    }

    testrun(NULL == ov_stun_transaction_store_free(store));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

    testrun_init();
    testrun_test(test_ov_stun_transaction_store_create);
    testrun_test(test_ov_stun_transaction_store_free);

    testrun_test(test_ov_stun_transaction_store_create_transaction);
    testrun_test(test_ov_stun_transaction_store_unset);

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
