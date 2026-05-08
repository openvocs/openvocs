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

        Bare minimum implementation of IPv6 (not even that - extension headers
        are currently not supported - in fact, only UPD, TCP and ICMPv6 packets
        as payload are supported currently)

        ------------------------------------------------------------------------
*/
#ifndef OV_FORMAT_IPV6_H
#define OV_FORMAT_IPV6_H
/*----------------------------------------------------------------------------*/

#include "ov_format.h"
#include "ov_format_registry.h"

#include <stdbool.h>
#include <stdlib.h>

/*****************************************************************************
                                   Data types
 ****************************************************************************/

typedef struct {

    uint8_t traffic_class;

    struct {

        uint32_t flow_label : 20;
        uint32_t payload_length : 20;
    };

    uint8_t next_header;

    uint8_t hop_limit;

    uint8_t src_ip[16];
    uint8_t dst_ip[16];

} ov_format_ipv6_header;

/*****************************************************************************
                    INSTALL function for the format registry
 ****************************************************************************/

bool ov_format_ipv6_install(ov_format_registry *registry);
/*----------------------------------------------------------------------------*/

bool ov_format_ipv6_get_header(ov_format const *f, ov_format_ipv6_header *ext);

/*----------------------------------------------------------------------------*/

char *ov_format_ipv6_ip_to_string(uint8_t *ip, char *out_buf,
                                  size_t out_buf_len);

/*----------------------------------------------------------------------------*/

#endif
