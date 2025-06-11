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
        @file           ov_vocs_events.c
        @author         Markus Töpfer

        @date           2024-01-22


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_events.h"

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_error_codes.h>

#include <ov_core/ov_event_connection.h>
#include <ov_core/ov_event_engine.h>
#include <ov_core/ov_event_socket.h>

/*----------------------------------------------------------------------------*/

#define OV_VOCS_EVENTS_MAGIC_BYTES 0xef78

/*----------------------------------------------------------------------------*/

struct ov_vocs_events {

    uint16_t magic_bytes;
    ov_vocs_events_config config;

    int socket;

    struct {

        ov_event_engine *engine;
        ov_event_socket *socket;

    } event;

    ov_dict *connections;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      #SOCKET #EVENTS
 *
 *      ------------------------------------------------------------------------
 */

static bool cb_event_register(void *userdata,
                              const int socket,
                              const ov_event_parameter *params,
                              ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_vocs_events *self = ov_vocs_events_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    const char *uuid = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_UUID));

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    if (!uuid || !type) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    intptr_t key = socket;

    ov_event_connection *conn = ov_event_connection_create(
        (ov_event_connection_config){.socket = socket, .params = *params});

    if (!conn || !ov_dict_set(self->connections, (void *)key, conn, NULL)) {

        conn = ov_event_connection_free(conn);

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_log_debug("REGISTER %s %s at %i", type, uuid, socket);

response:

    ov_event_io_send(params, socket, out);

    ov_json_value_free(out);
    ov_json_value_free(input);
    return true;
error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_unregister(void *userdata,
                                const int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_vocs_events *self = ov_vocs_events_cast(userdata);
    if (!self || !socket || !params || !input) goto error;

    intptr_t key = socket;

    ov_dict_del(self->connections, (void *)key);
    ov_log_debug("UNREGISTER %i", socket);

    out = ov_event_api_create_success_response(input);
    ov_event_io_send(params, socket, out);

    ov_json_value_free(out);
    ov_json_value_free(input);
    return true;
error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool register_events(ov_vocs_events *self) {

    if (!self) goto error;

    if (!ov_event_engine_register(
            self->event.engine, OV_KEY_REGISTER, cb_event_register))
        goto error;

    if (!ov_event_engine_register(
            self->event.engine, OV_KEY_UNREGISTER, cb_event_unregister))
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

static void cb_event_socket_close(void *userdata, int socket) {

    ov_vocs_events *self = ov_vocs_events_cast(userdata);

    intptr_t key = socket;
    ov_log_debug("UNREGISTER %i", socket);
    ov_dict_del(self->connections, (void *)key);
    return;
}

/*----------------------------------------------------------------------------*/

static bool check_config(ov_vocs_events_config *config) {

    if (!config->loop) goto error;
    if (0 == config->socket.manager.host[0]) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_vocs_events *ov_vocs_events_create(ov_vocs_events_config config) {

    ov_vocs_events *self = NULL;

    if (!check_config(&config)) goto error;

    self = calloc(1, sizeof(ov_vocs_events));
    if (!self) goto error;

    self->magic_bytes = OV_VOCS_EVENTS_MAGIC_BYTES;
    self->config = config;

    self->event.engine = ov_event_engine_create();
    self->event.socket = ov_event_socket_create(
        (ov_event_socket_config){.loop = config.loop,
                                 .engine = self->event.engine,
                                 .callback.userdata = self,
                                 .callback.close = cb_event_socket_close});

    if (!self->event.socket) goto error;

    ov_dict_config d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = ov_event_connection_free_void;

    self->connections = ov_dict_create(d_config);
    if (!self->connections) goto error;

    self->socket = ov_event_socket_create_listener(
        self->event.socket,
        (ov_event_socket_server_config){.socket = self->config.socket.manager});

    if (-1 == self->socket) goto error;

    if (!register_events(self)) goto error;

    return self;

error:
    ov_vocs_events_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_events *ov_vocs_events_free(ov_vocs_events *self) {

    if (!ov_vocs_events_cast(self)) goto error;

    if (-1 != self->socket) {

        ov_event_loop_unset(self->config.loop, self->socket, NULL);

        close(self->socket);
        self->socket = -1;
    }

    self->event.socket = ov_event_socket_free(self->event.socket);
    self->event.engine = ov_event_engine_free(self->event.engine);
    self->connections = ov_dict_free(self->connections);

    self = ov_data_pointer_free(self);

error:
    return self;
}

/*----------------------------------------------------------------------------*/

ov_vocs_events *ov_vocs_events_cast(const void *data) {

    if (!data) return NULL;

    if (*(uint16_t *)data != OV_VOCS_EVENTS_MAGIC_BYTES) return NULL;

    return (ov_vocs_events *)data;
}

/*----------------------------------------------------------------------------*/

ov_vocs_events_config ov_vocs_events_config_from_json(
    const ov_json_value *val) {

    ov_vocs_events_config config = (ov_vocs_events_config){0};

    const ov_json_value *data = ov_json_object_get(val, OV_KEY_EVENTS);
    if (!data) data = val;

    config.socket.manager = ov_socket_configuration_from_json(
        ov_json_get(data, "/" OV_KEY_SOCKET "/" OV_KEY_MANAGER),
        (ov_socket_configuration){0});

    return config;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      EMIT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool send_data(const void *key, void *val, void *data) {

    if (!key) return true;

    ov_json_value *out = ov_json_value_cast(data);
    ov_event_connection *conn = ov_event_connection_cast(val);

    if (!ov_event_connection_send(conn, out)) {

        ov_log_error(
            "failed to forward at %i", ov_event_connection_get_socket(conn));
    }

    return true;
}

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_media_ready(ov_vocs_events *self,
                                     const char *session_id) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;

    if (!self || !session_id) goto error;

    out = ov_event_api_message_create(OV_KEY_MEDIA_READY, NULL, 1);
    par = ov_event_api_set_parameter(out);
    val = ov_json_string(session_id);

    if (!ov_json_object_set(par, OV_KEY_SESSION, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    ov_dict_for_each(self->connections, out, send_data);

error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_mixer_aquired(ov_vocs_events *self,
                                       const char *session_id) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;

    if (!self || !session_id) goto error;

    out = ov_event_api_message_create(OV_KEY_MIXER_AQUIRED, NULL, 1);
    par = ov_event_api_set_parameter(out);
    val = ov_json_string(session_id);

    if (!ov_json_object_set(par, OV_KEY_SESSION, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    ov_dict_for_each(self->connections, out, send_data);

error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_mixer_lost(ov_vocs_events *self,
                                    const char *session_id) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;

    if (!self || !session_id) goto error;

    out = ov_event_api_message_create(OV_KEY_MIXER_LOST, NULL, 1);
    par = ov_event_api_set_parameter(out);
    val = ov_json_string(session_id);

    if (!ov_json_object_set(par, OV_KEY_SESSION, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    ov_dict_for_each(self->connections, out, send_data);

error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_mixer_released(ov_vocs_events *self,
                                        const char *session_id) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;

    if (!self || !session_id) goto error;

    out = ov_event_api_message_create(OV_KEY_MIXER_RELEASED, NULL, 1);
    par = ov_event_api_set_parameter(out);
    val = ov_json_string(session_id);

    if (!ov_json_object_set(par, OV_KEY_SESSION, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    ov_dict_for_each(self->connections, out, send_data);

error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_client_login(ov_vocs_events *self,
                                      const char *client_id,
                                      const char *user_id) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;

    if (!self || !client_id || !user_id) goto error;

    out = ov_event_api_message_create(OV_KEY_CLIENT_LOGIN, NULL, 1);
    par = ov_event_api_set_parameter(out);

    val = ov_json_string(client_id);
    if (!ov_json_object_set(par, OV_KEY_CLIENT, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(user_id);
    if (!ov_json_object_set(par, OV_KEY_USER, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    ov_dict_for_each(self->connections, out, send_data);

error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_client_authorize(ov_vocs_events *self,
                                          const char *session_id,
                                          const char *user_id,
                                          const char *role_id) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;

    if (!self || !session_id || !user_id || !role_id) goto error;

    out = ov_event_api_message_create(OV_KEY_AUTHORIZE, NULL, 1);
    par = ov_event_api_set_parameter(out);

    val = ov_json_string(session_id);
    if (!ov_json_object_set(par, OV_KEY_SESSION, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(user_id);
    if (!ov_json_object_set(par, OV_KEY_USER, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(role_id);
    if (!ov_json_object_set(par, OV_KEY_ROLE, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    ov_dict_for_each(self->connections, out, send_data);

error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_client_logout(ov_vocs_events *self,
                                       const char *session_id) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;

    if (!self || !session_id) goto error;

    out = ov_event_api_message_create(OV_KEY_LOGOUT, NULL, 1);
    par = ov_event_api_set_parameter(out);

    val = ov_json_string(session_id);
    if (!ov_json_object_set(par, OV_KEY_SESSION, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    ov_dict_for_each(self->connections, out, send_data);

error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_user_session_created(ov_vocs_events *self,
                                              const char *session_id,
                                              const char *client_id,
                                              const char *user_id) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;

    if (!self || !session_id || !client_id || !user_id) goto error;

    out = ov_event_api_message_create(OV_KEY_SESSION_CREATED, NULL, 1);
    par = ov_event_api_set_parameter(out);

    val = ov_json_string(session_id);
    if (!ov_json_object_set(par, OV_KEY_SESSION, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(client_id);
    if (!ov_json_object_set(par, OV_KEY_CLIENT, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(user_id);
    if (!ov_json_object_set(par, OV_KEY_USER, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    ov_dict_for_each(self->connections, out, send_data);

error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_user_session_closed(ov_vocs_events *self,
                                             const char *session_id,
                                             const char *client_id,
                                             const char *user_id) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;

    if (!self || !session_id || !client_id || !user_id) goto error;

    out = ov_event_api_message_create(OV_KEY_SESSION_CLOSED, NULL, 1);
    par = ov_event_api_set_parameter(out);

    val = ov_json_string(session_id);
    if (!ov_json_object_set(par, OV_KEY_SESSION, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(client_id);
    if (!ov_json_object_set(par, OV_KEY_CLIENT, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(user_id);
    if (!ov_json_object_set(par, OV_KEY_USER, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    ov_dict_for_each(self->connections, out, send_data);

error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_participation_state(ov_vocs_events *self,
                                             const char *session_id,
                                             const char *client_id,
                                             const char *user_id,
                                             const char *role_id,
                                             const char *loop_id,
                                             ov_socket_configuration multicast,
                                             ov_participation_state state) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;

    char const *str_state = ov_participation_state_to_string(state);

    if (!self || !session_id || !client_id || !user_id || !role_id ||
        !loop_id || !str_state)
        goto error;

    out = ov_event_api_message_create(OV_KEY_STATE, NULL, 1);
    par = ov_event_api_set_parameter(out);

    val = ov_json_string(session_id);
    if (!ov_json_object_set(par, OV_KEY_SESSION, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(client_id);
    if (!ov_json_object_set(par, OV_KEY_CLIENT, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(user_id);
    if (!ov_json_object_set(par, OV_KEY_USER, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(role_id);
    if (!ov_json_object_set(par, OV_KEY_ROLE, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(loop_id);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = NULL;
    ov_socket_configuration_to_json(multicast, &val);
    if (!ov_json_object_set(par, OV_KEY_MULTICAST, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(str_state);
    if (!ov_json_object_set(par, OV_KEY_STATE, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    ov_dict_for_each(self->connections, out, send_data);

error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_loop_volume(ov_vocs_events *self,
                                     const char *session_id,
                                     const char *client_id,
                                     const char *user_id,
                                     const char *role_id,
                                     const char *loop_id,
                                     ov_socket_configuration multicast,
                                     uint8_t volume) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;

    if (!self || !session_id || !user_id || !client_id || !role_id || !loop_id)
        goto error;

    out = ov_event_api_message_create(OV_KEY_VOLUME, NULL, 1);
    par = ov_event_api_set_parameter(out);

    val = ov_json_string(session_id);
    if (!ov_json_object_set(par, OV_KEY_SESSION, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(client_id);
    if (!ov_json_object_set(par, OV_KEY_CLIENT, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(user_id);
    if (!ov_json_object_set(par, OV_KEY_USER, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(role_id);
    if (!ov_json_object_set(par, OV_KEY_ROLE, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(loop_id);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = NULL;
    ov_socket_configuration_to_json(multicast, &val);
    if (!ov_json_object_set(par, OV_KEY_MULTICAST, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_number(volume);
    if (!ov_json_object_set(par, OV_KEY_VOLUME, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    ov_dict_for_each(self->connections, out, send_data);

error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_ptt(ov_vocs_events *self,
                             const char *session_id,
                             const char *client_id,
                             const char *user_id,
                             const char *role_id,
                             const char *loop_id,
                             ov_socket_configuration multicast,
                             bool off) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;

    if (!self || !session_id || !user_id || !client_id || !role_id || !loop_id)
        goto error;

    out = ov_event_api_message_create(OV_KEY_PTT, NULL, 1);
    par = ov_event_api_set_parameter(out);

    val = ov_json_string(session_id);
    if (!ov_json_object_set(par, OV_KEY_SESSION, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(client_id);
    if (!ov_json_object_set(par, OV_KEY_CLIENT, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(user_id);
    if (!ov_json_object_set(par, OV_KEY_USER, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(role_id);
    if (!ov_json_object_set(par, OV_KEY_ROLE, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = ov_json_string(loop_id);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    val = NULL;
    ov_socket_configuration_to_json(multicast, &val);
    if (!ov_json_object_set(par, OV_KEY_MULTICAST, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    if (!off) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }

    if (!ov_json_object_set(par, OV_KEY_PTT, val)) {
        val = ov_json_value_free(val);
        goto error;
    }

    ov_dict_for_each(self->connections, out, send_data);

error:
    out = ov_json_value_free(out);
    return;
}
