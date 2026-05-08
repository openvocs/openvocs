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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "../../include/ov_test_event_loop.h"
#include "../../include/ov_utils.h"

/*----------------------------------------------------------------------------*/

static void *run_loop(void *varg) {

    ov_event_loop *loop = varg;

    OV_ASSERT(0 != loop);

    loop->run(loop, UINT64_MAX);

    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_test_event_loop_run_in_thread(ov_event_loop *loop, pthread_t *tid) {

    if (0 == loop)
        return false;

    OV_ASSERT(0 != loop);

    if (0 != pthread_create(tid, 0, run_loop, loop)) {

        testrun_log_error("Could not start loop thread");

        return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_test_event_loop_stop_thread(ov_event_loop *loop, pthread_t tid) {

    if (0 == loop)
        return false;

    OV_ASSERT(0 != loop);

    loop->stop(loop);

    void *retval = 0;

    if (0 != pthread_join(tid, &retval)) {

        testrun_log_error("Could not stop loop thread");
        abort();

    } else {

        ov_log_info("Stopped loop thread");
    }

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_test_event_loop_run_for_secs(ov_event_loop *loop, unsigned int secs) {

    if (0 == loop)
        return false;

    pthread_t minions_loop_tid;
    ov_test_event_loop_run_in_thread(loop, &minions_loop_tid);

    sleep(secs);

    return ov_test_event_loop_stop_thread(loop, minions_loop_tid);
}

/*----------------------------------------------------------------------------*/
