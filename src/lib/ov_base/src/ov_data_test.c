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
        @file           ov_data_test.c
        @author         Markus Töpfer

        @date           2020-04-09


        ------------------------------------------------------------------------
*/
#include "ov_data.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_data_init() {

    ov_data source = {0};

    ov_data *data = ov_data_init(&source, sizeof(source));
    testrun(data);
    testrun(data->magic_byte == OV_DATA_MAGIC_BYTE);
    testrun(data->type == 0);
    testrun(data->free == NULL);

    testrun(ov_data_cast(&source));

    /*
     *      Check size < sizeof(ov_data)
     */

    testrun(!ov_data_init(data, sizeof(data)));
    testrun(!ov_data_init(data, sizeof(ov_data) - 1));

    testrun(ov_data_init(data, sizeof(ov_data)));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_data_cast() {

    ov_data source = {0};

    ov_data *data = ov_data_init(&source, sizeof(source));
    testrun(data);
    testrun(data->magic_byte == OV_DATA_MAGIC_BYTE);
    testrun(data->type == 0);
    testrun(data->free == NULL);

    testrun(ov_data_cast(&source));

    for (size_t i = 0; i <= 0xffff; i++) {

        ov_data check = (ov_data){.magic_byte = i};

        if (i != OV_DATA_MAGIC_BYTE) {
            testrun(!ov_data_cast(&check));
        } else {
            testrun(ov_data_cast(&check));
        }
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_data *dummy_free(ov_data *data) {

    if (data) { /* ignore */
    }

    return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_data *dummy_free_with_free(ov_data *data) {

    free(data);
    return NULL;
}

/*----------------------------------------------------------------------------*/

int test_ov_data_free() {

    ov_data source = {0};

    testrun(NULL == ov_data_free(NULL));

    ov_data *data = ov_data_init(&source, sizeof(source));
    testrun(ov_data_cast(&source));

    /*
     *      Check with no free
     */

    testrun(data == ov_data_free((void *)data));

    /*
     *      Check with dummy free
     */

    data->free = dummy_free;
    testrun(NULL == ov_data_free((void *)data));

    ov_data *allocated = calloc(1, sizeof(ov_data));
    data = ov_data_init(allocated, sizeof(ov_data));
    testrun(ov_data_cast(data));
    testrun(data == allocated);
    data->free = dummy_free_with_free;

    testrun(NULL == ov_data_free((void *)data));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

    testrun_init();

    testrun_test(test_ov_data_init);
    testrun_test(test_ov_data_cast);
    testrun_test(test_ov_data_free);

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
