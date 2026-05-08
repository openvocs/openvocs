/*** ------------------------------------------------------------------------

        Copyright 2017 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the openvocs project. http://openvocs.org

        ------------------------------------------------------------------------
*//**
        @file           ov_utils.c
        @author         Michael Beer

        @date           2017-12-10

        @ingroup        ov_basics

        @brief


        ------------------------------------------------------------------------
*/
#include "../../include/ov_utils.h"
#include "ov_result.h"
#include <math.h>
#include <time.h>

/*---------------------------------------------------------------------------*/

double ov_utils_db_to_factor(double dB) { return pow(10, dB / 10.0); }

/*----------------------------------------------------------------------------*/

void *ov_free(void *vptr) {

    if (0 != vptr) {
        free(vptr);
    }

    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_utils_init_random_generator() {

    time_t t;
    srandom(time(&t));
    return true;
}

/*----------------------------------------------------------------------------*/

static ssize_t get_empty_array_entry(void **array, size_t array_length) {

    if (0 == array)
        goto error;
    if (1 > array_length)
        goto error;

    for (size_t index = 0; array_length > index; ++index) {
        if (0 == array[index])
            return index;
    }

error:

    return -1;
}

/*----------------------------------------------------------------------------*/

bool ov_utils_add_to_array(void *array, size_t capacity, void *pointer_to_add) {

    void **my_array = array;

    ssize_t i = get_empty_array_entry(my_array, capacity);

    if (0 > i)
        return false;

    my_array[i] = pointer_to_add;

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_utils_del_from_array(void *array, size_t array_length,
                             void *pointer_to_remove) {

    if (0 == array)
        goto error;
    if (1 > array_length)
        goto error;

    void **my_array = array;

    for (size_t i = 0; i < array_length; ++i) {

        if (pointer_to_remove == my_array[i]) {
            my_array[i] = 0;
            return true;
        }
    }

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_utils_is_in_array(void const *array, size_t array_length,
                          void const *pointer) {

    if (0 == array) {
        return false;
    } else if (1 > array_length) {
        return false;
    } else {

        void const *const *my_array = array;

        for (size_t i = 0; i < array_length; ++i) {

            if (pointer == my_array[i]) {
                return true;
            }
        }

        return false;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_cond_valid_internal(char const *file, ov_log_level loglevel,
                            char const *function, size_t line,
                            struct ov_result *result, bool condition,
                            uint64_t error_code, char const *msg) {

    if (!condition) {

        ov_log_ng(loglevel, file, function, line, msg);
        ov_result_set(result, error_code, msg);
    }

    return condition;
}

/*----------------------------------------------------------------------------*/

bool ov_ptr_valid_internal(char const *file, ov_log_level loglevel,
                           char const *function, size_t line,
                           struct ov_result *result, void const *ptr,
                           uint64_t error_code, char const *msg) {

    if (0 == ptr) {

        ov_log_ng(loglevel, file, function, line, "%s: 0 pointer", msg);
        ov_result_set(result, error_code, msg);

        return false;

    } else {

        return true;
    }
}
