/***
        ------------------------------------------------------------------------

        Copyright (c) 2017, 2023 German Aerospace Center DLR e.V. (GSOC)

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

#include <pthread.h>
#if defined __linux__

#include "ov_alsa.h"

#include <alsa/asoundlib.h>
#include <ov_arch/ov_arch_math.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_string.h>
#include <stdio.h>
#include <stdlib.h>

/*----------------------------------------------------------------------------*/

char const *ov_alsa_mode_to_string(ov_alsa_mode mode) {

  switch (mode) {

  case CAPTURE:

    return "CAPTURE";

  case PLAYBACK:
    return "PLAYBACK";

  default:
    return "INVALID";
  };

  return "INVALID";
}

/*****************************************************************************
                                     Volume
 ****************************************************************************/

static char *strconcat(char const *str1, char const *str2) {

  size_t len1 = ov_string_len(str1);
  size_t len2 = ov_string_len(str2);
  char *concat = 0;

  if (0 < len1) {

    size_t capacity = len1 + len2 + 1;

    concat = calloc(1, capacity);
    strncpy(concat, str1, capacity);

    if (0 < len2) {
      strncat(concat, str2, capacity);
    }
  }

  return concat;
}

/*----------------------------------------------------------------------------*/

static char *device_to_card(char const *device) {

  char *card = 0;

  if (0 != device) {

    card = strconcat("hw:", device);

    char *endpos = strchr(card, ',');
    if (0 != endpos) {
      *endpos = 0;
    }
  }

  return card;
}

/*----------------------------------------------------------------------------*/

static snd_mixer_elem_t *get_mixer_elem(snd_mixer_t *mixer, char const *device,
                                        char const *name) {
  snd_mixer_elem_t *elem = 0;

  char *card = device_to_card(device);

  if ((0 != mixer) && (0 != card) && (0 != name)) {

    snd_mixer_selem_id_t *sid = 0;

    snd_mixer_attach(mixer, card);
    snd_mixer_selem_register(mixer, 0, 0);
    snd_mixer_load(mixer);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, name);

    elem = snd_mixer_find_selem(mixer, sid);
  }

  if (0 == elem) {
    ov_log_error("Could not find mixer element for %s:%s",
                 ov_string_sanitize(card), ov_string_sanitize(name));
  }

  card = ov_free(card);

  return elem;
}

/*----------------------------------------------------------------------------*/

static bool mute_element(snd_mixer_elem_t *elem, bool should_mute) {
  if (0 == elem) {
    return false;
  } else {
    if (snd_mixer_selem_has_playback_switch(elem)) {
      ov_log_debug("%sing playback\n", should_mute ? "Mut" : "Unmut");
      snd_mixer_selem_set_playback_switch_all(elem, should_mute ? 0 : 1);
    }
    if (snd_mixer_selem_has_capture_switch(elem)) {
      ov_log_debug("%sing capture\n", should_mute ? "Mut" : "Unmut");
      snd_mixer_selem_set_capture_switch_all(elem, should_mute ? 0 : 1);
    }
    return true;
  }
}

/*----------------------------------------------------------------------------*/

static bool element_volume_range(snd_mixer_elem_t *elem, ov_alsa_mode mode,
                                 long *min, long *max) {
  assert(0 != min);
  assert(0 != max);

  if (0 == elem) {
    ov_log_error("Cannot get ALSA device element - no mixer");
    return false;

  } else {
    switch (mode) {
    case PLAYBACK:

      if (1 == snd_mixer_selem_has_playback_volume(elem)) {
        snd_mixer_selem_get_playback_volume_range(elem, min, max);
        ov_log_info("Playback volume range is %li - %li\n", *min, *max);
        return true;
      } else {
        ov_log_info("Cannot get playback volume range\n");
        return false;
      }

    case CAPTURE:

      if (1 == snd_mixer_selem_has_capture_volume(elem)) {
        snd_mixer_selem_get_capture_volume_range(elem, min, max);
        ov_log_info("Capture volume range is %li - %li\n", *min, *max);
        return true;
      } else {
        ov_log_info("Cannot get capture volume range\n");
        return false;
      }

    default:

      return false;
    }
  }
}

/*----------------------------------------------------------------------------*/

static bool element_volume(snd_mixer_elem_t *elem, ov_alsa_mode mode,
                           long volume) {
  if (0 == elem) {
    ov_log_error("Cannot set ALSA volume: No mixer element");
    return false;

  } else {
    switch (mode) {
    case PLAYBACK:

      if (1 == snd_mixer_selem_has_playback_volume(elem)) {
        ov_log_info("Setting playback volume to %li\n", volume);
        snd_mixer_selem_set_playback_volume_all(elem, volume);
        return true;
      } else {
        ov_log_info("Cannot set playback volume to %li\n", volume);
        return false;
      }

    case CAPTURE:

      if (1 == snd_mixer_selem_has_capture_volume(elem)) {
        ov_log_info("Setting capture volume to %li\n", volume);
        snd_mixer_selem_set_capture_volume_all(elem, volume);
        return true;
      } else {
        ov_log_info("Cannot set capture volume to %li\n", volume);
        return false;
      }

    default:

      return false;
    }
  }
}
/*----------------------------------------------------------------------------*/

static bool set_volume(snd_mixer_elem_t *elem, ov_alsa_mode mode,
                       double volume) {
  if (0.0 > volume) {
    ov_log_error("Cannot set ALSA volume to %f: Volume negative", volume);
    return false;

  } else if (1.0 < volume) {
    ov_log_error("Cannot set ALSA volume to %f: Volume out of range - must be "
                 "between 0 and 1",
                 volume);
    return false;

  } else {
    long min = 0;
    long max = 0;

    element_volume_range(elem, mode, &min, &max);

    volume *= max - min;
    volume += min;

    return element_volume(elem, mode, (long)volume);
  }
}

/*----------------------------------------------------------------------------*/

bool ov_alsa_set_volume(char const *device, char const *element,
                        ov_alsa_mode mode, double volume) {

  snd_mixer_t *handle = 0;
  snd_mixer_open(&handle, 0);

  snd_mixer_elem_t *elem = get_mixer_elem(handle, device, element);

  bool ok = mute_element(elem, false) && set_volume(elem, mode, volume);

  snd_mixer_close(handle);

  if (ok) {
    ov_log_info("ALSA: %s mixer element %s: Set volume to %lf",
                ov_string_sanitize(device), ov_string_sanitize(element),
                volume);
  }

  return ok;
}

/*****************************************************************************
                                DEBUG - dump PCM
 ****************************************************************************/

static bool dump_pcm_to_file = false;

int open_dump_fh(char const *device, snd_pcm_stream_t mode) {

  char const *mode_str = snd_pcm_stream_name(mode);

  int fh = -1;

  if (dump_pcm_to_file && (0 != device) && (0 != mode_str)) {

    char name[400] = {0};
    snprintf(name, sizeof(name), "/tmp/%s_%s.raw", device, mode_str);
    name[sizeof(name) - 1] = 0;

    fh = open(name, O_CLOEXEC | O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);

    if (0 > fh) {

      ov_log_error("Could not open dump file:%s", ov_string_sanitize(name));
    }
  }

  return fh;
}

/*----------------------------------------------------------------------------*/

static void dump_pcm(int fh, int16_t *buf, size_t num_samples) {

  if ((-1 < fh) &&
      (ov_ptr_valid(buf, "Cannot dump PCM: No PCM") &&
       ov_cond_valid(0 < num_samples, "Cannot dump PCM: Number of samples is "
                                      "0"))) {

    ov_log_debug("Dumping %zu bytes\n", num_samples);

    ssize_t written = write(fh, buf, 2 * num_samples);

    if (0 > written) {

      ov_log_error("Failed to dump PCM: %s\n", strerror(errno));

    } else if ((size_t)written != 2 * num_samples) {

      ov_log_error("Could not dump all PCM\n");
    }
  }
}

/*****************************************************************************
                               Low-level helpers
 ****************************************************************************/

static char const *pcm_state_as_string(snd_pcm_t *apcm) {

  if (0 != apcm) {

    return snd_pcm_state_name(snd_pcm_state(apcm));

  } else {

    return "Invalid ALSA object";
  }
}

/*----------------------------------------------------------------------------*/

static char const *pcm_stream_name(snd_pcm_t *apcm) {

  if (0 == apcm) {

    return "Invalid ALSA handle";

  } else {

    return snd_pcm_name(apcm);
  }
}

/*----------------------------------------------------------------------------*/

bool ov_alsa_enabled() { return true; }

/*----------------------------------------------------------------------------*/

struct ov_alsa_struct {
  snd_pcm_t *handle;
  snd_pcm_uframes_t samples_per_period;

  int dump_fh;

  ov_alsa_counters counter;
};

/*----------------------------------------------------------------------------*/

static snd_pcm_t *get_handle(ov_alsa const *self) {

  if (0 != self) {
    return self->handle;
  } else {
    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static size_t get_samples_per_period(ov_alsa const *self) {

  if (0 != self) {
    return (size_t)self->samples_per_period;
  } else {
    return 0;
  }
}

/*----------------------------------------------------------------------------*/

bool ov_alsa_reset(ov_alsa *self) {

  snd_pcm_t *handle = get_handle(self);
  if (0 != handle) {
    snd_pcm_prepare(handle);
    return true;
  } else {
    return false;
  }
}

/*---------------------------------------------------------------------------*/

static bool alsa_call(ov_alsa const *self, int alsa_retval,
                      char const *message) {

  if (alsa_retval < 0) {
    ov_log_error("%s (%s)", OV_OR_DEFAULT(message, "Unknown ALSA error"),
                 snd_strerror(alsa_retval));
    return false;
  } else {

    snd_pcm_t *pcm = get_handle(self);

    printf("ALSA stream %s state: %s\n", pcm_stream_name(pcm),
           pcm_state_as_string(pcm));

    return true;
  }
}

/*----------------------------------------------------------------------------*/

static bool ensure_number_of_channels(snd_pcm_hw_params_t const *params,
                                      size_t exp_channels) {

  unsigned channels = 0;

  if (ov_ptr_valid(params, "Cannot get number of channels - 0 pointer") &&
      (-1 < snd_pcm_hw_params_get_channels(params, &channels)) &&
      ov_cond_valid(exp_channels == channels,
                    "Could not set required number of channels")) {

    ov_log_info("ALSA: Using %u channels", channels);
    return true;

  } else {
    return false;
  }
}

/*----------------------------------------------------------------------------*/

static bool ensure_format(snd_pcm_hw_params_t const *params,
                          snd_pcm_format_t exp_format) {

  snd_pcm_format_t format;

  if (ov_ptr_valid(params, "Cannot get actual format - 0 pointer") &&
      (-1 < snd_pcm_hw_params_get_format(params, &format)) &&
      ov_cond_valid(exp_format == format, "Could not set required format")) {

    ov_log_info("ALSA: Using format %s", snd_pcm_format_name(format));
    return true;

  } else {
    return false;
  }
}

/*----------------------------------------------------------------------------*/

static bool ensure_sample_rate(snd_pcm_hw_params_t const *params,
                               uint64_t exp_rate_hz) {
  unsigned rate_hz = 0;
  int dir = 0;

  if (ov_ptr_valid(params, "Cannot get actual sample rate - 0 pointer") &&
      (-1 < snd_pcm_hw_params_get_rate(params, &rate_hz, &dir)) &&
      ov_cond_valid(exp_rate_hz == rate_hz,
                    "Could not set required sample rate")) {

    ov_log_info("ALSA: Using sample rate of %u Hz", rate_hz);
    return true;

  } else {
    return false;
  }
}

/*----------------------------------------------------------------------------*/

static snd_pcm_access_t stream_mode_to_pcm_access_mode(snd_pcm_stream_t mode) {

  switch (mode) {

  case SND_PCM_STREAM_PLAYBACK:

    return SND_PCM_ACCESS_RW_NONINTERLEAVED;

  case SND_PCM_STREAM_CAPTURE:
    return SND_PCM_ACCESS_RW_INTERLEAVED;

  default:

    OV_ASSERT(!"MUST NEVER HAPPEN");
    return SND_PCM_ACCESS_RW_NONINTERLEAVED;
  };
}

/*----------------------------------------------------------------------------*/

static ov_alsa *alsa_create(char const *device_name, unsigned int sample_rate,
                            size_t *samples_per_period, snd_pcm_stream_t mode) {

  if ((0 == device_name) || (0 == samples_per_period)) {

    return 0;

  } else {

    snd_pcm_uframes_t frames_per_period =
        OV_OR_DEFAULT(*samples_per_period, OV_DEFAULT_FRAME_LENGTH_SAMPLES);

    ov_alsa *alsa = calloc(1, sizeof(ov_alsa));
    alsa->dump_fh = -1;

    snd_pcm_hw_params_t *hw_params;
    unsigned int actual_sample_rate = sample_rate;

    int err = -1;

    if ((err = snd_pcm_open(&alsa->handle, device_name, mode, 0)) < 0) {

      ov_log_error("cannot open audio device %s (%s)\n", device_name,
                   snd_strerror(err));
      return ov_free(alsa);
    }

    snd_config_update_free_global();

    if (alsa_call(alsa, snd_pcm_hw_params_malloc(&hw_params),
                  "cannot allocate hardware parameter structure") &&

        alsa_call(alsa, snd_pcm_hw_params_any(alsa->handle, hw_params),
                  "cannot initialize hardware parameter "
                  "structure") &&

        alsa_call(
            alsa,
            snd_pcm_hw_params_set_access(alsa->handle, hw_params,
                                         stream_mode_to_pcm_access_mode(mode)),
            "cannot set access type") &&

        alsa_call(alsa,
                  snd_pcm_hw_params_set_period_size(alsa->handle, hw_params,
                                                    frames_per_period, 0),
                  "Cannot set period size") &&

        alsa_call(alsa,
                  snd_pcm_hw_params_set_format(alsa->handle, hw_params,
                                               SND_PCM_FORMAT_S16_LE),
                  "cannot set sample format") &&

        alsa_call(alsa,
                  snd_pcm_hw_params_set_rate_near(alsa->handle, hw_params,
                                                  &actual_sample_rate, 0),
                  "cannot set sample rate") &&

        alsa_call(alsa,
                  snd_pcm_hw_params_set_channels(alsa->handle, hw_params, 1),
                  "cannot set channel count to 1") &&

        alsa_call(alsa, snd_pcm_hw_params(alsa->handle, hw_params),
                  "cannot set parameters") &&

        ensure_number_of_channels(hw_params, 1) &&
        ensure_format(hw_params, SND_PCM_FORMAT_S16_LE) &&
        ensure_sample_rate(hw_params, sample_rate) &&

        alsa_call(alsa, snd_pcm_prepare(alsa->handle),
                  "cannot prepare audio interface for use")) {

      unsigned int val = 0;
      int dir = 0;

      snd_pcm_hw_params_get_period_time(hw_params, &val, &dir);
      printf("period time = %d us\n", val);

      snd_pcm_hw_params_get_period_size(hw_params, samples_per_period, &dir);
      printf("period size = %d samples\n", (int)*samples_per_period);

      alsa->samples_per_period = *samples_per_period;

      snd_pcm_hw_params_get_buffer_time(hw_params, &val, &dir);
      printf("buffer time = %d us\n", val);

      snd_pcm_hw_params_free(hw_params);

      alsa->dump_fh = open_dump_fh(device_name, mode);

      return alsa;
    } else {
      snd_pcm_hw_params_free(hw_params);
      return ov_alsa_free(alsa);
    }
  }
}

/*----------------------------------------------------------------------------*/

ov_alsa *ov_alsa_create(char const *device_name, unsigned int sample_rate,
                        size_t *samples_per_period, ov_alsa_mode mode) {

  char *alsa_name = 0;

  if ((0 != device_name) && (0 == strcmp(device_name, "default"))) {

    alsa_name = strdup("default");

  } else {

    alsa_name = strconcat("plughw:", device_name);
  }

  ov_log_debug("Using ALSA device %s - device string to connect:%s \n",
               ov_string_sanitize(device_name), ov_string_sanitize(alsa_name));

  ov_alsa *self = 0;

  switch (mode) {

  case CAPTURE:

    self = alsa_create(alsa_name, sample_rate, samples_per_period,
                       SND_PCM_STREAM_CAPTURE);
    break;

  case PLAYBACK:

    self = alsa_create(alsa_name, sample_rate, samples_per_period,
                       SND_PCM_STREAM_PLAYBACK);

    break;

  default:
    ov_log_error("Cannot create ALSA object: Invalid mode");
  };

  alsa_name = ov_free(alsa_name);

  return self;
}

/*---------------------------------------------------------------------------*/

ov_alsa *ov_alsa_free(ov_alsa *self) {

  if (0 != self) {

    self->dump_fh = ov_socket_close(self->dump_fh);

    snd_pcm_drop(self->handle);
    snd_pcm_close(self->handle);
    // Cannot use ov_base/ov_utils.h:ov_free() because asoundlib.h
    // includes assert.h
    free(self);
    self = 0;
  }

  snd_config_update_free_global();

  return self;
}

/*----------------------------------------------------------------------------*/

size_t ov_alsa_get_buffer_size_samples(ov_alsa *self) {

  snd_pcm_uframes_t buffer_size = 0;
  snd_pcm_uframes_t period_size = 0;

  snd_pcm_t *handle = get_handle(self);

  if (ov_ptr_valid(handle,
                   "Cannot get ALSA buffer size - invalid ALSA handle") &&
      (0 == snd_pcm_get_params(handle, &buffer_size, &period_size))) {

    return buffer_size;

  } else {

    ov_log_error("Could not get buffer size for ALSA input buffer");
    return 0;
  }
}

/*---------------------------------------------------------------------------*/

size_t ov_alsa_get_samples_per_period(ov_alsa *self) {

  if (ov_ptr_valid(self, "Cannot get ALSA period size - invalid ALSA handle")) {

    return self->samples_per_period;
  } else {
    return 0;
  }
}

/*---------------------------------------------------------------------------*/

ssize_t ov_alsa_get_no_available_samples(ov_alsa *self) {

  snd_pcm_t *handle = get_handle(self);
  snd_pcm_sframes_t available_frames = -1;

  if (ov_ptr_valid(handle, "Cannot get number of available samples - invalid "
                           "ALSA handler")) {
    available_frames = snd_pcm_avail(handle);
  }

  if (0 > available_frames) {
    ov_log_error("Cannot get available samples from ALSA: %li:%s\n",
                 available_frames, snd_strerror(available_frames));

    ov_alsa_reset(self);
  }

  return (ssize_t)available_frames;
}

/*----------------------------------------------------------------------------*/

static snd_pcm_sframes_t snd_pcm_writen_wrapper(snd_pcm_t *handle, int16_t *buf,
                                                size_t sample_number) {

  if (ov_ptr_valid(handle, "Cannot write to ALSA device - invalid handle")) {

    snd_pcm_sframes_t written_frames =
        snd_pcm_writen(handle, (void **)&buf, sample_number);

    if (0 > written_frames) {

      ov_log_error("ALSA: Error writing: %i", written_frames);
      snd_pcm_recover(handle, written_frames, 0);

    } else {
      ov_log_debug("ALSA: Wrote %i frames", written_frames);
    }

    return written_frames;

  } else {
    return -ENODEV;
  }
}

/*----------------------------------------------------------------------------*/

bool ov_alsa_play_period(ov_alsa *self, int16_t *buf, ov_result *res) {

  if (ov_ptr_valid(buf, "Cannot play PCM: Invalid PCM pointer")) {

    snd_pcm_uframes_t samples_per_period = get_samples_per_period(self);

    dump_pcm(self->dump_fh, buf, samples_per_period);

    snd_pcm_sframes_t err =
        snd_pcm_writen_wrapper(get_handle(self), buf, samples_per_period);

    if (err != (snd_pcm_sframes_t)samples_per_period) {

      if (-EPIPE == err) {

        ov_result_set(res, OV_ERROR_AUDIO_UNDERRUN, "Buffer underrun occured");

        ov_counter_increase(self->counter.underruns, 1);

      } else {

        ov_result_set(res, OV_ERROR_AUDIO_IO,
                      "Could not write to AUDIO interface");

        ov_log_error("write to audio interface failed (%s)\n",
                     snd_strerror(err));

        ov_counter_increase(self->counter.other_error, 1);
      }

      return false;

    } else {

      ov_counter_increase(self->counter.periods_played, 1);
      ov_result_set(res, OV_ERROR_NOERROR, 0);
      return true;
    }

  } else {

    ov_result_set(res, OV_ERROR_BAD_ARG, "Invalid ALSA object");
    return false;
  }
}

/*----------------------------------------------------------------------------*/

static snd_pcm_sframes_t snd_pcm_readi_wrapper(snd_pcm_t *handle,
                                               int16_t *target_buffer,
                                               size_t samples_to_read) {

  if (!ov_ptr_valid(handle,
                    "Cannot read from ALSA device - invalid ALSA handle")) {

    return -ENODEV;

  } else if (!ov_ptr_valid(target_buffer,
                           "Cannot read from ALSA device - invalid "
                           "target buffer")) {

    return -EINVAL;

  } else {

    return snd_pcm_readi(handle, (void *)target_buffer, samples_to_read);
  }
}

/*----------------------------------------------------------------------------*/

size_t ov_alsa_capture_period(ov_alsa *self, int16_t *target_buffer,
                              ov_result *res) {

  snd_pcm_sframes_t err = snd_pcm_readi_wrapper(get_handle(self), target_buffer,
                                                get_samples_per_period(self));

  if (err < 1) {

    if (-EPIPE == err) {
      ov_result_set(res, OV_ERROR_AUDIO_IO, "Buffer underrun occured");
      ov_counter_increase(self->counter.underruns, 1);

    } else {

      ov_result_set(res, OV_ERROR_AUDIO_IO,
                    "Could not read from AUDIO interface");

      ov_log_error("Read from audio interface failed (%s)\n",
                   snd_strerror(err));
      ov_counter_increase(self->counter.other_error, 1);
    }

    ov_alsa_reset(self);
    return 0;

  } else {

    dump_pcm(self->dump_fh, target_buffer, get_samples_per_period(self));
    ov_counter_increase(self->counter.periods_read, 1);

    ov_result_set(res, OV_ERROR_NOERROR, 0);
    return (size_t)err;
  }
}

/*---------------------------------------------------------------------------*/

bool ov_alsa_drain_buffer(ov_alsa *self) {

  snd_pcm_t *apcm = get_handle(self);
  int err = 0;

  if (ov_ptr_valid(apcm, "Cannot drain ALSA buffer - invalid handle")) {

    if (0 == (err = snd_pcm_reset(apcm))) {
      return true;
    } else {

      ov_log_error("Cannot drain alsa buffer - %s",
                   ov_string_sanitize(snd_strerror(err)));
      return false;
    }

  } else {

    return false;
  }
}

/*---------------------------------------------------------------------------*/

void ov_alsa_enumerate_devices() {

  void **hints = {0};
  char *snd_iface_name;
  snd_iface_name = strdup("pcm");

  snd_device_name_hint(-1, snd_iface_name, &hints);

  void **str = hints;

  while (*str) {
    char *name = snd_device_name_get_hint(*str, "NAME");
    char *desc = snd_device_name_get_hint(*str, "DESC");
    char *io = snd_device_name_get_hint(*str, "IOID");
    printf("\n-- %s --\n", name);
    printf("IO: %s\n", io);
    char *desc_tmp = strtok(desc, "\n");
    while (desc_tmp != NULL) {
      printf("%s\n", desc_tmp);
      desc_tmp = strtok(NULL, "\n");
    }
    free(name);
    free(desc);
    free(io);
    str++;
  }

  snd_device_name_free_hint(hints);
}

/*----------------------------------------------------------------------------*/

char const *ov_alsa_state_to_string(ov_alsa const *self) {
  return pcm_state_as_string(get_handle(self));
}

/*----------------------------------------------------------------------------*/

bool ov_alsa_get_counters(ov_alsa const *self, ov_alsa_counters *counters) {

  if (ov_ptr_valid(self, "Cannot get counters - invalid ALSA objects") &&
      ov_ptr_valid(counters, "Cannot get counters - invalid counter object")) {

    *counters = self->counter;
    return true;

  } else {

    return false;
  }
}

#else

/*****************************************************************************
                               NON-LINUX dummies
 ****************************************************************************/

bool ov_alsa_enabled() { return true; }

/*----------------------------------------------------------------------------*/

ov_alsa *ov_alsa_create(char *device_name, unsigned int sample_rate,
                        size_t *buffer_size_frames) {

  UNUSED(device_name);
  UNUSED(sample_rate);
  UNUSED(buffer_size_frames);

  ov_log_error("ALSA only supported under Linux");
  return 0;
}

/*----------------------------------------------------------------------------*/

ov_alsa *ov_alsa_free(ov_alsa *self) { return self; }

/*----------------------------------------------------------------------------*/

size_t ov_alsa_get_buffer_size_samples(ov_alsa *self) {

  UNUSED(self);
  ov_log_error("ALSA only supported under Linux");
  return 0;
}

/*----------------------------------------------------------------------------*/

size_t ov_alsa_get_no_available_samples(ov_alsa *self) {

  UNUSED(self);
  ov_log_error("ALSA only supported under Linux");
  return 0;
}

/*----------------------------------------------------------------------------*/

int ov_alsa_play(ov_alsa *self, int16_t *buf, size_t sample_number) {

  UNUSED(self);
  UNUSED(buf);
  UNUSED(sample_number);

  ov_log_error("ALSA only supported under Linux");
  return 0;
}

/*----------------------------------------------------------------------------*/

int ov_alsa_get_poll_descriptors(ov_alsa *self, struct pollfd **target_array,
                                 size_t *num_fds) {

  UNUSED(self);
  UNUSED(target_array);
  UNUSED(num_fds);

  ov_log_error("ALSA only supported under Linux");
  return 0;
}

/*----------------------------------------------------------------------------*/

void ov_alsa_enumerate_devices() {
  ov_log_error("ALSA only supported under Linux");
}

/*---------------------------------------------------------------------------*/

char const *ov_alsa_state_to_string(ov_alsa const *self) {

  UNUSED(self);
  ov_log_error("ALSA only supported under Linux");

  return "ALSA only supported under Linux";
}

/*----------------------------------------------------------------------------*/

bool ov_alsa_get_counters(ov_alsa const *self, ov_alsa_counters *counters) {

  UNUSED(self);
  UNUSED(counters);

  ov_log_error("ALSA only supported under Linux");

  return false;
}

/*----------------------------------------------------------------------------*/

#endif
