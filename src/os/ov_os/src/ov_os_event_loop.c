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

        @author         Michael J. Beer

        ------------------------------------------------------------------------
*/

#include "../include/ov_os_event_loop.h"
#include <ov_arch/ov_arch.h>

#if OV_ARCH == OV_LINUX

#include <ov_os_linux/ov_event_loop_linux.h>

#define EVENT_LOOP_CREATOR ov_event_loop_linux

#else

#define EVENT_LOOP_CREATOR ov_event_loop_default

#endif

// yupp, that's complicated ...
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define EVENT_LOOP_CREATOR_STR STR(EVENT_LOOP_CREATOR)

// _Static_assert(EVENT_LOOP_CREATOR);

ov_event_loop *ov_os_event_loop(ov_event_loop_config config) {

    ov_log_info("Using " EVENT_LOOP_CREATOR_STR);
    return EVENT_LOOP_CREATOR(config);
}

#undef EVENT_LOOP_CREATOR

/*----------------------------------------------------------------------------*/
