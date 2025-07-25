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
        @file           ov_mc_mixer_data.c
        @author         Markus Töpfer

        @date           2023-01-21


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_mixer_data.h"

ov_mc_mixer_data ov_mc_mixer_data_set_socket(int socket) {

    ov_mc_mixer_data data = (ov_mc_mixer_data){0};
    data.socket = socket;
    ov_socket_get_data(socket, NULL, &data.remote);

    return data;
}