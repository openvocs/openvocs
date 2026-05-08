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
        @file           ov_mc_mixer_core.h
        @author         Markus Töpfer
        @author         Michael Beer

        @date           2023-01-21


        ------------------------------------------------------------------------
*/
#ifndef ov_mc_mixer_core_h
#define ov_mc_mixer_core_h

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_vad_config.h>

#include "ov_mc_loop.h"

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_mixer_core ov_mc_mixer_core;

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_mixer_core_forward {

    ov_socket_configuration socket;
    uint32_t ssrc;
    uint32_t payload_type;

} ov_mc_mixer_core_forward;

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_mixer_core_config {

    ov_event_loop *loop;

    ov_vad_config vad;
    bool incoming_vad;
    bool drop_no_va;

    uint64_t samplerate_hz;

    int16_t comfort_noise_max_amplitude;

    bool normalize_input;

    bool rtp_keepalive;

    size_t max_num_frames_to_mix;
    bool normalize_mixing_result_by_square_root;

    struct {

        size_t frame_buffer_max;

    } limit;

    ov_socket_configuration manager;

} ov_mc_mixer_core_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_mixer_core *ov_mc_mixer_core_create(ov_mc_mixer_core_config config);
ov_mc_mixer_core *ov_mc_mixer_core_free(ov_mc_mixer_core *self);
ov_mc_mixer_core *ov_mc_mixer_core_cast(const void *data);

/*----------------------------------------------------------------------------*/

bool ov_mc_mixer_core_reconfigure(ov_mc_mixer_core *self,
                                  ov_mc_mixer_core_config config);

/*----------------------------------------------------------------------------*/

bool ov_mc_mixer_core_set_name(ov_mc_mixer_core *self, const char *name);
const char *ov_mc_mixer_core_get_name(const ov_mc_mixer_core *self);

/*----------------------------------------------------------------------------*/

bool ov_mc_mixer_core_forward_data_is_valid(
    const ov_mc_mixer_core_forward *data);
bool ov_mc_mixer_core_set_forward(ov_mc_mixer_core *self,
                                  ov_mc_mixer_core_forward data);
ov_mc_mixer_core_forward ov_mc_mixer_core_get_forward(ov_mc_mixer_core *self);

/*----------------------------------------------------------------------------*/

/**
 *  Join some multicast loop
 *
 *  @param self     instance pointer
 *  @param loop     name and host of multicast loop to join
 *
 *  @returns true if the multicast group is joined
 */
bool ov_mc_mixer_core_join(ov_mc_mixer_core *self, ov_mc_loop_data loop);

/*----------------------------------------------------------------------------*/

/**
 *  Leave some multicast group by loop name
 *
 *  @param self     instance pointer
 *  @param name     loopname to leave
 */
bool ov_mc_mixer_core_leave(ov_mc_mixer_core *self, const char *name);

/*----------------------------------------------------------------------------*/

/**
 *  Release a mixer instance, returns to zero config
 *
 *  @param self     instance pointer
 */
bool ov_mc_mixer_core_release(ov_mc_mixer_core *self);

bool ov_mc_mixer_core_set_volume(ov_mc_mixer_core *self, const char *name,
                                 uint8_t vol);
uint8_t ov_mc_mixer_core_get_volume(const ov_mc_mixer_core *self,
                                    const char *name);

ov_json_value *ov_mc_mixer_state(ov_mc_mixer_core *self);

#endif /* ov_mc_mixer_core_h */
