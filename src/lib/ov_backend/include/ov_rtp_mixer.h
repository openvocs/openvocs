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

        The mixer is thread safe.

        ------------------------------------------------------------------------
*/
#ifndef OV_RTP_MIXER_H
#define OV_RTP_MIXER_H
/*----------------------------------------------------------------------------*/

#include <inttypes.h>
#include <ov_base/ov_chunker.h>
#include <ov_base/ov_json_value.h>
#include <ov_base/ov_rtp_frame.h>

/*----------------------------------------------------------------------------*/

struct ov_rtp_mixer_struct;
typedef struct ov_rtp_mixer_struct ov_rtp_mixer;

/*----------------------------------------------------------------------------*/

typedef struct {
    ov_json_value const *codec_config;
    size_t max_num_frames_per_stream;
    uint32_t ssid_to_cancel;
    uint16_t comfort_noise_max_amplitude;
    size_t frame_length_ms;
    double sample_rate_hertz;

} ov_rtp_mixer_config;

ov_rtp_mixer *ov_rtp_mixer_create(ov_rtp_mixer_config cfg);

ov_rtp_mixer *ov_rtp_mixer_free(ov_rtp_mixer *self);

/*----------------------------------------------------------------------------*/

bool ov_rtp_mixer_add_frame(ov_rtp_mixer *self, ov_rtp_frame *frame);

/**
 * @return true if there were frames to mix. False, if there weren't any or an
 * error occured. If comfort noise is generated, will return false as well as
 * there were no frames to mix!
 */
bool ov_rtp_mixer_mix(ov_rtp_mixer *self, ov_chunker *chunker_to_write_to);

/*----------------------------------------------------------------------------*/

bool ov_rtp_mixer_garbage_collect(ov_rtp_mixer *self,
                                  uint32_t max_stream_lifetime_secs);

/*----------------------------------------------------------------------------*/

void ov_rtp_mixer_enable_caching();

/*----------------------------------------------------------------------------*/
#endif
