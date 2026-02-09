/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

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

        @author         Michael J. Beer

        Global compile time constants.

        Value of OV_CONSTANT might be overwritten by
        -DOV_CONSTANT compiler flag like
        -DOV_UDP_PAYLOAD_OCTETS=512 to set max supported UDP payload length to 512 octets

        ------------------------------------------------------------------------
*/
#ifndef OV_CONSTANTS_H
#define OV_CONSTANTS_H

#include <stdint.h>

/*****************************************************************************
                                     DEBUG
 ****************************************************************************/

#ifdef DEBUG

#define OV_SIP_APP_DEBUG

#endif

/*****************************************************************************
                                  DEFAULTS ETC
 ****************************************************************************/

#ifndef OV_MAX_FRAME_LENGTH_MS

#define OV_MAX_FRAME_LENGTH_MS 20

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_FRAME_LENGTH_MS

#define OV_DEFAULT_FRAME_LENGTH_MS 20

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_MAX_CSRCS

#define OV_MAX_CSRCS 15

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_UDP_PAYLOAD_OCTETS

/* RFC  1122 requires a MINIMUM of 576 octets for the EMTU_R.
 * 576 minus the UDP header is the MINIMUM supported payload size.
 * However, over ethernet, the MTU is up to 1500 (dep. on the flavor),
 * thus over ethernet, we might encounter UDP datagrams of of up to 1500 octets.
 * Let's play it safe and assume 2k = 2048
 */
#define OV_UDP_PAYLOAD_OCTETS 2048

#endif

// #define OV_DISABLE_CACHING 1

/*----------------------------------------------------------------------------*/

/* You might also define `OV_ARCH` to force compilation for a specific
 * architecture like
 *
 * -DOV_ARCH=2 to enforce Linux
 *
 *  See ov_arch/ov_arch.h for possible options
 */

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_CACHE_SIZE

#define OV_DEFAULT_CACHE_SIZE 50

#endif

/*----------------------------------------------------------------------------*/

/**
 * Maximum length of a hostname
 * Under Linux, this would be the constant OV_HOST_NAME_MAX,
 * under BSDs, it would be _POSIX_OV_HOST_NAME_MAX.
 *
 * The maximum allowed hostname, in general, is 255 however.
 */
#ifndef OV_HOST_NAME_MAX

#define OV_HOST_NAME_MAX 255

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_USER

#define OV_DEFAULT_USER "unspecified"

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_RECORDER_REPOSITORY_ROOT_PATH

#define OV_DEFAULT_RECORDER_REPOSITORY_ROOT_PATH "/tmp"

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_NUM_THREADS

#define OV_DEFAULT_NUM_THREADS 4

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_LOCK_TIMEOUT_MSECS

#define OV_DEFAULT_LOCK_TIMEOUT_MSECS 500

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_REQUEST_TIMEOUT_MSECS

#define OV_DEFAULT_REQUEST_TIMEOUT_MSECS (3 * 1000)

#endif

/*----------------------------------------------------------------------------*/

/**
 * How many signaling requests are supported in parallel?
 * Used at least within SIP gateway to communicating with resource manager
 */
#ifndef OV_MAX_NUM_ACTIVE_REQUESTS

#define OV_MAX_NUM_ACTIVE_REQUESTS 50

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_RECONNECT_INTERVAL_SECS

#define OV_DEFAULT_RECONNECT_INTERVAL_SECS 10

#endif

/*----------------------------------------------------------------------------*/

/**
 * Maximum number of supported translations in a translation table,
 * i.e. maximum number of sources and destinations supported by a minion.
 */
#ifndef OV_DEFAULT_MAX_NUM_TRANSLATIONS

#define OV_DEFAULT_MAX_NUM_TRANSLATIONS OV_MAX_CSRCS

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_MAX_TALKERS_PER_LOOP

#define OV_MAX_TALKERS_PER_LOOP OV_MAX_CSRCS

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_MAX_LISTENERS_PER_LOOP

#define OV_MAX_LISTENERS_PER_LOOP OV_MAX_CSRCS

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_MAX_RECORDINGS_PER_LOOP

#define OV_MAX_RECORDINGS_PER_LOOP OV_MAX_CSRCS

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_MAX_PLAYBACKS_PER_LOOP

#define OV_MAX_PLAYBACKS_PER_LOOP OV_MAX_CSRCS

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_SSID

#define OV_DEFAULT_SSID 123456789

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_EC_FRAMES_PER_SSID

#define OV_DEFAULT_EC_FRAMES_PER_SSID 3

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_CONFIG_DIRECTORY

#define OV_DEFAULT_CONFIG_DIRECTORY "/etc/openvocs"

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_LOG_DIRECTORY

#define OV_DEFAULT_LOG_DIRECTORY "/tmp"

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_LOG_LEVEL_STRING

#define OV_DEFAULT_LOG_LEVEL_STRING "info"

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_LOG_MAX_NUM_FILES

#define OV_DEFAULT_LOG_MAX_NUM_FILES 5

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_LOG_MESSAGES_PER_FILE

#define OV_DEFAULT_LOG_MESSAGES_PER_FILE 10000

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_PATH_DELIMITER

#define OV_PATH_DELIMITER '/'

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_MAX_SAMPLERATE_HZ

#define OV_MAX_SAMPLERATE_HZ 48000

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_SAMPLERATE

#define OV_DEFAULT_SAMPLERATE 48000

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_CODEC

#define OV_DEFAULT_CODEC "opus"

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_COMFORT_NOISE_LEVEL_DB

#define OV_DEFAULT_COMFORT_NOISE_LEVEL_DB ((int16_t) - 50.0)

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_NORMALIZE_INPUT

#define OV_DEFAULT_NORMALIZE_INPUT true

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_RTP_PORT

#define OV_DEFAULT_RTP_PORT 12345

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_KEEPALIVE_RTP_FRAMES

#define OV_DEFAULT_KEEPALIVE_RTP_FRAMES true

#endif

/*----------------------------------------------------------------------------*/

/**
 * The first frame of an RTP stream ought to be marked to indicate start of
 * stream.
 * If the first (marked) frame is lost, the stream will never be considered
 * 'established'.
 * Hence, the first frame of an RTP stream is repeated several times.
 * This constant defines how often it should be repeated.
 */
#ifndef OV_DEFAULT_RTP_REPETITIONS_START_FRAME

#define OV_DEFAULT_RTP_REPETITIONS_START_FRAME 5

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_OCTETS_PER_SAMPLE

#define OV_DEFAULT_OCTETS_PER_SAMPLE 2

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_PAYLOAD_TYPE

#define OV_DEFAULT_PAYLOAD_TYPE 96

#endif

/*----------------------------------------------------------------------------*/

/**
 * Max difference for two floating points to still be considered equal
 */
#ifndef OV_MAX_FLOAT_DELTA

#define OV_MAX_FLOAT_DELTA 0.0000000000001

#endif

/*----------------------------------------------------------------------------*/

/**
 * MAX volume value in the backend
 */
#define OV_MAX_VOLUME UINT16_MAX

/*----------------------------------------------------------------------------*/

/**
 * For noise detection
 */
#ifndef OV_DEFAULT_POWERLEVEL_DENSITY_THRESHOLD_DB

#define OV_DEFAULT_POWERLEVEL_DENSITY_THRESHOLD_DB ((double)-50.0)

#endif

/**
 * For noise detection
 */
#ifndef OV_DEFAULT_ZERO_CROSSINGS_RATE_THRESHOLD_HZ

// Voice does not contain frequencies higher than 10kHz, thus normal voice
// cannot contain more than 10 zero crossings per 1msec.
// the unit is 'per sample'
#define OV_DEFAULT_ZERO_CROSSINGS_RATE_THRESHOLD_HZ 10000

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_MAX_BACKTRACE_DEPTH

#define OV_MAX_BACKTRACE_DEPTH 300

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_VM_TIMER_INTERVAL_USECS

#define OV_DEFAULT_VM_TIMER_INTERVAL_USECS (1 * 1000 * 1000)

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_SIP_NUM_HEADER

#define OV_DEFAULT_SIP_NUM_HEADER 15

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_DEFAULT_SIP_CSEQ

#define OV_DEFAULT_SIP_CSEQ 10

#endif

/*----------------------------------------------------------------------------*/

#ifndef OV_MAX_ANALOG_SSRC

#define OV_MAX_ANALOG_SSRC 66000

#endif

#ifndef OV_DEFAULT_CUTOFF_AFTER_FRAMES

/**
 * If Voice detection is active, determines after how many frames of no
 * voice having been detected the voice should be considered gone.
 * Used e.g. by the recorder to stop a recording if VAD is active.
 */
#define OV_DEFAULT_CUTOFF_AFTER_FRAMES 100

#endif

/*****************************************************************************
 *                           DERIVED CONSTANTS
 ****************************************************************************/

#define OV_DEFAULT_LOCK_TIMEOUT_USECS (1000 * OV_DEFAULT_LOCK_TIMEOUT_MSECS)

#define OV_DEFAULT_LOG_LEVEL                                                   \
    ov_log_level_from_string(OV_DEFAULT_LOG_LEVEL_STRING)

/*----------------------------------------------------------------------------*/

#define OV_MAX_FRAME_LENGTH_SAMPLES                                            \
    (OV_MAX_SAMPLERATE_HZ * OV_MAX_FRAME_LENGTH_MS / 1000)

#define OV_MAX_FRAME_LENGTH_BYTES                                              \
    (OV_MAX_FRAME_LENGTH_SAMPLES * sizeof(int16_t))

#define OV_DEFAULT_FRAME_LENGTH_SAMPLES                                        \
    (OV_DEFAULT_SAMPLERATE * OV_DEFAULT_FRAME_LENGTH_MS / 1000)

/*----------------------------------------------------------------------------*/

#define OV_INTERNAL_CODEC                                                      \
    "{"                                                                        \
    "\"sample_rate_hz\": " TO_STR(OV_DEFAULT_SAMPLERATE) ","                   \
                                                         "\"codec\": "         \
                                                         "\"" OV_DEFAULT_CODEC \
                                                         "\""                  \
                                                         "}"

/*----------------------------------------------------------------------------*/
#endif
