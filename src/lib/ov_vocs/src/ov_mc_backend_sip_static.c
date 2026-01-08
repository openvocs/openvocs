/***
        ------------------------------------------------------------------------

        Copyright (c) 2025 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_mc_backend_sip_static_static.c
        @author         Töpfer, Markus

        @date           2025-04-14


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_backend_sip_static.h"
#include "../include/ov_mc_sip_msg.h"

#include <ov_base/ov_dict.h>
#include <ov_base/ov_error_codes.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_time.h>

#include <ov_core/ov_event_api.h>
#include <ov_core/ov_event_app.h>
#include <ov_core/ov_event_async.h>

#define OV_MC_BACKEND_SIP_STATIC_MAGIC_BYTES 0xd324

struct ov_mc_backend_sip_static {

    uint16_t magic_bytes;
    ov_mc_backend_sip_static_config config;

    struct {

        int manager;

    } socket;

    struct {

        ov_dict *mixer;

    } resources;

    ov_event_app *app;
    ov_event_async_store *async;
};

/*----------------------------------------------------------------------------*/

static void cb_async_timedout(void *userdata, ov_event_async_data data) {

    ov_mc_backend_sip_static *self = ov_mc_backend_sip_static_cast(userdata);
    if (!self)
        goto error;

    char *str = ov_json_value_to_string(data.value);
    ov_log_error("async timeout %i | %s", data.socket, str);
    str = ov_data_pointer_free(str);

    ov_json_value *out = ov_event_api_create_error_response(
        data.value, OV_ERROR_TIMEOUT, OV_ERROR_DESC_TIMEOUT);
    ov_event_app_send(self->app, data.socket, out);
    out = ov_json_value_free(out);

error:
    ov_event_async_data_clear(&data);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_mixer_released(void *userdata, const char *uuid,
                              const char *user_uuid, uint64_t error_code,
                              const char *error_desc) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;
    ov_event_async_data adata = {0};

    ov_mc_backend_sip_static *self = ov_mc_backend_sip_static_cast(userdata);
    if (!self)
        goto error;

    adata = ov_event_async_unset(self->async, uuid);

    // no async call set, ignore released message
    if (!adata.value)
        goto error;

    switch (error_code) {

    case OV_ERROR_NO_ERROR:

        out = ov_event_api_create_success_response(adata.value);

        par = ov_event_api_get_response(out);
        val = ov_json_string(user_uuid);
        if (!ov_json_object_set(par, OV_KEY_USER, val))
            goto error;
        val = NULL;

        break;

    default:

        out = ov_event_api_create_error_response(adata.value, error_code,
                                                 error_desc);

        par = ov_event_api_get_response(out);
        val = ov_json_string(user_uuid);
        if (!ov_json_object_set(par, OV_KEY_USER, val))
            goto error;
        val = NULL;
    }

    ov_event_app_send(self->app, adata.socket, out);

error:
    ov_event_async_data_clear(&adata);
    ov_json_value_free(out);
    ov_json_value_free(val);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_mixer_aquired(void *userdata, const char *uuid,
                             const char *user_uuid, uint64_t error_code,
                             const char *error_desc) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;
    ov_event_async_data adata = {0};

    ov_mc_backend_sip_static *self = ov_mc_backend_sip_static_cast(userdata);
    if (!self)
        goto error;

    adata = ov_event_async_unset(self->async, uuid);

    // reverse timeout acquire
    if (!adata.value) {

        if (OV_ERROR_NO_ERROR == error_code) {

            ov_mc_backend_release_mixer(self->config.backend, uuid, user_uuid,
                                        self, cb_mixer_released);

        } else {

            // ignore timeout and error in acquisition
            goto error;
        }
    }

    switch (error_code) {

    case OV_ERROR_NO_ERROR:

        out = ov_event_api_create_success_response(adata.value);

        par = ov_event_api_get_response(out);
        val = ov_json_string(user_uuid);
        if (!ov_json_object_set(par, OV_KEY_USER, val))
            goto error;
        val = NULL;

        break;

    default:

        out = ov_event_api_create_error_response(adata.value, error_code,
                                                 error_desc);

        par = ov_event_api_get_response(out);
        val = ov_json_string(user_uuid);
        if (!ov_json_object_set(par, OV_KEY_USER, val))
            goto error;
        val = NULL;
    }

    ov_event_app_send(self->app, adata.socket, out);

error:
    ov_event_async_data_clear(&adata);
    ov_json_value_free(out);
    ov_json_value_free(val);
    return;
}

/*----------------------------------------------------------------------------*/

static bool drop_mixer(const void *key, void *val, void *data) {

    if (!key)
        return true;
    UNUSED(val);
    ov_mc_backend_sip_static *self = ov_mc_backend_sip_static_cast(data);

    ov_id id = {0};
    ov_id_fill_with_uuid(id);

    ov_mc_backend_release_mixer(self->config.backend, id, key, self,
                                cb_mixer_released);

    return true;
}

/*----------------------------------------------------------------------------*/

static void asign_sip_mixer(ov_mc_backend_sip_static *self, int socket,
                            const char *name) {

    ov_dict *res = ov_dict_get(self->resources.mixer, (void *)(intptr_t)socket);

    if (!res) {

        res = ov_dict_create(ov_dict_string_key_config(255));
        ov_dict_set(self->resources.mixer, (void *)(intptr_t)socket, res, NULL);
    }

    char *key = ov_string_dup(name);
    ov_dict_set(res, key, NULL, NULL);

    return;
}

/*----------------------------------------------------------------------------*/

static void drop_sip_mixer_assignment(ov_mc_backend_sip_static *self,
                                      int socket, const char *name) {

    ov_dict *res = ov_dict_get(self->resources.mixer, (void *)(intptr_t)socket);

    if (res)
        ov_dict_del(res, name);

    return;
}

/*----------------------------------------------------------------------------*/

static void drop_mixer_resources(ov_mc_backend_sip_static *self, int socket) {

    ov_dict *res = ov_dict_get(self->resources.mixer, (void *)(intptr_t)socket);
    if (!res)
        goto done;

    ov_dict_for_each(res, self, drop_mixer);

done:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_close(void *userdata, int connection) {

    ov_mc_backend_sip_static *self = ov_mc_backend_sip_static_cast(userdata);
    UNUSED(self);

    ov_log_debug("Socket close received at %i", connection);

    drop_mixer_resources(self, connection);
    return;
}

/*----------------------------------------------------------------------------*/

static bool cb_accept(void *userdata, int listener, int connection) {

    // accept any connection

    UNUSED(userdata);
    UNUSED(listener);
    UNUSED(connection);
    return true;
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

    ov_mc_backend_sip_static *self = ov_mc_backend_sip_static_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    ov_log_debug("New STATIC SIP gateway register at %i", socket);

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_get_multicast(void *userdata, const char *name, int socket,
                                   ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_backend_sip_static *self = ov_mc_backend_sip_static_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));

    if (!loop) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_socket_configuration config =
        ov_vocs_db_get_multicast_group(self->config.db, loop);

    if (0 == config.host[0]) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_UNKNOWN_LOOP, OV_ERROR_DESC_UNKNOWN_LOOP);

        goto response;
    }

    out = ov_mc_sip_msg_get_multicast_response(input, config);

    if (0 == out) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_INTERNAL_SERVER, OV_ERROR_DESC_INTERNAL_SERVER);
        goto error;
    }

response:

    ov_event_app_send(self->app, socket, out);

error:
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_acquire(void *userdata, const char *name, int socket,
                             ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_backend_sip_static *self = ov_mc_backend_sip_static_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    ov_mc_mixer_core_forward forward = ov_mc_sip_msg_get_loop_socket(input);

    const char *uuid = ov_event_api_get_uuid(input);
    const char *user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!uuid || !user)
        goto error;

    // set some default forward if forward is not set
    if (0 == forward.socket.host[0]) {

        forward = (ov_mc_mixer_core_forward){.socket.host = "127.0.0.1",
                                             .socket.port = 12345,
                                             .socket.type = UDP,
                                             .payload_type = 100,
                                             .ssrc = 12345

        };
    }

    if (!ov_mc_backend_acquire_mixer(self->config.backend, uuid, user, forward,
                                     self, cb_mixer_aquired)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_PROCESSING_ERROR,
            OV_ERROR_DESC_PROCESSING_ERROR);

        ov_event_app_send(self->app, socket, out);
        out = ov_json_value_free(out);
        goto error;
    }

    asign_sip_mixer(self, socket, user);

    if (!ov_event_async_set(
            self->async, uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = self,
                                  .timedout.callback = cb_async_timedout},
            10000000))
        goto error;

    input = NULL;
error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_release(void *userdata, const char *name, int socket,
                             ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_backend_sip_static *self = ov_mc_backend_sip_static_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    const char *uuid = ov_event_api_get_uuid(input);
    const char *user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!uuid || !user)
        goto error;

    if (!ov_mc_backend_release_mixer(self->config.backend, uuid, user, self,
                                     cb_mixer_released)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_PROCESSING_ERROR,
            OV_ERROR_DESC_PROCESSING_ERROR);

        ov_event_app_send(self->app, socket, out);
        out = ov_json_value_free(out);
        goto error;
    }

    drop_sip_mixer_assignment(self, socket, user);

    if (!ov_event_async_set(
            self->async, uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = self,
                                  .timedout.callback = cb_async_timedout},
            10000000))
        goto error;

    input = NULL;

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_loop_joined(void *userdata, const char *uuid,
                           const char *user_uuid, const char *loop_name,
                           uint64_t error_code, const char *error_desc) {

    ov_mc_backend_sip_static *self = ov_mc_backend_sip_static_cast(userdata);

    ov_log_debug("%s joined %s - err %" PRIu64 " %s", user_uuid, loop_name,
                 error_code, error_desc);

    UNUSED(uuid);
    UNUSED(self);

    return;
}

/*----------------------------------------------------------------------------*/

static void cb_mixer_forwarded(void *userdata, const char *uuid,
                               const char *user_uuid, uint64_t error_code,
                               const char *error_desc) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;
    ov_event_async_data adata = {0};

    ov_mc_backend_sip_static *self = ov_mc_backend_sip_static_cast(userdata);
    if (!self)
        goto error;

    adata = ov_event_async_unset(self->async, uuid);

    // reverse timeout acquire
    if (!adata.value) {

        if (OV_ERROR_NO_ERROR == error_code) {

            ov_mc_backend_release_mixer(self->config.backend, uuid, user_uuid,
                                        self, cb_mixer_released);

        } else {

            // ignore timeout and error in acquisition
            goto error;
        }
    }

    switch (error_code) {

    case OV_ERROR_NO_ERROR:

        out = ov_event_api_create_success_response(adata.value);

        par = ov_event_api_get_response(out);
        val = ov_json_string(user_uuid);
        if (!ov_json_object_set(par, OV_KEY_USER, val))
            goto error;
        val = NULL;

        break;

    default:

        out = ov_event_api_create_error_response(adata.value, error_code,
                                                 error_desc);

        par = ov_event_api_get_response(out);
        val = ov_json_string(user_uuid);
        if (!ov_json_object_set(par, OV_KEY_USER, val))
            goto error;
        val = NULL;
    }

    ov_event_app_send(self->app, adata.socket, out);

error:
    ov_event_async_data_clear(&adata);
    ov_json_value_free(out);
    ov_json_value_free(val);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_set_singlecast(void *userdata, const char *name,
                                    int socket, ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_backend_sip_static *self = ov_mc_backend_sip_static_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    const char *uuid = ov_event_api_get_uuid(input);
    const char *user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));
    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));

    if (!uuid || !user || !loop)
        goto error;

    ov_socket_configuration socket_config =
        ov_vocs_db_get_multicast_group(self->config.db, loop);

    if (0 == socket_config.host[0]) {
        ov_log_error("Could not get multicast group");
        goto error;
    }

    ov_mc_mixer_core_forward forward = ov_mc_sip_msg_get_loop_socket(input);

    ov_mc_loop_data data = {0};
    data.socket = socket_config;
    strncpy(data.name, loop, OV_MC_LOOP_NAME_MAX);
    data.volume = 50;

    if (!ov_mc_mixer_core_forward_data_is_valid(&forward)) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        ov_event_app_send(self->app, socket, out);
        out = ov_json_value_free(out);
        goto error;
    }

    if (!ov_mc_backend_set_mixer_forward(self->config.backend, uuid, user,
                                         forward, self, cb_mixer_forwarded)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_PROCESSING_ERROR,
            OV_ERROR_DESC_PROCESSING_ERROR);

        ov_event_app_send(self->app, socket, out);
        out = ov_json_value_free(out);
        goto error;
    }

    ov_id uuid_new = {0};
    ov_id_fill_with_uuid(uuid_new);

    if (!ov_mc_backend_join_loop(self->config.backend, uuid_new, user, data,
                                 self, cb_loop_joined)) {

        ov_log_error("Failed to send join loop %s for %s", data.name, user);
    }

    if (!ov_event_async_set(
            self->async, uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = self,
                                  .timedout.callback = cb_async_timedout},
            10000000))
        goto error;

    input = NULL;

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static bool register_app_callbacks(ov_mc_backend_sip_static *self) {

    if (!ov_event_app_register(self->app, OV_KEY_REGISTER, self,
                               cb_event_register))
        goto error;

    if (!ov_event_app_register(self->app, OV_SIP_EVENT_GET_MULTICAST, self,
                               cb_event_get_multicast))
        goto error;

    if (!ov_event_app_register(self->app, OV_SIP_EVENT_SET_SINGLECAST, self,
                               cb_event_set_singlecast))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_ACQUIRE, self,
                               cb_event_acquire))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_RELEASE, self,
                               cb_event_release))
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

ov_mc_backend_sip_static *
ov_mc_backend_sip_static_create(ov_mc_backend_sip_static_config config) {

    ov_mc_backend_sip_static *self = NULL;

    if (!config.loop)
        goto error;
    if (!config.backend)
        goto error;
    if (!config.db)
        goto error;

    self = calloc(1, sizeof(ov_mc_backend_sip_static));
    if (!self)
        goto error;

    self->magic_bytes = OV_MC_BACKEND_SIP_STATIC_MAGIC_BYTES;
    self->config = config;

    ov_event_app_config app_config = (ov_event_app_config){

        .io = config.io,
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

    } else {

        ov_log_debug("SIP static opened listener %s:%i",
                     self->config.socket.manager.host,
                     self->config.socket.manager.port);
    }

    if (!register_app_callbacks(self))
        goto error;

    ov_dict_config d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = ov_dict_free;

    self->resources.mixer = ov_dict_create(d_config);
    if (!self->resources.mixer)
        goto error;

    self->async = ov_event_async_store_create(
        (ov_event_async_store_config){.loop = config.loop});

    if (!self->async)
        goto error;

    return self;
error:
    ov_mc_backend_sip_static_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_sip_static *
ov_mc_backend_sip_static_free(ov_mc_backend_sip_static *self) {

    if (!ov_mc_backend_sip_static_cast(self))
        return self;

    self->app = ov_event_app_free(self->app);
    self->resources.mixer = ov_dict_free(self->resources.mixer);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_sip_static *ov_mc_backend_sip_static_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_MC_BACKEND_SIP_STATIC_MAGIC_BYTES)
        return NULL;

    return (ov_mc_backend_sip_static *)data;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_sip_static_config
ov_mc_backend_sip_static_config_from_json(const ov_json_value *val) {

    ov_mc_backend_sip_static_config config =
        (ov_mc_backend_sip_static_config){0};

    const ov_json_value *data = ov_json_object_get(val, OV_KEY_SIP_STATIC);
    if (!data)
        data = val;

    config.socket.manager = ov_socket_configuration_from_json(
        ov_json_get(data, "/" OV_KEY_SOCKET "/" OV_KEY_MANAGER),
        (ov_socket_configuration){0});

    return config;
}