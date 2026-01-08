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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/
#include <inttypes.h>
#include <ov_base/ov_chunker.h>
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_json_value.h>
#include <ov_base/ov_result.h>
#include <ov_base/ov_rtp_stream_buffer.h>
#include <ov_base/ov_socket.h>

/*----------------------------------------------------------------------------*/

#define OV_ALSA_PLAYBACK_DEFAULT_VOLUME 0.5
#define OV_ALSA_PLAYBACK_DEFAULT_MIXER_ELEMENT "Speaker"

struct ov_alsa_playback;
typedef struct ov_alsa_playback ov_alsa_playback;

typedef struct {

    char const *alsa_device;
    char const *mixer_element;

    uint16_t msecs_to_buffer_ahead;

    double volume;

    struct {
        uint16_t max_amplitude;
    } comfort_noise;

    int logging_fd;

} ov_alsa_playback_config;

/*----------------------------------------------------------------------------*/

ov_alsa_playback *ov_alsa_playback_create(const ov_alsa_playback_config cfg,
                                          ov_result *res);

/*----------------------------------------------------------------------------*/

ov_alsa_playback *ov_alsa_playback_free(ov_alsa_playback *self);

/*----------------------------------------------------------------------------*/

typedef enum {

    ALSA_REPLAY_FAILED,
    ALSA_REPLAY_OK,
    ALSA_REPLAY_INSUFFICIENT
} ov_alsa_playback_play_result;

ov_alsa_playback_play_result ov_alsa_playback_play(ov_alsa_playback *self,
                                                   ov_chunker *pcm);

/*----------------------------------------------------------------------------*/

bool ov_alsa_playback_play_comfort_noise(ov_alsa_playback *self);

/*----------------------------------------------------------------------------*/

bool ov_alsa_playback_reset(ov_alsa_playback *self);

/*----------------------------------------------------------------------------*/
