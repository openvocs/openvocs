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
        @file           ov_mc_backend.c
        @author         Markus Töpfer

        @date           2023-01-21


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_backend.h"

#include "../include/ov_mc_backend_registry.h"
#include "../include/ov_mc_mixer_data.h"
#include "../include/ov_mc_mixer_msg.h"

#define OV_MC_BACKEND_MAGIC_BYTES 0xdefa

#include <ov_base/ov_convert.h>
#include <ov_base/ov_error_codes.h>
#include <ov_base/ov_id.h>
#include <ov_core/ov_callback.h>
#include <ov_core/ov_event_app.h>

/*----------------------------------------------------------------------------*/

struct ov_mc_backend {

    uint16_t magic_bytes;
    ov_mc_backend_config config;

    ov_callback_registry *callbacks;

    struct {

        int manager;

    } socket;

    ov_event_app *app;

    ov_mc_backend_registry *registry;
};

/*----------------------------------------------------------------------------*/

static bool cb_accept(void *userdata, int listener, int connection) {

    // accept any connection

    UNUSED(userdata);
    UNUSED(listener);
    UNUSED(connection);
    return true;
}

/*----------------------------------------------------------------------------*/

static void close_mixer_data(ov_mc_backend *self, int socket) {

    ov_mc_mixer_data data =
        ov_mc_backend_registry_get_socket(self->registry, socket);

    ov_mc_backend_registry_unregister_mixer(self->registry, socket);

    if (0 != data.user[0]) {

        if (self->config.callback.mixer.lost)
            self->config.callback.mixer.lost(self->config.callback.userdata,
                                             data.user);
    }

    return;
}

/*----------------------------------------------------------------------------*/

static void close_mixer(ov_mc_backend *self, int socket) {

    close_mixer_data(self, socket);
    ov_mc_backend_registry_unregister_mixer(self->registry, socket);
    ov_event_app_close(self->app, socket);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_close(void *userdata, int connection) {

    ov_mc_backend *self = ov_mc_backend_cast(userdata);
    UNUSED(self);

    ov_log_debug("Socket close received at %i", connection);

    close_mixer_data(self, connection);
    return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      APP FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_event_register(void *userdata, const char *name, int socket,
                              ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_backend *self = ov_mc_backend_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    const char *uuid = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_UUID));

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    if (!uuid || !type)
        goto error;

    if (0 != strcmp(type, OV_KEY_AUDIO))
        goto error;

    ov_mc_mixer_data data = ov_mc_mixer_data_set_socket(socket);
    strncpy(data.uuid, uuid, sizeof(ov_id));

    if (!ov_mc_backend_registry_register_mixer(self->registry, data))
        goto error;

    out = ov_mc_mixer_msg_configure(self->config.mixer.config);
    if (!out)
        goto error;

    if (!ov_event_app_send(self->app, socket, out))
        goto error;

    ov_json_value_free(input);
    ov_json_value_free(out);
    return;

error:
    if (self)
        close_mixer(self, socket);
    ov_json_value_free(input);
    ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_acquire_response(ov_mc_backend *self, int socket,
                                      ov_json_value *input) {

    if (!self || !input)
        goto error;

    uint64_t code = 0;
    const char *desc = NULL;
    ov_event_api_get_error_parameter(input, &code, &desc);
    ov_json_value *request = ov_event_api_get_request(input);

    const char *uuid = ov_event_api_get_uuid(input);
    const char *user_uuid = ov_mc_mixer_msg_aquire_get_username(request);

    if (!uuid || !user_uuid)
        goto error;

    if (0 != code)
        ov_mc_backend_registry_release_user(self->registry, user_uuid);

    ov_callback cb = ov_callback_registry_unregister(self->callbacks, uuid);

    if (cb.function) {

        ov_mc_backend_cb_mixer function = cb.function;
        function((void *)cb.userdata, uuid, user_uuid, code, desc);

    } else {

        // let mixer reconnect for clean state
        close_mixer(self, socket);
    }

    input = ov_json_value_free(input);
    return;

error:
    if (self)
        close_mixer(self, socket);
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_acquire(void *userdata, const char *name, int socket,
                             ov_json_value *input) {

    ov_mc_backend *self = ov_mc_backend_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input))
        return cb_event_acquire_response(self, socket, input);

    // we do not expext some acquire, which is not a response

error:
    if (self)
        close_mixer(self, socket);
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_release_response(ov_mc_backend *self, int socket,
                                      ov_json_value *input) {

    if (!self || !input)
        goto error;

    uint64_t code = 0;
    const char *desc = NULL;
    ov_event_api_get_error_parameter(input, &code, &desc);

    const char *uuid = ov_event_api_get_uuid(input);
    const char *user_uuid =
        ov_mc_mixer_msg_aquire_get_username(ov_event_api_get_request(input));

    if (!uuid || !user_uuid)
        goto error;

    if (0 != code) {
        ov_event_app_close(self->app, socket);
    } else {
        if (!ov_mc_backend_registry_release_user(self->registry, user_uuid))
            ov_event_app_close(self->app, socket);
    }

    ov_callback cb = ov_callback_registry_unregister(self->callbacks, uuid);

    if (cb.function) {

        ov_mc_backend_cb_mixer function = cb.function;
        function((void *)cb.userdata, uuid, user_uuid, code, desc);
    }

    input = ov_json_value_free(input);
    return;

error:
    if (self)
        close_mixer(self, socket);
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_release(void *userdata, const char *name, int socket,
                             ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_backend *self = ov_mc_backend_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input))
        return cb_event_release_response(self, socket, input);

    // we do not expext some release, which is not a response

error:
    if (self)
        close_mixer(self, socket);
    ov_json_value_free(input);
    ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_join_response(ov_mc_backend *self, int socket,
                                   ov_json_value *input) {

    if (!self || !input)
        goto error;

    uint64_t code = 0;
    const char *desc = NULL;
    ov_event_api_get_error_parameter(input, &code, &desc);

    const char *uuid = ov_event_api_get_uuid(input);

    ov_json_value *request = ov_event_api_get_request(input);
    if (!request)
        goto error;

    ov_mc_loop_data data = ov_mc_mixer_msg_join_from_json(request);
    ov_mc_mixer_data mdata =
        ov_mc_backend_registry_get_socket(self->registry, socket);

    OV_ASSERT(uuid);

    ov_callback cb = ov_callback_registry_unregister(self->callbacks, uuid);

    if (cb.function) {

        ov_mc_backend_cb_loop function = cb.function;
        function((void *)cb.userdata, uuid, mdata.user, data.name, code, desc);

    } else {

        // let mixer reconnect for clean state
        close_mixer(self, socket);
    }

    input = ov_json_value_free(input);
    return;

error:
    if (self)
        close_mixer(self, socket);
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_join(void *userdata, const char *name, int socket,
                          ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_backend *self = ov_mc_backend_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input))
        return cb_event_join_response(self, socket, input);

    // we do not expext some join, which is not a response

error:
    if (self)
        close_mixer(self, socket);
    ov_json_value_free(input);
    ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_state_response(ov_mc_backend *self, int socket,
                                    ov_json_value *input) {

    if (!self || !input)
        goto error;

    const char *uuid = ov_event_api_get_uuid(input);

    ov_callback cb = ov_callback_registry_unregister(self->callbacks, uuid);

    if (cb.function) {

        ov_mc_backend_cb_state function = cb.function;
        function((void *)cb.userdata, uuid, ov_event_api_get_response(input));
    }

    input = ov_json_value_free(input);
    return;

error:
    if (self)
        close_mixer(self, socket);
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_state(void *userdata, const char *name, int socket,
                           ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_backend *self = ov_mc_backend_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input))
        return cb_event_state_response(self, socket, input);

    // we do not expext some join, which is not a response

error:
    if (self)
        close_mixer(self, socket);
    ov_json_value_free(input);
    ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_leave_response(ov_mc_backend *self, int socket,
                                    ov_json_value *input) {

    if (!self || !input)
        goto error;

    uint64_t code = 0;
    const char *desc = NULL;
    ov_event_api_get_error_parameter(input, &code, &desc);

    const char *uuid = ov_event_api_get_uuid(input);

    ov_json_value *request = ov_event_api_get_request(input);
    if (!request)
        goto error;

    const char *loop = ov_mc_mixer_msg_leave_from_json(request);
    ov_mc_mixer_data mdata =
        ov_mc_backend_registry_get_socket(self->registry, socket);

    ov_callback cb = ov_callback_registry_unregister(self->callbacks, uuid);

    if (cb.function) {

        ov_mc_backend_cb_loop function = cb.function;
        function((void *)cb.userdata, uuid, mdata.user, loop, code, desc);
    }

    input = ov_json_value_free(input);
    return;

error:
    if (self)
        close_mixer(self, socket);
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_leave(void *userdata, const char *name, int socket,
                           ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_backend *self = ov_mc_backend_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input))
        return cb_event_leave_response(self, socket, input);

    // we do not expext some leave, which is not a response

error:
    if (self)
        close_mixer(self, socket);
    ov_json_value_free(input);
    ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_volume_response(ov_mc_backend *self, int socket,
                                     ov_json_value *input) {

    if (!self || !input)
        goto error;

    uint64_t code = 0;
    const char *desc = NULL;
    ov_event_api_get_error_parameter(input, &code, &desc);

    const char *uuid = ov_event_api_get_uuid(input);

    ov_json_value *request = ov_event_api_get_request(input);
    const char *loop = ov_mc_mixer_msg_volume_get_name(request);
    uint8_t vol = ov_mc_mixer_msg_volume_get_volume(request);

    ov_mc_mixer_data mdata =
        ov_mc_backend_registry_get_socket(self->registry, socket);

    ov_callback cb = ov_callback_registry_unregister(self->callbacks, uuid);

    if (cb.function) {

        ov_mc_backend_cb_volume function = cb.function;
        function((void *)cb.userdata, uuid, mdata.user, loop, vol, code, desc);
    }

    input = ov_json_value_free(input);
    return;

error:
    if (self)
        close_mixer(self, socket);
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_volume(void *userdata, const char *name, int socket,
                            ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_backend *self = ov_mc_backend_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input))
        return cb_event_volume_response(self, socket, input);

    // we do not expext some volume, which is not a response

error:
    if (self)
        close_mixer(self, socket);
    ov_json_value_free(input);
    ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_forward(void *userdata, const char *name, int socket,
                             ov_json_value *input) {

    ov_json_value *out = NULL;

    uint64_t code = 0;
    const char *desc = NULL;

    ov_mc_backend *self = ov_mc_backend_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    const char *uuid = ov_event_api_get_uuid(input);
    ov_event_api_get_error_parameter(input, &code, &desc);

    ov_callback cb = ov_callback_registry_unregister(self->callbacks, uuid);

    ov_mc_mixer_data mdata =
        ov_mc_backend_registry_get_socket(self->registry, socket);

    if (cb.function) {

        ov_mc_backend_cb_loop function = cb.function;
        function((void *)cb.userdata, uuid, mdata.user, NULL, code, desc);
    }

    ov_json_value_free(input);
    ov_json_value_free(out);
    return;

error:
    if (self)
        close_mixer(self, socket);
    ov_json_value_free(input);
    ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static bool register_app_callbacks(ov_mc_backend *self) {

    OV_ASSERT(self);

    if (!ov_event_app_register(self->app, OV_KEY_REGISTER, self,
                               cb_event_register))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_ACQUIRE, self,
                               cb_event_acquire))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_RELEASE, self,
                               cb_event_release))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_JOIN, self, cb_event_join))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_LEAVE, self, cb_event_leave))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_VOLUME, self, cb_event_volume))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_STATE, self, cb_event_state))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_FORWARD, self,
                               cb_event_forward))
        goto error;

    return true;
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_backend *ov_mc_backend_create(ov_mc_backend_config config) {

    ov_mc_backend *self = NULL;

    if (!config.loop)
        goto error;
    if (0 == config.timeout.request_usec)
        config.timeout.request_usec = OV_MC_BACKEND_DEFAULT_TIMEOUT;

    self = calloc(1, sizeof(ov_mc_backend));
    if (!self)
        goto error;

    self->magic_bytes = OV_MC_BACKEND_MAGIC_BYTES;
    self->config = config;

    ov_event_app_config app_config =
        (ov_event_app_config){.io = config.io,
                              .callbacks.userdata = self,
                              .callbacks.accept = cb_accept,
                              .callbacks.close = cb_close};

    self->app = ov_event_app_create(app_config);
    if (!self->app)
        goto error;

    self->socket.manager = ov_event_app_open_listener(
        self->app,
        (ov_io_socket_config){.socket = self->config.socket.manager});

    if (-1 == self->socket.manager) {

        ov_log_error("Failed to open manager socket %s:%i",
                     self->config.socket.manager.host,
                     self->config.socket.manager.port);

        goto error;
    }

    if (!register_app_callbacks(self))
        goto error;

    self->registry =
        ov_mc_backend_registry_create((ov_mc_backend_registry_config){0});

    if (!self->registry)
        goto error;

    self->callbacks = ov_callback_registry_create(
        (ov_callback_registry_config){.loop = self->config.loop});

    if (!self->callbacks)
        goto error;
    return self;
error:
    ov_mc_backend_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend *ov_mc_backend_free(ov_mc_backend *self) {

    if (!ov_mc_backend_cast(self))
        return self;

    self->app = ov_event_app_free(self->app);
    self->callbacks = ov_callback_registry_free(self->callbacks);
    self->registry = ov_mc_backend_registry_free(self->registry);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend *ov_mc_backend_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_MC_BACKEND_MAGIC_BYTES)
        return NULL;

    return (ov_mc_backend *)data;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_acquire_mixer(ov_mc_backend *self, const char *uuid,
                                 const char *user_uuid,
                                 ov_mc_mixer_core_forward data, void *userdata,
                                 ov_mc_backend_cb_mixer callback) {

    ov_json_value *msg = NULL;

    if (!self || !user_uuid || !uuid)
        goto error;
    if (!userdata || !callback)
        goto error;

    ov_mc_mixer_data mixer =
        ov_mc_backend_registry_acquire_user(self->registry, user_uuid);

    if (0 >= mixer.socket) {

        callback((void *)userdata, uuid, user_uuid, OV_ERROR_NO_RESOURCE,
                 OV_ERROR_DESC_NO_RESOURCE);

        goto done;
    }

    if (!ov_callback_registry_register(
            self->callbacks, uuid,
            (ov_callback){.userdata = userdata, .function = callback},
            self->config.timeout.request_usec))
        goto error;

    msg = ov_mc_mixer_msg_acquire(user_uuid, data);
    if (!msg)
        goto rollback;

    if (!ov_event_api_set_uuid(msg, uuid))
        goto rollback;

    if (!ov_event_app_send(self->app, mixer.socket, msg))
        goto rollback;

    msg = ov_json_value_free(msg);

done:
    return true;

rollback:
    ov_mc_backend_registry_release_user(self->registry, user_uuid);
error:
    msg = ov_json_value_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_set_mixer_forward(ov_mc_backend *self, const char *uuid,
                                     const char *user_uuid,
                                     ov_mc_mixer_core_forward data,
                                     void *userdata,
                                     ov_mc_backend_cb_mixer callback) {

    ov_json_value *msg = NULL;

    if (!self || !user_uuid || !uuid)
        goto error;
    if (!userdata || !callback)
        goto error;

    ov_mc_mixer_data mixer =
        ov_mc_backend_registry_get_user(self->registry, user_uuid);

    if (0 >= mixer.socket) {

        callback((void *)userdata, uuid, user_uuid, OV_ERROR_CODE_UNKNOWN,
                 OV_ERROR_DESC_UNKNOWN);

        goto done;
    }

    if (!ov_callback_registry_register(
            self->callbacks, uuid,
            (ov_callback){.userdata = userdata, .function = callback},
            self->config.timeout.request_usec))
        goto error;

    msg = ov_mc_mixer_msg_forward(user_uuid, data);
    if (!msg)
        goto error;

    if (!ov_event_api_set_uuid(msg, uuid))
        goto error;

    if (!ov_event_app_send(self->app, mixer.socket, msg))
        goto error;
    msg = ov_json_value_free(msg);

done:
    return true;

error:
    msg = ov_json_value_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_release_mixer(ov_mc_backend *self, const char *uuid,
                                 const char *user_uuid, void *userdata,
                                 ov_mc_backend_cb_mixer callback) {

    ov_json_value *msg = NULL;

    if (!self || !user_uuid || !uuid)
        goto error;
    if (!userdata || !callback)
        goto error;

    ov_mc_mixer_data mixer =
        ov_mc_backend_registry_get_user(self->registry, user_uuid);

    if (0 >= mixer.socket) {

        callback((void *)userdata, uuid, user_uuid, 0, NULL);

        goto done;
    }

    if (0 >= mixer.socket)
        goto done;

    if (!ov_callback_registry_register(
            self->callbacks, uuid,
            (ov_callback){.userdata = userdata, .function = callback},
            self->config.timeout.request_usec))
        goto error;

    msg = ov_mc_mixer_msg_release(user_uuid);
    if (!msg)
        goto error;

    if (!ov_event_api_set_uuid(msg, uuid))
        goto error;

    if (!ov_event_app_send(self->app, mixer.socket, msg)) {
        ov_event_app_close(self->app, mixer.socket);
    }

    msg = ov_json_value_free(msg);

done:
    return ov_mc_backend_registry_release_user(self->registry, user_uuid);

error:
    msg = ov_json_value_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool send_message(ov_mc_backend *self, const char *user_uuid,
                         const ov_json_value *msg) {

    if (!self || !user_uuid || !msg)
        goto error;

    ov_mc_mixer_data mixer =
        ov_mc_backend_registry_get_user(self->registry, user_uuid);

    if (0 >= mixer.socket)
        goto error;

    if (!ov_event_app_send(self->app, mixer.socket, msg))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_join_loop(ov_mc_backend *self, const char *uuid,
                             const char *user_uuid, ov_mc_loop_data data,
                             void *userdata, ov_mc_backend_cb_loop callback) {

    ov_json_value *msg = NULL;

    if (!self || !uuid || !user_uuid)
        goto error;

    if (!userdata || !callback)
        goto error;

    if (!ov_callback_registry_register(
            self->callbacks, uuid,
            (ov_callback){.userdata = userdata, .function = callback},
            self->config.timeout.request_usec))
        goto error;

    msg = ov_mc_mixer_msg_join(data);
    if (!msg)
        goto error;

    if (!ov_event_api_set_uuid(msg, uuid))
        goto error;

    if (!send_message(self, user_uuid, msg))
        goto error;

    msg = ov_json_value_free(msg);
    return true;
error:
    msg = ov_json_value_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_leave_loop(ov_mc_backend *self, const char *uuid,
                              const char *user_uuid, const char *loopname,
                              void *userdata, ov_mc_backend_cb_loop callback) {

    ov_json_value *msg = NULL;

    if (!self || !uuid || !user_uuid || !loopname)
        goto error;

    if (!userdata || !callback)
        goto error;

    if (!ov_callback_registry_register(
            self->callbacks, uuid,
            (ov_callback){.userdata = userdata, .function = callback},
            self->config.timeout.request_usec))
        goto error;

    msg = ov_mc_mixer_msg_leave(loopname);
    if (!msg)
        goto error;

    if (!ov_event_api_set_uuid(msg, uuid))
        goto error;

    if (!send_message(self, user_uuid, msg))
        goto error;

    msg = ov_json_value_free(msg);
    return true;
error:
    msg = ov_json_value_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_set_loop_volume(ov_mc_backend *self, const char *uuid,
                                   const char *user_uuid, const char *loopname,
                                   uint8_t volume_percent, void *userdata,
                                   ov_mc_backend_cb_volume callback) {

    ov_json_value *msg = NULL;

    if (!self || !uuid || !user_uuid || !loopname)
        goto error;

    if (!userdata || !callback)
        goto error;

    if (!ov_callback_registry_register(
            self->callbacks, uuid,
            (ov_callback){.userdata = userdata, .function = callback},
            self->config.timeout.request_usec))
        goto error;

    uint8_t volume_scale = ov_convert_from_vol_percent(volume_percent, 3);

    msg = ov_mc_mixer_msg_volume(loopname, volume_scale);
    if (!msg)
        goto error;

    if (!ov_event_api_set_uuid(msg, uuid))
        goto error;

    if (!send_message(self, user_uuid, msg))
        goto error;

    msg = ov_json_value_free(msg);
    return true;
error:
    msg = ov_json_value_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_config ov_mc_backend_config_from_json(const ov_json_value *val) {

    ov_mc_backend_config config = (ov_mc_backend_config){0};

    const ov_json_value *data = ov_json_object_get(val, OV_KEY_BACKEND);
    if (!data)
        data = val;

    config.socket.manager = ov_socket_configuration_from_json(
        ov_json_get(data, "/" OV_KEY_SOCKET "/" OV_KEY_MANAGER),
        (ov_socket_configuration){0});

    const ov_json_value *par = ov_json_get(data, "/" OV_KEY_MIXER);

    config.mixer.config.vad.zero_crossings_rate_threshold_hertz =
        ov_json_number_get(ov_json_get(
            par, "/" OV_KEY_VAD "/" OV_KEY_ZERO_CROSSINGS_RATE_HERTZ));

    config.mixer.config.vad.powerlevel_density_threshold_db =
        ov_json_number_get(
            ov_json_get(par, "/" OV_KEY_VAD "/" OV_KEY_POWERLEVEL_DENSITY_DB));

    if (ov_json_is_true(ov_json_get(par, "/" OV_KEY_VAD "/" OV_KEY_ENABLED))) {
        config.mixer.config.incoming_vad = true;
    } else {
        config.mixer.config.incoming_vad = false;
    }

    config.mixer.config.samplerate_hz =
        ov_json_number_get(ov_json_get(par, "/" OV_KEY_SAMPLE_RATE_HERTZ));

    config.mixer.config.comfort_noise_max_amplitude =
        ov_json_number_get(ov_json_get(par, "/" OV_KEY_COMFORT_NOISE));

    config.mixer.config.max_num_frames_to_mix =
        ov_json_number_get(ov_json_get(par, "/" OV_KEY_MAX_NUM_FRAMES));

    config.mixer.config.limit.frame_buffer_max =
        ov_json_number_get(ov_json_get(par, "/" OV_KEY_FRAME_BUFFER));

    if (ov_json_is_true(ov_json_get(par, "/" OV_KEY_NORMALIZE_INPUT))) {
        config.mixer.config.normalize_input = true;
    } else {
        config.mixer.config.normalize_input = false;
    }

    if (ov_json_is_true(ov_json_get(par, "/" OV_KEY_RTP_KEEPALIVE))) {
        config.mixer.config.rtp_keepalive = true;
    } else {
        config.mixer.config.rtp_keepalive = false;
    }

    if (ov_json_is_true(ov_json_get(par, "/" OV_KEY_ROOT_MIX))) {
        config.mixer.config.normalize_mixing_result_by_square_root = true;
    } else {
        config.mixer.config.normalize_mixing_result_by_square_root = false;
    }

    return config;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_registry_count ov_mc_backend_state_mixers(ov_mc_backend *self) {

    if (!self)
        goto error;

    return ov_mc_backend_registry_count_mixers(self->registry);

error:
    return (ov_mc_backend_registry_count){0};
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_get_session_state(ov_mc_backend *self, const char *uuid,
                                     const char *session_id, void *userdata,
                                     ov_mc_backend_cb_state callback) {

    ov_json_value *msg = NULL;

    if (!self || !uuid || !session_id)
        goto error;

    if (!userdata || !callback)
        goto error;

    if (!ov_callback_registry_register(
            self->callbacks, uuid,
            (ov_callback){.userdata = userdata, .function = callback},
            self->config.timeout.request_usec))
        goto error;

    msg = ov_mc_mixer_msg_state();
    if (!msg)
        goto error;

    if (!ov_event_api_set_uuid(msg, uuid))
        goto error;

    if (!send_message(self, session_id, msg))
        goto error;

    msg = ov_json_value_free(msg);
    return true;
error:

    msg = ov_json_value_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/
