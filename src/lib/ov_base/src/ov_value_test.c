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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#define OV_VALUE_TEST

#include "../include/ov_utils.h"
#include "ov_value.c"
#include <math.h>
#include <ov_test/ov_test.h>
#include <stdio.h>

/*----------------------------------------------------------------------------*/

static bool doubles_equal(double d1, double d2) {

    const double MAX_DELTA = 0.00000001;
    return MAX_DELTA > fabs(d1 - d2);
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_free() {

    testrun(0 == ov_value_free(0));

    /* null */

    ov_value *v = ov_value_null();
    testrun(0 != v);

    testrun(0 == ov_value_free(v));

    v = ov_value_null();
    testrun(0 != v);

    testrun(0 == ov_value_free(v));

    /* true */

    v = ov_value_true();
    testrun(0 != v);

    testrun(0 == ov_value_free(v));

    v = ov_value_true();
    testrun(0 != v);

    testrun(0 == ov_value_free(v));

    /* false */

    v = ov_value_false();
    testrun(0 != v);

    testrun(0 == ov_value_free(v));

    v = ov_value_false();
    testrun(0 != v);

    testrun(0 == ov_value_free(v));

    /* number */

    v = ov_value_number(17 + 4);
    testrun(0 != v);

    testrun(0 == ov_value_free(v));

    v = ov_value_number(21);
    testrun(0 != v);

    testrun(0 == ov_value_free(v));

    /* string */

    v = ov_value_string("aesir");
    testrun(0 != v);

    testrun(0 == ov_value_free(v));

    v = ov_value_string("vanir");
    testrun(0 != v);

    testrun(0 == ov_value_free(v));

    v = 0;

    v = ov_value_list(0);
    testrun(0 != v);

    v = ov_value_free(v);
    testrun(0 == v);

    /* Check again with non-empty list */
    v = ov_value_list(ov_value_null(), ov_value_string("Valkyrjar"));
    testrun(0 != v);

    /* Must work with nested lists as well */
    v = ov_value_list(ov_value_number(42), ov_value_true(), v,
                      ov_value_string("Surtr"));
    testrun(0 != v);

    v = ov_value_free(v);
    testrun(0 == v);

    /* Objects */

    v = ov_value_object();
    testrun(0 != v);

    ov_value_object_set(v, "gungnir", ov_value_string("spear"));
    ov_value_object_set(v, "skidbladnir", ov_value_string("ship"));
    ov_value_object_set(v, "Sif's golden hair", ov_value_string("hair"));

    ov_value *top = ov_value_object();
    testrun(0 != top);
    ov_value_object_set(top, "presents", v);

    v = ov_value_object();
    ov_value_object_set(v, "draupnir", ov_value_string("ring"));
    ov_value_object_set(v, "gullinborsti", ov_value_string("boar"));
    ov_value_object_set(v, "mjoellnir", ov_value_string("hammer"));

    ov_value_object_set(top, "reparations", v);

    v = ov_value_list(ov_value_string("laeding"), ov_value_string("droma"),
                      ov_value_string("gleipnir"));

    ov_value_object_set(top, "fetters", v);

    /* list incorporated into top object */
    v = 0;

    top = ov_value_free(top);
    testrun(0 == top);

    top = ov_value_free(ov_value_object());
    testrun(0 == top);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_copy() {

    testrun(0 == ov_value_copy(0));

    /* null */

    ov_value *v = ov_value_null();
    testrun(0 != v);

    ov_value *c = ov_value_copy(v);
    testrun(0 != c);
    testrun(ov_value_is_null(c));

    testrun(ov_value_is_null(v));
    v = ov_value_free(v);

    testrun(ov_value_is_null(c));
    c = ov_value_free(c);

    testrun(0 == v);
    testrun(0 == c);

    /* true */

    v = ov_value_true();
    testrun(0 != v);

    c = ov_value_copy(v);
    testrun(0 != c);
    testrun(ov_value_is_true(c));

    testrun(ov_value_is_true(v));
    v = ov_value_free(v);

    testrun(ov_value_is_true(c));
    c = ov_value_free(c);

    testrun(0 == v);
    testrun(0 == c);

    /* false */

    v = ov_value_false();
    testrun(0 != v);

    c = ov_value_copy(v);
    testrun(0 != c);
    testrun(ov_value_is_false(c));

    testrun(ov_value_is_false(v));
    v = ov_value_free(v);

    testrun(ov_value_is_false(c));

    c = ov_value_free(c);

    testrun(0 == v);
    testrun(0 == c);

    /* number */

    const double BLACK_JACK = 17 + 4;

    v = ov_value_number(BLACK_JACK);
    testrun(0 != v);

    c = ov_value_copy(v);
    testrun(0 != c);
    testrun(ov_value_is_number(c));
    testrun(doubles_equal(BLACK_JACK, ov_value_get_number(c)));

    v = ov_value_free(v);

    /* Ensure there are no interdependences between the 2 number instances */
    testrun(doubles_equal(BLACK_JACK, ov_value_get_number(c)));

    c = ov_value_free(c);

    testrun(0 == v);
    testrun(0 == c);

    /* second number to be sure that we don't deal with constants */

    v = ov_value_number(1 + BLACK_JACK);
    testrun(0 != v);

    c = ov_value_copy(v);
    testrun(0 != c);
    testrun(ov_value_is_number(c));
    testrun(doubles_equal(1 + BLACK_JACK, ov_value_get_number(c)));

    v = ov_value_free(v);

    /* Ensure there are no interdependences between the 2 number instances */
    testrun(doubles_equal(1 + BLACK_JACK, ov_value_get_number(c)));

    c = ov_value_free(c);

    testrun(0 == v);
    testrun(0 == c);

    /* String */

    char const *AESIR = "aesir";

    v = ov_value_string(AESIR);
    c = ov_value_copy(v);

    char const *data = ov_value_get_string(c);
    testrun(0 != data);
    testrun(0 == strcmp(AESIR, data));

    v = ov_value_free(v);
    testrun(0 == v);

    data = ov_value_get_string(c);
    testrun(0 != data);
    testrun(0 == strcmp(AESIR, data));

    c = ov_value_free(c);
    testrun(0 == c);

    /* lists */

    v = ov_value_list(0);
    testrun(0 != v);

    c = ov_value_copy(v);
    testrun(0 != c);
    testrun(c != v);

    v = ov_value_free(v);
    c = ov_value_free(c);

    v = ov_value_list(ov_value_null(), ov_value_string("Ratatoskr"),
                      ov_value_number(42));

    testrun(0 != v);

    v = ov_value_list(ov_value_true(), ov_value_string("Heimdall"), v,
                      ov_value_false(), ov_value_number(7887));

    testrun(0 != v);

    c = ov_value_copy(v);
    testrun(5 == ov_value_count(c));

    v = ov_value_free(v);
    testrun(0 == v);

    testrun(ov_value_is_true(ov_value_list_get(c, 0)));

    ov_value *entry = ov_value_list_get(c, 1);
    char const *s = ov_value_get_string(entry);
    testrun(0 != s);

    testrun(0 == strcmp(s, "Heimdall"));
    s = 0;

    /* Next entry is our first list */
    entry = ov_value_list_get(c, 2);

    testrun(0 != entry);
    testrun(3 == ov_value_count(entry));

    testrun(ov_value_is_null(ov_value_list_get(entry, 0)));

    ov_value *nested_list_entry = ov_value_list_get(entry, 1);

    s = ov_value_get_string(nested_list_entry);
    nested_list_entry = 0;

    testrun(0 != s);

    testrun(0 == strcmp(s, "Ratatoskr"));

    s = 0;

    testrun(42 == ov_value_get_number(ov_value_list_get(entry, 2)));
    testrun(0 == ov_value_list_get(entry, 3));

    /* Here, the nested list ends */
    entry = 0;

    testrun(ov_value_is_false(ov_value_list_get(c, 3)));
    testrun(7887 == ov_value_get_number(ov_value_list_get(c, 4)));

    testrun(0 == ov_value_list_get(entry, 3));

    c = ov_value_free(c);
    testrun(0 == c);

    // Objects

    v = OBJECT(
        0, PAIR("key1", ov_value_number(12)),
        PAIR("object1", OBJECT(0, PAIR("key2", ov_value_string("Here I am")))));

    testrun(0 != v);

    c = ov_value_copy(v);

    testrun(ov_value_match(v, c));

    v = ov_value_free(v);
    c = ov_value_free(c);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_value_count() {

    testrun(0 == ov_value_count(0));

    /* null */

    ov_value *v = ov_value_null();
    testrun(0 != v);

    testrun(1 == ov_value_count(v));
    v = ov_value_free(v);

    /* true */
    v = ov_value_true();
    testrun(0 != v);

    testrun(1 == ov_value_count(v));
    v = ov_value_free(v);

    /* false */
    v = ov_value_false();
    testrun(0 != v);

    testrun(1 == ov_value_count(v));
    v = ov_value_free(v);

    /* false */
    v = ov_value_false();
    testrun(0 != v);

    testrun(1 == ov_value_count(v));
    v = ov_value_free(v);

    /* number */
    v = ov_value_number(13);
    testrun(0 != v);

    testrun(1 == ov_value_count(v));
    v = ov_value_free(v);

    /* string */
    v = ov_value_string("dvalin");
    testrun(0 != v);

    testrun(1 == ov_value_count(v));
    v = ov_value_free(v);

    /* list */
    v = ov_value_list(0);
    testrun(0 != v);

    testrun(0 == ov_value_count(v));
    v = ov_value_free(v);

    v = ov_value_list(ov_value_null(), ov_value_string("Ratatoskr"),
                      ov_value_number(42));

    testrun(0 != v);

    v = ov_value_list(ov_value_true(), ov_value_string("Heimdall"), v,
                      ov_value_false(), ov_value_number(12));

    testrun(0 != v);

    testrun(5 == ov_value_count(v));

    testrun(3 == ov_value_count(ov_value_list_get(v, 2)));

    v = ov_value_free(v);
    testrun(0 == v);

    /* Object */

    v = ov_value_object();
    testrun(0 != v);

    testrun(0 == ov_value_count(v));

    ov_value_object_set(v, "1", ov_value_string("I am one"));
    ov_value_object_set(v, "2", ov_value_string("I am two"));

    testrun(2 == ov_value_count(v));

    ov_value *o2 = ov_value_object();

    ov_value_object_set(o2, "tier1-1", ov_value_string("I am one one"));
    ov_value_object_set(o2, "tier1-2", ov_value_string("I am one two"));
    ov_value_object_set(o2, "tier1-3", ov_value_string("I am one three"));

    testrun(3 == ov_value_count(o2));

    ov_value_object_set(v, "tier1", o2);

    // count only 'shallowly' - v contains 2 elements plus a list containing
    // 3 elements -> Depending on how to count its 5 or 6 elements,
    // but on the first level down its only 3 - 2 elements plus the list
    testrun(3 == ov_value_count(v));

    v = ov_value_free(v);
    testrun(0 == v);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static char *dump_to_string(ov_value const *v) {

    FILE *string_stream = 0;

    if (0 == v)
        return false;

    char *dumped = 0;
    size_t length = 0;

    string_stream = open_memstream(&dumped, &length);
    OV_ASSERT(0 != string_stream);

    if (!ov_value_dump(string_stream, v))
        goto error;

    fflush(string_stream);

error:

    if (0 != string_stream) {
        fclose(string_stream);
    }

    string_stream = 0;

    return dumped;
}

/*----------------------------------------------------------------------------*/

static bool check_dump_and_free(ov_value *v, char const *expected) {

    bool successful_p = false;

    if (0 == v)
        return false;
    if (0 == expected)
        return false;

    char *dumped = dump_to_string(v);

    successful_p = (0 == strcmp(expected, dumped));

    v = ov_value_free(v);
    OV_ASSERT(0 == v);

    dumped = ov_free(dumped);

    return successful_p;
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_dump() {

    testrun(!ov_value_dump(0, 0));
    testrun(!ov_value_dump(stdout, 0));

    ov_value *v = ov_value_null();

    testrun(!ov_value_dump(0, v));
    v = ov_value_free(v);

    testrun(check_dump_and_free(ov_value_null(), "null"));
    testrun(check_dump_and_free(ov_value_true(), "true"));
    testrun(check_dump_and_free(ov_value_false(), "false"));

    testrun(check_dump_and_free(ov_value_number(4), "4"));
    testrun(check_dump_and_free(ov_value_number(4.001), "4.0010000000"));
    testrun(check_dump_and_free(ov_value_number(.001), "0.0010000000"));
    testrun(check_dump_and_free(ov_value_number(.001), "0.0010000000"));
    testrun(check_dump_and_free(ov_value_number(-4.001), "-4.0010000000"));
    testrun(check_dump_and_free(ov_value_number(-.001002), "-0.0010020000"));

#define AESIR "Baldr Heimdall Hel Loki Skirnir Thor"

    testrun(check_dump_and_free(ov_value_string(AESIR), "\"" AESIR "\""));

#undef AESIR

    testrun(check_dump_and_free(ov_value_string("\"Escaped \\ should be "
                                                "respected\""),
                                "\"\\\"Escaped \\\\ should be "
                                "respected\\\"\""));

    /* List checks */

    testrun(check_dump_and_free(ov_value_list(0), "[]"));

    ov_value *list = ov_value_list(ov_value_true(), ov_value_string("Heimdall"),
                                   ov_value_null());
    testrun(0 != list);

    list = ov_value_list(ov_value_number(1337), list,
                         ov_value_string("This is the end"));
    testrun(0 != list);

    testrun(check_dump_and_free(
        list, "[1337,[true,\"Heimdall\",null],\"This is the end\"]"));

    list = 0;

    /* Objects */

    ov_value *object = ov_value_object();

    testrun(check_dump_and_free(object, "{}"));

    object = ov_value_object();
    ov_value_object_set(object, "true", ov_value_true());

    testrun(check_dump_and_free(object, "{\"true\":true}"));

    object = ov_value_object();
    ov_value_object_set(object, "list",
                        ov_value_list(ov_value_number(17), ov_value_number(4)));

    testrun(check_dump_and_free(object, "{\"list\":[17,4]}"));
    object = 0;

    // Cannot check objects with more than one key here reliably
    // because order of keys depends on dict implementation

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool check_to_string(ov_value *value, char const *expected) {

    char *str = ov_value_to_string(value);

    if (0 == str)
        return (0 == expected);

    if (0 == expected)
        goto error;

    value = ov_value_free(value);
    OV_ASSERT(0 == value);

    bool succeeded_p = (0 == strcmp(expected, str));

    free(str);

    return succeeded_p;

error:

    if (0 != str) {
        free(str);
    }

    return false;
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_to_string() {

    testrun(0 == ov_value_to_string(0));

    testrun(check_to_string(ov_value_null(), "null"));
    testrun(check_to_string(ov_value_true(), "true"));
    testrun(check_to_string(ov_value_false(), "false"));

    testrun(check_to_string(ov_value_number(4), "4"));
    testrun(check_to_string(ov_value_number(4.001), "4.0010000000"));
    testrun(check_to_string(ov_value_number(.001), "0.0010000000"));
    testrun(check_to_string(ov_value_number(.001), "0.0010000000"));
    testrun(check_to_string(ov_value_number(-4.001), "-4.0010000000"));
    testrun(check_to_string(ov_value_number(-.001002), "-0.0010020000"));

#define AESIR "Baldr Heimdall Hel Loki Skirnir Thor"

    testrun(check_to_string(ov_value_string(AESIR), "\"" AESIR "\""));

#undef AESIR

    /* List checks */

    testrun(check_to_string(ov_value_list(0), "[]"));

    ov_value *list = ov_value_list(ov_value_true(), ov_value_string("Heimdall"),
                                   ov_value_null());
    testrun(0 != list);

    list = ov_value_list(ov_value_number(1337), list,
                         ov_value_string("This is the end"));
    testrun(0 != list);

    testrun(check_to_string(
        list, "[1337,[true,\"Heimdall\",null],\"This is the end\"]"));

    list = 0;

    /* Objects */

    TODO("Implement");

    return testrun_log_success();
}

/*****************************************************************************
                                    for each
 ****************************************************************************/

static ov_value const *last_visited_value = 0;
static char const *last_visited_key = 0;
static void *last_visited_userdata = 0;
static size_t visited_elements = 0;

static bool visit_func(char const *key, ov_value const *value, void *userdata) {

    last_visited_key = key;
    last_visited_value = value;
    last_visited_userdata = userdata;

    ++visited_elements;

    return true;
}

/*----------------------------------------------------------------------------*/

struct list_check_func_arg {

    ov_value *list;
    ov_value const **elements;
    size_t elements_first_empty;
};

static bool list_check_func(char const *key, ov_value const *value,
                            void *userdata) {

    OV_ASSERT(0 == key);
    OV_ASSERT(0 != value);
    OV_ASSERT(0 != userdata);

    struct list_check_func_arg *arg = userdata;

    arg->elements[arg->elements_first_empty++] = value;

    return true;
}

/*----------------------------------------------------------------------------*/

struct object_check_func_arg {

    size_t num_entries;
    char const **keys_expected;
    bool *keys_seen;
};

static bool object_check_func(char const *key, ov_value const *value,
                              void *arg) {

    OV_ASSERT(0 != key);
    OV_ASSERT(0 != value);
    OV_ASSERT(0 != arg);

    struct object_check_func_arg *object_arg = arg;

    for (size_t i = 0; i < object_arg->num_entries; ++i) {

        bool matches = 0 == strcmp(object_arg->keys_expected[i], key);

        OV_ASSERT(!matches || !object_arg->keys_seen[i]);

        if (matches)
            object_arg->keys_seen[i] = true;

        if (matches)
            break;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_for_each() {

    testrun(!ov_value_for_each(0, 0, 0));

    ov_value *number = ov_value_number(12);

    testrun(!ov_value_for_each(number, 0, 0));

    testrun(ov_value_for_each(number, visit_func, 0));
    testrun(12 == ov_value_get_number(last_visited_value));
    testrun(0 == last_visited_key);
    testrun(0 == last_visited_userdata);
    testrun(1 == visited_elements);

    last_visited_value = 0;
    visited_elements = 0;

    testrun(ov_value_for_each(number, visit_func, &last_visited_value));
    testrun(12 == ov_value_get_number(last_visited_value));
    testrun(0 == last_visited_key);
    testrun(&last_visited_value == last_visited_userdata);
    testrun(1 == visited_elements);

    last_visited_value = 0;
    last_visited_userdata = 0;
    visited_elements = 0;

    number = ov_value_free(number);
    testrun(0 == number);

    /* null */

    ov_value *val = ov_value_null();
    testrun(0 != val);

    testrun(ov_value_for_each(val, visit_func, &last_visited_value));
    testrun(ov_value_is_null(val));
    testrun(0 == last_visited_key);
    testrun(&last_visited_value == last_visited_userdata);
    testrun(1 == visited_elements);

    last_visited_value = 0;
    last_visited_userdata = 0;
    visited_elements = 0;

    val = ov_value_free(val);
    testrun(0 == val);

    /* true */

    val = ov_value_true();
    testrun(0 != val);

    testrun(ov_value_for_each(val, visit_func, &last_visited_value));
    testrun(ov_value_is_true(val));
    testrun(0 == last_visited_key);
    testrun(&last_visited_value == last_visited_userdata);
    testrun(1 == visited_elements);

    last_visited_value = 0;
    last_visited_userdata = 0;
    visited_elements = 0;

    val = ov_value_free(val);
    testrun(0 == val);

    /* false */

    val = ov_value_false();
    testrun(0 != val);

    testrun(ov_value_for_each(val, visit_func, &last_visited_value));
    testrun(ov_value_is_false(val));
    testrun(0 == last_visited_key);
    testrun(&last_visited_value == last_visited_userdata);
    testrun(1 == visited_elements);

    last_visited_value = 0;
    last_visited_userdata = 0;
    visited_elements = 0;

    val = ov_value_free(val);
    testrun(0 == val);

    /* Number was checked at beginning of this test */

    /* string */

    char const *test_string = "abraKadabra";
    val = ov_value_string(test_string);
    testrun(0 != val);

    testrun(ov_value_for_each(val, visit_func, &last_visited_value));
    testrun(0 != ov_value_get_string(val));
    testrun(0 == strcmp(ov_value_get_string(val), test_string));
    testrun(0 == last_visited_key);
    testrun(&last_visited_value == last_visited_userdata);
    testrun(1 == visited_elements);

    last_visited_value = 0;
    last_visited_userdata = 0;
    visited_elements = 0;

    val = ov_value_free(val);
    testrun(0 == val);

    /* empty list */

    val = ov_value_list(0);

    testrun(ov_value_for_each(val, visit_func, &last_visited_value));
    testrun(0 == last_visited_key);
    testrun(0 == last_visited_value);
    testrun(0 == visited_elements);

    last_visited_value = 0;
    last_visited_userdata = 0;
    visited_elements = 0;

    val = ov_value_free(val);
    testrun(0 == val);

    /* Several elements list */

    val = ov_value_list(ov_value_null(), ov_value_number(7887),
                        ov_value_false(), ov_value_string("Raswidr"));
    testrun(0 != val);

    ov_value const *found_elements[5] = {0};

    struct list_check_func_arg arg = {

        .list = val,
        .elements = found_elements,
        .elements_first_empty = 0,
    };

    testrun(ov_value_for_each(val, list_check_func, &arg));

    testrun(4 == arg.elements_first_empty);

    for (size_t i = 0; i < arg.elements_first_empty; ++i) {

        ov_value *element = ov_value_list_get(val, i);
        testrun(element == found_elements[i]);
    }

    val = ov_value_free(val);
    testrun(0 == val);

    /* Objects */

    char const *keys_expected[] = {"1", "2", "3"};
    bool keys_seen[sizeof(keys_expected) / sizeof(keys_expected[0])] = {0};

    val = ov_value_object();

    ov_value_object_set(val, "2", ov_value_true());
    ov_value_object_set(val, "1", ov_value_true());
    ov_value_object_set(val, "3", ov_value_true());

    struct object_check_func_arg object_arg = {

        .num_entries = sizeof(keys_expected) / sizeof(keys_expected[0]),
        .keys_expected = keys_expected,
        .keys_seen = keys_seen,

    };

    ov_value_for_each(val, object_check_func, &object_arg);

    for (size_t i = 0; i < sizeof(keys_expected) / sizeof(keys_expected[0]);
         ++i) {

        testrun(keys_seen[i]);
    }

    val = ov_value_free(val);
    testrun(0 == val);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool check_match(ov_value *v1, ov_value *v2) {

    bool match_p = ov_value_match(v1, v2);

    v1 = ov_value_free(v1);
    v2 = ov_value_free(v2);
    OV_ASSERT(0 == v1);
    OV_ASSERT(0 == v2);

    return match_p;
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_match() {

    testrun(check_match(0, 0));
    testrun(!check_match(ov_value_null(), 0));
    testrun(!check_match(0, ov_value_null()));

    testrun(check_match(ov_value_null(), ov_value_null()));

    testrun(!check_match(ov_value_true(), ov_value_null()));
    testrun(!check_match(ov_value_true(), ov_value_false()));
    testrun(!check_match(ov_value_null(), ov_value_false()));

    testrun(check_match(ov_value_true(), ov_value_true()));
    testrun(check_match(ov_value_false(), ov_value_false()));

    // Numbers

    testrun(!check_match(ov_value_number(12), ov_value_false()));
    testrun(!check_match(ov_value_number(12), ov_value_number(42)));
    testrun(check_match(ov_value_number(42), ov_value_number(42)));

    // Strings
    testrun(!check_match(ov_value_true(), ov_value_string("Garmr")));
    testrun(!check_match(ov_value_string("Garmr"), ov_value_true()));
    testrun(!check_match(ov_value_string("Garmr"), ov_value_number(12)));
    testrun(!check_match(ov_value_string("Garmr"), ov_value_string("Fafnir")));
    testrun(check_match(ov_value_string("Garmr"), ov_value_string("Garmr")));

    testrun(!check_match(ov_value_list(0), ov_value_list(ov_value_null())));
    testrun(!check_match(ov_value_list(ov_value_null()), ov_value_list(0)));
    testrun(!check_match(ov_value_null(), ov_value_list(0)));
    testrun(!check_match(ov_value_list(0), ov_value_true()));

    testrun(!check_match(
        ov_value_list(ov_value_string("yggdrassil"), ov_value_number(42)),
        ov_value_list(0)));

    testrun(!check_match(ov_value_list(ov_value_list(ov_value_string("yggdrassi"
                                                                     "l"),
                                                     ov_value_number(42)),
                                       ov_value_list(0)),
                         ov_value_list(0)));

    testrun(check_match(ov_value_list(0), ov_value_list(0)));

    testrun(check_match(ov_value_list(ov_value_number(1)),
                        ov_value_list(ov_value_number(1))));

    testrun(!check_match(ov_value_list(ov_value_list(ov_value_string("yggdrassi"
                                                                     "l"),
                                                     ov_value_number(42)),
                                       ov_value_true()),
                         ov_value_list(0)));

    testrun(!check_match(ov_value_list(0),
                         ov_value_list(ov_value_list(ov_value_string("yggdra"
                                                                     "ssil"),
                                                     ov_value_number(42)),
                                       ov_value_true())));

    testrun(!check_match(ov_value_list(ov_value_list(ov_value_string("yggdra"
                                                                     "ssil"),
                                                     ov_value_number(42)),
                                       ov_value_true()),
                         ov_value_list(ov_value_list(ov_value_string("yggdra"
                                                                     "ssil"),
                                                     ov_value_number(43)),
                                       ov_value_true())));

    testrun(check_match(ov_value_list(0), ov_value_list(0)));

    testrun(check_match(ov_value_list(ov_value_list(ov_value_string("yggdras"
                                                                    "sil"),
                                                    ov_value_number(42)),
                                      ov_value_true()),
                        ov_value_list(ov_value_list(ov_value_string("yggdras"
                                                                    "sil"),
                                                    ov_value_number(42)),
                                      ov_value_true())));

    // Objects

    testrun(check_match(OBJECT(0, PAIR(0, 0)), OBJECT(0, PAIR(0, 0))));

    testrun(!check_match(
        OBJECT(0, PAIR("key1", ov_value_number(12)),
               PAIR("object1",
                    OBJECT(0, PAIR("key2", ov_value_string("Here I am"))))),
        OBJECT(
            0, PAIR("key1", ov_value_number(12)),
            PAIR("object1",
                 OBJECT(0, PAIR("key2", ov_value_string("Here I am not")))))));

    testrun(check_match(
        OBJECT(0, PAIR("key1", ov_value_number(12)),
               PAIR("object1",
                    OBJECT(0, PAIR("key2", ov_value_string("Here I am"))))),
        OBJECT(0, PAIR("key1", ov_value_number(12)),
               PAIR("object1",
                    OBJECT(0, PAIR("key2", ov_value_string("Here I am")))))));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_null() {

    testrun(&g_null == ov_value_null());

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_is_null() {

    testrun(!ov_value_is_null(0));

    ov_value *v = ov_value_null();
    testrun(0 != v);

    testrun(ov_value_is_null(v));

    v = ov_value_free(v);
    testrun(0 == v);

    v = ov_value_true();
    testrun(!ov_value_is_null(v));

    v = ov_value_free(v);
    testrun(0 == v);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_true() {

    testrun(&g_true == ov_value_true());

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_is_true() {

    testrun(!ov_value_is_true(0));

    ov_value *v = ov_value_true();
    testrun(0 != v);

    testrun(ov_value_is_true(v));

    v = ov_value_free(v);
    testrun(0 == v);

    v = ov_value_false();
    testrun(!ov_value_is_true(v));

    v = ov_value_free(v);
    testrun(0 == v);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_false() {

    testrun(&g_false == ov_value_false());

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_is_false() {

    testrun(!ov_value_is_false(0));

    ov_value *v = ov_value_false();
    testrun(0 != v);

    testrun(ov_value_is_false(v));

    v = ov_value_free(v);
    testrun(0 == v);

    v = ov_value_null();
    testrun(!ov_value_is_false(v));

    v = ov_value_free(v);
    testrun(0 == v);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_number() {

    ov_value *v = ov_value_number(0);
    testrun(0 != v);
    testrun(0 == ov_value_get_number(v));

    v = ov_value_free(v);
    testrun(0 == v);

    const double BLACK_JACK = 17 + 4;

    v = ov_value_number(BLACK_JACK);
    testrun(0 != v);
    testrun(doubles_equal(BLACK_JACK, ov_value_get_number(v)));

    v = ov_value_free(v);
    testrun(0 == v);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_is_number() {

    testrun(!ov_value_is_number(0));

    ov_value *v = ov_value_number(0);
    testrun(0 != v);
    testrun(ov_value_is_number(v));

    v = ov_value_free(v);
    testrun(0 == v);

    v = ov_value_null();

    testrun(0 != v);
    testrun(!ov_value_is_number(v));

    v = ov_value_free(v);
    testrun(0 == v);

    v = ov_value_string("MeierDoedel und DoodleDoedel fressen Weihnachten den "
                        "Baum auf - Dein bruenftig roechelnder Sohn, Koenigin "
                        "Victoria");

    testrun(0 != v);
    testrun(!ov_value_is_number(v));

    v = ov_value_free(v);
    testrun(0 == v);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_get_number() {

    testrun(0 == ov_value_get_number(0));

    ov_value *v = ov_value_number(0);
    testrun(0 != v);
    testrun(0 == ov_value_get_number(v));

    v = ov_value_free(v);
    testrun(0 == v);

    const double BLACK_JACK = 17 + 4;

    v = ov_value_number(BLACK_JACK);
    testrun(0 != v);
    testrun(doubles_equal(BLACK_JACK, ov_value_get_number(v)));

    v = ov_value_free(v);
    testrun(0 == v);

    const double THE_A = 42;

    v = ov_value_number(THE_A);
    testrun(0 != v);
    testrun(doubles_equal(THE_A, ov_value_get_number(v)));

    v = ov_value_free(v);
    testrun(0 == v);

    v = ov_value_number(M_PI);
    testrun(0 != v);
    testrun(doubles_equal(M_PI, ov_value_get_number(v)));

    v = ov_value_free(v);
    testrun(0 == v);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_string() {

    testrun(0 == ov_value_string(0));

    char const *AESIR = "Odin Thor Heimdall Vidarr";

    ov_value *v = ov_value_string(AESIR);
    char const *data = ov_value_get_string(v);
    testrun(0 != data);
    testrun(0 == strcmp(AESIR, data));

    v = ov_value_free(v);

    char const *VANIR = "Njoerthr Freyr Freya";

    v = ov_value_string(VANIR);
    data = ov_value_get_string(v);
    testrun(0 != data);
    testrun(0 == strcmp(VANIR, data));

    v = ov_value_free(v);
    testrun(0 == v);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_get_string() {

    testrun(0 == ov_value_get_string(0));

    ov_value *v = ov_value_null();
    testrun(0 == ov_value_get_string(v));
    v = ov_value_free(v);
    testrun(0 == v);

    v = ov_value_number(13);
    testrun(0 == ov_value_get_string(v));
    v = ov_value_free(v);
    testrun(0 == v);

    char const *AESIR = "Aesir Again";

    v = ov_value_string(AESIR);
    char const *data = ov_value_get_string(v);
    testrun(0 != data);
    testrun(0 == strcmp(AESIR, data));

    v = ov_value_free(v);
    testrun(0 == v);

    char const *VANIR = "VANIR";

    v = ov_value_string(VANIR);
    data = ov_value_get_string(v);
    testrun(0 != data);
    testrun(0 == strcmp(VANIR, data));

    v = ov_value_free(v);
    testrun(0 == v);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_list() {

    ov_value *v = ov_value_list(0);
    testrun(0 != v);

    testrun(!ov_value_is_null(v));
    testrun(!ov_value_is_true(v));
    testrun(!ov_value_is_false(v));

    testrun(!ov_value_is_number(v));
    testrun(0 == ov_value_get_string(v));

    v = ov_value_free(v);

    testrun(0 == v);

    v = ov_value_list(ov_value_null(), ov_value_number(13), ov_value_null(),
                      ov_value_string("Audumbla"));

    testrun(0 != v);

    v = ov_value_free(v);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_list_set() {

    testrun(0 == ov_value_list_set(0, 0, 0));

    ov_value *l = ov_value_list(0);
    testrun(0 != l);

    ov_value *zeroth = ov_value_true();
    testrun(0 != zeroth);

    testrun(0 == ov_value_list_set(l, 0, 0));

    testrun(0 == ov_value_list_set(l, 0, zeroth));
    testrun(zeroth == ov_value_list_set(l, 0, ov_value_false()));

    zeroth = ov_value_free(zeroth);
    testrun(0 == zeroth);

    char const *test_string = "Himinbjoerg";

    ov_value *third = ov_value_string(test_string);
    testrun(0 != third);

    testrun(0 == ov_value_list_set(l, 3, third));

    testrun(ov_value_is_false(ov_value_list_get(l, 0)));
    testrun(0 == ov_value_list_get(l, 1));
    testrun(0 == ov_value_list_get(l, 2));
    testrun(third = ov_value_list_get(l, 3));

    testrun(third = ov_value_list_set(l, 3, ov_value_null()));

    third = ov_value_free(third);
    testrun(0 == third);

    l = ov_value_free(l);
    testrun(0 == l);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_list_get() {

    testrun(0 == ov_value_list_get(0, 0));

    ov_value *l = ov_value_list(0);
    testrun(0 != l);

    testrun(0 == ov_value_list_get(l, 0));
    testrun(0 == ov_value_list_get(l, 1));
    testrun(0 == ov_value_list_get(l, 2));
    testrun(0 == ov_value_list_get(l, 3));
    testrun(0 == ov_value_list_get(l, 5));
    testrun(0 == ov_value_list_get(l, 17));

    ov_value *zeroth = ov_value_true();
    testrun(0 != zeroth);

    testrun(0 == ov_value_list_set(l, 0, zeroth));
    testrun(zeroth == ov_value_list_get(l, 0));
    testrun(0 == ov_value_list_get(l, 1));
    testrun(0 == ov_value_list_get(l, 2));
    testrun(0 == ov_value_list_get(l, 3));
    testrun(0 == ov_value_list_get(l, 5));
    testrun(0 == ov_value_list_get(l, 17));

    char const *test_string = "Himinbjoerg";

    ov_value *third = ov_value_string(test_string);
    testrun(0 != third);

    testrun(0 == ov_value_list_set(l, 3, third));

    testrun(zeroth == ov_value_list_set(l, 0, zeroth));
    testrun(zeroth == ov_value_list_get(l, 0));
    testrun(0 == ov_value_list_get(l, 1));
    testrun(0 == ov_value_list_get(l, 2));
    testrun(third == ov_value_list_get(l, 3));
    testrun(0 == ov_value_list_get(l, 5));
    testrun(0 == ov_value_list_get(l, 17));

    l = ov_value_free(l);
    testrun(0 == l);

    zeroth = 0;
    third = 0;

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_list_push() {

    testrun(!ov_value_list_push(0, 0));

    ov_value *list = ov_value_list(0);
    testrun(0 != list);

    testrun(!ov_value_list_push(list, 0));

    ov_value *value = ov_value_true();
    testrun(0 != value);

    testrun(ov_value_list_push(list, value));

    value = ov_value_number(1337);
    testrun(0 != value);
    testrun(ov_value_list_push(list, value));

    value = ov_value_null();
    testrun(0 != value);
    testrun(ov_value_list_push(list, value));

    ov_value const *elements[50] = {0};

    struct list_check_func_arg arg = {

        .list = list,
        .elements = elements,
        .elements_first_empty = 0,
    };

    testrun(ov_value_for_each(list, list_check_func, &arg));

    testrun(3 == arg.elements_first_empty);
    testrun(ov_value_is_true(elements[0]));
    testrun(1337 == ov_value_get_number(elements[1]));
    testrun(ov_value_is_null(elements[2]));

    list = ov_value_free(list);
    testrun(0 == list);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_enable_caching() {

    ov_value *numbers_allocated[50] = {0};
    const size_t NUMBERS =
        sizeof(numbers_allocated) / sizeof(numbers_allocated[0]);

    ov_value *strings_allocated[45] = {0};
    const size_t STRINGS =
        sizeof(strings_allocated) / sizeof(strings_allocated[0]);

    ov_value *lists_allocated[45] = {0};
    const size_t LISTS = sizeof(lists_allocated) / sizeof(lists_allocated[0]);

    const size_t OBJECTS = 3;

    ov_value_enable_caching(NUMBERS, STRINGS, LISTS, OBJECTS);

    /* 1st bit: Check numbers */

    ov_value *old = 0;
    for (size_t i = 0; i < NUMBERS; ++i) {

        ov_value *n = ov_value_number(i);

        if (0 != old)
            testrun(old == n);

        testrun(i == ov_value_get_number(n));

        old = n;
        n = ov_value_free(n);
        testrun(n == 0);
    }

    old = 0;

    /* 2nd bit: Check strings */

    for (size_t i = 0; i < STRINGS; ++i) {

        char test_string[20] = {0};
        snprintf(test_string, sizeof(test_string), "%zu", i);

        ov_value *n = ov_value_string(test_string);

        if (0 != old)
            testrun(old == n);

        testrun(0 == strcmp(test_string, ov_value_get_string(n)));

        old = n;
        n = ov_value_free(n);
        testrun(n == 0);
    }

    old = 0;

    /* 3rd bit: Check lists */

    for (size_t i = 0; i < LISTS; ++i) {

        ov_value *n = ov_value_list(0);
        testrun(0 != n);

        if (0 != old)
            testrun(old == n);

        old = n;
        n = ov_value_free(n);
        testrun(n == 0);
    }

    old = 0;

    /* 3rd bit: Check objects */

    for (size_t i = 0; i < LISTS; ++i) {

        ov_value *n = ov_value_object();
        testrun(0 != n);

        if (0 != old)
            testrun(old == n);

        old = n;
        n = ov_value_free(n);
        testrun(n == 0);
    }

    ov_registered_cache_free_all();

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_object() {

    ov_value *v = ov_value_object();
    testrun(0 != v);

    testrun(0 == ov_value_count(v));

    v = ov_value_free(v);
    testrun(0 == v);

    v = ov_value_object();
    testrun(0 != v);

    testrun(0 == ov_value_count(v));

    v = ov_value_free(v);
    testrun(0 == v);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_object_set() {

    testrun(0 == ov_value_object_set(0, 0, 0));

    ov_value *o = ov_value_object();
    testrun(0 != o);

    testrun(0 == ov_value_object_set(o, 0, 0));
    testrun(0 == ov_value_object_set(0, "key", 0));

    testrun(0 == ov_value_object_get(o, "key"));
    testrun(0 == ov_value_count(o));

    testrun(0 == ov_value_object_set(o, "key", 0));

    testrun(0 == ov_value_object_get(o, "key"));
    testrun(0 == ov_value_count(o));

    ov_value *value = ov_value_number(17 + 4);

    testrun(0 == ov_value_object_set(0, 0, value));

    testrun(0 == ov_value_count(o));

    testrun(0 == ov_value_object_get(o, "key"));

    testrun(0 == ov_value_object_set(o, 0, value));
    testrun(0 == ov_value_object_set(0, "key", value));

    testrun(0 == ov_value_object_get(o, "key"));
    testrun(0 == ov_value_count(o));

    testrun(0 == ov_value_object_set(o, "key", value));

    testrun(17 + 4 == ov_value_get_number(ov_value_object_get(o, "key")));
    testrun(1 == ov_value_count(o));

    o = ov_value_free(o);
    testrun(0 == o);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_object_get() {

    testrun(0 == ov_value_object_get(0, 0));

    ov_value *v = ov_value_object();
    testrun(0 != v);

    testrun(0 == ov_value_object_get(v, 0));
    testrun(0 == ov_value_object_get(v, "str"));

    ov_value *str1 = ov_value_string("string1");

    ov_value_object_set(v, "str", str1);

    testrun(str1 == ov_value_object_get(v, "str"));

    ov_value *tier1 = ov_value_object();
    ov_value_object_set(v, "tier1", tier1);

    ov_value *tier2 = ov_value_object();
    ov_value_object_set(tier1, "tier2", tier2);

    ov_value *str3 = ov_value_string("string3");
    ov_value_object_set(tier2, "str", str3);

    testrun(str3 == ov_value_object_get(v, "/tier1/tier2/str"));
    testrun(str3 == ov_value_object_get(v, "tier1/tier2/str"));
    testrun(0 == ov_value_object_get(v, "tier1/str"));
    testrun(str1 == ov_value_object_get(v, "str"));
    testrun(str1 == ov_value_object_get(v, "/str"));

    ov_value *str2 = ov_value_string("string2");
    ov_value_object_set(tier1, "str", str2);

    testrun(str2 == ov_value_object_get(v, "tier1/str"));

    v = ov_value_free(v);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_value", test_ov_value_free, test_ov_value_copy,
            test_ov_value_count, test_ov_value_dump, test_ov_value_to_string,
            test_ov_value_for_each, test_ov_value_match, test_ov_value_null,
            test_ov_value_is_null, test_ov_value_true, test_ov_value_is_true,
            test_ov_value_false, test_ov_value_is_false, test_ov_value_number,
            test_ov_value_is_number, test_ov_value_get_number,
            test_ov_value_string, test_ov_value_get_string, test_ov_value_list,
            test_ov_value_list_set, test_ov_value_list_get,
            test_ov_value_list_push, test_ov_value_object,
            test_ov_value_object_set, test_ov_value_object_get,
            test_ov_value_enable_caching);
