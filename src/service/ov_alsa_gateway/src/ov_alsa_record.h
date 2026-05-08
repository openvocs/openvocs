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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_ALSA_RECORD_H
#define OV_ALSA_RECORD_H
/*----------------------------------------------------------------------------*/
#include <ov_base/ov_result.h>
#include <ov_base/ov_socket.h>

/*----------------------------------------------------------------------------*/

#define OV_ALSA_RECORD_DEFAULT_VOLUME 0.5
// Specific for OUR USB card
#define OV_ALSA_RECORD_DEFAULT_MIXER_ELEMENT "Mic"

/*----------------------------------------------------------------------------*/

struct ov_alsa_record_struct;
typedef struct ov_alsa_record_struct ov_alsa_record;

/*----------------------------------------------------------------------------*/

typedef struct {

    struct {
        ov_json_value const *codec_config;
        uint64_t frame_length_ms;
        uint16_t ssid;
        ov_socket_configuration target;
    } rtp_stream;

    uint64_t samplerate_hz;

    double volume;

    char const *alsa_device;
    char const *mixer_element;

    struct {
        uint16_t max_amplitude;
    } comfort_noise;

} ov_alsa_record_config;

/*----------------------------------------------------------------------------*/

ov_alsa_record *ov_alsa_record_create(const ov_alsa_record_config cfg,
                                      int sd_for_sending, ov_result *res);

/*----------------------------------------------------------------------------*/

ov_alsa_record *ov_alsa_record_free(ov_alsa_record *self);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_alsa_record_get_counters(ov_alsa_record *self);

/*----------------------------------------------------------------------------*/
#endif
