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

        All PCM is single channel, 16bit LE (signle channel i.e. interleaved or
        not interleaved does not matter any more).
        The sample rate is 48kHz.

        If you want to use alsa, you will encounter some problems on modern
        linux systems:
        Access to devices seems to be exclusive, and there is pulseaudio or
        pipewire or something similar running.

        However, pulseaudio/pipewire seem to provide a default ALSA device
        which actually redirects to pulseaudio/pipewire.

        It's most likely the 'default' device.

        You might use it for testing, but be aware, that using ALSA to access
        pulseaudio is bad, if you use ALSA in order to bypass pulseaudio
        latency ...

        If you want to use ALSA directly, I'd advise to stop pulseaudio.
        Ensure that there is no pulse/pipewire running on production devices.


        Usage:

        Is straight forward:

        1. Create a handle for a device using 'ov_alsa_create()'

        2. read/write to it using 'ov_alsa_play' or 'ov_alsa_read_period'

        3. Free handle using 'ov_alsa_free()'

        There are a few tricky bits:

        Transfer is done in chunks of a particular size.
        'ov_alsa_read_period' will always return a fixed number of samples/octets.
        This number is called samples_per_period.

        You can request a particular `samples_per_period`, but as not all
        hardware supports any value, you might get a value more or less close to
        your desired value:

        size_t samples_per_period = 1024;

        ov_alsa alsa = ov_alsa_create(
                             "plughw:0,0", 48000, &samples_per_period, RECORD);

        int16_t *capture_buffer = calloc(1, ov_alsa_samples_per_period(1024));

        ov_result res = {0};
        ov_alsa_read_period(alsa, capture_buffer, &res);

        ------------------------------------------------------------------------
*/
#ifndef OV_ALSA_H
#define OV_ALSA_H
/*----------------------------------------------------------------------------*/

#include <ov_base/ov_counter.h>
#include <ov_base/ov_result.h>
#include <poll.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*----------------------------------------------------------------------------*/

bool ov_alsa_enabled();

/*----------------------------------------------------------------------------*/

struct ov_alsa_struct;
typedef struct ov_alsa_struct ov_alsa;

typedef enum {
    CAPTURE,
    PLAYBACK,
} ov_alsa_mode;

/*----------------------------------------------------------------------------*/

/**
 * @param device_name is the device number without 'hw:' - card 2 is just '2'.
 * You might pass in device,subdevice like '2,0', in which case the subdevice is
 * cut off to '2'.
 * @param volume needs to be in range of [0,1]
 */
bool ov_alsa_set_volume(char const *device_name, char const *element,
                        ov_alsa_mode mode, double volume);

/*----------------------------------------------------------------------------*/

/**
 * Create an ALSA object to replay/capture audio from devices.
 * @param device_name ALSA - name of the device, usually something like "hw:0,0"
 * @param sample_rate Not every sample rate is supported. The call will fail
 * then
 * @param samples_per_period The number of samples you want to read/write per
 * call. Is set to the actually used number.
 * @param mode either PLAYBACK/CAPTURE
 */
ov_alsa *ov_alsa_create(char const *device_name, unsigned int sample_rate,
                        size_t *samples_per_period, ov_alsa_mode mode);

/*----------------------------------------------------------------------------*/

ov_alsa *ov_alsa_free(ov_alsa *self);

/*----------------------------------------------------------------------------*/

// TODO: Remove
size_t ov_alsa_get_buffer_size_samples(ov_alsa *self);

/*----------------------------------------------------------------------------*/

size_t ov_alsa_get_samples_per_period(ov_alsa *self);

size_t ov_alsa_samples_to_octets(ov_alsa *self, size_t num_samples);

/*----------------------------------------------------------------------------*/

bool ov_alsa_reset(ov_alsa *self);

/*----------------------------------------------------------------------------*/

ssize_t ov_alsa_get_no_available_samples(ov_alsa *self);

/*----------------------------------------------------------------------------*/

/**
 * Play one period worth of samples from target_buffer.
 * (The samples within one period have been given by you when you called
 * ov_alsa_create(). You can retrieve the actual number by calling
 * ov_alsa_get_samples_per_period().
 * `target_buffer` must contain 2 * ov_alsa_get_samples_per_period() big;
 */
bool ov_alsa_play_period(ov_alsa *self, int16_t *buf, ov_result *res);

/*----------------------------------------------------------------------------*/

/**
 * Reads one period worth of samples int target_buffer.
 * (The samples within one period have been given by you when you called
 * ov_alsa_create(). You can retrieve the actual number by calling
 * ov_alsa_get_samples_per_period().
 * `target_buffer` must be at least 2 * ov_alsa_get_samples_per_period() big;
 *
 * @return number of SAMPLES captured
 */
size_t ov_alsa_capture_period(ov_alsa *self, int16_t *target_buffer,
                              ov_result *res);

/*----------------------------------------------------------------------------*/

bool ov_alsa_drain_buffer(ov_alsa *self);

/*----------------------------------------------------------------------------*/

void ov_alsa_enumerate_devices();

/*----------------------------------------------------------------------------*/

char const *ov_alsa_state_to_string(ov_alsa const *self);

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_counter underruns;
    ov_counter other_error;
    ov_counter periods_played;
    ov_counter periods_read;
    ov_counter periods_written;

} ov_alsa_counters;

bool ov_alsa_get_counters(ov_alsa const *self, ov_alsa_counters *counters);

/*----------------------------------------------------------------------------*/
#endif
