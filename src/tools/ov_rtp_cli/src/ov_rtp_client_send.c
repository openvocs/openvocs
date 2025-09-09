/*
  (C) 2018 German Aerospace Center DLR e.V.,
  German Space Operations Center (GSOC)

  All rights reserved.

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
 */
/**
 * @author Michael J. Beer
 *
 */
#include <ov_base/ov_utils.h>
#include <stdlib.h>

#include <ov_base/ov_rtp_frame.h>

#include "ov_rtp_client.h"
#include "ov_rtp_client_send.h"
#include <ov_base/ov_rtcp.h>

/*----------------------------------------------------------------------------*/

#define max(a, b)                                                              \
    ({                                                                         \
        __typeof__(a) _a = (a);                                                \
        __typeof__(b) _b = (b);                                                \
        _a > _b ? _a : _b;                                                     \
    })

/*----------------------------------------------------------------------------*/

/**
 * Returns a jitter value in between +- params->max_jitter_usec
 * Takes care that the jitter is not that negative that the next timeout
 * is in the past.
 */
static int64_t get_jitter_usec(ov_rtp_client *client) {

    return 0;
    if (0 == client->send.max_jitter_usec) return 0;
    int64_t jitter = (RAND_MAX / 2 - random()) % client->send.max_jitter_usec;
    jitter = max(
        jitter, (int64_t)(client->audio.general_config.frame_length_usecs - 1));

    return jitter;
}

/*----------------------------------------------------------------------------*/

static ov_pcm_gen *pcm_gen_create(ov_pcm_gen_type type,
                                  ov_pcm_gen_config config,
                                  void *additional) {

    if (OV_PULSEAUDIO == type) {

        return ov_pcm_gen_pulse_create(config, additional);
    }

    return ov_pcm_gen_create(type, config, additional);
}

/*----------------------------------------------------------------------------*/

static bool setup_pcm_generator(ov_rtp_client *client,
                                ov_rtp_client_parameters *cp,
                                ov_rtp_client_audio_parameters *ap) {

    OV_ASSERT(0 != client);
    OV_ASSERT(0 != cp);
    OV_ASSERT(0 != ap);

    void *additional = 0;
    switch (ap->send.type) {

        case OV_SINUSOIDS:
            additional = &ap->send.sinusoids;
            break;
        case OV_FROM_FILE:
            additional = &ap->send.file;
            break;
        case OV_PULSEAUDIO:
            additional = &ap->send.pulse;
            break;
        default:
            OV_ASSERT(!"MUST NEVER HAPPEN");
    };

    ov_pcm_gen_config pcm_gen_config = {
        .frame_length_usecs = ap->general_config.frame_length_usecs,
        .sample_rate_hertz = OV_DEFAULT_SAMPLERATE, // We need to generate 48k, the codec will downsample if required
    };

    client->send.pcm_generator =
        pcm_gen_create(ap->send.type, pcm_gen_config, additional);

    return true;
}

/*----------------------------------------------------------------------------*/

static bool trigger_send_next_frame(uint32_t id, void *userdata);

static void reschedule_send_timer(ov_rtp_client *restrict const client) {
    OV_ASSERT(0 != client);
    OV_ASSERT(SEND == client->mode);

    uint64_t next_timeout_usecs =
        client->audio.general_config.frame_length_usecs +
        get_jitter_usec(client);

    client->event->timer.set(client->event, next_timeout_usecs, client,
                             trigger_send_next_frame);
}

/*----------------------------------------------------------------------------*/

static bool send_to(int sd, const void *buf, size_t len,
                    const struct sockaddr *dest_addr, socklen_t addrlen) {
    int retval = sendto(sd, buf, len, 0, dest_addr, addrlen);

    if ((int)len != retval) {
        if ((0 > retval) && (22 == errno)) {
            fprintf(stderr,
                    "Could not send UDP: %s - did you specify the "
                    "correct "
                    "local interface using `-f`?\n",
                    strerror(errno));

        } else if (0 > retval) {
            fprintf(stderr, "Could not send UDP: %s\n", strerror(errno));

        } else {
            fprintf(stderr, "Could send only %i bytes out of %lu bytes\n",
                    retval, len);
        }

        return false;

    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/
static bool trigger_send_next_frame(uint32_t id, void *userdata) {
    UNUSED(id);

    ov_buffer *buffer = 0;
    ov_buffer *encoded = 0;
    ov_rtp_frame *frame = 0;

    uint64_t usecs = ov_time_get_current_time_usecs();

    if (!userdata) {
        fprintf(stderr, "No userdata received");
        goto error;
    }

    ov_rtp_client *client = userdata;

    if (0 == client) {
        ov_log_error("no client given (0 pointer)");
        goto error;
    }

    ov_codec *codec = client->codec;

    if (0 == codec) {
        fprintf(stderr, "No codec set - aborting\n");
        goto error;
    }

    reschedule_send_timer(client);

    const size_t num_bytes_to_send =
        client->audio.general_config.frame_length_usecs / 1000.0 *
        OV_DEFAULT_SAMPLERATE / 1000 * sizeof(int16_t);
    /* Get more PCM */

    ov_pcm_gen *pcm_gen = client->send.pcm_generator;
    OV_ASSERT(0 != pcm_gen);

    buffer = pcm_gen->generate_frame(pcm_gen);
    OV_ASSERT(0 != buffer);
    OV_ASSERT(0 < buffer->capacity);

    encoded = ov_buffer_create(num_bytes_to_send);

    OV_ASSERT(0 != encoded);
    OV_ASSERT(num_bytes_to_send <= encoded->capacity);

    int encoded_length =
        ov_codec_encode(codec, buffer->start, buffer->length,
                        (uint8_t *)encoded->start, encoded->capacity);

    buffer = ov_buffer_free(buffer);

    if (0 > encoded_length) {
        fprintf(stderr, "Could NOT ENCODE pcm data\n");
        goto error;
    }

    ov_rtp_frame_expansion exp = {

        .version = RTP_VERSION_2,
        .ssrc = client->send.ssrc_id,
        .payload_type = client->send.payload_type,
        .sequence_number = client->send.sequence_number++,
        .timestamp = usecs,
        .payload.data = (uint8_t *)encoded->start,
        .payload.length = encoded_length,
        // First frame needs marker bit in order to tell recv 'start of stream'
        .marker_bit = client->send.first_frame,

    };

    frame = ov_rtp_frame_encode(&exp);

    size_t send_repeats = 1;

    if (client->send.first_frame) {
        // Repeat first frame - if this is lost, the stream might never start
        // due to lack of the marker bit
        send_repeats = 5;
    }

    bool sent_properly = false;

    for (size_t i = 0; i < send_repeats; ++i) {
        sent_properly =
            send_to(client->udp_socket, frame->bytes.data, frame->bytes.length,
                    client->send.dest_sockaddr, client->send.dest_sockaddr_len);
    }

    client->send.first_frame = false;

    encoded = ov_buffer_free(encoded);

    if (!sent_properly) {
        fprintf(stderr, "Could not send  RTP\n");
        goto error;
    }

    if ((0 != client->send.sdes) && (500 < ++client->send.sdes_count)) {
        fprintf(stderr, "Sending RTCP...\n");

        client->send.sdes_count = 0;

        ov_rtcp_message *msg =
            ov_rtcp_message_sdes(client->send.sdes, client->send.ssrc_id);
        ov_buffer *encoded = ov_rtcp_message_encode(msg);

        msg = ov_rtcp_message_free(msg);

        sent_properly =
            send_to(client->rtcp_socket, encoded->start, encoded->length,
                    client->send.rtcp_dest_sockaddr,
                    client->send.rtcp_dest_sockaddr_len);

        encoded = ov_buffer_free(encoded);

        if (!sent_properly) {
            fprintf(stderr, "Could not send  RTCP\n");
            goto error;
        }
    }

error:

    if (0 != buffer) {
        buffer = ov_buffer_free(buffer);
    }

    if (0 != encoded) {
        encoded = ov_buffer_free(encoded);
    }

    if (0 != frame) {
        frame = frame->free(frame);
    }

    return true;
}

/******************************************************************************
 *                              PUBLIC FUNCTIONS
 ******************************************************************************/

bool setup_sending_client(ov_rtp_client *client, ov_rtp_client_parameters *cp,
                          ov_rtp_client_audio_parameters *ap) {
    if (0 == client) {
        ov_log_error("Not client (0 pointer) given");
        goto error;
    }

    if (0 == client->event) {
        ov_log_error("Client got no event loop attached");
        goto error;
    }

    if (0 == cp) {
        ov_log_error("No client params (0 pointer) given");
        goto error;
    }

    if (0 == ap) {
        ov_log_error("No audio params (0 pointer) given");
        goto error;
    }

    /* Set up PCM source */

    if (!setup_pcm_generator(client, cp, ap)) {
        ov_log_error("Could not set up PCM generator");
        goto error;
    }

    client->send.ssrc_id = cp->ssrc_id;
    client->send.payload_type = cp->payload_type;

    client->send.sdes = cp->sdes;

    uint64_t next_timeout_usecs =
        client->audio.general_config.frame_length_usecs +
        get_jitter_usec(client);

    client->send.first_frame = true;
    client->event->timer.set(client->event, next_timeout_usecs, client,
                             trigger_send_next_frame);

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

void shutdown_sending_client(ov_rtp_client *client) {
    OV_ASSERT(SEND == client->mode);

    if (0 != client->send.pcm_generator) {
        client->send.pcm_generator =
            client->send.pcm_generator->free(client->send.pcm_generator);
    }

    if (0 != client->send.dest_sockaddr) {
        free(client->send.dest_sockaddr);
    }

    client->send.dest_sockaddr = 0;
}
