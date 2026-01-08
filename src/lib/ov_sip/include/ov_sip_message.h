/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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
        @copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_SIP_MESSAGE_H
#define OV_SIP_MESSAGE_H

/*----------------------------------------------------------------------------*/

#include "stdbool.h"
#include <inttypes.h>
#include <ov_base/ov_buffer.h>
#include <ov_base/ov_result.h>
#include <stdio.h>

/*----------------------------------------------------------------------------*/

#define OV_SIP_HEADER_CONTENT_LENGTH "Content-Length"
#define OV_SIP_HEADER_CONTENT_TYPE "Content-Type"

/*----------------------------------------------------------------------------*/

typedef struct ov_sip_message ov_sip_message;

typedef enum {
  OV_SIP_REQUEST,
  OV_SIP_RESPONSE,
  OV_SIP_INVALID
} ov_sip_message_type;

char const *ov_sip_message_type_to_string(ov_sip_message_type type);

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_request_create(char const *request,
                                              char const *uri);

ov_sip_message *ov_sip_message_response_create(uint16_t code,
                                               char const *reason);

ov_sip_message *ov_sip_message_free(ov_sip_message *self);

ov_sip_message *ov_sip_message_copy(ov_sip_message const *self);

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_cast(void *ptr);

ov_sip_message_type ov_sip_message_type_get(ov_sip_message const *self);

uint32_t ov_sip_message_cseq(ov_sip_message const *self, char const **method);

bool ov_sip_message_cseq_set(ov_sip_message *self, char const *method,
                             uint32_t seq);

/*****************************************************************************
                                    REQUEST
 ****************************************************************************/

char const *ov_sip_message_method(ov_sip_message const *self);

char const *ov_sip_message_uri(ov_sip_message const *self);

/*****************************************************************************
                                    RESPONSE
 ****************************************************************************/

int16_t ov_sip_message_response_code(ov_sip_message const *self);

char const *ov_sip_message_response_reason(ov_sip_message const *self);

/*----------------------------------------------------------------------------*/

bool ov_sip_message_header_set(ov_sip_message *self, char const *name,
                               char const *value);

char const *ov_sip_message_header(ov_sip_message const *self, char const *name);

bool ov_sip_message_header_for_each(ov_sip_message const *self,
                                    bool (*func)(char const *name,
                                                 char const *value,
                                                 void *additional),
                                    void *additional);

/*----------------------------------------------------------------------------*/

ov_buffer const *ov_sip_message_body(ov_sip_message const *self);

bool ov_sip_message_body_set(ov_sip_message *self, ov_buffer *body,
                             char const *content_type);

/*****************************************************************************
                                  SPECIAL DATA
 ****************************************************************************/

/**
 * @return Caller or 0 in case of error.
 * If non-null, the buffer contains a real, zero-terminated string.
 */
ov_buffer *ov_sip_message_get_caller(ov_sip_message const *self);

/*****************************************************************************
                                    Inspect
 ****************************************************************************/

void ov_sip_message_dump(FILE *out, ov_sip_message const *self);

/******************************************************************************
 *                                  CACHING
 ******************************************************************************/

/**
 * Enables caching.
 * BEWARE: Call ov_registered_cache_free_all() before exiting your process to
 * avoid memleaks!
 */
void ov_sip_message_enable_caching(size_t capacity);

#endif
