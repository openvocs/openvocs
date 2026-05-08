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

#include <ov_base/ov_utils.h>

#include "../include/ov_signaling_message.h"
#include <ov_base/ov_config_keys.h>
#include <ov_core/ov_event_api.h>

ov_json_value *ov_signaling_message_register(ov_app *app) {

    ov_json_value *msg = 0;

    if (0 == app) {
        ov_log_error("No app (0 pointer) given");
        goto error;
    }

    msg = ov_event_api_message_create(OV_EVENT_REGISTER, 0, 0);

    if (0 == msg) {

        goto error;
    }

    ov_json_value *params = ov_event_api_set_parameter(msg);

    if (0 == params) {

        ov_log_error("Could not add parameters to register message");
        goto error;
    }

    if (!ov_json_object_set(params, OV_KEY_TYPE,
                            ov_json_string(app->config.name))) {

        ov_log_error("Could not set service type");
        goto error;
    }

    return msg;

error:

    if (0 != msg) {

        msg = msg->free(msg);
    }

    OV_ASSERT(0 == msg);

    return 0;
}

/*----------------------------------------------------------------------------*/
