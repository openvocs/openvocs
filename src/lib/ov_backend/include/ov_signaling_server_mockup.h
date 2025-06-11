/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_SIGNALING_SERVER_MOCKUP_H
#define OV_SIGNALING_SERVER_MOCKUP_H

#include "ov_signaling_app.h"
#include <ov_base/ov_event_loop.h>

/*----------------------------------------------------------------------------*/

ov_app *ov_signaling_server_mockup_create(ov_event_loop *loop,
                                          ov_socket_configuration server_socket,
                                          uint32_t timeout_secs);

/*----------------------------------------------------------------------------*/

/**
 * @return old userdata if one was set before. 0 if none was set or in case of
 * error.
 */
void *ov_signaling_server_mockup_set_userdata(ov_app *self, void *userdata);

/*----------------------------------------------------------------------------*/

void *ov_signaling_server_mockup_get_userdata(ov_app *self);

/*----------------------------------------------------------------------------*/

bool ov_signaling_server_mockup_run(ov_app *self);

/*----------------------------------------------------------------------------*/

bool ov_signaling_server_mockup_stop(ov_app *self);

/*****************************************************************************
                               SENDING TO MOCKUP
 ****************************************************************************/

bool ov_signaling_server_mockup_send(ov_app *self, ov_json_value *msg);

/*****************************************************************************
                                 EVENT TRACKING
 ****************************************************************************/

void ov_signaling_server_mockup_track_event(ov_app *self,
                                            char const *event_name);

bool ov_signaling_server_mockup_event_received(ov_app *self,
                                               char const *event_name);

ov_json_value *ov_signaling_server_mockup_event_parameters(
    ov_app *self, char const *event_name);

#endif
