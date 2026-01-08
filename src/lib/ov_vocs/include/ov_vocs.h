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
        @file           ov_vocs.h
        @author         Töpfer, Markus

        @date           2025-02-03


        ------------------------------------------------------------------------
*/
#ifndef ov_vocs_h
#define ov_vocs_h

#include "ov_mc_backend.h"
#include "ov_mc_backend_sip.h"
#include "ov_mc_backend_sip_static.h"
#include "ov_mc_backend_vad.h"
#include "ov_mc_frontend.h"
#include "ov_vocs_recorder.h"

#include <ov_core/ov_event_io.h>
#include <ov_core/ov_event_trigger.h>
#include <ov_core/ov_io.h>
#include <ov_ldap/ov_ldap.h>

#include <ov_vocs_db/ov_vocs_db.h>
#include <ov_vocs_db/ov_vocs_db_app.h>
#include <ov_vocs_db/ov_vocs_env.h>

#define OV_KEY_VOCS "vocs"
#define OV_VOCS_DEFAULT_SETTINGS_PATH "/tmp/vocs/settings"
#define OV_VOCS_DEFAULT_SESSIONS_PATH "/tmp/vocs/sessions"

/*----------------------------------------------------------------------------*/

#define OV_VOCS_DEFAULT_SDP                                                    \
  "v=0\\r\\n"                                                                  \
  "o=- 0 0 IN IP4 0.0.0.0\\r\\n"                                               \
  "s=-\\r\\n"                                                                  \
  "t=0 0\\r\\n"                                                                \
  "m=audio 0 UDP/TLS/RTP/SAVPF 100\\r\\n"                                      \
  "a=rtpmap:100 opus/48000/2\\r\\n"                                            \
  "a=fmtp:100 "                                                                \
  "maxplaybackrate=48000;stereo=1;"                                            \
  "useinbandfec=1\\r\\n"

/*----------------------------------------------------------------------------*/

#define OV_VOCS_DEFAULT_CODEC "{\"codec\":\"opus\",\"sample_rate_hz\":48000}"

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs ov_vocs;

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_event_loop *loop;
    ov_vocs_db *db;
    ov_vocs_db_persistance *persistance;
    ov_io *io;
    ov_event_trigger *trigger;
    ov_vocs_env env;

  struct {

        ov_mc_backend_config backend;
        ov_mc_frontend_config frontend;
        ov_mc_backend_sip_config sip;
        ov_mc_backend_sip_static_config sip_static;
        ov_vocs_recorder_config recorder;
        ov_mc_backend_vad_config vad;

  } module;

  struct {

    uint64_t response_usec;
    uint64_t reconnect_interval_usec;

  } timeout;

  struct {

    bool enable;
    ov_ldap_config config;

  } ldap;

  struct {

    char path[PATH_MAX];

  } sessions;

} ov_vocs_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vocs *ov_vocs_create(ov_vocs_config config);
ov_vocs *ov_vocs_cast(const void *data);
void *ov_vocs_free(void *self);

/*----------------------------------------------------------------------------*/

ov_vocs_config ov_vocs_config_from_json(const ov_json_value *val);

/*----------------------------------------------------------------------------*/

ov_event_io_config ov_vocs_event_io_uri_config(ov_vocs *self);

#endif /* ov_vocs_h */
