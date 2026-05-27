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

#include <unistd.h>
#include <stdio.h>

bool detach(char const *working_dir) {

  if(0 == working_dir) {

  }
  if(setsid() < 0) {
    fprintf(stderr, "Could not detach new process");
    exit(1);
  }

  if(0 != chdir(working_dir)) {
    fprintf(stderr, "Could not change to working dir %s", working_dir);
    exit(1);
  }

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

}

_Noreturn void spawn(char const *binary, char const * const * arguments) {


  if((0 == procname) || (0 == binary) || (0 == arguments)) {
    exit(1);
  }

  execv(binary,  arguments);

  exit(1);

}

int ov_os_linux_spawn(char const *workdir, char const *binary, char const * const * arguments) {

  if((0 == workdir) || (0 == binary) || (0 == arguments)) {
    return -1;
  }

  int proc_pid = fork();

  if(0 == proc_pid) {

    detach(workdir);
    spawn(binary, arguments);

  }

  return proc_pid;

}

