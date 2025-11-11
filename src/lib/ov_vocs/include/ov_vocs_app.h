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
        @file           ov_vocs_app.h
        @author         Töpfer, Markus

        @date           2025-11-07


        ------------------------------------------------------------------------
*/
#ifndef ov_vocs_app_h
#define ov_vocs_app_h

#include <ov_base/ov_event_loop.h>

#include <ov_ldap/ov_ldap.h>

#include <ov_vocs_db/ov_vocs_db.h>
#include <ov_vocs_db/ov_vocs_env.h>

typedef struct ov_vocs_app ov_vocs_app;
typedef struct ov_vocs_app_config ov_vocs_app_config;

struct ov_vocs_app_config {

    ov_event_loop *loop;
    
    void *core;
    ov_vocs_env env;

    ov_vocs_db *db;
    ov_vocs_db_persistance *persistance;

    ov_ldap *ldap;

    struct {

        char path[PATH_MAX];

    } sessions;

    struct {

        uint64_t response_usec;

    } limits;

};

#include "ov_vocs_core.h"



/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vocs_app *ov_vocs_app_create(ov_vocs_app_config config);
ov_vocs_app *ov_vocs_app_free(ov_vocs_app *self);
ov_vocs_app *ov_vocs_app_cast(const void *self);

bool ov_vocs_app_drop_connection(ov_vocs_app *self, int socket);

bool ov_vocs_app_api_process(ov_vocs_app *self, 
                             int socket,
                             const ov_event_parameter *params, 
                             ov_json_value *input);

/*
 *      ------------------------------------------------------------------------
 *
 *      ICE FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_vocs_app_ice_session_create(ov_vocs_app *self,
                                    const ov_response_state event,
                                    const char *session_id,
                                    const char *type,
                                    const char *sdp);

bool ov_vocs_app_ice_session_drop(ov_vocs_app *self,
                                  int socket,
                                  const char *session_id);

bool ov_vocs_app_ice_session_update(ov_vocs_app *self,
                                    const ov_response_state event,
                                    const char *session_id);

bool ov_vocs_app_ice_session_complete(ov_vocs_app *self,
                                      int socket,
                                      const char *session_id,
                                      bool success);

bool ov_vocs_app_ice_session_state(ov_vocs_app *self,
                                   const ov_response_state event,
                                   const char *session_id,
                                   const ov_json_value *state);

bool ov_vocs_app_ice_candidate(ov_vocs_app *self,
                               const ov_response_state event,
                               int socket,
                               const ov_ice_candidate_info *info);


bool ov_vocs_app_ice_end_of_candidates(ov_vocs_app *self,
                               const ov_response_state event,
                               int socket,
                               const ov_ice_candidate_info *info);

bool ov_vocs_app_ice_talk(ov_vocs_app *self,
                            const ov_response_state event,
                            const char *session_id,
                            int socket,
                            const ov_mc_loop_data data,
                            bool on);

/*
 *      ------------------------------------------------------------------------
 *
 *      MEDIA FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */


bool ov_vocs_app_send_media_ready(ov_vocs_app *self, 
                                  const char *uuid,
                                  int socket, 
                                  const char *session_id);

bool ov_vocs_app_join_loop(ov_vocs_app *self, 
                           const char *uuid,
                           const char *session_id,
                           const char *loopname,
                           uint64_t error_code,
                           const char *error_desc);

bool ov_vocs_app_leave_loop(ov_vocs_app *self, 
                           const char *uuid,
                           const char *session_id,
                           const char *loopname,
                           uint64_t error_code,
                           const char *error_desc);

bool ov_vocs_app_loop_volume(ov_vocs_app *self, 
                           const char *uuid,
                           const char *session_id,
                           const char *loopname,
                           uint8_t volume,
                           uint64_t error_code,
                           const char *error_desc);

bool ov_vocs_app_mixer_state(ov_vocs_app *self, 
                             const char *uuid,
                             const ov_json_value *state);

/*
 *      ------------------------------------------------------------------------
 *
 *      SIP FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_vocs_app_sip_call_init(ov_vocs_app *app,
                         const char *uuid,
                         const char *loopname,
                         const char *call_id,
                         const char *caller,
                         const char *callee,
                         uint8_t error_code,
                         const char *error_desc);

bool ov_vocs_app_sip_new(ov_vocs_app *self, 
                         const char *loopname,
                         const char *call_id,
                         const char *peer);

bool ov_vocs_app_sip_terminated(ov_vocs_app *self,
                                const char *call_id,
                                const char *loopname);

bool ov_vocs_app_sip_permit(ov_vocs_app *self,
                            const ov_sip_permission permission,
                            uint64_t error_code,
                            const char *error_desc);

bool ov_vocs_app_sip_revoke(ov_vocs_app *self,
                            const ov_sip_permission permission,
                            uint64_t error_code,
                            const char *error_desc);

bool ov_vocs_app_sip_list_calls(ov_vocs_app *self,
                                const char *uuid,
                                const ov_json_value *calls,
                                uint64_t error_code,
                                const char *error_desc);

bool ov_vocs_app_sip_list_permissions(ov_vocs_app *self,
                                const char *uuid,
                                const ov_json_value *permissions,
                                uint64_t error_code,
                                const char *error_desc);

bool ov_vocs_app_sip_get_status(ov_vocs_app *self,
                                const char *uuid,
                                const ov_json_value *status,
                                uint64_t error_code,
                                const char *error_desc);

bool ov_vocs_app_sip_connected(ov_vocs_app *self,
                                bool status);

/*
 *      ------------------------------------------------------------------------
 *
 *      RECORDER FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_vocs_app_record_start(ov_vocs_app *self,
     const char *uuid, ov_result error);

bool ov_vocs_app_record_stop(ov_vocs_app *self,
     const char *uuid, ov_result error);

/*
 *      ------------------------------------------------------------------------
 *
 *      VAD FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_vocs_app_vad(ov_vocs_app *self,
                     const char *loop,
                     bool on);

#endif /* ov_vocs_app_h */
