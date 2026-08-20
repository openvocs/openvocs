/***
        ------------------------------------------------------------------------

        Copyright (c) 2026 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_sip_pointer.c
        @author         Töpfer, Markus

        @date           2026-06-30


        ------------------------------------------------------------------------
*/
#include "../include/ov_sip_pointer.h"
#include "../include/ov_imf.h"

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <strings.h>

#include <ov_base/ov_utils.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_uri.h>
#include <ov_base/ov_registered_cache.h>

#define IMPL_DEFAULT_HEADER_CAPACITY 100
#define IMPL_DEFAULT_BUFFER_SIZE 4096
#define IMPL_DEFAULT_MAX_METHOD_NAME 20
#define IMPL_DEFAULT_MAX_HEADER_LINE 1000 // max bytes of a line

#define OV_SIP_MESSAGE_MAGIC_BYTE 0x56e3

static ov_registered_cache *g_cache = 0;

/*----------------------------------------------------------------------------*/

static bool is_whitespace(uint8_t item) {

    // single space and htab

    switch (item) {

    case 0x20: // single space
        return true;

    case 0x09: // horizontal tab
        return true;

    default:
        break;
    }

    return false;
}

/*----------------------------------------------------------------------------*/

static bool is_control(uint8_t item) {

    if (item < 32)
        return true;

    if (item == 127)
        return true;

    return false;
}

/*----------------------------------------------------------------------------*/

static bool is_separator(uint8_t item) {

    /* separators     = "(" | ")" | "<" | ">" | "@"
                      | "," | ";" | ":" | "\" | <">
                      | "/" | "[" | "]" | "?" | "="
                      | "{" | "}" | SP | HT
    */

    switch (item) {

    case 0x28:
        return true; // "("
    case 0x29:
        return true; // ")"
    case 0x3C:
        return true; // "<"
    case 0x3E:
        return true; // ">"
    case 0x40:
        return true; // "@"
    case 0x2C:
        return true; // ","
    case 0x3B:
        return true; // ";"
    case 0x3A:
        return true; // ":"
    case 0x5C:
        return true; // "\"
    case 0x22:
        return true; // """
    case 0x2F:
        return true; // "/"
    case 0x5B:
        return true; // "["
    case 0x5D:
        return true; // "]"
    case 0x3F:
        return true; // "?"
    case 0x3D:
        return true; // "="
    case 0x7B:
        return true; // "{"
    case 0x7D:
        return true; // "}"
    case 0x20:
        return true; // single space
    case 0x09:
        return true; // horizontal tab
    default:
        break;
    }

    return false;
}

/*----------------------------------------------------------------------------*/

static bool is_token(uint8_t item) {

    // token          = 1*<any CHAR except CTLs or separators>

    if (is_control(item))
        return false;

    if (is_separator(item))
        return false;

    if (item < 127)
        return true;

    return false;
}

/*----------------------------------------------------------------------------*/

static bool is_token_string(const uint8_t *start, size_t length) {

    if (!start || length == 0)
        return false;

    for (size_t i = 0; i < length; i++) {

        if (!is_token(start[i]))
            return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static bool is_linear_whitespace(const uint8_t *start, size_t length) {

    if (!start || length < 3)
        return false;

    if ((start[0] == '\r') && (start[1] == '\n') &&
        is_whitespace(start[2]))
        return true;

    return false;
}

/*----------------------------------------------------------------------------*/

static const uint8_t *offset_linear_whitespace(const uint8_t *start,
                                                    size_t length) {

    if (!start || length < 3)
        return start;

    if (is_linear_whitespace(start, length))
        return start + 3;

    return start;
}

/*----------------------------------------------------------------------------*/

/*
static uint8_t *offset_whitespace_front(uint8_t *start, size_t size) {

    if (!start)
        goto error;

    if (size == 0)
        goto error;

    uint8_t *ptr = start;

    for (size_t i = 0; i < size; i++) {

        if (!is_whitespace(*ptr))
            break;

        ptr++;
    }

    return ptr;
error:
    return NULL;
}
*/
/*----------------------------------------------------------------------------*/
/*
static uint8_t *offset_whitespace_end(uint8_t *start, size_t size) {

    if (!start)
        goto error;

    if (size == 0)
        goto error;

    uint8_t *ptr = start + size - 1;
    if (*ptr == 0) {
        ptr--;
        size--;
    }

    for (size_t i = size; i > 0; i--) {

        if (!is_whitespace(*ptr))
            break;

        if (ptr == start)
            break;

        ptr--;
    }

    return ptr;
error:
    return NULL;
}
*/
/*----------------------------------------------------------------------------*/

static bool is_reason_phrase(const uint8_t *start, size_t length) {

    for (size_t i = 0; i < length; i++) {

        switch (start[i]) {

        case ' ':
            continue;
        case '\t':
            continue;

        default:
            break;
        }

        if (is_control(start[i]))
            return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static ov_sip_parser_state parse_version(const uint8_t *buffer,
                                                   size_t length,
                                                   ov_sip_version *version) {

    if (!buffer || !version)
        goto error;

    if (0 == length)
        return OV_SIP_PARSER_PROGRESS;

    const uint8_t *ptr = (uint8_t *)buffer;
    size_t size = length;

    // remove leading whitespace
    while (is_whitespace(*ptr)) {

        if ((size_t)(ptr - buffer) >= size)
            break;

        ptr++;
        size--;
    }

    size_t len = 4;
    if (len > size)
        len = size;

    if (0 != strncasecmp((char *)ptr, "SIP/", len))
        goto error;

    if (length < 5)
        return OV_SIP_PARSER_PROGRESS;

    if (!isdigit(ptr[4]))
        goto error;

    if (length < 6)
        return OV_SIP_PARSER_PROGRESS;

    if ('.' != ptr[5])
        goto error;

    if (size < 7)
        return OV_SIP_PARSER_PROGRESS;

    if (!isdigit(ptr[6]))
        goto error;

    version->major = ptr[4] - 48;
    version->minor = ptr[6] - 48;

    return OV_SIP_PARSER_SUCCESS;

error:
    return OV_SIP_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

static ov_sip_parser_state parse_status_line(const uint8_t *buffer,
                                                       size_t length,
                                                       ov_sip_status *status,
                                                       ov_sip_version *version,
                                                       uint8_t **next) {

    if (!buffer || !status || !version)
        goto error;

    const uint8_t *ptr = (uint8_t *)buffer;
    size_t size = length;

    // remove leading whitespace
    while (is_whitespace(*ptr)) {

        if ((size_t)(ptr - buffer) >= size)
            break;

        ptr++;
        size--;
    }

    ov_sip_parser_state state = parse_version(ptr, size, version);

    switch (state) {

    case OV_SIP_PARSER_PROGRESS:
        return OV_SIP_PARSER_PROGRESS;

    case OV_SIP_PARSER_SUCCESS:
        break;

    default:
        goto error;
    }

    if (7 == size)
        return OV_SIP_PARSER_PROGRESS;

    if (' ' != ptr[7])
        goto error;

    if (8 == size)
        return OV_SIP_PARSER_PROGRESS;

    size_t len = 3;
    if (len > size - 8)
        len = size - 8;

    for (size_t i = 0; i < len; i++) {

        if (!isdigit(ptr[8 + i]))
            goto error;
    }

    if (size < 12)
        return OV_SIP_PARSER_PROGRESS;

    if (' ' != ptr[11])
        goto error;

    if (size == 12)
        return OV_SIP_PARSER_PROGRESS;

    // parse code
    const uint8_t *start = ptr + 8;
    uint8_t *end = NULL;

    status->code = strtol((char *)start, (char **)&end, 10);
    if (!end)
        goto error;

    if (3 != (end - start))
        goto error;

    // parse phrase
    start = ptr + 12;
    end = (uint8_t *)ov_string_find((char *)start, size - (start - ptr) + 1,
                                    "\r\n", 2);
    if (!end)
        return OV_SIP_PARSER_PROGRESS;

    if ((end - start) < 1)
        goto error;

    len = (end - start);

    if (!is_reason_phrase(start, len))
        goto error;

    status->phrase.start = start;
    status->phrase.length = len;

    if (next)
        *next = end + 2;

    return OV_SIP_PARSER_SUCCESS;

error:
    return OV_SIP_PARSER_ERROR;
}


/*----------------------------------------------------------------------------*/

ov_sip_parser_state parse_request_line(
    const uint8_t *buffer, size_t length, ov_sip_request *request,
    ov_sip_version *version, uint32_t max_method_name, uint8_t **next) {

    // Request-Line   = Method SP Request-URI SP SIP-Version CRLF

    if (!buffer || !request)
        goto error;

    const uint8_t *ptr = buffer;
    uint8_t *end = NULL;
    const uint8_t *lineend =
        (uint8_t *)ov_string_find((char *)buffer, length, "\r", 1);

    size_t size = length;

    // remove leading whitespace
    while (is_whitespace(*ptr)) {

        if ((size_t)(ptr - buffer) >= size)
            break;

        ptr++;
        size--;
    }

    end = memchr(ptr, ' ', size);
    if (!end) {

        if (0 != max_method_name)
            if (size >= max_method_name)
                goto error;

        if (lineend)
            goto error;

        return OV_SIP_PARSER_PROGRESS;

    } else {

        if (0 != max_method_name)
            if ((end - ptr) > max_method_name)
                goto error;

        if (lineend && ((lineend - buffer) < (end - buffer)))
            goto error;
    }

    if (end[0] != ' ')
        goto error;

    request->method.start = (uint8_t *)ptr;
    request->method.length = (end - ptr);

    if (!is_token_string(request->method.start, request->method.length))
        goto error;

    size = size - request->method.length - 1;
    ptr = end + 1;

    if (size < 1)
        return OV_SIP_PARSER_PROGRESS;

    end = memchr(ptr, ' ', size);
    if (!end) {

        if (size > PATH_MAX)
            goto error;

        if (lineend)
            goto error;

        return OV_SIP_PARSER_PROGRESS;

    } else {

        if ((end - ptr) > PATH_MAX)
            goto error;

        if (lineend && ((lineend - buffer) < (end - buffer)))
            goto error;
    }

    if (end[0] != ' ')
        goto error;

    request->uri.start = (uint8_t *)ptr;
    request->uri.length = (end - ptr);

    if (!ov_uri_string_is_valid((char *)request->uri.start,
                                request->uri.length))
        goto error;

    size = size - request->uri.length - 1;
    ptr = end + 1;

    if (size < 1)
        return OV_SIP_PARSER_PROGRESS;

    switch (parse_version(ptr, size, version)) {

    case OV_SIP_PARSER_PROGRESS:
        return OV_SIP_PARSER_PROGRESS;

    case OV_SIP_PARSER_SUCCESS:
        break;

    default:
        goto error;
    }

    if (size < 9)
        return OV_SIP_PARSER_PROGRESS;

    if (ptr[7] != '\r')
        goto error;

    if (ptr[8] != '\n')
        goto error;

    if (next)
        *next = (uint8_t *)ptr + 9;

    return OV_SIP_PARSER_SUCCESS;

error:
    return OV_SIP_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

static bool is_text(const uint8_t *start, size_t length) {

    const uint8_t *ptr = start;
    const uint8_t *nxt = NULL;
    size_t size = length;

    while (size > 0) {

        if (size >= 3) {

            nxt = ptr;
            nxt = offset_linear_whitespace(ptr, size);

            if (ptr != nxt) {
                ptr = nxt;
                size -= 3;
                continue;
            }
        }

        if (is_whitespace(*ptr)) {

            ptr++;
            size--;
            continue;
        }

        if (is_control(*ptr))
            return false;

        ptr++;
        size--;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static bool is_field_content(const uint8_t *start, size_t length) {

    return is_text(start, length);
}

/*----------------------------------------------------------------------------*/

static ov_sip_parser_state parse_header_line(const uint8_t *buffer,
                                                       size_t length,
                                                       ov_sip_header *line,
                                                       uint32_t max,
                                                       uint8_t **next) {

    /*
     *      header-field   = field-name ":" OWS field-value OWS
     *
     *      field-name     = token
     *      field-value    = *( field-content / obs-fold )
     *      field-content  = field-vchar [ 1*( SP / HTAB ) field-vchar ]
     *      field-vchar    = VCHAR / obs-text
     *
     *      obs-fold       = CRLF 1*( SP / HTAB )
     *                     ; obsolete line folding
     *                     ; see Section 3.2.4
     *
     */

    if (!buffer || !line)
        goto error;

    const uint8_t *ptr = buffer;
    uint8_t *end = NULL;
    const uint8_t *lineend =
        (uint8_t *)ov_string_find((char *)buffer, length, "\r", 1);

    if (lineend && (0 != max))
        if ((lineend - buffer) > max)
            goto error;

    size_t size = length;

    // remove leading whitespace
    while (is_whitespace(*ptr)) {

        if ((size_t)(ptr - buffer) >= size)
            break;

        ptr++;
        size--;
    }

    end = memchr(ptr, ':', size);
    if (!end) {

        if (lineend)
            goto error;

        if ((max != 0) && size > max)
            goto error;

        return OV_SIP_PARSER_PROGRESS;

    } else {

        if (lineend && ((lineend - buffer) < (end - buffer)))
            goto error;
    }

    if (end[0] != ':')
        goto error;

    line->name.start = ptr;
    line->name.length = (end - ptr);

    if (!is_token_string(line->name.start, line->name.length))
        goto error;

    ptr = end + 1;
    size = size - line->name.length - 1;

    // remove optional whitespace
    while (is_whitespace(*ptr)) {

        if ((size_t)(ptr - buffer) >= size)
            break;

        ptr++;
        size--;
    }

    line->value.start = ptr;

    lineend = (uint8_t *)ov_string_find((char *)ptr, size, "\r\n", 2);
    if (!lineend) {

        if ((max != 0) && max < length)
            goto error;

        return OV_SIP_PARSER_PROGRESS;
    }

    while (lineend) {

        if (size == 2)
            return OV_SIP_PARSER_PROGRESS;

        ptr = lineend + 2;
        size -= 2;

        // check obs-fold
        if (is_whitespace(*ptr)) {

            lineend = (uint8_t *)ov_string_find((char *)ptr, size, "\r\n", 2);
            if (!lineend)
                return OV_SIP_PARSER_PROGRESS;

        } else {

            break;
        }
    }

    OV_ASSERT(lineend);
    ptr = lineend - 1;

    while (is_whitespace(*ptr)) {

        ptr--;
        if (ptr == line->value.start)
            goto error;
    }

    line->value.length = ptr - line->value.start + 1;

    if (!is_field_content(line->value.start, line->value.length))
        goto error;

    if (next)
        *next = (uint8_t *)lineend + 2;

    return OV_SIP_PARSER_SUCCESS;

error:
    if (next)
        *next = (uint8_t *)buffer;
    return OV_SIP_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

static ov_sip_parser_state parse_header(const uint8_t *buffer, size_t length,
                             ov_sip_header *array, size_t array_size,
                             uint32_t max, uint8_t **next) {

    if (!buffer || !array)
        goto error;

    size_t size = length;
    const uint8_t *ptr = buffer;
    uint8_t *nxt = NULL;

    if (next)
        *next = (uint8_t *)buffer;

    size_t i = 0;
    ov_sip_parser_state state = OV_SIP_PARSER_ERROR;

    for (i = 0; i < array_size; i++) {

        state = parse_header_line(ptr, size, &array[i], max, &nxt);

        switch (state) {

        case OV_SIP_PARSER_SUCCESS:
            size -= nxt - ptr;
            ptr = nxt;
            continue;

        case OV_SIP_PARSER_PROGRESS:
            return state;

        default:
            break;
        }

        /* No more header line, check for \r\n */

        if (size == 0)
            return OV_SIP_PARSER_PROGRESS;

        if (size > 0)
            if (ptr[0] != '\r')
                goto error;

        if (size > 1) {

            if (ptr[1] != '\n')
                goto error;

            if (next)
                *next = nxt + 2;

            return OV_SIP_PARSER_SUCCESS;

        } else {

            return OV_SIP_PARSER_PROGRESS;
        }
    }
    return OV_SIP_PARSER_OOB;
error:
    if (next)
        *next = (uint8_t *)buffer;
    return OV_SIP_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

static ov_sip_parser_state parse_body(ov_sip_message *msg, uint8_t **next) {

    if (!msg || !next)
        goto error;

    msg->body.start = *next;

    const ov_sip_header *content_length = NULL;

    size_t index_content = 0;

    content_length =
        ov_sip_header_get_next(msg->header, msg->config.header.capacity,
                                &index_content, OV_SIP_CONTENT_LENGTH);

    if (content_length) {

        // We do not support multiple content_length header
        if (ov_sip_header_get_next(msg->header, msg->config.header.capacity,
                                    &index_content, OV_SIP_CONTENT_LENGTH))
            goto error;

        uint8_t *end = NULL;
        msg->body.length =
            strtol((char *)content_length->value.start, (char **)&end, 10);

        if (!end)
            goto error;

        if (end[0] != '\r')
            goto error;

        // check if the buffer contains enough unparsed bytes
        if (msg->buffer->length <
            ((msg->body.start - msg->buffer->start) + msg->body.length))
            return OV_SIP_PARSER_PROGRESS;

        if (next)
            *next = (uint8_t *)msg->body.start + msg->body.length;

    } else {

        // No length or transfer header present
        msg->body.start = NULL;
        msg->body.length = 0;
    }

    return OV_SIP_PARSER_SUCCESS;
error:
    return OV_SIP_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

ov_sip_parser_state ov_sip_parse_message(ov_sip_message *msg,
                                                   uint8_t **next) {

    if (!msg)
        goto error;

    if (!msg->buffer)
        goto error;

    uint8_t *nxt = NULL;

    if (!msg->buffer->start)
        goto error;

    if (msg->buffer->length == 0)
        goto error;

    // try to parse the startline and switch between state and request
    ov_sip_parser_state state = parse_status_line(
        msg->buffer->start, msg->buffer->length, &msg->status, &msg->version,
        &nxt);

    switch (state) {

    case OV_SIP_PARSER_PROGRESS:
        break;

    case OV_SIP_PARSER_SUCCESS:
        break;

    default:

        state = parse_request_line(
            msg->buffer->start, msg->buffer->length, &msg->request,
            &msg->version, msg->config.header.max_bytes_method_name, &nxt);
    }

    switch (state) {

    case OV_SIP_PARSER_PROGRESS:
        return OV_SIP_PARSER_PROGRESS;

    case OV_SIP_PARSER_SUCCESS:
        break;

    default:
        goto error;
    }

    OV_ASSERT(nxt);

    state = parse_header(
        nxt, msg->buffer->length - (nxt - msg->buffer->start), msg->header,
        msg->config.header.capacity, msg->config.header.max_bytes_line, &nxt);

    switch (state) {

    case OV_SIP_PARSER_PROGRESS:
        return OV_SIP_PARSER_PROGRESS;

    case OV_SIP_PARSER_SUCCESS:
        break;

    case OV_SIP_PARSER_OOB:

        ov_log_error("SIP header OOB with %zu max config header fields,"
                     " cannot parse header",
                     msg->config.header.capacity);

        ov_buffer_dump(stderr, msg->buffer);
        goto error;

    default:
        goto error;
    }

    /* HEADER COMPLETE */

    state = parse_body(msg, &nxt);

    switch (state) {

    case OV_SIP_PARSER_PROGRESS:
        break;

    case OV_SIP_PARSER_SUCCESS:
        break;

    default:
        goto error;
    }

    if (next)
        *next = nxt;

    return state;

error:
    if (next && msg && msg->buffer)
        *next = msg->buffer->start;

    return OV_SIP_PARSER_ERROR;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static ov_sip_message *message_create(ov_registered_cache *cache, ov_sip_message_config config){

    ov_sip_message *msg = ov_registered_cache_get(cache);

    if (msg) {

        /* We need to ensure a cached msg is in line with the current config */

        if (msg->config.header.capacity < config.header.capacity)
            msg = ov_sip_message_free(msg);
    }

    if (!msg) {

        msg = calloc(1, sizeof(ov_sip_message) +
                            config.header.capacity * sizeof(ov_sip_header));

        if (!msg)
            goto error;

        msg->magic_byte = OV_SIP_MESSAGE_MAGIC_BYTE;
    }

    OV_ASSERT(ov_sip_message_cast(msg));

    /* Perform a (re)init to zero in any case */

    for (size_t i = 0; i < config.header.capacity; i++) {
        msg->header[i] = (ov_sip_header){0};
    }

    if (msg->buffer) {

        OV_ASSERT(ov_buffer_cast(msg->buffer));

        if (msg->buffer->capacity < config.buffer.default_size) {
            if (!ov_buffer_extend(msg->buffer, config.buffer.default_size -
                                                   msg->buffer->capacity))
                msg->buffer = ov_buffer_free(msg->buffer);
        }

        ov_buffer_clear(msg->buffer);

    } else {

        msg->buffer = ov_buffer_create(config.buffer.default_size);
        if (!msg->buffer)
            goto error;
    }

    msg->version = (ov_sip_version){0};
    msg->request = (ov_sip_request){0};
    msg->status = (ov_sip_status){0};
    msg->body = (ov_memory_pointer){0};

    msg->config = config;

    return msg;
error:
    msg = ov_sip_message_free_uncached(msg);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_sip_message_config
ov_sip_message_config_init(ov_sip_message_config config) {

    if (0 == config.header.capacity)
        config.header.capacity = IMPL_DEFAULT_HEADER_CAPACITY;

    if (0 == config.buffer.default_size)
        config.buffer.default_size = IMPL_DEFAULT_BUFFER_SIZE;

    if (0 == config.header.max_bytes_method_name)
        config.header.max_bytes_method_name = IMPL_DEFAULT_MAX_METHOD_NAME;

    if (0 == config.header.max_bytes_line)
        config.header.max_bytes_line = IMPL_DEFAULT_MAX_HEADER_LINE;

    return config;
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_create(ov_sip_message_config config) {

    config = ov_sip_message_config_init(config);

    ov_sip_message *msg = message_create(g_cache, config);

    if (!msg) {
        ov_log_error("Failed to create a HTTP message structure");
        return NULL;
    }

    OV_ASSERT(ov_sip_message_cast(msg));
    OV_ASSERT(msg->buffer);

    return msg;
}

/*----------------------------------------------------------------------------*/

bool ov_sip_message_clear(ov_sip_message *msg){

    if (!ov_sip_message_cast(msg))
        return false;

    for (size_t i = 0; i < msg->config.header.capacity; i++) {
        msg->header[i] = (ov_sip_header){0};
    }

    msg->version = (ov_sip_version){0};
    msg->request = (ov_sip_request){0};
    msg->status = (ov_sip_status){0};
    msg->body = (ov_memory_pointer){0};

    if (msg->buffer) {
        ov_buffer_clear(msg->buffer);
    }

    return true;
}

/*----------------------------------------------------------------------------*/

void *ov_sip_message_free_uncached(void *message) {

    ov_sip_message *msg = ov_sip_message_cast(message);
    if (!msg)
        return message;

    msg->buffer = ov_buffer_free_uncached(msg->buffer);
    msg = ov_data_pointer_free(msg);

    return msg;
}

/*----------------------------------------------------------------------------*/

void *ov_sip_message_free(void *message) {

    ov_sip_message *msg = ov_sip_message_cast(message);
    if (!msg)
        return message;

    ov_sip_message_clear(msg);
    msg = ov_registered_cache_put(g_cache, msg);

    return ov_sip_message_free_uncached(msg);
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_cast(void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data == OV_SIP_MESSAGE_MAGIC_BYTE)
        return (ov_sip_message *)data;

    return NULL;
}

/*----------------------------------------------------------------------------*/

void ov_sip_enable_caching(size_t capacity) {

    ov_registered_cache_config cfg = {

        .capacity = capacity,
        .item_free = ov_sip_message_free_uncached,

    };

    g_cache = ov_registered_cache_extend("sip_message", cfg);
}

/*----------------------------------------------------------------------------*/

const ov_sip_header *ov_sip_header_get_next(const ov_sip_header *array,
                                              size_t size, size_t *index,
                                              const char *name) {

    if (!array || !name || (size == 0) || !index)
        goto error;

    const ov_sip_header *header = NULL;

    size_t len = strlen(name);

    for (size_t i = *index; i < size; i++) {

        header = &array[i];
        if (!header)
            continue;

        if (!header->name.start)
            continue;

        if (len != header->name.length)
            continue;

        if (0 == strncasecmp(name, (char *)header->name.start,
                             header->name.length)) {
            *index = i + 1;
            return header;
        }
    }

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

const ov_sip_header *ov_sip_header_get_unique(const ov_sip_header *array,
                                                size_t size, const char *name) {

    const ov_sip_header *header = NULL;

    if (!array || !name || (size == 0))
        goto error;

    size_t index = 0;
    header = ov_sip_header_get_next(array, size, &index, name);
    if (!header)
        goto error;

    if (ov_sip_header_get_next(array, size, &index, name))
        goto error;

    return header;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

const ov_sip_header *ov_sip_header_get(const ov_sip_header *array,
                                         size_t size, const char *name) {

    size_t index = 0;
    return ov_sip_header_get_next(array, size, &index, name);
}

/*----------------------------------------------------------------------------*/

bool ov_sip_message_shift_trailing_bytes(ov_sip_message *source,
                                          uint8_t *next,
                                          ov_sip_message **dest) {

    ov_sip_message *new = NULL;

    if (!source || !dest || !next)
        goto error;

    if (next == source->buffer->start + source->buffer->length)
        goto done;

    new = ov_sip_message_create(source->config);
    if (!new)
        goto error;

    size_t len = source->buffer->length - (next - source->buffer->start);

    if (!ov_buffer_set(new->buffer, next, len))
        goto error;

    if (!memset(next, 0, len))
        goto error;

    source->buffer->length = (next - source->buffer->start);

done:
    *dest = new;
    return true;
error:
    new = ov_sip_message_free(new);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_sip_message_ensure_open_capacity(ov_sip_message *msg,
                                          size_t capacity) {

    if (!msg || (0 == capacity))
        goto error;

    if (!msg->buffer || !msg->buffer->start)
        goto error;

    size_t length = msg->buffer->length;
    size_t open = msg->buffer->capacity - length;

    if (open >= capacity)
        goto done;

    /* We need to reallocate */

    if (!ov_buffer_extend(msg->buffer, capacity - open))
        goto error;

done:
    OV_ASSERT(length == msg->buffer->length);
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_create_status(ov_sip_message_config config,
                                              ov_sip_version version,
                                              uint16_t code,
                                              const char *phrase) {

    ov_sip_message *msg = ov_sip_message_create(config);
    if (!msg)
        goto error;

    if (NULL == phrase)
        goto error;

    if ((code < 100) || (code > 999))
        goto error;

    memset(msg->buffer->start, 0, msg->buffer->capacity);

    ssize_t bytes = snprintf((char *)msg->buffer->start, msg->buffer->capacity,
                             "SIP/%i.%i %i %s\r\n", version.major,
                             version.minor, code, phrase);

    if (bytes < 0)
        goto error;

    if (bytes == (ssize_t)msg->buffer->capacity)
        goto error;

    msg->buffer->length = bytes;
    return msg;
error:
    ov_sip_message_free(msg);
    return NULL;
}


/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_create_request(ov_sip_message_config config,
                                               ov_sip_version version,
                                               const char *method,
                                               const char *uri) {

    ov_sip_message *msg = ov_sip_message_create(config);
    if (!msg)
        goto error;

    if (!method || !uri)
        goto error;

    if (strlen(method) > msg->config.header.max_bytes_method_name)
        goto error;

    if (strlen(uri) > PATH_MAX)
        goto error;

    memset(msg->buffer->start, 0, msg->buffer->capacity);

    ssize_t bytes = snprintf((char *)msg->buffer->start, msg->buffer->capacity,
                             "%s %s SIP/%i.%i\r\n", method, uri, version.major,
                             version.minor);

    if (bytes < 0)
        goto error;

    if (bytes == (ssize_t)msg->buffer->capacity)
        goto error;

    msg->buffer->length = bytes;
    return msg;
error:
    ov_sip_message_free(msg);
    return NULL;
}


/*----------------------------------------------------------------------------*/

bool ov_sip_message_add_header(ov_sip_message *msg, const char *key,
                                       const char *val) {

    if (!msg || !key || !val)
        goto error;

    /* Message MUST have some valid startline */

    uint8_t *ptr = NULL;

    if (OV_SIP_PARSER_SUCCESS != parse_status_line(
                                      msg->buffer->start, msg->buffer->length,
                                      &msg->status, &msg->version, &ptr))
        if (OV_SIP_PARSER_SUCCESS !=
            parse_request_line(
                msg->buffer->start, msg->buffer->length, &msg->request,
                &msg->version, msg->config.header.max_bytes_method_name, &ptr))
            goto error;

    ssize_t open = msg->buffer->capacity - msg->buffer->length;
    ssize_t bytes = -1;

    /*  Some headers may have been added previously.
     *
     *  We do NOT recheck the whole buffer for performance,
     *  as it is expected to be done by the user of the function
     *  for final verification of the message, before sending.
     */
    ptr = msg->buffer->start + msg->buffer->length;

    while (true) {

        bytes = snprintf((char *)ptr, open, "%s:%s\r\n", key, val);

        if (bytes < 0)
            goto error;

        if (bytes > (ssize_t)msg->config.header.max_bytes_line) {
            memset(ptr, 0, open);
            goto error;
        }

        if (bytes < open)
            break;

        open += msg->config.buffer.default_size;

        if (!ov_sip_message_ensure_open_capacity(msg, open))
            goto error;

        ptr = msg->buffer->start + msg->buffer->length;
        open = msg->buffer->capacity - msg->buffer->length;
    }

    msg->buffer->length += bytes;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_sip_message_add_ptr_header(ov_sip_message *msg, const char *key,
                                       const ov_memory_pointer *val) {

    if (!msg || !key || !val)
        goto error;

    /* Message MUST have some valid startline */

    uint8_t *ptr = NULL;

    if (OV_SIP_PARSER_SUCCESS != parse_status_line(
                                      msg->buffer->start, msg->buffer->length,
                                      &msg->status, &msg->version, &ptr))
        if (OV_SIP_PARSER_SUCCESS !=
            parse_request_line(
                msg->buffer->start, msg->buffer->length, &msg->request,
                &msg->version, msg->config.header.max_bytes_method_name, &ptr))
            goto error;

    ssize_t open = msg->buffer->capacity - msg->buffer->length;
    ssize_t bytes = -1;

    /*  Some headers may have been added previously.
     *
     *  We do NOT recheck the whole buffer for performance,
     *  as it is expected to be done by the user of the function
     *  for final verification of the message, before sending.
     */
    ptr = msg->buffer->start + msg->buffer->length;

    while (true) {

        bytes = snprintf((char *)ptr, open, "%s:%.*s\r\n", key, 
            (int)val->length,
            (char*)val->start);

        if (bytes < 0)
            goto error;

        if (bytes > (ssize_t)msg->config.header.max_bytes_line) {
            memset(ptr, 0, open);
            goto error;
        }

        if (bytes < open)
            break;

        open += msg->config.buffer.default_size;

        if (!ov_sip_message_ensure_open_capacity(msg, open))
            goto error;

        ptr = msg->buffer->start + msg->buffer->length;
        open = msg->buffer->capacity - msg->buffer->length;
    }

    msg->buffer->length += bytes;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_sip_message_add_header_copy(ov_sip_message *msg, const ov_sip_header *header){

    /* Message MUST have some valid startline */

    if (!msg || !header) goto error;
    if (!header->name.start) goto error;
    if (!header->value.start) goto error;

    uint8_t *ptr = NULL;

    if (OV_SIP_PARSER_SUCCESS != parse_status_line(
                                      msg->buffer->start, msg->buffer->length,
                                      &msg->status, &msg->version, &ptr))
        if (OV_SIP_PARSER_SUCCESS !=
            parse_request_line(
                msg->buffer->start, msg->buffer->length, &msg->request,
                &msg->version, msg->config.header.max_bytes_method_name, &ptr))
            goto error;

    ssize_t open = msg->buffer->capacity - msg->buffer->length;
    ssize_t bytes = -1;

    /*  Some headers may have been added previously.
     *
     *  We do NOT recheck the whole buffer for performance,
     *  as it is expected to be done by the user of the function
     *  for final verification of the message, before sending.
     */
    ptr = msg->buffer->start + msg->buffer->length;

    while (true) {

        bytes = snprintf((char *)ptr, open, "%.*s:%.*s\r\n", 
            (int) header->name.length,
            (char*) header->name.start, 
            (int) header->value.length,
            (char*) header->value.start);

        if (bytes < 0)
            goto error;

        if (bytes > (ssize_t)msg->config.header.max_bytes_line) {
            memset(ptr, 0, open);
            goto error;
        }

        if (bytes < open)
            break;

        open += msg->config.buffer.default_size;

        if (!ov_sip_message_ensure_open_capacity(msg, open))
            goto error;

        ptr = msg->buffer->start + msg->buffer->length;
        open = msg->buffer->capacity - msg->buffer->length;
    }

    msg->buffer->length += bytes;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_sip_message_close_header(ov_sip_message *msg) {

    if (!msg)
        goto error;

    uint8_t *ptr = msg->buffer->start + msg->buffer->length;
    ssize_t open = msg->buffer->capacity - msg->buffer->length;

    if (open < 3) {

        open += msg->config.buffer.default_size;
        if (!ov_sip_message_ensure_open_capacity(msg, open))
            goto error;
    }

    ptr = msg->buffer->start + msg->buffer->length;

    ssize_t bytes = snprintf((char *)ptr, open, "\r\n");

    if (bytes != 2)
        goto error;

    msg->buffer->length += bytes;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_sip_message_add_body(ov_sip_message *msg, ov_memory_pointer body) {

    if (!msg)
        goto error;

    if ((NULL == body.start) || (0 == body.length))
        goto error;

    uint8_t *ptr = msg->buffer->start + msg->buffer->length;

    if ((ptr - msg->buffer->start) < 5)
        goto error;

    if (ptr[-1] != '\n')
        goto error;

    if (ptr[-2] != '\r')
        goto error;

    if (ptr[-3] != '\n')
        goto error;

    if (ptr[-4] != '\r')
        goto error;

    if (!ov_sip_message_ensure_open_capacity(msg, body.length))
        goto error;

    ptr = msg->buffer->start + msg->buffer->length;

    if (!memcpy(ptr, body.start, body.length))
        goto error;

    msg->buffer->length += body.length;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_sip_message_add_body_string(ov_sip_message *msg, const char *body) {

    if (!msg || !body)
        goto error;

    if (0 == strlen(body))
        goto error;

    uint8_t *ptr = msg->buffer->start + msg->buffer->length;
    ssize_t open = msg->buffer->capacity - msg->buffer->length;

    if ((ptr - msg->buffer->start) < 5)
        goto error;

    if (ptr[-1] != '\n')
        goto error;

    if (ptr[-2] != '\r')
        goto error;

    if (ptr[-3] != '\n')
        goto error;

    if (ptr[-4] != '\r')
        goto error;

    ssize_t bytes = -1;

    while (true) {

        bytes = snprintf((char *)ptr, open, "%s", body);

        if (bytes < 0)
            goto error;

        if (bytes < open)
            break;

        open += msg->config.buffer.default_size;

        if (!ov_sip_message_ensure_open_capacity(msg, open))
            goto error;

        ptr = msg->buffer->start + msg->buffer->length;
        open = msg->buffer->capacity - msg->buffer->length;
    }

    msg->buffer->length += bytes;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_sip_is_request(const ov_sip_message *msg, const char *method) {

    if (!msg || !method)
        goto error;

    /* response ? */
    if (msg->status.code != 0)
        goto error;

    if (NULL == msg->request.method.start)
        goto error;

    if (0 != strncasecmp(method, (char *)msg->request.method.start,
                         msg->request.method.length))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_sip_message_set_date(ov_sip_message *msg) {

    if (!msg)
        goto error;

    char timestamp[35] = {0};
    if (!ov_imf_write_timestamp(timestamp, 35, NULL))
        goto error;

    if (!ov_sip_message_add_header(msg, "Date", timestamp))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_sip_message_set_content_length(ov_sip_message *msg, size_t length) {

    char string[25] = {0};
    if (!msg)
        goto error;

    if (!snprintf(string, 25, "%zu", length))
        goto error;

    if (!ov_sip_message_add_header(msg, OV_SIP_CONTENT_LENGTH, string))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_sip_message_add_content_type(ov_sip_message *msg, const char *mime,
                                      const char *charset) {

    if (!msg || !mime)
        return false;

    const char *charset_base = "charset=";

    size_t size = strlen(mime) + 1;
    if (charset)
        size += strlen(charset) + strlen(charset_base) + 1;

    char buf[size];
    memset(buf, 0, size);

    if (charset) {

        if (!snprintf(buf, size, "%s;%s%s", mime, charset_base, charset))
            goto error;

        if (!ov_sip_message_add_header(msg, OV_SIP_CONTENT_TYPE, buf))
            goto error;

    } else {

        if (!ov_sip_message_add_header(msg, OV_SIP_CONTENT_TYPE, mime))
            goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_pop(ov_buffer **input,
                                     const ov_sip_message_config *config,
                                     ov_sip_parser_state *state) {

    ov_sip_message *msg = NULL;
    ov_buffer *new_buffer = NULL;
    ov_sip_parser_state s = OV_SIP_PARSER_ERROR;

    if (!input || !state || !config)
        goto error;

    ov_buffer *buffer = *input;
    if (!ov_buffer_cast(buffer))
        goto error;

    if (0 == buffer->start[0]) {
        s = OV_SIP_PARSER_PROGRESS;
        new_buffer = buffer;
        goto done;
    }

    msg = calloc(1, sizeof(ov_sip_message) +
                        config->header.capacity * sizeof(ov_sip_header));

    if (!msg)
        goto error;

    msg->magic_byte = OV_SIP_MESSAGE_MAGIC_BYTE;
    msg->config = *config;
    msg->buffer = buffer;

    uint8_t *next = NULL;

    s = ov_sip_parse_message(msg, &next);

    switch (s) {

    case OV_SIP_PARSER_PROGRESS:
        msg->buffer = NULL;
        new_buffer = buffer;
        goto done;

    case OV_SIP_PARSER_SUCCESS:
        break;

    default:
        msg->buffer = NULL;
        goto error;
    }

    if (next == msg->buffer->start + msg->buffer->length)
        goto done;

    size_t len = msg->buffer->length - (next - msg->buffer->start);
    new_buffer = ov_buffer_create(len);

    if (!ov_buffer_set(new_buffer, next, len))
        goto error;

    if (!memset(next, 0, len))
        goto error;

    msg->buffer->length = (next - msg->buffer->start);

done:
    *state = s;
    *input = new_buffer;

    return msg;
error:
    msg = ov_sip_message_free(msg);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_sip_parser_state ov_sip_parse_message_buffer(
        const uint8_t *start,
        size_t len,
        uint8_t **next,
        ov_sip_message **out){

    ov_sip_message *msg = ov_sip_message_create((ov_sip_message_config){0});
    if (!msg) goto error;

    ov_buffer_push(msg->buffer, (uint8_t*) start, len);

    uint8_t *nxt = NULL;

    ov_sip_parser_state state = ov_sip_parse_message(msg, &nxt);

    *next = (uint8_t*) start + (nxt - msg->buffer->start);

    switch (state) {

        case OV_SIP_PARSER_SUCCESS:

            memset(nxt, 0, msg->buffer->length - (nxt - msg->buffer->start));
            msg->buffer->length = nxt - msg->buffer->start;

            *out = msg;

            return state;

        default:
            break;
    }

    *out = NULL;
    msg = ov_sip_message_free(msg);
    return state;
error:
    return OV_SIP_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

static ov_sip_message *sip_message(const char *head) {

    ov_sip_message *msg = ov_sip_message_create((ov_sip_message_config){0});
    memcpy(msg->buffer->start, head, msg->buffer->capacity);
    msg->buffer->length = strlen(head);
    return msg;

}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_register(const char *uri){

    char msg[1024] = {0};
    snprintf(msg, 1024, "REGISTER sip:%s SIP/2.0\r\n", uri);

    return sip_message(msg);
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_options(const char *uri){

    char msg[1024] = {0};
    snprintf(msg, 1024, "OPTIONS sip:%s SIP/2.0\r\n", uri);

    return sip_message(msg);
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_bye(const char *uri){

    char msg[1024] = {0};
    snprintf(msg, 1024, "BYE sip:%s SIP/2.0\r\n", uri);

    return sip_message(msg);
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_invite(const char *uri){

    char msg[1024] = {0};
    snprintf(msg, 1024, "INVITE sip:%s SIP/2.0\r\n", uri);

    return sip_message(msg);
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_ack(const char *uri){

    char msg[1024] = {0};
    snprintf(msg, 1024, "ACK sip:%s SIP/2.0\r\n", uri);

    return sip_message(msg);
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_cancel(const char *uri){

    char msg[1024] = {0};
    snprintf(msg, 1024, "CANCEL sip:%s SIP/2.0\r\n", uri);

    return sip_message(msg);
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_response(uint64_t code, const char *phrase){

    char msg[1024] = {0};
    snprintf(msg, 1024, "SIP/2.0 %"PRIu64" %s\r\n", code, phrase);

    return sip_message(msg);
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_copy(const ov_sip_message *in){

    ov_sip_message *out = NULL;
    uint8_t *next = NULL;

    ov_sip_parse_message_buffer(in->buffer->start, in->buffer->length, &next, &out);
    
    UNUSED(next);
    return out;
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_copy_header(const ov_sip_message *in){

    char msg[1024] = {0};

    snprintf(msg, 1024, "%.*s %.*s SIP/2.0\r\n",
        (int) in->request.method.length,
        (char*) in->request.method.start,
        (int)in->request.uri.length,
        (char*) in->request.uri.start);

    ov_sip_message *out = sip_message(msg);

    const ov_sip_header *header = NULL;
    for (size_t i = 0; i < in->config.header.capacity; i++){

        header = &in->header[i];

        if (!header)
            continue;

        if (!header->name.start)
            continue;

        ov_sip_message_add_header_copy(out, header);

    }
    
    return out;
}

/*----------------------------------------------------------------------------*/

static bool process_cseq(const ov_sip_message *msg, 
    uint64_t *cseq, 
    char **method){

    const ov_sip_header *header = ov_sip_header_get_unique(
        msg->header, msg->config.header.capacity, "Cseq");

    if (!header) goto error;

    char *end = NULL;
    uint64_t cs = strtol((char*)header->value.start, &end, 10);
    if (!end) goto error;

    *cseq = cs;

    if (end[0] != ' ') goto error;
    *method = end + 2;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_ack_from_msg(
    const ov_sip_message *in,
    const char *via){

    char msg[1024] = {0};

    snprintf(msg, 1024, "ACK %.*s SIP/2.0\r\n",
        (int)in->request.uri.length,
        (char*) in->request.uri.start);

    uint64_t cseq = 0;
    char *method = NULL;

    if (!process_cseq(in, &cseq, &method)) goto error;

    ov_sip_message *out = sip_message(msg);

    const ov_sip_header *header = NULL;
    for (size_t i = 0; i < in->config.header.capacity; i++){

        header = &in->header[i];

        if (!header)
            continue;

        if (!header->name.start)
            continue;

        if (0 == strncmp("Via", (char*)header->name.start, header->name.length))
            continue;

        if (0 == strncmp("Cseq", (char*)header->name.start, header->name.length))
            continue;

        ov_sip_message_add_header_copy(out, header);

    }

    memset(msg, 0, 1024);
    snprintf(msg, 1024, "%"PRIu64" ACK", cseq);
    ov_sip_message_add_header(out, "Cseq", msg);

    ov_sip_message_add_header(out, "Via", via);
    ov_sip_message_close_header(out);

    ov_sip_parse_message(out, NULL);
    return out;
error:
    return NULL;
}