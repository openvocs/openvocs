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
        @author         Michael Beer

        @date           2020-04-01

        @ingroup        ov_event_loop

        ------------------------------------------------------------------------
*/
/*
 * Necessary, because our build system will not link in
 * ov_reconnect_manager.o
 */
#include "ov_reconnect_manager.c"

#include "../../include/ov_reconnect_manager_test_interface.h"

#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

#include <stdbool.h>
#include <string.h>

/*----------------------------------------------------------------------------*/

int prepare() {

    ov_test_ignore_signals();
    return testrun_log_success();
}

/******************************************************************************
 *                        ov_reconnect_manager_create
 ******************************************************************************/

int test_ov_reconnect_manager_create() {

    testrun(0 == ov_reconnect_manager_create(0, 0, 0));
    testrun(0 == ov_reconnect_manager_create(0, 0, 1));
    testrun(0 == ov_reconnect_manager_create(0, 0, 2));
    testrun(0 == ov_reconnect_manager_create(0, 0, SIZE_MAX));

    ov_event_loop *el = ov_event_loop_default(ov_event_loop_config_default());

    testrun(0 != el);

    testrun(0 == ov_reconnect_manager_create(el, 0, 0));

    ov_reconnect_manager *rm = ov_reconnect_manager_create(el, 0, 1);

    testrun(0 != rm);

    testrun(0 == ov_reconnect_manager_free(rm));

    rm = ov_reconnect_manager_create(el, 0, 2);

    testrun(0 != rm);

    testrun(0 == ov_reconnect_manager_free(rm));

    rm = ov_reconnect_manager_create(el, 5, 2);

    testrun(0 != rm);

    testrun(0 == ov_reconnect_manager_free(rm));

    rm = 0;

    testrun(0 == el->free(el));

    el = 0;

    return testrun_log_success();
}

/******************************************************************************
 *                         ov_reconnect_manager_free
 ******************************************************************************/

int test_ov_reconnect_manager_free() {

    testrun(0 == ov_reconnect_manager_free(0));

    ov_event_loop *el = ov_event_loop_default(ov_event_loop_config_default());

    testrun(0 != el);

    testrun(0 == ov_reconnect_manager_create(el, 0, 0));

    ov_reconnect_manager *rm = ov_reconnect_manager_create(el, 0, 1);

    testrun(0 != rm);

    testrun(0 == ov_reconnect_manager_free(rm));

    testrun(0 == el->free(el));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_reconnect_manager_connect() {

    return ov_reconnect_manager_connect_test(ov_event_loop_default);
}

/******************************************************************************
 *                                 Run tests
 ******************************************************************************/

OV_TEST_RUN("ov_reconnect_manager", prepare, test_ov_reconnect_manager_create,
            test_ov_reconnect_manager_free, test_ov_reconnect_manager_connect);
