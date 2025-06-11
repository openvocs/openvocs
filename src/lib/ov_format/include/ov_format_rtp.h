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
#ifndef OV_FORMAT_RTP_H
#define OV_FORMAT_RTP_H
/*----------------------------------------------------------------------------*/

#include "ov_format.h"
#include "ov_format_registry.h"

extern ov_buffer const *OV_FORMAT_RTP_NO_PADDING;

/*****************************************************************************
                                   Data types
 ****************************************************************************/

typedef struct {

    struct {

        unsigned version : 2;
        unsigned marker : 1;
        unsigned payload_type : 7;
    };

    uint16_t sequence_number;
    uint32_t timestamp;
    uint32_t ssrc;

} ov_format_rtp_header;

/*----------------------------------------------------------------------------*/

typedef struct {

    struct {

        unsigned num : 4;
    };

    uint32_t ids[15];

} ov_format_rtp_contributing_sources;

/*----------------------------------------------------------------------------*/

typedef struct {

    uint16_t id;

    uint16_t length_4_octets;

    uint32_t *payload;

} ov_format_rtp_extension_header;

/*****************************************************************************
                    INSTALL function for the format registry
 ****************************************************************************/

bool ov_format_rtp_install(ov_format_registry *registry);

/*----------------------------------------------------------------------------*/

bool ov_format_rtp_get_header(ov_format const *f, ov_format_rtp_header *ext);

/*----------------------------------------------------------------------------*/

/**
 * If none were set, csrcs-> num == 0
 */
bool ov_format_rtp_get_contributing_sources(
    ov_format const *f, ov_format_rtp_contributing_sources *csrcs);

/*----------------------------------------------------------------------------*/

/**
 * @return false if none is present in current rtp frame
 */
bool ov_format_rtp_get_extension_header(ov_format const *f,
                                        ov_format_rtp_extension_header *ext);

/*----------------------------------------------------------------------------*/

/**
 * BEWARE: Due to RTP spec, the last octet of the padding contains the  number
 * of padding octets incl. the number octet...
 * @return buffer with padding, OV_FORMAT_RTP_NO_PADDING if none was set or 0 if
 * error
 */
ov_buffer *ov_format_rtp_get_padding(ov_format const *f);

/*----------------------------------------------------------------------------*/
#endif
