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

        @author         Michael J. Beer

        ------------------------------------------------------------------------
*/
#include "../include/ov_os_linux.h"
#include <ov_base/ov_error_codes.h>

#include <libgen.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <unistd.h>

/******************************************************************************/

bool executable(char const *path) { return (0 == access(path, F_OK || X_OK)); }

/******************************************************************************/

bool change_working_dir(char const *working_dir) {
    if ((0 != working_dir) && (0 != chdir(working_dir))) {
        fprintf(stderr, "Could not change to working dir %s", working_dir);
        exit(1);
    }

    return true;
}

/******************************************************************************/

bool detach() {

    if (setsid() < 0) {
        fprintf(stderr, "Could not detach new process");
        exit(1);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    return true;
}

/******************************************************************************/

bool set_die_on_parents_death() {

    if (prctl(PR_SET_PDEATHSIG, SIGKILL) == -1)
        _exit(1);

    if (getppid() == 1)
        _exit(1);

    return true;
}

/******************************************************************************/

char const **prepend(char const *const *array, const char *str) {

    size_t array_size = 0;

    char const *const *ptr = array;

    while (ptr[++array_size] != 0) {
    };

    char const **new_array = calloc(array_size + 1, sizeof(char const *));

    new_array[0] = str;

    for (size_t i = 0; array[i] != 0; ++i) {
        new_array[i + 1] = array[i];
    }

    new_array[array_size] = 0;

    return new_array;
}

/******************************************************************************/

_Noreturn void spawn(char const *binary, char const *const *arguments) {

    if ((0 == binary) || (0 == arguments)) {
        exit(1);
    }

    // Linux expects the first arg to be the binary name
    execv(binary, (char **)prepend(arguments, basename(strdup(binary))));

    exit(1);
}

/******************************************************************************/

int ov_os_linux_spawn(char const *workdir, char const *binary,
                      char const *const *arguments, bool detach_process) {

    if ((0 == workdir) || (0 == binary) || (0 == arguments)) {
        fprintf(stderr, "ov_os_linux_spawn: Invalid argument\n");
        return -OV_ERROR_BAD_ARG;
    }

    if (!executable(binary)) {
        fprintf(stderr, "ov_os_linux_spawn: %s not found\n", binary);
        return -OV_ERROR_CODE_NOT_FOUND_ERROR;
    }

    int proc_pid = fork();

    if (0 == proc_pid) {

        chdir(workdir);

        if (detach_process) {
            detach();
        } else {
            set_die_on_parents_death();
        }

        spawn(binary, arguments);
    }

    if (0 > proc_pid) {
        return -OV_ERROR_CODE_UNKNOWN;
    }

    return proc_pid;
}

/******************************************************************************/
