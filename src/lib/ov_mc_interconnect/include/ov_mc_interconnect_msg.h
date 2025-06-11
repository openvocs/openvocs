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
        @file           ov_mc_interconnect_msg.h
        @author         Markus Töpfer

        @date           2023-12-11


        ------------------------------------------------------------------------
*/
#ifndef ov_mc_interconnect_msg_h
#define ov_mc_interconnect_msg_h

#include <ov_base/ov_json.h>

#define OV_MC_INTERCONNECT_VERSION 1

#define OV_MC_INTERCONNECT_REGISTER "register"
#define OV_MC_INTERCONNECT_CONNECT_MEDIA "connect_media"
#define OV_MC_INTERCONNECT_CONNECT_LOOPS "connect_loops"

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_json_value *ov_mc_interconnect_msg_register(const char *name,
                                               const char *pass);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_interconnect_msg_connect_media(const char *name,
                                                    const char *codec,
                                                    const char *host,
                                                    uint32_t port);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_interconnect_msg_connect_loops();

#endif /* ov_mc_interconnect_msg_h */
