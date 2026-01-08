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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_DATABASE_EVENTS_H
#define OV_DATABASE_EVENTS_H
/*----------------------------------------------------------------------------*/

#include "ov_database.h"
#include <ov_base/ov_json_value.h>
#include <ov_base/ov_participation_state.h>
#include <stdbool.h>

/*----------------------------------------------------------------------------*/

bool ov_db_prepare(ov_database *self);

/**
 * Table events_participation_state:
 *
 * DateTime | user ID | Role ID | Loop | state |
 */
bool ov_db_events_add_participation_state(
    ov_database *self, const char *user_id, const char *role_id,
    const char *loop_id, ov_participation_state state, time_t time_epoch);

typedef struct {

    ov_participation_state state;
    char const *user;
    char const *role;
    char const *loop;
    time_t from_epoch_secs;
    time_t until_epoch_secs;

} ov_db_events_get_participation_state_params;

ov_json_value *ov_db_events_get_participation_state_struct(
    ov_database *self, uint32_t max_num_results,
    ov_db_events_get_participation_state_params params);

#define ov_db_events_get_participation_state(self, max_num_results, ...)       \
    ov_db_events_get_participation_state_struct(                               \
        self, max_num_results,                                                 \
        (ov_db_events_get_participation_state_params){__VA_ARGS__})

/*****************************************************************************
                                   Recordings
 ****************************************************************************/

bool ov_db_recordings_add(ov_database *self, const char *id, const char *uri,
                          char const *loop, time_t start_epoch_secs,
                          time_t end_epoch_secs);

typedef struct {

    char const *id;
    char const *loop;
    char const *user;
    ov_participation_state user_state;
    time_t from_epoch_secs;
    time_t until_epoch_secs;

} ov_db_recordings_get_params;

extern ov_json_value const *OV_DB_RECORDINGS_RESULT_TOO_BIG;

ov_json_value *ov_db_recordings_get_struct(ov_database *self,
                                           uint32_t max_num_results,
                                           ov_db_recordings_get_params params);

/**
 * Get recordings satisfying certain conditions:
 *
 * .user   User had joined the loop
 * .state  User had joined the loop in a particular state (send or receive)
 * .loop   Loop name
 * .from_epoch_secs Recording had started at this point in time
 * .until_epoch_secs Recording was still running at that point in time
 */
#define ov_db_recordings_get(self, max_results, ...)                           \
    ov_db_recordings_get_struct(self, max_results,                             \
                                (ov_db_recordings_get_params){__VA_ARGS__})

/*----------------------------------------------------------------------------*/
#endif
