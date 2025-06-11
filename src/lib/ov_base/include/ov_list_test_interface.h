/***
        ------------------------------------------------------------------------

        Copyright 2018 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_list.h
        @author         Michael Beer

        @date           2018-07-02

        @ingroup        ov_base

        @brief          Definition of a standard interface for LIST
                        implementations used for openvocs.
    ------------------------------------------------------------------------
    */

#ifndef ov_list_test_interface_c
#define ov_list_test_interface_c

#include "ov_list.h"
#include <ov_test/testrun.h>

/*---------------------------------------------------------------------------*/

extern ov_list *(*list_creator)(ov_list_config);

#define OV_LIST_PERFORM_INTERFACE_TESTS(create)                                \
    do {                                                                       \
                                                                               \
        list_creator = create;                                                 \
                                                                               \
        testrun_test(test_impl_list_is_empty);                                 \
                                                                               \
        testrun_test(test_impl_list_clear);                                    \
        testrun_test(test_impl_list_free);                                     \
                                                                               \
        testrun_test(test_impl_list_get_pos);                                  \
                                                                               \
        testrun_test(test_impl_list_get);                                      \
        testrun_test(test_impl_list_set);                                      \
                                                                               \
        testrun_test(test_impl_list_insert);                                   \
        testrun_test(test_impl_list_remove);                                   \
                                                                               \
        testrun_test(test_impl_list_push);                                     \
        testrun_test(test_impl_list_pop);                                      \
                                                                               \
        testrun_test(test_impl_list_count);                                    \
        testrun_test(test_impl_list_for_each);                                 \
                                                                               \
        testrun_test(test_impl_list_copy);                                     \
                                                                               \
        testrun_test(test_impl_list_iter);                                     \
        testrun_test(test_impl_list_next);                                     \
                                                                               \
    } while (0)

/******************************************************************************
                                     TESTS
 ******************************************************************************/

int test_impl_list_is_empty();
int test_impl_list_clear();
int test_impl_list_free();
int test_impl_list_copy();
int test_impl_list_set();
int test_impl_list_get();
int test_impl_list_get_pos();
int test_impl_list_insert();
int test_impl_list_remove();
int test_impl_list_push();
int test_impl_list_pop();
int test_impl_list_count();
int test_impl_list_for_each();
int test_impl_list_iter();
int test_impl_list_next();

/*---------------------------------------------------------------------------*/
#endif
