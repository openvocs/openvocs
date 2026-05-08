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
        @file           ov_event_socket.c
        @author         Markus Töpfer

        @date           2023-10-08


        ------------------------------------------------------------------------
*/
#include "../include/ov_event_socket.h"

#include "../include/ov_io_base.h"
#include <ov_base/ov_json_io_buffer.h>

#define OV_EVENT_SOCKET_MAGIC_BYTES 0xe50c

struct ov_event_socket {

    uint16_t magic_bytes;
    ov_event_socket_config config;

    bool debug;

    ov_io_base *io;

    ov_json_io_buffer *io_buffer;
};

/*----------------------------------------------------------------------------*/

static bool event_socket_send(void *userdata, int socket,
                              const ov_json_value *v) {

    ov_event_socket *app = ov_event_socket_cast(userdata);
    if (app->debug) {

        char *str = ov_json_value_to_string(v);
        ov_log_debug("<-- %s", str);
        str = ov_data_pointer_free(str);
    }
    return ov_event_socket_send(app, socket, v);
}

/*----------------------------------------------------------------------------*/

static void json_success(void *userdata, int socket, ov_json_value *value) {

    OV_ASSERT(userdata);
    OV_ASSERT(value);

    if (!userdata || !value)
        goto error;

    ov_event_socket *app = (ov_event_socket *)userdata;
    if (!app)
        goto error;

    if (app->debug) {

        char *str = ov_json_value_to_string(value);
        ov_log_debug("--> %s", str);
        str = ov_data_pointer_free(str);
    }

    if (!ov_event_engine_push(
            app->config.engine, socket,
            (ov_event_parameter){.send.instance = app,
                                 .send.send = event_socket_send

            },
            value))
        goto error;

    return;

error:
    ov_json_value_free(value);
    return;
}

/*----------------------------------------------------------------------------*/

static void json_failure(void *userdata, int socket) {

    OV_ASSERT(userdata);
    OV_ASSERT(socket >= 0);

    ov_event_socket *app = (ov_event_socket *)userdata;
    if (!app)
        goto error;

    ov_event_socket_close(app, socket);

error:
    return;
}

/*----------------------------------------------------------------------------*/

ov_event_socket *ov_event_socket_create(ov_event_socket_config config) {

    ov_event_socket *self = NULL;

    if (!config.loop)
        goto error;
    if (!config.engine)
        goto error;

    self = calloc(1, sizeof(ov_event_socket));
    if (!self)
        goto error;

    self->magic_bytes = OV_EVENT_SOCKET_MAGIC_BYTES;

    ov_io_base_config base_config = {

        .debug = false,

        .loop = config.loop,

        .timer.io_timeout_usec = config.timer.io_timeout_usec,
        .timer.accept_to_io_timeout_usec =
            config.timer.accept_to_io_timeout_usec,
        .timer.reconnect_interval_usec = config.timer.reconnect_interval_usec,

    };

    self->io = ov_io_base_create(base_config);
    if (!self->io)
        goto error;

    self->io_buffer = ov_json_io_buffer_create(
        (ov_json_io_buffer_config){.objects_only = true,
                                   .callback.userdata = self,
                                   .callback.success = json_success,
                                   .callback.failure = json_failure});

    if (!self->io_buffer)
        goto error;

    self->config = config;

    return self;
error:
    return ov_event_socket_free(self);
}

/*----------------------------------------------------------------------------*/

ov_event_socket *ov_event_socket_free(ov_event_socket *self) {

    if (!ov_event_socket_cast(self))
        goto error;

    self->io = ov_io_base_free(self->io);
    self->io_buffer = ov_json_io_buffer_free(self->io_buffer);
    self = ov_data_pointer_free(self);

error:
    return self;
}

/*----------------------------------------------------------------------------*/

ov_event_socket *ov_event_socket_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data == OV_EVENT_SOCKET_MAGIC_BYTES)
        return (ov_event_socket *)data;

    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_event_socket_load_ssl_config(ov_event_socket *self, const char *path) {

    if (!self || !path)
        goto error;

    return ov_io_base_load_ssl_config(self->io, path);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool event_app_accept_cb(void *userdata, int listener, int connection) {

    ov_event_socket *app = (ov_event_socket *)userdata;
    if (!app)
        return false;

    UNUSED(listener);
    UNUSED(connection);

    /*
        if (app->config.callback.accept)
            return app->config.callback.accept(
                app->config.callback.userdata, listener, connection);
    */

    return true;
}

/*----------------------------------------------------------------------------*/

static void event_app_close_cb(void *userdata, int connection) {

    ov_event_socket *app = (ov_event_socket *)userdata;
    if (!app)
        return;

    if (app->config.callback.close)
        app->config.callback.close(app->config.callback.userdata, connection);

    return;
}

/*----------------------------------------------------------------------------*/

static bool event_app_io_cb(void *userdata, int connection,
                            const ov_memory_pointer input) {

    ov_event_socket *app = (ov_event_socket *)userdata;
    if (!app)
        goto error;

    if (ov_json_io_buffer_push(app->io_buffer, connection, input))
        return true;

error:

    ov_event_socket_close(app, connection);

    return false;
}

/*----------------------------------------------------------------------------*/

int ov_event_socket_create_listener(ov_event_socket *self,
                                    ov_event_socket_server_config config) {

    if (!self || !self->io)
        goto error;

    ov_io_base_listener_config listener = (ov_io_base_listener_config){

        .socket = config.socket,

        .callback.userdata = self,
        .callback.accept = event_app_accept_cb,
        .callback.io = event_app_io_cb,
        .callback.close = event_app_close_cb};

    return ov_io_base_create_listener(self->io, listener);

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static void event_app_connected_cb(void *userdata, int socket, bool result) {

    ov_event_socket *app = (ov_event_socket *)userdata;
    if (!app)
        return;

    if (app->config.callback.connected)
        app->config.callback.connected(app->config.callback.userdata, socket,
                                       result);

    return;
}

/*----------------------------------------------------------------------------*/

int ov_event_socket_create_connection(ov_event_socket *self,
                                      ov_event_socket_client_config config) {

    if (!self || !self->io)
        goto error;

    ov_io_base_connection_config connection = (ov_io_base_connection_config){

        .connection.socket = config.socket,

        .connection.callback.userdata = self,
        .connection.callback.accept = event_app_accept_cb,
        .connection.callback.io = event_app_io_cb,
        .connection.callback.close = event_app_close_cb,

        .client_connect_trigger_usec = config.client_connect_trigger_usec,

        .auto_reconnect = config.auto_reconnect,
        .connected = event_app_connected_cb

    };

    if (0 != config.ssl.domain[0])
        strncat(connection.ssl.domain, config.ssl.domain, PATH_MAX - 1);

    if (0 != config.ssl.ca.path[0])
        strncat(connection.ssl.ca.path, config.ssl.ca.path, PATH_MAX - 1);

    if (0 != config.ssl.ca.file[0])
        strncat(connection.ssl.ca.file, config.ssl.ca.file, PATH_MAX - 1);

    return ov_io_base_create_connection(self->io, connection);

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

bool ov_event_socket_close(ov_event_socket *self, int socket) {

    if (!self)
        goto error;

    return ov_io_base_close(self->io, socket);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_event_socket_send(ov_event_socket *self, int socket,
                          const ov_json_value *value) {

    bool result = false;

    if (!self || !value)
        goto error;

    char *str = ov_json_value_to_string(value);
    if (!str)
        return false;

    size_t size = strlen(str);

    result = ov_io_base_send(
        self->io, socket,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = size});

    str = ov_data_pointer_free(str);

error:
    return result;
}

/*----------------------------------------------------------------------------*/

bool ov_event_socket_set_debug(ov_event_socket *self, bool on) {

    if (!self)
        return false;
    self->debug = on;
    return true;
}
