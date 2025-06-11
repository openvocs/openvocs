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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_LOG_ROTATE_H
#define OV_LOG_ROTATE_H

#include <stdbool.h>
#include <stdlib.h>

/*----------------------------------------------------------------------------*/

#define OV_LOG_ROTATE_MAX_FILES 1000

/*----------------------------------------------------------------------------*/

bool ov_log_rotate_files(char const *path, size_t max_num_files);

/*----------------------------------------------------------------------------*/
#endif
