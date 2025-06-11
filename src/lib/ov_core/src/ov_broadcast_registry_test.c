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
        @file           ov_named_broadcast_test.c
        @author         Markus Töpfer

        @date           2021-02-24


        ------------------------------------------------------------------------
*/
#include "ov_broadcast_registry.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_broadcast_registry_create() {

    ov_broadcast_registry *reg =
        ov_broadcast_registry_create((ov_event_broadcast_config){0});
    testrun(reg);
    testrun(reg->dict);
    testrun(reg->config.lock_timeout_usec == IMPL_THREAD_LOCK_TIMEOUT_USEC);
    testrun(NULL == ov_broadcast_registry_free(reg));

    reg = ov_broadcast_registry_create(
        (ov_event_broadcast_config){.lock_timeout_usec = 1, .max_sockets = 1});
    testrun(reg);
    testrun(reg->dict);
    testrun(reg->config.lock_timeout_usec == 1);
    testrun(reg->config.max_sockets == 1);
    testrun(NULL == ov_broadcast_registry_free(reg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_broadcast_registry_free() {

    ov_broadcast_registry *reg =
        ov_broadcast_registry_create((ov_event_broadcast_config){0});
    testrun(reg);
    testrun(NULL == ov_broadcast_registry_free(NULL));
    testrun(NULL == ov_broadcast_registry_free(reg));

    // free with entries
    reg = ov_broadcast_registry_create((ov_event_broadcast_config){0});

    testrun(ov_broadcast_registry_set(reg, "1", 1, OV_BROADCAST));
    testrun(ov_broadcast_registry_set(reg, "1", 2, OV_BROADCAST));
    testrun(ov_broadcast_registry_set(reg, "1", 3, OV_BROADCAST));
    testrun(ov_broadcast_registry_set(reg, "2", 1, OV_BROADCAST));
    testrun(ov_broadcast_registry_set(reg, "3", 1, OV_BROADCAST));

    testrun(NULL == ov_broadcast_registry_free(reg));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_broadcast_registry_set() {

    ov_broadcast_registry *reg =
        ov_broadcast_registry_create((ov_event_broadcast_config){0});
    testrun(reg);
    testrun(!ov_broadcast_registry_set(NULL, NULL, 0, 0));
    testrun(!ov_broadcast_registry_set(reg, NULL, 0, 0));
    testrun(!ov_broadcast_registry_set(NULL, "1", 0, 0));

    // min valid
    testrun(ov_broadcast_registry_set(reg, "1", 0, OV_BROADCAST));
    testrun(1 == ov_dict_count(reg->dict));

    testrun(ov_broadcast_registry_set(reg, "1", 1, OV_BROADCAST));
    testrun(1 == ov_dict_count(reg->dict));

    testrun(ov_broadcast_registry_set(reg, "1", 2, OV_BROADCAST));
    testrun(1 == ov_dict_count(reg->dict));

    testrun(ov_broadcast_registry_set(reg, "2", 1, OV_BROADCAST));
    testrun(2 == ov_dict_count(reg->dict));

    testrun(ov_broadcast_registry_set(reg, "2", 2, OV_BROADCAST));
    testrun(2 == ov_dict_count(reg->dict));

    // unset
    testrun(ov_broadcast_registry_set(reg, "1", 1, 0));
    testrun(2 == ov_dict_count(reg->dict));
    testrun(ov_broadcast_registry_set(reg, "1", 2, 0));
    testrun(2 == ov_dict_count(reg->dict));
    testrun(ov_broadcast_registry_set(reg, "1", 0, 0));
    testrun(1 == ov_dict_count(reg->dict));

    testrun(ov_broadcast_registry_set(reg, "2", 1, 0));
    testrun(1 == ov_dict_count(reg->dict));
    testrun(ov_broadcast_registry_set(reg, "2", 2, 0));
    testrun(0 == ov_dict_count(reg->dict));

    /* locked state */
    testrun(ov_thread_lock_try_lock(&reg->lock));
    testrun(0 == ov_dict_count(reg->dict));
    testrun(!ov_broadcast_registry_set(reg, "2", 2, OV_BROADCAST));
    testrun(0 == ov_dict_count(reg->dict));

    testrun(ov_thread_lock_unlock(&reg->lock));
    testrun(0 == ov_dict_count(reg->dict));
    testrun(ov_broadcast_registry_set(reg, "2", 2, OV_BROADCAST));
    testrun(1 == ov_dict_count(reg->dict));

    testrun(NULL == ov_broadcast_registry_free(reg));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_broadcast_registry_get() {

    ov_broadcast_registry *reg =
        ov_broadcast_registry_create((ov_event_broadcast_config){0});
    testrun(reg);

    uint8_t result = 0;
    testrun(!ov_broadcast_registry_get(NULL, NULL, 0, NULL));
    testrun(!ov_broadcast_registry_get(reg, NULL, 0, &result));
    testrun(!ov_broadcast_registry_get(NULL, "1", 0, &result));
    testrun(!ov_broadcast_registry_get(reg, "1", 0, NULL));

    testrun(ov_broadcast_registry_get(reg, "1", 0, &result));
    testrun(result == OV_BROADCAST_UNSET);

    testrun(ov_broadcast_registry_set(reg, "1", 0, OV_BROADCAST));
    testrun(ov_broadcast_registry_get(reg, "1", 0, &result));
    testrun(result == OV_BROADCAST);

    testrun(ov_broadcast_registry_set(reg, "1", 1, OV_BROADCAST));
    testrun(ov_broadcast_registry_get(reg, "1", 0, &result));
    testrun(result == OV_BROADCAST);
    testrun(ov_broadcast_registry_get(reg, "1", 1, &result));
    testrun(result == OV_BROADCAST);

    testrun(ov_broadcast_registry_set(reg, "2", 1, 12));
    testrun(ov_broadcast_registry_get(reg, "1", 0, &result));
    testrun(result == OV_BROADCAST);
    testrun(ov_broadcast_registry_get(reg, "1", 1, &result));
    testrun(result == OV_BROADCAST);
    testrun(ov_broadcast_registry_get(reg, "2", 1, &result));
    testrun(result == 12);

    /* locked state */
    testrun(ov_thread_lock_try_lock(&reg->lock));
    testrun(!ov_broadcast_registry_get(reg, "1", 0, &result));
    testrun(result == OV_BROADCAST_UNSET);

    testrun(ov_thread_lock_unlock(&reg->lock));
    testrun(ov_broadcast_registry_get(reg, "1", 1, &result));
    testrun(result == OV_BROADCAST);

    testrun(NULL == ov_broadcast_registry_free(reg));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool dummy_send(void *instance, int socket, const ov_json_value *val) {

    if (!instance) return false;

    ov_dict *dict = ov_dict_cast(instance);
    intptr_t key = socket;
    return ov_dict_set(dict, (void *)key, (void *)val, NULL);
}

/*----------------------------------------------------------------------------*/

int test_ov_broadcast_registry_send() {

    ov_dict *dict = ov_dict_create(ov_dict_intptr_key_config(255));

    ov_event_parameter p = {

        .send.instance = dict, .send.send = dummy_send};

    ov_broadcast_registry *reg =
        ov_broadcast_registry_create((ov_event_broadcast_config){0});
    testrun(reg);

    ov_json_value *msg = ov_json_object();

    testrun(ov_broadcast_registry_set(reg, "1", 1, OV_BROADCAST));

    testrun(!ov_broadcast_registry_send(NULL, NULL, NULL, NULL, 0));
    testrun(!ov_broadcast_registry_send(NULL, "1", &p, msg, OV_BROADCAST));
    testrun(!ov_broadcast_registry_send(reg, NULL, &p, msg, OV_BROADCAST));
    testrun(!ov_broadcast_registry_send(reg, "1", NULL, msg, OV_BROADCAST));
    testrun(!ov_broadcast_registry_send(reg, "1", &p, NULL, OV_BROADCAST));
    testrun(!ov_broadcast_registry_send(reg, "1", &p, msg, 0));
    testrun(!ov_broadcast_registry_send(reg, "2", &p, msg, OV_BROADCAST));

    testrun(0 == ov_dict_count(dict));
    testrun(ov_broadcast_registry_send(reg, "1", &p, msg, OV_BROADCAST));
    testrun(1 == ov_dict_count(dict));
    // got some msg at socket 1
    testrun(ov_dict_get(dict, (void *)1));
    testrun(ov_dict_clear(dict));

    // will send but not receice
    testrun(
        ov_broadcast_registry_send(reg, "1", &p, msg, OV_PROJECT_BROADCAST));
    testrun(0 == ov_dict_count(dict));

    testrun(ov_broadcast_registry_set(
        reg, "1", 1, OV_BROADCAST | OV_PROJECT_BROADCAST));
    testrun(
        ov_broadcast_registry_send(reg, "1", &p, msg, OV_PROJECT_BROADCAST));
    testrun(1 == ov_dict_count(dict));
    // got some msg at socket 1
    testrun(ov_dict_get(dict, (void *)1));
    testrun(ov_dict_clear(dict));
    testrun(ov_broadcast_registry_send(reg, "1", &p, msg, OV_BROADCAST));
    testrun(1 == ov_dict_count(dict));
    // got some msg at socket 1
    testrun(ov_dict_get(dict, (void *)1));
    testrun(ov_dict_clear(dict));

    testrun(ov_broadcast_registry_set(reg, "1", 1, OV_USER_BROADCAST));
    testrun(ov_broadcast_registry_set(reg, "2", 1, OV_USER_BROADCAST));
    testrun(ov_broadcast_registry_set(reg, "1", 2, OV_USER_BROADCAST));
    testrun(ov_broadcast_registry_set(reg, "1", 3, OV_USER_BROADCAST));
    testrun(ov_broadcast_registry_set(reg, "1", 4, OV_USER_BROADCAST));

    testrun(ov_broadcast_registry_send(reg, "2", &p, msg, OV_USER_BROADCAST));
    testrun(1 == ov_dict_count(dict));
    // got some msg at socket 1
    testrun(ov_dict_get(dict, (void *)1));
    testrun(ov_dict_clear(dict));

    testrun(ov_broadcast_registry_send(reg, "1", &p, msg, OV_USER_BROADCAST));
    testrun(4 == ov_dict_count(dict));
    testrun(ov_dict_get(dict, (void *)1));
    testrun(ov_dict_get(dict, (void *)2));
    testrun(ov_dict_get(dict, (void *)3));
    testrun(ov_dict_get(dict, (void *)4));
    testrun(ov_dict_clear(dict));

    // send different type of broadcast
    testrun(ov_broadcast_registry_set(reg, "1", 1, OV_ROLE_BROADCAST));
    testrun(ov_broadcast_registry_set(reg, "1", 2, OV_ROLE_BROADCAST));

    testrun(ov_broadcast_registry_send(reg, "1", &p, msg, OV_ROLE_BROADCAST));
    testrun(2 == ov_dict_count(dict));
    testrun(ov_dict_get(dict, (void *)1));
    testrun(ov_dict_get(dict, (void *)2));
    testrun(ov_dict_clear(dict));

    /* locked state */
    testrun(ov_thread_lock_try_lock(&reg->lock));
    testrun(!ov_broadcast_registry_send(reg, "1", &p, msg, OV_ROLE_BROADCAST));
    testrun(0 == ov_dict_count(dict));

    testrun(ov_thread_lock_unlock(&reg->lock));
    testrun(ov_broadcast_registry_send(reg, "1", &p, msg, OV_ROLE_BROADCAST));
    testrun(2 == ov_dict_count(dict));
    testrun(ov_dict_get(dict, (void *)1));
    testrun(ov_dict_get(dict, (void *)2));
    testrun(ov_dict_clear(dict));

    testrun(NULL == ov_dict_free(dict));
    testrun(NULL == ov_json_value_free(msg));
    testrun(NULL == ov_broadcast_registry_free(reg));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_broadcast_registry_unset() {

    ov_broadcast_registry *reg =
        ov_broadcast_registry_create((ov_event_broadcast_config){0});
    testrun(reg);

    testrun(!ov_broadcast_registry_unset(NULL, -1));
    testrun(!ov_broadcast_registry_unset(reg, -1));
    testrun(!ov_broadcast_registry_unset(NULL, 0));

    testrun(ov_broadcast_registry_unset(reg, 0));

    testrun(ov_broadcast_registry_set(reg, "1", 1, OV_ROLE_BROADCAST));
    testrun(ov_broadcast_registry_set(reg, "1", 2, OV_ROLE_BROADCAST));
    testrun(1 == ov_dict_count(reg->dict));

    testrun(ov_broadcast_registry_unset(reg, 1));
    testrun(1 == ov_dict_count(reg->dict));

    testrun(ov_broadcast_registry_unset(reg, 2));
    testrun(0 == ov_dict_count(reg->dict));

    /* locked state */
    testrun(ov_broadcast_registry_set(reg, "1", 1, OV_ROLE_BROADCAST));
    testrun(1 == ov_dict_count(reg->dict));

    testrun(ov_thread_lock_try_lock(&reg->lock));
    testrun(!ov_broadcast_registry_unset(reg, 1));
    testrun(1 == ov_dict_count(reg->dict));

    testrun(ov_thread_lock_unlock(&reg->lock));
    testrun(ov_broadcast_registry_unset(reg, 1));
    testrun(0 == ov_dict_count(reg->dict));

    testrun(NULL == ov_broadcast_registry_free(reg));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_broadcast_registry_state() {

    ov_broadcast_registry *reg =
        ov_broadcast_registry_create((ov_event_broadcast_config){0});
    testrun(reg);

    ov_json_value *val = NULL;
    ov_json_value *out = NULL;

    testrun(NULL == ov_broadcast_registry_state(NULL));

    out = ov_broadcast_registry_state(reg);
    testrun(out);
    testrun(0 == ov_json_object_count(out));
    out = ov_json_value_free(out);

    testrun(ov_broadcast_registry_set(reg, "1", 1, OV_ROLE_BROADCAST));
    out = ov_broadcast_registry_state(reg);
    testrun(out);
    testrun(1 == ov_json_object_count(out));
    val = ov_json_object_get(out, "1");
    testrun(1 == ov_json_object_count(val));
    testrun(ov_json_object_get(val, "1"));
    out = ov_json_value_free(out);

    testrun(ov_broadcast_registry_set(reg, "1", 2, OV_ROLE_BROADCAST));
    out = ov_broadcast_registry_state(reg);
    testrun(out);
    testrun(1 == ov_json_object_count(out));
    val = ov_json_object_get(out, "1");
    testrun(2 == ov_json_object_count(val));
    testrun(ov_json_object_get(val, "1"));
    testrun(ov_json_object_get(val, "2"));
    out = ov_json_value_free(out);

    testrun(ov_broadcast_registry_set(reg, "2", 2, OV_ROLE_BROADCAST));
    out = ov_broadcast_registry_state(reg);
    testrun(out);
    testrun(2 == ov_json_object_count(out));
    val = ov_json_object_get(out, "1");
    testrun(2 == ov_json_object_count(val));
    testrun(ov_json_object_get(val, "1"));
    testrun(ov_json_object_get(val, "2"));
    val = ov_json_object_get(out, "2");
    testrun(1 == ov_json_object_count(val));
    testrun(ov_json_object_get(val, "2"));
    out = ov_json_value_free(out);

    // broadcast type change does ot change output
    testrun(ov_broadcast_registry_set(
        reg, "2", 2, OV_ROLE_BROADCAST | OV_BROADCAST));
    out = ov_broadcast_registry_state(reg);
    testrun(out);
    testrun(2 == ov_json_object_count(out));
    val = ov_json_object_get(out, "1");
    testrun(2 == ov_json_object_count(val));
    testrun(ov_json_object_get(val, "1"));
    testrun(ov_json_object_get(val, "2"));
    val = ov_json_object_get(out, "2");
    testrun(1 == ov_json_object_count(val));
    testrun(ov_json_object_get(val, "2"));

    char *str = ov_json_value_to_string(out);
    fprintf(stdout, "%s\n", str);
    str = ov_data_pointer_free(str);
    out = ov_json_value_free(out);

    /* locked state */
    testrun(ov_thread_lock_try_lock(&reg->lock));
    testrun(NULL == ov_broadcast_registry_state(reg));

    testrun(ov_thread_lock_unlock(&reg->lock));
    out = ov_broadcast_registry_state(reg);
    testrun(out);
    out = ov_json_value_free(out);

    testrun(NULL == ov_broadcast_registry_free(reg));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_broadcast_registry_named_state() {

    ov_broadcast_registry *reg =
        ov_broadcast_registry_create((ov_event_broadcast_config){0});
    testrun(reg);

    ov_json_value *val = NULL;
    ov_json_value *out = NULL;

    testrun(NULL == ov_broadcast_registry_named_state(NULL, NULL));
    testrun(NULL == ov_broadcast_registry_named_state(reg, NULL));
    testrun(NULL == ov_broadcast_registry_named_state(NULL, "1"));

    out = ov_broadcast_registry_named_state(reg, "1");
    testrun(!out);

    testrun(ov_broadcast_registry_set(reg, "1", 1, OV_ROLE_BROADCAST));
    out = ov_broadcast_registry_named_state(reg, "1");
    testrun(out);
    testrun(1 == ov_json_object_count(out));
    val = ov_json_object_get(out, "1");
    testrun(1 == ov_json_object_count(val));
    testrun(ov_json_object_get(val, "1"));
    out = ov_json_value_free(out);

    testrun(ov_broadcast_registry_set(reg, "1", 2, OV_ROLE_BROADCAST));
    out = ov_broadcast_registry_named_state(reg, "1");
    testrun(out);
    testrun(1 == ov_json_object_count(out));
    val = ov_json_object_get(out, "1");
    testrun(2 == ov_json_object_count(val));
    testrun(ov_json_object_get(val, "1"));
    testrun(ov_json_object_get(val, "2"));
    out = ov_json_value_free(out);

    testrun(ov_broadcast_registry_set(reg, "2", 2, OV_ROLE_BROADCAST));
    out = ov_broadcast_registry_named_state(reg, "1");
    testrun(out);
    testrun(1 == ov_json_object_count(out));
    val = ov_json_object_get(out, "1");
    testrun(2 == ov_json_object_count(val));
    testrun(ov_json_object_get(val, "1"));
    testrun(ov_json_object_get(val, "2"));
    out = ov_json_value_free(out);

    out = ov_broadcast_registry_named_state(reg, "2");
    testrun(out);
    testrun(1 == ov_json_object_count(out));
    val = ov_json_object_get(out, "2");
    testrun(1 == ov_json_object_count(val));
    testrun(ov_json_object_get(val, "2"));
    out = ov_json_value_free(out);

    /* locked state */
    testrun(ov_thread_lock_try_lock(&reg->lock));
    testrun(NULL == ov_broadcast_registry_named_state(reg, "2"));

    testrun(ov_thread_lock_unlock(&reg->lock));
    out = ov_broadcast_registry_named_state(reg, "2");
    testrun(out);
    out = ov_json_value_free(out);

    testrun(NULL == ov_broadcast_registry_free(reg));
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
    testrun_test(test_ov_broadcast_registry_create);
    testrun_test(test_ov_broadcast_registry_free);
    testrun_test(test_ov_broadcast_registry_set);
    testrun_test(test_ov_broadcast_registry_get);
    testrun_test(test_ov_broadcast_registry_send);
    testrun_test(test_ov_broadcast_registry_unset);
    testrun_test(test_ov_broadcast_registry_state);
    testrun_test(test_ov_broadcast_registry_named_state);

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
