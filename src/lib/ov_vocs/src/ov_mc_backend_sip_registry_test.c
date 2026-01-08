/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_mc_backend_sip_registry_test.c
        @author         Markus Töpfer

        @date           2023-03-31


        ------------------------------------------------------------------------
*/
#include "ov_mc_backend_sip_registry.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_mc_backend_sip_registry_create() {

    ov_mc_backend_sip_registry *reg = ov_mc_backend_sip_registry_create(
        (ov_mc_backend_sip_registry_config){0});

    testrun(reg);
    testrun(ov_mc_backend_sip_registry_cast(reg));
    testrun(reg->proxys);
    testrun(reg->calls);

    testrun(NULL == ov_mc_backend_sip_registry_free(reg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_sip_registry_cast() {

    ov_mc_backend_sip_registry *reg = ov_mc_backend_sip_registry_create(
        (ov_mc_backend_sip_registry_config){0});

    testrun(reg);
    testrun(ov_mc_backend_sip_registry_cast(reg));
    testrun(reg->magic_bytes == OV_MC_BACKEND_SIP_REGISTRY_MAGIC_BYTES);
    reg->magic_bytes++;
    testrun(!ov_mc_backend_sip_registry_cast(reg));

    reg->magic_bytes = OV_MC_BACKEND_SIP_REGISTRY_MAGIC_BYTES;
    testrun(NULL == ov_mc_backend_sip_registry_free(reg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_sip_registry_free() {

    ov_mc_backend_sip_registry *reg = ov_mc_backend_sip_registry_create(
        (ov_mc_backend_sip_registry_config){0});

    testrun(reg);
    testrun(NULL == ov_mc_backend_sip_registry_free(reg));

    reg = ov_mc_backend_sip_registry_create(
        (ov_mc_backend_sip_registry_config){0});

    testrun(ov_mc_backend_sip_registry_register_proxy(reg, 1, "1-2-3-4"));
    testrun(ov_mc_backend_sip_registry_register_proxy(reg, 2, "2-2-3-4"));
    testrun(ov_mc_backend_sip_registry_register_proxy(reg, 3, "3-2-3-4"));

    testrun(ov_mc_backend_sip_registry_register_call(reg, 1, "1", "1", "1"));
    testrun(ov_mc_backend_sip_registry_register_call(reg, 2, "2", "1", "1"));
    testrun(ov_mc_backend_sip_registry_register_call(reg, 3, "3", "1", "1"));

    testrun(NULL == ov_mc_backend_sip_registry_free(reg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_sip_registry_register_proxy() {

    ov_mc_backend_sip_registry *reg = ov_mc_backend_sip_registry_create(
        (ov_mc_backend_sip_registry_config){0});

    testrun(reg);

    testrun(!ov_mc_backend_sip_registry_register_proxy(NULL, 1, "1-2-3-4"));
    testrun(!ov_mc_backend_sip_registry_register_proxy(reg, 0, "2-2-3-4"));

    testrun(ov_mc_backend_sip_registry_register_proxy(reg, 3, NULL));
    testrun(1 == ov_dict_count(reg->proxys));
    Proxy *proxy = ov_dict_get(reg->proxys, (void *)(intptr_t)3);
    testrun(proxy);
    testrun(proxy->socket == 3);
    testrun(0 == proxy->uuid[0]);

    testrun(ov_mc_backend_sip_registry_register_proxy(reg, 1, "1-2-3-4"));
    testrun(2 == ov_dict_count(reg->proxys));
    proxy = ov_dict_get(reg->proxys, (void *)(intptr_t)1);
    testrun(proxy);
    testrun(proxy->socket == 1);
    testrun(0 != proxy->uuid[0]);
    testrun(0 == strcmp(proxy->uuid, "1-2-3-4"));

    testrun(NULL == ov_mc_backend_sip_registry_free(reg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_sip_registry_unregister_proxy() {

    ov_mc_backend_sip_registry *reg = ov_mc_backend_sip_registry_create(
        (ov_mc_backend_sip_registry_config){0});

    testrun(reg);

    testrun(ov_mc_backend_sip_registry_register_proxy(reg, 1, NULL));
    testrun(1 == ov_dict_count(reg->proxys));

    testrun(!ov_mc_backend_sip_registry_unregister_proxy(NULL, 0));
    testrun(!ov_mc_backend_sip_registry_unregister_proxy(reg, 0));
    testrun(!ov_mc_backend_sip_registry_unregister_proxy(NULL, 1));

    testrun(ov_mc_backend_sip_registry_unregister_proxy(reg, 1));
    testrun(0 == ov_dict_count(reg->proxys));

    testrun(NULL == ov_mc_backend_sip_registry_free(reg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_sip_registry_get_proxy_socket() {

    ov_mc_backend_sip_registry *reg = ov_mc_backend_sip_registry_create(
        (ov_mc_backend_sip_registry_config){0});

    testrun(reg);

    testrun(ov_mc_backend_sip_registry_register_proxy(reg, 1, NULL));
    testrun(1 == ov_dict_count(reg->proxys));
    testrun(1 == ov_mc_backend_sip_registry_get_proxy_socket(reg));

    testrun(NULL == ov_mc_backend_sip_registry_free(reg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_sip_registry_register_call() {

    ov_mc_backend_sip_registry *reg = ov_mc_backend_sip_registry_create(
        (ov_mc_backend_sip_registry_config){0});

    testrun(reg);

    testrun(
        !ov_mc_backend_sip_registry_register_call(NULL, 0, NULL, NULL, NULL));
    testrun(
        !ov_mc_backend_sip_registry_register_call(reg, 0, NULL, NULL, NULL));
    testrun(
        !ov_mc_backend_sip_registry_register_call(NULL, 0, "1", NULL, NULL));

    testrun(ov_mc_backend_sip_registry_register_call(reg, 0, "1", NULL, NULL));

    testrun(1 == ov_dict_count(reg->calls));
    Call *call = ov_dict_get(reg->calls, "1");
    testrun(call);
    testrun(0 == call->socket);
    testrun(0 == strcmp(call->call_id, "1"));
    testrun(NULL == call->peer_id);
    testrun(NULL == call->loop_id);

    testrun(ov_mc_backend_sip_registry_register_call(reg, 1, "2", "3", "4"));

    testrun(2 == ov_dict_count(reg->calls));
    call = ov_dict_get(reg->calls, "2");
    testrun(call);
    testrun(1 == call->socket);
    testrun(0 == strcmp(call->call_id, "2"));
    testrun(0 == strcmp(call->peer_id, "4"));
    testrun(0 == strcmp(call->loop_id, "3"));

    testrun(NULL == ov_mc_backend_sip_registry_free(reg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_sip_registry_unregister_call() {

    ov_mc_backend_sip_registry *reg = ov_mc_backend_sip_registry_create(
        (ov_mc_backend_sip_registry_config){0});

    testrun(reg);

    testrun(ov_mc_backend_sip_registry_register_call(reg, 0, "1", NULL, NULL));
    testrun(1 == ov_dict_count(reg->calls));
    testrun(ov_mc_backend_sip_registry_unregister_call(reg, "1"));
    testrun(0 == ov_dict_count(reg->calls));

    testrun(ov_mc_backend_sip_registry_register_call(reg, 0, "1", NULL, NULL));
    testrun(1 == ov_dict_count(reg->calls));
    testrun(ov_mc_backend_sip_registry_register_call(reg, 0, "2", NULL, NULL));
    testrun(2 == ov_dict_count(reg->calls));
    testrun(ov_mc_backend_sip_registry_register_call(reg, 0, "3", NULL, NULL));
    testrun(3 == ov_dict_count(reg->calls));
    testrun(ov_mc_backend_sip_registry_register_call(reg, 0, "4", NULL, NULL));
    testrun(4 == ov_dict_count(reg->calls));

    testrun(ov_mc_backend_sip_registry_unregister_call(reg, "1"));
    testrun(3 == ov_dict_count(reg->calls));
    testrun(ov_dict_get(reg->calls, "2"));
    testrun(ov_dict_get(reg->calls, "3"));
    testrun(ov_dict_get(reg->calls, "4"));

    testrun(NULL == ov_mc_backend_sip_registry_free(reg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_sip_registry_get_call_proxy() {

    ov_mc_backend_sip_registry *reg = ov_mc_backend_sip_registry_create(
        (ov_mc_backend_sip_registry_config){0});

    testrun(reg);

    testrun(ov_mc_backend_sip_registry_register_call(reg, 1, "1", NULL, NULL));
    testrun(1 == ov_dict_count(reg->calls));
    testrun(ov_mc_backend_sip_registry_register_call(reg, 2, "2", NULL, NULL));
    testrun(2 == ov_dict_count(reg->calls));
    testrun(ov_mc_backend_sip_registry_register_call(reg, 3, "3", NULL, NULL));
    testrun(3 == ov_dict_count(reg->calls));
    testrun(ov_mc_backend_sip_registry_register_call(reg, 4, "4", NULL, NULL));
    testrun(4 == ov_dict_count(reg->calls));

    testrun(-1 == ov_mc_backend_sip_registry_get_call_proxy(NULL, NULL));
    testrun(-1 == ov_mc_backend_sip_registry_get_call_proxy(reg, NULL));
    testrun(-1 == ov_mc_backend_sip_registry_get_call_proxy(NULL, "1"));
    testrun(-1 == ov_mc_backend_sip_registry_get_call_proxy(reg, "5"));

    testrun(1 == ov_mc_backend_sip_registry_get_call_proxy(reg, "1"));
    testrun(2 == ov_mc_backend_sip_registry_get_call_proxy(reg, "2"));
    testrun(3 == ov_mc_backend_sip_registry_get_call_proxy(reg, "3"));
    testrun(4 == ov_mc_backend_sip_registry_get_call_proxy(reg, "4"));

    testrun(NULL == ov_mc_backend_sip_registry_free(reg));

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
    testrun_test(test_ov_mc_backend_sip_registry_create);
    testrun_test(test_ov_mc_backend_sip_registry_cast);
    testrun_test(test_ov_mc_backend_sip_registry_free);

    testrun_test(test_ov_mc_backend_sip_registry_register_proxy);
    testrun_test(test_ov_mc_backend_sip_registry_unregister_proxy);
    testrun_test(test_ov_mc_backend_sip_registry_get_proxy_socket);

    testrun_test(test_ov_mc_backend_sip_registry_register_call);
    testrun_test(test_ov_mc_backend_sip_registry_unregister_call);
    testrun_test(test_ov_mc_backend_sip_registry_get_call_proxy);

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
