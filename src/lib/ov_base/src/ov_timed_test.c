/***
        ------------------------------------------------------------------------

        Copyright (c) 2026 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_timed_test.c
        @author         Töpfer, Markus

        @date           2026-05-23


        ------------------------------------------------------------------------
*/
#include <ov_test/testrun.h>
#include "ov_timed.c"

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_timed_create(){
    
    ov_event_loop *loop = ov_event_loop_default(ov_event_loop_config_default());
    testrun(loop);

    ov_timed *timed = ov_timed_create((ov_timed_config){.loop = loop});
    testrun(timed);

    timed = ov_timed_free(timed);
    loop = ov_event_loop_free(loop);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

struct dummy {

    ov_id id;
};

/*----------------------------------------------------------------------------*/

static void callback(void *userdata, const char *uuid){

    ov_log_debug("CALLBACK CALLED");

    struct dummy *dummy = (struct dummy*) userdata;
    ov_id_set(dummy->id, uuid);
    return;
}

/*----------------------------------------------------------------------------*/

int test_ov_timed_add(){

    struct dummy dummy = {0};
    
    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_timed *timed = ov_timed_create((ov_timed_config){.loop = loop});
    testrun(timed);

    ov_id id = {0};
    ov_id_fill_with_uuid(id);

    ov_time time = ov_timestamp_create();
    time.second += 5;

    testrun(ov_timed_add(timed, time, id, &dummy, callback));

    sleep(1);
    ov_event_loop_run(loop, OV_RUN_ONCE);
    testrun(0 == dummy.id[0]);
    sleep(1);
    ov_event_loop_run(loop, OV_RUN_ONCE);
    testrun(0 == dummy.id[0]);
    sleep(1);
    ov_event_loop_run(loop, OV_RUN_ONCE);
    testrun(0 == dummy.id[0]);

    sleep(3);
    ov_event_loop_run(loop, OV_RUN_ONCE);
    testrun(0 != dummy.id[0]);

    timed = ov_timed_free(timed);
    loop = ov_event_loop_free(loop);

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
    testrun_test(test_ov_timed_create);
    testrun_test(test_ov_timed_add);

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
