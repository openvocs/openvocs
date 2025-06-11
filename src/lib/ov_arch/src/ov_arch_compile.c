/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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

#include "../include/ov_arch_compile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FRAME_LENGTH 120

/*****************************************************************************
                                    GNU LIBC
 ****************************************************************************/

#if defined(__GNUC__) && (__GNUC__ >= 4)

#include <execinfo.h>

static char *frame_to_string(char *target,
                             const size_t max_bytes,
                             void const *address,
                             char const *symbol) {

    if (0 == target) {
        abort();
    }

    if (0 == symbol) {
        symbol = "UNKNOWN";
    }

    snprintf(target, max_bytes, "%s (%p)", symbol, address);
    // just to be sure...
    target[max_bytes - 1] = 0;

    return target;
}

/*----------------------------------------------------------------------------*/

static char *backtrace_to_string(void const **addresses,
                                 char const *const *symbols,
                                 size_t num) {

    char *bt = calloc(num + 1, MAX_FRAME_LENGTH);
    char *bt_copy = calloc(num + 1, MAX_FRAME_LENGTH);

    char frame[MAX_FRAME_LENGTH] = {0};

    const size_t max_len_bytes = (num + 1) * MAX_FRAME_LENGTH;

    for (size_t i = 0; i < num; ++i) {

        snprintf(
            bt,
            max_len_bytes,
            "%s\n%s",
            bt_copy,
            frame_to_string(frame, MAX_FRAME_LENGTH, addresses[i], symbols[i]));

        strcpy(bt_copy, bt);
    }

    free(bt_copy);

    return bt;
}

/*----------------------------------------------------------------------------*/

char *ov_arch_compile_backtrace(size_t max_frames) {

    void **addresses = calloc(1, sizeof(void *) * max_frames);

    size_t actual_no_frames = backtrace(addresses, max_frames);
    char **symbols = backtrace_symbols(addresses, actual_no_frames);

    char *backtrace = backtrace_to_string((void const **)addresses,
                                          (char const *const *)symbols,
                                          actual_no_frames);

    free(symbols);
    free(addresses);

    return backtrace;
}

/*****************************************************************************
                                EVERY ELSE LIBC
 ****************************************************************************/

#else

char *ov_arch_compile_backtrace() {

    return strdup("=== Backtrace not supported ===");
}

#endif
