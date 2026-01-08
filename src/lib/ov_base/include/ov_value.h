/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

        Minimalistic Data Representation.

        Notable stuff:

        - There is no need for ov_value_is_string, just do an
          ov_value_get_string() and check if it returns 0

        ------------------------------------------------------------------------
*/
#ifndef OV_VALUE_H
#define OV_VALUE_H

#include "ov_utils.h"
#include <stdbool.h>
#include <stddef.h>

/*----------------------------------------------------------------------------*/

struct ov_value;

typedef struct ov_value ov_value;

/*****************************************************************************
                                Common functions
 ****************************************************************************/

ov_value *ov_value_free(ov_value *value);

ov_value *ov_value_copy(ov_value const *value);

/**
 * @returns the number of values directly contained within `value`
 */
size_t ov_value_count(ov_value const *value);

bool ov_value_dump(FILE *stream, ov_value const *value);

char *ov_value_to_string(ov_value const *value);

/**
 * Traverses all values in `value` and calls `func` on them.
 * If `value` is an object, called with `key/val`.
 * If `value` is not an object, only the `val` is given, `key` is 0.
 *
 * For ordered things like a list, the order in which the elements appear
 * is guaranteed to be the order of the elements in the list.
 *
 * @func Function to be called for all values contained.
 */
bool ov_value_for_each(ov_value const *value,
                       bool (*func)(char const *key, ov_value const *val,
                                    void *userdata),
                       void *userdata);

bool ov_value_match(ov_value const *v1, ov_value const *v2);

/*****************************************************************************
                                     Atoms
 ****************************************************************************/

/* null */

ov_value *ov_value_null();

bool ov_value_is_null(ov_value const *value);

/*----------------------------------------------------------------------------*/
/* true */

ov_value *ov_value_true();

bool ov_value_is_true(ov_value const *value);

/*----------------------------------------------------------------------------*/
/* false */

ov_value *ov_value_false();

bool ov_value_is_false(ov_value const *value);

/*****************************************************************************
                                    Literals
 ****************************************************************************/

/* number */

ov_value *ov_value_number(double number);

bool ov_value_is_number(ov_value const *value);

double ov_value_get_number(ov_value const *value);

/*----------------------------------------------------------------------------*/
/* string */

ov_value *ov_value_string(char const *string);

char const *ov_value_get_string(ov_value const *value);

/*****************************************************************************
                                   List
 ****************************************************************************/

/**
 * Creates a new list.
 * You might use it like
 *
 * // Create an empty list
 * ov_value *value = ov_value_list(0);
 *
 * // Create list ["ratatoskr", null, 42]
 * value = ov_value_list(
 *     ov_value_string("ratatoskr"),
 *     ov_value_null(),
 *     ov_value_number(42));
 *
 * etc...
 */
/**
 * Create a list from given values. The values are put straight into the list
 * and not copied
 */
#define ov_value_list(...)                                                     \
    internal_ov_value_list((ov_value *[]){__VA_ARGS__, 0})

ov_value *ov_value_list_get(ov_value *list, size_t i);

ov_value *ov_value_list_set(ov_value *list, size_t i, ov_value *content);

bool ov_value_list_push(ov_value *list, ov_value *value);

bool ov_value_is_list(ov_value const *value);

/*****************************************************************************
                                     Object
 ****************************************************************************/

/**
 */
ov_value *ov_value_object();

bool ov_value_is_object(ov_value const *value);

/**
 * Retrieve an element from an object.
 * Paths are supported:
 *
 * If your object looks like
 *
 * {"tier1":{ "str" : "string2" }, "str" : "string1}
 *
 * You can do a ov_value_object_get(o, "str") => "string1"
 * You can do a ov_value_object_get(o, "/str") => "string1"
 * You can do a ov_value_object_get(o, "tier1/str") => "string2"
 * You can do a ov_value_object_get(o, "/tier1/str") => "string2"
 *
 * BEWARE: The returned value is still part of the surrounding `val` .
 *         If you alter it, you alter `val` - freeing it will corrupt `val`.
 */
ov_value const *ov_value_object_get(ov_value const *val, char const *key);

ov_value *ov_value_object_set(ov_value *val, char const *key,
                              ov_value *content);

/*****************************************************************************
                                    Caching
 ****************************************************************************/

void ov_value_enable_caching(size_t numbers, size_t strings, size_t lists,
                             size_t objects);

/*****************************************************************************
                                   Internals
 ****************************************************************************/

ov_value *internal_ov_value_list(ov_value **values);

/*----------------------------------------------------------------------------*/
//                  Simplified object building for tests

#ifdef OV_VALUE_TEST

typedef struct {

    char const *key;
    ov_value *value;

} ov_key_value_pair;

#define PAIR(k, v)                                                             \
    (ov_key_value_pair) { .key = k, .value = v }

#define OBJECT(n, ...) object_build(n, (ov_key_value_pair[]){__VA_ARGS__, {0}})

static ov_value *object_build(size_t dummy, ov_key_value_pair pairs[]) {

    UNUSED(dummy);

    ov_value *o = ov_value_object();

    ov_key_value_pair *pair = pairs;

    while (0 != pair->key) {

        ov_value_object_set(o, pair->key, pair->value);
        ++pair;
    }

    return o;
}
#endif

/*----------------------------------------------------------------------------*/

#endif
