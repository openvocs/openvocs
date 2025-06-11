/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_turn_permission_test.c
        @author         Markus Töpfer

        @date           2022-01-30


        ------------------------------------------------------------------------
*/
#include "ov_turn_permission.c"
#include <ov_test/testrun.h>

int test_ov_turn_permisson_create() {

    char *host = "192.168.1.1";

    uint64_t now = ov_time_get_current_time_usecs();

    testrun(!ov_turn_permission_create(NULL, 0));
    ov_turn_permission *perm = ov_turn_permission_create(host, 0);
    testrun(perm);
    testrun(now <= perm->updated);
    testrun(0 == perm->lifetime_usec);
    testrun(0 == strcmp(host, perm->host))
        testrun(NULL == ov_turn_permission_free(perm));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_permission_free() {

    char *host = "192.168.1.1";

    ov_turn_permission *perm = ov_turn_permission_create(host, 0);
    testrun(NULL == ov_turn_permission_free(perm));
    testrun(NULL == ov_turn_permission_free(NULL));

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
    testrun_test(test_ov_turn_permisson_create);
    testrun_test(test_ov_turn_permission_free);

    return testrun_counter;
}

testrun_run(all_tests);
