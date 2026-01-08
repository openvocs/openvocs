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
        @file           ov_mc_test_common_test.c
        @author         Markus Töpfer

        @date           2022-11-11


        ------------------------------------------------------------------------
*/
#include "ov_mc_test_common.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_mc_test_common_open_socket() {

    int socket[20] = {0};

    for (size_t i = 0; i < 10; i++) {

        socket[i] = ov_mc_test_common_open_socket(AF_INET);
        testrun(-1 != socket[i]);

        ov_socket_data data = {0};
        testrun(ov_socket_get_data(socket[i], &data, NULL));
        ov_log_debug("opened socket %i %s:%i", socket[i], data.host, data.port);
    }

    for (size_t i = 0; i < 10; i++) {

        close(socket[i]);
    }

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
    testrun_test(test_ov_mc_test_common_open_socket);

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
