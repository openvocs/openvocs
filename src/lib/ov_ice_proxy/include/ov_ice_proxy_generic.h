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
        @file           ov_ice_proxy_generic.h
        @author         Markus Toepfer

        @date           2024-07-26


        ------------------------------------------------------------------------
*/
#ifndef ov_ice_proxy_generic_h
#define ov_ice_proxy_generic_h

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_sdp.h>
#include <ov_base/ov_socket.h>

#include <ov_ice/ov_ice_candidate.h>
#include <ov_ice/ov_ice_state.h>

#include "ov_ice_proxy_generic_dtls_config.h"
#include "ov_ice_proxy_generic_dtls_cookie.h"

#define OV_ICE_PROXY_GENERIC_MAGIC_BYTES 0x01ce

#define OV_ICE_PROXY_GENERIC_TURN_USERNAME_MAX 255
#define OV_ICE_PROXY_GENERIC_TURN_PASSWORD_MAX 255

/*----------------------------------------------------------------------------*/

typedef enum {

  OV_ICE_PROXY_GENERIC_ERROR = -1,
  OV_ICE_PROXY_GENERIC_INIT = 0,
  OV_ICE_PROXY_GENERIC_RUNNING = 1,
  OV_ICE_PROXY_GENERIC_COMPLETED = 2,
  OV_ICE_PROXY_GENERIC_FAILED = 3

} ov_ice_proxy_generic_state;

/*----------------------------------------------------------------------------*/

typedef struct ov_ice_proxy_generic_config {

  ov_event_loop *loop;

  ov_socket_configuration external;

  struct {

    // only for dynamic proxy not for multiplexing

    struct {

      uint32_t min;
      uint32_t max;

    } ports;

    struct {

      ov_socket_configuration server;

    } stun;

    struct {

      ov_socket_configuration server;
      char username[OV_ICE_PROXY_GENERIC_TURN_USERNAME_MAX];
      char password[OV_ICE_PROXY_GENERIC_TURN_PASSWORD_MAX];

    } turn;

  } dynamic;

  struct {

    ov_ice_proxy_generic_dtls_config dtls;

    struct {

      uint64_t transaction_lifetime_usecs;

    } limits;

    struct {

      struct {

        uint64_t connectivity_pace_usecs;
        uint64_t session_timeout_usecs;
        uint64_t keepalive_usecs;

      } stun;

    } timeouts;

  } config;

  struct {

    void *userdata;

    struct {

      void (*drop)(void *userdata, const char *uuid);
      void (*state)(void *userdata, const char *uuid,
                    ov_ice_proxy_generic_state state);

    } session;

    struct {

      void (*io)(void *userdata, const char *session_id, int stream_id,
                 uint8_t *buffer, size_t size);

    } stream;

    struct {

      bool (*send)(void *userdata, ov_json_value *out);
      void (*end_of_candidates)(void *userdata, const char *session_id);

    } candidate;

  } callbacks;

} ov_ice_proxy_generic_config;

/*----------------------------------------------------------------------------*/

typedef struct ov_ice_proxy_generic ov_ice_proxy_generic;

struct ov_ice_proxy_generic {

  uint16_t magic_bytes;
  uint16_t type;

  ov_ice_proxy_generic_config config;

  ov_ice_proxy_generic *(*free)(ov_ice_proxy_generic *self);

  struct {

    const char *(*create)(ov_ice_proxy_generic *self, ov_sdp_session *sdp);

    bool (*drop)(ov_ice_proxy_generic *self, const char *session_id);

    bool (*update)(ov_ice_proxy_generic *self, const char *session_id,
                   const ov_sdp_session *sdp);

  } session;

  struct {

    bool (*candidate_in)(ov_ice_proxy_generic *self, const char *session_id,
                         uint32_t stream_id, const ov_ice_candidate *candidate);

    bool (*end_of_candidates_in)(ov_ice_proxy_generic *self,
                                 const char *session_id, uint32_t stream_id);

    uint32_t (*get_ssrc)(ov_ice_proxy_generic *self, const char *session_id,
                         uint32_t stream_id);

    ssize_t (*send)(ov_ice_proxy_generic *self, const char *session_id,
                    uint32_t stream_id, uint8_t *buffer, size_t size);

  } stream;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_ice_proxy_generic_config
ov_ice_proxy_generic_config_from_json(const ov_json_value *input);

/*----------------------------------------------------------------------------*/

ov_ice_proxy_generic *ov_ice_proxy_generic_free(ov_ice_proxy_generic *self);

/*----------------------------------------------------------------------------*/

ov_ice_proxy_generic *ov_ice_proxy_generic_cast(const void *data);

/*----------------------------------------------------------------------------*/

const char *ov_ice_proxy_generic_create_session(ov_ice_proxy_generic *self,
                                                ov_sdp_session *sdp);

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_generic_drop_session(ov_ice_proxy_generic *self,
                                       const char *session_id);

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_generic_update_session(ov_ice_proxy_generic *self,
                                         const char *session_id,
                                         const ov_sdp_session *sdp);

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_generic_stream_candidate_in(
    ov_ice_proxy_generic *self, const char *session_id, uint32_t stream_id,
    const ov_ice_candidate *candidate);

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_generic_stream_end_of_candidates_in(
    ov_ice_proxy_generic *self, const char *session_id, uint32_t stream_id);

/*----------------------------------------------------------------------------*/

uint32_t ov_ice_proxy_generic_stream_get_ssrc(ov_ice_proxy_generic *self,
                                              const char *session_id,
                                              uint32_t stream_id);

/*----------------------------------------------------------------------------*/

ssize_t ov_ice_proxy_generic_stream_send(ov_ice_proxy_generic *self,
                                         const char *session_id,
                                         uint32_t stream_id, uint8_t *buffer,
                                         size_t size);

#endif /* ov_ice_proxy_generic_h */
