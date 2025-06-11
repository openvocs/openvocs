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

#include "ov_base/ov_socket.h"
#include <netinet/in.h>
#include <ov_base/ov_mc_socket.h>
#include <ov_base/ov_rtp_frame.h>
#include <ov_base/ov_string.h>
#include <ov_codec/ov_codec_factory.h>
#include <ov_pcm16s/ov_pcm16_mod.h>

/*----------------------------------------------------------------------------*/

static int recv_socket(char const *mc_group, int port) {

    ov_socket_configuration scfg = {.port = port, .type = UDP};

    size_t max_hostlen = sizeof(scfg.host);

    strncpy(scfg.host, mc_group, max_hostlen);
    scfg.host[max_hostlen - 1] = 0;

    int fd = ov_mc_socket(scfg);
    ov_socket_set_reuseaddress(fd);
    return fd;
}

/*----------------------------------------------------------------------------*/

static ov_codec *get_codec(char const *codec_json_str) {

    ov_codec *codec = 0;

    if (ov_ptr_valid(codec_json_str, "No codec given")) {
        ov_json_value *codec_json =
            ov_json_value_from_string(codec_json_str, strlen(codec_json_str));
        codec = ov_codec_factory_get_codec_from_json(0, codec_json, 1);
        codec_json = ov_json_value_free(codec_json);
    }

    if (0 == codec) {

        fprintf(stderr,
                "Could not create codec for %s\n",
                ov_string_sanitize(codec_json_str));
    }

    return codec;
}

/*----------------------------------------------------------------------------*/

static bool print_rtp_characteristics(ov_rtp_frame const *frame, FILE *out) {

    if (0 == frame) {
        fprintf(stderr, "Received UDP data could not be decoded as RTP");
        return false;
    } else {
        fprintf(out,
                "SSRC %" PRIu32 "  SEQ  %" PRIu16,
                frame->expanded.ssrc,
                frame->expanded.sequence_number);
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool decode_to_pcm(ov_rtp_frame const *frame,
                          ov_codec *codec,
                          uint8_t *pcm,
                          ssize_t *pcm_octets) {

    if (0 == frame) {
        return false;
    } else if (0 == codec) {
        fprintf(stderr, "Could not decode RTP audio - no codec\n");
        return false;
    } else {

        OV_ASSERT(0 != pcm_octets);

        *pcm_octets = ov_codec_decode(codec,
                                      frame->expanded.sequence_number,
                                      frame->expanded.payload.data,
                                      frame->expanded.payload.length,
                                      pcm,
                                      *pcm_octets);

        if (0 > *pcm_octets) {

            fprintf(stderr, "Decoding RTP payload as Opus audio failed\n");
            return false;
        } else {
            return true;
        }
    }
}

/*----------------------------------------------------------------------------*/

static bool check__plausibility(FILE *out,
                                ov_vad_parameters params,
                                double zero_crossings_hz_ref,
                                double powerlevel_density_db_ref) {

    double zero_crossings_hz = params.zero_crossings_per_sample;
    zero_crossings_hz *= 48000;

    double zero_crossings_diff =
        fabs(zero_crossings_hz - zero_crossings_hz_ref);

    // Powerlevel density
    double powerlevel_density =
        20.0 / log(10.0) * log(params.powerlevel_density_per_sample);

    double powerlevel_diff =
        fabs(powerlevel_density - powerlevel_density_db_ref);

    bool ok = true;

    if (0.001 < zero_crossings_diff) {
        ok = false;
        fprintf(out, "Zero crossing differ too much: %f", zero_crossings_diff);
    }

    if (0.001 < powerlevel_diff) {
        ok = false;
        fprintf(out, "Power levels differ too much: %f", powerlevel_diff);
    }

    if (ok) {
        fprintf(out, "  VAD parameters plausible");
    }

    return ok;
}

/*----------------------------------------------------------------------------*/

static bool print_audio_characteristics(uint8_t *pcm,
                                        size_t pcm_octets,
                                        ov_vad_config vad,
                                        FILE *out) {

    OV_ASSERT(0 != pcm);

    ov_vad_parameters params = {0};

    ov_pcm_16_get_vad_parameters(pcm_octets / 2, (int16_t *)pcm, &params);

    double powerlevel_density_db =
        20.0 / log(10.0) * log(params.powerlevel_density_per_sample);

    double zero_crossings_hz = params.zero_crossings_per_sample * 48000;

    fprintf(out,
            "   Powerlevel density %f   zero crossing rate %f Hz",
            powerlevel_density_db,
            zero_crossings_hz);

    check__plausibility(out, params, zero_crossings_hz, powerlevel_density_db);

    if (vad.zero_crossings_rate_threshold_hertz != 0) {

        if (ov_pcm_vad_detected(48000, params, vad)) {

            fprintf(out, "   VOICE detected");

        } else {

            fprintf(out, "   voice NOT DETECTED");
        }
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static void receive_from(int fd,
                         char const *mc_ip,
                         char const *mc_port,
                         ov_codec *codec,
                         ov_vad_config vad,
                         FILE *out) {

    // One UDP frame wont exceed 1500 bytes in 'our' ethernet world
    // In linux, it's likely even less than 768 (the minimum required
    // supported UDP frame length - 2000 and we are safe for the near future
    uint8_t buf[2000] = {0};

    while (true) {

        ssize_t read_octets = recv(fd, buf, sizeof(buf) - 1, 0);

        if (read_octets >= 0) {

            buf[(size_t)read_octets] = 0;

            ov_rtp_frame *frame = ov_rtp_frame_decode(buf, read_octets);

            // We send frames of length 20ms@48000Hz with 2 bytes per sample
            // -> one frame is 20 * 48000 /1000 * 2 = 960 octets
            uint8_t pcm[8000] = {0};

            ssize_t pcm_octets = sizeof(pcm);

            fprintf(out,
                    "From %s:%s  ",
                    ov_string_sanitize(mc_ip),
                    ov_string_sanitize(mc_port));

            if (print_rtp_characteristics(frame, out) &&
                decode_to_pcm(frame, codec, pcm, &pcm_octets)) {
                print_audio_characteristics(pcm, (size_t)pcm_octets, vad, out);
            }

            fprintf(out, "\n");

            frame = ov_rtp_frame_free(frame);

        } else if (EAGAIN != errno) {

            fprintf(
                stderr, "Could not read from socket: %s\n", strerror(errno));
        }
    }
}

/*----------------------------------------------------------------------------*/

int main(int argc, char const **argv) {

    if (3 > argc) {

        fprintf(stderr,
                " Multicast test client\n\n"
                " Joins multicast group, tries to decode as RTP/OPUS and "
                " prints audio parameters, i.e. zero crossings  rate and "
                " power density\n"
                " If POWERLEVEL and ZERO_CROSSINGS is given, VAD detection is "
                "tried\n"
                "\n\n"
                "     Usage: %s MULTICAST_IP PORT  [POWERLEVEL "
                "ZERO_CROSSINGS]\n"
                "\n\n",
                argv[0]);

        return EXIT_FAILURE;

    } else {

        bool detect_voice = !(argc < 5);

        ov_vad_config vad_config = {0};

        if (detect_voice) {

            vad_config.powerlevel_density_threshold_db = atof(argv[3]);
            vad_config.zero_crossings_rate_threshold_hertz = atof(argv[4]);
        }

        int recv_fd = recv_socket(argv[1], atoi(argv[2]));
        ov_codec *codec = get_codec(OV_INTERNAL_CODEC);

        int retval = EXIT_FAILURE;

        if (0 > recv_fd) {

            fprintf(stderr, "Could not open socket: %s\n", strerror(errno));

        } else {

            receive_from(recv_fd, argv[1], argv[2], codec, vad_config, stdout);
            close(recv_fd);
            retval = EXIT_SUCCESS;
        }

        codec = ov_codec_free(codec);

        return retval;
    }
}
