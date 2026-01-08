/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_result.h
        @author         Michael J. Beer

        @date           2019-10-29

        ------------------------------------------------------------------------
*/
#ifndef OV_RESULT_H
#define OV_RESULT_H

#include "ov_error_codes.h"
#include "ov_utils.h"
#include <stdbool.h>
#include <stdint.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_result {

    uint64_t error_code; /* See ov_error_codes.h for possible values */
    char *message;       /* If error_code == OV_ERROR_NOERROR, must be 0. */

} ov_result;

/*----------------------------------------------------------------------------*/

bool ov_result_set(ov_result *result, int error_code, char const *message);

/*----------------------------------------------------------------------------*/

char const *ov_result_get_message(ov_result const result);

/*----------------------------------------------------------------------------*/

bool ov_result_clear(ov_result *result);

/*----------------------------------------------------------------------------*/
#endif
