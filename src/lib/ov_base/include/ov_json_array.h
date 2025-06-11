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
        @file           ov_json_array.h
        @author         Markus Toepfer

        @date           2018-12-02

        @ingroup        ov_json_array

        @brief          Definition of a standard interface for JSON array
                        implementations.

        ------------------------------------------------------------------------
*/
#ifndef ov_json_array_h
#define ov_json_array_h

#include <stdbool.h>
#include <stdlib.h>

#include "ov_data_function.h"
#include "ov_json_value.h"

/*
 *      ------------------------------------------------------------------------
 *
 *      DEFAULT
 *
 *      ------------------------------------------------------------------------
 */

ov_json_value *ov_json_array();
bool ov_json_is_array(const ov_json_value *value);

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ... implementation independent functions
 *
 *      ------------------------------------------------------------------------
 */

bool ov_json_array_clear(void *data);
void *ov_json_array_free(void *data);
void *ov_json_array_copy(void **dest, const void *src);
bool ov_json_array_dump(FILE *stream, const void *src);
ov_data_function ov_json_array_data_functions();

/*
 *      ------------------------------------------------------------------------
 *
 *      VALUE BASED INTERFACE FUNCTION CALLS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_json_array_push(ov_json_value *array, ov_json_value *value);

ov_json_value *ov_json_array_pop(ov_json_value *array);

size_t ov_json_array_find(ov_json_value *array, const ov_json_value *child);

bool ov_json_array_del(ov_json_value *array, size_t position);

ov_json_value *ov_json_array_get(ov_json_value *array, size_t position);

ov_json_value *ov_json_array_remove(ov_json_value *array, size_t position);

bool ov_json_array_insert(ov_json_value *array,
                          size_t position,
                          ov_json_value *value);

size_t ov_json_array_count(const ov_json_value *array);

bool ov_json_array_is_empty(const ov_json_value *array);

bool ov_json_array_for_each(ov_json_value *array,
                            void *data,
                            bool (*function)(void *value, void *data));

bool ov_json_array_remove_child(ov_json_value *array, ov_json_value *child);

/*
 *      ------------------------------------------------------------------------
 *
 *      INLINE INTERFACE TEST DEFINITION
 *
 *      ------------------------------------------------------------------------
 */

#include <ov_test/testrun.h>

extern ov_json_value *(*array_creator)();

#define OV_JSON_ARRAY_PERFORM_INTERFACE_TESTS(create)                          \
    do {                                                                       \
                                                                               \
        array_creator = create;                                                \
                                                                               \
        testrun_test(test_impl_json_array_clear);                              \
        testrun_test(test_impl_json_array_free);                               \
                                                                               \
        testrun_test(test_impl_json_array_push);                               \
        testrun_test(test_impl_json_array_pop);                                \
                                                                               \
        testrun_test(test_impl_json_array_find);                               \
        testrun_test(test_impl_json_array_del);                                \
        testrun_test(test_impl_json_array_get);                                \
        testrun_test(test_impl_json_array_remove);                             \
        testrun_test(test_impl_json_array_insert);                             \
                                                                               \
        testrun_test(test_impl_json_array_count);                              \
        testrun_test(test_impl_json_array_is_empty);                           \
        testrun_test(test_impl_json_array_for_each);                           \
        testrun_test(test_impl_json_array_remove_child);                       \
                                                                               \
    } while (0)

/*
 *      ------------------------------------------------------------------------
 *
 *      TESTS
 *
 *      ------------------------------------------------------------------------
 */

int test_impl_json_array_clear();
int test_impl_json_array_free();

int test_impl_json_array_push();
int test_impl_json_array_pop();

int test_impl_json_array_find();
int test_impl_json_array_del();
int test_impl_json_array_get();
int test_impl_json_array_remove();
int test_impl_json_array_insert();

int test_impl_json_array_count();
int test_impl_json_array_is_empty();
int test_impl_json_array_for_each();
int test_impl_json_array_remove_child();

#endif /* ov_json_array_h */
