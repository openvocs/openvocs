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
        @file           ov_mc_frontend.h
        @author         Markus Töpfer

        @date           2023-01-22


        ------------------------------------------------------------------------
*/
#ifndef ov_mc_frontend_h
#define ov_mc_frontend_h

// #include <ov_backend/ov_backend_event.h>
#include <ov_base/ov_event_loop.h>
#include <ov_core/ov_io.h>
#include <ov_core/ov_mc_loop_data.h>
#include <ov_core/ov_media_definitions.h>
#include <ov_core/ov_response_state.h>
#include <ov_ice/ov_ice_candidate.h>
#include <ov_ice_proxy/ov_ice_proxy_vocs_stream_forward.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_frontend ov_mc_frontend;

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_frontend_config {

    ov_event_loop *loop;
    ov_io *io;

    struct {

        ov_socket_configuration manager; // manager liege socket

    } socket;

    struct {

        void *userdata;

        struct {

            void (*dropped)(void *userdata, const char *session_id);

            void (*created)(void *userdata, const ov_response_state event,
                            const char *session_id, const char *type,
                            const char *sdp, size_t array_size,
                            const ov_ice_proxy_vocs_stream_forward_data *array);

            void (*completed)(void *userdata, const char *session_id,
                              bool success);

            void (*update)(void *userdata, const ov_response_state event,
                           const char *session_id);

            void (*state)(void *userdata, const ov_response_state event,
                          const char *session_id, const ov_json_value *state);

        } session;

        /* callback for info from ice proxy, or response send to ice proxy */
        void (*candidate)(void *userdata, const ov_response_state event,
                          const char *session_id,
                          const ov_ice_candidate_info *info);

        /* callback for info from ice proxy, or response send to ice proxy */
        void (*end_of_candidates)(void *userdata, const ov_response_state event,
                                  const char *session_id,
                                  const ov_ice_candidate_info *info);

        void (*talk)(void *userdata, const ov_response_state event,
                     const char *session_id, const ov_mc_loop_data data,
                     bool on);

    } callback;

} ov_mc_frontend_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_frontend *ov_mc_frontend_create(ov_mc_frontend_config config);
ov_mc_frontend *ov_mc_frontend_free(ov_mc_frontend *self);
ov_mc_frontend *ov_mc_frontend_cast(const void *self);

ov_mc_frontend_config ov_mc_frontend_config_from_json(const ov_json_value *in);

/*
 *      ------------------------------------------------------------------------
 *
 *      CONTROL FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
 *  Create a ICE session at some proxy and send the offer to the client,
 *  using the clients definition.
 *
 * @params self         instance pointer
 * @params uuid         uuid to be set and delivered in response
 * @params sdp          SDP definition to be used
 *
 * NOTE will callback whenever some response arrives.
 */
bool ov_mc_frontend_create_session(ov_mc_frontend *self, char const *uuid,
                                   const char *sdp);

/*----------------------------------------------------------------------------*/

/**
 *  Update a ICE session at some proxy
 *
 * @params self         instance pointer
 * @params uuid         uuid to be set and delivered in response
 * @params session_uuid session id
 * @params type         OFFER or ANSWER
 * @params sdp          SDP definition to be used
 */
bool ov_mc_frontend_update_session(ov_mc_frontend *self, char const *uuid,
                                   const char *session_uuid, ov_media_type type,
                                   const char *sdp);

/*----------------------------------------------------------------------------*/

/**
 *  Create a ICE session at some proxy and send the offer to the client,
 *  using the clients definition.
 *
 * @params self         instance pointer
 * @params uuid         uuid to be set and delivered in response
 * @params session_id   session to drop
 *
 * NOTE will callback whenever some response arrives.
 */
bool ov_mc_frontend_drop_session(ov_mc_frontend *self, const char *uuid,
                                 const char *session_id);

/*----------------------------------------------------------------------------*/

/**
 *  Forward some candidate information of some session to the ICE proxy of
 *  the session.
 *
 * @params self         instance pointer
 * @params uuid         uuid to be set and delivered in response
 * @params session_id   ice_session_id delivered over create_session callback
 * @params info         some candidate info to be send
 *
 * NOTE will callback whenever some response arrives.
 */
bool ov_mc_frontend_candidate(ov_mc_frontend *self, char const *uuid,
                              const char *ice_session_id,
                              const ov_ice_candidate_info *info);

/*----------------------------------------------------------------------------*/

/**
 *  Forward some end_of_candidate information of some session to the ICE proxy
 *  of the session.
 *
 * @params self         instance pointer
 * @params uuid         uuid to be set and delivered in response
 * @params session_id   ice_session_id delivered over create_session callback
 *
 * NOTE will callback whenever some response arrives.
 */
bool ov_mc_frontend_end_of_candidates(ov_mc_frontend *self, char const *uuid,
                                      const char *session_id);

/*----------------------------------------------------------------------------*/

/**
 *  Switch talk on for some session.
 *
 * @params self         instance pointer
 * @params uuid         uuid to be set and delivered in response
 * @params session_id   ice_session_id delivered over create_session callback
 * @params on           switch talk on or off for loop data
 * @params data         loop data to switch
 *
 * NOTE will callback whenever some response arrives.
 */
bool ov_mc_frontend_talk(ov_mc_frontend *self, char const *uuid,
                         const char *session_id, bool on, ov_mc_loop_data data);

/*----------------------------------------------------------------------------*/

bool ov_mc_frontened_get_session_state(ov_mc_frontend *self, const char *uuid,
                                       const char *session_id);

#endif /* ov_mc_frontend_h */
