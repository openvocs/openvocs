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
        @file           ov_vocs_core.c
        @author         Töpfer, Markus

        @date           2025-11-07


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_core.h"
#include "../include/ov_vocs_app.h"

#define OV_VOCS_CORE_MAGIC_BYTES 0xc042

/*----------------------------------------------------------------------------*/

struct ov_vocs_core {

    uint16_t magic_byte;
    ov_vocs_core_config config;

    bool debug;

    ov_mc_backend *backend;
    ov_mc_frontend *frontend;
    ov_mc_backend_sip *sip;
    ov_mc_backend_sip_static *sip_static;
    ov_vocs_recorder *recorder;
    ov_mc_backend_vad *vad;
    ov_vocs_app *app;

    ov_ldap *ldap;

    ov_dict *sessions; // sessions spanning over ice proxy and resmgr

    ov_callback_registry *callbacks;

}; 

/*----------------------------------------------------------------------------*/




/*
 *      ------------------------------------------------------------------------
 *
 *      #BACKEND FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_backend_mixer_lost(void *userdata, const char *uuid);

static void cb_backend_mixer_acquired(void *userdata,
                                      const char *uuid,
                                      const char *session_id,
                                      uint64_t error_code,
                                      const char *error_desc);

static void cb_backend_mixer_released(void *userdata,
                                      const char *uuid,
                                      const char *session_id,
                                      uint64_t error_code,
                                      const char *error_desc);

static void cb_backend_mixer_join(void *userdata,
                                  const char *uuid,
                                  const char *session_id,
                                  const char *loopname,
                                  uint64_t error_code,
                                  const char *error_desc);

static void cb_backend_mixer_leave(void *userdata,
                                   const char *uuid,
                                   const char *session_id,
                                   const char *loopname,
                                   uint64_t error_code,
                                   const char *error_desc);

static void cb_backend_mixer_volume(void *userdata,
                                    const char *uuid,
                                    const char *session_id,
                                    const char *loopname,
                                    uint8_t volume,
                                    uint64_t error_code,
                                    const char *error_desc);

static void cb_backend_mixer_state(void *userdata,
                                   const char *uuid,
                                   const ov_json_value *state);

/*----------------------------------------------------------------------------*/

static void cb_backend_mixer_lost(void *userdata, const char *uuid) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self || !uuid) goto error;

    intptr_t socket = (intptr_t)ov_dict_get(self->sessions, uuid);

    ov_log_error("mixer lost %s", uuid);

    ov_vocs_app_drop_connection(self->app, socket);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_backend_mixer_acquired(void *userdata,
                                      const char *uuid,
                                      const char *session_id,
                                      uint64_t error_code,
                                      const char *error_desc) {

    ov_id new_uuid = {0};

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self || !uuid || !session_id) goto error;

    intptr_t socket = (intptr_t)ov_dict_get(self->sessions, session_id);
    if (0 == socket) goto drop_mixer_acquisition;

    switch (error_code) {

        case OV_ERROR_NOERROR:
            break;

        default:

            ov_log_error("acquire mixer failed %i|%s for %s",
                         error_code,
                         error_desc,
                         session_id);

            ov_vocs_app_drop_connection(self->app, socket);
            goto error;
    }

    if (!ov_vocs_app_send_media_ready(self->app, uuid, socket, session_id))
        goto error;

drop_mixer_acquisition:

    ov_id_fill_with_uuid(new_uuid);
    ov_mc_backend_release_mixer(
        self->backend, new_uuid, session_id, self, cb_backend_mixer_released);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_backend_mixer_released(void *userdata,
                                      const char *uuid,
                                      const char *session_id,
                                      uint64_t error_code,
                                      const char *error_desc) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self || !uuid || !session_id) goto error;

    intptr_t socket = (intptr_t)ov_dict_get(self->sessions, session_id);
    if (0 == socket) goto error;

    switch (error_code) {

        case OV_ERROR_NOERROR:
            break;

        default:

            ov_log_error("release mixer failed %i|%s for %s",
                         error_code,
                         error_desc,
                         session_id);

            goto error;
    }

    ov_vocs_app_drop_connection(self->app, socket);
error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_backend_mixer_join(void *userdata,
                                  const char *uuid,
                                  const char *session_id,
                                  const char *loopname,
                                  uint64_t error_code,
                                  const char *error_desc) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self || !loopname || !uuid || !session_id) goto error;

    if (!ov_vocs_app_join_loop(self->app, 
                               uuid, 
                               session_id, 
                               loopname,
                               error_code,
                               error_desc)) goto switch_off_loop;

    return;

switch_off_loop:
    ov_mc_backend_leave_loop(self->backend,
                             uuid,
                             session_id,
                             loopname,
                             self,
                             cb_backend_mixer_leave);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_backend_mixer_leave(void *userdata,
                                   const char *uuid,
                                   const char *session_id,
                                   const char *loopname,
                                   uint64_t error_code,
                                   const char *error_desc) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self || !loopname || !uuid || !session_id) goto error;

    intptr_t socket = (intptr_t)(void*)ov_dict_get(self->sessions, session_id);

    if (!ov_vocs_app_leave_loop(self->app,
                                uuid, 
                                session_id,
                                loopname,
                                error_code,
                                error_desc)) goto error;

    return;
   
error:
    ov_vocs_app_drop_connection(self->app, socket);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_backend_mixer_volume(void *userdata,
                                    const char *uuid,
                                    const char *session_id,
                                    const char *loopname,
                                    uint8_t volume,
                                    uint64_t error_code,
                                    const char *error_desc) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self || !loopname || !uuid || !session_id) goto error;

    intptr_t socket = (intptr_t)(void*)ov_dict_get(self->sessions, session_id);

    if (!ov_vocs_app_loop_volume(self->app,
                                uuid, 
                                session_id,
                                loopname,
                                volume,
                                error_code,
                                error_desc)) goto error;

    return;
   
error:
    ov_vocs_app_drop_connection(self->app, socket);
    return;
}


/*----------------------------------------------------------------------------*/

static void cb_backend_mixer_state(void *userdata,
                                   const char *uuid,
                                   const ov_json_value *state) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self) return;

    ov_vocs_app_mixer_state(self->app, uuid, state);
    return;
}   

/*----------------------------------------------------------------------------*/

static bool module_load_backend(ov_vocs_core *self) {

    OV_ASSERT(self);

    self->config.module.backend.io = self->config.io;
    self->config.module.backend.loop = self->config.loop;
    self->config.module.backend.callback.userdata = self;
    self->config.module.backend.callback.mixer.lost = cb_backend_mixer_lost;

    self->backend = ov_mc_backend_create(self->config.module.backend);
    if (!self->backend) return false;

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #FRONTEND FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_frontend_session_dropped(void *userdata,
                                        const char *session_id) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self || !session_id) goto error;

    intptr_t socket = (intptr_t)ov_dict_get(self->sessions, session_id);
    ov_vocs_app_drop_connection(self->app, socket);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_frontend_session_created(
    void *userdata,
    const ov_response_state event,
    const char *session_id,
    const char *type,
    const char *sdp,
    size_t array_size,
    const ov_ice_proxy_vocs_stream_forward_data *array) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self) goto error;

    UNUSED(array_size);

    if (!ov_vocs_app_ice_session_create(self->app, 
                                        event,
                                        session_id,
                                        type,
                                        sdp)) goto error;

    ov_mc_mixer_core_forward forward = (ov_mc_mixer_core_forward){
        .socket = array[0].socket, .ssrc = array[0].ssrc, .payload_type = 100};

    if (!ov_mc_backend_acquire_mixer(self->backend,
                                     event.id,
                                     session_id,
                                     forward,
                                     self,
                                     cb_backend_mixer_acquired)) {

        ov_log_error("failed to request aquire_mixer %s", session_id);
        goto error;
    }

    return;

error:
    ov_mc_frontend_drop_session(self->frontend, event.id, session_id);
    return;
}

/*----------------------------------------------------------------------------*/


static void cb_frontend_session_completed(void *userdata,
                                          const char *session_id,
                                          bool success) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self) goto error;

    intptr_t socket = (intptr_t)ov_dict_get(self->sessions, session_id);

    ov_vocs_app_ice_session_complete(self->app, socket, session_id, success);

    return;
error:
    return;
}


/*----------------------------------------------------------------------------*/

static void cb_frontend_session_update(void *userdata,
                                       const ov_response_state event,
                                       const char *session_id) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self) goto error;

    if (!session_id) goto error;

    if (!ov_vocs_app_ice_session_update(self->app,
                                        event,
                                        session_id)) goto error;

    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_frontend_session_state(void *userdata,
                                      const ov_response_state event,
                                      const char *session_id,
                                      const ov_json_value *state) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self || !session_id || !state) goto error;

    if (!ov_vocs_app_ice_session_state(self->app,
                                       event,
                                       session_id,
                                       state)) goto error;

    if (!ov_mc_backend_get_session_state(
            self->backend, event.id, session_id, self, cb_backend_mixer_state))
        goto error;

    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_frontend_candidate(void *userdata,
                                  const ov_response_state event,
                                  const char *session_id,
                                  const ov_ice_candidate_info *info) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self || !session_id || !info) goto error;

    intptr_t socket = (intptr_t)(void*) ov_dict_get(self->sessions, session_id);

    if (!ov_vocs_app_ice_candidate(self->app, event, socket, info)) goto error;

    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_frontend_end_of_candidates(void *userdata,
                                          const ov_response_state event,
                                          const char *session_id,
                                          const ov_ice_candidate_info *info) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self || !session_id || !info) goto error;

    intptr_t socket = (intptr_t)(void*) ov_dict_get(self->sessions, session_id);

    if (!ov_vocs_app_ice_end_of_candidates(self->app, event, socket, info)) goto error;

    return;
error:
    return;
}


/*----------------------------------------------------------------------------*/

static void cb_frontend_talk(void *userdata,
                             const ov_response_state event,
                             const char *session_id,
                             const ov_mc_loop_data ldata,
                             bool on) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self || !session_id) goto error;

    intptr_t socket = (intptr_t)(void*) ov_dict_get(self->sessions, session_id);

    if (!ov_vocs_app_ice_talk(self->app, event, session_id, socket, ldata, on)) goto error;

    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool module_load_frontend(ov_vocs_core *self) {

    OV_ASSERT(self);

    self->config.module.frontend.io = self->config.io;

    self->config.module.frontend.loop = self->config.loop;
    self->config.module.frontend.callback.userdata = self;
    self->config.module.frontend.callback.session.dropped =
        cb_frontend_session_dropped;
    self->config.module.frontend.callback.session.created =
        cb_frontend_session_created;
    self->config.module.frontend.callback.session.completed =
        cb_frontend_session_completed;
    self->config.module.frontend.callback.session.update =
        cb_frontend_session_update;
    self->config.module.frontend.callback.session.state =
        cb_frontend_session_state;
    self->config.module.frontend.callback.candidate = cb_frontend_candidate;
    self->config.module.frontend.callback.end_of_candidates =
        cb_frontend_end_of_candidates;
    self->config.module.frontend.callback.talk = cb_frontend_talk;

    self->frontend = ov_mc_frontend_create(self->config.module.frontend);
    if (!self->frontend) return false;

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #LDAP FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool module_load_ldap(ov_vocs_core *self) {

    OV_ASSERT(self);

    self->config.ldap.config.loop = self->config.loop;
    self->ldap = ov_ldap_create(self->config.ldap.config);
    if (!self->ldap) goto error;

    return true;
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #SIP FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_call_init(void *userdata,
                         const char *uuid,
                         const char *loopname,
                         const char *call_id,
                         const char *caller,
                         const char *callee,
                         uint8_t error_code,
                         const char *error_desc) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self) goto error;

    if (!ov_vocs_app_sip_call_init(
        self->app,
        uuid,
        loopname,
        call_id,
        caller,
        callee,
        error_code,
        error_desc)) goto error;

    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_sip_new(void *userdata,
                       const char *loopname,
                       const char *call_id,
                       const char *peer) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self) goto error;

    if (!ov_vocs_app_sip_new(self->app, 
                             loopname, 
                             call_id,
                             peer)) goto error;

    return;
error:
    return;
}


/*----------------------------------------------------------------------------*/

static void cb_sip_terminated(void *userdata,
                              const char *call_id,
                              const char *loopname) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self) goto error;

    if (!ov_vocs_app_sip_terminated(
        self->app,
        call_id, 
        loopname)) goto error;

    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_sip_permit(void *userdata,
                          const ov_sip_permission permission,
                          uint64_t error_code,
                          const char *error_desc) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self) goto error;

    if (!ov_vocs_app_sip_permit(
        self->app,
        permission, 
        error_code,
        error_desc)) goto error;

    return;
error:
    return;
}


/*----------------------------------------------------------------------------*/

static void cb_sip_revoke(void *userdata,
                          const ov_sip_permission permission,
                          uint64_t error_code,
                          const char *error_desc) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self) goto error;

    if (!ov_vocs_app_sip_revoke(
        self->app,
        permission, 
        error_code,
        error_desc)) goto error;

    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_sip_list_calls(void *userdata,
                              const char *uuid,
                              const ov_json_value *calls,
                              uint64_t error_code,
                              const char *error_desc) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self) goto error;

    if (!ov_vocs_app_sip_list_calls(
        self->app,
        uuid, 
        calls,
        error_code,
        error_desc)) goto error;

    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_sip_list_permissions(void *userdata,
                                    const char *uuid,
                                    const ov_json_value *permissions,
                                    uint64_t error_code,
                                    const char *error_desc) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self) goto error;

    if (!ov_vocs_app_sip_list_permissions(
        self->app,
        uuid, 
        permissions,
        error_code,
        error_desc)) goto error;

    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_sip_get_status(void *userdata,
                              const char *uuid,
                              const ov_json_value *status,
                              uint64_t error_code,
                              const char *error_desc) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self) goto error;

    if (!ov_vocs_app_sip_get_status(
        self->app,
        uuid, 
        status,
        error_code,
        error_desc)) goto error;

    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_sip_connected(void *userdata, bool status) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self) goto error;

    if (!ov_vocs_app_sip_connected(
        self->app,
        status)) goto error;

    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool module_load_sip(ov_vocs_core *self) {

    OV_ASSERT(self);

    self->config.module.sip.io = self->config.io;
    self->config.module.sip.backend = self->backend;
    self->config.module.sip.loop = self->config.loop;
    self->config.module.sip.db = self->config.db;

    self->config.module.sip.callback.userdata = self;
    self->config.module.sip.callback.call.init = cb_call_init;
    self->config.module.sip.callback.call.new = cb_sip_new;
    self->config.module.sip.callback.call.terminated = cb_sip_terminated;
    self->config.module.sip.callback.call.permit = cb_sip_permit;
    self->config.module.sip.callback.call.revoke = cb_sip_revoke;
    self->config.module.sip.callback.list_calls = cb_sip_list_calls;
    self->config.module.sip.callback.list_permissions = cb_sip_list_permissions;
    self->config.module.sip.callback.get_status = cb_sip_get_status;
    self->config.module.sip.callback.connected = cb_sip_connected;

    self->sip = ov_mc_backend_sip_create(self->config.module.sip);
    if (!self->sip) return false;

    return true;
}

/*----------------------------------------------------------------------------*/

static bool module_load_sip_static(ov_vocs_core *self) {

    OV_ASSERT(self);

    self->config.module.sip_static.io = self->config.io;
    self->config.module.sip_static.backend = self->backend;
    self->config.module.sip_static.loop = self->config.loop;
    self->config.module.sip_static.db = self->config.db;

    self->sip_static =
        ov_mc_backend_sip_static_create(self->config.module.sip_static);
    if (!self->sip_static) return false;

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #RECORDER FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_start_record(void *userdata, const char *uuid, ov_result error){

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self || !uuid) goto error;

    if (!ov_vocs_app_record_start(self->app,
                                  uuid, 
                                  error)) goto error;

    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_stop_record(void *userdata, const char *uuid, ov_result error){

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self || !uuid) goto error;

    if (!ov_vocs_app_record_stop(self->app,
                                  uuid, 
                                  error)) goto error;

    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool module_load_recorder(ov_vocs_core *self) {

    OV_ASSERT(self);

    self->config.module.recorder.loop = self->config.loop;
    self->config.module.recorder.vocs_db = self->config.db;

    self->config.module.recorder.timeout.response_usec = self->config.limits.response_usec;

    self->config.module.recorder.callbacks.userdata =self;
    self->config.module.recorder.callbacks.start_record = cb_start_record;
    self->config.module.recorder.callbacks.stop_record = cb_stop_record;

    self->recorder = ov_vocs_recorder_create(self->config.module.recorder);
    if (!self->recorder) return false;

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #VAD FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void io_vad(void *userdata, const char *loop, bool on) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self) goto error;

    if (!ov_vocs_app_vad(self->app,
                         loop,
                         on)) goto error;

    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool module_load_vad(ov_vocs_core *self) {

    OV_ASSERT(self);

    self->config.module.vad.loop = self->config.loop;
    self->config.module.vad.db = self->config.db;
    self->config.module.vad.io = self->config.io;

    self->config.module.vad.callbacks.userdata = self;
    self->config.module.vad.callbacks.vad = io_vad;

    self->vad = ov_mc_backend_vad_create(self->config.module.vad);
    if (!self->vad) return false;

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #APP FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool module_load_app(ov_vocs_core *self) {

    OV_ASSERT(self);

    self->config.module.app.loop = self->config.loop;
    self->config.module.app.core = self;
    self->config.module.app.env = self->config.env;
    self->config.module.app.db = self->config.db;
    self->config.module.app.persistance = self->config.persistance;
    self->config.module.app.ldap = self->ldap;

    strncpy(self->config.module.app.sessions.path, 
        self->config.sessions.path,PATH_MAX);

    self->config.module.app.limits.response_usec = self->config.limits.response_usec;

    self->app = ov_vocs_app_create(self->config.module.app);
    if (!self->app) return false;

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #LOOP FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */



bool ov_vocs_core_ptt(ov_vocs_core *self,
                      const char *user, 
                      const char *role,
                      const char *loop,
                      bool off){

    if (!self) goto error;

    ov_vocs_recorder_ptt(self->recorder, user, role, loop, off);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_core_leave_loop(ov_vocs_core *self,
                             const char *uuid,
                             const char *session_id, 
                             const char *loop){

    if (!self || !uuid || !session_id || !loop) goto error;

    return ov_mc_backend_leave_loop(self->backend,
                                  uuid,
                                  session_id,
                                  loop,
                                  self,
                                  cb_backend_mixer_leave);
error:
    return false;

}

/*----------------------------------------------------------------------------*/

bool ov_vocs_core_set_loop_volume(ov_vocs_core *self,
                                        const char *uuid,
                                        const char *sess,
                                        const char *loop,
                                        uint64_t percent){

    if (!self) goto error;

    if (!ov_mc_backend_set_loop_volume(self->backend,
                                       uuid,
                                       sess,
                                       loop,
                                       percent,
                                       self,
                                       cb_backend_mixer_volume)) goto error;

    return true;
error:
    return false;

}

/*----------------------------------------------------------------------------*/

bool ov_vocs_core_perform_switch_loop_request(ov_vocs_core *self,
                                        const char *uuid,
                                        const char *sess,
                                        const char *user,
                                        const char *role,
                                        const char *loop,
                                        ov_vocs_permission current,
                                        ov_vocs_permission request) {

    if (!self || !uuid || !sess || !loop) goto error;

    ov_socket_configuration multicast =
        ov_vocs_db_get_multicast_group(self->config.db, loop);

    if (0 == multicast.host[0]) {
        ov_log_error("no multicast group configured for loop %s", loop);
        goto error;
    }

    ov_mc_loop_data data = {0};
    data.socket = multicast;
    strncpy(data.name, loop, OV_MC_LOOP_NAME_MAX);
    data.volume = ov_vocs_db_get_volume(self->config.db, user, role, loop);

    switch (current) {

        case OV_VOCS_NONE:

            switch (request) {

                case OV_VOCS_NONE:
                    break;

                case OV_VOCS_RECV:
                case OV_VOCS_SEND:

                    ov_vocs_recorder_join_loop(
                        self->recorder, user, role, loop);

                    return ov_mc_backend_join_loop(self->backend,
                                                   uuid,
                                                   sess,
                                                   data,
                                                   self,
                                                   cb_backend_mixer_join);

                    break;
            }

            break;

        case OV_VOCS_RECV:

            switch (request) {

                case OV_VOCS_NONE:

                    ov_vocs_recorder_leave_loop(
                        self->recorder, user, role, loop);

                    return ov_mc_backend_leave_loop(self->backend,
                                                    uuid,
                                                    sess,
                                                    loop,
                                                    self,
                                                    cb_backend_mixer_leave);

                    break;

                case OV_VOCS_RECV:
                    break;

                case OV_VOCS_SEND:

                    ov_vocs_recorder_talk_on_loop(
                        self->recorder, user, role, loop);

                    return ov_mc_frontend_talk(
                        self->frontend, uuid, sess, true, data);

                    break;
            }

            break;

        case OV_VOCS_SEND:

            switch (request) {

                case OV_VOCS_NONE:
                case OV_VOCS_RECV:

                    ov_vocs_recorder_talk_off_loop(
                        self->recorder, user, role, loop);

                    return ov_mc_frontend_talk(
                        self->frontend, uuid, sess, false, data);

                    break;

                case OV_VOCS_SEND:
                    break;
            }

            break;
    }

error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #SIP FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

char* ov_vocs_core_sip_create_call(ov_vocs_core *self,
                                   const char *loop,
                                   const char *dest,
                                   const char *from){

    return ov_mc_backend_sip_create_call(self->sip, loop, dest, from);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_core_sip_terminate_call(ov_vocs_core *self,
                                     const char *call_id){

    return ov_mc_backend_sip_terminate_call(self->sip, call_id);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_core_sip_create_permission(ov_vocs_core *self,
                                     ov_sip_permission permission){

    return ov_mc_backend_sip_create_permission(self->sip, permission);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_core_sip_terminate_permission(ov_vocs_core *self,
                                     ov_sip_permission permission){

    return ov_mc_backend_sip_terminate_permission(self->sip, permission);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_core_sip_list_calls(ov_vocs_core *self, const char *uuid){

    return ov_mc_backend_sip_list_calls(self->sip, uuid);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_core_sip_list_call_permissions(ov_vocs_core *self, const char *uuid){

    return ov_mc_backend_sip_list_permissions(self->sip, uuid);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_core_sip_list_sip_status(ov_vocs_core *self, const char *uuid){

    return ov_mc_backend_sip_get_status(self->sip, uuid);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_core_get_sip_status(ov_vocs_core *self){

    return ov_mc_backend_sip_get_connect_status(self->sip);
}

/*
 *      ------------------------------------------------------------------------
 *
 *      RECORDING FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_json_value *ov_vocs_core_get_recording(ov_vocs_core *self, 
                                          ov_db_recordings_get_params params){

    return ov_vocs_recorder_get_recording(self->recorder, params);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_core_start_recording(ov_vocs_core *self, 
                                  const char *uuid,
                                  const char *loop,
                                  void *userdata,
                                  int socket,
                                  void (*callback)(void *,int, const char*, const char*, ov_result)){

    return ov_vocs_recorder_start_loop_recording(
        self->recorder, uuid, loop, userdata, socket, callback);

}

/*----------------------------------------------------------------------------*/

bool ov_vocs_core_stop_recording(ov_vocs_core *self, 
                                  const char *uuid,
                                  const char *loop,
                                  void *userdata,
                                  int socket,
                                  void (*callback)(void *,int, const char*, const char*, ov_result)){

    return ov_vocs_recorder_stop_loop_recording(
        self->recorder, uuid, loop, userdata, socket, callback);

}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_core_get_recorded_loops(ov_vocs_core *self){

    return ov_vocs_recorder_get_recorded_loops(self->recorder);
}

/*
 *      ------------------------------------------------------------------------
 *
 *      MIXER FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_backend_registry_count ov_vocs_core_count_mixers(ov_vocs_core *self){

    return ov_mc_backend_state_mixers(self->backend);
}


/*----------------------------------------------------------------------------*/

static void cb_mixer_state_mgmt(void *userdata, const char *uuid, const ov_json_value *input){

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self || !uuid || !input) goto error;

    ov_vocs_app_mixer_state(self->app, uuid, input);
error:
    return;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_core_get_mixer_state(ov_vocs_core *self,
    const char *uuid, const char *session){

    if (!self || !uuid || !session) goto error;

    if (!ov_mc_backend_get_session_state(self->backend,
        uuid, session, self, cb_mixer_state_mgmt)) {
        goto error;
    
    }

    return true;
error:
    return false;
}

struct container_updata_sip {

    const char *loop;
    ov_vocs_core *vocs;
};

/*----------------------------------------------------------------------------*/

static bool revoke_sip_call(void *item, void *data) {

    struct container_updata_sip *container =
        (struct container_updata_sip *)data;

    ov_sip_permission permission = (ov_sip_permission){
        .caller = ov_json_string_get(ov_json_object_get(item, OV_KEY_CALLER)),
        .callee = ov_json_string_get(ov_json_object_get(item, OV_KEY_CALLEE)),
        .loop = container->loop,
        .from_epoch =
            ov_json_number_get(ov_json_object_get(item, OV_KEY_VALID_FROM)),
        .until_epoch =
            ov_json_number_get(ov_json_object_get(item, OV_KEY_VALID_UNTIL))};

    return ov_mc_backend_sip_terminate_permission(
        container->vocs->sip, permission);
}

/*----------------------------------------------------------------------------*/

static bool permit_sip_call(void *item, void *data) {

    struct container_updata_sip *container =
        (struct container_updata_sip *)data;

    ov_sip_permission permission = (ov_sip_permission){
        .caller = ov_json_string_get(ov_json_object_get(item, OV_KEY_CALLER)),
        .callee = ov_json_string_get(ov_json_object_get(item, OV_KEY_CALLEE)),
        .loop = container->loop,
        .from_epoch =
            ov_json_number_get(ov_json_object_get(item, OV_KEY_VALID_FROM)),
        .until_epoch =
            ov_json_number_get(ov_json_object_get(item, OV_KEY_VALID_UNTIL))};

    return ov_mc_backend_sip_create_permission(
        container->vocs->sip, permission);
}

/*----------------------------------------------------------------------------*/

static bool update_sip_backend(const void *key, void *val, void *data) {

    if (!key) return true;

    const char *loop = (const char *)key;

    ov_json_value *revoke = ov_json_object_get(val, OV_KEY_REVOKE);
    ov_json_value *permit = ov_json_object_get(val, OV_KEY_PERMIT);

    struct container_updata_sip container =
        (struct container_updata_sip){.loop = loop, .vocs = ov_vocs_core_cast(data)};

    ov_json_array_for_each(revoke, &container, revoke_sip_call);
    ov_json_array_for_each(permit, &container, permit_sip_call);

    return true;
}

/*----------------------------------------------------------------------------*/

static void process_trigger(void *userdata, ov_json_value *input) {

    ov_vocs_core *vocs = ov_vocs_core_cast(userdata);
    if (!vocs || !input) goto error;

    const char *event = ov_event_api_get_event(input);
    if (!event) goto error;

    if (0 == ov_string_compare(event, "update_db")) {

        ov_json_value *proc = (ov_json_value *)ov_json_get(
            input, "/" OV_KEY_PARAMETER "/" OV_KEY_PROCESSING);
        ov_json_object_for_each(proc, vocs, update_sip_backend);
    }

    input = ov_json_value_free(input);
error:
    input = ov_json_value_free(input);
}

/*
 *      ------------------------------------------------------------------------
 *
 *      SESSION FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_vocs_core_set_session(ov_vocs_core *self, int socket, const char *session_id){

    if (!self || !session_id) goto error;

    char *key = ov_string_dup(session_id);

    if (!ov_dict_set(self->sessions, key, (void*)(intptr_t)socket, NULL)){

        key = ov_data_pointer_free(key);
        goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_core_connection_drop(ov_vocs_core *self, 
                                  int socket,
                                  const char *session){

    if (NULL != session) {

        ov_id uuid = {0};
        ov_id_fill_with_uuid(uuid);

        ov_mc_frontend_drop_session(self->frontend, uuid, session);
        ov_mc_backend_release_mixer(self->backend,
                                         uuid,
                                         session,
                                         self,
                                         cb_backend_mixer_released);

        ov_dict_del(self->sessions, session);
    }


    return ov_vocs_app_drop_connection(self->app, socket);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_core_session_drop(ov_vocs_core *self, 
                               const char *uuid, 
                               const char *session){

    return ov_mc_frontend_drop_session(self->frontend, uuid, session);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_core_session_create(ov_vocs_core *self,
                                 const char *uuid,
                                 const char *sdp){

    return ov_mc_frontend_create_session(self->frontend, uuid, sdp);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_core_session_update(ov_vocs_core *self,
                                 const char *uuid,
                                 const char *session,
                                 ov_media_type type,
                                 const char *sdp){

    return ov_mc_frontend_update_session(self->frontend, 
        uuid, session, type, sdp);
}

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
                            const ov_ice_candidate_info *info){

    return ov_mc_frontend_candidate(self->frontend, uuid, session_id, info);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_core_end_of_candidates(ov_vocs_core *self,
                            const char *uuid,
                            const char *session_id){

    return ov_mc_frontend_end_of_candidates(self->frontend, uuid, session_id);
}



/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool init_config(ov_vocs_core_config *config){

    if (!config) goto error;

    if (!config->loop){

        ov_log_error("No eventloop configured - abort");
        goto error;

    }

    if (!config->env.userdata || !config->env.send || !config->env.close){

        ov_log_error("No evironment configured - abort");
        goto error;
    }

    if (0 == config->sessions.path[0]) {
        strncpy(config->sessions.path, "/tmp/openvocs/sessions/", PATH_MAX);
    }

    if (0 == config->limits.response_usec)
        config->limits.response_usec = 5000000;

    if (0 == config->limits.reconnect_interval_usec)
        config->limits.reconnect_interval_usec = 2000000;

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_vocs_core *ov_vocs_core_create(ov_vocs_core_config config){

    ov_vocs_core *self = NULL;

    if (!init_config(&config)) goto error;

    self = calloc(1, sizeof(ov_vocs_core));
    if (!self) goto error;

    self->magic_byte = OV_VOCS_CORE_MAGIC_BYTES;
    self->config = config;

    self->sessions = ov_dict_create(ov_dict_string_key_config(255));
    if (!self->sessions) goto error;

    if (!module_load_backend(self)) {
        ov_log_error("Failed to enable Backend");
        goto error;
    }

    if (config.ldap.enable && !module_load_ldap(self)) {
        ov_log_error("Failed to enable LDAP");
        goto error;
    }

    if (!module_load_frontend(self)) {
        ov_log_error("Failed to enable Frontend");
        goto error;
    }

    if (!module_load_sip(self)) {
        ov_log_error("Failed to enable SIP");
        goto error;
    }

    if (!module_load_sip_static(self)) {
        ov_log_error("Failed to enable SIP static");
        goto error;
    }

    if (!module_load_recorder(self)) {
        ov_log_error("Failed to enable Recorder");
        goto error;
    }

    if (!module_load_vad(self)) {
        ov_log_error("Failed to enable VAD");
        goto error;
    }

    if (!module_load_app(self)) {
        ov_log_error("Failed to enable APP");
        goto error;
    }

    if (config.trigger)
        ov_event_trigger_register_listener(
            config.trigger,
            "VOCS",
            (ov_event_trigger_data){
                .userdata = self, .process = process_trigger});

    self->callbacks = ov_callback_registry_create((ov_callback_registry_config){
        .loop = self->config.loop,
        .timeout_usec = self->config.limits.response_usec
    });

    if (!self->callbacks) goto error;

    return self;

error:
    ov_vocs_core_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_core *ov_vocs_core_cast(const void *self){

    if (!self) goto error;

    if (*(uint16_t *)self == OV_VOCS_CORE_MAGIC_BYTES) 
        return (ov_vocs_core *)self;
error:
    return NULL;

}

/*----------------------------------------------------------------------------*/

void *ov_vocs_core_free(void *userdata){

    ov_vocs_core *self = ov_vocs_core_cast(userdata);
    if (!self) return userdata;

    self->backend = ov_mc_backend_free(self->backend);
    self->frontend = ov_mc_frontend_free(self->frontend);
    self->sip = ov_mc_backend_sip_free(self->sip);
    self->sip_static = ov_mc_backend_sip_static_free(self->sip_static);
    self->recorder = ov_vocs_recorder_free(self->recorder);
    self->vad = ov_mc_backend_vad_free(self->vad);
    self->app = ov_vocs_app_free(self->app);
    self->ldap = ov_ldap_free(self->ldap);
    self->sessions = ov_dict_free(self->sessions);
    self->callbacks = ov_callback_registry_free(self->callbacks);

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static void cb_socket_close(void *userdata, int socket) {

    ov_vocs_core *vocs = ov_vocs_core_cast(userdata);
    if (!vocs || socket < 0) goto error;

    ov_log_debug("Client socket close at %i", socket);
    ov_vocs_app_drop_connection(vocs->app, socket);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool cb_client_process(void *userdata,
                              const int socket,
                              const ov_event_parameter *params,
                              ov_json_value *input) {

    ov_vocs_core *self = ov_vocs_core_cast(userdata);

    return ov_vocs_app_api_process(self->app, socket, params, input);
}

/*----------------------------------------------------------------------------*/
ov_event_io_config ov_vocs_core_event_io_uri_config(ov_vocs_core *self) {

    return (ov_event_io_config){.name = "api",
                                .userdata = self,
                                .callback.close = cb_socket_close,
                                .callback.process = cb_client_process};
}

/*----------------------------------------------------------------------------*/

ov_vocs_core_config ov_vocs_core_config_from_json(const ov_json_value *val){

    ov_vocs_core_config out = {0};

    if (!val) goto error;

    const ov_json_value *config = ov_json_object_get(val, "vocs");
    if (!config) config = val;

    out.module.backend = ov_mc_backend_config_from_json(config);
    out.module.frontend = ov_mc_frontend_config_from_json(config);
    out.module.sip = ov_mc_backend_sip_config_from_json(config);
    out.module.sip_static = ov_mc_backend_sip_static_config_from_json(config);
    out.module.recorder = ov_vocs_recorder_config_from_json(config);
    out.module.vad = ov_mc_backend_vad_config_from_json(config);

    out.limits.response_usec = ov_json_number_get(
        ov_json_get(config, "/" OV_KEY_TIMEOUT "/" OV_KEY_RESPONSE));

    out.limits.reconnect_interval_usec = ov_json_number_get(
        ov_json_get(config, "/" OV_KEY_TIMEOUT "/" OV_KEY_RECONNECT_TIMEOUT));

    if (ov_json_is_true(ov_json_get(val, "/" OV_KEY_LDAP "/" OV_KEY_ENABLED)))
        out.ldap.enable = true;

    out.ldap.config = ov_ldap_config_from_json(val);

    const char *session_path = ov_json_string_get(
        ov_json_get(config, "/" OV_KEY_SESSION "/" OV_KEY_PATH));

    if (session_path) strncpy(out.sessions.path, session_path, PATH_MAX);

    return out;
error:
    return (ov_vocs_core_config){0};
}