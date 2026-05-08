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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "../include/ov_recorder_events.h"

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_json_pointer.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_utils.h>

/*----------------------------------------------------------------------------*/

#define SILENCE_CUTOFF_INTERVAL "silence_cutoff_msecs"

/*****************************************************************************
                                 recorder start
 ****************************************************************************/

bool ov_recorder_event_start_clear(ov_recorder_event_start *event) {
    if (ov_ptr_valid(event, "Cannot clear event - no event given")) {
        event->loop = ov_free(event->loop);
        event->mc_ip = ov_free(event->mc_ip);
        event->mc_port = 0;
        return true;

    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_event_start_equal(ov_recorder_event_start const *event1,
                                   ov_recorder_event_start const *event2) {
    if (0 == event1) {
        return (0 == event2);

    } else if (0 == event2) {
        return (0 == event1);

    } else {
        return (event1->mc_port == event2->mc_port) &&
               ov_string_equal(event1->loop, event2->loop) &&
               ov_string_equal(event1->mc_ip, event2->mc_ip) &&
               (event1->vad.powerlevel_density_threshold_db ==
                event2->vad.powerlevel_density_threshold_db) &&
               (event1->vad.zero_crossings_rate_threshold_hertz ==
                event2->vad.zero_crossings_rate_threshold_hertz) &&
               (event1->silence_cutoff_interval_msecs ==
                event2->silence_cutoff_interval_msecs);
    }
}

/*----------------------------------------------------------------------------*/

static bool json_add_string(ov_json_value *json, char const *key,
                            char const *value) {
    ov_json_value *jvalue = ov_json_string(value);

    if (ov_json_object_set(json, key, jvalue)) {
        return true;
    } else {
        jvalue = ov_json_value_free(jvalue);
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool json_add_uint16(ov_json_value *json, char const *key,
                            uint16_t value) {
    ov_json_value *jvalue = ov_json_number(value);

    if (ov_json_object_set(json, key, jvalue)) {
        return true;
    } else {
        jvalue = ov_json_value_free(jvalue);
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool json_add_uint64(ov_json_value *json, char const *key,
                            uint64_t value) {
    ov_json_value *jvalue = ov_json_number(value);

    if (ov_json_object_set(json, key, jvalue)) {
        return true;
    } else {
        jvalue = ov_json_value_free(jvalue);
        return false;
    }
}

/*----------------------------------------------------------------------------*/

bool add_vad_if_required(ov_json_value *target, ov_vad_config vad_params) {
    return (vad_params.powerlevel_density_threshold_db == 0) ||
           (vad_params.zero_crossings_rate_threshold_hertz == 0) ||
           ov_vad_config_to_json(vad_params, target);
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_event_start_to_json(ov_json_value *target,
                                     ov_recorder_event_start const *event) {
    return ov_ptr_valid(target,
                        "Cannot turn Recorder start event to JSON - no target "
                        "JSON object") &&
           ov_ptr_valid(event,
                        "Cannot turn Recorder start event to JSON - no event "
                        "given") &&
           ov_ptr_valid(event->loop,
                        "Cannot turn Recorder start event to JSON - invalid "
                        "loop "
                        "name") &&
           ov_ptr_valid(event->mc_ip,
                        "Cannot turn Recorder start event to JSON - invalid "
                        "multicast ip") &&
           json_add_uint16(target, OV_KEY_PORT, event->mc_port) &&
           json_add_uint16(target, OV_KEY_ROLL_AFTER_SECS,
                           event->roll_after_secs) &&
           json_add_string(target, OV_KEY_LOOP, event->loop) &&
           json_add_string(target, OV_KEY_MULTICAST, event->mc_ip) &&
           add_vad_if_required(target, event->vad) &&
           ((0 == event->silence_cutoff_interval_msecs) ||
            json_add_uint64(target, SILENCE_CUTOFF_INTERVAL,
                            event->silence_cutoff_interval_msecs));
}

/*----------------------------------------------------------------------------*/

static int32_t json_get_uint16(ov_json_value const *json, char const *key) {
    ov_json_value const *jnumber = ov_json_get(json, key);
    double number = ov_json_number_get(jnumber);

    if (0 == jnumber) {
        return 0;

    } else if (ov_json_is_number(jnumber) && (-1 < number) &&
               (UINT16_MAX >= number)) {
        return (int32_t)number;

    } else {
        return -1;
    }
}

/*----------------------------------------------------------------------------*/

static int64_t json_get_uint64(ov_json_value const *json, char const *key) {
    ov_json_value const *jnumber = ov_json_get(json, key);
    double number = ov_json_number_get(jnumber);

    if (0 == jnumber) {
        return 0;

    } else if (ov_json_is_number(jnumber) && (-1 < number)) {
        return (int64_t)number;

    } else {
        return -1;
    }
}

/*----------------------------------------------------------------------------*/

static bool set_vad_from(ov_json_value const *jval, ov_vad_config *cfg) {
    if ((0 == jval) || (0 == cfg)) {
        return false;
    } else if (0 == ov_json_get(jval, "/" OV_KEY_VAD)) {
        return true;
    } else {
        *cfg = ov_vad_config_from_json(jval);
        return true;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_event_start_from_json(ov_json_value const *json,
                                       ov_recorder_event_start *event) {
    char const *loop = ov_json_string_get(ov_json_get(json, "/" OV_KEY_LOOP));

    char const *mc_ip =
        ov_json_string_get(ov_json_get(json, "/" OV_KEY_MULTICAST));

    int32_t port = json_get_uint16(json, "/" OV_KEY_PORT);
    int32_t roll_after_secs = json_get_uint16(json, "/" OV_KEY_ROLL_AFTER_SECS);
    int64_t silence_cutoff_interval_msecs =
        json_get_uint64(json, "/" SILENCE_CUTOFF_INTERVAL);

    if (ov_ptr_valid(json,
                     "Cannot read recorder start event form JSON - no JSON") &&
        ov_ptr_valid(event,
                     "Cannot read recorder start event form JSON - no target "
                     "event") &&
        ov_cond_valid(0 == event->loop,
                      "Cannot read recorder start event from JSON - target "
                      "event is not empty") &&
        ov_cond_valid(0 == event->mc_ip,
                      "Cannot read recorder start event from JSON - target "
                      "event is not empty") &&
        ov_ptr_valid(loop,
                     "Cannot read recorder start event form JSON - JSON does "
                     "not contain valid " OV_KEY_LOOP) &&
        ov_ptr_valid(mc_ip,
                     "Cannot read recorder start event form JSON - JSON does "
                     "not contain valid " OV_KEY_MULTICAST) &&
        ov_cond_valid(-1 < port,
                      "Cannot read recorder start event from JSON - Port is "
                      "invalid") &&
        set_vad_from(json, &event->vad)) {
        event->loop = ov_string_dup(loop);
        event->mc_ip = ov_string_dup(mc_ip);
        event->mc_port = (uint16_t)port;
        event->roll_after_secs = 0;

        if (0 < roll_after_secs) {
            event->roll_after_secs = roll_after_secs;
        }

        if (0 < silence_cutoff_interval_msecs) {
            event->silence_cutoff_interval_msecs =
                silence_cutoff_interval_msecs;
        }

        return true;

    } else {
        return false;
    }
}

/*****************************************************************************
                            recorder start response
 ****************************************************************************/

bool ov_recorder_response_start_clear(ov_recorder_response_start *response) {
    if (ov_ptr_valid(response, "Cannot clear response - no event given")) {
        response->filename = ov_free(response->filename);
        ov_id_clear(response->id);
        return true;

    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_response_start_equal(ov_recorder_response_start const *resp1,
                                      ov_recorder_response_start const *resp2) {
    if (0 == resp1) {
        return (0 == resp2);

    } else if (0 == resp2) {
        return (0 == resp1);

    } else {
        return ov_string_equal(resp1->filename, resp2->filename) &&
               ov_string_equal(resp1->id, resp2->id);
    }
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_response_start_to_json(
    ov_json_value *target, ov_recorder_response_start const *response) {
    return ov_ptr_valid(target,
                        "Cannot turn Recorder start response to JSON - no "
                        "target "
                        "JSON object") &&
           ov_ptr_valid(response,
                        "Cannot turn Recorder start response to JSON - no "
                        "event "
                        "given") &&
           ov_cond_valid(ov_id_valid(response->id),
                         "Cannot turn Recorder start response to JSON - "
                         "invalid id") &&
           json_add_string(target, OV_KEY_UUID, response->id) &&
           ((0 == response->filename) ||
            json_add_string(target, OV_KEY_FILE, response->filename));
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_response_start_from_json(
    ov_json_value const *json, ov_recorder_response_start *response) {
    char const *filename =
        ov_json_string_get(ov_json_get(json, "/" OV_KEY_FILE));
    char const *id = ov_json_string_get(ov_json_get(json, "/" OV_KEY_UUID));

    if (ov_ptr_valid(
            json, "Cannot read recorder start response form JSON - no JSON") &&
        ov_ptr_valid(response,
                     "Cannot read recorder start response form JSON - no "
                     "target "
                     "response") &&
        ov_cond_valid(0 == response->filename,
                      "Cannot read recorder start response from JSON - target "
                      "response is not empty") &&
        ov_ptr_valid(filename,
                     "Cannot read recorder start response form JSON - JSON "
                     "does "
                     "not contain valid " OV_KEY_FILE) &&
        ov_ptr_valid(id, "Cannot read recorder start response form JSON - JSON "
                         "does "
                         "not contain valid " OV_KEY_UUID)) {
        response->filename = ov_string_dup(filename);
        ov_string_copy(response->id, id, sizeof(response->id));

        return true;

    } else {
        return false;
    }
}

/*****************************************************************************
                                 recorder stop
 ****************************************************************************/

bool ov_recorder_event_stop_clear(ov_recorder_event_stop *event) {
    if (ov_ptr_valid(event, "Cannot clear event - no event given")) {
        ov_id_clear(event->id);
        return true;

    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_event_stop_equal(ov_recorder_event_stop const *event1,
                                  ov_recorder_event_stop const *event2) {
    if (0 == event1) {
        return (0 == event2);

    } else if (0 == event2) {
        return (0 == event1);

    } else {
        return ov_string_equal(event1->id, event2->id);
    }
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_event_stop_to_json(ov_json_value *target,
                                    ov_recorder_event_stop const *event) {
    return ov_ptr_valid(target, "Cannot turn Recorder stop event to JSON - no "
                                "target "
                                "JSON object") &&
           ov_ptr_valid(event, "Cannot turn Recorder stop event to JSON - no "
                               "event "
                               "given") &&
           ov_cond_valid(ov_id_valid(event->id),
                         "Cannot turn Recorder stop event to JSON - "
                         "invalid "
                         "id ") &&
           json_add_string(target, OV_KEY_UUID, event->id);
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_event_stop_from_json(ov_json_value const *json,
                                      ov_recorder_event_stop *event) {
    char const *id = ov_json_string_get(ov_json_get(json, "/" OV_KEY_UUID));

    return ov_ptr_valid(json, "Cannot read recorder stop event form JSON - no "
                              "JSON") &&
           ov_ptr_valid(event, "Cannot read recorder stop event form JSON - no "
                               "target "
                               "event") &&
           ov_ptr_valid(id, "Cannot read recorder stop event form JSON - JSON "
                            "does "
                            "not contain valid " OV_KEY_UUID) &&
           (0 != ov_string_copy(event->id, id, sizeof(ov_id)));
}

/*----------------------------------------------------------------------------*/
