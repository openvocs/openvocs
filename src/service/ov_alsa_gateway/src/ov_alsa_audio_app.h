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
#ifndef OV_ALSA_AUDIO_APP_H
#define OV_ALSA_AUDIO_APP_H
/*----------------------------------------------------------------------------*/

#include <ov_backend/ov_analogue_events.h>
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_json_value.h>
#include <ov_base/ov_result.h>
#include <unistd.h>

/*----------------------------------------------------------------------------*/

#define OV_ALSA_MAX_DEVICES 1

_Static_assert(OV_ALSA_MAX_DEVICES <= OV_MAX_ANALOG_SSRC - 1,
               "Misconfiguration: More ALSA devices than allowed ALSA RTP "
               "SSRCs");

/*----------------------------------------------------------------------------*/

struct ov_alsa_audio_app;
typedef struct ov_alsa_audio_app ov_alsa_audio_app;

typedef struct {

  size_t max_num_streams;
  size_t max_num_frames;

  struct {
    char *input[OV_ALSA_MAX_DEVICES];
    char *output[OV_ALSA_MAX_DEVICES];
  } channels;

  struct {
    double input[OV_ALSA_MAX_DEVICES];
    double output[OV_ALSA_MAX_DEVICES];
  } channel_volumes;

  struct {
    char *input[OV_ALSA_MAX_DEVICES];
    char *output[OV_ALSA_MAX_DEVICES];
  } channel_mixer;

  // Certain channels might be statically connected to multicast loops
  // If any entry here is not null (i.e. contains a multicast ip),
  // the correspondig alsa channel will be statically connected to this
  // multicast ip.
  struct {

    char *input[OV_ALSA_MAX_DEVICES];
    uint16_t input_ports[OV_ALSA_MAX_DEVICES];

    char *output[OV_ALSA_MAX_DEVICES];
    uint16_t output_ports[OV_ALSA_MAX_DEVICES];

  } static_loops;

  uint32_t ssid_of_first_channel;

  ov_socket_configuration rtp_socket;

  struct {
    char const *rtp_logging;
  } debug;

} ov_alsa_audio_app_config;

/*----------------------------------------------------------------------------*/

ov_alsa_audio_app_config
ov_alsa_audio_app_config_from_json(ov_json_value const *jcfg,
                                   ov_alsa_audio_app_config default_config,
                                   bool *ok);

/*----------------------------------------------------------------------------*/

bool ov_alsa_audio_app_config_clear(ov_alsa_audio_app_config *cfg);

/*----------------------------------------------------------------------------*/

ov_alsa_audio_app *ov_alsa_audio_app_create(ov_event_loop *loop,
                                            ov_alsa_audio_app_config cfg);

ov_alsa_audio_app *ov_alsa_audio_app_free(ov_alsa_audio_app *self);

/*----------------------------------------------------------------------------*/

bool ov_alsa_audio_app_start_playbacks(ov_alsa_audio_app *self,
                                       char const **loops,
                                       uint16_t *loop_mc_ports,
                                       size_t num_loops);

/*----------------------------------------------------------------------------*/

bool ov_alsa_audio_app_start_recordings(ov_alsa_audio_app *self,
                                        char const **loops,
                                        uint16_t *loop_mc_ports,
                                        size_t num_loops);

/*----------------------------------------------------------------------------*/

bool ov_alsa_audio_app_stop_stream(ov_alsa_audio_app *self,
                                   ov_analogue_event_type type,
                                   uint32_t channel, ov_result *res);

/*---------------------------------------------------------------------------*/

bool ov_alsa_audio_app_record_to(ov_alsa_audio_app *self,
                                 ov_socket_configuration mcsocket,
                                 uint32_t ssid,
                                 ov_json_value const *codec_config,
                                 uint32_t analogue_channel, ov_result *res);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_alsa_audio_app_list_channels(ov_alsa_audio_app const *self,
                                               ov_analogue_event_type type);

/*----------------------------------------------------------------------------*/

bool ov_alsa_audio_app_start_playback_thread(ov_alsa_audio_app *self);

/*----------------------------------------------------------------------------*/

void ov_alsa_audio_app_enable_caching();

/*----------------------------------------------------------------------------*/
#endif
