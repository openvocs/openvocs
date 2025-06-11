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
        @file           ov_event_session.h
        @author         Markus Töpfer

        @date           2023-12-09

        ov_event_session provides a session password for some user login.
        The session has a default runtime of 1 hour and can be updated within
        that timespan to be valid for another hour.

        ------------------------------------------------------------------------
*/
#ifndef ov_event_session_h
#define ov_event_session_h

#include <ov_base/ov_event_loop.h>

#define OV_EVENT_SESSIONS_PATH "/tmp/vocs/sessions"
#define OV_EVENT_SESSIONS_FILE "sessions.json"

/*----------------------------------------------------------------------------*/

typedef struct ov_event_session ov_event_session;

/*----------------------------------------------------------------------------*/

typedef struct ov_event_session_config {

    ov_event_loop *loop;

    char path[PATH_MAX];

    struct {

        uint64_t max_lifetime_usec;

    } limit;

} ov_event_session_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_event_session *ov_event_session_create(ov_event_session_config config);
ov_event_session *ov_event_session_free(ov_event_session *self);
ov_event_session *ov_event_session_cast(const void *data);

/*----------------------------------------------------------------------------*/

const char *ov_event_session_init(ov_event_session *self,
                                  const char *client,
                                  const char *user);

/*----------------------------------------------------------------------------*/

bool ov_event_session_delete(ov_event_session *self, const char *client);

/*----------------------------------------------------------------------------*/

bool ov_event_session_update(ov_event_session *self,
                             const char *client,
                             const char *user,
                             const char *id);

/*----------------------------------------------------------------------------*/

const char *ov_event_session_get_user(ov_event_session *self,
                                      const char *client);

/*----------------------------------------------------------------------------*/

bool ov_event_session_verify(ov_event_session *self,
                             const char *client,
                             const char *user,
                             const char *id);

/*----------------------------------------------------------------------------*/

bool ov_event_session_save(ov_event_session *self);

/*----------------------------------------------------------------------------*/

bool ov_event_session_load(ov_event_session *self);

#endif /* ov_event_session_h */
