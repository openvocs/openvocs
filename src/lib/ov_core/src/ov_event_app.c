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
        @file           ov_event_app.c
        @author         Markus Töpfer

        @date           2024-12-28


        ------------------------------------------------------------------------
*/
#include "../include/ov_event_app.h"

#include "../include/ov_event_api.h"

#include <ov_base/ov_dict.h>
#include <ov_base/ov_json_io_buffer.h>
#include <ov_base/ov_string.h>

#define ov_event_app_MAGIC_BYTES 0xeff0

/*----------------------------------------------------------------------------*/

struct ov_event_app {

    uint16_t magic_bytes;
    ov_event_app_config config;

    ov_json_io_buffer *io_buffer;
    ov_dict *registry;
};

/*----------------------------------------------------------------------------*/

typedef struct Callback {

    void *userdata;
    void (*callback)(void *userdata,
                     const char *name,
                     int socket,
                     ov_json_value *input);

} Callback;

/*----------------------------------------------------------------------------*/

static void json_io_success(void *userdata, int socket, ov_json_value *val) {

    ov_event_app *self = ov_event_app_cast(userdata);
    if (!self || !socket || !val) goto error;

    const char *event = ov_event_api_get_event(val);
    if (!event) goto error;

    Callback *cb = ov_dict_get(self->registry, event);
    if (!cb) goto error;

    cb->callback(cb->userdata, event, socket, val);
    return;

error:
    if (self) ov_io_close(self->config.io, socket);
    ov_json_value_free(val);
    return;
}

/*----------------------------------------------------------------------------*/

static void json_io_failure(void *userdata, int socket) {

    ov_event_app *self = ov_event_app_cast(userdata);
    if (!self || !socket) return;

    ov_io_close(self->config.io, socket);
    return;
}

/*----------------------------------------------------------------------------*/

ov_event_app *ov_event_app_create(ov_event_app_config config) {

    ov_event_app *self = NULL;

    if (!config.io) goto error;

    self = calloc(1, sizeof(ov_event_app));
    if (!self) goto error;

    self->magic_bytes = ov_event_app_MAGIC_BYTES;
    self->config = config;

    ov_json_io_buffer_config json_buffer_config =
        (ov_json_io_buffer_config){.callback.userdata = self,
                                   .callback.success = json_io_success,
                                   .callback.failure = json_io_failure};

    self->io_buffer = ov_json_io_buffer_create(json_buffer_config);
    if (!self->io_buffer) goto error;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = ov_data_pointer_free;
    self->registry = ov_dict_create(d_config);
    if (!self->registry) goto error;

    return self;
error:
    ov_event_app_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_event_app *ov_event_app_free(ov_event_app *self) {

    if (!ov_event_app_cast(self)) return self;

    self->io_buffer = ov_json_io_buffer_free(self->io_buffer);
    self->registry = ov_dict_free(self->registry);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_event_app *ov_event_app_cast(const void *data) {

    if (!data) return NULL;

    if (*(uint16_t *)data != ov_event_app_MAGIC_BYTES) return NULL;

    return (ov_event_app *)data;
}

/*----------------------------------------------------------------------------*/

bool ov_event_app_register(ov_event_app *app,
                           const char *name,
                           void *userdata,
                           void (*callback)(void *userdata,
                                            const char *name,
                                            int socket,
                                            ov_json_value *input)) {

    char *key = NULL;
    Callback *val = NULL;

    if (!app || !name || !userdata || !callback) goto error;

    key = ov_string_dup(name);
    val = calloc(1, sizeof(Callback));

    if (!key || !val) goto error;

    val->userdata = userdata;
    val->callback = callback;

    if (!ov_dict_set(app->registry, key, val, NULL)) goto error;
    return true;
error:
    key = ov_data_pointer_free(key);
    val = ov_data_pointer_free(val);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_event_app_deregister(ov_event_app *app, const char *name) {

    if (!app || !name) return false;
    return ov_dict_del(app->registry, name);
}

/*----------------------------------------------------------------------------*/

bool ov_event_app_push(ov_event_app *self, int socket, ov_json_value *json) {

    if (!self || !socket || !json) return false;
    json_io_success(self, socket, json);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool io_accept(void *userdata, int listener, int connection) {

    ov_event_app *self = ov_event_app_cast(userdata);

    if (self->config.callbacks.accept)
        return self->config.callbacks.accept(
            self->config.callbacks.userdata, listener, connection);

    return true;
}

/*----------------------------------------------------------------------------*/

static void io_close_socket(void *userdata, int connection) {

    ov_event_app *self = ov_event_app_cast(userdata);
    if (!self) return;

    ov_json_io_buffer_drop(self->io_buffer, connection);

    if (self->config.callbacks.close)
        self->config.callbacks.close(
            self->config.callbacks.userdata, connection);

    return;
}

/*----------------------------------------------------------------------------*/

static void io_connected(void *userdata, int connection) {

    ov_event_app *self = ov_event_app_cast(userdata);

    if (self->config.callbacks.connected)
        self->config.callbacks.connected(
            self->config.callbacks.userdata, connection);

    return;
}

/*----------------------------------------------------------------------------*/

static bool io_socket(void *userdata,
                      int connection,
                      const char *domain,
                      const ov_memory_pointer data) {

    UNUSED(domain);

    ov_event_app *self = ov_event_app_cast(userdata);
    return ov_json_io_buffer_push(self->io_buffer, connection, data);
}

/*----------------------------------------------------------------------------*/

int ov_event_app_open_listener(ov_event_app *self, ov_io_socket_config config) {

    if (!self) return -1;

    config.callbacks.userdata = self;
    config.callbacks.io = io_socket;
    config.callbacks.accept = io_accept;
    config.callbacks.close = io_close_socket;

    return ov_io_open_listener(self->config.io, config);
}

/*----------------------------------------------------------------------------*/

int ov_event_app_open_connection(ov_event_app *self,
                                 ov_io_socket_config config) {

    if (!self) return -1;

    config.callbacks.userdata = self;
    config.callbacks.io = io_socket;
    config.callbacks.close = io_close_socket;
    config.callbacks.connected = io_connected;

    return ov_io_open_connection(self->config.io, config);
}

/*----------------------------------------------------------------------------*/

bool ov_event_app_send(ov_event_app *self,
                       int socket,
                       const ov_json_value *msg) {

    if (!self || !socket || !msg) goto error;

    char *string = ov_json_value_to_string(msg);
    if (!string) goto error;

    bool r = ov_io_send(self->config.io,
                        socket,
                        (ov_memory_pointer){.start = (uint8_t *)string,
                                            .length = strlen(string)});

    string = ov_data_pointer_free(string);
    return r;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_event_app_close(ov_event_app *self, int socket) {

    if (!self || !socket) goto error;

    return ov_io_close(self->config.io, socket);
error:
    return false;
}