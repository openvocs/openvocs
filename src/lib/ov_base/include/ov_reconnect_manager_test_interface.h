/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

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

        @author         Michael J. Beer

        ------------------------------------------------------------------------
*/
#ifndef OV_RECONNECT_MANAGER_TEST_INTERFACE_H
#define OV_RECONNECT_MANAGER_TEST_INTERFACE_H
/*----------------------------------------------------------------------------*/

#include "ov_reconnect_manager.h"

/*----------------------------------------------------------------------------*/

/**
 * Tests reconnect manager with an arbitrary event loop.
 */
int ov_reconnect_manager_connect_test(
    ov_event_loop *(*loop_create)(ov_event_loop_config cfg));

/*----------------------------------------------------------------------------*/

bool ov_equals_json_string(ov_json_value *jstring, char const *ref_string);

/*----------------------------------------------------------------------------*/
#endif
