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
#ifndef OV_SIGNALING_MESSAGE_H
#define OV_SIGNALING_MESSAGE_H
/*----------------------------------------------------------------------------*/

#include <ov_base/ov_event_keys.h>

/*----------------------------------------------------------------------------*/

#include "ov_app.h"

/**
 * Creates a service regiser_request .
 * This message will be sent towards the liege to notify about a new service
 * becoming available and ready for reconfigure.
 */
ov_json_value *ov_signaling_message_register(ov_app *app);

/*----------------------------------------------------------------------------*/
#endif
