/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_mc_mixer_data.h
        @author         Markus Töpfer

        @date           2023-01-21


        ------------------------------------------------------------------------
*/
#ifndef ov_mc_mixer_data_h
#define ov_mc_mixer_data_h

#include <ov_base/ov_id.h>
#include <ov_base/ov_socket.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_mixer_data {

    int socket;
    ov_socket_data remote;
    ov_id uuid;
    ov_id user;

} ov_mc_mixer_data;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_mixer_data ov_mc_mixer_data_set_socket(int socket);

#endif /* ov_mc_mixer_data_h */
