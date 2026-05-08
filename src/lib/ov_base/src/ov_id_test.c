/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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

#include <ov_test/ov_test.h>

#include "ov_id.c"

/*----------------------------------------------------------------------------*/

static int test_ov_id_valid() {

    testrun(!ov_id_valid(0));
    testrun(!ov_id_valid(""));
    testrun(ov_id_valid("1"));
    testrun(ov_id_valid("123456789012345678901234567890123456"));
    testrun(!ov_id_valid("This string is too long by one char.."));
    testrun(
        !ov_id_valid("This string is way too long - ought to shorten it to 36 "
                     "chars"));

    ov_id id = {0};
    testrun(!ov_id_valid(id));

    testrun(ov_id_set(id, "arachnophobia"));
    testrun(ov_id_valid(id));

    testrun(ov_id_clear(id));
    testrun(!ov_id_valid(id));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_id_set() {

    testrun(!ov_id_set(0, 0));

    ov_id id = {0};

    testrun(!ov_id_set(id, 0));
    testrun(!ov_id_set(id, ""));
    testrun(!ov_id_set(0, "arbiter"));

    testrun(ov_id_set(id, "arbiter"));
    testrun(ov_id_match(id, "arbiter"));

    testrun(ov_id_set(id, "melanchthon-a fine name if you dead"));
    testrun(ov_id_match(id, "melanchthon-a fine name if you dead"));
    testrun(ov_id_set(id, "melanchthon-a fine name if you dead"));

    // desired ID string is too long
    testrun(!ov_id_set(id, "1234567890123456789012345678901234567"));

    // Can use ov_id as source as well?
    ov_id src = "abcdef";
    testrun(ov_id_set(id, src));
    testrun(ov_id_valid(id));
    testrun(ov_id_match(id, src));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_id_fill_with_uuid() {

    testrun(!ov_id_fill_with_uuid(0));

    for (size_t i = 0; i < 25; ++i) {

        ov_id id1;
        testrun(ov_id_fill_with_uuid(id1));
        testrun(ov_id_valid(id1));

        ov_id id2;
        testrun(ov_id_fill_with_uuid(id2));
        testrun(ov_id_valid(id2));

        testrun(!ov_id_match(id1, id2));

        fprintf(stderr, "id1 is %s, id2 is %s\n", id1, id2);
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_id_match() {

    testrun(ov_id_match(0, 0));

    testrun(!ov_id_match("anton", 0));
    testrun(!ov_id_match(0, "anton"));
    testrun(!ov_id_match("alpha", "bravo"));

    testrun(ov_id_match("alpha", "alpha"));

    // Check that the first 36 chars are taken into account
    testrun(!ov_id_match("alpha", "alphabravo"));

    testrun(ov_id_match("123456789012345678901234567890123456",
                        "123456789012345678901234567890123456"));

    testrun(!ov_id_match("123456789012345678901234567890123456",
                         "123456789012345678901234567890123456 trailing stuff "
                         "not to be ignored"));

    testrun(!ov_id_match("123456789012345678901234567890123456 trailing stuff "
                         "ignored",
                         "123456789012345678901234567890123456"));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_id_array_reset() {
    // bool ov_id_array_reset(ov_id ids[], size_t capacity);

    testrun(!ov_id_array_reset(0, 0));

    ov_id ids[3] = {"a", "b222", "c333333"};

    size_t capacity = sizeof(ids) / sizeof(ids[0]);

    for (size_t i = 0; i < capacity; ++i) {
        fprintf(stderr, "ID %s\n", ids[i]);
    }

    testrun(ov_id_array_reset(ids, 0));
    testrun(ov_id_match(ids[0], "a"));
    testrun(ov_id_match(ids[1], "b222"));
    testrun(ov_id_match(ids[2], "c333333"));

    testrun(!ov_id_array_reset(0, capacity));
    testrun(ov_id_array_reset(ids, 1));

    testrun(!ov_id_valid(ids[0]));
    testrun(ov_id_match(ids[1], "b222"));
    testrun(ov_id_match(ids[2], "c333333"));

    for (size_t i = 0; i < capacity; ++i) {
        fprintf(stderr, "ID %s\n", ids[i]);
    }

    testrun(ov_id_array_reset(ids, capacity));

    testrun(!ov_id_valid(ids[0]));
    testrun(!ov_id_valid(ids[1]));
    testrun(!ov_id_valid(ids[2]));

    for (size_t i = 0; i < capacity; ++i) {
        fprintf(stderr, "ID %s\n", ids[i]);
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_id_array_add() {
    // bool ov_id_array_add(ov_id ids[], size_t capacity, const ov_id id);

    ov_id ids[3] = {0};

    testrun(!ov_id_array_add(0, 0, 0));
    testrun(!ov_id_array_add(ids, 0, 0));
    testrun(!ov_id_array_add(0, 3, 0));
    testrun(!ov_id_array_add(ids, 3, 0));

    testrun(!ov_id_array_add(0, 0, "a1"));
    testrun(!ov_id_array_add(ids, 0, "a1"));
    testrun(!ov_id_array_add(0, 3, "a1"));

    testrun(!ov_id_valid(ids[0]));
    testrun(!ov_id_valid(ids[1]));
    testrun(!ov_id_valid(ids[2]));

    testrun(ov_id_array_add(ids, 3, "a1"));
    testrun(ov_id_match(ids[0], "a1"));
    testrun(!ov_id_valid(ids[1]));
    testrun(!ov_id_valid(ids[2]));

    testrun(ov_id_array_add(ids, 3, "b22"));
    testrun(ov_id_match(ids[0], "a1"));
    testrun(ov_id_match(ids[1], "b22"));
    testrun(!ov_id_valid(ids[2]));

    testrun(!ov_id_array_add(ids, 2, "c33"));
    testrun(ov_id_match(ids[0], "a1"));
    testrun(ov_id_match(ids[1], "b22"));
    testrun(!ov_id_valid(ids[2]));

    testrun(ov_id_array_add(ids, 3, "c333"));
    testrun(ov_id_match(ids[0], "a1"));
    testrun(ov_id_match(ids[1], "b22"));
    testrun(ov_id_match(ids[2], "c333"));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_id_array_del() {
    // bool ov_id_array_del(ov_id ids[], size_t capacity, const ov_id id);

    ov_id ids[] = {"a", "b2", "c33"};

    testrun(!ov_id_array_del(0, 0, 0));
    testrun(!ov_id_array_del(ids, 0, 0));
    testrun(!ov_id_array_del(0, 3, 0));
    testrun(!ov_id_array_del(ids, 0, 0));
    testrun(!ov_id_array_del(ids, 3, 0));
    testrun(!ov_id_array_del(0, 0, "quota"));
    testrun(ov_id_array_del(ids, 0, "quota"));

    testrun(ov_id_match(ids[0], "a"));
    testrun(ov_id_match(ids[1], "b2"));
    testrun(ov_id_match(ids[2], "c33"));

    testrun(!ov_id_array_del(0, 3, "quota"));

    testrun(ov_id_match(ids[0], "a"));
    testrun(ov_id_match(ids[1], "b2"));
    testrun(ov_id_match(ids[2], "c33"));

    testrun(ov_id_array_del(ids, 3, "quota"));

    testrun(ov_id_match(ids[0], "a"));
    testrun(ov_id_match(ids[1], "b2"));
    testrun(ov_id_match(ids[2], "c33"));

    testrun(ov_id_array_del(ids, 3, "c33"));

    testrun(ov_id_match(ids[0], "a"));
    testrun(ov_id_match(ids[1], "b2"));
    testrun(!ov_id_valid(ids[2]));

    testrun(ov_id_array_del(ids, 3, "c33"));

    testrun(ov_id_match(ids[0], "a"));
    testrun(ov_id_match(ids[1], "b2"));
    testrun(!ov_id_valid(ids[2]));

    testrun(ov_id_array_del(ids, 3, "a"));

    testrun(!ov_id_valid(ids[0]));
    testrun(ov_id_match(ids[1], "b2"));
    testrun(!ov_id_valid(ids[2]));

    testrun(ov_id_array_del(ids, 3, "b2"));

    testrun(!ov_id_valid(ids[0]));
    testrun(!ov_id_valid(ids[1]));
    testrun(!ov_id_valid(ids[2]));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_id_array_get_index() {
    // ssize_t ov_id_array_get_index(ov_id const *uuids, size_t capacity, const
    // ov_id id);

    ov_id ids[] = {"a", "22", "ccc"};

    testrun(0 > ov_id_array_get_index(0, 0, 0));
    testrun(0 > ov_id_array_get_index(ids, 0, 0));
    testrun(0 > ov_id_array_get_index(0, 3, 0));
    testrun(0 > ov_id_array_get_index(ids, 3, 0));
    testrun(0 > ov_id_array_get_index(0, 0, "ccc"));
    testrun(0 > ov_id_array_get_index(ids, 0, "ccc"));
    testrun(2 == ov_id_array_get_index(ids, 3, "ccc"));
    testrun(0 > ov_id_array_get_index(ids, 2, "ccc"));
    testrun(0 > ov_id_array_get_index(ids, 3, "dddd"));
    testrun(1 == ov_id_array_get_index(ids, 3, "22"));
    testrun(0 == ov_id_array_get_index(ids, 3, "a"));

    return testrun_log_success();
}
/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_id", test_ov_id_valid, test_ov_id_set,
            test_ov_id_fill_with_uuid, test_ov_id_match, test_ov_id_array_reset,
            test_ov_id_array_add, test_ov_id_array_del,
            test_ov_id_array_get_index);
