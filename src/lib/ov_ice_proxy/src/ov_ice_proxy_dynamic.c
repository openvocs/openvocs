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
        @file           ov_ice_proxy_dynamic.c
        @author         Markus

        @date           2024-07-29


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_proxy_dynamic.h"
#include "../include/ov_ice_config_from_generic.h"

#include <ov_ice/ov_ice.h>

#define OV_ICE_PROXY_DYNAMIC_MAGIC_BYTES 0x1ced

/*----------------------------------------------------------------------------*/

typedef struct ov_ice_proxy_dynamic {

  ov_ice_proxy_generic public;

  ov_ice *core;

} ov_ice_proxy_dynamic;

/*----------------------------------------------------------------------------*/

static ov_ice_proxy_dynamic *as_ice_proxy_dynamic(const void *data) {

  ov_ice_proxy_generic *generic = ov_ice_proxy_generic_cast(data);
  if (!generic)
    return NULL;

  if (generic->type == OV_ICE_PROXY_DYNAMIC_MAGIC_BYTES)
    return (ov_ice_proxy_dynamic *)data;

  return NULL;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      PUBLIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static const char *proxy_session_create(ov_ice_proxy_generic *self,
                                        ov_sdp_session *sdp) {

  ov_ice_proxy_dynamic *proxy = as_ice_proxy_dynamic(self);
  if (!proxy || !sdp)
    return NULL;

  return ov_ice_create_session_offer(proxy->core, sdp);
}

/*----------------------------------------------------------------------------*/

static bool proxy_session_drop(ov_ice_proxy_generic *self,
                               const char *session_id) {

  ov_ice_proxy_dynamic *proxy = as_ice_proxy_dynamic(self);
  if (!proxy || !session_id)
    return false;

  return ov_ice_drop_session(proxy->core, session_id);
}

/*----------------------------------------------------------------------------*/

static bool proxy_session_update(ov_ice_proxy_generic *self,
                                 const char *session_id,
                                 const ov_sdp_session *sdp) {

  ov_ice_proxy_dynamic *proxy = as_ice_proxy_dynamic(self);
  if (!proxy || !session_id || !sdp)
    return false;

  return ov_ice_session_process_answer(proxy->core, session_id, sdp);
}

/*----------------------------------------------------------------------------*/

static bool proxy_stream_candidate_in(ov_ice_proxy_generic *self,
                                      const char *session_id,
                                      uint32_t stream_id,
                                      const ov_ice_candidate *candidate) {

  ov_ice_proxy_dynamic *proxy = as_ice_proxy_dynamic(self);
  if (!proxy || !session_id || !candidate)
    return false;

  ov_ice_candidate *c = calloc(1, sizeof(ov_ice_candidate));
  *c = *candidate;

  return ov_ice_add_candidate(proxy->core, session_id, stream_id, c);
}

/*----------------------------------------------------------------------------*/

static bool proxy_stream_end_of_candidates_in(ov_ice_proxy_generic *self,
                                              const char *session_id,
                                              uint32_t stream_id) {

  ov_ice_proxy_dynamic *proxy = as_ice_proxy_dynamic(self);
  if (!proxy || !session_id)
    return false;

  return ov_ice_add_end_of_candidates(proxy->core, session_id, stream_id);
}

/*----------------------------------------------------------------------------*/

static uint32_t proxy_stream_get_ssrc(ov_ice_proxy_generic *self,
                                      const char *session_id,
                                      uint32_t stream_id) {

  ov_ice_proxy_dynamic *proxy = as_ice_proxy_dynamic(self);
  if (!proxy || !session_id)
    return false;

  return ov_ice_get_stream_ssrc(proxy->core, session_id, stream_id);
}

/*----------------------------------------------------------------------------*/

static ssize_t proxy_stream_send(ov_ice_proxy_generic *self,
                                 const char *session_id, uint32_t stream_id,
                                 uint8_t *buffer, size_t size) {

  ov_ice_proxy_dynamic *proxy = as_ice_proxy_dynamic(self);
  if (!proxy || !session_id || !buffer || !size)
    return -1;

  return ov_ice_stream_send(proxy->core, session_id, stream_id, buffer, size);
}

/*
 *      ------------------------------------------------------------------------
 *
 *      CORE CALLBACKS
 *
 *      ------------------------------------------------------------------------
 */

static void core_cb_session_drop(void *userdata, const char *uuid) {

  ov_ice_proxy_dynamic *proxy = as_ice_proxy_dynamic(userdata);
  if (!proxy || !uuid)
    goto error;

  if (proxy->public.config.callbacks.session.drop)
    proxy->public.config.callbacks.session.drop(
        proxy->public.config.callbacks.userdata, uuid);

error:
  return;
}

/*----------------------------------------------------------------------------*/

static void core_cb_session_state(void *userdata, const char *uuid,
                                  ov_ice_state s) {

  ov_ice_proxy_dynamic *proxy = as_ice_proxy_dynamic(userdata);
  if (!proxy || !uuid)
    goto error;

  ov_ice_proxy_generic_state state = (ov_ice_proxy_generic_state)s;

  if (proxy->public.config.callbacks.session.state)
    proxy->public.config.callbacks.session.state(
        proxy->public.config.callbacks.userdata, uuid, state);

error:
  return;
}

/*----------------------------------------------------------------------------*/

static void core_cb_stream_io(void *userdata, const char *session_uuid,
                              int stream_id, uint8_t *buffer, size_t size) {

  ov_ice_proxy_dynamic *proxy = as_ice_proxy_dynamic(userdata);
  if (!proxy || !session_uuid)
    goto error;

  if (proxy->public.config.callbacks.stream.io)
    proxy->public.config.callbacks.stream.io(
        proxy->public.config.callbacks.userdata, session_uuid, stream_id,
        buffer, size);

error:
  return;
}

/*----------------------------------------------------------------------------*/

static bool core_cb_candidates_new(void *userdata, const char *session_uuid,
                                   const char *ufrag, int stream_id,
                                   ov_ice_candidate candidate) {

  ov_ice_proxy_dynamic *proxy = as_ice_proxy_dynamic(userdata);
  if (!proxy || !session_uuid)
    goto error;

  ov_json_value *info =
      ov_ice_candidate_json_info(&candidate, session_uuid, ufrag, stream_id);

  if (proxy->public.config.callbacks.candidate.send) {
    proxy->public.config.callbacks.candidate.send(
        proxy->public.config.callbacks.userdata, info);
  } else {
    info = ov_json_value_free(info);
  }

error:
  return true;
}

/*----------------------------------------------------------------------------*/

static bool core_cb_end_of_candidates(void *userdata, const char *session_uuid,
                                      int stream_id) {

  ov_ice_proxy_dynamic *proxy = as_ice_proxy_dynamic(userdata);
  if (!proxy || !session_uuid)
    goto error;

  UNUSED(stream_id);

  if (proxy->public.config.callbacks.candidate.end_of_candidates)
    proxy->public.config.callbacks.candidate.end_of_candidates(
        proxy->public.config.callbacks.userdata, session_uuid);
error:
  return true;
}

/*----------------------------------------------------------------------------*/

ov_ice_proxy_dynamic *ov_ice_proxy_dynamic_free(ov_ice_proxy_dynamic *self) {

  if (!self)
    return NULL;

  self->core = ov_ice_free(self->core);
  self = ov_data_pointer_free(self);
  return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_ice_proxy_generic *proxy_free(ov_ice_proxy_generic *self) {

  ov_ice_proxy_dynamic *proxy = as_ice_proxy_dynamic(self);
  if (!proxy)
    return self;

  ov_ice_proxy_dynamic_free(proxy);
  return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice_proxy_generic *
ov_ice_proxy_dynamic_create(ov_ice_proxy_generic_config config) {

  ov_ice_proxy_dynamic *self = NULL;

  if (0 == config.external.host[0]) {
    ov_log_error("Config without external host, cannot create ICE PROXY.");
    goto error;
  }

  self = calloc(1, sizeof(ov_ice_proxy_dynamic));
  if (!self)
    goto error;
  self->public.config = config;

  ov_ice_config core_config = ov_ice_config_from_generic(config);
  core_config.callbacks.userdata = self;
  core_config.callbacks.session.drop = core_cb_session_drop;
  core_config.callbacks.session.state = core_cb_session_state;
  core_config.callbacks.stream.io = core_cb_stream_io;
  core_config.callbacks.candidates.new = core_cb_candidates_new;
  core_config.callbacks.candidates.end_of_candidates =
      core_cb_end_of_candidates;

  self->public.magic_bytes = OV_ICE_PROXY_GENERIC_MAGIC_BYTES;
  self->public.type = OV_ICE_PROXY_DYNAMIC_MAGIC_BYTES;

  self->public.free = proxy_free;
  self->public.session.create = proxy_session_create;
  self->public.session.drop = proxy_session_drop;
  self->public.session.update = proxy_session_update;
  self->public.stream.candidate_in = proxy_stream_candidate_in;
  self->public.stream.end_of_candidates_in = proxy_stream_end_of_candidates_in;
  self->public.stream.get_ssrc = proxy_stream_get_ssrc;
  self->public.stream.send = proxy_stream_send;

  self->core = ov_ice_create(&core_config);
  if (!self->core)
    goto error;

  if (!ov_ice_set_debug_stun(self->core, false))
    goto error;

  if (!ov_ice_set_debug_ice(self->core, true))
    goto error;

  if (!ov_ice_set_debug_dtls(self->core, true))
    goto error;

  if (0 != config.dynamic.ports.min || 0 != config.dynamic.ports.max)
    ov_ice_set_port_range(self->core, config.dynamic.ports.min,
                          config.dynamic.ports.max);

  if (0 != config.dynamic.stun.server.host[0]) {

    ov_ice_server stun_config = (ov_ice_server){
        .type = OV_ICE_STUN_SERVER, .socket = config.dynamic.stun.server};

    if (!ov_ice_add_server(self->core, stun_config)) {

      ov_log_error("Failed to add STUN server.");
      goto error;
    }
  }

  if (0 != config.dynamic.turn.server.host[0]) {

    ov_ice_server turn_config = (ov_ice_server){
        .type = OV_ICE_TURN_SERVER, .socket = config.dynamic.turn.server};

    memcpy(turn_config.auth.user, config.dynamic.turn.username,
           OV_ICE_STUN_USER_MAX);
    memcpy(turn_config.auth.pass, config.dynamic.turn.password,
           OV_ICE_STUN_PASS_MAX);

    if (!ov_ice_add_server(self->core, turn_config)) {

      ov_log_error("Failed to add TURN server.");
      goto error;
    }
  }

  if (!ov_ice_add_interface(self->core, config.external.host)) {

    ov_log_error("Failed to set external host.");
    goto error;
  }

  return ov_ice_proxy_generic_cast(self);
error:
  ov_ice_proxy_dynamic_free(self);
  return NULL;
}
