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
        @file           ov_mc_loop_data.h
        @author         Markus

        @date           2022-11-10

        Definition data for some multicast loop. 

        MUST be at least the name of the loop and the multicast IP to be used. 

        ------------------------------------------------------------------------
*/
#ifndef ov_mc_loop_data_h
#define ov_mc_loop_data_h

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_socket.h>

#define OV_MC_LOOP_NAME_MAX 255

typedef struct ov_mc_loop_data {

    ov_socket_configuration socket;
    char name[OV_MC_LOOP_NAME_MAX];
    uint8_t volume;

} ov_mc_loop_data;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_loop_data ov_mc_loop_data_from_json(const ov_json_value *value);
ov_json_value *ov_mc_loop_data_to_json(ov_mc_loop_data data);

#endif /* ov_mc_loop_data_h */
