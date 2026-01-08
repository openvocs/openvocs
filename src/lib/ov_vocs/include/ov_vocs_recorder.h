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
        @file           ov_vocs_recorder.h
        @author         Markus Töpfer

        @date           2024-01-09

        Implementation of the recorder functinality


        ------------------------------------------------------------------------
*/
#ifndef ov_vocs_recorder_h
#define ov_vocs_recorder_h

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_result.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_vad_config.h>

#include <ov_core/ov_event_io.h>

#include <ov_database/ov_database.h>
#include <ov_database/ov_database_events.h>
#include <ov_vocs_db/ov_vocs_db.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_recorder ov_vocs_recorder;

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_recorder_config {

    ov_event_loop *loop;
    ov_vocs_db *vocs_db;

    ov_vad_config vad;

    struct {

        uint64_t silence_cutoff_interval_msec;

    } limits;

    struct {

        ov_socket_configuration manager;

    } socket;

    struct {

        uint64_t response_usec;

    } timeout;

    struct {

        void *userdata;

        void (*start_record)(void *userdata, const char *uuid, ov_result error);
        void (*stop_record)(void *userdata, const char *uuid, ov_result error);

    } callbacks;

    ov_database_info db;

} ov_vocs_recorder_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vocs_recorder *ov_vocs_recorder_create(ov_vocs_recorder_config config);
ov_vocs_recorder *ov_vocs_recorder_free(ov_vocs_recorder *self);
ov_vocs_recorder *ov_vocs_recorder_cast(const void *data);

/*----------------------------------------------------------------------------*/

ov_vocs_recorder_config
ov_vocs_recorder_config_from_json(const ov_json_value *value);

/*----------------------------------------------------------------------------*/

ov_event_io_config ov_vocs_recorder_io_uri_config(ov_vocs_recorder *self);

/*----------------------------------------------------------------------------*/

bool ov_vocs_recorder_start_recording(ov_vocs_recorder *self, const char *loop,
                                      const char *uuid);
bool ov_vocs_recorder_stop_recording(ov_vocs_recorder *self, const char *loop,
                                     const char *uuid);

/*----------------------------------------------------------------------------*/

ov_json_value *
ov_vocs_recorder_get_recording(ov_vocs_recorder *self,
                               ov_db_recordings_get_params params);

/*----------------------------------------------------------------------------*/

void ov_vocs_recorder_join_loop(ov_vocs_recorder *self, const char *user,
                                const char *role, const char *loop);

/*----------------------------------------------------------------------------*/

void ov_vocs_recorder_leave_loop(ov_vocs_recorder *self, const char *user,
                                 const char *role, const char *loop);

/*----------------------------------------------------------------------------*/

void ov_vocs_recorder_talk_on_loop(ov_vocs_recorder *self, const char *user,
                                   const char *role, const char *loop);

/*----------------------------------------------------------------------------*/

void ov_vocs_recorder_talk_off_loop(ov_vocs_recorder *self, const char *user,
                                    const char *role, const char *loop);

/*----------------------------------------------------------------------------*/

void ov_vocs_recorder_ptt(ov_vocs_recorder *self, const char *user,
                          const char *role, const char *loop, bool off);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_recorder_get_recorded_loops(ov_vocs_recorder *self);

/*----------------------------------------------------------------------------*/

bool ov_vocs_recorder_start_loop_recording(
    ov_vocs_recorder *self, const char *uuid, const char *loop, void *userdata,
    int socket,
    void (*callback)(void *, int, const char *, const char *, ov_result));

/*----------------------------------------------------------------------------*/

bool ov_vocs_recorder_stop_loop_recording(
    ov_vocs_recorder *self, const char *uuid, const char *loop, void *userdata,
    int socket,
    void (*callback)(void *, int, const char *, const char *, ov_result));

#endif /* ov_vocs_recorder_h */
