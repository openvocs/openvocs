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
        @file           ov_vocs_record_test.c
        @author         Markus Töpfer

        @date           2024-01-26


        ------------------------------------------------------------------------
*/
#include "ov_vocs_record.c"
#include <ov_test/testrun.h>

struct dummy_userdata {

    char loopname[OV_MC_LOOP_NAME_MAX];
    bool stop;
};

/*----------------------------------------------------------------------------*/

static void cb_stop(void *userdata, ov_vocs_record *const self) {

    struct dummy_userdata *data = (struct dummy_userdata *)userdata;
    strncpy(data->loopname, self->config.loopname, OV_MC_LOOP_NAME_MAX);
    data->stop = true;
    return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_vocs_record_create() {

    struct dummy_userdata userdata = (struct dummy_userdata){0};

    ov_vocs_record_config config =
        (ov_vocs_record_config){.loopname = "loop1",
                                .callback.userdata = &userdata,
                                .callback.stop = cb_stop};

    ov_vocs_record *rec = ov_vocs_record_create(config);
    testrun(rec);
    testrun(NULL == ov_vocs_record_free_void(rec));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_record_free_void() {

    struct dummy_userdata userdata = (struct dummy_userdata){0};

    ov_vocs_record_config config =
        (ov_vocs_record_config){.loopname = "loop1",
                                .callback.userdata = &userdata,
                                .callback.stop = cb_stop};

    ov_vocs_record *rec = ov_vocs_record_create(config);
    testrun(rec);
    testrun(NULL == ov_vocs_record_free_void(rec));
    testrun(0 == strcmp("loop1", userdata.loopname));
    testrun(userdata.stop);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_record_add_ptt() {

    struct dummy_userdata userdata = (struct dummy_userdata){0};

    ov_vocs_record_config config =
        (ov_vocs_record_config){.loopname = "loop1",
                                .callback.userdata = &userdata,
                                .callback.stop = cb_stop};

    ov_vocs_record *rec = ov_vocs_record_create(config);
    testrun(rec);

    testrun(ov_vocs_record_add_ptt(rec, "1-2-3-4"));
    testrun(rec->data);
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/1-2-3-4")));

    testrun(ov_vocs_record_add_ptt(rec, "2"));
    testrun(rec->data);
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/1-2-3-4")));
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/2")));

    testrun(ov_vocs_record_add_ptt(rec, "3"));
    testrun(rec->data);
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/1-2-3-4")));
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/2")));
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/3")));

    testrun(NULL == ov_vocs_record_free_void(rec));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_record_drop_ptt() {

    struct dummy_userdata userdata = (struct dummy_userdata){0};

    ov_vocs_record_config config =
        (ov_vocs_record_config){.loopname = "loop1",
                                .callback.userdata = &userdata,
                                .callback.stop = cb_stop};

    ov_vocs_record *rec = ov_vocs_record_create(config);
    testrun(rec);

    testrun(ov_vocs_record_add_ptt(rec, "1"));
    testrun(ov_vocs_record_add_ptt(rec, "2"));
    testrun(ov_vocs_record_add_ptt(rec, "3"));
    testrun(ov_vocs_record_add_ptt(rec, "4"));
    testrun(rec->data);
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/1")));
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/2")));
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/3")));
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/4")));

    testrun(ov_vocs_record_drop_ptt(rec, "2"));
    testrun(rec->data);
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/1")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/2")));
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/3")));
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/4")));

    testrun(ov_vocs_record_drop_ptt(rec, "3"));
    testrun(rec->data);
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/1")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/2")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/3")));
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/4")));

    testrun(ov_vocs_record_drop_ptt(rec, "1"));
    testrun(rec->data);
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/1")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/2")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/3")));
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/4")));

    testrun(ov_vocs_record_drop_ptt(rec, "5"));
    testrun(rec->data);
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/1")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/2")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/3")));
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/4")));

    testrun(ov_vocs_record_drop_ptt(rec, "4"));
    testrun(rec->data);
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/1")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/2")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/3")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/4")));

    testrun(NULL == ov_vocs_record_free_void(rec));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_record_get_ptt_count() {

    struct dummy_userdata userdata = (struct dummy_userdata){0};

    ov_vocs_record_config config =
        (ov_vocs_record_config){.loopname = "loop1",
                                .callback.userdata = &userdata,
                                .callback.stop = cb_stop};

    ov_vocs_record *rec = ov_vocs_record_create(config);
    testrun(rec);

    testrun(0 == ov_vocs_record_get_ptt_count(rec));

    testrun(ov_vocs_record_add_ptt(rec, "1"));
    testrun(1 == ov_vocs_record_get_ptt_count(rec));
    testrun(ov_vocs_record_add_ptt(rec, "2"));
    testrun(2 == ov_vocs_record_get_ptt_count(rec));
    testrun(ov_vocs_record_add_ptt(rec, "3"));
    testrun(3 == ov_vocs_record_get_ptt_count(rec));
    testrun(ov_vocs_record_add_ptt(rec, "4"));
    testrun(4 == ov_vocs_record_get_ptt_count(rec));

    testrun(ov_vocs_record_drop_ptt(rec, "2"));
    testrun(3 == ov_vocs_record_get_ptt_count(rec));
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/1")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/2")));
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/3")));
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/4")));

    testrun(ov_vocs_record_drop_ptt(rec, "3"));
    testrun(2 == ov_vocs_record_get_ptt_count(rec));
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/1")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/2")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/3")));
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/4")));

    testrun(ov_vocs_record_drop_ptt(rec, "1"));
    testrun(1 == ov_vocs_record_get_ptt_count(rec));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/1")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/2")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/3")));
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/4")));

    testrun(ov_vocs_record_drop_ptt(rec, "5"));
    testrun(1 == ov_vocs_record_get_ptt_count(rec));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/1")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/2")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/3")));
    testrun(ov_json_is_true(ov_json_get(rec->data, "/" OV_KEY_PTT "/4")));

    testrun(ov_vocs_record_drop_ptt(rec, "4"));
    testrun(0 == ov_vocs_record_get_ptt_count(rec));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/1")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/2")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/3")));
    testrun(!(ov_json_get(rec->data, "/" OV_KEY_PTT "/4")));

    testrun(NULL == ov_vocs_record_free_void(rec));

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
    testrun_test(test_ov_vocs_record_create);
    testrun_test(test_ov_vocs_record_free_void);
    testrun_test(test_ov_vocs_record_add_ptt);
    testrun_test(test_ov_vocs_record_drop_ptt);
    testrun_test(test_ov_vocs_record_get_ptt_count);

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
