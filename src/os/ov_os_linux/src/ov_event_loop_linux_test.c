/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_event_loop_linux_test.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2019-09-04

        @ingroup        ov_event_loop

        ------------------------------------------------------------------------
*/

#include <ov_test/ov_test.h>

#if defined __linux__

#include "ov_event_loop_linux.c"
#include <ov_base/ov_event_loop_test_interface.h>
#include <ov_base/ov_reconnect_manager_test_interface.h>

/*----------------------------------------------------------------------------*/

static int test_reconnect() {

  return ov_reconnect_manager_connect_test(ov_event_loop_linux);
}

/*----------------------------------------------------------------------------*/

int all_tests() {

  testrun_init();

  OV_EVENT_LOOP_PERFORM_INTERFACE_TESTS(ov_event_loop_linux);
  testrun_test(test_reconnect);

  return testrun_counter;
}

testrun_run(all_tests);

#else

OV_TEST_RUN("OV_EVENT_LOOP_LINUX", NULL);

#endif /* linux */
