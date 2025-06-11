/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_RECORDER_CONFIG_H
#define OV_RECORDER_CONFIG_H
/*----------------------------------------------------------------------------*/

#include <ov_base/ov_json_value.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_vad_config.h>
#include <stdint.h>

/*----------------------------------------------------------------------------*/

#define OV_RECORDER_DEFAULT_REPOSITORY_ROOT_PATH "/tmp"
#define OV_RECORDER_DEFAULT_MAX_PARALLEL_RECORDINGS 20

/**
 * Startup configuration
 */
typedef struct {

    ov_socket_configuration liege_socket;

    uint32_t reconnect_interval_secs;

    size_t num_threads;

    uint64_t lock_timeout_usecs;

    // Currently not read to / written from JSON
    size_t max_parallel_recordings;

    struct {

        char *file_format;
        ov_json_value *codec;
        ov_vad_config vad;

        uint64_t silence_cutoff_interval_secs;

    } defaults;

    ov_json_value *original_config;

    char *repository_root_path;

} ov_recorder_config;

/*----------------------------------------------------------------------------*/

bool ov_recorder_config_clear(ov_recorder_config *cfg);

/*----------------------------------------------------------------------------*/

#endif
