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
        @file           ov_vocs_record.h
        @author         Markus Töpfer

        @date           2024-01-26


        ------------------------------------------------------------------------
*/
#ifndef ov_vocs_record_h
#define ov_vocs_record_h

#include <ov_base/ov_id.h>
#include <ov_base/ov_json.h>
#include <ov_core/ov_mc_loop_data.h>

typedef struct ov_vocs_record ov_vocs_record;

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_record_config {

  char loopname[OV_MC_LOOP_NAME_MAX];

  struct {

    void *userdata;

    void (*stop)(void *userdata, ov_vocs_record *const self);

  } callback;

} ov_vocs_record_config;

/*----------------------------------------------------------------------------*/

struct ov_vocs_record {

  ov_vocs_record_config config;

  struct {

    bool running;
    ov_id id;          // id of current recording
    intptr_t recorder; // socket id of recorder
    time_t started_at_epoch_secs;
    char *uri;

  } active;

  ov_json_value *data;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vocs_record *ov_vocs_record_create(ov_vocs_record_config config);
void *ov_vocs_record_free_void(void *self);

bool ov_vocs_record_set_active(ov_vocs_record *self, char const *id,
                               char const *loop, char const *uri,
                               int recorder_fh);

bool ov_vocs_record_reset_active(ov_vocs_record *self);

bool ov_vocs_record_add_ptt(ov_vocs_record *self, const char *session);
bool ov_vocs_record_drop_ptt(ov_vocs_record *self, const char *session);

int64_t ov_vocs_record_get_ptt_count(const ov_vocs_record *self);

#endif /* ov_vocs_record_h */
