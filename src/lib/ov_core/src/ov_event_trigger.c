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
        @file           ov_event_trigger.c
        @author         Markus

        @date           2024-09-25


        ------------------------------------------------------------------------
*/
#include "../include/ov_event_trigger.h"

#include <ov_base/ov_dict.h>
#include <ov_base/ov_string.h>

/*----------------------------------------------------------------------------*/

#define OV_EVENT_TRIGGER_MAGIC_BYTES 0xefe4

/*----------------------------------------------------------------------------*/

struct ov_event_trigger {

    uint16_t magic_bytes;
    ov_event_trigger_config config;

    ov_dict *events;
};

/*----------------------------------------------------------------------------*/

ov_event_trigger *ov_event_trigger_create(ov_event_trigger_config config) {

    ov_event_trigger *trigger = NULL;

    trigger = calloc(1, sizeof(ov_event_trigger));
    if (!trigger) goto error;

    trigger->magic_bytes = OV_EVENT_TRIGGER_MAGIC_BYTES;
    trigger->config = config;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = ov_data_pointer_free;

    trigger->events = ov_dict_create(d_config);
    if (!trigger->events) goto error;

    return trigger;
error:
    ov_event_trigger_free(trigger);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_event_trigger *ov_event_trigger_cast(const void *self) {

    if (!self) goto error;

    if (*(uint16_t *)self == OV_EVENT_TRIGGER_MAGIC_BYTES)
        return (ov_event_trigger *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

void *ov_event_trigger_free(void *self) {

    ov_event_trigger *trigger = ov_event_trigger_cast(self);
    if (!trigger) return self;

    trigger->events = ov_dict_free(trigger->events);
    trigger = ov_data_pointer_free(trigger);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_event_trigger_register_listener(ov_event_trigger *self,
                                        const char *key,
                                        ov_event_trigger_data data) {

    if (!self || !key || !data.userdata || !data.process) goto error;

    char *k = ov_string_dup(key);
    ov_event_trigger_data *val = calloc(1, sizeof(ov_event_trigger_data));
    *val = data;

    if (!ov_dict_set(self->events, k, val, NULL)) {

        k = ov_data_pointer_free(k);
        val = ov_data_pointer_free(val);
        goto error;
    }

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_event_trigger_send(ov_event_trigger *self,
                           const char *key,
                           ov_json_value *event) {

    if (!self || !key || !event) goto error;

    ov_event_trigger_data *data = ov_dict_get(self->events, key);
    if (!data) goto error;

    if (!data->process) goto error;

    data->process(data->userdata, event);
    return true;
error:
    return false;
}
