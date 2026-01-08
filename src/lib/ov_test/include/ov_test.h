/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

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
        @copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

        Slight enhancements over testrun.h

        Fully compatible with testrun.h

        ------------------------------------------------------------------------
*/
#ifndef OV_TEST_H
#define OV_TEST_H
/*----------------------------------------------------------------------------*/

#include "testrun.h"

#include <ov_arch/ov_arch_compile.h>

/*----------------------------------------------------------------------------*/

char *ov_test_get_resource_path(char const *rel_resource_path);

/*----------------------------------------------------------------------------*/

/**
 * Set an exit hook to be called at the very end of the test process
 */
void ov_test_set_exit_hook(void (*hook)());

/*----------------------------------------------------------------------------*/

#define TEST_ASSERT(x) TEST_ASSERT_INTERNAL(x)

/*----------------------------------------------------------------------------*/

/**
 * Set method to handle signals in tests
 */

#define SET_SIGNAL_HANDLING_METHOD ov_test_exit_on_signal
// #define SET_SIGNAL_HANDLING_METHOD ov_test_ignore_signals

/*----------------------------------------------------------------------------*/

/**
 * Declare a test as (temporarily) disabled.
 * Use like
 *
 * testrun_disabled(test_my_fancy_func) {
 *
 *     ...
 *
 *     return testrun_log_success();
 *
 * }
 */

#define ov_test_disabled(test_func)                                            \
    OV_ARCH_COMPILE_ATTRIBUTE_WARNING("Test " #test_func " disabled")          \
    int test_func() {                                                          \
        testrun_log_function_info("Test " #test_func " Disabled");             \
        return testrun_log_success();                                          \
    }                                                                          \
    static int test_func##_disabled()

/*----------------------------------------------------------------------------*/

#define OV_TEST_RUN(test_name, ...)                                            \
    int main(int argc, char *argv[]) {                                         \
        srandom(time(0));                                                      \
        SET_SIGNAL_HANDLING_METHOD();                                          \
        size_t tests_run = 0; /** counter for successful tests **/             \
        size_t tests_failed = 0;                                               \
        int result = EXIT_SUCCESS;                                             \
        ov_test_set_test_directory(argv[0]);                                   \
        argc = argc;                                                           \
        argc = 1;                                                              \
        clock_t start_t, end_t;                                                \
        start_t = clock();                                                     \
        testrun_log("............................................");           \
        testrun_log("RUNNING\t%s", argv[0]);                                   \
        int (**tests)() = (int (*[])()){__VA_ARGS__, 0};                       \
        for (; 0 != *tests; ++tests) {                                         \
            int test_result = (*tests)();                                      \
            tests_run++;                                                       \
            if (0 >= test_result) {                                            \
                result = EXIT_FAILURE;                                         \
                ++tests_failed;                                                \
                break;                                                         \
            };                                                                 \
        }                                                                      \
        end_t = clock();                                                       \
        fprintf(stderr, "ALL TESTS RUN - %zu/%zu succeeded\n",                 \
                tests_run - tests_failed, tests_run);                          \
        testrun_log_clock(start_t, end_t);                                     \
        void (*exit_hook)(void) = ov_test_get_exit_hook();                     \
        if (0 != exit_hook) {                                                  \
            exit_hook();                                                       \
        }                                                                      \
        if (tests_failed != 0)                                                 \
            exit(EXIT_FAILURE);                                                \
        ov_test_clear_test_directory();                                        \
        result = result >= 0 ? EXIT_SUCCESS : EXIT_FAILURE;                    \
        exit(result);                                                          \
    }

/**
 * Alternative to OV_RUN_TESTS for global initialization / tear down.
 *
 * Use like
 *
 * int main(int argc, char ** argv) {
 *
 *     ov_test_set_exit_hook(ov_registered_cache_free_all);
 *
 *     OV_TEST_EXECUTE_TESTS("ov_rtp_frame",
 *                           test_ov_rtp_frame_enable_caching,
 *                           test_ov_rtp_frame_encode,
 *                           test_ov_rtp_frame_decode,
 *                           test_ov_rtp_frame_dump,
 *                           test_rtp_frame_create,
 *                           test_impl_rtp_frame_free,
 *                           test_impl_rtp_frame_copy);
 * }
 *
 */
#define OV_TEST_EXECUTE_TESTS(test_name, ...)                                  \
    do {                                                                       \
        SET_SIGNAL_HANDLING_METHOD();                                          \
        size_t tests_run = 0; /** counter for successful tests **/             \
        size_t tests_failed = 0;                                               \
        int result = EXIT_SUCCESS;                                             \
        bool testing = true;                                                   \
        ov_test_set_test_directory(argv[0]);                                   \
        argc = argc;                                                           \
        argc = 1;                                                              \
        clock_t start_t, end_t;                                                \
        start_t = clock();                                                     \
        testrun_log("............................................");           \
        testrun_log("RUNNING\t%s", argv[0]);                                   \
        if (!testing) {                                                        \
            fprintf(stdout, "TESTING SWITCHED OFF for " test_name);            \
            exit(EXIT_SUCCESS);                                                \
        }                                                                      \
        int (**tests)() = (int (*[])()){__VA_ARGS__, 0};                       \
        for (; 0 != *tests; ++tests) {                                         \
            tests_run++;                                                       \
            if (0 >= (*tests)()) {                                             \
                result = EXIT_FAILURE;                                         \
                ++tests_failed;                                                \
                break;                                                         \
            };                                                                 \
        }                                                                      \
        end_t = clock();                                                       \
        fprintf(stderr, "ALL TESTS RUN - %zu/%zu succeeded\n",                 \
                tests_run - tests_failed, tests_run);                          \
        testrun_log_clock(start_t, end_t);                                     \
        if (tests_failed != 0)                                                 \
            exit(EXIT_FAILURE);                                                \
        ov_test_clear_test_directory();                                        \
        void (*exit_hook)(void) = ov_test_get_exit_hook();                     \
        if (0 != exit_hook) {                                                  \
            exit_hook();                                                       \
        }                                                                      \
        result = result >= 0 ? EXIT_SUCCESS : EXIT_FAILURE;                    \
        exit(result);                                                          \
    } while (0)

/*****************************************************************************
                            FOR INTERNAL USAGE ONLY
 ****************************************************************************/

void ov_test_set_test_directory(char const *binary_path);
void ov_test_clear_test_directory();
void (*ov_test_get_exit_hook())(void);

void ov_test_exit_on_signal();
void ov_test_ignore_signals();

/*----------------------------------------------------------------------------*/

#undef assert
#define assert(x) _Static_assert(0, "Trying to use 'assert'")

#ifdef DEBUG

#define TEST_ASSERT_TO_STR_HELPER_INTERNAL(x) #x
#define TEST_ASSERT_TO_STR_INTERNAL(x) TEST_ASSERT_TO_STR_HELPER_INTERNAL(x)

// Only reproduce the behaviour of `assert`
#define TEST_ASSERT_INTERNAL(x)                                                \
    do {                                                                       \
        bool cond = (x);                                                       \
        if (!cond) {                                                           \
            fprintf(stderr, "assertion failed: %s:%i (%s): %s\n", __FILE__,    \
                    __LINE__, __FUNCTION__, TEST_ASSERT_TO_STR_INTERNAL(x));   \
            abort();                                                           \
        }                                                                      \
    } while (0)

#else

#define TEST_ASSERT_INTERNAL(x)

#endif

#endif
