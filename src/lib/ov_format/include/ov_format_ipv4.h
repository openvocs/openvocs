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
#ifndef OV_FORMAT_IPV4_H
#define OV_FORMAT_IPV4_H
/*----------------------------------------------------------------------------*/

#include "ov_format.h"
#include "ov_format_registry.h"

/*****************************************************************************
                                   Data types
 ****************************************************************************/

typedef struct {

    uint8_t src_ip[4];
    uint8_t dst_ip[4];

    size_t header_length_octets;
    uint16_t total_length_octets;
    uint8_t protocol;
    uint8_t time_to_live;

    uint16_t header_checksum;

} ov_format_ipv4_header;

/*****************************************************************************
                    INSTALL function for the format registry
 ****************************************************************************/

bool ov_format_ipv4_install(ov_format_registry *registry);

/*----------------------------------------------------------------------------*/

bool ov_format_ipv4_get_header(ov_format const *f, ov_format_ipv4_header *ext);

/*----------------------------------------------------------------------------*/

char *ov_format_ipv4_ip_to_string(uint8_t *ip, char *out_buf,
                                  size_t out_buf_len);

/*----------------------------------------------------------------------------*/
#endif
