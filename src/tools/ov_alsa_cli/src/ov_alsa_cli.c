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
#include <alsa/asoundlib.h>
#include <assert.h>
#include <getopt.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_utils.h>
#include <ov_os_linux/ov_alsa.h>
#include <wchar.h>

/*----------------------------------------------------------------------------*/

#define PANIC(...)                                                             \
  do {                                                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
    fprintf(stderr, "\n");                                                     \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

/*----------------------------------------------------------------------------*/

typedef struct {

  char const *device;
  char const *element;

  struct {
    bool try:
      1;
    bool list : 1;
    bool setvolume : 1;
    bool help : 1;
  };

  snd_pcm_access_t access_mode;
  snd_pcm_stream_t stream_mode;

  size_t num_channels;

  bool nonblock;

  double volume;

} options;

/*----------------------------------------------------------------------------*/

ov_alsa_mode stream_mode_to_alsa_mode(snd_pcm_stream_t mode) {

  switch (mode) {

  case SND_PCM_STREAM_PLAYBACK:
    return PLAYBACK;

  case SND_PCM_STREAM_CAPTURE:
    return CAPTURE;

  default:
    return PLAYBACK;
  };
}

/*----------------------------------------------------------------------------*/

static char const *access_mode_to_string(snd_pcm_access_t mode) {

  switch (mode) {

  case SND_PCM_ACCESS_MMAP_INTERLEAVED:
    return "MMAP_INTERLEAVED";

  case SND_PCM_ACCESS_MMAP_NONINTERLEAVED:
    return "MMAP_NONINTERLEAVED";

  case SND_PCM_ACCESS_MMAP_COMPLEX:
    return "MMAP_COMPLEX";

  case SND_PCM_ACCESS_RW_INTERLEAVED:
    return "RW_INTERLEAVED";

  case SND_PCM_ACCESS_RW_NONINTERLEAVED:
    return "RW_NONINTERLEAVED";

  default:
    return 0;
  };
}

/*----------------------------------------------------------------------------*/

static char const *stream_mode_to_string(snd_pcm_stream_t mode) {

  switch (mode) {

  case SND_PCM_STREAM_CAPTURE:
    return "capture";

  case SND_PCM_STREAM_PLAYBACK:
    return "playback";

  default:
    return 0;
  };
}

/*----------------------------------------------------------------------------*/

static void print_options(options opts) {

  printf("ALSA device: %s\n", ov_string_sanitize(opts.device));
  printf("ALSA access: %s\n", access_mode_to_string(opts.access_mode));
  printf("ALSA stream: %s\n", stream_mode_to_string(opts.stream_mode));
  printf("ALSA channels: %zu\n", opts.num_channels);
}

/*----------------------------------------------------------------------------*/

static snd_pcm_access_t access_mode_from_string(char const *str) {

  if (ov_string_equal(str, "mi")) {
    return SND_PCM_ACCESS_MMAP_INTERLEAVED;
  } else if (ov_string_equal(str, "mn")) {
    return SND_PCM_ACCESS_MMAP_NONINTERLEAVED;
  } else if (ov_string_equal(str, "mc")) {
    return SND_PCM_ACCESS_MMAP_COMPLEX;
  } else if (ov_string_equal(str, "rwi")) {
    return SND_PCM_ACCESS_RW_INTERLEAVED;
  } else if (ov_string_equal(str, "rwn")) {
    return SND_PCM_ACCESS_RW_NONINTERLEAVED;
  } else {
    PANIC("Invalid access mode specified: %s.\nOnly 'mi', 'mn', 'mc', 'rwi', "
          "'rwn' allowed\n",
          ov_string_sanitize(str));
    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static snd_pcm_stream_t stream_mode_from_string(char const *str) {

  if (ov_string_equal(str, "capture")) {
    return SND_PCM_STREAM_CAPTURE;
  } else if (ov_string_equal(str, "playback")) {
    return SND_PCM_STREAM_PLAYBACK;
  } else {
    PANIC("Invalid stream mode '%s'\n", ov_string_sanitize(str));
  }
}

/*----------------------------------------------------------------------------*/

static void print_help(void) {

  fprintf(stderr, "\n"
                  "   Options that select behaviour:\n"
                  "\n"
                  "       -l               list all ALSA devices\n"
                  "       -t               try to establish connection to ALSA "
                  "device\n"
                  "       -v VOLUME(0 - 1) set volume\n"
                  "\n"
                  "   Options that select parameters\n"
                  "       -d ALSA_DEVICE\n"
                  "       -e ALSA_ELEMENT\n"
                  "       -c NUM_CHANNELS\n"
                  "       -n 'non-blocking mode'\n"
                  "       -a  mi|mn|mc|rwi|rwn\n"
                  "       -s  capture|playback\n");
}

/*----------------------------------------------------------------------------*/

static options get_options(int argc, char *const argv[]) {

  options opts = {
      .device = 0,
      .list = true,
      .access_mode = SND_PCM_ACCESS_RW_NONINTERLEAVED,
      .stream_mode = SND_PCM_STREAM_PLAYBACK,
      .num_channels = 1,
      .nonblock = false,

  };

  char *optstring = "hltv:d:e:a:s:nc:";

  int c = 0;

  while (-1 != (c = getopt(argc, argv, optstring))) {

    switch (c) {

    case 'h':
      opts.help = true;
      break;

    case 't':
      opts.try = true;
      opts.setvolume = false;
      opts.list = false;
      break;

    case 'v':
      opts.try = false;
      opts.setvolume = true;
      opts.list = false;
      opts.volume = strtod(optarg, 0);
      break;

    case 'd':
      opts.device = optarg;
      break;

    case 'e':
      opts.element = optarg;
      break;

    case 'l':
      opts.list = true;
      opts.setvolume = false;
      break;

    case 'a':
      opts.access_mode = access_mode_from_string(optarg);
      break;

    case 's':
      opts.stream_mode = stream_mode_from_string(optarg);
      break;

    case 'n':
      opts.nonblock = true;
      break;

    case 'c':
      // Bad conversion!!!
      opts.num_channels = atoi(optarg);

    default:
      break;
    };
  };

  return opts;
}

/*----------------------------------------------------------------------------*/

static bool list_alsa_devices() {

  ov_alsa_enumerate_devices();
  return true;
}

/*---------------------------------------------------------------------------*/

static bool alsa_call(int alsa_retval, char const *message) {

  if (alsa_retval < 0) {
    fprintf(stderr, "%s (%s)\n", OV_OR_DEFAULT(message, "Unknown ALSA error"),
            snd_strerror(alsa_retval));
    return false;
  } else {
    return true;
  }
}

/*----------------------------------------------------------------------------*/

static int snd_pcm_mode_from_opts(options opts) {

  //    if (SND_PCM_STREAM_CAPTURE == opts.stream_mode) {
  //        return SND_PCM_NONBLOCK;
  //    } else {
  //        return 0;
  //    }

  return opts.nonblock ? SND_PCM_NONBLOCK : 0;
}

/*----------------------------------------------------------------------------*/

static snd_pcm_t *snd_pcm_from_opts(options opts,
                                    snd_pcm_hw_params_t **params) {

  print_options(opts);

  unsigned int sample_rate = 48000;
  snd_pcm_t *apcm = 0;

  int err = -1;

  if ((err = snd_pcm_open(&apcm, opts.device, opts.stream_mode,
                          snd_pcm_mode_from_opts(opts))) < 0) {

    fprintf(stderr, "cannot open audio device %s (%s)\n", opts.device,
            snd_strerror(err));
    return 0;

  } else if (alsa_call(snd_pcm_hw_params_malloc(params),
                       "cannot allocate hardware parameter structure") &&

             alsa_call(snd_pcm_hw_params_any(apcm, *params),
                       "cannot initialize hardware parameter structure") &&

             // Seems to work: SND_PCM_ACCESS_RW_INTERLEAVED
             alsa_call(
                 snd_pcm_hw_params_set_access(apcm, *params, opts.access_mode),
                 "cannot set access type") &&

             alsa_call(snd_pcm_hw_params_set_channels(apcm, *params,
                                                      opts.num_channels),
                       "cannot set channel number") &&

             alsa_call(snd_pcm_hw_params_set_format(apcm, *params,
                                                    SND_PCM_FORMAT_S16_LE),
                       "cannot set sample format") &&

             alsa_call(snd_pcm_hw_params_set_rate_near(apcm, *params,
                                                       &sample_rate, 0),
                       "cannot set sample rate") &&

             // Set period size...

             alsa_call(snd_pcm_hw_params(apcm, *params),
                       "cannot set parameters")) {

    printf("Successfully connected\n");
    return apcm;

  } else {

    fprintf(stderr, "Connection failed\n");
    snd_pcm_close(apcm);
    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static void print_stream_info(snd_pcm_t *handle, snd_pcm_hw_params_t *params) {

  if (0 != handle) {

    printf("PCM handle name = '%s'\n", snd_pcm_name(handle));

    printf("PCM state = %s\n", snd_pcm_state_name(snd_pcm_state(handle)));

    snd_pcm_access_t access;
    snd_pcm_hw_params_get_access(params, &access);
    printf("access type = %s\n", snd_pcm_access_name(access));

    snd_pcm_format_t format;
    snd_pcm_hw_params_get_format(params, &format);
    printf("format = '%s' (%s)\n", snd_pcm_format_name(format),
           snd_pcm_format_description(format));

    snd_pcm_subformat_t subformat;
    snd_pcm_hw_params_get_subformat(params, &subformat);
    printf("subformat = '%s' (%s)\n", snd_pcm_subformat_name(subformat),
           snd_pcm_subformat_description(subformat));

    unsigned val = 0;
    snd_pcm_hw_params_get_channels(params, &val);
    printf("channels = %d\n", val);

    int dir = 0;
    snd_pcm_hw_params_get_rate(params, &val, &dir);
    printf("rate = %d bps\n", val);

    snd_pcm_hw_params_get_period_time(params, &val, &dir);
    printf("period time = %d us\n", val);

    snd_pcm_uframes_t frames;
    snd_pcm_hw_params_get_period_size(params, &frames, &dir);
    printf("period size = %" PRIu64 " frames\n", (uint64_t)frames);

    snd_pcm_hw_params_get_buffer_time(params, &val, &dir);
    printf("buffer time = %d us\n", val);

    snd_pcm_hw_params_get_buffer_size(params, &frames);
    printf("buffer size = %" PRIu64 " frames\n", (uint64_t)frames);

    snd_pcm_hw_params_get_periods(params, &val, &dir);
    printf("periods per buffer = %u frames\n", val);

    unsigned val2 = 0;
    snd_pcm_hw_params_get_rate_numden(params, &val, &val2);
    printf("exact rate = %u/%u bps\n", val, val2);

    val = snd_pcm_hw_params_get_sbits(params);
    printf("significant bits = %u\n", val);

    // snd_pcm_hw_params_get_tick_time(params, &val, &dir);
    // printf("tick time = %u us\n", val);

    val = snd_pcm_hw_params_is_batch(params);
    printf("is batch = %u\n", val);

    val = snd_pcm_hw_params_is_block_transfer(params);
    printf("is block transfer = %u\n", val);

    val = snd_pcm_hw_params_is_double(params);
    printf("is double = %u\n", val);

    val = snd_pcm_hw_params_is_half_duplex(params);
    printf("is half duplex = %u\n", val);

    val = snd_pcm_hw_params_is_joint_duplex(params);
    printf("is joint duplex = %u\n", val);

    val = snd_pcm_hw_params_can_overrange(params);
    printf("can overrange = %u\n", val);

    val = snd_pcm_hw_params_can_mmap_sample_resolution(params);
    printf("can mmap = %u\n", val);

    val = snd_pcm_hw_params_can_pause(params);
    printf("can pause = %u\n", val);

    val = snd_pcm_hw_params_can_resume(params);
    printf("can resume = %u\n", val);

    val = snd_pcm_hw_params_can_sync_start(params);
    printf("can sync start = %u\n", val);

  } else {
    printf("Invalid handle\n");
  }
}

/*----------------------------------------------------------------------------*/

static void snd_pcm_close_wrapper(snd_pcm_t *apcm) {

  if (0 != apcm) {
    snd_pcm_close(apcm);
  }
}

/*----------------------------------------------------------------------------*/

static bool print_available_frames(snd_pcm_t *apcm) {

  if (ov_ptr_valid(apcm,
                   "cannot read available number of frames - invalid alsa "
                   "handle")) {

    printf("Available frames by ALSA device: %" PRIi64 "(avail) / %" PRIi64
           " (update)\n",
           (int64_t)snd_pcm_avail(apcm), (int64_t)snd_pcm_avail_update(apcm));
    return true;

  } else {
    return false;
  }
}

/*----------------------------------------------------------------------------*/

static void print_alsa_state(int state) {

  printf("ALSA state: %i ;", state);

  if (0 > state) {
    printf("Failed: %s\n", snd_strerror(state));
  } else {
    printf("Samples: %i\n", state);
  }
}

/*----------------------------------------------------------------------------*/

static bool try_io_on_snd_pcm_with_buffer(snd_pcm_t *apcm,
                                          snd_pcm_stream_t mode,
                                          snd_pcm_uframes_t no_samples,
                                          int16_t *buffer) {

  snd_pcm_sframes_t no_frames;

  if (0 == apcm) {

    fprintf(stderr, "Cannot access ALSA: Invalid handle\n");
    return false;

  } else if (0 == buffer) {

    fprintf(stderr, "Cannot access ALSA: No buffer to use\n");
    return false;

  } else {

    switch (mode) {

    case SND_PCM_STREAM_PLAYBACK:

      no_frames = snd_pcm_writen(apcm, (void **)&buffer, no_samples);
      printf("Tried to write %zu frames. ", no_samples);
      print_alsa_state(no_frames);
      return true;

    case SND_PCM_STREAM_CAPTURE:

      no_frames = snd_pcm_readi(apcm, (void *)buffer, no_samples);
      printf("Tried to read %zu frames using readi ", no_samples);
      print_alsa_state(no_frames);

      no_frames = snd_pcm_readn(apcm, (void *)buffer, no_samples);
      printf("Tried to read %zu frames using readn ", no_samples);
      print_alsa_state(no_frames);
      return true;

    default:
      fprintf(stderr, "Could not try IO on ALSA device: Invalid mode");
      return false;
    };
  }
}

/*----------------------------------------------------------------------------*/

static bool try_io_on_snd_pcm(snd_pcm_t *apcm, snd_pcm_stream_t mode,
                              size_t no_channels,
                              snd_pcm_uframes_t no_samples) {

  int16_t *buffer = malloc(no_samples * 2 * no_channels);
  memset(buffer, 0, sizeof(*buffer));

  bool ok = try_io_on_snd_pcm_with_buffer(apcm, mode, no_samples, buffer);

  free(buffer);

  return ok;
}

/*----------------------------------------------------------------------------*/

static int bool_to_exit_state(bool ok) {

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*----------------------------------------------------------------------------*/

static snd_pcm_hw_params_t *
snd_pcm_hw_params_free_wrapper(snd_pcm_hw_params_t *params) {

  if (0 != params) {

    snd_pcm_hw_params_free(params);
  }

  return 0;
}

/*----------------------------------------------------------------------------*/

static snd_pcm_uframes_t
get_period_length_samples(snd_pcm_hw_params_t *params) {

  snd_pcm_uframes_t samples = 0;
  int dir = 0;
  snd_pcm_hw_params_get_period_size(params, &samples, &dir);
  fprintf(stdout, "One period has %lu samples\n", samples);
  return samples;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char **argv) {

  options opts = get_options(argc, argv);

  if (opts.help) {

    print_help();
    return EXIT_SUCCESS;

  } else if (opts.list) {
    return bool_to_exit_state(list_alsa_devices());

  } else if (opts.setvolume) {
    return bool_to_exit_state(ov_alsa_set_volume(
        opts.device, opts.element, stream_mode_to_alsa_mode(opts.stream_mode),
        opts.volume));
  } else {

    bool ok = true;

    snd_pcm_hw_params_t *params = 0;

    snd_pcm_t *apcm = snd_pcm_from_opts(opts, &params);
    snd_pcm_uframes_t period_length_samples = get_period_length_samples(params);

    print_stream_info(apcm, params);

    params = snd_pcm_hw_params_free_wrapper(params);

    ok = (0 != apcm);
    ok = ok && print_available_frames(apcm);
    ok = ok && try_io_on_snd_pcm(apcm, opts.stream_mode, opts.num_channels,
                                 period_length_samples);

    snd_pcm_close_wrapper(apcm);

    return bool_to_exit_state(ok);
  }
}
