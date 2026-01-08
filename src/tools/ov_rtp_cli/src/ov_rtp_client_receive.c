/*
 * (C) 2018 German Aerospace Center DLR e.V.,
 * German Space Operations Center (GSOC)
 *
 * All rights reserved.
 *
 * Redistribution  and use in source and binary forms, with or with‐
 * out modification, are permitted provided that the following  con‐
 * ditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above  copy‐
 * right  notice,  this  list  of  conditions and the following dis‐
 * claimer in the documentation and/or other materials provided with
 * the distribution.
 *
 * 3.  Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote  products  derived
 * from this software without specific prior written permission.
 *
 * THIS  SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBU‐
 * TORS "AS IS" AND ANY EXPRESS OR  IMPLIED  WARRANTIES,  INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE  ARE  DISCLAIMED.  IN  NO  EVENT
 * SHALL  THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DI‐
 * RECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR  CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS IN‐
 * TERRUPTION)  HOWEVER  CAUSED  AND  ON  ANY  THEORY  OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING  NEGLI‐
 * GENCE  OR  OTHERWISE)  ARISING  IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * @author Michael J. Beer
 */

/*----------------------------------------------------------------------------*/

#include <fcntl.h>
#include <ov_base/ov_utils.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <ov_base/ov_rtp_frame.h>
#include <ov_codec/ov_codec_factory.h>
#include <ov_codec/ov_codec_pcm16_signed.h>

#include "ov_rtp_client_receive.h"

/******************************************************************************
 *                                IO CALLBACKS
 ******************************************************************************/

static bool receive_udp_handler(int fd, uint8_t events, void *userdata) {

    UNUSED(events);

    ov_buffer *buffer = 0;
    ov_rtp_frame *frame = 0;

    ov_rtp_client *client = userdata;

    OV_ASSERT(0 != client);
    OV_ASSERT(RECEIVE == client->mode);

    ov_codec *codec = client->codec;

    if (0 == codec) {

        fprintf(stderr, "No codec set - aborting\n");
        goto error;
    }

    int sz = 0;

    if (ioctl(fd, FIONREAD, &sz) < 0)
        goto error;

    if (0 >= sz)
        goto error;

    buffer = ov_buffer_create(sz);

    OV_ASSERT(0 != buffer);
    OV_ASSERT((size_t)sz <= buffer->capacity);

    ssize_t received = recv(fd, buffer->start, sz, 0);

    if (0 > received) {

        fprintf(stderr, "Failed during reading from socket: %s",
                strerror(errno));
        goto error;
    }

    buffer->length = (size_t)received;
    /* Does not matter if `frame` is 0, decode() will allocate a new one
     * if 0 */
    frame = ov_rtp_frame_decode(buffer->start, buffer->length);

    if (!frame) {

        fprintf(stderr, "Received NON-RTP");
        goto error;

    } else if (client->debug) {

        fprintf(stdout, "Received RTP frame\n");
    }

    ov_rtp_frame_expansion *exp = (ov_rtp_frame_expansion *)frame;

    if (1 + client->receive.last_sequence_number != exp->sequence_number) {
        fprintf(stderr,
                "Frame drop encountered, expected %" PRIu32 "   got %" PRIu32
                "\n",
                1 + client->receive.last_sequence_number, exp->sequence_number);
    }

    client->receive.last_sequence_number = exp->sequence_number;

    const size_t num_bytes_expected =
        client->audio.general_config.frame_length_usecs / 1000.0 *
        OV_DEFAULT_SAMPLERATE / 1000.0 * sizeof(int16_t);

    if (num_bytes_expected < exp->payload.length) {
        ov_log_warning("Received more octets than we are able to handle"
                       "   Expected %zu,  received %zu",
                       num_bytes_expected, exp->payload.length);
    }

    /* The buffer start - buffer  has been coerced into frame,
     * don't free */
    buffer->start = calloc(1, num_bytes_expected);
    buffer->capacity = num_bytes_expected;

    ssize_t bytes_to_write =
        ov_codec_decode(codec, exp->sequence_number, exp->payload.data,
                        exp->payload.length, buffer->start, buffer->capacity);

    /* DONT FREE - its an alias to 'frame' only */
    exp = 0;

    buffer->length = bytes_to_write;

    ssize_t written =
        client->receive.output_pcm(client, buffer->start, bytes_to_write);

    if (bytes_to_write != written) {
        fprintf(stderr, "Could not write entire data to target: %s\n",
                strerror(errno));
    }

error:

    if (frame) {
        frame = frame->free(frame);
    }

    if (buffer) {
        buffer = ov_buffer_free(buffer);
    }

    OV_ASSERT(0 == buffer);
    OV_ASSERT(0 == frame);

    /* We want to continue reading nevertheless! */
    return 0;
}

/*----------------------------------------------------------------------------*/

static size_t output_pulse(ov_rtp_client *client, uint8_t *pcm,
                           size_t length_bytes) {
    OV_ASSERT(0 != client);
    if (0 == pcm)
        goto error;
    if (0 == length_bytes)
        goto error;

    ov_buffer buffer = {
        .start = pcm, .length = length_bytes, .capacity = length_bytes};

    if (ov_pulse_write(client->receive.pulse_context, &buffer)) {
        return length_bytes;
    }

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

static size_t output_to_file(ov_rtp_client *client, uint8_t *pcm,
                             size_t length_bytes) {
    ov_buffer *buffer = 0;
    size_t bytes_written = 0;

    OV_ASSERT(0 != client);
    OV_ASSERT(-1 < client->receive.file.fd);

    ov_codec *codec = client->receive.file.codec;

    OV_ASSERT(0 != codec);

    buffer = ov_buffer_create(length_bytes);

    OV_ASSERT(0 != buffer);
    OV_ASSERT(length_bytes <= buffer->capacity);

    buffer->length = ov_codec_encode(codec, pcm, length_bytes, buffer->start,
                                     buffer->capacity);

    if (0 == buffer->length) {
        ov_log_error("Could not encode received pcm");
        goto error;
    }

    bytes_written =
        write(client->receive.file.fd, buffer->start, buffer->length);

error:

    if (0 != buffer) {
        buffer = ov_buffer_free(buffer);
    }

    OV_ASSERT(0 == buffer);

    return bytes_written;
}

/******************************************************************************
 *                              SETUP FUNCTIONS
 ******************************************************************************/

bool setup_write_to_file(ov_rtp_client *client) {
    int fd = -1;

    OV_ASSERT(0 != client);
    OV_ASSERT(RECEIVE == client->mode);

    char const *file_name = client->audio.receive.file_name;

    if (0 == file_name) {
        ov_log_error("no filename given");
        goto error;
    }

    fd = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if (0 > fd) {
        ov_log_error("Could not open file: %s", strerror(errno));
        goto error;
    }

    client->receive.file.fd = fd;
    client->receive.output_pcm = output_to_file;

    ov_json_value *codec_parameters = ov_json_object();

    ov_codec_parameters_set_sample_rate_hertz(
        codec_parameters, client->audio.general_config.sample_rate_hertz);

    char const *CODEC_NAME = ov_codec_pcm16_signed_id();

    client->receive.file.codec = ov_codec_factory_get_codec(
        0, CODEC_NAME, client->send.ssrc_id, codec_parameters);

    codec_parameters = codec_parameters->free(codec_parameters);

    if (0 == client->receive.file.codec) {
        fprintf(stderr, "Could not find codec for %s\n", CODEC_NAME);
        goto error;
    }

    return true;

error:

    if (-1 < fd)
        close(fd);

    return false;
}

/*----------------------------------------------------------------------------*/

bool setup_write_to_pulse(ov_rtp_client *client) {
    OV_ASSERT(0 != client);
    OV_ASSERT(RECEIVE == client->mode);

    ov_pulse_parameters params = {

        .sample_rate_hertz = OV_DEFAULT_SAMPLERATE,
        .usecs_to_buffer = 2000 * 1000, /* Amounts to 0.5 secs */
        .frame_length_usecs = client->audio.general_config.frame_length_usecs,
        .server = client->audio.receive.pa_server,

    };

    client->receive.pulse_context = ov_pulse_connect(params);
    client->receive.output_pcm = output_pulse;

    return 0 != client->receive.pulse_context;
}

/******************************************************************************
 *                              PUBLIC FUNCTIONS
 ******************************************************************************/

bool setup_receiving_client(ov_rtp_client *client, ov_rtp_client_parameters *cp,
                            ov_rtp_client_audio_parameters *ap) {
    if (ov_ptr_valid(client, "No client (0 pointer) given") &&
        ov_ptr_valid(cp, "No client parameters (0 pointer) given") &&
        ov_ptr_valid(ap, "No audio parameters (0 pointer) given") &&
        ov_cond_valid(RECEIVE == client->mode, "Client not in receive mode")) {
        if (0 != ap->receive.file_name) {
            setup_write_to_file(client);

        } else {
            setup_write_to_pulse(client);
        }

        client->event->callback.set(client->event, client->udp_socket,
                                    OV_EVENT_IO_IN, client,
                                    receive_udp_handler);

        return true;

    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

void shutdown_receiving_client(ov_rtp_client *client) {
    OV_ASSERT(RECEIVE == client->mode);

    if (0 == client->receive.output_pcm)
        return;

    if (0 != client->receive.pulse_context) {
        ov_pulse_context *context = client->receive.pulse_context;
        context = ov_pulse_disconnect(context);

    } else {
        close(client->receive.file.fd);
        client->receive.file.fd = -1;

        client->receive.file.codec = ov_codec_free(client->receive.file.codec);
    }
}

/*----------------------------------------------------------------------------*/
