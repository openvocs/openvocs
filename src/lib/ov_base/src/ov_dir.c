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
        @file           ov_dir.c
        @author         Michael Beer

        @date           2021-02-12


        ------------------------------------------------------------------------
*/
#include "../include/ov_dir.h"
#include "../include/ov_uri.h"
#include "../include/ov_utils.h"

#include <ftw.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

/*----------------------------------------------------------------------------*/

static int delete_node(char const *path, const struct stat *sb, int typeflag,
                       struct FTW *ftwbuf) {

  UNUSED(sb);
  UNUSED(typeflag);
  UNUSED(ftwbuf);

  int retval = remove(path);

  return retval;
}

/*----------------------------------------------------------------------------*/

bool ov_dir_tree_remove(const char *path) {

  struct stat statbuf = {0};
  errno = 0;

  if (0 == path)
    return false;

  // no stat not existing
  if (0 != stat(path, &statbuf))
    return true;

  if (0 !=
      nftw(path, delete_node, FOPEN_MAX, FTW_DEPTH | FTW_MOUNT | FTW_PHYS)) {
    return false;
  }

  return true;
}

/*----------------------------------------------------------------------------*/

static mode_t get_path_mode(char const *path) {
  // Returns 0 if path does not exist

  struct stat statbuf = {0};
  if (stat(path, &statbuf)) {

    /* Does not exist */
    return 0;
  }

  return statbuf.st_mode;
}

/*----------------------------------------------------------------------------*/

static bool our_mkdir(char const *path) {

  return 0 == mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);
}

/*----------------------------------------------------------------------------*/

static bool mkdir_recursive(char *path) {
  // Modifies path

  bool result = false;

  if (0 == path)
    goto error;

  mode_t path_mode = get_path_mode(path);

  /* Special cases */

  /* 1. Path exists */
  if (0 != path_mode) {

    return S_ISDIR(path_mode);
  }

  char *last_occurrence = strrchr(path, '/');

  /* 2: one relative dir, no path separator */
  if (0 == last_occurrence) {
    result = our_mkdir(path);
    goto finish;
  }

  /* 3. Absolute path, one tier */
  if (last_occurrence == path) {

    result = our_mkdir(path);
    goto finish;
  }

  /* General case: Path separator found ... */

  /* 'Cut' path at second last dir */
  *last_occurrence = 0;

  if (!mkdir_recursive(path)) {

    ov_log_error("Could not create %s", path);
    goto error;
  }

  /* Now, path minus last dir exists,
   * 'append' last path component again and create entire path
   */

  *last_occurrence = '/';

  result = our_mkdir(path);

finish:

  return result;

error:

  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_dir_tree_create(char const *path) {

  if (0 == path)
    goto error;

  char clean[PATH_MAX] = {0};

  if (!ov_uri_path_remove_dot_segments(path, clean))
    goto error;

  size_t len = strlen(clean);
  if ('/' == clean[len - 1])
    clean[len - 1] = 0;

  char *path_copy = strdup(clean);
  len = strlen(path_copy);

  bool result = mkdir_recursive(path_copy);

  free(path_copy);

  return result;

error:

  return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_dir_access_to_path(const char *path) {

  struct stat statbuf = {0};
  errno = 0;

  if (!path)
    goto error;

  if (0 != stat(path, &statbuf))
    goto error;

  mode_t mode = statbuf.st_mode & S_IFMT;

  if (mode != S_IFDIR)
    goto error;

  if (0 != access(path, F_OK))
    goto error;

  return true;
error:
  return false;
}
