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
#ifndef OV_TEST_FILE_H
#define OV_TEST_FILE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/*----------------------------------------------------------------------------*/

#define OV_TEST_FILE_TMP_PATH_LEN 20

/**
 * Creates temporary file and write  `content` into it.
 * Returns name of temp file in `file_name`.
 * The file stays open after the function returns, thus be sure to close the
 * file descriptor when done.
 * @param file_name must have been allocated with at least
 * `OV_TEST_FILE_TMP_PATH_LEN` chars
 * @return file descriptor of the opened file.
 */
int ov_test_file_tmp_write(uint8_t const *content, size_t length,
                           char *file_name);

/*----------------------------------------------------------------------------*/

/**
 * Create temp dir.
 *
 * @param prefix If not set, defaults to '/tmp'
 */
char *ov_test_temp_dir(char const *prefix);

/*----------------------------------------------------------------------------*/
#endif
