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
        @file           ov_event_connection.c
        @author         Markus

        @date           2024-01-09


        ------------------------------------------------------------------------
*/
#include "../include/ov_event_connection.h"

#define OV_EVENT_CONNECTION_MAGIC_BYTES 0xef98

/*----------------------------------------------------------------------------*/

struct ov_event_connection {

    uint16_t magic_bytes;
    ov_event_connection_config config;

    ov_json_value *data;
};

/*----------------------------------------------------------------------------*/

ov_event_connection *
ov_event_connection_create(ov_event_connection_config config) {

    ov_event_connection *self = calloc(1, sizeof(ov_event_connection));
    if (!self)
        goto error;

    self->magic_bytes = OV_EVENT_CONNECTION_MAGIC_BYTES;
    self->config = config;

    self->data = ov_json_object();
    if (!self->data)
        goto error;

    return self;
error:
    ov_event_connection_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_event_connection *ov_event_connection_free(ov_event_connection *self) {

    if (!ov_event_connection_cast(self))
        goto error;

    self->data = ov_json_value_free(self->data);
    self = ov_data_pointer_free(self);
error:
    return self;
}

/*----------------------------------------------------------------------------*/

void *ov_event_connection_free_void(void *self) {

    ov_event_connection *c = ov_event_connection_cast(self);
    if (!c)
        return self;

    return ov_event_connection_free(c);
}

/*----------------------------------------------------------------------------*/

ov_event_connection *ov_event_connection_cast(const void *self) {

    if (!self)
        goto error;

    if (*(uint16_t *)self == OV_EVENT_CONNECTION_MAGIC_BYTES)
        return (ov_event_connection *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_event_connection_send(ov_event_connection *self,
                              const ov_json_value *json) {

    if (!self || !json)
        goto error;

    return ov_event_io_send(&self->config.params, self->config.socket, json);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

int ov_event_connection_get_socket(ov_event_connection *self) {

    if (!self)
        return -1;
    return self->config.socket;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC DATA
 *
 *      ------------------------------------------------------------------------
 */

bool ov_event_connection_set(ov_event_connection *self, const char *key,
                             const char *value) {

    if (!self || !key || !value)
        goto error;

    ov_json_value *str = ov_json_string(value);
    if (!ov_json_object_set(self->data, key, str)) {
        str = ov_json_value_free(str);
        goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

const char *ov_event_connection_get(ov_event_connection *self,
                                    const char *key) {

    if (!self || !key)
        goto error;

    return ov_json_string_get(ov_json_object_get(self->data, key));
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_event_connection_set_json(ov_event_connection *self, const char *key,
                                  const ov_json_value *value) {

    if (!self || !key || !value)
        goto error;

    ov_json_value *copy = NULL;
    if (!ov_json_value_copy((void **)&copy, value))
        goto error;

    if (!ov_json_object_set(self->data, key, copy)) {
        copy = ov_json_value_free(copy);
        goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

const ov_json_value *ov_event_connection_get_json(ov_event_connection *self,
                                                  const char *key) {

    if (!self || !key)
        goto error;

    return ov_json_object_get(self->data, key);
error:
    return NULL;
}
