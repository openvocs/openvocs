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
        @file           ov_vocs_events.h
        @author         Markus Töpfer

        @date           2024-01-22


        ------------------------------------------------------------------------
*/
#ifndef ov_vocs_events_h
#define ov_vocs_events_h

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_participation_state.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_events ov_vocs_events;

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_event_config {

  ov_event_loop *loop;

  struct {

    ov_socket_configuration manager; // manager liege socket

  } socket;

} ov_vocs_events_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vocs_events *ov_vocs_events_create(ov_vocs_events_config config);
ov_vocs_events *ov_vocs_events_free(ov_vocs_events *self);
ov_vocs_events *ov_vocs_events_cast(const void *data);

ov_vocs_events_config ov_vocs_events_config_from_json(const ov_json_value *val);

/*
 *      ------------------------------------------------------------------------
 *
 *      EMIT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

void ov_vocs_events_emit_media_ready(ov_vocs_events *self,
                                     const char *session_id);

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_mixer_aquired(ov_vocs_events *self,
                                       const char *session_id);

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_mixer_lost(ov_vocs_events *self,
                                    const char *session_id);

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_mixer_released(ov_vocs_events *self,
                                        const char *session_id);

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_client_login(ov_vocs_events *self,
                                      const char *client_id,
                                      const char *user_id);

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_client_authorize(ov_vocs_events *self,
                                          const char *session_id,
                                          const char *user_id,
                                          const char *role_id);

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_client_logout(ov_vocs_events *self,
                                       const char *session_id);

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_user_session_created(ov_vocs_events *self,
                                              const char *session_id,
                                              const char *client_id,
                                              const char *user_id);

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_user_session_closed(ov_vocs_events *self,
                                             const char *session_id,
                                             const char *client_id,
                                             const char *user_id);

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_participation_state(
    ov_vocs_events *self, const char *session_id, const char *client_id,
    const char *user_id, const char *role_id, const char *loop_id,
    ov_socket_configuration multicast, ov_participation_state state);

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_loop_volume(ov_vocs_events *self,
                                     const char *session_id,
                                     const char *client_id, const char *user_id,
                                     const char *role_id, const char *loop_id,
                                     ov_socket_configuration multicast,
                                     uint8_t volume);

/*----------------------------------------------------------------------------*/

void ov_vocs_events_emit_ptt(ov_vocs_events *self, const char *session_id,
                             const char *client_id, const char *user_id,
                             const char *role_id, const char *loop_id,
                             ov_socket_configuration multicast, bool off);

/*----------------------------------------------------------------------------*/

#endif /* ov_vocs_events_h */
