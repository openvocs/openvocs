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
        @file           ov_registered_cache_test.c
        @author         Michael Beer
        @author         Markus Toepfer

        ------------------------------------------------------------------------
*/
#include "ov_registered_cache.c"
#include <ov_test/ov_test.h>

#ifndef OV_DISABLE_CACHING

#include <pthread.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST HELPER
 *
 *      ------------------------------------------------------------------------
 */

/* To be used to concurrently modify a cache */
static void *cache_worker(void *arg) {

    OV_ASSERT(0 == pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0));
    OV_ASSERT(0 == pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0));

    ov_registered_cache *cache = arg;

    struct timespec time_to_wait = {.tv_sec = 0, .tv_nsec = 1000};

    while (true) {

        size_t *value = ov_registered_cache_get(cache);
        if (0 != value) ov_registered_cache_put(cache, value);

        nanosleep(&time_to_wait, 0);
    }

    return 0;
}

/*----------------------------------------------------------------------------*/

static int test_ov_registered_cache_get() {

    ov_registered_cache_config cfg = {0};

    /* Content does not matter - all we want is addresses */
    size_t testvalues[10] = {0};

    const size_t num_testvalues = sizeof(testvalues) / sizeof(testvalues[0]);

    testrun(0 == ov_registered_cache_get(0));

    ov_registered_cache *cache = ov_registered_cache_extend("get", cfg);

    testrun(0 == ov_registered_cache_get(cache));

    testrun(0 == ov_registered_cache_put(cache, &testvalues[0]));
    testrun(&testvalues[0] == ov_registered_cache_get(cache));

    for (size_t i = 0; num_testvalues > i; ++i) {

        testrun(0 == ov_registered_cache_put(cache, testvalues + i));
    }

    for (size_t i = 0; num_testvalues > i; ++i) {

        void *object = ov_registered_cache_get(cache);
        testrun((void *)testvalues <= object);
        testrun((void *)(testvalues + num_testvalues) > object);
    }

    testrun(0 == ov_registered_cache_get(cache));

    /**********************************************************************
                The same gain, but with finite capacity
     **********************************************************************/

    cfg.capacity = num_testvalues - 1;

    cache = ov_registered_cache_extend("get_2", cfg);

    testrun(0 == ov_registered_cache_get(cache));

    testrun(0 == ov_registered_cache_put(cache, &testvalues[0]));
    testrun(&testvalues[0] == ov_registered_cache_get(cache));

    for (size_t i = 0; num_testvalues - 1 > i; ++i) {

        testrun(0 == ov_registered_cache_put(cache, testvalues + i));
    }

    for (size_t i = 0; num_testvalues - 1 > i; ++i) {

        void *object = ov_registered_cache_get(cache);
        testrun((void *)testvalues <= object);
        testrun((void *)(testvalues + num_testvalues - 1) > object);
    }

    testrun(0 == ov_registered_cache_get(cache));

    for (size_t i = 0; num_testvalues - 1 > i; ++i) {

        testrun(0 == ov_registered_cache_put(cache, testvalues + i));
    }

    ov_registered_cache_free_all();

    cache = 0;

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_registered_cache_put() {

    /* Tests done by test_ov_registered_cache_get */
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int check_concurrent_access() {

    ov_registered_cache_config cfg = {0};

    /* Content does not matter - all we want is addresses */
    size_t testvalues[10] = {0};

    const size_t num_testvalues = sizeof(testvalues) / sizeof(testvalues[0]);

    testrun(0 == ov_registered_cache_get(0));

    ov_registered_cache *cache =
        ov_registered_cache_extend("multithreaded", cfg);

    for (size_t i = 0; num_testvalues > i; ++i) {

        testrun(0 == ov_registered_cache_put(cache, testvalues + i));
    }

    pthread_t threads[5] = {0};

    const size_t num_threads = sizeof(threads) / sizeof(threads[0]);

    for (size_t i = 0; num_threads > i; ++i) {
        testrun(0 == pthread_create(threads + i, 0, cache_worker, cache));
    }

    struct timespec time_to_wait = {
        .tv_sec = 1,
    };

    testrun(0 == nanosleep(&time_to_wait, 0));

    for (size_t i = 0; num_threads > i; ++i) {

        testrun(0 == pthread_cancel(threads[i]));
    }

    void *thread_retval;

    for (size_t i = 0; num_threads > i; ++i) {

        testrun(0 == pthread_join(threads[i], &thread_retval));
    }

    ov_registered_cache_free_all();
    cache = 0;

    return testrun_log_success();
}

/******************************************************************************
 *                        ov_registered_cache_set_element_checker
 ******************************************************************************/

static bool is_23(void *element) {

    OV_ASSERT(0 != element);

    int *ipointer = element;

    return 23 == *ipointer;
}

/*----------------------------------------------------------------------------*/

static int test_ov_registered_cache_set_element_checker() {

    ov_registered_cache_config cfg = {0};

    ov_registered_cache *cache = ov_registered_cache_extend("checker", cfg);
    testrun(0 != cache);

    /* Verify it works without element checker */
    int not_correct = 17;
    int correct = 23;

    testrun(0 == ov_registered_cache_put(cache, &not_correct));
    testrun(0 == ov_registered_cache_put(cache, &correct));

    cache = ov_registered_cache_extend("checker_2", cfg);
    testrun(0 != cache);

    ov_registered_cache_set_element_checker(cache, is_23);

    // this test triggers OV_ASSERT
    // testrun(&not_correct == ov_registered_cache_put(cache, &not_correct));
    testrun(0 == ov_registered_cache_put(cache, &correct));

    // this test triggers OV_ASSERT
    // testrun(ov_registered_cache_put(cache, &not_correct));

    ov_registered_cache_free_all();

    cache = 0;

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static void *string_free(void *str) {

    if (0 != str) {

        free(str);
        str = 0;
    }

    return str;
}

/*----------------------------------------------------------------------------*/

static int test_ov_registered_cache_register_extend() {

#define TEST_NAME "test_cache"

    ov_registered_cache_config cfg = {0};

    /* Should create a non-registered cache with default cache size */
    ov_registered_cache *cache = ov_registered_cache_extend(0, cfg);
    testrun(0 == cache);

    /* Create a registered cache with default capacity */
    cache = ov_registered_cache_extend(TEST_NAME, cfg);
    testrun(0 != cache);

    ov_registered_cache_free_all();

    /* Create a registered cache with capacity 1 */
    cfg.capacity = 1;

    cache = ov_registered_cache_extend(TEST_NAME, cfg);
    testrun(0 != cache);

    /* Extending cache with different item_free should fail */
    void *(*old_free)(void *) = cfg.item_free;

    cfg.item_free = string_free;
    testrun(0 == ov_registered_cache_extend(TEST_NAME, cfg));

    cfg.item_free = old_free;

    int testvals[] = {9, 8, 7, 6, 5, 4, 3, 2, 1};

    const size_t num_testvals = sizeof(testvals) / sizeof(testvals[0]);

    testrun(0 == ov_registered_cache_put(cache, testvals + 0));

    for (size_t i = 1; i < num_testvals; ++i) {
        testrun(testvals + i == ov_registered_cache_put(cache, testvals + i));
    }

    cfg.capacity = 1;
    testrun(cache == ov_registered_cache_extend(TEST_NAME, cfg));

    /* Total capacity: 2 */

    testrun(0 == ov_registered_cache_put(cache, testvals + 1));

    for (size_t i = 0; i < num_testvals; ++i) {
        testrun(testvals + i == ov_registered_cache_put(cache, testvals + i));
    }

    testrun(0 != ov_registered_cache_get(cache));
    testrun(0 != ov_registered_cache_get(cache));

    testrun(0 == ov_registered_cache_get(cache));

    cfg.capacity = num_testvals;
    cache = ov_registered_cache_extend(TEST_NAME, cfg);
    testrun(0 != cache);

    for (size_t i = 0; i < num_testvals; ++i) {
        testrun(0 == ov_registered_cache_put(cache, testvals + i));
    }

    for (size_t i = 0; i < num_testvals; ++i) {
        testrun(0 != ov_registered_cache_get(cache));
    }

    testrun(0 == ov_registered_cache_get(cache));

    /* Lastly: Check whether elements are moved into new cache upon extension */

    for (size_t i = 0; i < num_testvals; ++i) {
        testrun(0 == ov_registered_cache_put(cache, testvals + i));
    }

    cfg.capacity = 12;
    cache = ov_registered_cache_extend(TEST_NAME, cfg);

    for (size_t i = 0; i < num_testvals; ++i) {
        testrun(0 != ov_registered_cache_get(cache));
    }

    testrun(0 == ov_registered_cache_get(cache));

    ov_registered_cache_free_all();
    cache = 0;

    /* Check excess elements properly freed */
    cfg.item_free = string_free;
    cfg.capacity = 12;

    cache = ov_registered_cache_extend(TEST_NAME "_string", cfg);

    for (size_t i = 0; i < 12; ++i) {

        char entry[] = "XXXXXXXXXXXXXXXXXXXX";
        snprintf(entry, sizeof(entry), "entry-%zu", i);
        testrun(0 == ov_registered_cache_put(cache, strdup(entry)));
    }

    ov_registered_cache_free_all();

    cache = 0;

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_registered_cache_free_all() {

    /* Dummy, there is not really much to test */

    ov_registered_cache_free_all();

    return testrun_log_success();
}

/*****************************************************************************
                                 CONFIGURATION
 ****************************************************************************/

static ov_json_value *json_from_string(char const *str) {

    OV_ASSERT(0 != str);
    return ov_json_value_from_string(str, strlen(str));
}

/*----------------------------------------------------------------------------*/

static ov_registered_cache_sizes *sizes_from_string(char const *str) {

    ov_json_value *jval = json_from_string(str);
    OV_ASSERT(0 != jval);
    ov_registered_cache_sizes *sizes =
        ov_registered_cache_sizes_from_json(jval);

    jval = ov_json_value_free(jval);
    OV_ASSERT(0 == jval);

    return sizes;
}

/*----------------------------------------------------------------------------*/

static int test_ov_registered_cache_sizes_free() {

    testrun(0 == ov_registered_cache_sizes_free(0));

    ov_registered_cache_sizes *cfg = sizes_from_string("{}");
    testrun(0 == ov_registered_cache_sizes_free(cfg));

    // pointer is freed above
    // cfg = ov_registered_cache_sizes_free(cfg);
    // testrun(0 == cfg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_registered_cache_sizes_from_json() {

#define CACHE_NAME_1 "draupnir"
#define CACHE_NAME_2 "gullinborsti"

    testrun(!ov_registered_cache_sizes_from_json(0));

    ov_json_value *jval = json_from_string("{}");
    testrun(0 != jval);

    ov_registered_cache_sizes *sizes =
        ov_registered_cache_sizes_from_json(jval);
    testrun(0 != sizes);

    jval = ov_json_value_free(jval);
    testrun(0 == jval);

    testrun(OV_DEFAULT_CACHE_SIZE ==
            ov_registered_cache_size_for(sizes, CACHE_NAME_1));

    testrun(OV_DEFAULT_CACHE_SIZE ==
            ov_registered_cache_size_for(sizes, CACHE_NAME_2));

    sizes = ov_registered_cache_sizes_free(sizes);
    testrun(0 == sizes);

    // CACHING DISABLED

    jval = json_from_string("{\"" OV_KEY_CACHE_ENABLED "\": false}");
    testrun(0 != jval);

    sizes = ov_registered_cache_sizes_from_json(jval);
    testrun(0 != sizes);

    jval = ov_json_value_free(jval);
    testrun(0 == jval);

    testrun(0 == ov_registered_cache_size_for(sizes, CACHE_NAME_1));
    testrun(0 == ov_registered_cache_size_for(sizes, CACHE_NAME_2));

    sizes = ov_registered_cache_sizes_free(sizes);
    testrun(0 == sizes);

    // CACHE ENABLED - No explicit sizes

    jval = json_from_string("{\"" OV_KEY_CACHE_ENABLED "\": true}");
    testrun(0 != jval);

    sizes = ov_registered_cache_sizes_from_json(jval);
    testrun(0 != sizes);

    jval = ov_json_value_free(jval);
    testrun(0 == jval);

    testrun(OV_DEFAULT_CACHE_SIZE ==
            ov_registered_cache_size_for(sizes, CACHE_NAME_1));

    testrun(OV_DEFAULT_CACHE_SIZE ==
            ov_registered_cache_size_for(sizes, CACHE_NAME_2));

    sizes = ov_registered_cache_sizes_free(sizes);
    testrun(0 == sizes);

    /* Case 2: Empty sizes section */

    jval = json_from_string("{\"" OV_KEY_CACHE_ENABLED
                            "\": true,"
                            "\"" OV_KEY_CACHE_SIZES "\": {}}");
    testrun(0 != jval);

    sizes = ov_registered_cache_sizes_from_json(jval);
    testrun(0 != sizes);

    jval = ov_json_value_free(jval);
    testrun(0 == jval);

    testrun(OV_DEFAULT_CACHE_SIZE ==
            ov_registered_cache_size_for(sizes, CACHE_NAME_1));

    testrun(OV_DEFAULT_CACHE_SIZE ==
            ov_registered_cache_size_for(sizes, CACHE_NAME_2));

    sizes = ov_registered_cache_sizes_free(sizes);
    testrun(0 == sizes);

    /* Enabled with cache sizes section */

    jval = json_from_string("{\"" OV_KEY_CACHE_ENABLED
                            "\": true,"
                            "\"" OV_KEY_CACHE_SIZES
                            "\": {"
                            "\"" CACHE_NAME_1
                            "\":1,"
                            "\"" CACHE_NAME_2
                            "\":3"
                            "}}");
    testrun(0 != jval);

    sizes = ov_registered_cache_sizes_from_json(jval);
    testrun(0 != sizes);

    jval = ov_json_value_free(jval);
    testrun(0 == jval);

    testrun(1 == ov_registered_cache_size_for(sizes, CACHE_NAME_1));
    testrun(3 == ov_registered_cache_size_for(sizes, CACHE_NAME_2));

    sizes = ov_registered_cache_sizes_free(sizes);
    testrun(0 == sizes);

    return testrun_log_success();

#undef CACHE_NAME_1
#undef CACHE_NAME_2
}

/*----------------------------------------------------------------------------*/

static int test_ov_registered_cache_size_for() {

#define CACHE_NAME_1 "draupnir"
#define CACHE_NAME_2 "gullinborsti"

    testrun(0 == ov_registered_cache_size_for(0, 0));
    testrun(0 == ov_registered_cache_size_for(0, CACHE_NAME_1));
    testrun(0 == ov_registered_cache_size_for(0, CACHE_NAME_2));

    // Remainder checked in ov_registered_cache_size_from_json

    return testrun_log_success();

#undef CACHE_NAME_1
#undef CACHE_NAME_2
}

/*----------------------------------------------------------------------------*/

static bool check_caches_enabled(ov_hashtable *sizes,
                                 char const **cache_names,
                                 size_t num_names) {

    UNUSED(sizes);

    for (size_t i = 0; i < num_names; ++i) {

        ov_registered_cache *cache =
            ov_hashtable_get(g_registry, cache_names[i]);

        if (0 == cache) {
            testrun_log_error("Expected cache %s not found", cache_names[i]);
            return false;
        }
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static bool check_caches(ov_registered_cache_sizes *sizes) {

    // Tricky: those need to be the names the corr. units use internally to
    // identify their cache
    char const *cache_names[] = {"buffer",
                                 "linked_list",
                                 "ov_value_number",
                                 "ov_value_strings",
                                 "ov_value_lists",
                                 "ov_value_objects"};

    size_t num_cache_names = sizeof(cache_names) / sizeof(cache_names[0]);

    if (sizes->enabled) {
        return check_caches_enabled(sizes->sizes, cache_names, num_cache_names);
    }

    OV_ASSERT(0 == sizes->sizes);

    for (size_t i = 0; i < num_cache_names; ++i) {
        ov_registered_cache *cache =
            ov_hashtable_get(g_registry, cache_names[i]);

        if (0 != cache) {
            return false;
        }
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static int test_ov_registered_cache_sizes_configure() {

    ov_registered_cache_free_all();

    /* All caches enabled - default values */
    ov_registered_cache_sizes *sizes =
        sizes_from_string("{\"" OV_KEY_CACHE_ENABLED "\":true}");
    ov_registered_cache_sizes_configure(sizes);

    testrun(check_caches(sizes));

    ov_registered_cache_free_all();

    sizes = ov_registered_cache_sizes_free(sizes);
    testrun(0 == sizes);
    sizes = sizes_from_string("{\"" OV_KEY_CACHE_ENABLED "\":false}");

    ov_registered_cache_sizes_configure(sizes);

    sizes = ov_registered_cache_sizes_free(sizes);
    testrun(0 == sizes);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("registered_cache",
            test_ov_registered_cache_get,
            test_ov_registered_cache_put,
            test_ov_registered_cache_set_element_checker,

            check_concurrent_access,

            test_ov_registered_cache_register_extend,
            test_ov_registered_cache_free_all,
            test_ov_registered_cache_sizes_free,
            test_ov_registered_cache_sizes_from_json,
            test_ov_registered_cache_size_for,
            test_ov_registered_cache_sizes_configure);

#else

int main(int argc, char **argv) {

    UNUSED(argc);
    UNUSED(argv);

    return 0;
}

#endif
