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
        @file           ov_http_pointer.c
        @author         Markus Töpfer

        @date           2020-12-10


        ------------------------------------------------------------------------
*/
#include "../include/ov_http_pointer.h"

#include <ctype.h>
#include <limits.h>
#include <ov_base/ov_utils.h>
#include <stdbool.h>
#include <strings.h>

#include <ov_base/ov_string.h>
#include <ov_base/ov_uri.h>

#include "../include/ov_imf.h"
#include <ov_base/ov_constants.h>
#include <ov_base/ov_registered_cache.h>
#include <ov_stun/ov_stun_grammar.h>

#define IMPL_DEFAULT_HEADER_CAPACITY 100
#define IMPL_DEFAULT_BUFFER_SIZE 4096
#define IMPL_DEFAULT_MAX_BUFFER_RECACHE IMPL_DEFAULT_BUFFER_SIZE * 10
#define IMPL_DEFAULT_MAX_TRANSFER 10
#define IMPL_DEFAULT_MAX_METHOD_NAME 7    // max of HTTP 1.1 methods
#define IMPL_DEFAULT_MAX_HEADER_LINE 1000 // max bytes of a line

static ov_registered_cache *g_cache = 0;

/*----------------------------------------------------------------------------*/

static bool http_is_separator(uint8_t item) {

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

static bool http_is_whitespace(uint8_t item) {

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

static bool http_is_control(uint8_t item) {

    if (item < 32) return true;

    if (item == 127) return true;

    return false;
}

/*----------------------------------------------------------------------------*/

static bool http_is_token(uint8_t item) {

    // token          = 1*<any CHAR except CTLs or separators>

    if (http_is_control(item)) return false;

    if (http_is_separator(item)) return false;

    if (item < 127) return true;

    return false;
}

/*----------------------------------------------------------------------------*/

static bool http_is_token_string(const uint8_t *start, size_t length) {

    if (!start || length == 0) return false;

    for (size_t i = 0; i < length; i++) {

        if (!http_is_token(start[i])) return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static bool http_is_linear_whitespace(const uint8_t *start, size_t length) {

    if (!start || length < 3) return false;

    if ((start[0] == '\r') && (start[1] == '\n') &&
        http_is_whitespace(start[2]))
        return true;

    return false;
}

/*----------------------------------------------------------------------------*/

static const uint8_t *http_offset_linear_whitespace(const uint8_t *start,
                                                    size_t length) {

    if (!start || length < 3) return start;

    if (http_is_linear_whitespace(start, length)) return start + 3;

    return start;
}

/*----------------------------------------------------------------------------*/

static bool http_is_text(const uint8_t *start, size_t length) {

    const uint8_t *ptr = start;
    const uint8_t *nxt = NULL;
    size_t size = length;

    while (size > 0) {

        if (size >= 3) {

            nxt = ptr;
            nxt = http_offset_linear_whitespace(ptr, size);

            if (ptr != nxt) {
                ptr = nxt;
                size -= 3;
                continue;
            }
        }

        if (http_is_whitespace(*ptr)) {

            ptr++;
            size--;
            continue;
        }

        if (http_is_control(*ptr)) return false;

        ptr++;
        size--;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static bool http_is_reason_phrase(const uint8_t *start, size_t length) {

    for (size_t i = 0; i < length; i++) {

        switch (start[i]) {

            case ' ':
                continue;
            case '\t':
                continue;

            default:
                break;
        }

        if (http_is_control(start[i])) return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static bool http_is_quoted_pair(const uint8_t *start, size_t length) {

    if (!start) goto error;

    if (2 > length) goto error;

    // quoted-pair = "\" ( HTAB / SP / VCHAR / obs-text )
    // obs-text = %x80-FF

    if (start[0] != 0x5C) goto error;

    if (http_is_whitespace(start[1])) return true;

    if (http_is_control(start[1])) return false;

    // VCHAR 32 (space -> 0xff without 127 (DEL))

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool http_is_qd_char(const uint8_t item) {

    /*
        qdtext = HTAB / SP / "!" / %x23-5B ; '#'-'['
        / %x5D-7E ; ']'-'~'
        / obs-text
    */

    // obs text
    if (item >= 0x80) return true;

    if (item == 0x5C) return false;

    if ((item >= 0x23) && (item <= 0x7E)) return true;

    if (http_is_whitespace(item)) return true;

    switch (item) {

        case '!':
        case '#':
        case '-':
        case '[':
        case ']':
        case '~':
            return true;
        default:
            break;
    }

    return false;
}

/*----------------------------------------------------------------------------*/

static bool http_is_quoted_string(const uint8_t *start, size_t length) {

    if (!start) return false;

    if (length < 2) return false;

    // quoted-string = DQUOTE *( qdtext / quoted-pair ) DQUOTE

    if (start[0] != 0x22) return false;

    if (start[length - 1] != 0x22) return false;

    // empty quoted string
    if (length == 2) return true;

    size_t open = length - 2;
    uint8_t *ptr = (uint8_t *)start + 1;

    while (open > 0) {

        if (http_is_qd_char(ptr[0])) {
            ptr++;
            open--;
            continue;
        }

        if (open < 2) goto error;

        if (!http_is_quoted_pair(ptr, open)) goto error;

        ptr += 2;
        open -= 2;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool http_is_field_content(const uint8_t *start, size_t length) {

    return http_is_text(start, length);
}

/*----------------------------------------------------------------------------*/

ov_http_parser_state ov_http_pointer_parse_version(const uint8_t *buffer,
                                                   size_t length,
                                                   ov_http_version *version) {

    //                   0123   4  5        6  7
    // HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT

    if (!buffer || !version) goto error;

    if (0 == length) return OV_HTTP_PARSER_PROGRESS;

    const uint8_t *ptr = (uint8_t *)buffer;
    size_t size = length;

    // remove leading whitespace
    while (http_is_whitespace(*ptr)) {

        if ((size_t)(ptr - buffer) >= size) break;

        ptr++;
        size--;
    }

    size_t len = 5;
    if (len > size) len = size;

    if (0 != strncasecmp((char *)ptr, "HTTP/", len)) goto error;

    if (length < 6) return OV_HTTP_PARSER_PROGRESS;

    if (!isdigit(ptr[5])) goto error;

    if (length < 7) return OV_HTTP_PARSER_PROGRESS;

    if ('.' != ptr[6]) goto error;

    if (size < 8) return OV_HTTP_PARSER_PROGRESS;

    if (!isdigit(ptr[7])) goto error;

    version->major = ptr[5] - 48;
    version->minor = ptr[7] - 48;

    return OV_HTTP_PARSER_SUCCESS;

error:
    return OV_HTTP_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

ov_http_parser_state ov_http_pointer_parse_status_line(const uint8_t *buffer,
                                                       size_t length,
                                                       ov_http_status *status,
                                                       ov_http_version *version,
                                                       uint8_t **next) {

    //                            8  9 10 11     12
    // Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF

    if (!buffer || !status || !version) goto error;

    const uint8_t *ptr = (uint8_t *)buffer;
    size_t size = length;

    // remove leading whitespace
    while (http_is_whitespace(*ptr)) {

        if ((size_t)(ptr - buffer) >= size) break;

        ptr++;
        size--;
    }

    ov_http_parser_state state =
        ov_http_pointer_parse_version(ptr, size, version);

    switch (state) {

        case OV_HTTP_PARSER_PROGRESS:
            return OV_HTTP_PARSER_PROGRESS;

        case OV_HTTP_PARSER_SUCCESS:
            break;

        default:
            goto error;
    }

    if (8 == size) return OV_HTTP_PARSER_PROGRESS;

    if (' ' != ptr[8]) goto error;

    if (9 == size) return OV_HTTP_PARSER_PROGRESS;

    size_t len = 3;
    if (len > size - 9) len = size - 9;

    for (size_t i = 0; i < len; i++) {

        if (!isdigit(ptr[9 + i])) goto error;
    }

    if (size < 13) return OV_HTTP_PARSER_PROGRESS;

    if (' ' != ptr[12]) goto error;

    if (size == 13) return OV_HTTP_PARSER_PROGRESS;

    // parse code
    const uint8_t *start = ptr + 9;
    uint8_t *end = NULL;

    status->code = strtol((char *)start, (char **)&end, 10);
    if (!end) goto error;

    if (3 != (end - start)) goto error;

    // parse phrase
    start = ptr + 13;
    end = (uint8_t *)ov_string_find(
        (char *)start, size - (start - ptr) + 1, "\r\n", 2);
    if (!end) return OV_HTTP_PARSER_PROGRESS;

    if ((end - start) < 1) goto error;

    len = (end - start);

    if (!http_is_reason_phrase(start, len)) goto error;

    status->phrase.start = start;
    status->phrase.length = len;

    if (next) *next = end + 2;

    return OV_HTTP_PARSER_SUCCESS;

error:
    return OV_HTTP_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

ov_http_parser_state ov_http_pointer_parse_request_line(
    const uint8_t *buffer,
    size_t length,
    ov_http_request *request,
    ov_http_version *version,
    uint32_t max_method_name,
    uint8_t **next) {

    // Request-Line   = Method SP Request-URI SP HTTP-Version CRLF

    if (!buffer || !request) goto error;

    const uint8_t *ptr = buffer;
    uint8_t *end = NULL;
    const uint8_t *lineend =
        (uint8_t *)ov_string_find((char *)buffer, length, "\r", 1);

    size_t size = length;

    // remove leading whitespace
    while (http_is_whitespace(*ptr)) {

        if ((size_t)(ptr - buffer) >= size) break;

        ptr++;
        size--;
    }

    end = memchr(ptr, ' ', size);
    if (!end) {

        if (0 != max_method_name)
            if (size >= max_method_name) goto error;

        if (lineend) goto error;

        return OV_HTTP_PARSER_PROGRESS;

    } else {

        if (0 != max_method_name)
            if ((end - ptr) > max_method_name) goto error;

        if (lineend && ((lineend - buffer) < (end - buffer))) goto error;
    }

    if (end[0] != ' ') goto error;

    request->method.start = (uint8_t *)ptr;
    request->method.length = (end - ptr);

    if (!http_is_token_string(request->method.start, request->method.length))
        goto error;

    size = size - request->method.length - 1;
    ptr = end + 1;

    if (size < 1) return OV_HTTP_PARSER_PROGRESS;

    end = memchr(ptr, ' ', size);
    if (!end) {

        if (size > PATH_MAX) goto error;

        if (lineend) goto error;

        return OV_HTTP_PARSER_PROGRESS;

    } else {

        if ((end - ptr) > PATH_MAX) goto error;

        if (lineend && ((lineend - buffer) < (end - buffer))) goto error;
    }

    if (end[0] != ' ') goto error;

    request->uri.start = (uint8_t *)ptr;
    request->uri.length = (end - ptr);

    if (!ov_uri_string_is_valid(
            (char *)request->uri.start, request->uri.length))
        goto error;

    size = size - request->uri.length - 1;
    ptr = end + 1;

    if (size < 1) return OV_HTTP_PARSER_PROGRESS;

    switch (ov_http_pointer_parse_version(ptr, size, version)) {

        case OV_HTTP_PARSER_PROGRESS:
            return OV_HTTP_PARSER_PROGRESS;

        case OV_HTTP_PARSER_SUCCESS:
            break;

        default:
            goto error;
    }

    if (size < 10) return OV_HTTP_PARSER_PROGRESS;

    if (ptr[8] != '\r') goto error;

    if (ptr[9] != '\n') goto error;

    if (next) *next = (uint8_t *)ptr + 10;

    return OV_HTTP_PARSER_SUCCESS;

error:
    return OV_HTTP_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

ov_http_parser_state ov_http_pointer_parse_header(const uint8_t *buffer,
                                                  size_t length,
                                                  ov_http_header *array,
                                                  size_t array_size,
                                                  uint32_t max,
                                                  uint8_t **next) {

    if (!buffer || !array) goto error;

    size_t size = length;
    const uint8_t *ptr = buffer;
    uint8_t *nxt = NULL;

    if (next) *next = (uint8_t *)buffer;

    size_t i = 0;
    ov_http_parser_state state = OV_HTTP_PARSER_ERROR;

    for (i = 0; i < array_size; i++) {

        state =
            ov_http_pointer_parse_header_line(ptr, size, &array[i], max, &nxt);

        switch (state) {

            case OV_HTTP_PARSER_SUCCESS:
                size -= nxt - ptr;
                ptr = nxt;
                continue;

            case OV_HTTP_PARSER_PROGRESS:
                return state;

            default:
                break;
        }

        /* No more header line, check for \r\n */

        if (size == 0) return OV_HTTP_PARSER_PROGRESS;

        if (size > 0)
            if (ptr[0] != '\r') goto error;

        if (size > 1) {

            if (ptr[1] != '\n') goto error;

            if (next) *next = nxt + 2;

            return OV_HTTP_PARSER_SUCCESS;

        } else {

            return OV_HTTP_PARSER_PROGRESS;
        }
    }
    return OV_HTTP_PARSER_OOB;
error:
    if (next) *next = (uint8_t *)buffer;
    return OV_HTTP_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

ov_http_parser_state ov_http_pointer_parse_header_line(const uint8_t *buffer,
                                                       size_t length,
                                                       ov_http_header *line,
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

    if (!buffer || !line) goto error;

    const uint8_t *ptr = buffer;
    uint8_t *end = NULL;
    const uint8_t *lineend =
        (uint8_t *)ov_string_find((char *)buffer, length, "\r", 1);

    if (lineend && (0 != max))
        if ((lineend - buffer) > max) goto error;

    size_t size = length;

    // remove leading whitespace
    while (http_is_whitespace(*ptr)) {

        if ((size_t)(ptr - buffer) >= size) break;

        ptr++;
        size--;
    }

    end = memchr(ptr, ':', size);
    if (!end) {

        if (lineend) goto error;

        if ((max != 0) && size > max) goto error;

        return OV_HTTP_PARSER_PROGRESS;

    } else {

        if (lineend && ((lineend - buffer) < (end - buffer))) goto error;
    }

    if (end[0] != ':') goto error;

    line->name.start = ptr;
    line->name.length = (end - ptr);

    if (!http_is_token_string(line->name.start, line->name.length)) goto error;

    ptr = end + 1;
    size = size - line->name.length - 1;

    // remove optional whitespace
    while (http_is_whitespace(*ptr)) {

        if ((size_t)(ptr - buffer) >= size) break;

        ptr++;
        size--;
    }

    line->value.start = ptr;

    lineend = (uint8_t *)ov_string_find((char *)ptr, size, "\r\n", 2);
    if (!lineend) {

        if ((max != 0) && max < length) goto error;

        return OV_HTTP_PARSER_PROGRESS;
    }

    while (lineend) {

        if (size == 2) return OV_HTTP_PARSER_PROGRESS;

        ptr = lineend + 2;
        size -= 2;

        // check obs-fold
        if (http_is_whitespace(*ptr)) {

            lineend = (uint8_t *)ov_string_find((char *)ptr, size, "\r\n", 2);
            if (!lineend) return OV_HTTP_PARSER_PROGRESS;

        } else {

            break;
        }
    }

    OV_ASSERT(lineend);
    ptr = lineend - 1;

    while (http_is_whitespace(*ptr)) {

        ptr--;
        if (ptr == line->value.start) goto error;
    }

    line->value.length = ptr - line->value.start + 1;

    if (!http_is_field_content(line->value.start, line->value.length))
        goto error;

    if (next) *next = (uint8_t *)lineend + 2;

    return OV_HTTP_PARSER_SUCCESS;

error:
    if (next) *next = (uint8_t *)buffer;
    return OV_HTTP_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

const ov_http_header *ov_http_header_get_next(const ov_http_header *array,
                                              size_t size,
                                              size_t *index,
                                              const char *name) {

    if (!array || !name || (size == 0) || !index) goto error;

    const ov_http_header *header = NULL;

    size_t len = strlen(name);

    for (size_t i = *index; i < size; i++) {

        header = &array[i];
        if (!header) continue;

        if (!header->name.start) continue;

        if (len != header->name.length) continue;

        if (0 == strncasecmp(
                     name, (char *)header->name.start, header->name.length)) {
            *index = i + 1;
            return header;
        }
    }

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

const ov_http_header *ov_http_header_get_unique(const ov_http_header *array,
                                                size_t size,
                                                const char *name) {

    const ov_http_header *header = NULL;

    if (!array || !name || (size == 0)) goto error;

    size_t index = 0;
    header = ov_http_header_get_next(array, size, &index, name);
    if (!header) goto error;

    if (ov_http_header_get_next(array, size, &index, name)) goto error;

    return header;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

const ov_http_header *ov_http_header_get(const ov_http_header *array,
                                         size_t size,
                                         const char *name) {

    size_t index = 0;
    return ov_http_header_get_next(array, size, &index, name);
}

/*----------------------------------------------------------------------------*/

void ov_http_message_enable_caching(size_t capacity) {

    ov_registered_cache_config cfg = {

        .capacity = capacity,
        .item_free = ov_http_message_free_uncached,

    };

    g_cache = ov_registered_cache_extend("http_message", cfg);
}

/*----------------------------------------------------------------------------*/

static ov_http_message *message_create(ov_registered_cache *cache,
                                       ov_http_message_config config) {

    ov_http_message *msg = ov_registered_cache_get(cache);

    if (msg) {

        /* We need to ensure a cached msg is in line with the current config */

        if (msg->config.header.capacity < config.header.capacity)
            msg = ov_http_message_free(msg);
    }

    if (!msg) {

        msg = calloc(1,
                     sizeof(ov_http_message) +
                         config.header.capacity * sizeof(ov_http_header));

        if (!msg) goto error;

        msg->magic_byte = OV_HTTP_MESSAGE_MAGIC_BYTE;
    }

    OV_ASSERT(ov_http_message_cast(msg));

    /* Perform a (re)init to zero in any case */

    for (size_t i = 0; i < config.header.capacity; i++) {
        msg->header[i] = (ov_http_header){0};
    }

    if (msg->buffer) {

        OV_ASSERT(ov_buffer_cast(msg->buffer));

        if (msg->buffer->capacity < config.buffer.default_size) {
            if (!ov_buffer_extend(
                    msg->buffer,
                    config.buffer.default_size - msg->buffer->capacity))
                msg->buffer = ov_buffer_free(msg->buffer);
        }

        ov_buffer_clear(msg->buffer);

    } else {

        msg->buffer = ov_buffer_create(config.buffer.default_size);
        if (!msg->buffer) goto error;
    }

    msg->version = (ov_http_version){0};
    msg->request = (ov_http_request){0};
    msg->status = (ov_http_status){0};
    msg->body = (ov_memory_pointer){0};

    msg->config = config;

    return msg;
error:
    msg = ov_http_message_free_uncached(msg);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_http_message_config ov_http_message_config_init(
    ov_http_message_config config) {

    /* Set default config */
    if (0 == config.header.capacity)
        config.header.capacity = IMPL_DEFAULT_HEADER_CAPACITY;

    if (0 == config.buffer.default_size)
        config.buffer.default_size = IMPL_DEFAULT_BUFFER_SIZE;

    if (0 == config.buffer.max_bytes_recache)
        config.buffer.max_bytes_recache = IMPL_DEFAULT_MAX_BUFFER_RECACHE;

    if (0 == config.transfer.max)
        config.transfer.max = IMPL_DEFAULT_MAX_TRANSFER;

    if (0 == config.header.max_bytes_method_name)
        config.header.max_bytes_method_name = IMPL_DEFAULT_MAX_METHOD_NAME;

    if (0 == config.header.max_bytes_line)
        config.header.max_bytes_line = IMPL_DEFAULT_MAX_HEADER_LINE;

    return config;
}

/*----------------------------------------------------------------------------*/

ov_http_message *ov_http_message_create(ov_http_message_config config) {

    config = ov_http_message_config_init(config);

    ov_http_message *msg = message_create(g_cache, config);

    if (!msg) {
        ov_log_error("Failed to create a HTTP message structure");
        return NULL;
    }

    OV_ASSERT(ov_http_message_cast(msg));
    OV_ASSERT(msg);
    OV_ASSERT(msg->buffer);

    return msg;
}

/*----------------------------------------------------------------------------*/

bool ov_http_message_clear(ov_http_message *msg) {

    if (!ov_http_message_cast(msg)) return false;

    for (size_t i = 0; i < msg->config.header.capacity; i++) {
        msg->header[i] = (ov_http_header){0};
    }

    msg->version = (ov_http_version){0};
    msg->request = (ov_http_request){0};
    msg->status = (ov_http_status){0};
    msg->body = (ov_memory_pointer){0};

    if (msg->buffer) {

        if (msg->buffer->capacity > msg->config.buffer.max_bytes_recache) {
            msg->buffer = ov_buffer_free_uncached(msg->buffer);
        } else {
            ov_buffer_clear(msg->buffer);
        }
    }

    return true;
}

/*----------------------------------------------------------------------------*/

void *ov_http_message_free_uncached(void *message) {

    ov_http_message *msg = ov_http_message_cast(message);
    if (!msg) return message;

    msg->buffer = ov_buffer_free_uncached(msg->buffer);
    msg = ov_data_pointer_free(msg);

    return msg;
}

/*----------------------------------------------------------------------------*/

void *ov_http_message_free(void *message) {

    ov_http_message *msg = ov_http_message_cast(message);
    if (!msg) return message;

    ov_http_message_clear(msg);
    msg = ov_registered_cache_put(g_cache, msg);

    return ov_http_message_free_uncached(msg);
}

/*----------------------------------------------------------------------------*/

ov_http_message *ov_http_message_cast(void *data) {

    if (!data) return NULL;

    if (*(uint16_t *)data == OV_HTTP_MESSAGE_MAGIC_BYTE)
        return (ov_http_message *)data;

    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool validate_tranfer_encoding_grammar(const uint8_t *start,
                                              size_t length,
                                              ov_memory_pointer *transfer) {

    /*
     *  This function will parse the transfer encoding and check if the
     *  transfer encoding is valid for the grammar.
     *
     *  If transfer is handover, the transfer encoding will be set.
     *  If the transfer encoding is a transfer extension, the grammar will be
     *  checked, but any transfer-parameter MUST be parsed using some other
     *  function. (Grammar check only)
     */

    if (!start || (0 == length)) goto error;

    uint8_t *delimiter = memchr(start, ';', length);
    if (!delimiter) {

        if (!http_is_token_string(start, length)) goto error;

        if (transfer) {
            transfer->start = start;
            transfer->length = length;
        }

        return true;
    }

    if (delimiter == start) goto error;

    if (length == (size_t)(delimiter - start)) goto error;

    uint8_t *ptr = delimiter - 1;
    while (http_is_whitespace(*ptr)) {

        if (ptr == start) goto error;

        ptr--;
    }

    if (!http_is_token_string(start, (ptr - start))) goto error;

    if (transfer) {
        transfer->start = start;
        transfer->length = ptr - start;
    }

    uint8_t *first = delimiter + 1;
    uint8_t *last = NULL;

    while (delimiter) {

        // offset intro whitespace

        ptr = first;

        while (http_is_whitespace(*ptr)) {

            if (length == (size_t)(ptr - start)) goto error;

            ptr++;
        }

        // find key value delimiter

        delimiter = memchr(ptr, '=', length - (ptr - start));
        if (!delimiter) goto error;

        // find key value delimiter

        if (delimiter == first) goto error;

        // offset key outro BWS

        last = delimiter - 1;
        while (http_is_whitespace(*last)) {

            last--;

            if (last == first) goto error;
        }

        // key MUST be token
        if (!http_is_token_string(ptr, (last - ptr))) goto error;

        ptr = delimiter + 1;

        // offset value intro BWS

        while (http_is_whitespace(*ptr)) {

            if (length == (size_t)(ptr - start)) goto error;

            ptr++;
        }

        size_t len = length - (size_t)(ptr - start);
        delimiter = memchr(ptr, ';', len);

        if (delimiter) {

            len = delimiter - ptr;

            if (length == (size_t)(delimiter - start)) goto error;

            first = delimiter + 1;

            // offest intro white space for key before checking val

            last = delimiter - 1;
            while (http_is_whitespace(*last)) {

                if (last == ptr) goto error;

                last--;
            }

            len = (last - ptr);
        }

        // val MUST be token or quoted string
        if (!http_is_token_string(ptr, len))
            if (!http_is_quoted_string(ptr, len)) goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_http_parser_state ov_http_pointer_parse_transfer_encodings(
    const ov_http_message *msg, ov_memory_pointer *array, size_t array_size) {

    if (!msg || !array) goto error;

    size_t index_header = 0;
    size_t index_array = 0;

    // clear the array
    for (size_t i = 0; i < array_size; i++) {
        array[i] = (ov_memory_pointer){0};
    }

    const ov_http_header *header =
        ov_http_header_get_next(msg->header,
                                msg->config.header.capacity,
                                &index_header,
                                OV_HTTP_KEY_TRANSFER_ENCODING);

    if (!header) return OV_HTTP_PARSER_ABSENT;

    const uint8_t *ptr = NULL;
    uint8_t *end = NULL;
    size_t size = 0;

    while (header) {

        if (index_array == array_size) return OV_HTTP_PARSER_OOB;

        ptr = header->value.start;
        size = header->value.length;

        if (size == 0) goto error;

        while (size > 0) {

            if (http_is_whitespace(*ptr)) {
                ptr++;
                size--;
            }

            if (size == 0) goto error;

            end = memchr(ptr, ',', size);

            if (!end) {

                end = (uint8_t *)ptr + size;
                while (http_is_whitespace(*end))
                    end--;

                if (end == ptr) goto error;

                array[index_array].start = ptr;
                array[index_array].length = (end - ptr);

                if (0 == array[index_array].length) goto error;

                if (!validate_tranfer_encoding_grammar(
                        array[index_array].start,
                        array[index_array].length,
                        NULL))
                    goto error;

                index_array++;
                header = ov_http_header_get_next(msg->header,
                                                 msg->config.header.capacity,
                                                 &index_header,
                                                 OV_HTTP_KEY_TRANSFER_ENCODING);

                break;
            }

            // comma only ?
            if (end == ptr) goto error;

            // last valid char is , ?
            if ((size_t)(end - ptr) + 1 == size) goto error;

            array[index_array].start = ptr;
            array[index_array].length = 0;
            ptr = end - 1;

            while (http_is_whitespace(*ptr))
                ptr--;

            if (ptr == array[index_array].start) goto error;

            array[index_array].length = ptr - array[index_array].start;

            // only whitespace after , ?
            if (0 == array[index_array].length) goto error;

            if (!validate_tranfer_encoding_grammar(
                    array[index_array].start, array[index_array].length, NULL))
                goto error;

            ptr = end + 1;
            size = header->value.length - (ptr - header->value.start);

            index_array++;

            if (index_array >= array_size) return OV_HTTP_PARSER_OOB;
        }
    }

    return OV_HTTP_PARSER_SUCCESS;

error:
    return OV_HTTP_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

static bool check_chunk_extension_grammar(const uint8_t *start, size_t size) {

    /*
     *  Here we do have some chunk extension in use and check if they
     *  are valid in terms of grammar.
     *
     *
     *
     *  chunk-ext      = *( ";" chunk-ext-name [ "=" chunk-ext-val ] )
     *  chunk-ext-name = token
     *  chunk-ext-val  = token / quoted-string
     */

    uint8_t *ptr = NULL;
    uint8_t *del = NULL;
    uint8_t *ext = (uint8_t *)start;
    size_t len = 0;

    if (size < 1) goto error;

    if (!start) goto error;

    while (ext) {

        if (*ext != ';') goto error;

        if (size <= (size_t)(ext - start)) goto error;

        ptr = ext + 1;
        del = memchr(ptr, '=', size - (ext - start));
        if (!del) goto error;

        if (!http_is_token_string(ptr, (del - ptr))) goto error;

        if (size <= (size_t)(del - start)) goto error;

        ptr = del + 1;
        ext = memchr(ptr, ';', size - (ptr - start));

        if (ext) {
            len = (ext - ptr);
        } else {
            len = size - (ptr - start);
        }

        if (!http_is_token_string(ptr, len))
            if (!http_is_quoted_string(ptr, len)) goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static ov_http_parser_state parse_chunked(ov_http_message *msg,
                                          uint8_t **next) {

    if (!msg || !next || !msg->body.start) goto error;

    uint8_t *start = (uint8_t *)msg->body.start;
    if ((size_t)(msg->body.start - msg->buffer->start) > msg->buffer->length)
        goto error;

    size_t length =
        msg->buffer->length - (msg->body.start - msg->buffer->start);

    if (0 == length) return OV_HTTP_PARSER_PROGRESS;

    /*
        chunked-body   = *chunk
                          last-chunk
                          trailer-part
                          CRLF

         chunk          = chunk-size [ chunk-ext ] CRLF
                          chunk-data CRLF
         chunk-size     = 1*HEXDIG
         last-chunk     = 1*("0") [ chunk-ext ] CRLF

         chunk-data     = 1*OCTET ; a sequence of chunk-size octets

    */

    uint8_t *ptr = start;
    size_t size = length;
    uint8_t *lineend = memchr(ptr, '\r', size);

    if (!lineend) return OV_HTTP_PARSER_PROGRESS;

    uint8_t *ext = memchr(ptr, ';', lineend - ptr);
    uint8_t *end = NULL;

    size_t s = lineend - ptr;
    if (ext) s = ext - ptr;

    int64_t chunk_size =
        ov_string_parse_hex_digits((char *)ptr, s, (char **)&end);

    if (ext) {

        if (end != ext) goto error;

        if (!check_chunk_extension_grammar(ext, (lineend - ext))) goto error;

    } else {

        if (end != lineend) goto error;
    }

    // check for full lineend \r\n
    if (length - 2 < (size_t)(lineend - start)) return OV_HTTP_PARSER_PROGRESS;

    if (lineend[1] != '\n') goto error;

    // now we expect chunkdata up to the next lineend
    ptr = lineend + 2;
    size = length - (ptr - start);

    if (chunk_size == 0) goto done;

    if (size < (size_t)chunk_size + 2) return OV_HTTP_PARSER_PROGRESS;

    if (ptr[chunk_size] != '\r') goto error;

    if (ptr[chunk_size + 1] != '\n') goto error;

done:
    msg->chunk.start = ptr;
    msg->chunk.length = (size_t)chunk_size;

    ptr += chunk_size + 2;
    msg->body.length = ptr - msg->body.start;
    *next = ptr;

    return OV_HTTP_PARSER_SUCCESS;
error:
    return OV_HTTP_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

static ov_http_parser_state parse_with_transfer(ov_http_message *msg,
                                                uint8_t **next) {

    OV_ASSERT(msg);
    OV_ASSERT(next);

    ov_memory_pointer array[msg->config.transfer.max];
    memset(array, 0, msg->config.transfer.max * sizeof(ov_memory_pointer));

    ov_http_parser_state state = ov_http_pointer_parse_transfer_encodings(
        msg, array, msg->config.transfer.max);

    switch (state) {

        case OV_HTTP_PARSER_SUCCESS:
            break;

        case OV_HTTP_PARSER_OOB:

            ov_log_error(
                "HTTP header with more than %i transfer "
                "encodings is not supported",
                msg->config.transfer.max);

        default:
            goto error;
    }

    /* if we have some transfer encoding we expect chunked as last applied
     * and do NOT support any other option yet */

    size_t i = 0;
    for (i = msg->config.transfer.max - 1; i > 0; i--) {

        if (0 == array[i].start) continue;

        if (0 !=
            strncasecmp("chunked", (char *)array[i].start, array[i].length))
            goto error;

        break;
    }

    if (!array[0].start) goto error;

    /*
     *  We do have at least one transfer encoding parsed,
     *  and the last applied encoding is chunked.
     */

    return parse_chunked(msg, next);
error:
    return OV_HTTP_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

static ov_http_parser_state parse_body(ov_http_message *msg, uint8_t **next) {

    if (!msg || !next) goto error;

    msg->body.start = *next;

    const ov_http_header *content_length = NULL;
    const ov_http_header *transfer_encoding = NULL;

    size_t index_content = 0;
    size_t index_transfer = 0;

    content_length = ov_http_header_get_next(msg->header,
                                             msg->config.header.capacity,
                                             &index_content,
                                             OV_HTTP_KEY_CONTENT_LENGTH);
    transfer_encoding = ov_http_header_get_next(msg->header,
                                                msg->config.header.capacity,
                                                &index_transfer,
                                                OV_HTTP_KEY_TRANSFER_ENCODING);

    if (transfer_encoding) {

        // content_length and transfer_encoding in parallel is not allowed
        if (content_length) goto error;

        return parse_with_transfer(msg, next);

    } else if (content_length) {

        // We do not support multiple content_length header
        if (ov_http_header_get_next(msg->header,
                                    msg->config.header.capacity,
                                    &index_content,
                                    OV_HTTP_KEY_CONTENT_LENGTH))
            goto error;

        uint8_t *end = NULL;
        msg->body.length =
            strtol((char *)content_length->value.start, (char **)&end, 10);

        if (!end) goto error;

        if (end[0] != '\r') goto error;

        // check if the buffer contains enough unparsed bytes
        if (msg->buffer->length <
            ((msg->body.start - msg->buffer->start) + msg->body.length))
            return OV_HTTP_PARSER_PROGRESS;

        if (next) *next = (uint8_t *)msg->body.start + msg->body.length;

    } else {

        // No length or transfer header present
        msg->body.start = NULL;
        msg->body.length = 0;
    }

    return OV_HTTP_PARSER_SUCCESS;
error:
    return OV_HTTP_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

ov_http_parser_state ov_http_pointer_parse_message(ov_http_message *msg,
                                                   uint8_t **next) {

    if (!msg) goto error;

    if (!msg->buffer) goto error;

    uint8_t *nxt = NULL;

    if (!msg->buffer->start) goto error;

    if (msg->buffer->length == 0) goto error;

    // try to parse the startline and switch between state and request
    ov_http_parser_state state =
        ov_http_pointer_parse_status_line(msg->buffer->start,
                                          msg->buffer->length,
                                          &msg->status,
                                          &msg->version,
                                          &nxt);

    switch (state) {

        case OV_HTTP_PARSER_PROGRESS:
            break;

        case OV_HTTP_PARSER_SUCCESS:
            break;

        default:

            state = ov_http_pointer_parse_request_line(
                msg->buffer->start,
                msg->buffer->length,
                &msg->request,
                &msg->version,
                msg->config.header.max_bytes_method_name,
                &nxt);
    }

    switch (state) {

        case OV_HTTP_PARSER_PROGRESS:
            return OV_HTTP_PARSER_PROGRESS;

        case OV_HTTP_PARSER_SUCCESS:
            break;

        default:
            goto error;
    }

    OV_ASSERT(nxt);

    state = ov_http_pointer_parse_header(
        nxt,
        msg->buffer->length - (nxt - msg->buffer->start),
        msg->header,
        msg->config.header.capacity,
        msg->config.header.max_bytes_line,
        &nxt);

    switch (state) {

        case OV_HTTP_PARSER_PROGRESS:
            return OV_HTTP_PARSER_PROGRESS;

        case OV_HTTP_PARSER_SUCCESS:
            break;

        case OV_HTTP_PARSER_OOB:

            ov_log_error(
                "HTTP header OOB with %zu max config header fields,"
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

        case OV_HTTP_PARSER_PROGRESS:
            break;

        case OV_HTTP_PARSER_SUCCESS:
            break;

        default:
            goto error;
    }

    if (next) *next = nxt;

    return state;

error:
    if (next && msg && msg->buffer) *next = msg->buffer->start;

    return OV_HTTP_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

bool ov_http_message_shift_trailing_bytes(ov_http_message *source,
                                          uint8_t *next,
                                          ov_http_message **dest) {

    ov_http_message *new = NULL;

    if (!source || !dest || !next) goto error;

    if (next == source->buffer->start + source->buffer->length) goto done;

    new = ov_http_message_create(source->config);
    if (!new) goto error;

    size_t len = source->buffer->length - (next - source->buffer->start);

    if (!ov_buffer_set(new->buffer, next, len)) goto error;

    if (!memset(next, 0, len)) goto error;

    source->buffer->length = (next - source->buffer->start);

done:
    *dest = new;
    return true;
error:
    new = ov_http_message_free(new);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_http_message_config ov_http_message_config_from_json(
    const ov_json_value *value) {

    ov_http_message_config out = {0};
    if (!value) goto error;

    ov_json_value *val = NULL;
    const ov_json_value *config =
        ov_json_object_get(value, OV_KEY_HTTP_MESSAGE);
    if (!config) config = value;

    val = ov_json_object_get(config, OV_KEY_BUFFER);
    out.buffer.default_size =
        ov_json_number_get(ov_json_object_get(val, OV_KEY_SIZE));
    out.buffer.max_bytes_recache =
        ov_json_number_get(ov_json_object_get(val, OV_KEY_MAX_CACHE));

    val = ov_json_object_get(config, OV_KEY_HEADER);

    out.header.capacity =
        ov_json_number_get(ov_json_object_get(val, OV_KEY_CAPACITY));
    out.header.max_bytes_method_name =
        ov_json_number_get(ov_json_object_get(val, OV_KEY_METHOD));
    out.header.max_bytes_line =
        ov_json_number_get(ov_json_object_get(val, OV_KEY_LINES));

    val = ov_json_object_get(config, OV_KEY_TRANSFER);
    out.transfer.max = ov_json_number_get(ov_json_object_get(val, OV_KEY_MAX));

    val = ov_json_object_get(config, OV_KEY_CHUNK);
    out.chunk.max_bytes =
        ov_json_number_get(ov_json_object_get(val, OV_KEY_MAX));

    return out;
error:
    return (ov_http_message_config){0};
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_http_message_config_to_json(ov_http_message_config config) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *item = NULL;

    out = ov_json_object();
    if (!out) goto error;

    val = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_TRANSFER, val)) goto error;

    item = val;
    val = NULL;

    val = ov_json_number(config.transfer.max);
    if (!ov_json_object_set(item, OV_KEY_MAX, val)) goto error;

    val = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_CHUNK, val)) goto error;

    item = val;
    val = NULL;

    val = ov_json_number(config.chunk.max_bytes);
    if (!ov_json_object_set(item, OV_KEY_MAX, val)) goto error;

    val = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_BUFFER, val)) goto error;

    item = val;
    val = NULL;

    val = ov_json_number(config.buffer.default_size);
    if (!ov_json_object_set(item, OV_KEY_SIZE, val)) goto error;

    val = ov_json_number(config.buffer.max_bytes_recache);
    if (!ov_json_object_set(item, OV_KEY_MAX_CACHE, val)) goto error;

    val = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_HEADER, val)) goto error;

    item = val;
    val = NULL;

    val = ov_json_number(config.header.capacity);
    if (!ov_json_object_set(item, OV_KEY_CAPACITY, val)) goto error;

    val = ov_json_number(config.header.max_bytes_method_name);
    if (!ov_json_object_set(item, OV_KEY_METHOD, val)) goto error;

    val = ov_json_number(config.header.max_bytes_line);
    if (!ov_json_object_set(item, OV_KEY_LINES, val)) goto error;

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_http_message_ensure_open_capacity(ov_http_message *msg,
                                          size_t capacity) {

    if (!msg || (0 == capacity)) goto error;

    if (!msg->buffer || !msg->buffer->start) goto error;

    size_t length = msg->buffer->length;
    size_t open = msg->buffer->capacity - length;

    if (open >= capacity) goto done;

    /* We need to reallocate */

    if (!ov_buffer_extend(msg->buffer, capacity - open)) goto error;

done:
    OV_ASSERT(length == msg->buffer->length);
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_http_message *ov_http_create_status(ov_http_message_config config,
                                       ov_http_version version,
                                       ov_http_status status) {

    ov_http_message *msg = ov_http_message_create(config);
    if (!msg) goto error;

    if (NULL == status.phrase.start) goto error;

    if (0 == status.phrase.length) goto error;

    if ((status.code < 100) || (status.code > 999)) goto error;

    memset(msg->buffer->start, 0, msg->buffer->capacity);

    ssize_t bytes = snprintf((char *)msg->buffer->start,
                             msg->buffer->capacity,
                             "HTTP/%i.%i %i %.*s\r\n",
                             version.major,
                             version.minor,
                             status.code,
                             (int)status.phrase.length,
                             status.phrase.start);

    if (bytes < 0) goto error;

    if (bytes == (ssize_t)msg->buffer->capacity) goto error;

    msg->buffer->length = bytes;
    return msg;
error:
    ov_http_message_free(msg);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_http_message *ov_http_create_status_string(ov_http_message_config config,
                                              ov_http_version version,
                                              uint16_t code,
                                              const char *phrase) {

    ov_http_message *msg = ov_http_message_create(config);
    if (!msg) goto error;

    if (NULL == phrase) goto error;

    if ((code < 100) || (code > 999)) goto error;

    memset(msg->buffer->start, 0, msg->buffer->capacity);

    ssize_t bytes = snprintf((char *)msg->buffer->start,
                             msg->buffer->capacity,
                             "HTTP/%i.%i %i %s\r\n",
                             version.major,
                             version.minor,
                             code,
                             phrase);

    if (bytes < 0) goto error;

    if (bytes == (ssize_t)msg->buffer->capacity) goto error;

    msg->buffer->length = bytes;
    return msg;
error:
    ov_http_message_free(msg);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_http_message *ov_http_create_request(ov_http_message_config config,
                                        ov_http_version version,
                                        ov_http_request request) {

    ov_http_message *msg = ov_http_message_create(config);
    if (!msg) goto error;

    if (NULL == request.method.start) goto error;

    if (NULL == request.uri.start) goto error;

    if (0 == request.method.length) goto error;

    if (0 == request.uri.length) goto error;

    if (request.method.length > msg->config.header.max_bytes_method_name)
        goto error;

    if (request.uri.length > PATH_MAX) goto error;

    memset(msg->buffer->start, 0, msg->buffer->capacity);

    ssize_t bytes = snprintf((char *)msg->buffer->start,
                             msg->buffer->capacity,
                             "%.*s %.*s HTTP/%i.%i\r\n",
                             (int)request.method.length,
                             request.method.start,
                             (int)request.uri.length,
                             request.uri.start,
                             version.major,
                             version.minor);

    if (bytes < 0) goto error;

    if (bytes == (ssize_t)msg->buffer->capacity) goto error;

    msg->buffer->length = bytes;
    return msg;
error:
    ov_http_message_free(msg);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_http_message *ov_http_create_request_string(ov_http_message_config config,
                                               ov_http_version version,
                                               const char *method,
                                               const char *uri) {

    ov_http_message *msg = ov_http_message_create(config);
    if (!msg) goto error;

    if (!method || !uri) goto error;

    if (strlen(method) > msg->config.header.max_bytes_method_name) goto error;

    if (strlen(uri) > PATH_MAX) goto error;

    memset(msg->buffer->start, 0, msg->buffer->capacity);

    ssize_t bytes = snprintf((char *)msg->buffer->start,
                             msg->buffer->capacity,
                             "%s %s HTTP/%i.%i\r\n",
                             method,
                             uri,
                             version.major,
                             version.minor);

    if (bytes < 0) goto error;

    if (bytes == (ssize_t)msg->buffer->capacity) goto error;

    msg->buffer->length = bytes;
    return msg;
error:
    ov_http_message_free(msg);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_http_message_add_header(ov_http_message *msg, ov_http_header header) {

    if (!msg) goto error;

    uint8_t *ptr = NULL;

    if ((NULL == header.name.start) || (0 == header.name.length) ||
        (NULL == header.value.start) || (0 == header.value.length))
        goto error;

    /* Message MUST have some valid startline */

    if (OV_HTTP_PARSER_SUCCESS !=
        ov_http_pointer_parse_status_line(msg->buffer->start,
                                          msg->buffer->length,
                                          &msg->status,
                                          &msg->version,
                                          &ptr))
        if (OV_HTTP_PARSER_SUCCESS !=
            ov_http_pointer_parse_request_line(
                msg->buffer->start,
                msg->buffer->length,
                &msg->request,
                &msg->version,
                msg->config.header.max_bytes_method_name,
                &ptr))
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

        bytes = snprintf((char *)ptr,
                         open,
                         "%.*s:%.*s\r\n",
                         (int)header.name.length,
                         header.name.start,
                         (int)header.value.length,
                         header.value.start);

        if (bytes < 0) goto error;

        if (bytes > (ssize_t)msg->config.header.max_bytes_line) {
            memset(ptr, 0, open);
            goto error;
        }

        if (bytes < open) break;

        open += msg->config.buffer.default_size;

        if (!ov_http_message_ensure_open_capacity(msg, open)) goto error;

        ptr = msg->buffer->start + msg->buffer->length;
        open = msg->buffer->capacity - msg->buffer->length;
    }

    msg->buffer->length += bytes;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_http_message_add_header_string(ov_http_message *msg,
                                       const char *key,
                                       const char *val) {

    if (!msg || !key || !val) goto error;

    /* Message MUST have some valid startline */

    uint8_t *ptr = NULL;

    if (OV_HTTP_PARSER_SUCCESS !=
        ov_http_pointer_parse_status_line(msg->buffer->start,
                                          msg->buffer->length,
                                          &msg->status,
                                          &msg->version,
                                          &ptr))
        if (OV_HTTP_PARSER_SUCCESS !=
            ov_http_pointer_parse_request_line(
                msg->buffer->start,
                msg->buffer->length,
                &msg->request,
                &msg->version,
                msg->config.header.max_bytes_method_name,
                &ptr))
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

        if (bytes < 0) goto error;

        if (bytes > (ssize_t)msg->config.header.max_bytes_line) {
            memset(ptr, 0, open);
            goto error;
        }

        if (bytes < open) break;

        open += msg->config.buffer.default_size;

        if (!ov_http_message_ensure_open_capacity(msg, open)) goto error;

        ptr = msg->buffer->start + msg->buffer->length;
        open = msg->buffer->capacity - msg->buffer->length;
    }

    msg->buffer->length += bytes;
    return true;
error:
    return false;
}
/*----------------------------------------------------------------------------*/

bool ov_http_message_close_header(ov_http_message *msg) {

    if (!msg) goto error;

    uint8_t *ptr = msg->buffer->start + msg->buffer->length;
    ssize_t open = msg->buffer->capacity - msg->buffer->length;

    if (open < 3) {

        open += msg->config.buffer.default_size;
        if (!ov_http_message_ensure_open_capacity(msg, open)) goto error;
    }

    ptr = msg->buffer->start + msg->buffer->length;

    ssize_t bytes = snprintf((char *)ptr, open, "\r\n");

    if (bytes != 2) goto error;

    msg->buffer->length += bytes;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_http_message_add_body(ov_http_message *msg, ov_memory_pointer body) {

    if (!msg) goto error;

    if ((NULL == body.start) || (0 == body.length)) goto error;

    uint8_t *ptr = msg->buffer->start + msg->buffer->length;

    if ((ptr - msg->buffer->start) < 5) goto error;

    if (ptr[-1] != '\n') goto error;

    if (ptr[-2] != '\r') goto error;

    if (ptr[-3] != '\n') goto error;

    if (ptr[-4] != '\r') goto error;

    if (!ov_http_message_ensure_open_capacity(msg, body.length)) goto error;

    ptr = msg->buffer->start + msg->buffer->length;

    if (!memcpy(ptr, body.start, body.length)) goto error;

    msg->buffer->length += body.length;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_http_message_add_body_string(ov_http_message *msg, const char *body) {

    if (!msg || !body) goto error;

    if (0 == strlen(body)) goto error;

    uint8_t *ptr = msg->buffer->start + msg->buffer->length;
    ssize_t open = msg->buffer->capacity - msg->buffer->length;

    if ((ptr - msg->buffer->start) < 5) goto error;

    if (ptr[-1] != '\n') goto error;

    if (ptr[-2] != '\r') goto error;

    if (ptr[-3] != '\n') goto error;

    if (ptr[-4] != '\r') goto error;

    ssize_t bytes = -1;

    while (true) {

        bytes = snprintf((char *)ptr, open, "%s", body);

        if (bytes < 0) goto error;

        if (bytes < open) break;

        open += msg->config.buffer.default_size;

        if (!ov_http_message_ensure_open_capacity(msg, open)) goto error;

        ptr = msg->buffer->start + msg->buffer->length;
        open = msg->buffer->capacity - msg->buffer->length;
    }

    msg->buffer->length += bytes;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static uint8_t *offset_http_whitespace_front(uint8_t *start, size_t size) {

    if (!start) goto error;

    if (size == 0) goto error;

    uint8_t *ptr = start;

    for (size_t i = 0; i < size; i++) {

        if (!http_is_whitespace(*ptr)) break;

        ptr++;
    }

    return ptr;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static uint8_t *offset_http_whitespace_end(uint8_t *start, size_t size) {

    if (!start) goto error;

    if (size == 0) goto error;

    uint8_t *ptr = start + size - 1;
    if (*ptr == 0) {
        ptr--;
        size--;
    }

    for (size_t i = size; i > 0; i--) {

        if (!http_is_whitespace(*ptr)) break;

        if (ptr == start) break;

        ptr--;
    }

    return ptr;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_http_pointer_parse_comma_list_item(const uint8_t *start,
                                           size_t length,
                                           uint8_t **item,
                                           size_t *item_len,
                                           uint8_t **next) {

    if (!start || (length == 0) || !item || !item_len || !next) goto error;

    uint8_t *ptr = NULL;
    uint8_t *comma = NULL;

    /* offset intro whitespace */
    ptr = offset_http_whitespace_front((uint8_t *)start, length);

    /* Whitespace only without comma ? */
    if ((ssize_t)length == (ptr - start)) goto error;

    comma = memchr(ptr, ',', length - (ptr - start));

    if (!comma) {

        *item = ptr;
        ptr = offset_http_whitespace_end(ptr, (length - (ptr - start)));

        if (ptr == NULL) {
            *item_len = 0;
        } else {
            *item_len = (ptr - *item) + 1;
        }

        *next = NULL;
        goto done;
    }

    /* Comma found */

    if (comma == start) goto error;

    /* Whitespace only ? */

    if (comma == ptr) goto error;

    *next = comma + 1;
    *item = ptr;

    ptr = offset_http_whitespace_end(ptr, (comma - ptr));
    *item_len = (ptr - *item) + 1;

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

const uint8_t *ov_http_pointer_find_item_in_comma_list(const uint8_t *start,
                                                       size_t length,
                                                       const uint8_t *string,
                                                       size_t string_length,
                                                       bool case_ignore) {

    if (!start || (length == 0) || !string || (string_length == 0)) goto error;

    uint8_t *next = (uint8_t *)start;
    uint8_t *item = NULL;
    size_t len = 0;

    while (next) {

        if (!ov_http_pointer_parse_comma_list_item(
                next, length - (next - start), &item, &len, &next))
            goto error;

        OV_ASSERT(item);

        if (string_length != len) continue;

        if (case_ignore) {

            if (0 == strncasecmp((char *)item, (char *)string, len))
                return item;

        } else {

            /* Here we do support byte instead of char! */
            if (0 == memcmp(item, string, len)) return item;
        }
    }

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_http_is_request(const ov_http_message *msg, const char *method) {

    if (!msg || !method) goto error;

    /* response ? */
    if (msg->status.code != 0) goto error;

    if (NULL == msg->request.method.start) goto error;

    if (0 != strncasecmp(method,
                         (char *)msg->request.method.start,
                         msg->request.method.length))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_http_message *ov_http_status_not_found(const ov_http_message *src) {

    size_t size = 1000;
    char buffer[size];
    memset(buffer, 0, size);

    ov_http_message *out = NULL;
    if (!src) goto error;

    out = ov_http_create_status_string(
        src->config, src->version, 404, OV_HTTP_NOT_FOUND);

    if (!ov_http_message_set_date(out)) goto error;

    if (!snprintf(buffer,
                  size,
                  "<!doctype html>"
                  "<html lang=\"en\">"
                  "<head>"
                  "<meta charset=\"utf-8\">"
                  "<title>File not found</title>"
                  "</head>"
                  "<body>"
                  "File %.*s not found."
                  "</body>",
                  (int)src->request.uri.length,
                  (char *)src->request.uri.start))
        goto error;

    if (!ov_http_message_set_content_length(out, strlen(buffer))) goto error;

    if (!ov_http_message_add_header_string(
            out, OV_HTTP_KEY_CONTENT_TYPE, "text/html"))
        goto error;

    if (!ov_http_message_add_header_string(
            out, OV_HTTP_KEY_CONTENT_TYPE, "charset=utf-8"))
        goto error;

    if (!ov_http_message_close_header(out)) goto error;

    if (!ov_http_message_add_body_string(out, buffer)) goto error;

    return out;
error:
    ov_http_message_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_http_message *ov_http_status_forbidden(const ov_http_message *src) {

    ov_http_message *out = NULL;
    if (!src) goto error;

    out = ov_http_create_status_string(
        src->config, src->version, 403, OV_HTTP_FORBIDDEN);

    if (!ov_http_message_set_date(out)) goto error;

    if (!ov_http_message_close_header(out)) goto error;

    return out;
error:
    ov_http_message_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_http_message_set_date(ov_http_message *msg) {

    if (!msg) goto error;

    char timestamp[35] = {0};
    if (!ov_imf_write_timestamp(timestamp, 35, NULL)) goto error;

    if (!ov_http_message_add_header_string(msg, OV_HTTP_KEY_DATE, timestamp))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_http_message_set_content_length(ov_http_message *msg, size_t length) {

    char string[25] = {0};
    if (!msg) goto error;

    if (!snprintf(string, 25, "%zu", length)) goto error;

    if (!ov_http_message_add_header_string(
            msg, OV_HTTP_KEY_CONTENT_LENGTH, string))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_http_message_set_content_range(ov_http_message *msg,
                                       size_t length,
                                       size_t from,
                                       size_t to) {

    char string[255] = {0};
    if (!msg) goto error;

    if (!snprintf(string, 255, "bytes %zu-%zu/%zu", from, to, length))
        goto error;

    if (!ov_http_message_add_header_string(
            msg, OV_HTTP_KEY_CONTENT_RANGE, string))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_http_message_add_content_type(ov_http_message *msg,
                                      const char *mime,
                                      const char *charset) {

    if (!msg || !mime) return false;

    const char *charset_base = "charset=";

    size_t size = strlen(mime) + 1;
    if (charset) size += strlen(charset) + strlen(charset_base) + 1;

    char buf[size];
    memset(buf, 0, size);

    if (charset) {

        if (!snprintf(buf, size, "%s;%s%s", mime, charset_base, charset))
            goto error;

        if (!ov_http_message_add_header_string(
                msg, OV_HTTP_KEY_CONTENT_TYPE, buf))
            goto error;

    } else {

        if (!ov_http_message_add_header_string(
                msg, OV_HTTP_KEY_CONTENT_TYPE, mime))
            goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_http_message_add_transfer_encodings(ov_http_message *msg,
                                            const ov_file_format_desc *format) {

    if (!msg) return false;

    size_t size = 1000;
    char buf[size];
    memset(buf, 0, size);

    size_t open = size - strlen(buf);

    if (format) {

        for (size_t i = 0; i < OV_FILE_EXT_MAX; i++) {

            if (0 == format->desc.ext[i][0]) break;

            /* Some extension present, may be supported compression */

            if (0 == strncasecmp(
                         "gzip", format->desc.ext[i], OV_FILE_EXT_STRING_MAX)) {

                if (open < 6) goto error;

                if (!strcat(buf, "gzip;")) goto error;

                open -= 5;

            } else if (0 == strncasecmp("compress",
                                        format->desc.ext[i],
                                        OV_FILE_EXT_STRING_MAX)) {

                if (open < 10) goto error;

                if (!strcat(buf, "compress;")) goto error;

                open -= 9;

            } else if (0 == strncasecmp("deflate",
                                        format->desc.ext[i],
                                        OV_FILE_EXT_STRING_MAX)) {

                if (open < 9) goto error;

                if (!strcat(buf, "deflate;")) goto error;

                open -= 8;

            } else {

                /* some unknown compression encoding,
                 * or some non compression encoding. */
                break;
            }
        }
    }

    open = size - strlen(buf);
    if (open < 7) goto error;

    if (!strcat(buf, "chunked")) goto error;

    if (!ov_http_message_add_header_string(
            msg, OV_HTTP_KEY_TRANSFER_ENCODING, buf))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_http_message *ov_http_message_pop(ov_buffer **input,
                                     const ov_http_message_config *config,
                                     ov_http_parser_state *state) {

    ov_http_message *msg = NULL;
    ov_buffer *new_buffer = NULL;
    ov_http_parser_state s = OV_HTTP_PARSER_ERROR;

    if (!input || !state || !config) goto error;

    ov_buffer *buffer = *input;
    if (!ov_buffer_cast(buffer)) goto error;

    if (0 == buffer->start[0]) {
        s = OV_HTTP_PARSER_PROGRESS;
        new_buffer = buffer;
        goto done;
    }

    msg = calloc(1,
                 sizeof(ov_http_message) +
                     config->header.capacity * sizeof(ov_http_header));

    if (!msg) goto error;

    msg->magic_byte = OV_HTTP_MESSAGE_MAGIC_BYTE;
    msg->config = *config;
    msg->buffer = buffer;

    uint8_t *next = NULL;

    s = ov_http_pointer_parse_message(msg, &next);

    switch (s) {

        case OV_HTTP_PARSER_PROGRESS:
            msg->buffer = NULL;
            new_buffer = buffer;
            goto done;

        case OV_HTTP_PARSER_SUCCESS:
            break;

        default:
            msg->buffer = NULL;
            goto error;
    }

    if (next == msg->buffer->start + msg->buffer->length) goto done;

    size_t len = msg->buffer->length - (next - msg->buffer->start);
    new_buffer = ov_buffer_create(len);

    if (!ov_buffer_set(new_buffer, next, len)) goto error;

    if (!memset(next, 0, len)) goto error;

    msg->buffer->length = (next - msg->buffer->start);

done:
    *state = s;
    *input = new_buffer;

    return msg;
error:
    msg = ov_http_message_free(msg);
    return NULL;
}
