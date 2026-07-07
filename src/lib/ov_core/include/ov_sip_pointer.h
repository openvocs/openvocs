/***
        ------------------------------------------------------------------------

        Copyright (c) 2026 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                sip://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the openvocs project. sips://openvocs.org

        ------------------------------------------------------------------------
*//**
        @file           ov_sip_pointer.h
        @author         Töpfer, Markus

        @date           2026-06-30


        ------------------------------------------------------------------------
*/
#ifndef ov_sip_pointer_h
#define ov_sip_pointer_h

#include <inttypes.h>
#include <stdlib.h>

#include <ov_base/ov_buffer.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_memory_pointer.h>

#define OV_SIP_CONTENT_LENGTH "Content-Length"
#define OV_SIP_CONTENT_TYPE "Content-Type"
#define OV_SIP_DATE "Date"

/*----------------------------------------------------------------------------*/

typedef enum {

    OV_SIP_PARSER_ABSENT = -3,  // not present e.g. when searching a key
    OV_SIP_PARSER_OOB = -2,     // Out of Bound e.g. for array
    OV_SIP_PARSER_ERROR = -1,   // processing error or mismatch
    OV_SIP_PARSER_PROGRESS = 0, // still matching, need more input data
    OV_SIP_PARSER_SUCCESS = 1   // content match

} ov_sip_parser_state;

/*----------------------------------------------------------------------------*/

typedef struct ov_sip_message_config {

    struct {

        size_t capacity;              // amount of headers supported
        size_t max_bytes_method_name; // max bytes of a method name
        size_t max_bytes_line;        // max bytes of a header line

    } header;

    struct {

        size_t default_size;

    } buffer;

} ov_sip_message_config;

/*----------------------------------------------------------------------------*/

typedef struct {

    uint8_t major;
    uint8_t minor;

} ov_sip_version;

/*----------------------------------------------------------------------------*/

typedef struct {

    uint32_t code;

    ov_memory_pointer phrase;

} ov_sip_status;

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_memory_pointer method;
    ov_memory_pointer uri;

} ov_sip_request;

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_memory_pointer name;
    ov_memory_pointer value;

} ov_sip_header;

/*----------------------------------------------------------------------------*/

typedef struct {

    uint16_t magic_byte;
    ov_sip_message_config config;

    ov_buffer *buffer;

    ov_sip_version version;
    /* Either request or status will be set */
    ov_sip_request request;
    ov_sip_status status;

    ov_memory_pointer body;

    /*  Array of header field pointers in order of reception */
    ov_sip_header header[];

} ov_sip_message;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_sip_message *ov_sip_message_create(ov_sip_message_config config);
bool ov_sip_message_clear(ov_sip_message *message);
void *ov_sip_message_free(void *message);
void *ov_sip_message_free_uncached(void *message);
ov_sip_message *ov_sip_message_cast(void *message);

/*----------------------------------------------------------------------------*/

void ov_sip_enable_caching(size_t capacity);
/**
    Set default parameter, if config parameter is 0
*/
ov_sip_message_config
ov_sip_message_config_init(ov_sip_message_config config);

/*
 *      ------------------------------------------------------------------------
 *
 *      DE / ENCODING
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

/**
    This function will parse the state out of its own buffer.

    @param message  sip message with msg->buffer to parse
    @param next     pointer to next byte within own buffer

    @NOTE This function will only parse the length specific characteristics,
    independent of message type (request/state) or any type related specifics.
    The purpose of this parsing is the check for length compliance and length
    estimation, as long as the buffer content is in line with the sip grammar.
*/
ov_sip_parser_state ov_sip_parse_message(ov_sip_message *message, 
        uint8_t **next);

ov_sip_parser_state ov_sip_parse_message_buffer(
        const uint8_t *start,
        size_t len,
        uint8_t **next,
        ov_sip_message **out);

/*----------------------------------------------------------------------------*/

/**
    Get some header line, out of some header array.

    @param array    some array of ov_http_header pointers
    @param size     size of the array
    @param index    start of search
    @param name     (mandatory) field name to search

    @returns first occurance of the name after index within the array
    will forward index to the the returned header if found
*/
const ov_sip_header *ov_sip_header_get_next(const ov_sip_header *array,
                                              size_t size, size_t *index,
                                              const char *name);

const ov_sip_header *ov_sip_header_get_unique(const ov_sip_header *array,
                                                size_t size, const char *name);

const ov_sip_header *ov_sip_header_get(const ov_sip_header *array,
                                         size_t size, const char *name);

bool ov_sip_message_ensure_open_capacity(ov_sip_message *msg,
                                          size_t capacity);

bool ov_sip_message_shift_trailing_bytes(ov_sip_message *source,
                                          uint8_t *next,
                                          ov_sip_message **dest);

ov_sip_message *ov_sip_create_request(ov_sip_message_config config,
                                               ov_sip_version version,
                                               const char *method,
                                               const char *uri);

ov_sip_message *ov_sip_create_status(ov_sip_message_config config,
                                              ov_sip_version version,
                                              uint16_t code,
                                              const char *phrase);

bool ov_sip_message_add_header(ov_sip_message *msg, const char *key,
                                       const char *val);

bool ov_sip_message_add_header_copy(ov_sip_message *msg, const ov_sip_header *header);

bool ov_sip_message_close_header(ov_sip_message *msg);

bool ov_sip_message_add_body(ov_sip_message *msg, ov_memory_pointer body);
bool ov_sip_message_add_body_string(ov_sip_message *msg, const char *body);

bool ov_sip_is_request(const ov_sip_message *msg, const char *method);

bool ov_sip_message_add_content_type(ov_sip_message *msg, const char *mime,
                                      const char *charset);

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_register(const char *uri);
ov_sip_message *ov_sip_message_options(const char *uri);

ov_sip_message *ov_sip_message_response(uint64_t code, const char *phrase);

#endif /* ov_sip_pointer_h */
