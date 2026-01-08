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
        @file           ov_cache_test.c

        @date           2018-12-11

        @ingroup        ov_cache

        @brief          Unit tests of ov_cache


        ------------------------------------------------------------------------
*/
#include "ov_cache.c"
#include <ov_test/testrun.h>

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
void *cache_worker(void *arg) {

  OV_ASSERT(0 == pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0));
  OV_ASSERT(0 == pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0));

  ov_cache *cache = arg;

  struct timespec time_to_wait = {.tv_sec = 0, .tv_nsec = 1000};

  while (true) {

    size_t *value = ov_cache_get(cache);
    if (0 != value)
      ov_cache_put(cache, value);

    nanosleep(&time_to_wait, 0);
  }

  return 0;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

int test_ov_cache_extend() {

  /* Should create a cache with default cache size */
  ov_cache *cache = ov_cache_extend(0, 0);
  testrun(0 != cache);

  cache = ov_cache_free(cache, 0, 0);
  testrun(0 == cache);

  cache = ov_cache_extend(0, 1);
  testrun(0 != cache);

  int testvals[] = {9, 8, 7, 6, 5, 4, 3, 2, 1};

  const size_t num_testvals = sizeof(testvals) / sizeof(testvals[0]);

  testrun(0 == ov_cache_put(cache, testvals + 0));
  for (size_t i = 1; i < num_testvals; ++i) {
    testrun(testvals + i == ov_cache_put(cache, testvals + i));
  }

  cache = ov_cache_extend(cache, 1);
  testrun(0 != cache);

  /* Total capacity: 2 */

  testrun(0 == ov_cache_put(cache, testvals + 1));

  for (size_t i = 0; i < num_testvals; ++i) {
    testrun(testvals + i == ov_cache_put(cache, testvals + i));
  }

  testrun(0 != ov_cache_get(cache));
  testrun(0 != ov_cache_get(cache));

  testrun(0 == ov_cache_get(cache));

  cache = ov_cache_extend(cache, num_testvals);
  testrun(0 != cache);

  for (size_t i = 0; i < num_testvals; ++i) {
    testrun(0 == ov_cache_put(cache, testvals + i));
  }

  for (size_t i = 0; i < num_testvals; ++i) {
    testrun(0 != ov_cache_get(cache));
  }

  testrun(0 == ov_cache_get(cache));

  /* Lastly: Check whether elements are moved into new cache upon extension */

  for (size_t i = 0; i < num_testvals; ++i) {
    testrun(0 == ov_cache_put(cache, testvals + i));
  }

  cache = ov_cache_extend(cache, 12);

  for (size_t i = 0; i < num_testvals; ++i) {
    testrun(0 != ov_cache_get(cache));
  }

  testrun(0 == ov_cache_get(cache));

  cache = ov_cache_free(cache, 0, 0);

  testrun(0 == cache);

  return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

static void *dummy_free(void *item) {

  if (item)
    free(item);
  return NULL;
}

/*---------------------------------------------------------------------------*/

int test_ov_cache_free() {

  testrun(0 == ov_cache_free(0, 0, NULL));

  ov_cache *cache = ov_cache_extend(0, 0);
  cache = ov_cache_free(cache, 0, NULL);
  testrun(0 == cache);

  cache = ov_cache_extend(0, 11);

  cache = ov_cache_free(cache, 0, NULL);
  testrun(0 == cache);

  cache = ov_cache_extend(0, 0);

  /* Forcibly mark the cache as in use */

  atomic_flag_test_and_set(&cache->in_use);

  /* Should fail after 1 sec because in use */
  cache = ov_cache_free(cache, 1, NULL);
  testrun(0 != cache);

  atomic_flag_clear(&cache->in_use);
  /* Should fail after 1 sec because in use */
  cache = ov_cache_free(cache, 1, NULL);
  testrun(0 == cache);

  // use item free

  cache = ov_cache_extend(0, 11);

  char *item = NULL;

  for (size_t i = 0; i < 10; i++) {

    item = calloc(10, sizeof(char));
    testrun(NULL == ov_cache_put(cache, item));
  }

  cache = ov_cache_free(cache, 0, dummy_free);
  testrun(0 == cache);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cache_get() {

  /* Content does not matter - all we want is addresses */
  size_t testvalues[10] = {0};

  const size_t num_testvalues = sizeof(testvalues) / sizeof(testvalues[0]);

  testrun(0 == ov_cache_get(0));

  ov_cache *cache = ov_cache_extend(0, 0);

  testrun(0 == ov_cache_get(cache));

  testrun(0 == ov_cache_put(cache, &testvalues[0]));
  testrun(&testvalues[0] == ov_cache_get(cache));

  for (size_t i = 0; num_testvalues > i; ++i) {

    testrun(0 == ov_cache_put(cache, testvalues + i));
  }

  for (size_t i = 0; num_testvalues > i; ++i) {

    void *object = ov_cache_get(cache);
    testrun((void *)testvalues <= object);
    testrun((void *)(testvalues + num_testvalues) > object);
  }

  testrun(0 == ov_cache_get(cache));

  cache = ov_cache_free(cache, 0, NULL);
  testrun(0 == cache);

  /**********************************************************************
              The same gain, but with finite capacity
   **********************************************************************/

  cache = ov_cache_extend(0, num_testvalues - 1);

  testrun(0 == ov_cache_get(cache));

  testrun(0 == ov_cache_put(cache, &testvalues[0]));
  testrun(&testvalues[0] == ov_cache_get(cache));

  for (size_t i = 0; num_testvalues - 1 > i; ++i) {

    testrun(0 == ov_cache_put(cache, testvalues + i));
  }

  for (size_t i = 0; num_testvalues - 1 > i; ++i) {

    void *object = ov_cache_get(cache);
    testrun((void *)testvalues <= object);
    testrun((void *)(testvalues + num_testvalues - 1) > object);
  }

  testrun(0 == ov_cache_get(cache));

  for (size_t i = 0; num_testvalues - 1 > i; ++i) {

    testrun(0 == ov_cache_put(cache, testvalues + i));
  }

  cache = ov_cache_free(cache, 0, NULL);
  testrun(0 == cache);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cache_put() {

  /* Tests done by test_ov_cache_get */
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_concurrent_access() {

  /* Content does not matter - all we want is addresses */
  size_t testvalues[10] = {0};

  const size_t num_testvalues = sizeof(testvalues) / sizeof(testvalues[0]);

  testrun(0 == ov_cache_get(0));

  ov_cache *cache = ov_cache_extend(0, 0);

  for (size_t i = 0; num_testvalues > i; ++i) {

    testrun(0 == ov_cache_put(cache, testvalues + i));
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

  testrun(0 == ov_cache_free(cache, 0, NULL));
  cache = 0;

  return testrun_log_success();
}

/******************************************************************************
 *                        ov_cache_set_element_checker
 ******************************************************************************/

bool is_23(void *element) {

  OV_ASSERT(0 != element);

  int *ipointer = element;

  return 23 == *ipointer;
}

/*----------------------------------------------------------------------------*/

int test_ov_cache_set_element_checker() {

  ov_cache *cache = ov_cache_extend(0, 0);
  testrun(0 != cache);

  /* Verify it works without element checker */
  int not_correct = 17;
  int correct = 23;

  testrun(0 == ov_cache_put(cache, &not_correct));
  testrun(0 == ov_cache_put(cache, &correct));

  cache = ov_cache_free(cache, 0, 0);
  testrun(0 == cache);

  cache = ov_cache_extend(0, 0);
  testrun(0 != cache);

  ov_cache_set_element_checker(cache, is_23);

  testrun(&not_correct == ov_cache_put(cache, &not_correct));
  testrun(0 == ov_cache_put(cache, &correct));

  testrun(ov_cache_put(cache, &not_correct));

  cache = ov_cache_free(cache, 0, 0);
  testrun(0 == cache);

  return testrun_log_success();
}

#endif

/*----------------------------------------------------------------------------*/

int check_cache_disabled() {

  ov_cache cache = {0};

  char *test = "string";

  testrun(!ov_cache_extend(0, 0));
  testrun(!ov_cache_free(&cache, 0, NULL));
  testrun(!ov_cache_get(&cache));
  testrun(!ov_cache_put(&cache, test));

  return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

  testrun_init();

#ifndef OV_DISABLE_CACHING

  testrun_test(test_ov_cache_extend);
  testrun_test(test_ov_cache_free);
  testrun_test(test_ov_cache_get);
  testrun_test(test_ov_cache_put);
  testrun_test(test_ov_cache_set_element_checker);

  testrun_test(check_concurrent_access);

#else
  testrun_test(check_cache_disabled);
#endif

  return testrun_counter;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST EXECUTION                                                  #EXEC
 *
 *      ------------------------------------------------------------------------
 */

testrun_run(all_tests);
