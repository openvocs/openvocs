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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "../include/testrun.h"

#include <getopt.h>
#include <libgen.h>
#include <signal.h>
#include <string.h>

static char *test_directory = 0;

static void (*s_exit_hook)(void) = 0;

/*----------------------------------------------------------------------------*/

void ov_test_set_test_directory(char const *binary_path) {

    test_directory = dirname(strdup(binary_path));
}

/*----------------------------------------------------------------------------*/

void ov_test_clear_test_directory() {

    if (0 == test_directory)
        return;

    free(test_directory);
    test_directory = 0;
}

/*----------------------------------------------------------------------------*/

void ov_test_set_exit_hook(void (*hook)()) { s_exit_hook = hook; }

/*----------------------------------------------------------------------------*/

void (*ov_test_get_exit_hook())(void) { return s_exit_hook; }

/*----------------------------------------------------------------------------*/

char *ov_test_get_resource_path(char const *rel_resource_path) {

    if (0 == rel_resource_path)
        return strdup(test_directory);

    char path[255] = {0};
    strncpy(path, test_directory, sizeof(path));

    size_t printed = snprintf(path, sizeof(path), "%s/%s", test_directory,
                              rel_resource_path);

    if (sizeof(path) <= printed)
        return 0;

    path[sizeof(path) - 1] = 0;

    return strdup(path);
}

/*----------------------------------------------------------------------------*/

static void exit_sighandler(int sig) {

    // Only for testing - signal handlers SHOULD NOT do I/O
    fprintf(stderr, "Signal %i received\n", sig);
    exit(1);
}

/*----------------------------------------------------------------------------*/

void ov_test_exit_on_signal() {

    struct sigaction exit_action = {
        .sa_handler = exit_sighandler,
    };

    for (int i = 0; i < NSIG; ++i) {

        if (i == SIGKILL)
            continue;
        if (i == SIGSTOP)
            continue;
        if (i == SIGSEGV)
            continue;
        if (i == SIGCHLD)
            continue;

        sigaction(i, &exit_action, 0);
    }
}
/*----------------------------------------------------------------------------*/

void ov_test_ignore_signals() {

    struct sigaction ignore_action = {
        .sa_handler = SIG_IGN,
    };

    for (int i = 0; i < NSIG; ++i) {

        if (i == SIGKILL)
            continue;
        if (i == SIGSTOP)
            continue;
        if (i == SIGSEGV)
            continue;
        if (i == SIGCHLD)
            continue;

        sigaction(i, &ignore_action, 0);
    }
}

/*----------------------------------------------------------------------------*/
