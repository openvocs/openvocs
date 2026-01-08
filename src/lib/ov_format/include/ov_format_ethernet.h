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

        ------------------------------------------------------------------------
*/
#ifndef OV_FORMAT_ETHERNET_H
#define OV_FORMAT_ETHERNET_H
/*----------------------------------------------------------------------------*/

#include "ov_format.h"
#include "ov_format_registry.h"

/*----------------------------------------------------------------------------*/

#define OV_FORMAT_ETHERNET_MAC_LEN_OCTETS 6

extern char const *OV_FORMAT_ETHERNET_TYPE_STRING;

/*----------------------------------------------------------------------------*/

/**
 * Can be given as argument to `ov_format_as("ethernet", ...)`
 * CRC might not be part of the ethernet frame, as seen in PCAP files.
 * By default, CRC is assumed to be there.
 */
typedef struct {

    bool crc_present : 1;

} ov_format_option_ethernet;

/*****************************************************************************
                                   Data types
 ****************************************************************************/

typedef struct {

    uint8_t src_mac[OV_FORMAT_ETHERNET_MAC_LEN_OCTETS];
    uint8_t dst_mac[OV_FORMAT_ETHERNET_MAC_LEN_OCTETS];

    uint16_t type;
    uint16_t length;

    struct {

        bool type_set : 1;
    };

} ov_format_ethernet_header;

/*****************************************************************************
                    INSTALL function for the format registry
 ****************************************************************************/

bool ov_format_ethernet_install(ov_format_registry *registry);

/*----------------------------------------------------------------------------*/

bool ov_format_ethernet_get_header(ov_format const *f,
                                   ov_format_ethernet_header *ext);

/*----------------------------------------------------------------------------*/

uint32_t ov_format_ethernet_get_crc32(ov_format const *f);

/*----------------------------------------------------------------------------*/

uint32_t ov_format_ethernet_calculate_crc32(ov_format const *f);

/*****************************************************************************
                                 OTHER HELPERS
 ****************************************************************************/

char *ov_format_ethernet_mac_to_string(uint8_t *mac, char *out_string,
                                       size_t out_len);

/*****************************************************************************
                     Dispatcher for encapsulated protocols
 ****************************************************************************/

extern char const *OV_FORMAT_ETHERNET_DISPATCHER_TYPE_STRING;

/**
 * ov_format_as(f, "ethernet_dispatcher") creates a new format on top of
 * an ethernet format that dispatches to encapsulated protocols.
 *
 * Will act as if it was the encapsulated protocol, e.g. ipv4 or ipv6 .
 *
 * ov_format_payload_read_chunk(ethernet_dispatcher, 0)
 * returns as if it was called on the appropriate encapsulated protocol format.
 *
 * BEWARE: To use OV_FORMAT_ETHERNET_DISPATCHER_TYPE_STRING format,
 *         "ipv4" and "ipv6" formats are required to be installed as well!
 */
bool ov_format_ethernet_dispatcher_install(ov_format_registry *registry);

/*----------------------------------------------------------------------------*/

#endif
