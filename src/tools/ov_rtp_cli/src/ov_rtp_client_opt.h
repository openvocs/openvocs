/***

  Copyright   2020    German Aerospace Center DLR e.V.,
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

 ***/ /**

      \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
      \date               2020-05-10

     **/
#ifndef OV_RTP_CLIENT_OPT_H
#define OV_RTP_CLIENT_OPT_H

/*----------------------------------------------------------------------------*/

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/*----------------------------------------------------------------------------*/

#define OPT_INPUTFILE 'a'
#define OPT_CODEC 'c'
#define OPT_PULSEOUT 'd'
#define OPT_LISTENIF 'f'
#define OPT_HELP 'h'
#define OPT_RHOST 'H'
#define OPT_INTERVAL 'i'
#define OPT_JITTER 'j'
#define OPT_PULSEIN 'm'
#define OPT_OUTPUTFILE 'o'
#define OPT_RPORT 'p'
#define OPT_LPORT 'q'
#define OPT_PAYLOAD_TYPE 'P'
#define OPT_SAMPLERATE 'r'
#define OPT_SSID 's'
#define OPT_TONEFREQ 't'
#define OPT_WOBBLEPERIOD 'u'
#define OPT_WOBBLEFREQ 'w'
#define OPT_SEND_SDES 'S'
#define OPT_MULTICAST 'M'

struct ov_rtp_client_opt_entry {

    char sopt;
    char const *lopt;
    char const *desc;
    bool arg_required;
};

extern const struct ov_rtp_client_opt_entry OV_RTP_CLIENT_OPTIONS[];

/*----------------------------------------------------------------------------*/

#define OV_RTP_CLIENT_HELP                                                     \
    "Generates / receives RTP streams\n\n"                                     \
    "Supported reception modes are\n\n"                                        \
    "* Playback to Pulseaudio (activate with switch '-d')\n"                   \
    "* PCM to file (activate with switch '-o')\n\n"                            \
    "\n\nSupported send modes are\n\n"                                         \
    "* PCM generation (default)\n"                                             \
    "* PCM from file (activate with switch '-a')\n"                            \
    "* Record from Pulseaudio (activate with switch '-m')\n\n"

/*----------------------------------------------------------------------------*/

size_t ov_rtp_client_opt_size();

void ov_rtp_client_print_help(FILE *f);

#endif
