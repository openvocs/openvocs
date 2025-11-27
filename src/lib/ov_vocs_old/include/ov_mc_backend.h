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
        @file           ov_mc_backend.h
        @author         Markus Töpfer

        @date           2023-01-21


        ------------------------------------------------------------------------
*/
#ifndef ov_mc_backend_h
#define ov_mc_backend_h

#include <ov_base/ov_event_loop.h>
#include <ov_core/ov_callback.h>
#include <ov_core/ov_io.h>

#include "ov_mc_backend_registry.h"
#include "ov_mc_mixer_core.h"
#include <ov_base/ov_event_loop.h>

#define OV_MC_BACKEND_DEFAULT_TIMEOUT 10000000 // 10 seconds

typedef void (*ov_mc_backend_cb_mixer)(void *userdata,
                                       const char *uuid,
                                       const char *user_uuid,
                                       uint64_t error_code,
                                       const char *error_desc);

typedef void (*ov_mc_backend_cb_loop)(void *userdata,
                                      const char *uuid,
                                      const char *session_id,
                                      const char *loopname,
                                      uint64_t error_code,
                                      const char *error_desc);

typedef void (*ov_mc_backend_cb_volume)(void *userdata,
                                        const char *uuid,
                                        const char *session_id,
                                        const char *loopname,
                                        uint8_t volume,
                                        uint64_t error_code,
                                        const char *error_desc);

typedef void (*ov_mc_backend_cb_state)(void *userdata,
                                       const char *uuid,
                                       const ov_json_value *state);

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_backend ov_mc_backend;

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_backend_config {

    ov_event_loop *loop;
    ov_io *io;

    struct {

        ov_socket_configuration manager; // manager liege socket

    } socket;

    struct {

        ov_mc_mixer_core_config config;

    } mixer;

    struct {

        uint64_t request_usec;

    } timeout;

    struct {

        void *userdata;

        struct {

            void (*lost)(void *userdata, const char *uuid);

        } mixer;

    } callback;

} ov_mc_backend_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_backend *ov_mc_backend_create(ov_mc_backend_config config);
ov_mc_backend *ov_mc_backend_free(ov_mc_backend *self);
ov_mc_backend *ov_mc_backend_cast(const void *self);

/*----------------------------------------------------------------------------*/

ov_mc_backend_config ov_mc_backend_config_from_json(const ov_json_value *val);

/*----------------------------------------------------------------------------*/

/**
 *  Acquire some mixer
 *
 *  @params self        instance pointer
 *  @params uuid        request uuid
 *  @params user_uuid   uuid for user
 *  @params data        forward data to be set
 *  @params userdata    userdata to be used in callback
 *  @params callback    callback to be called once answer arrives
 */
bool ov_mc_backend_acquire_mixer(ov_mc_backend *self,
                                 const char *uuid,
                                 const char *user_uuid,
                                 ov_mc_mixer_core_forward data,
                                 void *userdata,
                                 ov_mc_backend_cb_mixer callback);

/*----------------------------------------------------------------------------*/

/**
 *  Reset mixer forward for some mixer
 *
 *  @params self        instance pointer
 *  @params uuid        request uuid
 *  @params user_uuid   uuid for user
 *  @params data        forward data to be set
 *  @params userdata    userdata to be used in callback
 *  @params callback    callback to be called once answer arrives
 */
bool ov_mc_backend_set_mixer_forward(ov_mc_backend *self,
                                     const char *uuid,
                                     const char *user_uuid,
                                     ov_mc_mixer_core_forward data,
                                     void *userdata,
                                     ov_mc_backend_cb_mixer callback);

/*----------------------------------------------------------------------------*/

/**
 *  Release some mixer
 *
 *  @params self        instance pointer
 *  @params uuid        request uuid
 *  @params user_uuid   uuid of user
 *  @params userdata    userdata to be used in callback
 *  @params callback    callback to be called once answer arrives
 */
bool ov_mc_backend_release_mixer(ov_mc_backend *self,
                                 const char *uuid,
                                 const char *user_uuid,
                                 void *userdata,
                                 ov_mc_backend_cb_mixer callback);

/*----------------------------------------------------------------------------*/

/**
 *  Join some loop for listening
 *
 *  @params self        instance pointer
 *  @params uuid        request uuid
 *  @params user_uud    uuid of user
 *  @params data        loop data
 *  @params userdata    userdata to be used in callback
 *  @params callback    callback to be called once answer arrives
 */
bool ov_mc_backend_join_loop(ov_mc_backend *self,
                             const char *uuid,
                             const char *user_uuid,
                             ov_mc_loop_data data,
                             void *userdata,
                             ov_mc_backend_cb_loop callback);

/*----------------------------------------------------------------------------*/

/**
 *  Leave some loop for listening
 *
 *  @params self        instance pointer
 *  @params uuid        request uuid
 *  @params user_uud    uuid of user
 *  @params loopname    loop data
 *  @params userdata    userdata to be used in callback
 *  @params callback    callback to be called once answer arrives
 */
bool ov_mc_backend_leave_loop(ov_mc_backend *self,
                              const char *uuid,
                              const char *user_uuid,
                              const char *loopname,
                              void *userdata,
                              ov_mc_backend_cb_loop callback);

/*----------------------------------------------------------------------------*/

/**
 *  Set listening volume
 *
 *  @params self        instance pointer
 *  @params uuid        request uuid
 *  @params user_uud    uuid of user
 *  @params loopname    loop data
 *  @params volume      volume percent
 *  @params userdata    userdata to be used in callback
 *  @params callback    callback to be called once answer arrives
 */
bool ov_mc_backend_set_loop_volume(ov_mc_backend *self,
                                   const char *uuid,
                                   const char *user_uuid,
                                   const char *loopname,
                                   uint8_t volume,
                                   void *userdata,
                                   ov_mc_backend_cb_volume callback);

/*----------------------------------------------------------------------------*/

ov_mc_backend_registry_count ov_mc_backend_state_mixers(ov_mc_backend *self);

/*----------------------------------------------------------------------------*/

/**
 *  Get session state of session_id
 *
 *  @params self        instance pointer
 *  @params uuid        request uuid
 *  @params session_id  session to request
 *  @params userdata    userdata to be used in callback
 *  @params callback    callback to be called once answer arrives
 */
bool ov_mc_backend_get_session_state(ov_mc_backend *self,
                                     const char *uuid,
                                     const char *session_id,
                                     void *userdata,
                                     ov_mc_backend_cb_state callback);

/*----------------------------------------------------------------------------*/



#endif /* ov_mc_backend_h */
