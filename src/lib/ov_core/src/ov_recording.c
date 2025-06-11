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
#include "ov_recording.h"
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_id.h>
#include <ov_base/ov_json_pointer.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_utils.h>

#define END_EPOCH_SECS "end_epoch_secs"
#define START_EPOCH_SECS "start_epoch_secs"

/*----------------------------------------------------------------------------*/

bool ov_recording_set(ov_recording *self,
                      char const *id,
                      char const *loop,
                      char const *uri,
                      time_t start_epoch_secs,
                      time_t end_epoch_secs) {

    if (ov_ptr_valid(self, "Cannot set recording - no recording")) {

        self->id = ov_string_dup(id);
        self->loop = ov_string_dup(loop);
        self->uri = ov_string_dup(uri);
        self->start_epoch_secs = start_epoch_secs;
        self->end_epoch_secs = end_epoch_secs;
    }

    return 0 != self;
}

/*----------------------------------------------------------------------------*/

bool ov_recording_clear(ov_recording *self) {

    if (0 != self) {

        self->id = ov_free(self->id);
        self->loop = ov_free(self->loop);
        self->uri = ov_free(self->uri);
        self->start_epoch_secs = 0;
        self->end_epoch_secs = 0;
    }

    return 0 != self;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_recording_to_json(ov_recording recording) {

    if (ov_ptr_valid(recording.id,
                     "Cannot notify lieges about new recording: Invalid "
                     "recording ID") &&
        ov_ptr_valid(recording.loop,
                     "Cannot notify lieges about new recording: Invalid "
                     "loop") &&
        ov_ptr_valid(recording.uri,
                     "Cannot notify lieges about new recording: Invalid "
                     "URI")) {

        ov_json_value *jval = ov_json_object();

        ov_json_object_set(jval, OV_KEY_ID, ov_json_string(recording.id));
        ov_json_object_set(jval, OV_KEY_LOOP, ov_json_string(recording.loop));
        ov_json_object_set(jval, OV_KEY_URI, ov_json_string(recording.uri));
        ov_json_object_set(
            jval, START_EPOCH_SECS, ov_json_number(recording.start_epoch_secs));
        ov_json_object_set(
            jval, END_EPOCH_SECS, ov_json_number(recording.end_epoch_secs));

        return jval;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

ov_recording ov_recording_from_json(ov_json_value const *jval) {

    ov_recording rec = {
        .id =
            ov_string_dup(ov_json_string_get(ov_json_get(jval, "/" OV_KEY_ID))),
        .loop = ov_string_dup(
            ov_json_string_get(ov_json_get(jval, "/" OV_KEY_LOOP))),
        .uri = ov_string_dup(
            ov_json_string_get(ov_json_get(jval, "/" OV_KEY_URI))),
        .start_epoch_secs =
            ov_json_number_get(ov_json_get(jval, "/" START_EPOCH_SECS)),
        .end_epoch_secs =
            ov_json_number_get(ov_json_get(jval, "/" END_EPOCH_SECS)),
    };

    if ((!ov_ptr_valid(rec.id, "Could not read recording ID")) ||
        (!ov_ptr_valid(rec.loop, "Could not read recording loop")) ||
        (!ov_ptr_valid(rec.uri, "Could not read recording URI"))) {

        rec.id = ov_free(rec.id);
        rec.loop = ov_free(rec.loop);
        rec.uri = ov_free(rec.uri);
    }

    return rec;
}

/*----------------------------------------------------------------------------*/
