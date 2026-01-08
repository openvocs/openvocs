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
        @file           ov_ice_proxy_vocs_app.c
        @author         Markus

        @date           2024-05-08


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_proxy_vocs_app.h"

#include "../include/ov_ice_proxy_vocs.h"
#include "../include/ov_ice_proxy_vocs_app.h"
#include "../include/ov_ice_proxy_vocs_msg.h"

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_error_codes.h>
#include <ov_base/ov_json.h>

#include <ov_core/ov_event_api.h>
#include <ov_core/ov_event_engine.h>
#include <ov_core/ov_event_io.h>
#include <ov_core/ov_event_socket.h>
#include <ov_core/ov_mc_loop_data.h>
#include <ov_core/ov_media_definitions.h>

/*----------------------------------------------------------------------------*/

#define ov_ice_proxy_vocs_APP_MAGIC_BYTES 0x1ce8

/*----------------------------------------------------------------------------*/

struct ov_ice_proxy_vocs_app {

  uint16_t magic_bytes;
  ov_id uuid;

  ov_ice_proxy_vocs_app_config config;

  ov_ice_proxy_vocs *proxy;

  int socket;
  ov_socket_data local;
  ov_socket_data remote;

  ov_event_socket *event_socket;
  ov_event_engine *engine;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      EVENT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool cb_event_ice_session_create(void *userdata, const int socket,
                                        const ov_event_parameter *params,
                                        ov_json_value *input) {

  ov_ice_proxy_vocs_session_data session;

  uint64_t code = OV_ERROR_CODE;
  const char *desc = OV_ERROR_DESC;

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;
  ov_json_value *res = NULL;

  ov_ice_proxy_vocs_app *self = ov_ice_proxy_vocs_app_cast(userdata);
  if (!self || !socket || !params || !input)
    goto error;

  ov_json_value *parameter = ov_event_api_get_parameter(input);
  if (!parameter)
    goto error_response;

  if (OV_MEDIA_OFFER !=
      ov_media_type_from_string(ov_event_api_get_type(parameter)))
    goto error_response;

  code = OV_ERROR_CODE_SESSION_CREATE;
  desc = OV_ERROR_DESC_SESSION_CREATE;

  session = ov_ice_proxy_vocs_create_session(self->proxy);
  if (0 == session.uuid[0])
    goto error_response;

  out = ov_event_api_create_success_response(input);
  res = ov_event_api_get_response(out);
  if (!res)
    goto error;

  val = ov_ice_proxy_vocs_session_data_description_to_json(&session);
  if (!val)
    goto error;

  if (!ov_json_object_set(res, OV_KEY_SDP, val))
    goto error;

  if (!ov_event_api_set_type(res, ov_media_type_to_string(OV_MEDIA_OFFER)))
    goto error;

  val = ov_json_string(session.uuid);
  if (!ov_json_object_set(res, OV_KEY_SESSION, val))
    goto error;

  val = ov_ice_proxy_vocs_session_data_to_json(&session);
  if (!ov_json_object_set(res, OV_KEY_PROXY, val))
    goto error;

  code = OV_ERROR_CODE_COMMS_ERROR;
  desc = OV_ERROR_DESC_COMMS_ERROR;

  ov_event_io_send(params, socket, out);
  out = ov_json_value_free(out);
  ov_json_value_free(input);

  session = ov_ice_proxy_vocs_session_data_clear(&session);
  return true;

error_response:

  out = ov_json_value_free(out);
  out = ov_event_api_create_error_response(input, code, desc);
  ov_event_io_send(params, socket, out);
  out = ov_json_value_free(out);

error:
  out = ov_json_value_free(out);
  session = ov_ice_proxy_vocs_session_data_clear(&session);
  ov_json_value_free(input);
  return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_ice_session_drop(void *userdata, const int socket,
                                      const ov_event_parameter *params,
                                      ov_json_value *input) {

  uint64_t code = OV_ERROR_CODE;
  const char *desc = OV_ERROR_DESC;

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;
  ov_json_value *res = NULL;

  ov_ice_proxy_vocs_app *self = ov_ice_proxy_vocs_app_cast(userdata);
  if (!self || !socket || !params || !input)
    goto error;

  code = OV_ERROR_CODE_INPUT_ERROR;
  desc = OV_ERROR_DESC_INPUT_ERROR;

  const char *id = ov_json_string_get(
      ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION));

  OV_ASSERT(id);

  code = OV_ERROR_CODE_PROCESSING_ERROR;
  desc = OV_ERROR_DESC_PROCESSING_ERROR;

  if (!ov_ice_proxy_vocs_drop_session(self->proxy, id)) {

    out = ov_event_api_create_error_response(input, code, desc);

  } else {

    out = ov_event_api_create_success_response(input);
    res = ov_event_api_get_response(out);
    val = ov_json_string(id);
    if (!ov_json_object_set(res, OV_KEY_SESSION, val))
      goto error;
  }

  ov_event_io_send(params, socket, out);

error:
  ov_json_value_free(out);
  ov_json_value_free(input);
  return true;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_ice_session_update(void *userdata, const int socket,
                                        const ov_event_parameter *params,
                                        ov_json_value *input) {

  uint64_t code = OV_ERROR_CODE;
  const char *desc = OV_ERROR_DESC;

  ov_sdp_session *sdp = NULL;

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;
  ov_json_value *res = NULL;

  ov_media_type type = OV_MEDIA_OFFER;

  ov_ice_proxy_vocs_app *self = ov_ice_proxy_vocs_app_cast(userdata);
  if (!self || !socket || !params || !input)
    goto error;

  code = OV_ERROR_CODE_INPUT_ERROR;
  desc = OV_ERROR_DESC_INPUT_ERROR;

  ov_json_value *parameter = ov_event_api_get_parameter(input);
  if (!parameter)
    goto error_response;

  type = ov_media_type_from_string(ov_event_api_get_type(parameter));

  sdp = ov_sdp_session_from_json(parameter);
  if (!sdp)
    goto error_response;

  const char *session_id =
      ov_json_string_get(ov_json_get(parameter, "/" OV_KEY_SESSION));

  if (!session_id)
    goto error_response;

  code = OV_ERROR_CODE_PROCESSING_ERROR;
  desc = OV_ERROR_DESC_PROCESSING_ERROR;

  bool result = false;

  switch (type) {

  case OV_MEDIA_OFFER:

    code = OV_ERROR_CODE_NOT_IMPLEMENTED;
    desc = OV_ERROR_DESC_NOT_IMPLEMENTED;

    goto error_response;
    break;

  case OV_MEDIA_ANSWER:

    result = ov_ice_proxy_vocs_update_answer(self->proxy, session_id, sdp);

    break;

  default:
    OV_ASSERT(1 == 0);
    goto error;
  }

  if (false == result)
    goto error_response;

  out = ov_event_api_create_success_response(input);
  res = ov_event_api_get_response(out);

  val = ov_json_string(session_id);
  if (!ov_json_object_set(res, OV_KEY_SESSION, val))
    goto error;

  ov_event_io_send(params, socket, out);
  out = ov_json_value_free(out);
  ov_json_value_free(input);
  sdp = ov_sdp_session_free(sdp);
  return true;

error_response:

  out = ov_json_value_free(out);
  out = ov_event_api_create_error_response(input, code, desc);
  ov_event_io_send(params, socket, out);
  out = ov_json_value_free(out);

error:
  ov_json_value_free(input);
  return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_ice_session_state(void *userdata, const int socket,
                                       const ov_event_parameter *params,
                                       ov_json_value *input) {

  ov_ice_proxy_vocs_app *self = ov_ice_proxy_vocs_app_cast(userdata);
  if (!self || !socket || !params || !input)
    goto error;

/*
    uint64_t code = OV_ERROR_CODE;
    const char *desc = OV_ERROR_DESC;

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *res = NULL;

    ov_ice_proxy_vocs_app *self = ov_ice_proxy_vocs_app_cast(userdata);
    if (!self || !params || socket < 0 || !input) goto error;

    code = OV_ERROR_CODE_INPUT_ERROR;
    desc = OV_ERROR_DESC_INPUT_ERROR;

    ov_json_value *parameter = ov_event_api_get_parameter(input);
    if (!parameter) goto error_response;

    const char *session_id =
        ov_json_string_get(ov_json_get(parameter, "/" OV_KEY_SESSION));

    if (!session_id) goto error_response;

    code = OV_ERROR_CODE_PROCESSING_ERROR;
    desc = OV_ERROR_DESC_PROCESSING_ERROR;

    val = ov_ice_proxy_vocs_get_session_state(self->proxy, session_id);

    out = ov_event_api_create_success_response(input);
    res = ov_event_api_get_response(out);
    if (!ov_json_object_set(res, OV_KEY_RESULT, val)) goto error;

    val = ov_json_string(session_id);
    if (!ov_json_object_set(res, OV_KEY_SESSION, val)) goto error;

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    ov_json_value_free(input);
    return true;

error_response:

    out = ov_json_value_free(out);
    out = ov_event_api_create_error_response(input, code, desc);
    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
*/
error:
  ov_json_value_free(input);
  return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_ice_candidate(void *userdata, const int socket,
                                   const ov_event_parameter *params,
                                   ov_json_value *input) {

  uint64_t code = OV_ERROR_CODE;
  const char *desc = OV_ERROR_DESC;

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;
  ov_json_value *res = NULL;

  ov_ice_candidate *candidate = NULL;

  ov_ice_proxy_vocs_app *self = ov_ice_proxy_vocs_app_cast(userdata);
  if (!self || !socket || !params || !input)
    goto error;

  code = OV_ERROR_CODE_INPUT_ERROR;
  desc = OV_ERROR_DESC_INPUT_ERROR;

  ov_json_value *parameter = ov_event_api_get_parameter(input);
  if (!parameter)
    goto error_response;

  const char *session_id =
      ov_json_string_get(ov_json_get(parameter, "/" OV_KEY_SESSION));

  if (!session_id)
    goto error_response;

  ov_ice_candidate_info info = ov_ice_candidate_info_from_json(parameter);

  if (!info.candidate || !info.ufrag)
    goto error;

  candidate = ov_ice_candidate_from_json_string(parameter);

  if (!candidate)
    goto error_response;

  code = OV_ERROR_CODE_CANDIDATE_PROCESSING;
  desc = OV_ERROR_DESC_CANDIDATE_PROCESSING;

  if (!ov_ice_proxy_vocs_candidate_in(self->proxy, session_id,
                                      info.SDPMlineIndex, candidate))
    goto error_response;

  candidate = ov_ice_candidate_free(candidate);

  out = ov_event_api_create_success_response(input);
  res = ov_event_api_get_response(out);
  val = ov_json_string(session_id);
  if (!ov_json_object_set(res, OV_KEY_SESSION, val))
    goto error;
  val = NULL;

  ov_event_io_send(params, socket, out);
  out = ov_json_value_free(out);
  input = ov_json_value_free(input);
  return true;

error_response:

  out = ov_json_value_free(out);
  out = ov_event_api_create_error_response(input, code, desc);
  ov_event_io_send(params, socket, out);
  out = ov_json_value_free(out);

error:
  ov_json_value_free(input);
  return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_ice_end_of_candidates(void *userdata, const int socket,
                                           const ov_event_parameter *params,
                                           ov_json_value *input) {

  uint64_t code = OV_ERROR_CODE;
  const char *desc = OV_ERROR_DESC;

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;
  ov_json_value *res = NULL;

  ov_ice_proxy_vocs_app *self = ov_ice_proxy_vocs_app_cast(userdata);
  if (!self || !params || socket < 0 || !input)
    goto error;

  code = OV_ERROR_CODE_INPUT_ERROR;
  desc = OV_ERROR_DESC_INPUT_ERROR;

  ov_json_value *parameter = ov_event_api_get_parameter(input);
  if (!parameter)
    goto error_response;

  const char *session_id =
      ov_json_string_get(ov_json_get(parameter, "/" OV_KEY_SESSION));

  if (!session_id)
    goto error_response;

  code = OV_ERROR_CODE_CANDIDATE_PROCESSING;
  desc = OV_ERROR_DESC_CANDIDATE_PROCESSING;

  if (!ov_ice_proxy_vocs_end_of_candidates_in(self->proxy, session_id, 0))
    goto error_response;

  out = ov_event_api_create_success_response(input);
  res = ov_event_api_get_response(out);
  val = ov_json_string(session_id);
  if (!ov_json_object_set(res, OV_KEY_SESSION, val))
    goto error;
  val = NULL;

  ov_event_io_send(params, socket, out);
  out = ov_json_value_free(out);
  input = ov_json_value_free(input);
  return true;

error_response:

  out = ov_json_value_free(out);
  out = ov_event_api_create_error_response(input, code, desc);
  ov_event_io_send(params, socket, out);
  out = ov_json_value_free(out);

error:
  ov_json_value_free(input);
  return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_talk(void *userdata, const int socket,
                          const ov_event_parameter *params,
                          ov_json_value *input) {

  uint64_t code = OV_ERROR_CODE;
  const char *desc = OV_ERROR_DESC;

  bool on = false;

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;
  ov_json_value *res = NULL;

  ov_ice_proxy_vocs_app *self = ov_ice_proxy_vocs_app_cast(userdata);
  if (!self || !params || socket < 0 || !input)
    goto error;

  code = OV_ERROR_CODE_INPUT_ERROR;
  desc = OV_ERROR_DESC_INPUT_ERROR;

  ov_json_value *parameter = ov_event_api_get_parameter(input);
  if (!parameter)
    goto error_response;

  const char *session_id =
      ov_json_string_get(ov_json_get(parameter, "/" OV_KEY_SESSION));

  ov_mc_loop_data data =
      ov_mc_loop_data_from_json(ov_json_get(parameter, "/" OV_KEY_LOOP));

  if (ov_json_is_true(ov_json_get(parameter, "/" OV_KEY_ON))) {
    on = true;
  } else {
    on = false;
  }

  if (!session_id)
    goto error_response;

  code = OV_ERROR_CODE_PROCESSING_ERROR;
  desc = OV_ERROR_DESC_PROCESSING_ERROR;

  if (!ov_ice_proxy_vocs_talk(self->proxy, session_id, on, data))
    goto error_response;

  out = ov_event_api_create_success_response(input);
  res = ov_event_api_get_response(out);
  val = ov_json_string(session_id);
  if (!ov_json_object_set(res, OV_KEY_SESSION, val))
    goto error;
  if (on) {
    val = ov_json_true();
  } else {
    val = ov_json_false();
  }
  if (!ov_json_object_set(res, OV_KEY_ON, val))
    goto error;
  val = ov_mc_loop_data_to_json(data);
  if (!ov_json_object_set(res, OV_KEY_LOOP, val))
    goto error;
  val = NULL;

  ov_event_io_send(params, socket, out);
  out = ov_json_value_free(out);
  input = ov_json_value_free(input);
  return true;

error_response:

  out = ov_json_value_free(out);
  out = ov_event_api_create_error_response(input, code, desc);
  ov_event_io_send(params, socket, out);
  out = ov_json_value_free(out);

error:
  ov_json_value_free(input);
  return false;
}

/*----------------------------------------------------------------------------*/

static bool register_event_callbacks(ov_ice_proxy_vocs_app *app) {

  OV_ASSERT(app);
  if (!app)
    goto error;

  if (!ov_event_engine_register(app->engine, OV_KEY_ICE_SESSION_CREATE,
                                cb_event_ice_session_create))
    goto error;

  if (!ov_event_engine_register(app->engine, OV_KEY_ICE_SESSION_DROP,
                                cb_event_ice_session_drop))
    goto error;

  if (!ov_event_engine_register(app->engine, OV_KEY_ICE_SESSION_UPDATE,
                                cb_event_ice_session_update))
    goto error;

  if (!ov_event_engine_register(app->engine, OV_KEY_ICE_SESSION_STATE,
                                cb_event_ice_session_state))
    goto error;

  if (!ov_event_engine_register(app->engine, OV_ICE_STRING_CANDIDATE,
                                cb_event_ice_candidate))
    goto error;

  if (!ov_event_engine_register(app->engine, OV_ICE_STRING_END_OF_CANDIDATES,
                                cb_event_ice_end_of_candidates))
    goto error;

  if (!ov_event_engine_register(app->engine, OV_KEY_TALK, cb_event_talk))
    goto error;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static void cb_close(void *userdata, int socket) {

  ov_ice_proxy_vocs_app *app = ov_ice_proxy_vocs_app_cast(userdata);
  if (!app)
    goto error;

  ov_log_error("proxy socket closed %i", socket);
  app->socket = -1;

  TODO("... close all sessions?");

error:
  return;
}

/*----------------------------------------------------------------------------*/

static void cb_connected(void *userdata, int socket, bool result) {

  ov_ice_proxy_vocs_app *app = ov_ice_proxy_vocs_app_cast(userdata);

  if (result) {

    app->socket = socket;
    ov_socket_get_data(socket, &app->local, &app->remote);

    ov_log_info("Proxy connected at %i from %s:%i to %s:%i", socket,
                app->local.host, app->local.port, app->remote.host,
                app->remote.port);

    ov_json_value *out = ov_ice_proxy_vocs_msg_register(app->uuid);
    ov_event_socket_send(app->event_socket, socket, out);
    out = ov_json_value_free(out);

  } else {

    ov_log_debug("Proxy reconnect attempt failed.");
  }

  return;
}

/*----------------------------------------------------------------------------*/

static bool send_candidate(void *userdata, ov_json_value *out) {

  ov_ice_proxy_vocs_app *app = ov_ice_proxy_vocs_app_cast(userdata);
  if (!app || !out)
    goto error;

  ov_event_socket_send(app->event_socket, app->socket, out);
  out = ov_json_value_free(out);

error:
  ov_json_value_free(out);
  return true;
}

/*----------------------------------------------------------------------------*/

static bool send_end_of_candidates(void *userdata, const char *session_id) {

  ov_json_value *out = NULL;

  ov_ice_proxy_vocs_app *app = ov_ice_proxy_vocs_app_cast(userdata);
  if (!app || !session_id)
    goto error;

  out = ov_ice_proxy_vocs_msg_end_of_candidates(session_id);

  ov_event_socket_send(app->event_socket, app->socket, out);
  out = ov_json_value_free(out);

error:
  ov_json_value_free(out);
  return true;
}

/*----------------------------------------------------------------------------*/

static bool session_completed(void *userdata, const char *uuid,
                              ov_ice_state state) {

  ov_json_value *out = NULL;

  ov_ice_proxy_vocs_app *app = ov_ice_proxy_vocs_app_cast(userdata);
  if (!app || !uuid)
    goto error;

  ov_log_debug("ICE Session %s %s", uuid, ov_ice_state_to_string(state));

  switch (state) {
  case OV_ICE_COMPLETED:
    break;
  default:
    ov_ice_proxy_vocs_drop_session(app->proxy, uuid);
  }

  out = ov_ice_proxy_vocs_msg_session_completed(uuid, state);
  if (!ov_event_socket_send(app->event_socket, app->socket, out))
    ov_log_error("Failed to forward session state");
  out = ov_json_value_free(out);

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

ov_ice_proxy_vocs_app *
ov_ice_proxy_vocs_app_create(ov_ice_proxy_vocs_app_config config) {

  ov_ice_proxy_vocs_app *app = NULL;

  if (!config.loop)
    goto error;

  config.proxy.loop = config.loop;

  app = calloc(1, sizeof(ov_ice_proxy_vocs_app));
  if (!app)
    goto error;

  app->magic_bytes = ov_ice_proxy_vocs_APP_MAGIC_BYTES;
  ov_id_fill_with_uuid(app->uuid);

  config.proxy.callback.userdata = app;
  config.proxy.callback.send_candidate = send_candidate;
  config.proxy.callback.send_end_of_candidates = send_end_of_candidates;
  config.proxy.callback.session_completed = session_completed;

  app->proxy = ov_ice_proxy_vocs_create(config.proxy);
  app->engine = ov_event_engine_create();
  if (!ov_ptr_valid(app->proxy, "Could not create proxy object") ||
      !ov_ptr_valid(app->engine, "Could not create engine object"))
    goto error;

  ov_event_socket_config ev_config = (ov_event_socket_config){

      .loop = config.loop,
      .engine = app->engine,
      .timer.io_timeout_usec = config.timer.io_timeout_usec,
      .timer.accept_to_io_timeout_usec = config.timer.accept_to_io_timeout_usec,
      .timer.reconnect_interval_usec = config.timer.reconnect_interval_usec,
      .callback.userdata = app,
      .callback.close = cb_close,
      .callback.connected = cb_connected

  };

  app->event_socket = ov_event_socket_create(ev_config);
  if (!app->event_socket)
    goto error;

  ov_event_socket_set_debug(app->event_socket, true);

  ov_event_socket_client_config manager = (ov_event_socket_client_config){

      .socket = config.manager,
      .client_connect_trigger_usec = config.timer.client_connect_sec * 1000,
      .auto_reconnect = true

  };

  app->socket = ov_event_socket_create_connection(app->event_socket, manager);

  if (!register_event_callbacks(app))
    goto error;

  return app;
error:
  ov_ice_proxy_vocs_app_free(app);
  return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice_proxy_vocs_app *ov_ice_proxy_vocs_app_free(ov_ice_proxy_vocs_app *self) {

  if (!ov_ice_proxy_vocs_app_cast(self))
    goto error;

  self->proxy = ov_ice_proxy_vocs_free(self->proxy);

  self->event_socket = ov_event_socket_free(self->event_socket);
  self->engine = ov_event_engine_free(self->engine);

  self = ov_data_pointer_free(self);

error:
  return self;
}

/*----------------------------------------------------------------------------*/

ov_ice_proxy_vocs_app *ov_ice_proxy_vocs_app_cast(const void *data) {

  if (!data)
    return NULL;

  if (*(uint16_t *)data != ov_ice_proxy_vocs_APP_MAGIC_BYTES)
    return NULL;

  return (ov_ice_proxy_vocs_app *)data;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      WEBIO EVENTS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_socket_close(void *userdata, int socket) {

  ov_ice_proxy_vocs_app *self = ov_ice_proxy_vocs_app_cast(userdata);
  if (!self || socket < 0)
    goto error;

  ov_log_info("client socket close at %i", socket);

error:
  return;
}

/*----------------------------------------------------------------------------*/

static bool cb_client_process(void *userdata, const int socket,
                              const ov_event_parameter *params,
                              ov_json_value *input) {

  ov_ice_proxy_vocs_app *self = ov_ice_proxy_vocs_app_cast(userdata);
  if (!self || socket < 0 || !params || !input)
    goto error;

  ov_event_engine_push(self->engine, self, socket, *params, input);
  return true;

error:
  input = ov_json_value_free(input);
  return false;
}

/*----------------------------------------------------------------------------*/

ov_event_io_config
ov_ice_proxy_vocs_app_io_uri_config(ov_ice_proxy_vocs_app *self) {

  return (ov_event_io_config){.name = "proxy",
                              .userdata = self,
                              .callback.close = cb_socket_close,
                              .callback.process = cb_client_process};
}

/*----------------------------------------------------------------------------*/

ov_ice_proxy_vocs_app_config
ov_ice_proxy_vocs_app_config_from_json(const ov_json_value *v) {

  ov_ice_proxy_vocs_app_config config = {0};

  if (!ov_ptr_valid(v, "No configuration given"))
    goto error;

  const ov_json_value *conf = ov_json_object_get(v, OV_KEY_PROXY);
  if (!conf)
    conf = v;

  config.proxy = ov_ice_proxy_vocs_config_from_json(v);

  config.manager = ov_socket_configuration_from_json(
      ov_json_get(conf, "/" OV_KEY_MANAGER), (ov_socket_configuration){0});

  ov_json_value *limits = ov_json_object_get(conf, OV_KEY_LIMITS);

  config.timer.io_timeout_usec =
      ov_json_number_get(ov_json_object_get(limits, OV_KEY_TIMEOUT_USEC));
  config.timer.accept_to_io_timeout_usec = ov_json_number_get(
      ov_json_object_get(limits, OV_KEY_ACCEPT_TIMEOUT_USEC));
  config.timer.reconnect_interval_usec = ov_json_number_get(
      ov_json_object_get(limits, OV_KEY_RECONNECT_TIMEOUT_USEC));
  config.timer.client_connect_sec =
      ov_json_number_get(ov_json_object_get(limits, OV_KEY_CLIENT));

  return config;
error:
  return (ov_ice_proxy_vocs_app_config){0};
}
