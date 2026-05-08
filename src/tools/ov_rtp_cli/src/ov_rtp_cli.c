/***

  Copyright   2018, 2020    German Aerospace Center DLR e.V.
                            German Space Operations Center (GSOC)

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

  This is a test client for sending/receiving RTP streams.
  It will function in one of 4 modes:

  - PCM  GENERATION: Generates PCM data and sends it as RTP to some socket -
 default if no other mode is selected
  - FILE GENERATION: Loops over content of a file and sends it as RTP to some
 socket - triggered by switch -a
  - PULSE AUDIO OUTPUT: Receives RTP frames and plays them to a pa server -
 triggered by switch -d
  - FILE OUTPUT: Receives RTP frames and writes the payload to a file -
 triggered by switch -o

  See parse_command_line_args of a thorough list of possible command line
  switches / arguments.

  \file               ov_test_rtp_client.c
  \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
  \date               2018-09-03

  \ingroup            empty

  \brief              empty

  TODO:
     - Remove output / Toggle output via --verbose
     - Debug missing samples (~ 50% )

 **/
/*---------------------------------------------------------------------------*/
#include <getopt.h>
#include <math.h>
#include <unistd.h>

#include <ov_base/ov_utils.h>

#include <ov_base/ov_registered_cache.h>
#include <ov_base/ov_rtp_frame.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_utils.h>

#include "ov_rtp_client.h"
#include "ov_rtp_client_opt.h"
#include <ov_base/ov_config_log.h>
#include <ov_codec/ov_codec_factory.h>

/*---------------------------------------------------------------------------*/

#define max(a, b)                                                              \
    ({                                                                         \
        __typeof__(a) _a = (a);                                                \
        __typeof__(b) _b = (b);                                                \
        _a > _b ? _a : _b;                                                     \
    })

/*----------------------------------------------------------------------------*/

static void setup_logging() {

    const char LOG_FILE_TEMPLATE[] = "/tmp/rtp_cli.log_";

    char cfg[1024] = {0};

    int retval = snprintf(cfg, sizeof(cfg),
                          "{"
                          "\"systemd\" : true,"
                          "\"file\" : {"
                          "\"file\" : \"%s%ld\","
                          "\"messages_per_file\" : 10000,"
                          "\"num_files\" : 4"
                          "}"
                          "}",
                          LOG_FILE_TEMPLATE, (long)getpid());

    ov_json_value *jcfg = ov_json_value_from_string(cfg, ov_string_len(cfg));

    if ((0 > retval) || (0 == jcfg) || (!ov_config_log_from_json(jcfg))) {

        fprintf(stderr, "Could not setup logging - log to console\n");

    } else {

        fprintf(stdout, "Logging to %s%ld\n", LOG_FILE_TEMPLATE,
                (long)getpid());
    }

    jcfg = ov_json_value_free(jcfg);
}

/******************************************************************************
 *                              DEFAULT SETTINGS
 ******************************************************************************/

static const ov_rtp_client_parameters default_client_parameters = {

    .mode = SEND,

    .debug = true,

    .multicast_group = 0,

    .local_if = "0.0.0.0",
    .local_port = 0,
    .remote_if = "127.0.0.1",
    .remote_port = 55555,

    .max_jitter_usec = 0,
    .ssrc_id = 0,
    .sequence_number = 0,

};

/*----------------------------------------------------------------------------*/

static const ov_rtp_client_audio_parameters default_audio_parameters = {
    .general_config.sample_rate_hertz = 48000,
    .general_config.frame_length_usecs = 20 * 1000,
    .send.type = OV_SINUSOIDS,
    .send.sinusoids.frequency_hertz = 4000,
    .codec_name = "opus",
};

/******************************************************************************
 *                      COMMAND LINDE ARGUMENT HANDLING
 ******************************************************************************/

static void set_out_to_pulse(ov_rtp_client_parameters *cp,
                             ov_rtp_client_audio_parameters *ap,
                             char const *server_name) {

    cp->mode = RECEIVE;

    ap->receive.pa_server = 0;

    if (0 != strcmp(server_name, "-")) {
        ap->receive.pa_server = (char *)server_name;
    }
}

/*----------------------------------------------------------------------------*/

static void set_in_from_pulse(ov_rtp_client_parameters *cp,
                              ov_rtp_client_audio_parameters *ap,
                              char const *server_name) {

    cp->mode = SEND;

    ap->send.type = OV_PULSEAUDIO;
    ap->send.pulse.server = 0;

    if (0 != strcmp(server_name, "-")) {
        ap->send.pulse.server = (char *)server_name;
    }
}

/*----------------------------------------------------------------------------*/

static bool parse_command_line_args(int argc, char **argv,
                                    ov_rtp_client_parameters *cp,
                                    ov_rtp_client_audio_parameters *ap) {

    struct option *longoptions = calloc(
        1, sizeof(struct ov_rtp_client_opt_entry) * ov_rtp_client_opt_size());

    char *optstring = calloc(1, ov_rtp_client_opt_size() * 2 + 1);
    size_t optstring_index = 0;

    for (size_t i = 0; i < ov_rtp_client_opt_size(); ++i) {

        struct option opt = {
            .name = OV_RTP_CLIENT_OPTIONS[i].lopt,
            .flag = 0,
            .val = OV_RTP_CLIENT_OPTIONS[i].sopt,
        };

        optstring[optstring_index++] = OV_RTP_CLIENT_OPTIONS[i].sopt;

        if (OV_RTP_CLIENT_OPTIONS[i].arg_required) {

            optstring[optstring_index++] = ':';
            opt.has_arg = required_argument;
        }

        longoptions[i] = opt;
    }

    optstring[optstring_index] = 0;

    bool ok = true;
    int c = 0;
    int index = -1;
    uint32_t uint32_value = 0;

    while (-1 !=
           (c = getopt_long(argc, argv, optstring, longoptions, &index))) {
        switch (c) {

        case OPT_HELP:

            ov_rtp_client_print_help(stdout);
            exit(1);

        case OPT_RHOST:

            cp->remote_if = optarg;
            break;

        case OPT_RPORT:
            cp->remote_port = ov_string_to_uint16(optarg, &ok);
            break;

        case OPT_LISTENIF:
            cp->local_if = optarg;
            if (0 != cp->multicast_group) {
                fprintf(stderr,
                        "BEWARE: Multicast is going to be used - if you "
                        "select a specific local interface (%s), multicast "
                        "might not be receivable.\nIf in doubt, don't set "
                        "the local interface",
                        cp->local_if);
            }
            break;

        case OPT_LPORT:
            cp->local_port = ov_string_to_uint16(optarg, &ok);
            break;

        case OPT_INTERVAL:
            ap->general_config.frame_length_usecs =
                ov_string_to_uint16(optarg, &ok);
            break;

        case OPT_JITTER:
            cp->max_jitter_usec = ov_string_to_uint16(optarg, &ok);
            break;

        case OPT_SAMPLERATE:
            uint32_value = ov_string_to_uint16(optarg, &ok);
            if (0 == uint32_value) {

                fprintf(stderr, "Cowardly refusing to set sample rate to 0\n");
                exit(EXIT_FAILURE);
            }
            ap->general_config.sample_rate_hertz = uint32_value;
            break;

        case OPT_CODEC:
            ap->codec_name = optarg;
            break;

        case OPT_TONEFREQ:
            ap->send.sinusoids.frequency_hertz =
                ov_string_to_uint16(optarg, &ok);
            break;

        case OPT_WOBBLEFREQ:
            ap->send.sinusoids.wobble.frequency_disp_hertz =
                ov_string_to_uint16(optarg, &ok);
            break;

        case OPT_WOBBLEPERIOD:
            ap->send.sinusoids.wobble.period_secs =
                ov_string_to_uint16(optarg, &ok);
            break;

        case OPT_SSID:
            cp->ssrc_id = (uint32_t)ov_string_to_uint32(optarg, &ok);
            break;

        case OPT_PAYLOAD_TYPE:
            cp->payload_type = ov_string_to_uint16(optarg, &ok);
            break;

        case OPT_PULSEOUT:
            set_out_to_pulse(cp, ap, optarg);
            break;

        case OPT_PULSEIN:
            set_in_from_pulse(cp, ap, optarg);
            break;

        case OPT_OUTPUTFILE:
            cp->mode = RECEIVE;
            ap->receive.file_name = optarg;
            break;

        case OPT_INPUTFILE:
            cp->mode = SEND;
            ap->send.type = OV_FROM_FILE;
            ap->send.file.file_name = optarg;
            ap->send.file.codec_config = 0;
            break;

        case OPT_SEND_SDES:

            cp->sdes = optarg;
            break;

        case OPT_MULTICAST:

            if (default_client_parameters.local_if != cp->local_if) {
                fprintf(stderr,
                        "Warning: Interface to bind to (%s) might prevent "
                        "receiving multicast - don't set local interface "
                        "in case of troubles\n",
                        cp->local_if);
            }
            cp->multicast_group = optarg;
            break;

        default:
            fprintf(stderr, "Unknown command line option %c\n", c);
            exit(EXIT_FAILURE);
        };
    };

    free(longoptions);
    free(optstring);

    return true;
}

/*---------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {

    ov_rtp_client *client = 0;

    int retval = EXIT_SUCCESS;

    setup_logging();

    ov_rtp_frame_enable_caching(100);
    ov_buffer_enable_caching(100);

    ov_rtp_client_parameters client_parameters = default_client_parameters;
    ov_rtp_client_audio_parameters audio_parameters = default_audio_parameters;

    if (!parse_command_line_args(argc, argv, &client_parameters,
                                 &audio_parameters)) {
        fprintf(stderr, "Could not parse command line\n");
        goto error;
    }

    fprintf(stdout, "Client configuration:\n");
    ov_rtp_client_parameters_print(stdout, &client_parameters, 4);
    ov_rtp_client_audio_parameters_print(stdout, &audio_parameters, 4);
    fprintf(stdout, "\n");

    client = ov_rtp_client_create(&client_parameters, &audio_parameters);

    fprintf(stdout, "\nStarting Client:\n");
    ov_rtp_client_print(stdout, client, 4);

    if (0 == client) {

        fprintf(stderr, "Could not create client\n");
        goto error;
    }

    ov_rtp_client_run(client);

    goto finish;

error:

    retval = EXIT_FAILURE;

finish:

    ov_rtp_client_free(client);

    ov_registered_cache_free_all();

    ov_codec_factory_free(0);

    return retval;
}

/*----------------------------------------------------------------------------*/
