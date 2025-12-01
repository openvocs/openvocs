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
        @file           ov_file_access_check.c
        @author         Markus Töpfer

        @date           2021-11-04


        ------------------------------------------------------------------------
*/

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <ov_base/ov_config.h>
#include <ov_base/ov_dir.h>
#include <ov_base/ov_file.h>
#include <ov_base/ov_utils.h>
#include <ov_core/ov_domain.h>

/*----------------------------------------------------------------------------*/

int main(int argc, char **argv) {

  struct stat statbuf = {0};
  errno = 0;

  const char *path = ov_config_path_from_command_line(argc, argv);
  if (!path)
    goto error;

  if (access(path, F_OK) == -1) {
    fprintf(stdout, "access failed errno %i|%s", errno, strerror(errno));
    goto error;
  }

  if (0 != stat(path, &statbuf)) {
    fprintf(stdout, "stat failed errno %i|%s", errno, strerror(errno));
    goto error;
  }

  mode_t mode = statbuf.st_mode & S_IFMT;

  if (mode == S_IFDIR) {
    fprintf(stdout, "mode is dir errno %i|%s", errno, strerror(errno));
    goto error;
  }

  if ((mode == S_IFIFO) || (mode == S_IFREG) || (mode == S_IFSOCK)) {

    int fd = -1;
    fd = open(path, O_RDONLY);

    if (0 > fd) {
      fprintf(stdout, "open failed errno %i|%s", errno, strerror(errno));
      goto error;
    }

    close(fd);
  }

  return EXIT_SUCCESS;

error:
  return EXIT_FAILURE;
}