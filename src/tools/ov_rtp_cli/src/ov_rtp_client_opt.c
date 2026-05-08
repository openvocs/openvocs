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

        ------------------------------------------------------------------------
*/

#include <stdio.h>

#include "ov_rtp_client_opt.h"

/*----------------------------------------------------------------------------*/

const struct ov_rtp_client_opt_entry OV_RTP_CLIENT_OPTIONS[] = {

    {OPT_HELP, "help", "print help", false},

    {OPT_RHOST, "rhost", "remote host to send to", true},
    {OPT_RPORT, "rport", "remote port to send to", true},
    {OPT_LISTENIF, "listenif", "interface to bind to", true},
    {OPT_LPORT, "lport", "local port to bind to", true},

    {OPT_INTERVAL, "interval", "interval to send out rtp frames (usecs)", true},
    {OPT_JITTER, "jitter", "max jitter to emulate (usecs)", true},

    {OPT_SAMPLERATE, "samplerate", "samplerate to use (Hz)", true},
    {OPT_CODEC, "codec", "codec to use", true},

    {OPT_TONEFREQ, "tonefreq", "Frequency of tone to generate (Hz)", true},
    {OPT_WOBBLEFREQ, "wobblefreq",
     "Frequency interval to shift base frequency (Hz)", true},
    {OPT_WOBBLEPERIOD, "wobbleperiod",
     "Time period used to shift frequency entire "
     "wobble interval (secs)",
     true},

    {OPT_SSID, "ssid", "ssid to use in outgoing rtp frames", true},
    {OPT_PAYLOAD_TYPE, "payload", "Payload type to use for outbound RTP", true},

    {OPT_PULSEOUT, "pulseout",
     "pulse device to use for output (use '-' for default device", true},

    {OPT_PULSEIN, "pulsein",
     "pulse device to use for input (use '-' for default device", true},

    {OPT_OUTPUTFILE, "outputfile", "file to write received PCM to", true},

    {OPT_INPUTFILE, "inputfile", "file to read PCM to send from", true},

    {OPT_SEND_SDES, "send_sdes", "Source description to send via RTCP", true},

    {OPT_MULTICAST, "multicast",
     "Receive multicast group. Don't set '-f', otherwise you might not receive "
     "anything",
     true},

};

/*----------------------------------------------------------------------------*/

size_t ov_rtp_client_opt_size() {

    return sizeof(OV_RTP_CLIENT_OPTIONS) / sizeof(OV_RTP_CLIENT_OPTIONS[0]);
}

/*----------------------------------------------------------------------------*/

void ov_rtp_client_print_help(FILE *f) {

    fprintf(f, OV_RTP_CLIENT_HELP);
    fprintf(f, "\nUnderstood command line arguments:\n\n");

    for (size_t i = 0;
         i < sizeof(OV_RTP_CLIENT_OPTIONS) / sizeof(OV_RTP_CLIENT_OPTIONS[0]);
         ++i) {

        struct ov_rtp_client_opt_entry entry = OV_RTP_CLIENT_OPTIONS[i];

        fprintf(f, "-%c   --%s     %s\n", entry.sopt, entry.lopt, entry.desc);
    }

    fprintf(f, "\n\n");
}
