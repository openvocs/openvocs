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
#ifndef OV_FORMAT_UDP_H
#define OV_FORMAT_UDP_H
/*----------------------------------------------------------------------------*/

#include "ov_format.h"
#include "ov_format_registry.h"

/*----------------------------------------------------------------------------*/

/*****************************************************************************
                                   Data types
 ****************************************************************************/

typedef struct {

    uint16_t source_port;
    uint16_t destination_port;

    uint16_t length_octets;
    uint16_t checksum;

} ov_format_udp_header;

/*****************************************************************************
                    INSTALL function for the format registry
 ****************************************************************************/

bool ov_format_udp_install(ov_format_registry *registry);

/*----------------------------------------------------------------------------*/

bool ov_format_udp_get_header(ov_format const *f, ov_format_udp_header *ext);

/*----------------------------------------------------------------------------*/
#endif
