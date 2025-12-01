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
        @file           ov_event_loop_test_interface.h
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2019-04-05

        @ingroup        ov_event_loop_test_interface

        @brief          Definition of interface tests for ov_event_loop


        ------------------------------------------------------------------------
*/
#ifndef ov_event_loop_test_interface_h
#define ov_event_loop_test_interface_h

#include "ov_event_loop.h"

extern ov_event_loop *(*event_loop_creator)(ov_event_loop_config config);

#define OV_EVENT_LOOP_PERFORM_INTERFACE_TESTS(create)                          \
  do {                                                                         \
                                                                               \
    event_loop_creator = create;                                               \
                                                                               \
    testrun_log("test_interface will run for several seconds "                 \
                "!!!");                                                        \
    testrun_test(test_impl_event_loop_free);                                   \
    testrun_test(test_impl_event_loop_is_running);                             \
    testrun_test(test_impl_event_loop_stop);                                   \
    testrun_test(test_impl_event_loop_run);                                    \
                                                                               \
    testrun_test(test_impl_event_loop_callback_set);                           \
    testrun_test(test_impl_event_loop_callback_unset);                         \
                                                                               \
    testrun_test(test_impl_event_loop_timer_set);                              \
    testrun_test(test_impl_event_loop_timer_unset);                            \
                                                                               \
  } while (0)

/*
 *      ------------------------------------------------------------------------
 *
 *      TESTS
 *
 *      ------------------------------------------------------------------------
 */

int test_impl_event_loop_is_running();
int test_impl_event_loop_stop();
int test_impl_event_loop_run();
int test_impl_event_loop_free();

int test_impl_event_loop_callback_set();
int test_impl_event_loop_callback_unset();

int test_impl_event_loop_timer_set();
int test_impl_event_loop_timer_unset();

#endif /* ov_event_loop_test_interface_h */
