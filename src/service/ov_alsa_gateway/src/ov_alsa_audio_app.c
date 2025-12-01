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

#include "ov_alsa_audio_app.h"
#include "ov_alsa_playback.h"
#include "ov_alsa_record.h"
#include "ov_alsa_rtp_mixer.h"
#include "ov_base/ov_error_codes.h"
#include <fcntl.h>
#include <ov_base/ov_config.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_constants.h>
#include <ov_base/ov_mc_socket.h>
#include <ov_base/ov_random.h>
#include <ov_base/ov_rtp_app.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_time.h>
#include <wchar.h>

/*----------------------------------------------------------------------------*/

static void set_bool(bool *b, bool value) {
  if (0 != b) {
    *b = value;
  }
}

/*----------------------------------------------------------------------------*/

static bool set_string(char const **target, char const *src) {

  if (0 == target) {

    return true;

  } else {

    *target = src;
    return 0 != *target;
  }
}

/*----------------------------------------------------------------------------*/

static bool set_double_from_json(double *target, ov_json_value const *jval) {

  if (0 == target) {

    return true;

  } else if (ov_json_is_number(jval)) {

    *target = ov_json_number_get(jval);
    return true;

  } else {

    return false;
  }
}

/*----------------------------------------------------------------------------*/

static bool set_uint16_from_json(uint16_t *target, ov_json_value const *jval) {

  double number = ov_json_number_get(jval);

  if (0 == target) {

    return true;

  } else if (ov_json_is_number(jval) &&
             ov_cond_valid((number >= 0) && (number < UINT16_MAX),
                           "Error in configuration: UINT16 out of range")) {

    *target = (uint16_t)number;
    return true;

  } else {

    return false;
  }
}

/*----------------------------------------------------------------------------*/

static bool device_info_from_extended_format(ov_json_value const *jval,
                                             char const **device_str,
                                             char const **multicast_ip_str,
                                             uint16_t *multicast_port,
                                             char const **mixer,
                                             double *volume) {

  if (0 == jval) {
    return false;
  } else if (ov_json_is_object(jval)) {

    ov_json_value const *jdevice =
        ov_json_get((ov_json_value *)jval, "/" OV_KEY_DEVICE);

    ov_json_value const *jmc =
        ov_json_get((ov_json_value *)jval, "/" OV_KEY_LOOP);

    ov_json_value const *jmixer =
        ov_json_get((ov_json_value *)jval, "/" OV_KEY_MIXER);

    ov_json_value const *jvolume =
        ov_json_get((ov_json_value *)jval, "/" OV_KEY_VOLUME);

    ov_json_value const *jport =
        ov_json_get((ov_json_value *)jval, "/" OV_KEY_PORT);

    return set_string(device_str, ov_json_string_get(jdevice)) &&
           ((0 == jmc) ||
            set_string(multicast_ip_str, ov_json_string_get(jmc))) &&
           ((0 == jmixer) || set_string(mixer, ov_json_string_get(jmixer))) &&
           ((0 == jvolume) || set_double_from_json(volume, jvolume)) &&
           ((jport == 0) || set_uint16_from_json(multicast_port, jport));

  } else {

    return set_string(device_str, ov_json_string_get(jval));
  }
}

/*----------------------------------------------------------------------------*/

// Assumed device list format:
//
// Either
//
// [ "plughw:1,0", "plughw:0,0", ...]
//
// or
//
// [
//   {
//   "device" : "plughw:1,0",
//   "loop" : "224.0.0.2",
//   "port" : 12345,
//   "volume" : 1.2
//   },
//   ...
// ]
//
// "loop", "port" and "volume" are optional.

static size_t get_devices_list(char **devices_target,
                               char **multicast_ip_target,
                               uint16_t *multicast_port_target,
                               char **mixer_target, double *volumes,
                               size_t capacity, ov_json_value const *jdevices) {

  size_t num_devices = ov_json_array_count(jdevices);

  if (capacity < num_devices) {
    ov_log_warning(
        "Number of configured devices %zu exeeds limits, only using the "
        "first %zu numbers",
        num_devices, capacity);
  }

  if (ov_ptr_valid(devices_target, "Cannot read devices list - 0 pointer") &&
      ov_ptr_valid(multicast_ip_target,
                   "Cannot read devices list - 0 pointer")) {

    bool result = true;

    for (size_t i = 0; (i < num_devices) && (i < capacity); ++i) {

      char const *devstring = 0;
      char const *multicast_ip = 0;
      char const *mixer = 0;
      uint16_t port = 0;
      double volume = 0;

      device_info_from_extended_format(
          ov_json_array_get((ov_json_value *)jdevices, 1 + i), &devstring,
          &multicast_ip, &port, &mixer, &volume);

      if (ov_ptr_valid(devstring, "Invalid device entry in ALSA devices")) {
        devices_target[i] = ov_string_dup(devstring);

        if (0 != multicast_ip_target) {
          multicast_ip_target[i] = ov_string_dup(multicast_ip);
        }

        if (0 != mixer_target) {
          mixer_target[i] = ov_string_dup(mixer);
        }

        if (0 != volumes) {
          volumes[i] = volume;
        }

        multicast_port_target[i] = port;

      } else {
        result = false;
        break;
      }
    }

    return result;

  } else {

    return false;
  }
}

/*----------------------------------------------------------------------------*/

static bool clear_device_list(char **strarray, size_t capacity) {

  if (0 != strarray) {
    for (size_t i = 0; i < capacity; ++i) {
      strarray[i] = ov_free(strarray[i]);
    }
    return true;
  } else {
    return false;
  }
}

/*----------------------------------------------------------------------------*/

bool ov_alsa_audio_app_config_clear(ov_alsa_audio_app_config *cfg) {

  if (0 != cfg) {

    clear_device_list(cfg->channels.output, OV_ALSA_MAX_DEVICES);
    clear_device_list(cfg->static_loops.output, OV_ALSA_MAX_DEVICES);
    clear_device_list(cfg->channels.input, OV_ALSA_MAX_DEVICES);
    clear_device_list(cfg->static_loops.input, OV_ALSA_MAX_DEVICES);
    clear_device_list(cfg->channel_mixer.input, OV_ALSA_MAX_DEVICES);
    clear_device_list(cfg->channel_mixer.output, OV_ALSA_MAX_DEVICES);
    memset(cfg, 0, sizeof(*cfg));
  }
  return true;
}

/*----------------------------------------------------------------------------*/

ov_alsa_audio_app_config
ov_alsa_audio_app_config_from_json(ov_json_value const *jcfg,
                                   ov_alsa_audio_app_config default_config,
                                   bool *ok) {

  ov_json_value const *channels = ov_json_get(jcfg, "/" OV_KEY_CHANNELS);

  if (0 != channels) {
    ov_alsa_audio_app_config cfg = {0};

    get_devices_list(cfg.channels.output, cfg.static_loops.output,
                     cfg.static_loops.output_ports, cfg.channel_mixer.output,
                     cfg.channel_volumes.output, OV_ALSA_MAX_DEVICES,
                     ov_json_get(channels, "/" OV_KEY_OUTPUT));

    get_devices_list(cfg.channels.input, cfg.static_loops.input,
                     cfg.static_loops.input_ports, cfg.channel_mixer.input,
                     cfg.channel_volumes.input, OV_ALSA_MAX_DEVICES,
                     ov_json_get(channels, "/" OV_KEY_INPUT));

    cfg.max_num_frames = ov_config_u32_or_default(
        jcfg, "/" OV_KEY_MAX_FRAMES_TO_BUFFER_PER_STREAMS, 1);

    cfg.ssid_of_first_channel = ov_config_u32_or_default(
        jcfg, "/ssid_of_first_channel", cfg.static_loops.input_ports[0]);

    cfg.max_num_streams = ov_config_u32_or_default(jcfg, "/" OV_KEY_STREAMS, 1);

    cfg.rtp_socket =
        (ov_socket_configuration){.host = "0.0.0.0", .port = 0, .type = UDP};

    char const *listen_interface =
        OV_OR_DEFAULT(ov_json_string_get(ov_json_get(jcfg, "/"
                                                           "rtp_interface")),
                      "0.0.0.0");

    strcpy(cfg.rtp_socket.host, listen_interface);

    ov_json_value const *debug = ov_json_get(jcfg, "/" OV_KEY_DEBUG);

    cfg.debug.rtp_logging =
        ov_json_string_get(ov_json_get(debug, "/rtp_logging"));

    set_bool(ok, true);
    return cfg;

  } else {
    set_bool(ok, true);
    return default_config;
  }
}

/*----------------------------------------------------------------------------*/

typedef struct {

  uint32_t magic_bytes;

  ov_rtp_app *rtp;
  ov_alsa_playback *playback;
  ov_chunker *buffer;
  ov_alsa_rtp_mixer *mixer;

  char *device;

  char *mixer_element;
  double volume;

  uint32_t ssid_to_cancel;

  size_t times_run_since_last_gc;

  int rtp_logging_fd;

} OutChannel;

/*----------------------------------------------------------------------------*/

#define OUT_MAGIC_BYTES 0x123451

static OutChannel *as_out_channel_mut(void *ptr) {

  OutChannel *out = ptr;

  if ((0 != ptr) && (OUT_MAGIC_BYTES == out->magic_bytes)) {
    return out;
  } else {
    return 0;
  }
}

/*----------------------------------------------------------------------------*/

typedef struct {

  ov_alsa_record *record;
  int sd;
  char *device;

  char *mixer_element;
  double volume;

} InChannel;

/*----------------------------------------------------------------------------*/

#define MAGIC_BYTES 0xA13AA7D1

struct ov_alsa_audio_app {

  uint32_t magic_bytes;

  ov_event_loop *loop;
  ov_rtp_app *rtp_app_send;

  size_t num_channels;
  OutChannel out_channel[OV_ALSA_MAX_DEVICES];
  InChannel in_channel[OV_ALSA_MAX_DEVICES];

  uint32_t ssid_of_first_channel;

  struct {
    size_t frame_length_ms;
    size_t max_frames_to_buffer;
    ov_json_value *default_codec;
  } settings;

  uint32_t mix_and_replay_timer;
};

/*----------------------------------------------------------------------------*/

static ov_alsa_audio_app const *as_alsa_audio_app(void const *ptr) {

  ov_alsa_audio_app const *alsa_audio = ptr;

  if ((0 != alsa_audio) && (MAGIC_BYTES == alsa_audio->magic_bytes)) {
    return alsa_audio;
  } else {
    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static ov_alsa_audio_app *as_alsa_audio_app_mut(void *ptr) {
  return (ov_alsa_audio_app *)as_alsa_audio_app(ptr);
}

/*----------------------------------------------------------------------------*/

static ov_rtp_app const *get_rtp_app_send(ov_alsa_audio_app const *self) {

  if (ov_ptr_valid(as_alsa_audio_app(self),
                   "Cannot get RTP app from ALSA app - 0 pointer")) {
    return self->rtp_app_send;
  } else {
    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static OutChannel const *get_out_channel_ptr(ov_alsa_audio_app const *self,
                                             size_t i) {

  if (ov_ptr_valid(as_alsa_audio_app(self),
                   "Cannot get ALSA channel - 0 pointer") &&
      ov_cond_valid(i < self->num_channels,
                    "Cannot get ALSA channel - channel index out of range")) {

    return self->out_channel + i;

  } else {

    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static OutChannel *get_out_channel_ptr_mut(ov_alsa_audio_app *self, size_t i) {
  return (OutChannel *)get_out_channel_ptr(self, i);
}

/*----------------------------------------------------------------------------*/

static InChannel const *get_in_channel_ptr(ov_alsa_audio_app const *self,
                                           size_t i) {

  if (ov_ptr_valid(as_alsa_audio_app(self),
                   "Cannot get ALSA channel - 0 pointer") &&
      ov_cond_valid(i < self->num_channels,
                    "Cannot get ALSA channel - channel index out of range")) {

    return self->in_channel + i;

  } else {

    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static InChannel *get_in_channel_mut(ov_alsa_audio_app *self, size_t i) {
  return (InChannel *)get_in_channel_ptr(self, i);
}

/*----------------------------------------------------------------------------*/

static char const *in_channel_device(InChannel const *channel) {
  if (ov_ptr_valid(channel, "Cannot get alsa device: Invalid channel")) {
    return channel->device;
  } else {
    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static char const *in_channel_mixer_element(InChannel const *channel) {
  if (ov_ptr_valid(channel, "Cannot get alsa device: Invalid channel")) {
    return channel->mixer_element;
  } else {
    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static double in_channel_volume(InChannel const *channel) {
  if (ov_ptr_valid(channel, "Cannot get alsa device: Invalid channel")) {
    return channel->volume;
  } else {
    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static uint64_t get_frame_length_ms(ov_alsa_audio_app const *self) {

  if (0 != self) {

    return self->settings.frame_length_ms;

  } else {

    return OV_DEFAULT_FRAME_LENGTH_MS;
  }
}

/*----------------------------------------------------------------------------*/

static ov_json_value const *get_send_codec_for(ov_alsa_audio_app const *self,
                                               size_t i) {

  UNUSED(i);

  if (ov_ptr_valid(self, "Cannot get codec for sending")) {

    return self->settings.default_codec;

  } else {

    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static ov_json_value const *get_recv_codec_for(ov_alsa_audio_app const *self,
                                               size_t i) {

  UNUSED(i);

  if (ov_ptr_valid(self, "Cannot get codec for receiving")) {

    return self->settings.default_codec;

  } else {

    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static uint32_t get_channel_ssid(ov_alsa_audio_app const *self, size_t index) {

  if (0 == self) {
    return 0;

  } else {
    return self->ssid_of_first_channel + index;
  }
}

/*----------------------------------------------------------------------------*/

static ov_alsa_playback *create_alsa_playback(OutChannel const *channel,
                                              uint16_t msecs_to_buffer_ahead) {

  ov_alsa_playback *ap = 0;

  if (ov_ptr_valid(channel,
                   "Cannot prepare ALSA playback for output channel") &&
      ov_cond_valid(0 == channel->playback, "ALSA channel already in use")) {

    ov_alsa_playback_config pb_config = {
        .alsa_device = channel->device,
        .mixer_element = channel->mixer_element,
        .volume = channel->volume,
        .msecs_to_buffer_ahead = msecs_to_buffer_ahead,
        .comfort_noise.max_amplitude = 1,
        .logging_fd = channel->rtp_logging_fd,

    };

    ov_result result = {0};
    ap = ov_alsa_playback_create(pb_config, &result);

    if (0 == ap) {

      ov_log_error("Could not create ASLA playback: %s",
                   ov_result_get_message(result));
    }
  }

  return ap;
}

/*----------------------------------------------------------------------------*/

static ov_alsa_rtp_mixer *create_mixer_for(OutChannel *channel,
                                           ov_json_value const *codec_config,
                                           uint64_t frame_length_ms) {

  if (ov_ptr_valid(channel, "Cannot create mixer for channel - channel pointer "
                            "invalid")) {

    ov_alsa_rtp_mixer_config cfg = {

        .frame_length_ms = frame_length_ms,
        .codec_config = codec_config,
        .max_num_frames_per_stream = 10,
        .ssid_to_cancel = channel->ssid_to_cancel,

    };

    return ov_alsa_rtp_mixer_create(cfg);

  } else {

    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static size_t initialize_out_channels(ov_alsa_audio_app *self,
                                      OutChannel *channels, int rtp_logging_fd,
                                      size_t capacity, char const **devices,
                                      char const **mixer_elements,
                                      double const *volumes,
                                      size_t num_devices) {

  size_t i = 0;

  if (capacity < num_devices) {

    ov_log_warning("More ALSA devices configured than supported: %zu vs %zu",
                   capacity, num_devices);
  }

  if (ov_ptr_valid(channels,
                   "Cannot initialize ALSA device list - 0 pointer") &&
      ov_ptr_valid(devices, "Cannot initialize ALSA device list - 0 pointer")) {

    uint64_t frame_length_ms = get_frame_length_ms(self);

    for (i = 0; (i < capacity) && (i < num_devices); ++i) {

      memset(channels + i, 0, sizeof(OutChannel));
      channels[i].magic_bytes = OUT_MAGIC_BYTES;
      channels[i].device = ov_string_dup(devices[i]);
      channels[i].mixer_element = ov_string_dup(mixer_elements[i]);
      channels[i].volume = volumes[i];
      channels[i].ssid_to_cancel = get_channel_ssid(self, i);
      channels[i].rtp_logging_fd = rtp_logging_fd;

      if (0 == devices[i]) {

        ov_log_info("Not activating ALSA playback for channel %zu - "
                    "deactivated in config (no device set)",
                    i);

      } else {

        channels[i].buffer = ov_chunker_create();
        channels[i].playback = create_alsa_playback(channels + i, 100);
        channels[i].mixer = create_mixer_for(
            channels + i, get_recv_codec_for(self, i), frame_length_ms);

        ov_log_info("Prepared output channel %zu to device %s", i, devices[i]);
      }
    }
  }

  return i;
}

/*----------------------------------------------------------------------------*/

static size_t initialize_in_channels(InChannel *channels, size_t capacity,
                                     char const **devices,
                                     char const **mixer_elements,
                                     double const *volumes,
                                     size_t num_devices) {

  size_t i = 0;

  if (capacity < num_devices) {

    ov_log_warning("More ALSA devices configured than supported: %zu vs %zu",
                   capacity, num_devices);
  }

  if (ov_ptr_valid(channels,
                   "Cannot initialize ALSA device list - 0 pointer") &&
      ov_ptr_valid(devices, "Cannot initialize ALSA device list - 0 pointer")) {

    for (i = 0; (i < capacity) && (i < num_devices); ++i) {
      memset(channels + i, 0, sizeof(InChannel));
      channels[i].device = ov_string_dup(devices[i]);
      channels[i].mixer_element = ov_string_dup(mixer_elements[i]);
      channels[i].volume = volumes[i];
    }
  }

  return i;
}

/*----------------------------------------------------------------------------*/

static bool cb_rtp_io(ov_rtp_frame *rtp_frame, void *userdata) {

  UNUSED(rtp_frame);
  UNUSED(userdata);

  ov_log_info("Got RTP on sending socket?");

  rtp_frame = ov_rtp_frame_free(rtp_frame);

  return true;
}

/*----------------------------------------------------------------------------*/

static ov_socket_configuration socket_config_for_mc_loop(char const *loop,
                                                         uint16_t port) {

  ov_socket_configuration scfg = {
      .port = OV_OR_DEFAULT(port, OV_MC_SOCKET_PORT),
      .type = UDP,
  };

  if (0 != loop) {
    strncpy(scfg.host, loop, sizeof(scfg.host));
  }

  return scfg;
}

/*----------------------------------------------------------------------------*/

static bool cb_rtp_recv(ov_rtp_frame *rtp_frame, void *userdata) {

  bool result = false;
  OutChannel *channel = as_out_channel_mut(userdata);

  if (ov_ptr_valid(channel,
                   "Cannot forward RTP - Frame - invalid Channel pointer")) {

    if (rtp_frame->expanded.ssrc == channel->ssid_to_cancel) {

      ov_log_debug("Received RTP frame for own SSRC %" PRIu32 " - ignoring",
                   channel->ssid_to_cancel);
      result = true;

    } else if (ov_alsa_rtp_mixer_add_frame(channel->mixer, rtp_frame)) {

      rtp_frame = 0;
      result = true;
    }
  }

  rtp_frame = ov_rtp_frame_free(rtp_frame);

  return result;
}

/*----------------------------------------------------------------------------*/

static ov_rtp_app *create_rtp_app(ov_alsa_audio_app *self, OutChannel *channel,
                                  char const *mc_loop, uint16_t mc_port) {

  if (ov_ptr_valid(channel, "Cannot open RTP MC socket - ALSA channel pointer "
                            "invalid") &&
      ov_cond_valid(0 == channel->rtp,
                    "Cannot open RTP MC socket - ALSA channel already in "
                    "use")) {

    ov_rtp_app_config rtp_cfg = {

        .multicast = true,
        .rtp_socket = socket_config_for_mc_loop(mc_loop, mc_port),
        .rtp_handler = cb_rtp_recv,
        .rtp_handler_userdata = channel,

    };

    return ov_rtp_app_create(self->loop, rtp_cfg);

  } else {

    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static uint16_t get_mc_port_from_list(uint16_t *ports, size_t index) {

  if (0 == ports) {
    return OV_MC_SOCKET_PORT;
  } else {
    return OV_OR_DEFAULT(ports[index], OV_MC_SOCKET_PORT);
  }
}

/*----------------------------------------------------------------------------*/

bool ov_alsa_audio_app_start_playbacks(ov_alsa_audio_app *self,
                                       char const **loops,
                                       uint16_t *loop_mc_ports,
                                       size_t num_loops) {

  UNUSED(self);

  size_t num_failed_to_set_up = 0;

  for (size_t i = 0; i < num_loops; ++i) {

    OutChannel *channel = get_out_channel_ptr_mut(self, i);

    if ((0 != loops[i]) && (0 != channel) && (0 != channel->playback) &&
        (0 == channel->rtp)) {

      channel->rtp = create_rtp_app(self, channel, loops[i],
                                    get_mc_port_from_list(loop_mc_ports, i));

      if (!ov_ptr_valid(channel->rtp, "Could not join MC loop")) {
        ov_log_info("Setup static playback of openvocs loop %s:%" PRIu16
                    " on channel %zu "
                    "FAILED",
                    loops[i], get_mc_port_from_list(loop_mc_ports, i), i);

        ++num_failed_to_set_up;

      } else {
        ov_log_info("Setup static playback of openvocs loop %s:%" PRIu16
                    " on channel %zu SUCCESSFUL",
                    loops[i], get_mc_port_from_list(loop_mc_ports, i), i);
      }

    } else if (0 != loops[i]) {

      ++num_failed_to_set_up;
      ov_log_error(
          "Could not setup static playback of openvocs loop %s:%" PRIu16 " on "
          "channel %zu",
          loops[i], get_mc_port_from_list(loop_mc_ports, i), i);
    }
  }

  return 0 == num_failed_to_set_up;
}

/*----------------------------------------------------------------------------*/

bool ov_alsa_audio_app_start_recordings(ov_alsa_audio_app *self,
                                        char const **loops,
                                        uint16_t *loop_mc_ports,
                                        size_t num_loops) {

  if (ov_ptr_valid(loops, "Cannot start recordings - invalid loops")) {

    size_t num_failed_to_set_up = 0;
    ov_result res = {0};
    for (size_t i = 0; i < num_loops; ++i) {

      if ((0 != loops[i]) &&
          ov_alsa_audio_app_record_to(
              self,
              socket_config_for_mc_loop(
                  loops[i], get_mc_port_from_list(loop_mc_ports, i)),
              get_channel_ssid(self, i), get_send_codec_for(self, i), i,
              &res)) {

        ov_log_info("Setup static recording of channel %zu towards multicast "
                    "loop "
                    "%s",
                    i, loops[i]);

      } else if (0 != loops[i]) {

        ++num_failed_to_set_up;
        ov_log_error("Could not setup static recording of channel %zu towards "
                     "%s: "
                     "%s",
                     i, loops[i], ov_result_get_message(res));
      }

      ov_result_clear(&res);
    }

    return 0 == num_failed_to_set_up;

  } else {
    return false;
  }
}

/*----------------------------------------------------------------------------*/

static int open_file(char const *path, char const *err_msg) {

  if (0 == path) {

    return -1;

  } else {

    int fd = open(path, O_WRONLY | O_CREAT | O_CLOEXEC | O_TRUNC,
                  S_IRWXU | S_IRWXG | S_IRWXO);

    if (0 > fd) {

      ov_log_error("%s %s: %s", ov_string_sanitize(err_msg),
                   ov_string_sanitize(path),
                   ov_string_sanitize(strerror(errno)));
    }

    return fd;
  }
}

/*----------------------------------------------------------------------------*/

ov_alsa_audio_app *ov_alsa_audio_app_create(ov_event_loop *loop,
                                            ov_alsa_audio_app_config cfg) {

  ov_alsa_audio_app *self = 0;

  ov_rtp_app *rtp_app =
      ov_rtp_app_create(loop, (ov_rtp_app_config){
                                  .rtp_handler = cb_rtp_io,
                                  .rtp_socket = cfg.rtp_socket,
                              });

  if (ov_ptr_valid(rtp_app,
                   "Cannot create alsa app - Cannot open RTP socket")) {

    self = calloc(1, sizeof(ov_alsa_audio_app));
    self->magic_bytes = MAGIC_BYTES;

    self->num_channels = OV_ALSA_MAX_DEVICES;

    self->ssid_of_first_channel = cfg.ssid_of_first_channel;

    self->settings.default_codec =
        ov_json_value_from_string(OV_INTERNAL_CODEC, strlen(OV_INTERNAL_CODEC));

    self->loop = loop;
    self->rtp_app_send = rtp_app;
    rtp_app = 0;

    self->mix_and_replay_timer = OV_TIMER_INVALID;

    initialize_out_channels(
        self, self->out_channel, self->num_channels,
        open_file(cfg.debug.rtp_logging, "Cannot open RTP logging file"),
        (char const **)cfg.channels.output,
        (char const **)cfg.channel_mixer.output, cfg.channel_volumes.output,
        OV_ALSA_MAX_DEVICES);

    initialize_in_channels(self->in_channel, self->num_channels,
                           (char const **)cfg.channels.input,
                           (char const **)cfg.channel_mixer.input,
                           cfg.channel_volumes.input, OV_ALSA_MAX_DEVICES);

    self->settings.frame_length_ms = OV_DEFAULT_FRAME_LENGTH_MS;
    self->settings.max_frames_to_buffer = cfg.max_num_frames;
    self->mix_and_replay_timer = OV_TIMER_INVALID;
  }

  return self;
}

/*----------------------------------------------------------------------------*/

static void clear_out_channels(OutChannel *channels, size_t num_channels) {

  if ((0 != channels) && (0 < num_channels)) {

    ov_socket_close(channels[0].rtp_logging_fd);

    for (size_t i = 0; i < num_channels; ++i) {
      channels[i].playback = ov_alsa_playback_free(channels[i].playback);
      channels[i].rtp = ov_rtp_app_free(channels[i].rtp);
      channels[i].buffer = ov_chunker_free(channels[i].buffer);
      channels[i].device = ov_free(channels[i].device);
      channels[i].mixer_element = ov_free(channels[i].mixer_element);
      channels[i].mixer = ov_alsa_rtp_mixer_free(channels[i].mixer);
      channels[i].rtp_logging_fd = -1;
    }
  }
}

/*----------------------------------------------------------------------------*/

static void clear_in_channels(InChannel *channels, size_t num_channels) {

  if (0 != channels) {

    for (size_t i = 0; i < num_channels; ++i) {
      channels[i].device = ov_free(channels[i].device);
      channels[i].mixer_element = ov_free(channels[i].mixer_element);
      channels[i].record = ov_alsa_record_free(channels[i].record);
    }
  }
}

/*----------------------------------------------------------------------------*/

static bool stop_mix_and_replay_timer(ov_alsa_audio_app *self) {

  return ((0 != self) && ((OV_TIMER_INVALID == self->mix_and_replay_timer) ||
                          ov_event_loop_timer_unset(
                              self->loop, self->mix_and_replay_timer, 0)));
}

/*----------------------------------------------------------------------------*/

ov_alsa_audio_app *ov_alsa_audio_app_free(ov_alsa_audio_app *self) {

  if ((0 != self) && stop_mix_and_replay_timer(self)) {

    clear_out_channels(self->out_channel, self->num_channels);
    clear_in_channels(self->in_channel, self->num_channels);
    self->rtp_app_send = ov_rtp_app_free(self->rtp_app_send);

    self->settings.default_codec =
        ov_json_value_free(self->settings.default_codec);

    self = ov_free(self);
  }

  return self;
}

/*----------------------------------------------------------------------------*/

static ov_alsa_record *
create_record(ov_rtp_app const *rtp_app, char const *device,
              char const *mixer_element, ov_socket_configuration mcsocket,
              uint32_t ssid, ov_json_value const *codec_config, double volume,
              ov_result *res) {

  if (!ov_ptr_valid(device, "Invalid input channel")) {
    ov_result_set(res, OV_ERROR_NO_RESOURCE, "Invalid analogue channel");
    return false;

  } else {
    ov_alsa_record_config record_cfg = {

        .rtp_stream = {.codec_config = codec_config,
                       .frame_length_ms = OV_DEFAULT_FRAME_LENGTH_MS,
                       .ssid = ssid,
                       .target = mcsocket},

        .volume = volume,

        .alsa_device = device,
        .mixer_element = mixer_element,

        .comfort_noise =
            {
                .max_amplitude = 1,
            },

    };

    return ov_alsa_record_create(record_cfg, ov_rtp_app_get_rtp_sd(rtp_app),
                                 res);
  }
}

/*----------------------------------------------------------------------------*/

bool ov_alsa_audio_app_record_to(ov_alsa_audio_app *self,
                                 ov_socket_configuration mcsocket,
                                 uint32_t ssid,
                                 ov_json_value const *codec_config,
                                 uint32_t analogue_channel, ov_result *res) {

  InChannel *channel = get_in_channel_mut(self, analogue_channel);

  ov_alsa_record *record =
      create_record(get_rtp_app_send(self), in_channel_device(channel),
                    in_channel_mixer_element(channel), mcsocket, ssid,
                    codec_config, in_channel_volume(channel), res);

  if ((0 != channel) && (0 != record)) {

    channel->record = record;
    record = 0;
    return true;

  } else {

    record = ov_alsa_record_free(record);
    return false;
  }
}

/*****************************************************************************
                                 list_channels
 ****************************************************************************/

static bool in_channel_to_json_to(ov_json_value *jchannel,
                                  InChannel const *channel) {

  if (ov_ptr_valid(jchannel, "No target JSON object") &&
      ov_ptr_valid(channel, "Invalid channel") && (0 != channel->device) &&
      ov_json_object_set(jchannel, OV_KEY_DEVICE,
                         ov_json_string(channel->device)) &&
      ov_json_object_set(jchannel, OV_KEY_IN_USE,
                         ov_json_bool(0 != channel->record))) {

    return true;

  } else {

    return false;
  }
}

/*----------------------------------------------------------------------------*/

static ov_json_value *in_channel_to_json(InChannel const *channel) {

  ov_json_value *jchannel = ov_json_object();

  if (!in_channel_to_json_to(jchannel, channel)) {
    jchannel = ov_json_value_free(jchannel);
  }

  return jchannel;
}

/*----------------------------------------------------------------------------*/

static bool out_channel_to_json_to(ov_json_value *jchannel,
                                   OutChannel const *channel) {

  if (ov_ptr_valid(jchannel, "No target JSON object") &&
      ov_ptr_valid(channel, "Invalid channel") && (0 != channel->device) &&
      ov_json_object_set(jchannel, OV_KEY_DEVICE,
                         ov_json_string(channel->device)) &&
      ov_json_object_set(jchannel, OV_KEY_IN_USE,
                         ov_json_bool(0 != channel->playback))) {

    return true;

  } else {

    return false;
  }
}

/*----------------------------------------------------------------------------*/

static ov_json_value *out_channel_to_json(OutChannel const *channel) {

  ov_json_value *jchannel = ov_json_object();

  if (!out_channel_to_json_to(jchannel, channel)) {
    jchannel = ov_json_value_free(jchannel);
  }

  return jchannel;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *channel_to_json(ov_alsa_audio_app const *self,
                                      size_t channel_no,
                                      ov_analogue_event_type type) {

  switch (type) {

  case ANALOGUE_IN:
    return in_channel_to_json(get_in_channel_ptr(self, channel_no));

  case ANALOGUE_OUT:
    return out_channel_to_json(get_out_channel_ptr(self, channel_no));

  case ANALOGUE_INVALID:
  default:

    ov_log_error("Invalid channel type");
    return 0;
  };
}

/*----------------------------------------------------------------------------*/

static bool list_channels(ov_json_value *jchannels,
                          ov_alsa_audio_app const *self,
                          ov_analogue_event_type type) {

  if (ov_ptr_valid(jchannels, "Cannot list output channels - no target JSON "
                              "object") &&
      ov_ptr_valid(self, "Cannot list output channels - no ALSA app")) {

    // a byte yields 3 digits max...
    char channel_number[3 * sizeof(size_t) + 1] = {0};

    for (size_t i = 0; i < self->num_channels; ++i) {

      snprintf(channel_number, sizeof(channel_number), "%zu", i);
      ov_json_value *jchannel = channel_to_json(self, i, type);

      if (jchannel) {
        ov_json_object_set(jchannels, channel_number, jchannel);
      }
    }

    return true;

  } else {

    return false;
  }
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_alsa_audio_app_list_channels(ov_alsa_audio_app const *self,
                                               ov_analogue_event_type type) {

  ov_json_value *channels = ov_json_object();

  if (!ov_cond_valid((ANALOGUE_IN == type) || (ANALOGUE_OUT == type),
                     "Cannot list channels - invalid channel type "
                     "requested") ||
      (!list_channels(channels, self, type))) {
    channels = ov_json_value_free(channels);
  }

  return channels;
}

/*----------------------------------------------------------------------------*/

static bool mix_channel(OutChannel *channel) {

  if ((0 != channel) && (0 != channel->mixer)) {

    if (2000 < ++channel->times_run_since_last_gc) {

      ov_alsa_rtp_mixer_garbage_collect(channel->mixer, 5);
      channel->times_run_since_last_gc = 0;
    }

    if (!ov_alsa_rtp_mixer_mix(channel->mixer, channel->buffer)) {

      ov_log_error("Could not mix frames for ALSA device: %s",
                   ov_string_sanitize(channel->device));

      return false;
    }

    return true;

  } else {
    return false;
  }
}

/*----------------------------------------------------------------------------*/

static bool mix_channels(ov_alsa_audio_app *self) {

  if (ov_ptr_valid(self, "Cannot mix frames: Invalid ASLA audio app pointer")) {

    for (size_t i = 0; i < self->num_channels; ++i) {

      mix_channel(get_out_channel_ptr_mut(self, i));
    }

    return true;

  } else {

    return false;
  }
}

/*----------------------------------------------------------------------------*/

static bool replay_channels(ov_alsa_audio_app *self) {

  if (ov_ptr_valid(self,
                   "Cannot replay frames: Invalid ASLA audio app pointer")) {

    for (size_t i = 0; i < self->num_channels; ++i) {

      OutChannel const *channel = get_out_channel_ptr(self, i);

      if ((0 != channel) && (0 != channel->playback)) {

        switch (ov_alsa_playback_play(channel->playback, channel->buffer)) {

        case ALSA_REPLAY_OK:
          break;

        case ALSA_REPLAY_INSUFFICIENT:

          // ov_log_warning(
          //     "Not enough PCM - playing comfort noise");
          // ov_alsa_playback_play_comfort_noise(channel->playback);
          break;

        case ALSA_REPLAY_FAILED:
        default:

          ov_log_error("Replaying failed for ALSA device %s",
                       ov_string_sanitize(channel->device));

          break;
        }
      }
    }

    return true;

  } else {

    return false;
  }
}

/*----------------------------------------------------------------------------*/

static bool cb_mix_and_replay(uint32_t id, void *data) {

  ov_alsa_audio_app *app = as_alsa_audio_app_mut(data);

  OV_ASSERT(0 != app);
  OV_ASSERT(app->mix_and_replay_timer == id);

  app->mix_and_replay_timer = ov_event_loop_timer_set(
      app->loop, app->settings.frame_length_ms * 1000, app, cb_mix_and_replay);

  if (ov_ptr_valid(app, "Cannot mix and replay: Invalid pointer")) {

    replay_channels(app);
    mix_channels(app);
  }

  return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_alsa_audio_app_start_playback_thread(ov_alsa_audio_app *self) {

  if (ov_ptr_valid(self, "Cannot start mix and playback thread: Invalid "
                         "pointer") &&
      ov_cond_valid(OV_TIMER_INVALID == self->mix_and_replay_timer,
                    "Cannot start mix and playback timer: Already "
                    "running")) {

    self->mix_and_replay_timer = ov_event_loop_timer_set(
        self->loop, self->settings.frame_length_ms * 1000, self,
        cb_mix_and_replay);

    return ov_cond_valid(OV_TIMER_INVALID != self->mix_and_replay_timer,
                         "Could not start mix and replay timer");

  } else {

    return false;
  }
}

/*----------------------------------------------------------------------------*/

void ov_alsa_audio_app_enable_caching() { ov_alsa_rtp_mixer_enable_caching(); }

/*----------------------------------------------------------------------------*/
