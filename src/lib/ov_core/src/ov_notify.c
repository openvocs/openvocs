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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
/*----------------------------------------------------------------------------*/

#include "../include/ov_notify.h"
#include "../include/ov_event_api.h"
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_event_keys.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_utils.h>

/*----------------------------------------------------------------------------*/

char const *ov_notify_type_to_string(ov_notify_type type) {

    switch (type) {

    case NOTIFY_INVALID:

        return 0;

    case NOTIFY_ENTITY_LOST:

        return "entity_lost";

    case NOTIFY_CALL_TERMINATED:

        return "call_terminated";

    case NOTIFY_NEW_CALL:
        return "new_call";

    case NOTIFY_INCOMING_CALL:
        return "incoming_call";

    case NOTIFY_NEW_RECORDING:

        return "new_recording";

    case NOTIFY_PLAYBACK_STOPPED:

        return "playback_stopped";

    default:

        // OV_ASSERT(!"MUST NEVER HAPPEN");
        return 0;
    };
}

/*----------------------------------------------------------------------------*/

ov_notify_type ov_notify_type_from_string(char const *str) {

    if (!ov_ptr_valid(str, "Invalid notify type (0 pointer)")) {
        return NOTIFY_INVALID;

    } else if (0 == strcmp("entity_lost", str)) {

        return NOTIFY_ENTITY_LOST;

    } else if (0 == strcmp("incoming_call", str)) {

        return NOTIFY_INCOMING_CALL;

    } else if (0 == strcmp("new_call", str)) {

        return NOTIFY_NEW_CALL;

    } else if (0 == strcmp("call_terminated", str)) {

        return NOTIFY_CALL_TERMINATED;

    } else if (0 == strcmp("new_recording", str)) {

        return NOTIFY_NEW_RECORDING;

    } else {

        return NOTIFY_INVALID;
    }
}

/*----------------------------------------------------------------------------*/

static ov_json_value *notify_message(char const *uuid, char const *type_str) {

    ov_json_value *msg = ov_event_api_message_create(OV_EVENT_NOTIFY, uuid, 0);
    ov_json_value *params = ov_event_api_set_parameter(msg);
    ov_json_value *type_json = ov_json_string(type_str);

    if ((!ov_ptr_valid(type_str,
                       "Cannot create notify message: No type given")) ||
        (!ov_ptr_valid(params, "Could not create basic notify message")) ||
        (!ov_ptr_valid(type_json,
                       "Cannot create notify message: No type given")) ||
        (!ov_json_object_set(params, OV_KEY_TYPE, type_json))) {

        msg = ov_json_value_free(msg);
        type_json = ov_json_value_free(type_json);
        return 0;

    } else {

        return msg;
    }
}

/*----------------------------------------------------------------------------*/

static ov_json_value *notify_new_call(ov_notify_type type, char const *uuid,
                                      char const *id, char const *peer,
                                      char const *loop) {

    ov_json_value *msg = notify_message(uuid, ov_notify_type_to_string(type));
    ov_json_value *params = ov_event_api_get_parameter(msg);

    if ((!ov_ptr_valid(id, "Cannot notify lieges: Invalid ID")) ||
        (!ov_ptr_valid(params,
                       "Cannot notify lieges: Could not create fresh signaling "
                       "message")) ||
        (!ov_json_object_set(params, OV_KEY_ID, ov_json_string(id)))) {

        msg = ov_json_value_free(msg);

    } else if ((0 != peer) && (!ov_json_object_set(params, OV_KEY_PEER,
                                                   ov_json_string(peer)))) {

        ov_log_error("Could not set " OV_KEY_PEER " on notification message");
        msg = ov_json_value_free(msg);

    } else if ((0 != loop) && (!ov_json_object_set(params, OV_KEY_LOOP,
                                                   ov_json_string(loop)))) {

        ov_log_error("Could not set " OV_KEY_PEER " on notification message");
        msg = ov_json_value_free(msg);
    }

    return msg;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *notify_call_terminated(char const *uuid, char const *id) {

    ov_json_value *msg =
        notify_message(uuid, ov_notify_type_to_string(NOTIFY_CALL_TERMINATED));
    ov_json_value *params = ov_event_api_get_parameter(msg);

    if ((!ov_ptr_valid(id, "Cannot notify lieges: Invalid ID")) ||
        (!ov_ptr_valid(params,
                       "Cannot notify lieges: Could not create fresh signaling "
                       "message")) ||
        (!ov_json_object_set(params, OV_KEY_ID, ov_json_string(id)))) {

        msg = ov_json_value_free(msg);
    }

    return msg;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *notify_new_recording(char const *uuid,
                                           ov_recording recording) {

    ov_json_value *msg =
        notify_message(uuid, ov_notify_type_to_string(NOTIFY_NEW_RECORDING));
    ov_json_value *params = ov_event_api_get_parameter(msg);

    ov_json_value *jrecording = ov_recording_to_json(recording);

    if ((!ov_ptr_valid(params,
                       "Cannot notify lieges: Could not create fresh signaling "
                       "message")) ||
        (!ov_json_object_set(params, OV_KEY_RECORDING, jrecording))) {

        jrecording = ov_json_value_free(jrecording);
        msg = ov_json_value_free(msg);
    }

    return msg;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *entity_to_json(char const *name, char const *type) {

    ov_json_value *entity = ov_json_object();

    if ((!ov_ptr_valid(name, "Invalid entity name (0 pointer)")) ||
        (!ov_ptr_valid(type, "Invalid entity name (0 pointer)")) ||
        (!ov_json_object_set(entity, OV_KEY_NAME, ov_json_string(name))) ||
        (!ov_json_object_set(entity, OV_KEY_TYPE, ov_json_string(type)))) {

        entity = ov_json_value_free(entity);
    }

    return entity;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *notify_entity_lost(char const *uuid,
                                         char const *entity_name,
                                         char const *entity_type) {

    ov_json_value *msg =
        notify_message(uuid, ov_notify_type_to_string(NOTIFY_ENTITY_LOST));

    ov_json_value *params = ov_event_api_get_parameter(msg);

    ov_json_value *entity_json = entity_to_json(entity_name, entity_type);

    if ((!ov_ptr_valid(params,
                       "Cannot notify lieges: Could not create fresh signaling "
                       "message")) ||
        (!ov_json_object_set(params, OV_KEY_ENTITY, entity_json))) {

        entity_json = ov_json_value_free(entity_json);
        msg = ov_json_value_free(msg);
    }

    return msg;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_notify_message(char const *uuid, ov_notify_type notify_type,
                                 ov_notify_parameters parameters) {

    switch (notify_type) {

    case NOTIFY_NEW_CALL:

        return notify_new_call(NOTIFY_NEW_CALL, uuid, parameters.call.id,
                               parameters.call.peer, parameters.call.loop);

    case NOTIFY_INCOMING_CALL:

        return notify_new_call(NOTIFY_INCOMING_CALL, uuid, parameters.call.id,
                               parameters.call.peer, parameters.call.loop);

    case NOTIFY_CALL_TERMINATED:

        return notify_call_terminated(uuid, parameters.call.id);

    case NOTIFY_NEW_RECORDING:

        return notify_new_recording(uuid, parameters.recording);

    case NOTIFY_ENTITY_LOST:

        return notify_entity_lost(uuid, parameters.entity.name,
                                  parameters.entity.type);

    case NOTIFY_INVALID:
    default:

        ov_log_error("Cannot create notify message: Invalid type");
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ov_notify_type parse_entity_lost(ov_json_value const *params,
                                        ov_notify_parameters *notify_params) {

    ov_json_value const *entity = ov_json_get(params, "/" OV_KEY_ENTITY);

    if (!ov_ptr_valid(entity, "Cannot parse entity_lost event: No entity")) {

        return NOTIFY_INVALID;
    }
    if (0 != notify_params) {

        notify_params->entity.type =
            ov_json_string_get(ov_json_get(entity, "/" OV_KEY_TYPE));
        notify_params->entity.name =
            ov_json_string_get(ov_json_get(entity, "/" OV_KEY_NAME));

        return NOTIFY_ENTITY_LOST;

    } else {

        return NOTIFY_ENTITY_LOST;
    }
}

/*----------------------------------------------------------------------------*/

static ov_notify_type parse_call(ov_json_value const *params,
                                 ov_notify_parameters *notify_params,
                                 ov_notify_type return_type) {

    if (0 != notify_params) {

        notify_params->call.id =
            ov_json_string_get(ov_json_get(params, "/" OV_KEY_ID));
        notify_params->call.peer =
            ov_json_string_get(ov_json_get(params, "/" OV_KEY_PEER));
        notify_params->call.loop =
            ov_json_string_get(ov_json_get(params, "/" OV_KEY_LOOP));
    }

    return return_type;
}

/*----------------------------------------------------------------------------*/

static ov_notify_type parse_new_recording(ov_json_value const *params,
                                          ov_notify_parameters *notify_params,
                                          ov_notify_type return_type) {

    ov_json_value const *jrecording = ov_json_get(params, "/" OV_KEY_RECORDING);

    if (0 != jrecording) {

        notify_params->recording = ov_recording_from_json(jrecording);
        return return_type;

    } else {

        return NOTIFY_INVALID;
    }
}

/*----------------------------------------------------------------------------*/

ov_notify_type ov_notify_parse(ov_json_value const *parameters,
                               ov_notify_parameters *notify_params) {

    ov_notify_type type = ov_notify_type_from_string(
        ov_json_string_get(ov_json_get(parameters, "/" OV_KEY_TYPE)));

    switch (type) {

    case NOTIFY_INVALID:
    default:

        return NOTIFY_INVALID;

    case NOTIFY_ENTITY_LOST:
        return parse_entity_lost(parameters, notify_params);

    case NOTIFY_NEW_CALL:
        return parse_call(parameters, notify_params, NOTIFY_NEW_CALL);

    case NOTIFY_INCOMING_CALL:
        return parse_call(parameters, notify_params, NOTIFY_INCOMING_CALL);

    case NOTIFY_CALL_TERMINATED:
        return parse_call(parameters, notify_params, NOTIFY_CALL_TERMINATED);

    case NOTIFY_NEW_RECORDING:
        return parse_new_recording(parameters, notify_params,
                                   NOTIFY_NEW_RECORDING);
    }

    return type;
}

/*----------------------------------------------------------------------------*/
