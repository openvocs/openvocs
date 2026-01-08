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
        @file           ov_dict_test_interface.h
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-08-15

        @ingroup        ov_base

        @brief          Definition of Interface test for
                        ov_dict implementations.

        ------------------------------------------------------------------------
*/
#ifndef ov_dict_test_interface_h
#define ov_dict_test_interface_h

#include "ov_dict.h"
#include <ov_test/testrun.h>

/*---------------------------------------------------------------------------*/

extern ov_dict *(*dict_creator)(ov_dict_config);

#define OV_DICT_PERFORM_PERFORM_INTERFACE_TESTS(create)                        \
    do {                                                                       \
                                                                               \
        dict_creator = create;                                                 \
                                                                               \
        testrun_test(test_impl_dict_is_empty);                                 \
        testrun_test(test_impl_dict_clear);                                    \
        testrun_test(test_impl_dict_free);                                     \
                                                                               \
        testrun_test(test_impl_dict_get_keys);                                 \
                                                                               \
        testrun_test(test_impl_dict_get);                                      \
        testrun_test(test_impl_dict_set);                                      \
        testrun_test(test_impl_dict_del);                                      \
        testrun_test(test_impl_dict_remove);                                   \
                                                                               \
        testrun_test(test_impl_dict_for_each);                                 \
                                                                               \
    } while (0)

/******************************************************************************
                                     TESTS
 ******************************************************************************/

int test_impl_dict_is_empty();
int test_impl_dict_clear();
int test_impl_dict_free();

int test_impl_dict_get_keys();

int test_impl_dict_get();
int test_impl_dict_set();
int test_impl_dict_del();
int test_impl_dict_remove();

int test_impl_dict_for_each();

#endif /* ov_dict_test_interface_h */
