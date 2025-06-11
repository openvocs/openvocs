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
        @file           ov_ice_proxy_multiplexing.h
        @author         Markus Toepfer

        @date           2024-07-26
        
        Implementation of a multiplexing ICE proxy. This proxy multiplexes 
        connections over one port. 
        

        ------------------------------------------------------------------------
*/
#ifndef ov_ice_proxy_multiplexing_h
#define ov_ice_proxy_multiplexing_h

#include "ov_ice_proxy_generic.h"

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_ice_proxy_generic *ov_ice_proxy_multiplexing_create(
    ov_ice_proxy_generic_config config);

#endif /* ov_ice_proxy_multiplexing_h */
