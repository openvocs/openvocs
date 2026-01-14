/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_mixer_msg.h
        @author         Markus Töpfer

        @date           2022-11-11

        Message generation for ov_mixer

        ------------------------------------------------------------------------
*/
#ifndef ov_mixer_msg_h
#define ov_mixer_msg_h

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_vad_config.h>

#include <ov_core/ov_event_api.h>
#include <ov_core/ov_mc_loop_data.h>

typedef struct ov_mixer_forward {

    ov_socket_configuration socket;
    uint32_t ssrc;
    uint32_t payload_type;

} ov_mixer_forward;

/*----------------------------------------------------------------------------*/

typedef struct ov_mixer_config {

    ov_vad_config vad;
    bool incoming_vad;

    uint64_t samplerate_hz;

    int16_t comfort_noise_max_amplitude;

    bool normalize_input;

    bool rtp_keepalive;

    size_t max_num_frames_to_mix;
    bool normalize_mixing_result_by_square_root;

    struct {

        size_t frame_buffer_max;

    } limit;

} ov_mixer_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_json_value *ov_mixer_msg_register(const char *uuid, const char *type);

ov_json_value *ov_mixer_msg_configure(ov_mixer_config config);
ov_mixer_config ov_mixer_msg_configure_from_json(const ov_json_value *json);
ov_mixer_config ov_mixer_config_from_json(const ov_json_value *json);

/*----------------------------------------------------------------------------*/

/**
 * {
    "event":"acquire",
    "parameter":
    {
        "name" : "username",
        "socket":
        {
            "host":"127.0.0.1",
            "port":1234,
            "type":"UDP"
        },
        "ssrc":1
    },
    "uuid":"13916c23-1029-4b4a-aad1-ce31db07e437"
    }
*/
ov_json_value *ov_mixer_msg_acquire(const char *username,
                                       ov_mixer_forward data);

/*----------------------------------------------------------------------------*/

/**
 * {
    "event":"forward",
    "parameter":
    {
        "name" : "username",
        "socket":
        {
            "host":"127.0.0.1",
            "port":1234,
            "type":"UDP"
        },
        "ssrc":1,
        "payload_type" : 1234
    },
    "uuid":"13916c23-1029-4b4a-aad1-ce31db07e437"
    }
*/
ov_json_value *ov_mixer_msg_forward(const char *username,
                                       ov_mixer_forward data);

/*----------------------------------------------------------------------------*/

const char *ov_mixer_msg_aquire_get_username(const ov_json_value *m);
ov_mixer_forward ov_mixer_msg_acquire_get_forward(const ov_json_value *m);

/*----------------------------------------------------------------------------*/

/**
 * {
    "event":"release",
    "uuid":"3bc4a60c-6adf-486c-ab62-61ae041d054e",
    "parameter":
    {
        "name" : "username"
    }
*/
ov_json_value *ov_mixer_msg_release(const char *name);

/*----------------------------------------------------------------------------*/

/**
 * {
    "event":"join",
    "parameter":
    {
        "host":"224.0.0.0",
        "loop":"loop1"
    },
    "uuid":"3bc4a60c-6adf-486c-ab62-61ae041d054e"
    }
*/
ov_json_value *ov_mixer_msg_join(ov_mc_loop_data data);
ov_mc_loop_data ov_mixer_msg_join_from_json(const ov_json_value *msg);

/*----------------------------------------------------------------------------*/

/**
 * {
    "event":"leave",
    "parameter":
    {
        "loop":"loop1"
    },
    "uuid":"e74aecdc-22c5-4083-b387-2e039baa4321"
    }
*/
ov_json_value *ov_mixer_msg_leave(const char *loopname);
const char *ov_mixer_msg_leave_from_json(const ov_json_value *msg);

/*----------------------------------------------------------------------------*/

/**
 * {
    "event":"volume",
    "parameter":
    {
        "loop":"loop1",
        "volume" : 100
    },
    "uuid":"e74aecdc-22c5-4083-b387-2e039baa4321"
    }
*/
ov_json_value *ov_mixer_msg_volume(const char *loopname, uint8_t vol);
const char *ov_mixer_msg_volume_get_name(const ov_json_value *msg);
uint8_t ov_mixer_msg_volume_get_volume(const ov_json_value *msg);

/*----------------------------------------------------------------------------*/

/**
 * {
    "event":"shutdown",
    "uuid":"3bc4a60c-6adf-486c-ab62-61ae041d054e"
    }
*/
ov_json_value *ov_mixer_msg_shutdown();

/*----------------------------------------------------------------------------*/

/**
 * {
    "event":"state",
    "uuid":"3bc4a60c-6adf-486c-ab62-61ae041d054e"
    }
*/
ov_json_value *ov_mixer_msg_state();

/*----------------------------------------------------------------------------*/

#endif /* ov_mixer_msg_h */
