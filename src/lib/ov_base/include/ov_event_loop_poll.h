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
        @file           ov_event_loop_poll.h
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-12-17

        @ingroup        ov_event_loop_poll

        @brief          Definition of a poll based implementation of
                        ov_event_loop

                        This implementation is the default implementation
                        for ov_event_loop.

                        All timer calls are inlined to the poll event loop.
                        Based on the poll loop characteristics timer the best
                        possible timer resolution under optimal conditions
                        is 1ms (milli second).

                        Timers may be delayed by other running timers,
                        which have registered for the same time window
                        (again time window of 1 ms).

        ------------------------------------------------------------------------
*/
#ifndef ov_event_loop_poll_h
#define ov_event_loop_poll_h

#include "ov_event_loop.h"
#include <poll.h>

/**
        Create a poll based ov_event_loop.
*/
ov_event_loop *ov_event_loop_poll(ov_event_loop_config config);

/*---------------------------------------------------------------------------*/

#endif /* ov_event_loop_poll_h */
