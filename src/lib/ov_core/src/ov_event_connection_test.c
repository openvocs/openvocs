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
        @file           ov_event_connection_test.c
        @author         Markus

        @date           2024-01-09


        ------------------------------------------------------------------------
*/
#include "ov_event_connection.c"
#include <ov_test/testrun.h>

struct userdata {

    int socket;
    ov_json_value *value;
};

/*----------------------------------------------------------------------------*/

static void dummy_clear(struct userdata *dummy) {

    dummy->socket = 0;
    dummy->value = ov_json_value_free(dummy->value);
}

/*----------------------------------------------------------------------------*/

static bool dummy_send(void *instance, int socket, const ov_json_value *v) {

    struct userdata *dummy = (struct userdata *)instance;
    dummy_clear(dummy);
    dummy->socket = socket;
    ov_json_value_copy((void **)&dummy->value, v);
    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_event_connection_create() {

    struct userdata dummy = {0};

    ov_event_connection *c =
        ov_event_connection_create((ov_event_connection_config){0});
    testrun(c);
    testrun(ov_event_connection_cast(c));
    testrun(NULL == ov_event_connection_free(c));

    c = ov_event_connection_create(
        (ov_event_connection_config){.socket = 1,
                                     .params.send.instance = &dummy,
                                     .params.send.send = dummy_send});

    testrun(c);
    testrun(c->config.socket == 1);
    testrun(c->config.params.send.instance == &dummy);
    testrun(c->config.params.send.send == dummy_send);
    testrun(NULL == ov_event_connection_free(c));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_connection_free() {

    struct userdata dummy = {0};

    ov_event_connection *c = NULL;

    c = ov_event_connection_create(
        (ov_event_connection_config){.socket = 1,
                                     .params.send.instance = &dummy,
                                     .params.send.send = dummy_send});

    testrun(c);
    testrun(NULL == ov_event_connection_free(c));

    c = ov_event_connection_create(
        (ov_event_connection_config){.socket = 1,
                                     .params.send.instance = &dummy,
                                     .params.send.send = dummy_send});

    testrun(c);
    testrun(ov_event_connection_set(c, "key", "value"));
    testrun(NULL == ov_event_connection_free(c));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_connection_free_void() {

    struct userdata dummy = {0};

    ov_event_connection *c = NULL;

    c = ov_event_connection_create(
        (ov_event_connection_config){.socket = 1,
                                     .params.send.instance = &dummy,
                                     .params.send.send = dummy_send});

    testrun(c);
    testrun(NULL == ov_event_connection_free_void(c));
    testrun(&dummy == ov_event_connection_free_void(&dummy));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_connection_send() {

    struct userdata dummy = {0};

    ov_event_connection *c = NULL;

    c = ov_event_connection_create(
        (ov_event_connection_config){.socket = 1,
                                     .params.send.instance = &dummy,
                                     .params.send.send = dummy_send});

    testrun(c);
    ov_json_value *val = ov_json_string("test");

    testrun(ov_event_connection_send(c, val));
    testrun(dummy.value);
    testrun(ov_json_is_string(dummy.value));
    testrun(0 == strcmp("test", ov_json_string_get(dummy.value)));

    dummy_clear(&dummy);
    testrun(NULL == ov_event_connection_free(c));
    val = ov_json_value_free(val);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_connection_set() {

    ov_event_connection *c =
        ov_event_connection_create((ov_event_connection_config){0});

    testrun(c);

    testrun(!ov_event_connection_set(NULL, NULL, NULL));
    testrun(!ov_event_connection_set(c, "key", NULL));
    testrun(!ov_event_connection_set(NULL, "key", "val"));
    testrun(!ov_event_connection_set(c, NULL, "val"));

    testrun(ov_event_connection_set(c, "key", "val"));
    testrun(
        0 ==
        strcmp("val", ov_json_string_get(ov_json_object_get(c->data, "key"))));

    testrun(ov_event_connection_set(c, "key", "other"));
    testrun(0 ==
            strcmp("other",
                   ov_json_string_get(ov_json_object_get(c->data, "key"))));

    testrun(1 == ov_json_object_count(c->data));

    testrun(NULL == ov_event_connection_free(c));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_connection_get() {

    ov_event_connection *c =
        ov_event_connection_create((ov_event_connection_config){0});

    testrun(c);

    testrun(ov_event_connection_set(c, "key", "val"));
    testrun(0 == strcmp("val", ov_event_connection_get(c, "key")));

    testrun(ov_event_connection_set(c, "key", "other"));
    testrun(0 == strcmp("other", ov_event_connection_get(c, "key")));

    testrun(1 == ov_json_object_count(c->data));

    testrun(NULL == ov_event_connection_free(c));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_connection_set_json() {

    ov_event_connection *c =
        ov_event_connection_create((ov_event_connection_config){0});

    testrun(c);

    ov_json_value *val = ov_json_string("test");

    testrun(!ov_event_connection_set_json(NULL, NULL, NULL));
    testrun(!ov_event_connection_set_json(c, "key", NULL));
    testrun(!ov_event_connection_set_json(NULL, "key", val));
    testrun(!ov_event_connection_set_json(c, NULL, val));

    testrun(ov_event_connection_set_json(c, "key", val));
    testrun(0 == strcmp("test", ov_event_connection_get(c, "key")));

    testrun(ov_event_connection_set(c, "key", "other"));
    testrun(0 == strcmp("other", ov_event_connection_get(c, "key")));

    testrun(1 == ov_json_object_count(c->data));

    testrun(NULL == ov_event_connection_free(c));
    val = ov_json_value_free(val);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_connection_get_json() {

    ov_event_connection *c =
        ov_event_connection_create((ov_event_connection_config){0});

    testrun(c);

    ov_json_value *val = ov_json_string("test");

    testrun(ov_event_connection_set_json(c, "key", val));
    testrun(0 == strcmp("test", ov_event_connection_get(c, "key")));
    testrun(0 ==
            strcmp("test",
                   ov_json_string_get(ov_event_connection_get_json(c, "key"))));

    testrun(NULL == ov_event_connection_free(c));
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
    testrun_test(test_ov_event_connection_create);
    testrun_test(test_ov_event_connection_free);
    testrun_test(test_ov_event_connection_free_void);

    testrun_test(test_ov_event_connection_send);

    testrun_test(test_ov_event_connection_set);
    testrun_test(test_ov_event_connection_get);

    testrun_test(test_ov_event_connection_set_json);
    testrun_test(test_ov_event_connection_get_json);

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
