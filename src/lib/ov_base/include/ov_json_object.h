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
        @file           ov_json_object.h
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-12-02

        @ingroup        ov_json_object

        @brief          Definition of a JSON object interface.

        ------------------------------------------------------------------------
*/
#ifndef ov_json_object_h
#define ov_json_object_h

#include <stdbool.h>
#include <stdlib.h>

#include "ov_data_function.h"
#include "ov_list.h"

#include "ov_json_value.h"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      DEFAULT
 *
 *      ------------------------------------------------------------------------
 */

ov_json_value *ov_json_object();
bool ov_json_is_object(const ov_json_value *value);

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ... implementation independent functions
 *
 *      ------------------------------------------------------------------------
 */

bool ov_json_object_clear(void *data);
void *ov_json_object_free(void *data);
void *ov_json_object_copy(void **dest, const void *src);
bool ov_json_object_dump(FILE *stream, const void *src);
ov_data_function ov_json_object_data_functions();

/*
 *      ------------------------------------------------------------------------
 *
 *      VALUE BASED INTERFACE FUNCTION CALLS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_json_object_set(ov_json_value *obj, const char *key,
                        ov_json_value *value);

bool ov_json_object_del(ov_json_value *obj, const char *key);

ov_json_value *ov_json_object_get(const ov_json_value *obj, const char *key);

ov_json_value *ov_json_object_remove(ov_json_value *obj, const char *key);

size_t ov_json_object_count(const ov_json_value *obj);

bool ov_json_object_is_empty(const ov_json_value *obj);

bool ov_json_object_for_each(ov_json_value *obj, void *data,
                             bool (*function)(const void *key, void *value,
                                              void *data));

bool ov_json_object_remove_child(ov_json_value *obj, ov_json_value *child);

/*
 *      ------------------------------------------------------------------------
 *
 *      INLINE INTERFACE TEST DEFINITION
 *
 *      ------------------------------------------------------------------------
 */

extern ov_json_value *(*object_creator)();

#define OV_JSON_OBJECT_PERFORM_INTERFACE_TESTS(create)                         \
    do {                                                                       \
                                                                               \
        object_creator = create;                                               \
                                                                               \
        testrun_test(test_impl_json_object_clear);                             \
        testrun_test(test_impl_json_object_free);                              \
                                                                               \
        testrun_test(test_impl_json_object_set);                               \
        testrun_test(test_impl_json_object_del);                               \
        testrun_test(test_impl_json_object_get);                               \
        testrun_test(test_impl_json_object_remove);                            \
                                                                               \
        testrun_test(test_impl_json_object_count);                             \
        testrun_test(test_impl_json_object_is_empty);                          \
        testrun_test(test_impl_json_object_for_each);                          \
        testrun_test(test_impl_json_object_remove_child);                      \
                                                                               \
    } while (0)

/*
 *      ------------------------------------------------------------------------
 *
 *      TESTS
 *
 *      ------------------------------------------------------------------------
 */

int test_impl_json_object_clear();
int test_impl_json_object_free();

int test_impl_json_object_set();
int test_impl_json_object_del();
int test_impl_json_object_get();
int test_impl_json_object_remove();

int test_impl_json_object_count();
int test_impl_json_object_is_empty();
int test_impl_json_object_for_each();
int test_impl_json_object_remove_child();

#endif /* ov_json_object_h */
