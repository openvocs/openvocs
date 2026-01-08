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

        This file is part of the openvocs project. http://openvocs.org

        ------------------------------------------------------------------------
*//**

        @file           testrun.h
        @author         Markus Toepfer
        @author         Michael Beer
        @date           2017-11-24

        @brief          Simple serial test execution framework.

        NOTE            This framework uses an alternative to assert based
                        testing, which is compatible with parallel
                        test execution of @see testrun2.h. So this header is
                        replacable with testrun2.h for parallel test setup,
                        without replacing any written testcase.

                        (Assert.h is not included, to force to write testrun2.h
                        compatible tests by default)

        ------------------------------------------------------------------------
*/

#ifndef testrun_h
#define testrun_h

#include <errno.h>  /* C89/C90 */
#include <stdio.h>  /* C89/C90 */
#include <stdlib.h> /* C89/C90 */
#include <string.h> /* C89/C90 */
#include <time.h>   /* C89/C90 */
#include <unistd.h> /* C89/C90 */

#include <stdbool.h>

/*----------------------------------------------------------------------------*/

/**
        String error initialization of no error.
*/
#define testrun_errno() (errno == 0 ? "NONE" : strerror(errno))

/*----------------------------------------------------------------------------*/

/**
        Log a failure. Failure: Inability to perform a function as expected.
*/
#define testrun_log_failure(msg, ...)                                          \
    fprintf(stderr, "\t[FAIL]\t%s line:%d errno:%s message: " msg "\n",        \
            __FUNCTION__, __LINE__, testrun_errno(), ##__VA_ARGS__)

/*----------------------------------------------------------------------------*/

/**
        Log an error. Error: Difference between expected and actual result.
*/
#define testrun_log_error(msg, ...)                                            \
    fprintf(stderr, "\t[ERROR]\t%s line:%d errno:%s message: " msg "\n",       \
            __FUNCTION__, __LINE__, testrun_errno(), ##__VA_ARGS__)

/*----------------------------------------------------------------------------*/

#define testrun_log_success(msg, ...)                                          \
    fprintf(stdout, "\t[OK] \t%s " msg "\n", __FUNCTION__, ##__VA_ARGS__)

/*----------------------------------------------------------------------------*/

#define testrun_log(msg, ...) fprintf(stdout, "\t" msg "\n", ##__VA_ARGS__)

/*----------------------------------------------------------------------------*/

#define testrun_log_function_info(msg, ...)                                    \
    fprintf(stdout, "\t[INFO] \t%s line:%d message: " msg "\n", __FUNCTION__,  \
            __LINE__, ##__VA_ARGS__)

/*----------------------------------------------------------------------------*/

#define testrun_log_clock(start, end)                                          \
    fprintf(stdout, "\tClock ticks function: ( %s ) | %f s | %.0f ms \n",      \
            __func__, ((double)(end - start)) / CLOCKS_PER_SEC,                \
            (((double)(end - start)) / CLOCKS_PER_SEC) * 1000)

/*----------------------------------------------------------------------------*/

#define testrun_init()                                                         \
    int result = 0;                                                            \
    int testrun_counter = 0;

/*----------------------------------------------------------------------------*/

/**
        Run a single atomar test. Return the surrounding block on error.
        This function will leave the context block running on error. The
        Mindset is a defused assert. LEAVE THE FUNCTION NOT THE PROGRAM.

        @param test     boolean decision input.
        @returns        the calling function on error with -1
*/
#define testrun_check(test, ...)                                               \
    if (!(test)) {                                                             \
        testrun_log_error(__VA_ARGS__);                                        \
        return -1;                                                             \
    }

/*----------------------------------------------------------------------------*/

/**
        Alias to @see testrun_check.
*/
#define testrun(test, ...) testrun_check(test, __VA_ARGS__)

/*----------------------------------------------------------------------------*/

/*--------------- For EXAMPLE code check http://testrun.info -----------------*/
/**
        Run a single test (execute a function pointer. Runs a test function.
        On non negative return value of the function run, a testrun_counter
        is increased, on negative result, the negative result will be returned.

        @param test     function pointer of the test to run
        @NOTE           The surrounding block is left on negative result of the
                        function pointer execution.
*/
#define testrun_test(test)                                                     \
    result = test();                                                           \
    testrun_counter++;                                                         \
    if (result < 0)                                                            \
        return result;

/**
        Runs a function pointer, which SHALL contain the test function pointers
        to run. The function pointer is wrapped in a main procedure, which and
        allows indepentent testruns of the input testcluster over external
        execution.

        A clock will be started, as soon as the main is executed and the the
        time is stopped again, at the end of the execution. The difference
        will be printed and is the runtime of the whole input testcluster.

        A run will fail, as soon as one of the tests in the testcluster fails.
        (Fail on first) or will run all functions dependent on the testsetup.

        @param testcluster      function pointer to be executed.
*/
#define testrun_run(testcluster)                                               \
    int main(int argc, char *argv[]) {                                         \
        argc = argc;                                                           \
        clock_t start1_t, end1_t;                                              \
        start1_t = clock();                                                    \
        testrun_log("\ntestrun\t%s", argv[0]);                                 \
        int result = testcluster();                                            \
        if (result > 0)                                                        \
            testrun_log("ALL TESTS RUN SUCCESSFUL (%i tests)", result);        \
        end1_t = clock();                                                      \
        testrun_log_clock(start1_t, end1_t);                                   \
        testrun_log("");                                                       \
        result >= 0 ? exit(EXIT_SUCCESS) : exit(EXIT_FAILURE);                 \
    }

#define RUN_TESTS(test_name, ...)                                              \
    int main(int argc, char *argv[]) {                                         \
        size_t tests_run = 0; /** counter for successful tests **/             \
        size_t tests_failed = 0;                                               \
        int result = EXIT_SUCCESS;                                             \
        bool testing = true;                                                   \
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
            if (0 >= (*tests)()) {                                             \
                result = EXIT_FAILURE;                                         \
                ++tests_failed;                                                \
            };                                                                 \
            tests_run++;                                                       \
        }                                                                      \
        end_t = clock();                                                       \
        testrun_log("ALL TESTS RUN - %zu/%zu succeeded",                       \
                    tests_run - tests_failed, tests_run);                      \
        testrun_log_clock(start_t, end_t);                                     \
        if (tests_failed != 0)                                                 \
            exit(EXIT_FAILURE);                                                \
        result = result >= 0 ? EXIT_SUCCESS : EXIT_FAILURE;                    \
        exit(result);                                                          \
    }

/**     -----------------------------------------------------------------------

        @example        testrun_base_example.c
        @author         Markus Toepfer
        @date           2017-11-24

        @brief          Example test file using testrun.h

        This example shows the test style for testing with testrun.h and is
        build around the testrun_test() macro, which increases a counter which
        MUST be initialized in a testcluster function.

        //---------------------------------------------------------------------

        @code
        #include "../tools/testrun.h"

        int example_function() {
                return 1;
        }

        //---------------------------------------------------------------------

        int test_function1() {

                // use of testrun_check() for evaluation
                testrun_check(1 == 1);
                testrun_check(1 == 1, "some additional information");

                // use of testrun() for evaluation
                testrun(1 == 1);
                testrun(1 == 1, "some additional information");

                // use of manual evaluation and logging
                if (1 != example_function()){
                        testrun_log_error("some additional information.");
                        return -1;
                }

                // will not be reached in case of error
                return testrun_log_success();
        }

        //---------------------------------------------------------------------

        int test_function2() {

                testrun_check(1 == 1);
                return testrun_log_success();
        }

        //---------------------------------------------------------------------

        int test_function3() {

                testrun_check(1 == 1);
                return testrun_log_success();
        }

        //---------------------------------------------------------------------

        int testcase_cluster() {

                testrun_init();

                testrun_test(test_function1);
                testrun_test(test_function2);
                testrun_test(test_function3);

                return testrun_counter;

        }

        testrun_run(testcase_cluster);
        @endcode

*/

#endif /* testrun_h */
