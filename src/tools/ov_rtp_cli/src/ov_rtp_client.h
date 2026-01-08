/***

  Copyright   2018    German Aerospace Center DLR e.V., German Space Operations
Center (GSOC)

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

This file is part of the openvocs project. http://openvocs.org

 ***/
/**
 * @author Michael J. Beer
 */
/*----------------------------------------------------------------------------*/
#ifndef OV_RTP_CLIENT_H
#define OV_RTP_CLIENT_H
/*----------------------------------------------------------------------------*/

#include <stdint.h>

#include <ov_base/ov_cache.h>
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_rtp_frame.h>

#include <ov_codec/ov_codec.h>
#include <ov_pcm_gen/ov_pcm_gen.h>

#include "ov_pcm_gen_pulse.h"
#include "ov_pulse_context.h"

/*----------------------------------------------------------------------------*/

typedef struct ov_rtp_client ov_rtp_client;

/*----------------------------------------------------------------------------*/

typedef enum operation_mode {

  SEND,
  RECEIVE

} operation_mode;

/*---------------------------------------------------------------------------*/

/**
 * Defines the general setup of the client, i.e. its ops mode (send or receive)
 * alon with the network setup (where to listen/where to send to).
 * This struct is meant to contain immutable stuff only!
 */
typedef struct ov_rtp_client_parameters {

  operation_mode mode;

  bool debug : 1;

  char const *multicast_group;

  char const *local_if;
  uint16_t local_port;
  char const *remote_if;
  uint16_t remote_port;

  int64_t max_jitter_usec;
  uint32_t ssrc_id;

  uint8_t payload_type;

  uint16_t sequence_number;

  char const *sdes;

} ov_rtp_client_parameters;

/*----------------------------------------------------------------------------*/

/**
 * Defines how audio shall be processed by the client.
 * This struct is meant to contain immutable stuff only!
 */
typedef struct ov_rtp_client_audio_parameters {

  ov_pcm_gen_config general_config;

  struct {
    struct {
      int type;
      union {
        ov_pcm_gen_sinusoids sinusoids;
        ov_pcm_gen_from_file file;
        ov_pcm_gen_pulse pulse;
      };
    } send;

    struct {

      char const *file_name; /* if 0, use pa_server */
      char const *pa_server; /* if 0, use default server */
    } receive;
  };
  char const *codec_name;

} ov_rtp_client_audio_parameters;

/*---------------------------------------------------------------------------*/

struct ov_rtp_client {

  operation_mode mode;

  union {
    struct send_t {
      struct sockaddr *dest_sockaddr;
      socklen_t dest_sockaddr_len;

      ov_pcm_gen *pcm_generator;

      int64_t max_jitter_usec;
      uint32_t ssrc_id;
      uint8_t payload_type;
      uint16_t sequence_number;

      struct sockaddr *rtcp_dest_sockaddr;
      socklen_t rtcp_dest_sockaddr_len;

      bool first_frame;

      size_t sdes_count;

      char const *sdes;

    } send;

    struct receive_t {

      struct {
        int fd;
        ov_buffer *encode_buffer;
        ov_codec *codec;
      } file;

      ov_pulse_context *pulse_context; /* If 0, out to file */

      /**
       * Processed PCM data received via RTP.
       * @return number of samples in buffer.
       */
      size_t (*output_pcm)(ov_rtp_client *, uint8_t *pcm, size_t length_bytes);
      uint16_t last_sequence_number;
    } receive;
  };

  struct {
    bool debug : 1;
  };

  ov_rtp_client_audio_parameters audio;

  int udp_socket;
  int rtcp_socket;

  ov_codec *codec; /* Codec for encoding/decoding RTP payload */

  ov_event_loop *event;

  bool (*io_handler)(int socket_fd, uint8_t events, void *userdata);
};

/*----------------------------------------------------------------------------*/

ov_rtp_client *
ov_rtp_client_create(ov_rtp_client_parameters *client_params,
                     ov_rtp_client_audio_parameters *audio_params);

/*----------------------------------------------------------------------------*/

void ov_rtp_client_run(ov_rtp_client *client);

/*----------------------------------------------------------------------------*/

ov_rtp_client *ov_rtp_client_free(ov_rtp_client *client);

/*----------------------------------------------------------------------------*/

char const *ov_operation_mode_to_string(operation_mode mode);

/*----------------------------------------------------------------------------*/

void ov_rtp_client_parameters_print(
    FILE *out, ov_rtp_client_parameters const *client_params,
    size_t indentation_level);

/*----------------------------------------------------------------------------*/

void ov_rtp_client_audio_parameters_print(
    FILE *out, ov_rtp_client_audio_parameters const *params,
    size_t indentation_level);

/*----------------------------------------------------------------------------*/

void ov_rtp_client_print(FILE *out, ov_rtp_client const *client,
                         size_t indentation_level);

/*----------------------------------------------------------------------------*/

#endif
