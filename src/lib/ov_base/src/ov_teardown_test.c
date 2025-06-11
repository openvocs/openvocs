/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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

#include "ov_teardown.c"

/*----------------------------------------------------------------------------*/

static size_t counter_1 = 0;
static bool counter_1_first = false;

static size_t counter_2 = 0;
static bool counter_2_first = false;

/*----------------------------------------------------------------------------*/

static void count_1(void) {

    ++counter_1;
    counter_1_first = counter_1_first || (0 == counter_2);
}

/*----------------------------------------------------------------------------*/

static size_t get_reset_1() {

    size_t c = counter_1;
    counter_1 = 0;
    counter_1_first = false;
    return c;
}

/*----------------------------------------------------------------------------*/

static void count_2(void) {

    ++counter_2;
    counter_2_first = counter_2_first || (0 == counter_1);
}

/*----------------------------------------------------------------------------*/

static size_t get_reset_2() {

    size_t c = counter_2;
    counter_2 = 0;
    counter_2_first = false;
    return c;
}

/*----------------------------------------------------------------------------*/

int ov_teardown_register_test() {

    /*                                 1                                      */
    testrun(!ov_teardown_register(0, 0));

    /*                                 2                                      */
    testrun(ov_teardown_register(count_1, 0));

    ov_teardown();

    testrun(counter_1_first);
    testrun(1 == get_reset_1());

    ov_teardown();
    testrun(0 == get_reset_1());

    /*                                 3                                      */

    testrun(ov_teardown_register(count_1, 0));
    testrun(ov_teardown_register(count_1, 0));

    ov_teardown();

    testrun(counter_1_first);
    testrun(1 == get_reset_1());

    ov_teardown();
    testrun(0 == get_reset_1());
    testrun(0 == get_reset_2());

    /*                                 4                                      */

    testrun(ov_teardown_register(count_2, 0));
    testrun(ov_teardown_register(count_1, 0));
    testrun(ov_teardown_register(count_1, 0));

    ov_teardown();

    testrun(!counter_2_first);
    testrun(counter_1_first);
    testrun(1 == get_reset_1());
    testrun(1 == get_reset_2());

    ov_teardown();
    testrun(0 == get_reset_1());
    testrun(0 == get_reset_2());

    /*                                 5                                      */

    testrun(ov_teardown_register(count_1, 0));
    testrun(ov_teardown_register(count_2, 0));
    testrun(ov_teardown_register(count_1, 0));
    testrun(ov_teardown_register(count_1, 0));
    testrun(ov_teardown_register(count_1, 0));
    testrun(ov_teardown_register(count_2, 0));

    ov_teardown();

    testrun(!counter_1_first);
    testrun(counter_2_first);
    testrun(1 == get_reset_1());
    testrun(1 == get_reset_2());

    ov_teardown();
    testrun(0 == get_reset_1());
    testrun(0 == get_reset_2());

    /*                                 6                                      */

    testrun(ov_teardown_register(count_1, "count_1"));
    testrun(ov_teardown_register(count_2, "count_2"));
    testrun(ov_teardown_register(count_1, "count_1"));
    testrun(ov_teardown_register(count_1, "count_1"));
    testrun(ov_teardown_register(count_1, "count_1"));
    testrun(ov_teardown_register(count_2, "count_2"));
    testrun(ov_teardown_register(count_1, "count_1"));

    ov_teardown();

    testrun(!counter_1_first);
    testrun(counter_2_first);
    testrun(1 == get_reset_1());
    testrun(1 == get_reset_2());

    ov_teardown();
    testrun(0 == get_reset_1());
    testrun(0 == get_reset_2());

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int ov_teardown_register_multiple_test() {

    /*                                 1                                      */
    testrun(!ov_teardown_register_multiple(0, 0));

    /*                                 2                                      */
    testrun(ov_teardown_register_multiple(count_1, 0));

    ov_teardown();

    testrun(counter_1_first);
    testrun(1 == get_reset_1());

    ov_teardown();
    testrun(0 == get_reset_1());

    /*                                 3                                      */

    testrun(ov_teardown_register_multiple(count_1, 0));
    testrun(ov_teardown_register_multiple(count_1, 0));

    ov_teardown();

    testrun(counter_1_first);
    testrun(2 == get_reset_1());

    ov_teardown();
    testrun(0 == get_reset_1());
    testrun(0 == get_reset_2());

    /*                                 4                                      */

    testrun(ov_teardown_register_multiple(count_2, 0));
    testrun(ov_teardown_register_multiple(count_1, 0));
    testrun(ov_teardown_register_multiple(count_1, 0));

    ov_teardown();

    testrun(!counter_2_first);
    testrun(counter_1_first);
    testrun(2 == get_reset_1());
    testrun(1 == get_reset_2());

    ov_teardown();
    testrun(0 == get_reset_1());
    testrun(0 == get_reset_2());

    /*                                 5                                      */

    testrun(ov_teardown_register_multiple(count_1, 0));
    testrun(ov_teardown_register_multiple(count_2, 0));
    testrun(ov_teardown_register_multiple(count_1, 0));
    testrun(ov_teardown_register_multiple(count_1, 0));
    testrun(ov_teardown_register_multiple(count_1, 0));
    testrun(ov_teardown_register_multiple(count_2, 0));

    ov_teardown();

    testrun(!counter_1_first);
    testrun(counter_2_first);
    testrun(4 == get_reset_1());
    testrun(2 == get_reset_2());

    ov_teardown();
    testrun(0 == get_reset_1());
    testrun(0 == get_reset_2());

    /*                                 6                                      */

    testrun(ov_teardown_register_multiple(count_1, "count_1"));
    testrun(ov_teardown_register_multiple(count_2, "count_2"));
    testrun(ov_teardown_register_multiple(count_1, "count_1"));
    testrun(ov_teardown_register_multiple(count_1, "count_1"));
    testrun(ov_teardown_register_multiple(count_1, "count_1"));
    testrun(ov_teardown_register_multiple(count_2, "count_2"));
    testrun(ov_teardown_register_multiple(count_1, "count_1"));

    ov_teardown();

    testrun(counter_1_first);
    testrun(!counter_2_first);
    testrun(5 == get_reset_1());
    testrun(2 == get_reset_2());

    ov_teardown();
    testrun(0 == get_reset_1());
    testrun(0 == get_reset_2());

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

bool ov_teardown();

int ov_teardown_test() { return testrun_log_success(); }

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_teardown",
            ov_teardown_register_test,
            ov_teardown_register_multiple_test,
            ov_teardown_test);
