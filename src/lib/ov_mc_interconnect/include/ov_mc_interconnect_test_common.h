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
        @file           ov_mc_interconnect_test_common.h
        @author         Markus Töpfer

        @date           2023-12-18


        ------------------------------------------------------------------------
*/
#ifndef ov_mc_interconnect_test_common_h
#define ov_mc_interconnect_test_common_h

#include <ov_base/ov_json.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

int ov_mc_interconnect_common_domains_init();
int ov_mc_interconnect_common_domains_deinit();

const char *ov_mc_interconnect_test_common_get_test_resource_dir();

ov_json_value *ov_mc_interconnect_test_common_get_loops();

#endif /* ov_mc_interconnect_test_common_h */
