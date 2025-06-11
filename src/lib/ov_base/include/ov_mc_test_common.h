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
        @file           ov_mc_test_common.h
        @author         Markus Töpfer

        @date           2022-11-11


        ------------------------------------------------------------------------
*/
#ifndef ov_mc_test_common_h
#define ov_mc_test_common_h

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
 *  Open some socket on any interface
 *
 *  @param net  AF_INET or AF_INET6
 *
 *  @returns new socket which is not localhost
 */
int ov_mc_test_common_open_socket(int net);

#endif /* ov_mc_test_common_h */
