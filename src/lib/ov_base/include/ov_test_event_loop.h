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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_EVENT_LOOP_TEST_H
#define OV_EVENT_LOOP_TEST_H

/*----------------------------------------------------------------------------*/

#include "ov_event_loop.h"
#include <pthread.h>

/*----------------------------------------------------------------------------*/

bool ov_test_event_loop_run_in_thread(ov_event_loop *loop, pthread_t *tid);

bool ov_test_event_loop_stop_thread(ov_event_loop *loop, pthread_t tid);

bool ov_test_event_loop_run_for_secs(ov_event_loop *loop, unsigned int secs);

/*----------------------------------------------------------------------------*/
#endif
