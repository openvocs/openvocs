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
        @file           ov_vocs_record.c
        @author         Markus Töpfer

        @date           2024-01-26


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_record.h"

#include <ov_base/ov_string.h>
#include <ov_base/ov_time.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vocs_record *ov_vocs_record_create(ov_vocs_record_config config) {

    ov_vocs_record *self = calloc(1, sizeof(ov_vocs_record));
    if (!self)
        goto error;

    self->config = config;

error:
    return self;
}

/*----------------------------------------------------------------------------*/

void *ov_vocs_record_free_void(void *self) {

    if (!self)
        return NULL;

    ov_vocs_record *record = (ov_vocs_record *)self;

    if (record->config.callback.stop)
        record->config.callback.stop(record->config.callback.userdata, record);

    record->data = ov_json_value_free(record->data);
    record->active.uri = ov_free(record->active.uri);
    record = ov_data_pointer_free(record);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_record_set_active(ov_vocs_record *self, char const *id,
                               char const *loop, char const *uri,
                               int recorder_fh) {

    if (ov_ptr_valid(self,
                     "Cannot set recording active - no recording object") &&
        ov_ptr_valid(id, "Cannot set recording active - no ID") &&
        ov_ptr_valid(loop, "Cannot set recording active - no loop") &&
        ov_ptr_valid(uri, "Cannot set recording active - no URI")) {

        self->active.running = true;
        ov_id_set(self->active.id, id);
        self->active.started_at_epoch_secs = time(0);
        self->active.recorder = recorder_fh;

        ov_free(self->active.uri);
        self->active.uri = ov_string_dup(uri);

        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_record_reset_active(ov_vocs_record *self) {

    if (ov_ptr_valid(self, "Cannot reset record entry - no entry")) {

        self->active.running = false;
        ov_id_clear(self->active.id);
        self->active.recorder = -1;
        self->active.uri = ov_free(self->active.uri);

        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_record_add_ptt(ov_vocs_record *self, const char *session) {

    if (!self || !session)
        goto error;

    if (!self->data)
        self->data = ov_json_object();

    ov_json_value *ptt = ov_json_object_get(self->data, OV_KEY_PTT);

    if (!ptt) {

        ptt = ov_json_object();
        if (!ov_json_object_set(self->data, OV_KEY_PTT, ptt)) {
            ptt = ov_json_value_free(ptt);
            goto error;
        }
    }

    if (!ov_json_object_set(ptt, session, ov_json_true()))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_record_drop_ptt(ov_vocs_record *self, const char *session) {

    if (!self || !session)
        goto error;

    ov_json_value *ptt = ov_json_object_get(self->data, OV_KEY_PTT);
    if (!ptt)
        goto done;

    return ov_json_object_del(ptt, session);

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

int64_t ov_vocs_record_get_ptt_count(const ov_vocs_record *self) {

    int64_t result = 0;

    if (!self)
        goto error;

    ov_json_value *ptt = ov_json_object_get(self->data, OV_KEY_PTT);
    if (!ptt)
        goto done;

    result = ov_json_object_count(ptt);

done:
    return result;
error:
    return -1;
}
