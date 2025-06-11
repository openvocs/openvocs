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
        @file           ov_mc_backend_registry_test.c
        @author         Markus Töpfer

        @date           2023-01-21


        ------------------------------------------------------------------------
*/
#include "ov_mc_backend_registry.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_mc_backend_registry_create() {

    ov_mc_backend_registry *reg =
        ov_mc_backend_registry_create((ov_mc_backend_registry_config){0});
    testrun(reg);
    testrun(ov_mc_backend_registry_cast(reg));
    testrun(reg->users);

    testrun(reg->sockets > 0);

    for (size_t i = 0; i < reg->sockets; i++) {

        testrun(reg->socket[i].socket == -1);
    }

    testrun(NULL == ov_mc_backend_registry_free(reg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_registry_free() {

    ov_mc_backend_registry *reg =
        ov_mc_backend_registry_create((ov_mc_backend_registry_config){0});
    testrun(reg);
    testrun(ov_mc_backend_registry_cast(reg));
    testrun(NULL == ov_mc_backend_registry_free(reg));

    reg = ov_mc_backend_registry_create((ov_mc_backend_registry_config){0});

    testrun(ov_mc_backend_registry_register_mixer(
        reg, (ov_mc_mixer_data){.socket = 1, .uuid = "m-1"}));

    testrun(ov_mc_backend_registry_register_mixer(
        reg, (ov_mc_mixer_data){.socket = 2, .uuid = "m-2"}));

    testrun(ov_mc_backend_registry_register_mixer(
        reg, (ov_mc_mixer_data){.socket = 3, .uuid = "m-3"}));

    ov_mc_backend_registry_acquire_user(reg, "1-1-1-1");
    ov_mc_backend_registry_acquire_user(reg, "2-2-2-2");

    testrun(NULL == ov_mc_backend_registry_free(reg));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_registry_cast() {

    ov_mc_backend_registry *reg =
        ov_mc_backend_registry_create((ov_mc_backend_registry_config){0});
    testrun(reg);
    testrun(ov_mc_backend_registry_cast(reg));

    for (size_t i = 0; i < 0xffff; i++) {

        reg->magic_bytes = i;
        if (i == OV_MC_BACKEND_REGISTRY_MAGIC_BYTES) {
            testrun(ov_mc_backend_registry_cast(reg));
        } else {
            testrun(!ov_mc_backend_registry_cast(reg));
        }
    }

    reg->magic_bytes = OV_MC_BACKEND_REGISTRY_MAGIC_BYTES;

    testrun(NULL == ov_mc_backend_registry_free(reg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_registry_register_mixer() {

    ov_mc_backend_registry *reg =
        ov_mc_backend_registry_create((ov_mc_backend_registry_config){0});
    testrun(reg);

    testrun(ov_mc_backend_registry_register_mixer(
        reg, (ov_mc_mixer_data){.socket = 1, .uuid = "m-1"}));

    testrun(0 == strcmp("m-1", reg->socket[1].uuid));
    testrun(1 == reg->socket[1].socket);
    testrun(0 == reg->socket[1].user[0]);

    testrun(!ov_mc_backend_registry_register_mixer(
        reg, (ov_mc_mixer_data){.socket = 0, .uuid = "m-2"}));

    testrun(!ov_mc_backend_registry_register_mixer(
        reg, (ov_mc_mixer_data){.socket = reg->sockets + 1, .uuid = "m-2"}));

    testrun(NULL == ov_mc_backend_registry_free(reg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_registry_unregister_mixer() {

    ov_mc_backend_registry *reg =
        ov_mc_backend_registry_create((ov_mc_backend_registry_config){0});
    testrun(reg);

    testrun(ov_mc_backend_registry_unregister_mixer(reg, 1));
    testrun(!ov_mc_backend_registry_unregister_mixer(reg, 0));
    testrun(!ov_mc_backend_registry_unregister_mixer(NULL, 1));
    testrun(!ov_mc_backend_registry_unregister_mixer(reg, reg->sockets + 1));

    testrun(ov_mc_backend_registry_register_mixer(
        reg, (ov_mc_mixer_data){.socket = 1, .uuid = "m-1"}));

    testrun(0 == strcmp("m-1", reg->socket[1].uuid));
    testrun(1 == reg->socket[1].socket);
    testrun(0 == reg->socket[1].user[0]);

    testrun(ov_mc_backend_registry_unregister_mixer(reg, 1));

    testrun(-1 == reg->socket[1].socket);
    testrun(0 == reg->socket[1].user[0]);
    testrun(0 == reg->socket[1].uuid[0]);

    testrun(NULL == ov_mc_backend_registry_free(reg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_registry_count_mixers() {

    ov_mc_backend_registry *reg =
        ov_mc_backend_registry_create((ov_mc_backend_registry_config){0});
    testrun(reg);

    ov_mc_backend_registry_count count =
        ov_mc_backend_registry_count_mixers(reg);
    testrun(0 == count.mixers);
    testrun(0 == count.used);

    testrun(ov_mc_backend_registry_register_mixer(
        reg, (ov_mc_mixer_data){.socket = 1, .uuid = "m-1"}));

    count = ov_mc_backend_registry_count_mixers(reg);
    testrun(1 == count.mixers);
    testrun(0 == count.used);

    testrun(ov_mc_backend_registry_register_mixer(
        reg, (ov_mc_mixer_data){.socket = 2, .uuid = "m-2"}));

    count = ov_mc_backend_registry_count_mixers(reg);
    testrun(2 == count.mixers);
    testrun(0 == count.used);

    testrun(ov_mc_backend_registry_register_mixer(
        reg, (ov_mc_mixer_data){.socket = 3, .uuid = "m-4"}));

    count = ov_mc_backend_registry_count_mixers(reg);
    testrun(3 == count.mixers);
    testrun(0 == count.used);

    ov_mc_backend_registry_acquire_user(reg, "1-1-1-1");

    count = ov_mc_backend_registry_count_mixers(reg);
    testrun(3 == count.mixers);
    testrun(1 == count.used);

    ov_mc_backend_registry_acquire_user(reg, "2-1-1-1");

    count = ov_mc_backend_registry_count_mixers(reg);
    testrun(3 == count.mixers);
    testrun(2 == count.used);

    ov_mc_backend_registry_acquire_user(reg, "3-1-1-1");

    count = ov_mc_backend_registry_count_mixers(reg);
    testrun(3 == count.mixers);
    testrun(3 == count.used);

    testrun(ov_mc_backend_registry_unregister_mixer(reg, 1));

    count = ov_mc_backend_registry_count_mixers(reg);
    testrun(2 == count.mixers);
    testrun(2 == count.used);

    testrun(NULL == ov_mc_backend_registry_free(reg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_registry_acquire_user() {

    ov_mc_backend_registry *reg =
        ov_mc_backend_registry_create((ov_mc_backend_registry_config){0});
    testrun(reg);

    ov_mc_mixer_data data = ov_mc_backend_registry_acquire_user(reg, "1-1-1-1");
    testrun(0 == data.socket);

    testrun(ov_mc_backend_registry_register_mixer(
        reg, (ov_mc_mixer_data){.socket = 1, .uuid = "m-1"}));

    testrun(0 == reg->socket[1].user[0]);
    testrun(0 == strcmp("m-1", reg->socket[1].uuid));

    data = ov_mc_backend_registry_acquire_user(reg, "1-1-1-1");
    testrun(1 == data.socket);
    testrun(0 == strcmp(data.uuid, "m-1"));
    testrun(0 == strcmp(data.user, "1-1-1-1"));
    testrun(0 == strcmp("m-1", reg->socket[1].uuid));
    testrun(0 == strcmp("1-1-1-1", reg->socket[1].user));
    testrun(1 == (intptr_t)ov_dict_get(reg->users, "1-1-1-1"));

    testrun(ov_mc_backend_registry_register_mixer(
        reg, (ov_mc_mixer_data){.socket = 2, .uuid = "m-2"}));

    data = ov_mc_backend_registry_acquire_user(reg, "2-1-1-1");
    testrun(2 == data.socket);
    testrun(0 == strcmp(data.uuid, "m-2"));
    testrun(0 == strcmp(data.user, "2-1-1-1"));
    testrun(0 == strcmp("m-2", reg->socket[2].uuid));
    testrun(0 == strcmp("2-1-1-1", reg->socket[2].user));
    testrun(1 == (intptr_t)ov_dict_get(reg->users, "1-1-1-1"));
    testrun(2 == (intptr_t)ov_dict_get(reg->users, "2-1-1-1"));

    testrun(NULL == ov_mc_backend_registry_free(reg));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_registry_get_user() {

    ov_mc_backend_registry *reg =
        ov_mc_backend_registry_create((ov_mc_backend_registry_config){0});
    testrun(reg);

    ov_mc_mixer_data data = ov_mc_backend_registry_get_user(reg, "1-1-1-1");
    testrun(-1 == data.socket);

    testrun(ov_mc_backend_registry_register_mixer(
        reg, (ov_mc_mixer_data){.socket = 1, .uuid = "m-1"}));
    data = ov_mc_backend_registry_acquire_user(reg, "1-1-1-1");

    data = ov_mc_backend_registry_get_user(reg, "1-1-1-1");
    testrun(1 == data.socket);
    testrun(0 == strcmp(data.user, "1-1-1-1"));
    testrun(0 == strcmp(data.uuid, "m-1"));

    testrun(NULL == ov_mc_backend_registry_free(reg));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_registry_get_socket() {

    ov_mc_backend_registry *reg =
        ov_mc_backend_registry_create((ov_mc_backend_registry_config){0});
    testrun(reg);

    ov_mc_mixer_data data = ov_mc_backend_registry_get_socket(reg, 1);
    testrun(-1 == data.socket);

    testrun(ov_mc_backend_registry_register_mixer(
        reg, (ov_mc_mixer_data){.socket = 1, .uuid = "m-1"}));
    data = ov_mc_backend_registry_acquire_user(reg, "1-1-1-1");

    data = ov_mc_backend_registry_get_socket(reg, 1);
    testrun(1 == data.socket);
    testrun(0 == strcmp(data.user, "1-1-1-1"));
    testrun(0 == strcmp(data.uuid, "m-1"));

    testrun(NULL == ov_mc_backend_registry_free(reg));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_registry_release_user() {

    ov_mc_backend_registry *reg =
        ov_mc_backend_registry_create((ov_mc_backend_registry_config){0});
    testrun(reg);

    testrun(!ov_mc_backend_registry_release_user(NULL, "1-1-1-1"));
    testrun(!ov_mc_backend_registry_release_user(reg, NULL));
    testrun(ov_mc_backend_registry_release_user(reg, "1-1-1-1"));

    testrun(ov_mc_backend_registry_register_mixer(
        reg, (ov_mc_mixer_data){.socket = 1, .uuid = "m-1"}));
    ov_mc_mixer_data data = ov_mc_backend_registry_acquire_user(reg, "1-1-1-1");

    data = ov_mc_backend_registry_get_socket(reg, 1);
    testrun(1 == data.socket);
    testrun(0 == strcmp(data.user, "1-1-1-1"));
    testrun(0 == strcmp(data.uuid, "m-1"));
    testrun(1 == (intptr_t)ov_dict_get(reg->users, "1-1-1-1"));
    testrun(1 == reg->socket[1].socket);
    testrun(0 != reg->socket[1].user[0]);
    testrun(0 != reg->socket[1].uuid[0]);

    testrun(ov_mc_backend_registry_release_user(reg, "1-1-1-1"));
    testrun(1 == reg->socket[1].socket);
    testrun(0 == reg->socket[1].user[0]);
    testrun(0 != reg->socket[1].uuid[0]);
    testrun(0 == (intptr_t)ov_dict_get(reg->users, "1-1-1-1"));

    testrun(NULL == ov_mc_backend_registry_free(reg));
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
    testrun_test(test_ov_mc_backend_registry_create);
    testrun_test(test_ov_mc_backend_registry_free);
    testrun_test(test_ov_mc_backend_registry_cast);

    testrun_test(test_ov_mc_backend_registry_register_mixer);
    testrun_test(test_ov_mc_backend_registry_unregister_mixer);

    testrun_test(test_ov_mc_backend_registry_count_mixers);

    testrun_test(test_ov_mc_backend_registry_acquire_user);
    testrun_test(test_ov_mc_backend_registry_get_user);
    testrun_test(test_ov_mc_backend_registry_get_socket);
    testrun_test(test_ov_mc_backend_registry_release_user);

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
