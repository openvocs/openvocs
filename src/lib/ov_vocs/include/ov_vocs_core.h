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
        @file           ov_vocs_core.h
        @author         Töpfer, Markus

        @date           2025-11-07


        ------------------------------------------------------------------------
*/
#ifndef ov_vocs_core_h
#define ov_vocs_core_h

#include "ov_mc_backend.h"
#include "ov_mc_backend_sip.h"
#include "ov_mc_backend_sip_static.h"
#include "ov_mc_backend_vad.h"
#include "ov_mc_frontend.h"
#include "ov_vocs_recorder.h"
#include "ov_vocs_app.h"

#include <ov_core/ov_event_io.h>
#include <ov_core/ov_event_trigger.h>
#include <ov_core/ov_io.h>
#include <ov_ldap/ov_ldap.h>

#include <ov_vocs_db/ov_vocs_db.h>
#include <ov_vocs_db/ov_vocs_env.h>

/*----------------------------------------------------------------------------*/

#define OV_VOCS_DEFAULT_SDP                                                    \
    "v=0\\r\\n"                                                                \
    "o=- 0 0 IN IP4 0.0.0.0\\r\\n"                                             \
    "s=-\\r\\n"                                                                \
    "t=0 0\\r\\n"                                                              \
    "m=audio 0 UDP/TLS/RTP/SAVPF 100\\r\\n"                                    \
    "a=rtpmap:100 opus/48000/2\\r\\n"                                          \
    "a=fmtp:100 "                                                              \
    "maxplaybackrate=48000;stereo=1;"                                          \
    "useinbandfec=1\\r\\n"

/*----------------------------------------------------------------------------*/

#define OV_VOCS_DEFAULT_CODEC "{\"codec\":\"opus\",\"sample_rate_hz\":48000}"

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_core ov_vocs_core;
typedef struct ov_vocs_core_config ov_vocs_core_config;

struct ov_vocs_core_config {

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
        ov_vocs_app_config app;

    } module;

    struct {

        uint64_t response_usec;
        uint64_t reconnect_interval_usec;

    } limits;

    struct {

        bool enable;
        ov_ldap_config config;

    } ldap;

    struct {

        char path[PATH_MAX];

    } sessions;


};


/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vocs_core *ov_vocs_core_create(ov_vocs_core_config config);
ov_vocs_core *ov_vocs_core_cast(const void *data);
void *ov_vocs_core_free(void *self);

/*----------------------------------------------------------------------------*/

ov_vocs_core_config ov_vocs_core_config_from_json(const ov_json_value *val);

ov_event_io_config ov_vocs_core_event_io_uri_config(ov_vocs_core *self);

/*
 *      ------------------------------------------------------------------------
 *
 *      SESSION FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_vocs_core_set_session(ov_vocs_core *self, int socket, const char *session_id);

bool ov_vocs_core_connection_drop(ov_vocs_core *self, 
                                  int socket,
                                  const char *session);

bool ov_vocs_core_session_drop(ov_vocs_core *self, 
                               const char *uuid, 
                               const char *session);

bool ov_vocs_core_session_create(ov_vocs_core *self,
                                 const char *uuid,
                                 const char *sdp);

bool ov_vocs_core_session_update(ov_vocs_core *self,
                                 const char *uuid,
                                 const char *session,
                                 ov_media_type type,
                                 const char *sdp);

/*
 *      ------------------------------------------------------------------------
 *
 *      ICE FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_vocs_core_candidate(ov_vocs_core *self,
                            const char *uuid,
                            const char *session_id,
                            const ov_ice_candidate_info *info);

bool ov_vocs_core_end_of_candidates(ov_vocs_core *self,
                            const char *uuid,
                            const char *session_id);

/*
 *      ------------------------------------------------------------------------
 *
 *      LOOP FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_vocs_core_perform_switch_loop_request(ov_vocs_core *self,
                                        const char *uuid,
                                        const char *sess,
                                        const char *user,
                                        const char *role,
                                        const char *loop,
                                        ov_vocs_permission current,
                                        ov_vocs_permission request);

bool ov_vocs_core_set_loop_volume(ov_vocs_core *self,
                                        const char *uuid,
                                        const char *sess,
                                        const char *loop,
                                        uint64_t percent);

bool ov_vocs_core_leave_loop(ov_vocs_core *self,
                             const char *uuid,
                             const char *session_id, 
                             const char *loop);

bool ov_vocs_core_ptt(ov_vocs_core *self,
                      const char *user, 
                      const char *role,
                      const char *loop,
                      bool off);

/*
 *      ------------------------------------------------------------------------
 *
 *      SIP FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

char* ov_vocs_core_sip_create_call(ov_vocs_core *self,
                                   const char *loop,
                                   const char *dest,
                                   const char *from);

bool ov_vocs_core_sip_terminate_call(ov_vocs_core *self,
                                     const char *call_id);

bool ov_vocs_core_sip_create_permission(ov_vocs_core *self,
                                     ov_sip_permission permission);

bool ov_vocs_core_sip_terminate_permission(ov_vocs_core *self,
                                     ov_sip_permission permission);

bool ov_vocs_core_sip_list_calls(ov_vocs_core *self, const char *uuid);

bool ov_vocs_core_sip_list_call_permissions(ov_vocs_core *self, const char *uuid);

bool ov_vocs_core_sip_list_sip_status(ov_vocs_core *self, const char *uuid);

bool ov_vocs_core_get_sip_status(ov_vocs_core *self);

/*
 *      ------------------------------------------------------------------------
 *
 *      RECORDING FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_json_value *ov_vocs_core_get_recording(ov_vocs_core *self, 
                                          ov_db_recordings_get_params params);

bool ov_vocs_core_start_recording(ov_vocs_core *self, 
                                  const char *uuid,
                                  const char *loop,
                                  void *userdata,
                                  int socket,
                                  void (*function)(void *,int, const char*, const char*, ov_result));

bool ov_vocs_core_stop_recording(ov_vocs_core *self, 
                                  const char *uuid,
                                  const char *loop,
                                  void *userdata,
                                  int socket,
                                  void (*function)(void *,int, const char*, const char*, ov_result));

ov_json_value *ov_vocs_core_get_recorded_loops(ov_vocs_core *self);

/*
 *      ------------------------------------------------------------------------
 *
 *      MIXER FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_backend_registry_count ov_vocs_core_count_mixers(ov_vocs_core *self);


bool ov_vocs_core_get_mixer_state(ov_vocs_core *self,
    const char *uuid, const char *session);



#endif /* ov_vocs_core_h */
