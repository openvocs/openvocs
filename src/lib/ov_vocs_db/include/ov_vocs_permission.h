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
        @file           ov_vocs_permission.h
        @author         Markus Töpfer

        @date           2022-07-26


        ------------------------------------------------------------------------
*/
#ifndef ov_vocs_permission_h
#define ov_vocs_permission_h

#include <ov_base/ov_json.h>
#include <stdbool.h>

/*----------------------------------------------------------------------------*/

typedef enum ov_vocs_permission {

    OV_VOCS_NONE = 0,
    OV_VOCS_RECV = 1,
    OV_VOCS_SEND = 2

} ov_vocs_permission;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/* returns the standard strings for ov_vocs [none,recv,send] */
const char *ov_vocs_permission_to_string(ov_vocs_permission self);

/*----------------------------------------------------------------------------*/

/* parses the standard strings for ov_vocs [none,recv,send] */
ov_vocs_permission ov_vocs_permission_from_string(const char *str);

/*----------------------------------------------------------------------------*/

/* Test if the permission to check is allowed by reference */
bool ov_vocs_permission_granted(ov_vocs_permission reference,
                                ov_vocs_permission check);

/*----------------------------------------------------------------------------*/

ov_vocs_permission ov_vocs_permission_from_json(const ov_json_value *val);

#endif /* ov_vocs_permission_h */
