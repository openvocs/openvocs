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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_RTCP_H
#define OV_RTCP_H

#include "ov_buffer.h"

/*----------------------------------------------------------------------------*/

typedef enum {

  OV_RTCP_INVALID = -1,
  OV_RTCP_SENDER_REPORT = 200,
  OV_RTCP_RECEIVER_REPORT = 201,
  OV_RTCP_SOURCE_DESC = 202,
  OV_RTCP_BYE = 203,

} ov_rtcp_type;

char const *ov_rtcp_type_to_string(ov_rtcp_type type);

struct ov_rtcp_message_struct;

typedef struct ov_rtcp_message_struct ov_rtcp_message;

ov_rtcp_message *ov_rtcp_message_free(ov_rtcp_message *self);

/**
 * Tries to decode next RTCP message.
 * On success, *buf points to the first octet AFTER the decoded message,
 * *lenoctets is set to the number of remaining unparsed octets
 */
ov_rtcp_message *ov_rtcp_message_decode(uint8_t const **buf, size_t *lenoctets);

#define ov_rtcp_message_encode(...)                                            \
  ov_rtcp_messages_encode((ov_rtcp_message const *[]){__VA_ARGS__, 0})

ov_buffer *ov_rtcp_messages_encode(ov_rtcp_message const *msgs[]);

ov_rtcp_type ov_rtcp_message_type(ov_rtcp_message const *self);

/**
 * Get number of octets this message consumes when encoded
 */
size_t ov_rtcp_message_len_octets(ov_rtcp_message const *self);

/**
 * Create an Service description RTCP message.
 * We only support ONE SSRC/CNAME pair for now
 */
ov_rtcp_message *ov_rtcp_message_sdes(char const *cname, uint32_t ssrc);

uint32_t ov_rtcp_message_sdes_ssrc(ov_rtcp_message const *self,
                                   size_t chunk_index);

char const *ov_rtcp_message_sdes_cname(ov_rtcp_message const *self,
                                       size_t chunk_index);

void ov_rtpc_enable_caching(size_t capacity);

/*----------------------------------------------------------------------------*/
#endif
