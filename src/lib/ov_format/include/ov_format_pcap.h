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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

        The PCAP file format as used by libpcap, tcpdump(1) and wireshark

        See https://wiki.wireshark.org/Development/LibpcapFileFormat

        or, for the PCAPng:
        See http://xml2rfc.tools.ietf.org/cgi-bin/xml2rfc.cgi?url=https://raw.githubusercontent.com/pcapng/pcapng/master/draft-gharris-opsawg-pcap.xml&modeAsFormat=html/ascii&type=ascii#rfc.section.3

        ------------------------------------------------------------------------
*/
#ifndef OV_FILE_PCAP_H
#define OV_FILE_PCAP_H

#include "ov_format.h"
#include "ov_format_registry.h"

#include <stdbool.h>

extern char const *OV_FORMAT_PCAP_TYPE_STRING;

/*****************************************************************************
                             Data link layer types
 ****************************************************************************/

/*
 * See https://www.tcpdump.org/linktypes.html
 */

#define OV_FORMAT_PCAP_LINKTYPE_ETHERNET 1
#define OV_FORMAT_PCAP_LINKTYPE_LINUX_SLL 113

/*****************************************************************************
                                   Data types
 ****************************************************************************/

typedef struct {

  bool bytes_swapped;
  uint16_t version_major;
  uint16_t version_minor;

  int32_t thiszone;
  uint32_t sigfigs;
  uint32_t snaplen;
  uint32_t data_link_type; /* 'network' in documentation - one of
                              OV_FORMAT_PCAP_LINKTYPE_* */

} ov_format_pcap_global_header;

typedef struct {

  uint32_t timestamp_secs;
  uint32_t timestamp_usecs;
  uint32_t length_stored_bytes;
  uint32_t length_origin_bytes;

} ov_format_pcap_packet_header;

/*****************************************************************************
                    INSTALL function for the format registry
 ****************************************************************************/

bool ov_format_pcap_install(ov_format_registry *registry);

/*****************************************************************************
                            PCAP SPECIFIC functions
 ****************************************************************************/

bool ov_format_pcap_get_global_header(ov_format const *f,
                                      ov_format_pcap_global_header *hdr);

/*----------------------------------------------------------------------------*/

bool ov_format_pcap_get_current_packet_header(
    ov_format const *f, ov_format_pcap_packet_header *hdr);

/*----------------------------------------------------------------------------*/

int ov_format_pcap_print_global_header(FILE *f,
                                       ov_format_pcap_global_header *hdr);

/*----------------------------------------------------------------------------*/

ov_format *ov_format_pcap_create_network_layer_format(ov_format *pcap);

/*----------------------------------------------------------------------------*/
#endif
