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

        @author         Michael J. Beer

        ------------------------------------------------------------------------
*/

#include "ov_error_codes.h"
#include "ov_result.c"

#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

static int test_ov_result_set() {

    ov_result result = {0};

    testrun(!ov_result_set(0, 0, 0));
    testrun(ov_result_set(&result, OV_ERROR_NOERROR, 0));

    testrun(OV_ERROR_NOERROR == result.error_code);
    testrun(0 == result.message);

    testrun(!ov_result_set(0, OV_ERROR_INTERNAL_SERVER, 0));
    testrun(!ov_result_set(&result, OV_ERROR_INTERNAL_SERVER, 0));

    char const *TEST_MSG = "fensalir";

    testrun(ov_result_set(&result, OV_ERROR_INTERNAL_SERVER, TEST_MSG));

    testrun(OV_ERROR_INTERNAL_SERVER == result.error_code);
    testrun(0 != result.message);
    testrun(0 == strcmp(TEST_MSG, result.message));

    testrun(ov_result_set(
        &result, OV_ERROR_ALREADY_ACQUIRED, OV_ERROR_DESC_ALREADY_ACQUIRED));
    testrun(OV_ERROR_ALREADY_ACQUIRED == result.error_code);
    testrun(0 == strcmp(OV_ERROR_DESC_ALREADY_ACQUIRED, result.message));

    ov_result_clear(&result);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_result_get_message() {

    ov_result result = {0};

    testrun(0 != ov_result_get_message(result));

    ov_result_set(&result, OV_ERROR_NOERROR, 0);
    testrun(0 != ov_result_get_message(result));

    char const *MESSAGE = "Geir nu garmr mjoek fyr gnipahellir";

    testrun(ov_result_set(&result, OV_ERROR_INTERNAL_SERVER, MESSAGE));
    testrun(0 == strcmp(MESSAGE, ov_result_get_message(result)));

    ov_result_clear(&result);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_result_clear() {

    testrun(!ov_result_clear(0));

    ov_result result = {0};

    testrun(ov_result_clear(&result));

    testrun(0 == result.error_code);
    testrun(0 == result.message);

    result.message = strdup("himinbjoerg");
    result.error_code = 1243;

    testrun(ov_result_clear(&result));

    testrun(0 == result.error_code);
    testrun(0 == result.message);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_result",
            test_ov_result_set,
            test_ov_result_get_message,
            test_ov_result_clear);
