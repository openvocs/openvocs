/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_ice_state.c
        @author         Markus

        @date           2024-09-18


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_state.h"

#include "../include/ov_ice_string.h"
#include <ov_base/ov_config_keys.h>

const char *ov_ice_state_to_string(ov_ice_state state) {

    switch (state) {

    case OV_ICE_ERROR:
        return OV_KEY_ERROR;

    case OV_ICE_INIT:
        return OV_KEY_INIT;

    case OV_ICE_RUNNING:
        return OV_KEY_RUNNING;

    case OV_ICE_COMPLETED:
        return OV_KEY_COMPLETED;

    case OV_ICE_FAILED:
        return OV_KEY_FAILED;
    }

    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice_state ov_ice_state_from_string(const char *string) {

    if (!string)
        goto error;

    if (0 == strcmp(string, OV_KEY_COMPLETED))
        return OV_ICE_COMPLETED;

    if (0 == strcmp(string, OV_KEY_FAILED))
        return OV_ICE_FAILED;

    if (0 == strcmp(string, OV_KEY_RUNNING))
        return OV_ICE_RUNNING;

    if (0 == strcmp(string, OV_KEY_INIT))
        return OV_ICE_INIT;

error:
    return OV_ICE_ERROR;
}