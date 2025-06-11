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
        @file           ov_ice_state.h
        @author         Markus Töpfer

        @date           2024-09-18


        ------------------------------------------------------------------------
*/
#ifndef ov_ice_state_h
#define ov_ice_state_h

/*----------------------------------------------------------------------------*/

typedef enum {

    OV_ICE_ERROR = -1,
    OV_ICE_INIT = 0,
    OV_ICE_RUNNING = 1,
    OV_ICE_COMPLETED = 2,
    OV_ICE_FAILED = 3

} ov_ice_state;

/*----------------------------------------------------------------------------*/

typedef enum {

    OV_ICE_PAIR_FROZEN = 0,
    OV_ICE_PAIR_WAITING = 1,
    OV_ICE_PAIR_PROGRESS = 2,
    OV_ICE_PAIR_SUCCESS = 4,
    OV_ICE_PAIR_FAILED = 8

} ov_ice_pair_state;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

const char *ov_ice_state_to_string(ov_ice_state state);
ov_ice_state ov_ice_state_from_string(const char *state);

#endif /* ov_ice_state_h */
