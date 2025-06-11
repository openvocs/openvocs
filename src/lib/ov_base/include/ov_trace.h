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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_TRACE_H
#define OV_TRACE_H

/*----------------------------------------------------------------------------*/

#include <stdbool.h>

#define OV_TRACE_ACTIVE true

#ifdef OV_TRACE_ACTIVE

#define OV_TRACE_LIMIT_SECS 90

extern bool ov_trace_active;

#undef assert
#include <assert.h>

#define OV_TRACE_IN                                                            \
    int trace_time__ = time(0);                                                \
    if (ov_trace_active) {                                                     \
        fprintf(stderr, "TRACE IN %s: %i\n", __FUNCTION__, __LINE__);          \
    }

#define OV_TRACE_OUT                                                           \
    if (ov_trace_active) {                                                     \
        trace_time__ = time(0) - trace_time__;                                 \
        fprintf(stderr,                                                        \
                "TRACE OUT %s: %i    secs passed: %i\n",                       \
                __FUNCTION__,                                                  \
                __LINE__,                                                      \
                trace_time__);                                                 \
        if (trace_time__ > OV_TRACE_LIMIT_SECS) {                              \
            fprintf(stderr,                                                    \
                    "TRACE OUT: Function %s took too long: %i seconds\n",      \
                    __FUNCTION__,                                              \
                    trace_time__);                                             \
            assert(false);                                                     \
        }                                                                      \
    }

#define OV_TRACE_TIMEOUT(start, now, entity)                                   \
    if (ov_trace_active) {                                                     \
        fprintf(stderr, "TRACE: %s took %i secs\n", entity, now - start);      \
        if ((now - start) > OV_TRACE_LIMIT_SECS) {                             \
            fprintf(stderr, "\n");                                             \
            fprintf(stderr,                                                    \
                    "TRACE: ---- %s TOOK TOO LONG: %i seconds ----\n",         \
                    entity,                                                    \
                    now - start);                                              \
            fprintf(stderr, "\n");                                             \
            assert(false);                                                     \
        }                                                                      \
    }

#endif // OV_TRACE_ACTIVE

/*----------------------------------------------------------------------------*/

void ov_trace_activate();

#endif
