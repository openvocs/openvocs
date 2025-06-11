/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_sdp.c
        @author         Markus Töpfer

        @date           2019-12-08

        @ingroup        ov_sdp

        @brief          Implementation of


        ------------------------------------------------------------------------
*/
#include "../../include/ov_sdp.h"

#include "../../include/ov_utils.h"

#include "../../include/ov_cache.h"
#include "../../include/ov_sdp_grammar.h"

/*----------------------------------------------------------------------------*/

#define IMPL_TYPE 0x0001
#define IMPL_DEFAULT_BUFFER_SIZE 1024

/*----------------------------------------------------------------------------*/

typedef struct DefaultSession {

    ov_sdp_session public;

    /*
     *      internal message buffer holding the string.
     */
    ov_buffer *buffer;

} DefaultSession;

/*----------------------------------------------------------------------------*/

typedef struct {

    bool enabled;

    ov_cache *buffer;
    ov_cache *session;
    ov_cache *description;
    ov_cache *bandwidth;
    ov_cache *list;
    ov_cache *time;
    ov_cache *connection;

} ov_static_global_sdp_cache;

/*----------------------------------------------------------------------------*/

static ov_static_global_sdp_cache ov_sdp_cache = {0};

/*----------------------------------------------------------------------------*/

#define AS_DEFAULT_SESSION(x)                                                  \
    (((ov_sdp_session_cast(x) != 0) &&                                         \
      (IMPL_TYPE == ((ov_sdp_session *)x)->type))                              \
         ? (DefaultSession *)(x)                                               \
         : 0)

/*
 *      ------------------------------------------------------------------------
 *
 *      #PRIVATE
 *
 *      ------------------------------------------------------------------------
 */

/*
        Parse a line frame.

        EXAMPLE
                        x=line\r\nwhatever
                        ^ ^  ^    ^      ^
        start   ________| |  |    |      |
        next    __________|  |    |      |
        last    _____________|    |      |
        outer   __________________|      |
        size    _________________________|

        @param start    ... of the buffer to check
        @param size     ... of the buffer to check
        @param next     ... first byte of line content
        @param inner    ... last  byte of line content
        @param outer    ... next byte after line
        @param in       if true intro must be present "[intro]="
        @param out      if true \r\n will be checked (last != outer)
        @param intro    char of the intro start
*/
static bool parse_line_frame(const char *start,
                             size_t size,
                             char **next,
                             char **inner,
                             char **outer,
                             bool in,
                             bool out,
                             char intro) {

    char *last = NULL;

    if (!start || !inner) return false;

    if (in && out)
        if (size < 4) return false;

    if (in) {

        if (size < 2) return false;

        if (start[0] != intro) return false;

        if (start[1] != '=') return false;

        if (next) *next = (char *)start + 2;

    } else {

        if (next) *next = (char *)start;
    }

    if (out) {

        if (size < 2) return false;

        last = memchr(start, '\n', size);
        if (!last) return false;

        if (last[-1] != '\r') return false;

        *inner = last - 2;
        if (outer) *outer = last + 1;

    } else {

        *inner = (char *)start + size - 1;
        if (outer) *outer = (char *)start + size;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static bool parse_number(const char *start, size_t size, uint64_t *number) {

    if (!start || size < 1 || !number) return false;

    char buffer[size + 1];
    memset(buffer, 0, size + 1);
    strncat(buffer, start, size);

    uint64_t nbr = 0;
    char *end = NULL;

    nbr = strtoll(buffer, &end, 10);

    if ((end - buffer) != (ssize_t)size) goto error;

    *number = nbr;

    return true;
error:
    if (number) *number = 0;
    return false;
}

/*----------------------------------------------------------------------------*/

static bool parse_content_line(const char *start,
                               size_t size,
                               char **content_start,
                               size_t *content_length,
                               char **next,
                               char intro,
                               bool (*match)(const char *, uint64_t)) {

    if (!start || size < 5 || !intro) return false;

    if (!content_start || !content_length) goto error;

    if (*start != intro) goto error;

    char *ptr = NULL;
    char *last = NULL;

    if (!parse_line_frame(start, size, &ptr, &last, next, true, true, intro))
        goto error;

    if (match)
        if (!match(ptr, last + 1 - ptr)) goto error;

    *content_start = ptr;
    *content_length = (last + 1 - ptr);

    return true;
error:
    if (content_start) *content_start = NULL;

    if (content_length) *content_length = 0;

    if (next) *next = (char *)start;

    return false;
}

/*----------------------------------------------------------------------------*/

static bool parse_information(const char *start,
                              size_t size,
                              char **content,
                              size_t *length) {

    if (!start || size < 5 || !content || !length) return false;

    char *next = NULL;

    return parse_content_line(
        start, size, content, length, &next, 'i', ov_sdp_is_text);
}

/*----------------------------------------------------------------------------*/

static bool parse_uri(const char *start,
                      size_t size,
                      char **content,
                      size_t *length) {

    if (!start || size < 5 || !content || !length) return false;

    char *next = NULL;
    char *ptr = (char *)start;
    char *last = NULL;

    if (!parse_line_frame(start, size, &next, &last, &ptr, true, true, 'u'))
        goto error;

    *content = next;
    *length = (last + 1 - next);

    // TBD validate URI content string
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_content_list(const char *start,
                                  size_t size,
                                  char **next,
                                  char intro,
                                  bool (*match)(const char *, uint64_t)) {

    if (!next || !start || !match || size < 1) goto error;

    if (*start != intro) goto error;

    char *ptr = (char *)start;
    char *content = NULL;
    size_t length = 0;

    while (*ptr == intro) {

        content = NULL;
        length = 0;

        if (!parse_content_line(ptr,
                                size - (ptr - start),
                                &content,
                                &length,
                                &ptr,
                                intro,
                                match))
            goto error;

        if (!content || length < 1) goto error;
    }

    if (next) *next = ptr;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_line(const char *start, size_t size, char **next) {

    if (!start || !next) return false;

    if (size < 5) goto error;
    if (start[1] != '=') goto error;

    char *last = memchr(start, '\n', size);
    if (!last) goto error;

    if (last[-1] != '\r') goto error;

    *next = last + 1;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_version(const char *line, size_t length) {

    if (!line || length < 5) goto error;

    if (0 != strncmp(line, "v=0\r\n", length)) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_session_name(const char *line, size_t length) {

    if (!line || length < 5) goto error;

    if (line[0] != 's') goto error;
    if (line[length - 2] != '\r') goto error;
    if (line[length - 1] != '\n') goto error;

    return ov_sdp_is_token(line + 2, length - 4);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_key(const char *line, size_t length) {

    if (!line || length < 5) goto error;

    if (line[0] != 'k') goto error;
    if (line[length - 2] != '\r') goto error;
    if (line[length - 1] != '\n') goto error;

    return ov_sdp_is_key(line + 2, length - 4);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_connection(const char *line,
                                size_t length,
                                bool intro,
                                bool outro) {

    if (!line || length < 5) goto error;

    char *space = NULL;
    char *ptr = (char *)line;

    if (intro) {
        if (ptr[0] != 'c') goto error;
        if (ptr[1] != '=') goto error;
        ptr += 2;
    }

    if (outro) {
        if (line[length - 2] != '\r') goto error;
        if (line[length - 1] != '\n') goto error;
        length -= 2;
    }

    space = memchr(ptr, 0x20, length);
    if (!space) goto error;
    if (!ov_sdp_is_token(ptr, space - ptr)) goto error;

    ptr = space + 1;
    space = memchr(ptr, 0x20, length);
    if (!space) goto error;
    if (!ov_sdp_is_token(ptr, space - ptr)) goto error;

    ptr = space + 1;
    if (!ov_sdp_is_address(ptr, length - (ptr - line))) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_origin(const char *line, size_t length) {

    if (!line || length < 11) goto error;

    if (line[0] != 'o') goto error;

    char *space = NULL;
    char *ptr = (char *)line;

    space = memchr(ptr, 0x20, length);
    if (!space) goto error;

    if (!ov_sdp_is_username(ptr, space - ptr)) goto error;

    ptr = space + 1;
    space = memchr(ptr, 0x20, length);
    if (!space) goto error;

    if (!ov_sdp_is_digit(ptr, space - ptr)) goto error;

    ptr = space + 1;
    space = memchr(ptr, 0x20, length);
    if (!space) goto error;

    if (!ov_sdp_is_digit(ptr, space - ptr)) goto error;

    ptr = space + 1;

    if (!validate_connection(ptr, length - (ptr - line), false, true))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_information(const char *line, size_t length) {

    if (!line || length < 5) goto error;

    char *content = NULL;
    size_t clength = 0;

    if (!parse_information(line, length, &content, &clength)) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_uri(const char *line, size_t length) {

    if (!line || length < 5) goto error;

    char *content = NULL;
    size_t clength = 0;

    if (!parse_uri(line, length, &content, &clength)) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_time(const char *line, size_t length, char **next) {

    if (!line || !next || length < 5) goto error;

    /*
     *  "t=0 0\r\n
     *
     *  time-fields =  1*( %x74 "=" start-time SP stop-time
     *          *(CRLF repeat-fields) CRLF)
     *           [zone-adjustments CRLF]
     */

    char *space = NULL;
    char *out = (char *)line;
    char *ptr = NULL;
    char *last = NULL;

    if (out[0] != 't') goto error;

    while (out[0] == 't') {

        ptr = out;
        if (!validate_line(ptr, length - (ptr - line), &out)) goto error;

        ptr += 2;
        space = memchr(ptr, 0x20, (out - ptr));
        if (!space) goto error;

        if (!ov_sdp_is_time(ptr, (space - ptr))) goto error;

        ptr = space + 1;

        if (!ov_sdp_is_time(ptr, (out - 2 - ptr))) goto error;

        while (out[0] == 'r') {

            /*
             *  "r=repeat-interval SP typed-time 1*(SP
             * typed-time)\r\n"
             *
             *  All array elements in separate lines:
             *
             *  EXAMPLES:
             *      (1) "r=1 1d 1\r\n"
             *      (2) "r=12345678 1d 1h 1m 1s\r\n"
             */

            ptr = out;
            last = out - 2;

            if (!validate_line(ptr, length - (ptr - line), &out)) goto error;

            ptr += 2;

            space = memchr(ptr, 0x20, (last - ptr));
            if (!space) goto error;

            if (!ov_sdp_is_typed_time(ptr, (space - ptr), false)) goto error;

            ptr = space + 1;

            space = memchr(ptr, 0x20, (last - ptr));
            if (!space) goto error;

            while (space) {

                if (!ov_sdp_is_typed_time(ptr, (space - ptr), false))
                    goto error;

                ptr = space + 1;
                space = memchr(ptr, 0x20, (out - ptr));
            }

            if (!ov_sdp_is_typed_time(ptr, (out - ptr) - 2, false)) goto error;
        }

        if (out[0] == 'z') {

            /*
             *  zone-adjustments = %x7a "="
             *      time SP ["-"] typed-time
             * *(SP time SP ["-"] typed-time)
             *
             *  All array elements in one line:
             *
             *  EXAMPLES:
             *      (1) "z=1 -1d\r\n"
             *      (2) "z=1 1d 2 2d 3 3\r\n"
             */
            ptr = out + 2;

            if (!validate_line(out, length - (out - line), &out)) goto error;

            do {
                space = memchr(ptr, 0x20, (out - ptr));
                if (!space) goto error;

                if (!ov_sdp_is_typed_time(ptr, (space - ptr), false))
                    goto error;

                ptr = space + 1;
                space = memchr(ptr, 0x20, (out - ptr));

                if (!space) {

                    if (!ov_sdp_is_typed_time(ptr, (out - ptr) - 2, false))
                        goto error;

                } else {

                    if (!ov_sdp_is_typed_time(ptr, (space - ptr), false))
                        goto error;

                    ptr = space + 1;
                }

            } while (space);
        }
    }

    *next = out;

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_attributes(const char *line, size_t length, char **next) {

    if (!line || !next || length < 5) goto error;

    /*
     *  EXAMPLES:
     *      (1) "a=attribute\r\n"
     *      (2) "a=attribute\r\na=attribute\r\na=attribute\r\n"
     *
     * SPECIFICATION:
     *
     *  attribute-fields =    *(%x61 "=" attribute CRLF)
     *
     *  ; sub-rules of 'a='
     *  attribute =           (att-field ":" att-value) / att-field
     *
     *  att-field =           token
     *
     *  att-value =           byte-string
     */

    char *out = (char *)line;
    char *ptr = NULL;
    char *col = NULL;
    if (out[0] != 'a') goto error;

    while (out[0] == 'a') {

        ptr = out + 2;
        if (!validate_line(out, length - (out - line), &out)) goto error;

        col = memchr(ptr, ':', out - ptr);

        if (col) {

            if (!ov_sdp_is_token(ptr, (col - ptr))) goto error;

            ptr = col + 1;
            if (!ov_sdp_is_byte_string(ptr, (out - ptr) - 2)) goto error;

        } else {

            if (!ov_sdp_is_token(ptr, (out - ptr) - 2)) goto error;
        }
    }

    *next = out;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_media(const char *start, size_t size, char **next) {

    if (!start || size < 11 || !next) goto error;

    if (start[0] != 'm') goto error;

    /*
     *  EXAMPLES:
     *      (1) "m=1 0 1 1\r\n"
     *
     * SPECIFICATION:
     *
     *   media-field =      %x6d "=" media SP port ["/" integer]
     *                      SP proto 1*(SP fmt) CRLF
     */

    char *ptr = (char *)start;
    char *space = NULL;
    char *nxt = NULL;
    char *end = NULL;

    uint32_t port = 0;

    if (!validate_line(ptr, size - (ptr - start), &nxt)) goto error;

    ptr += 2;

    *next = nxt;

    space = memchr(ptr, ' ', (nxt - ptr));
    if (!space) goto error;

    if (!ov_sdp_is_token(ptr, space - ptr)) goto error;

    ptr = space + 1;

    space = memchr(ptr, ' ', (nxt - ptr));
    if (!space) goto error;

    port = strtol(ptr, &end, 10);
    if (!end) goto error;

    if (((end - ptr < 1)) || ((end - ptr) > 5)) goto error;

    if (0 == port) { /* ignore */
    };

    ptr = end;

    if (*ptr == '/') {

        ptr++;

        port = strtol(ptr, &end, 10);
        if (!end) goto error;

        if (((end - ptr < 1)) || ((end - ptr) > 5)) goto error;

        ptr = end;
    }

    if (end != space) goto error;

    ptr = space + 1;
    space = memchr(ptr, ' ', (nxt - ptr));
    if (!space) goto error;

    if (!ov_sdp_is_proto(ptr, (space - ptr))) goto error;

    ptr = space + 1;

    space = memchr(ptr, ' ', (nxt - ptr));

    while (space) {

        if (!ov_sdp_is_token(ptr, (space - ptr))) goto error;

        ptr = space + 1;
        space = memchr(ptr, ' ', (nxt - ptr));
    }

    if (!ov_sdp_is_token(ptr, (nxt - 2 - ptr))) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_media_descriptions(const char *line,
                                        size_t length,
                                        char **next) {

    if (!line || !next || length < 11) goto error;

    char *ptr = NULL;
    char *out = (char *)line;

    if (out[0] != 'm') goto error;

    while (out[0] == 'm') {

        ptr = out;
        if (!validate_line(out, length - (out - line), &out)) goto error;

        if (!validate_media(ptr, (out - ptr), &ptr)) goto error;

        // OPTIONAL INFO

        if (*ptr == 'i') {

            if (!validate_line(ptr, length - (ptr - line), &out)) goto error;

            if (!validate_information(ptr, (out - ptr))) goto error;

            ptr = out;
        }

        // OPTIONAL CONNECTIONS

        while (*ptr == 'c') {

            if (!validate_line(ptr, length - (ptr - line), &out)) goto error;

            if (!validate_connection(ptr, (out - ptr), true, true)) goto error;

            ptr = out;
        }

        // OPTIONAL BANDWIDTH

        if (*ptr == 'b') {

            if (!validate_content_list(
                    ptr, length - (ptr - line), &out, 'b', ov_sdp_is_bandwidth))
                goto error;

            ptr = out;
        }

        // OPTIONAL KEY

        if (*ptr == 'k') {

            if (!validate_line(ptr, length - (ptr - line), &out)) goto error;

            if (!validate_key(ptr, (out - ptr))) goto error;

            ptr = out;
        }

        // OPTIONAL ATTRIBUTES

        if (*ptr == 'a') {

            if (!validate_attributes(ptr, length - (ptr - line), &out))
                goto error;

            ptr = out;
        }
    }

    *next = ptr;

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool masked_session_connection(ov_sdp_connection *connection,
                                      char *line_start,
                                      size_t line_length) {

    /*
     *      connection-field =  'c' "="
     *          nettype SP addrtype SP unicast-address CRLF
     *
     */

    if (!connection || !line_start || line_length < 7) goto error;

    char *ptr = line_start;
    char *space = NULL;

    if (line_start[line_length - 2] != '\r') goto error;
    if (line_start[line_length - 1] != '\n') goto error;

    line_start[line_length - 2] = 0;
    line_start[line_length - 1] = 0;

    space = memchr(ptr, 0x20, line_length);
    if (!space) goto error;
    if (!ov_sdp_is_token(ptr, space - ptr)) goto error;

    connection->nettype = ptr;

    ptr = space + 1;
    *space = 0;

    space = memchr(ptr, 0x20, line_length - (ptr - line_start));
    if (!space) goto error;
    if (!ov_sdp_is_token(ptr, space - ptr)) goto error;

    connection->addrtype = ptr;

    ptr = space + 1;
    *space = 0;

    /*
     *      address end already set above,
     *      the string at ptr MUST be a valid address.
     */
    connection->address = ptr;

    if (!ov_sdp_is_address(connection->address, strlen(connection->address)))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool mask_origin(DefaultSession *session,
                        char *line_start,
                        size_t line_length) {

    /*
     *      origin-field =  %x6f "="
     *                      name SP sess-id SP sess-version SP
     *                      nettype SP addrtype SP unicast-address CRLF
     *
     *
     *      "o=u 1 2 n a x\r\n"
     */

    if (!session || !line_start || line_length < 11) goto error;

    char *ptr = NULL;
    char *space = NULL;
    uint64_t nbr = 0;

    if (line_start[0] != 'o') goto error;
    if (line_start[1] != '=') goto error;

    ptr = line_start + 2;

    space = memchr(ptr, 0x20, line_length - (ptr - line_start));
    if (!space) goto error;

    /*
     *      MASK the name string within the original buffer
     */
    session->public.origin.name = ptr;
    *space = 0;

    ptr = space + 1;
    space = memchr(ptr, 0x20, line_length - (ptr - line_start));
    if (!space) goto error;
    *space = 0;

    /*
     *      Parse the id number
     */

    if (!parse_number(ptr, space - ptr, &nbr)) goto error;

    session->public.origin.id = nbr;

    ptr = space + 1;
    space = memchr(ptr, 0x20, line_length - (ptr - line_start));
    if (!space) goto error;
    *space = 0;

    /*
     *      Parse the version number
     */

    if (!parse_number(ptr, space - ptr, &nbr)) goto error;

    session->public.origin.version = nbr;

    ptr = space + 1;

    if (!masked_session_connection(&session->public.origin.connection,
                                   ptr,
                                   line_length - (ptr - line_start)))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static ov_sdp_list *create_list_node(ov_sdp_list **head) {

    ov_sdp_list *list = NULL;

    if (!head) goto error;

    list = ov_sdp_list_create();

    if (!ov_node_push((void **)head, list)) goto error;

    return list;

error:
    return ov_sdp_list_free(list);
}

/*----------------------------------------------------------------------------*/

static bool create_masked_list(ov_sdp_list **head,
                               char *start,
                               size_t length,
                               char **next,
                               char intro,
                               bool (*match)(const char *, uint64_t)) {

    ov_sdp_list *list = NULL;

    if (!head || !start || length < 5 || !next || !match) goto error;

    if (start[0] != intro) goto error;
    if (start[1] != '=') goto error;

    char *ptr = start;
    char *val = NULL;
    size_t len = 0;

    while (*ptr == intro) {

        val = NULL;
        len = 0;
        list = NULL;

        if (!parse_content_line(
                ptr, length - (ptr - start), &val, &len, &ptr, intro, match))
            goto error;

        if (!val || len < 1) goto error;

        list = create_list_node(head);
        if (!list) goto error;

        list->value = val;

        /*
         *      resuse of the val pointer,
         *      to mark the end of the content string
         */

        val = val + len + 1;
        *val = 0;
    }

    *next = ptr;

    return true;
error:
    if (head) *head = ov_sdp_list_free(*head);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool create_masked_connection(ov_sdp_connection **head,
                                     char *line_start,
                                     size_t line_length,
                                     bool intro) {

    ov_sdp_connection *connection = NULL;

    /*
     *      connection-field =  'c' "="
     *          nettype SP addrtype SP unicast-address CRLF
     *
     */

    if (!head || !line_start || line_length < 7) goto error;

    char *ptr = line_start;
    char *space = NULL;

    if (intro) {

        if (line_start[0] != 'c') goto error;
        if (line_start[1] != '=') goto error;
        ptr += 2;
    }

    if (line_start[line_length - 2] != '\r') goto error;
    if (line_start[line_length - 1] != '\n') goto error;

    line_start[line_length - 2] = 0;
    line_start[line_length - 1] = 0;

    space = memchr(ptr, 0x20, line_length);
    if (!space) goto error;
    if (!ov_sdp_is_token(ptr, space - ptr)) goto error;

    connection = ov_sdp_connection_create();
    connection->nettype = ptr;

    ptr = space + 1;
    *space = 0;

    space = memchr(ptr, 0x20, line_length - (ptr - line_start));
    if (!space) goto error;
    if (!ov_sdp_is_token(ptr, space - ptr)) goto error;

    connection->addrtype = ptr;

    ptr = space + 1;
    *space = 0;

    /*
     *      address end already set above,
     *      the string at ptr MUST be a valid address.
     */
    connection->address = ptr;

    if (!ov_sdp_is_address(connection->address, strlen(connection->address)))
        goto error;

    if (!ov_node_push((void **)head, connection)) goto error;

    return true;
error:
    connection = ov_sdp_connection_free(connection);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool create_bandwith_dict(ov_dict **head,
                                 char *start,
                                 size_t length,
                                 char **next) {

    ov_dict *dict = NULL;
    char *key = NULL;

    if (!head || !start || length < 5 || !next) goto error;

    char intro = 'b';

    if (start[0] != intro) goto error;
    if (start[1] != '=') goto error;

    char *ptr = start;
    char *val = NULL;
    char *col = NULL;
    size_t len = 0;
    uint64_t nbr = 0;
    intptr_t value = 0;

    dict = ov_sdp_bandwidth_create();
    if (!dict) goto error;

    *head = dict;

    while (*ptr == intro) {

        val = NULL;
        len = 0;
        col = NULL;

        if (!parse_content_line(ptr,
                                length - (ptr - start),
                                &val,
                                &len,
                                &ptr,
                                intro,
                                ov_sdp_is_bandwidth))
            goto error;

        col = memchr(val, ':', len);
        if (!col) goto error;
        *col = 0;

        key = strdup(val);
        if (!key) goto error;

        len = len - (col + 1 - val);
        val = col + 1;
        col = col + len + 1;
        *col = 0;

        if (!parse_number(val, len, &nbr)) goto error;

        value = nbr;

        if (!ov_dict_set(dict, key, (void *)value, NULL)) goto error;

        key = NULL;
    }

    *next = ptr;
    return true;
error:
    if (key) free(key);

    ov_sdp_bandwidth_free(dict);
    if (head) *head = NULL;
    return false;
}

/*----------------------------------------------------------------------------*/

static bool create_masked_time(ov_sdp_time **head,
                               char *line,
                               size_t length,
                               char **next) {

    /*
     *  "t=0 0\r\n
     *
     *  time-fields =  1*( %x74 "=" start-time SP stop-time
     *          *(CRLF repeat-fields) CRLF)
     *           [zone-adjustments CRLF]
     */

    ov_sdp_time *data = NULL;
    ov_sdp_list *list = NULL;

    if (!head || !line || length < 7 || !next) goto error;

    char *space = NULL;
    char *out = (char *)line;
    char *ptr = NULL;
    char *last = NULL;
    uint64_t nbr = 0;

    if (out[0] != 't') goto error;

    while (out[0] == 't') {

        data = ov_sdp_time_create();

        if (!ov_node_push((void **)head, data)) {
            data = ov_sdp_time_free(data);
            goto error;
        }

        ptr = out;
        if (!validate_line(ptr, length - (ptr - line), &out)) goto error;

        ptr += 2;
        out[-1] = 0;
        out[-2] = 0;

        space = memchr(ptr, 0x20, (out - ptr));
        if (!space) goto error;

        if (!ov_sdp_is_digit(ptr, (space - ptr))) goto error;

        if (!parse_number(ptr, space - ptr, &nbr)) goto error;

        data->start = nbr;

        ptr = space + 1;

        if (!ov_sdp_is_time(ptr, (out - 2 - ptr))) goto error;

        if (!parse_number(ptr, (out - 2 - ptr), &nbr)) goto error;

        data->stop = nbr;

        while (out[0] == 'r') {

            /*
             *  "r=repeat-interval SP typed-time 1*(SP
             * typed-time)\r\n"
             *
             *  All array elements in separate lines:
             *
             *  EXAMPLES:
             *      (1) "r=1 1d 1\r\n"
             *      (2) "r=12345678 1d 1h 1m 1s\r\n"
             */

            ptr = out;
            last = out - 2;

            out[-1] = 0;
            out[-2] = 0;

            if (!validate_line(ptr, length - (ptr - line), &out)) goto error;

            ptr += 2;

            space = memchr(ptr, 0x20, (last - ptr));
            if (!space) goto error;

            if (!ov_sdp_is_typed_time(ptr, (space - ptr), false)) goto error;

            list = create_list_node(&data->repeat);
            if (!list) goto error;

            list->key = ptr;
            *space = 0;

            ptr = space + 1;

            space = memchr(ptr, 0x20, (last - ptr));
            if (!space) goto error;

            list->value = space + 1;

            while (space) {

                if (!ov_sdp_is_typed_time(ptr, (space - ptr), false))
                    goto error;

                ptr = space + 1;
                space = memchr(ptr, 0x20, (out - ptr));
            }

            if (!ov_sdp_is_typed_time(ptr, (out - ptr) - 2, false)) goto error;
        }

        if (out[0] == 'z') {

            /*
             *  zone-adjustments = %x7a "="
             *      time SP ["-"] typed-time
             * *(SP time SP ["-"] typed-time)
             *
             *  All array elements in one line:
             *
             *  EXAMPLES:
             *      (1) "z=1 -1d\r\n"
             *      (2) "z=1 1d 2 2d 3 3\r\n"
             */

            ptr = out + 2;

            out[-1] = 0;
            out[-2] = 0;

            if (!validate_line(out, length - (out - line), &out)) goto error;

            do {
                space = memchr(ptr, 0x20, (out - ptr));
                if (!space) goto error;

                if (!ov_sdp_is_typed_time(ptr, (space - ptr), false))
                    goto error;

                list = create_list_node(&data->zone);
                if (!list) goto error;

                list->key = ptr;
                *space = 0;

                ptr = space + 1;
                space = memchr(ptr, 0x20, (out - ptr));
                list->value = ptr;

                if (!space) {

                    if (!ov_sdp_is_typed_time(ptr, (out - ptr) - 2, true))
                        goto error;

                } else {

                    if (!ov_sdp_is_typed_time(ptr, (space - ptr), true))
                        goto error;

                    ptr = space + 1;
                }

            } while (space);
        }
    }

    *next = out;

    return true;
error:
    if (head) *head = ov_sdp_time_free(*head);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool create_masked_attributes(ov_sdp_list **head,
                                     char *start,
                                     size_t length,
                                     char **next) {

    ov_sdp_list *list = NULL;

    /*
     *  EXAMPLES:
     *      (1) "a=attribute\r\n"
     *      (2) "a=attribute\r\na=attribute\r\na=attribute\r\n"
     *
     * SPECIFICATION:
     *
     *  attribute-fields =    *(%x61 "=" attribute CRLF)
     *
     *  ; sub-rules of 'a='
     *  attribute =           (att-field ":" att-value) / att-field
     *
     *  att-field =           token
     *
     *  att-value =           byte-string
     */

    if (!head || !start || length < 5 || !next) goto error;

    char intro = 'a';

    if (start[0] != intro) goto error;
    if (start[1] != '=') goto error;

    char *out = (char *)start;
    char *ptr = NULL;
    char *col = NULL;

    if (out[0] != 'a') goto error;

    while (*out == intro) {

        ptr = out + 2;
        if (!validate_line(out, length - (out - start), &out)) goto error;

        out[-1] = 0;
        out[-2] = 0;

        col = memchr(ptr, ':', out - ptr);

        list = create_list_node(head);
        if (!list) goto error;
        list->key = ptr;

        if (col) {

            *col = 0;

            if (!ov_sdp_is_token(ptr, (col - ptr))) goto error;

            ptr = col + 1;
            if (!ov_sdp_is_byte_string(ptr, (out - ptr) - 2)) goto error;

            list->value = ptr;

        } else {

            if (!ov_sdp_is_token(ptr, (out - ptr) - 2)) goto error;
        }
    }

    *next = out;
    return true;
error:
    if (head) *head = ov_sdp_list_free(*head);

    return false;
}

/*----------------------------------------------------------------------------*/

static bool masked_media(ov_sdp_description *desc,
                         const char *start,
                         size_t size,
                         char **next) {

    ov_sdp_list *list = NULL;

    if (!desc || !start || size < 11 || !next) goto error;

    if (start[0] != 'm') goto error;

    /*
     *  EXAMPLES:
     *      (1) "m=1 0 1 1\r\n"
     *
     * SPECIFICATION:
     *
     *   media-field =      %x6d "=" media SP port ["/" integer]
     *                      SP proto 1*(SP fmt) CRLF
     */

    char *ptr = (char *)start;
    char *space = NULL;
    char *nxt = NULL;
    char *end = NULL;

    uint32_t port = 0;

    if (!validate_line(ptr, size - (ptr - start), &nxt)) goto error;

    ptr += 2;

    nxt[-1] = 0;
    nxt[-2] = 0;

    *next = nxt;

    space = memchr(ptr, ' ', (nxt - ptr));
    if (!space) goto error;

    if (!ov_sdp_is_token(ptr, space - ptr)) goto error;

    *space = 0;
    desc->media.name = ptr;

    ptr = space + 1;

    space = memchr(ptr, ' ', (nxt - ptr));
    if (!space) goto error;
    *space = 0;

    port = strtol(ptr, &end, 10);
    if (!end) goto error;

    if (((end - ptr < 1)) || ((end - ptr) > 5)) goto error;

    desc->media.port = port;

    ptr = end;

    if (*ptr == '/') {

        *ptr = 0;
        ptr++;

        port = strtol(ptr, &end, 10);
        if (!end) goto error;

        if (((end - ptr < 1)) || ((end - ptr) > 5)) goto error;

        desc->media.port2 = port;

        ptr = end;
    }

    if (end != space) goto error;

    ptr = space + 1;
    space = memchr(ptr, ' ', (nxt - ptr));
    if (!space) goto error;

    if (!ov_sdp_is_proto(ptr, (space - ptr))) goto error;

    *space = 0;
    desc->media.protocol = ptr;

    ptr = space + 1;

    space = memchr(ptr, ' ', (nxt - ptr));

    while (space) {

        if (!ov_sdp_is_token(ptr, (space - ptr))) goto error;

        list = create_list_node(&desc->media.formats);
        if (!list) goto error;

        *space = 0;
        list->value = ptr;

        ptr = space + 1;
        space = memchr(ptr, ' ', (nxt - ptr));
    }

    if (!ov_sdp_is_token(ptr, (nxt - 2 - ptr))) goto error;

    list = create_list_node(&desc->media.formats);
    if (!list) goto error;

    list->value = ptr;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool create_masked_descriptions(ov_sdp_description **head,
                                       char *line,
                                       size_t length,
                                       char **next) {

    ov_sdp_description *desc = NULL;

    if (!line || !next || length < 11) goto error;

    char *ptr = NULL;
    char *out = (char *)line;
    char *val = NULL;
    size_t len = 0;

    if (out[0] != 'm') goto error;

    while (out[0] == 'm') {

        ptr = out;
        if (!validate_line(out, length - (out - line), &out)) goto error;

        desc = ov_sdp_description_create();

        if (!ov_node_push((void **)head, desc)) {
            desc = ov_sdp_description_free(desc);
            goto error;
        }

        if (!masked_media(desc, ptr, (out - ptr), &out)) goto error;

        ptr = out;

        // OPTIONAL INFO

        if (*ptr == 'i') {

            if (!validate_line(ptr, length - (ptr - line), &out)) goto error;

            if (!parse_information(ptr, (out - ptr), &val, &len)) goto error;

            desc->info = val;
            val = (char *)desc->info + len + 1;
            *val = 0;

            out[-1] = 0;
            out[-2] = 0;
            ptr = out;
        }

        // OPTIONAL CONNECTIONS

        while (*ptr == 'c') {

            if (!validate_line(ptr, length - (ptr - line), &out)) goto error;

            if (!create_masked_connection(
                    &desc->connection, ptr, (out - ptr), true))
                goto error;

            out[-1] = 0;
            out[-2] = 0;
            ptr = out;
        }

        // OPTIONAL BANDWIDTH

        if (*ptr == 'b') {

            if (!create_bandwith_dict(
                    &desc->bandwidth, ptr, length - (ptr - line), &out))
                goto error;

            out[-1] = 0;
            out[-2] = 0;
            ptr = out;
        }

        // OPTIONAL KEY

        if (*ptr == 'k') {

            if (!parse_content_line(
                    ptr, length - (ptr - line), &val, &len, &out, 'k', NULL))
                goto error;

            if (!ov_sdp_is_key(val, len)) goto error;

            desc->key = val;
            val = (char *)desc->key + len + 1;
            *val = 0;

            out[-1] = 0;
            out[-2] = 0;
            ptr = out;
        }

        // OPTIONAL ATTRIBUTES

        if (*ptr == 'a') {

            if (!create_masked_attributes(
                    &desc->attributes, ptr, length - (ptr - line), &out))
                goto error;

            out[-1] = 0;
            out[-2] = 0;
        }
    }

    *next = ptr;
    return true;
error:

    if (head) *head = ov_sdp_description_free(*head);
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #WRITE #SERIALIZE #STRINGIY
 *
 *      ------------------------------------------------------------------------
 */

static bool validate_connection_pointers(ov_sdp_connection *connection) {

    OV_ASSERT(connection);

    while (connection) {

        if (!connection->nettype) goto error;
        if (!connection->addrtype) goto error;
        if (!connection->address) goto error;

        if (!ov_sdp_is_token(
                connection->nettype, strlen(connection->nettype))) {
            ov_log_error(
                "SDP connection nettype %s"
                "- NOT an SDP token",
                connection->nettype);
            goto error;
        }

        if (!ov_sdp_is_token(
                connection->addrtype, strlen(connection->addrtype))) {
            ov_log_error(
                "SDP connection addrtype %s"
                "- NOT an SDP token",
                connection->addrtype);
            goto error;
        }

        if (!ov_sdp_is_address(
                connection->address, strlen(connection->address))) {
            ov_log_error(
                "SDP connection address %s"
                "- NOT an SDP address",
                connection->address);
            goto error;
        }

        connection = ov_node_next(connection);
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_vos(const ov_sdp_session *session) {

    if (!session) goto error;

    if (!session->origin.name || !session->origin.connection.nettype ||
        !session->origin.connection.addrtype ||
        !session->origin.connection.address || !session->name)
        goto error;

    if (!ov_sdp_is_text(session->name, strlen(session->name))) {
        ov_log_error(
            "SDP session name %s"
            "- NOT an SDP text",
            session->name);
        goto error;
    }

    if (!validate_connection_pointers(
            (ov_sdp_connection *)&session->origin.connection))
        goto error;

    if (!ov_sdp_is_username(
            session->origin.name, strlen(session->origin.name))) {
        ov_log_error(
            "SDP origin name %s"
            "- NOT an SDP username",
            session->origin.name);
        goto error;
    }

    if (!ov_sdp_is_username(
            session->origin.name, strlen(session->origin.name))) {
        ov_log_error(
            "SDP origin name %s"
            "- NOT an SDP username",
            session->origin.name);
        goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool write_vos(const ov_sdp_session *session,
                      char *start,
                      size_t size,
                      bool escaped,
                      char **next) {

    OV_ASSERT(session);
    OV_ASSERT(start);
    OV_ASSERT(next);

    if (!session || !start || !next) goto error;

    char *lineend = "\r\n";
    if (escaped) lineend = "\\r\\n";

    size_t len = snprintf(start,
                          size,
                          "v=%i%s"
                          "o=%s %" PRIu64 " %" PRIu64
                          " %s %s %s%s"
                          "s=%s%s",
                          session->version,
                          lineend,
                          session->origin.name,
                          session->origin.id,
                          session->origin.version,
                          session->origin.connection.nettype,
                          session->origin.connection.addrtype,
                          session->origin.connection.address,
                          lineend,
                          session->name,
                          lineend);

    if ((len == 0) || (len >= size)) goto error;

    *next = start + len;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool write_line(char *start,
                       size_t size,
                       bool escaped,
                       char **next,
                       char intro,
                       const char *content) {

    if (!start || size < 5 || !next || !content) goto error;

    char *lineend = "\r\n";
    if (escaped) lineend = "\\r\\n";

    char *ptr = start;

    start[0] = intro;
    ptr++;
    size--;

    size_t len = snprintf(ptr, size, "=%s%s", content, lineend);

    if ((len == 0) || (len >= size)) goto error;

    *next = ptr + len;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool write_node_lines(char *start,
                             size_t size,
                             bool escaped,
                             char **next,
                             char intro,
                             ov_sdp_list *node) {

    OV_ASSERT(start);
    OV_ASSERT(next);
    OV_ASSERT(node);

    char *ptr = start;

    while (node) {

        if (!node->value) goto error;

        if (!write_line(
                ptr, size - (ptr - start), escaped, &ptr, intro, node->value))
            goto error;

        node = ov_node_next(node);
    }

    *next = ptr;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_pointer_list(ov_sdp_list *node,
                                  const char *name,
                                  bool (*match)(const char *, uint64_t)) {

    if (!node) goto error;

    while (node) {

        if (!node->value) goto error;

        if (!match(node->value, strlen(node->value))) {
            ov_log_error("SDP %s invalid content %s", name, node->value);
            goto error;
        }

        node = ov_node_next(node);
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool write_connections(ov_sdp_connection *connection,
                              char *start,
                              size_t size,
                              bool escaped,
                              char **next) {

    OV_ASSERT(start);
    OV_ASSERT(next);
    OV_ASSERT(connection);

    char *ptr = start;

    char *lineend = "\r\n";
    if (escaped) lineend = "\\r\\n";

    size_t len = 0;

    while (connection) {

        len = snprintf(ptr,
                       size - (ptr - start),
                       "c=%s %s %s%s",
                       connection->nettype,
                       connection->addrtype,
                       connection->address,
                       lineend);

        if ((len == 0) || (len >= size)) goto error;

        ptr += len;
        connection = ov_node_next(connection);
    }

    *next = ptr;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_bandwith_entry(const void *key, void *value, void *data) {

    if (!key) return true;

    char *string = (char *)key;

    if (!ov_sdp_is_token(string, strlen(string))) {
        ov_log_error(
            "SDP bandwidth key %s"
            "- NOT an SDP token",
            string);
        return false;
    }

    if (value || data) { /* unused */
    }
    return true;
}

/*----------------------------------------------------------------------------*/

static bool validate_bandwith(ov_dict *dict) {

    if (!dict) goto error;
    if (ov_dict_is_empty(dict)) return true;

    return ov_dict_for_each(dict, NULL, validate_bandwith_entry);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

struct container_bandwidth_write {

    const char *lineend;
    size_t size;
    char *ptr;
};

/*----------------------------------------------------------------------------*/

static bool write_bandwith_entry(const void *key, void *value, void *data) {

    if (!key) return true;
    if (!data) return false;

    intptr_t val = (intptr_t)value;
    char *name = (char *)key;

    struct container_bandwidth_write *container =
        (struct container_bandwidth_write *)data;

    size_t len = snprintf(container->ptr,
                          container->size,
                          "b=%s:%" PRIu64 "%s",
                          name,
                          (uint64_t)val,
                          container->lineend);

    if ((len == 0) || (len >= container->size)) goto error;

    container->ptr += len;
    container->size -= len;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool write_bandwith(
    ov_dict *bandwidth, char *start, size_t size, bool escaped, char **next) {

    OV_ASSERT(start);
    OV_ASSERT(next);
    OV_ASSERT(bandwidth);

    if (ov_dict_is_empty(bandwidth)) {
        *next = start;
        return true;
    }

    char *lineend = "\r\n";
    if (escaped) lineend = "\\r\\n";

    struct container_bandwidth_write container =
        (struct container_bandwidth_write){

            .lineend = lineend, .ptr = start, .size = size};

    if (!ov_dict_for_each(bandwidth, &container, write_bandwith_entry))
        goto error;

    *next = container.ptr;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_time_write(const ov_sdp_session *session) {

    if (!session) goto error;

    if (!session->time) goto error;

    ov_sdp_time *time_node = (ov_sdp_time *)session->time;
    ov_sdp_list *node = NULL;

    while (time_node) {

        if (0 != time_node->start)
            if (time_node->start < 999999999) {
                ov_log_error(
                    "SDP time start NOT 0 "
                    "but less than 10 digits");
                goto error;
            }

        if (0 != time_node->stop)
            if (time_node->stop < 999999999) {
                ov_log_error(
                    "SDP time stop NOT 0 "
                    "but less than 10 digits");
                goto error;
            }

        node = session->time->repeat;

        while (node) {

            if (!node->key) {
                ov_log_error(
                    "SDP time repeat "
                    "missing key");
                goto error;
            }

            if (!node->value) {
                ov_log_error(
                    "SDP time repeat "
                    "missing value");
                goto error;
            }

            size_t len = strlen(node->value);
            char *ptr = (char *)node->value;
            char *space = memchr(ptr, ' ', len);

            while (space) {

                if (!ov_sdp_is_typed_time(ptr, (space - ptr), false)) {
                    ov_log_error(
                        "SDP time repeat %s "
                        "NOT typed time clean",
                        node->value);
                    goto error;
                }

                ptr = space + 1;
                space = memchr(ptr, ' ', len - (ptr - node->value));
            }

            if (!ov_sdp_is_typed_time(ptr, len - (ptr - node->value), false)) {
                ov_log_error(
                    "SDP time repeat %s "
                    "NOT typed time clean",
                    node->value);
                goto error;
            }

            node = ov_node_next(node);
        }

        node = session->time->zone;

        while (node) {

            if (!node->key) {
                ov_log_error(
                    "SDP time zone "
                    "missing key");
                goto error;
            }

            if (!node->value) {
                ov_log_error(
                    "SDP time zone "
                    "missing value");
                goto error;
            }

            size_t len = strlen(node->value);
            char *ptr = (char *)node->value;
            char *space = memchr(ptr, ' ', len);

            while (space) {

                if (!ov_sdp_is_typed_time(ptr, (space - ptr), true)) {
                    ov_log_error(
                        "SDP time zone %s "
                        "NOT typed time clean",
                        node->value);
                    goto error;
                }

                ptr = space + 1;
                space = memchr(ptr, ' ', len - (ptr - node->value));
            }

            if (!ov_sdp_is_typed_time(ptr, len - (ptr - node->value), true)) {
                ov_log_error(
                    "SDP time zone %s "
                    "NOT typed time clean",
                    node->value);
                goto error;
            }

            node = ov_node_next(node);
        }

        time_node = ov_node_next(time_node);
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool write_time_entries(ov_sdp_list *node,
                               char *start,
                               size_t size,
                               char intro,
                               char *end,
                               char **next) {

    if (!node || !start || !end || !next) goto error;

    char *ptr = start;
    size_t len = 0;

    while (node) {

        if (size < 5) goto error;

        OV_ASSERT(node->key);
        if (!node->key) goto error;

        OV_ASSERT(node->value);
        if (!node->value) goto error;

        len = snprintf(
            ptr, size, "%c=%s %s%s", intro, node->key, node->value, end);

        if ((len == 0) || (len >= size)) goto error;

        ptr += len;
        size -= len;

        node = ov_node_next(node);
    }

    *next = ptr;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool write_time(const ov_sdp_session *session,
                       char *start,
                       size_t size,
                       bool escaped,
                       char **next) {

    OV_ASSERT(session);
    OV_ASSERT(start);
    OV_ASSERT(next);

    char *lineend = "\r\n";
    if (escaped) lineend = "\\r\\n";

    size_t len = 0;
    char *ptr = start;
    size_t orig = size;

    ov_sdp_time *node = session->time;

    while (node) {

        len = snprintf(ptr,
                       size,
                       "t=%" PRIu64 " %" PRIu64 "%s",
                       node->start,
                       node->stop,
                       lineend);

        if ((len == 0) || (len >= size)) goto error;

        ptr += len;
        size -= len;

        if (node->repeat) {

            if (!write_time_entries(
                    session->time->repeat, ptr, size, 'r', lineend, &ptr))
                goto error;

            size = orig - (ptr - start);
        }

        if (node->zone) {

            if (!write_time_entries(
                    session->time->zone, ptr, size, 'z', lineend, &ptr))
                goto error;

            size = orig - (ptr - start);
        }

        node = ov_node_next(node);
    }

    *next = ptr;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_attributes_write(const ov_sdp_list *attributes) {

    if (!attributes) goto error;

    ov_sdp_list *attr = (ov_sdp_list *)attributes;

    while (attr) {

        if (!attr->key) {

            ov_log_error(
                "SDP attribute "
                "missing key");
            goto error;
        }

        if (!ov_sdp_is_token(attr->key, strlen(attr->key))) {

            ov_log_error(
                "SDP attribute key %s "
                "NOT a valid sdp token",
                attr->key);
            goto error;
        }

        if (attr->value) {

            if (0 != strlen(attr->value))
                if (!ov_sdp_is_byte_string(attr->value, strlen(attr->value))) {

                    ov_log_error(
                        "SDP attribute key %s "
                        "value %s NOT a valid "
                        "byte string",
                        attr->key,
                        attr->value);
                    goto error;
                }
        }

        attr = ov_node_next(attr);
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool write_attributes(const ov_sdp_list *attributes,
                             char *start,
                             size_t size,
                             bool escaped,
                             char **next) {

    if (!attributes || !start || !next) goto error;

    char *lineend = "\r\n";
    if (escaped) lineend = "\\r\\n";

    size_t len = 0;

    char *ptr = start;

    ov_sdp_list *attr = (ov_sdp_list *)attributes;

    while (attr) {

        if (attr->value && strlen(attr->value) > 0) {

            len = snprintf(
                ptr, size, "a=%s:%s%s", attr->key, attr->value, lineend);

        } else {

            len = snprintf(ptr, size, "a=%s%s", attr->key, lineend);
        }

        if ((len == 0) || (len >= size)) goto error;

        ptr += len;
        size -= len;

        attr = ov_node_next(attr);
    }

    *next = ptr;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool validate_description_write(const ov_sdp_description *description) {

    if (!description) goto error;

    ov_sdp_description *desc = (ov_sdp_description *)description;
    ov_sdp_list *node = NULL;

    while (desc) {

        if (!desc->media.name || !desc->media.protocol || !desc->media.formats)
            goto error;

        if (!ov_sdp_is_token(desc->media.name, strlen(desc->media.name))) {

            ov_log_error(
                "SDP media description name "
                "%s NOT a valid SDP token",
                desc->media.name);

            goto error;
        }

        if (!ov_sdp_is_proto(
                desc->media.protocol, strlen(desc->media.protocol))) {

            ov_log_error(
                "SDP media description protocol "
                "%s NOT a valid SDP protocol",
                desc->media.protocol);

            goto error;
        }

        node = desc->media.formats;
        while (node) {

            if (!node->value) {

                ov_log_error(
                    "SDP media description "
                    "format empty");
                goto error;
            }

            if (!ov_sdp_is_token(node->value, strlen(node->value))) {

                ov_log_error(
                    "SDP media description "
                    "format %s NOT a valid SDP token",
                    node->value);

                goto error;
            }

            node = ov_node_next(node);
        }

        if (desc->info) {

            if (!ov_sdp_is_text(desc->info, strlen(desc->info))) {
                ov_log_error(
                    "SDP media description "
                    "info %s - NOT an SDP text",
                    desc->info);
                goto error;
            }
        }

        if (desc->connection) {

            if (!validate_connection_pointers(desc->connection)) goto error;
        }

        if (desc->bandwidth) {

            if (!validate_bandwith(desc->bandwidth)) goto error;
        }

        if (desc->key) {

            if (!ov_sdp_is_key(desc->key, strlen(desc->key))) {
                ov_log_error(
                    "SDP media description "
                    "key %s - NOT an SDP key",
                    desc->key);
                goto error;
            }
        }

        if (desc->attributes) {

            if (!validate_attributes_write(desc->attributes)) goto error;
        }

        desc = ov_node_next(desc);
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool write_description(const ov_sdp_description *description,
                              char *start,
                              size_t size,
                              bool escaped,
                              char **next) {

    if (!description || !start || !next) goto error;

    char *lineend = "\r\n";
    if (escaped) lineend = "\\r\\n";

    size_t len = 0;

    char *ptr = start;
    size_t orig = size;

    ov_sdp_description *desc = (ov_sdp_description *)description;
    ov_sdp_list *node = NULL;

    while (desc) {

        if (0 != desc->media.port2) {

            len = snprintf(ptr,
                           size,
                           "m=%s %i/%i %s",
                           desc->media.name,
                           desc->media.port,
                           desc->media.port2,
                           desc->media.protocol);

        } else {

            len = snprintf(ptr,
                           size,
                           "m=%s %i %s",
                           desc->media.name,
                           desc->media.port,
                           desc->media.protocol);
        }

        if ((len == 0) || (len >= size)) goto error;

        ptr += len;
        size -= len;

        node = desc->media.formats;
        while (node) {

            len = snprintf(ptr, size, " %s", node->value);

            if ((len == 0) || (len >= size)) goto error;

            ptr += len;
            size -= len;

            node = ov_node_next(node);
        }

        len = snprintf(ptr, size, "%s", lineend);

        if ((len == 0) || (len >= size)) goto error;

        ptr += len;
        size -= len;

        if (desc->info) {

            if (!write_line(ptr, size, escaped, &ptr, 'i', desc->info))
                goto error;

            size = orig - (ptr - start);
        }

        if (desc->connection) {

            if (!write_connections(desc->connection, ptr, size, escaped, &ptr))
                goto error;

            size = orig - (ptr - start);
        }

        if (desc->bandwidth) {

            if (!write_bandwith(desc->bandwidth, ptr, size, escaped, &ptr))
                goto error;

            size = orig - (ptr - start);
        }

        if (desc->key) {

            if (!write_line(ptr, size, escaped, &ptr, 'k', desc->key))
                goto error;

            size = orig - (ptr - start);
        }

        if (desc->attributes) {

            if (!write_attributes(desc->attributes, ptr, size, escaped, &ptr))
                goto error;

            size = orig - (ptr - start);
        }

        desc = ov_node_next(desc);
    }

    *next = ptr;
    return true;
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #JSON
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

static ov_json_value *connection_create(const ov_sdp_connection *connection) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!connection) {
        out = ov_json_null();
        return out;
    }

    out = ov_json_object();

    val = ov_json_string(connection->nettype);
    if (!ov_json_object_set(out, OV_KEY_TYPE, val)) goto error;

    val = ov_json_string(connection->addrtype);
    if (!ov_json_object_set(out, OV_KEY_ADDRESS_TYPE, val)) goto error;

    val = ov_json_string(connection->address);
    if (!ov_json_object_set(out, OV_KEY_ADDRESS, val)) goto error;

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *origin_create(const ov_sdp_session *session) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!session) goto error;

    out = ov_json_object();
    if (!out) goto error;

    val = ov_json_string(session->origin.name);
    if (!ov_json_object_set(out, OV_KEY_NAME, val)) goto error;

    val = ov_json_number(session->origin.id);
    if (!ov_json_object_set(out, OV_KEY_ID, val)) goto error;

    val = ov_json_number(session->origin.version);
    if (!ov_json_object_set(out, OV_KEY_VERSION, val)) goto error;

    val = connection_create(&session->origin.connection);
    if (!ov_json_object_set(out, OV_KEY_CONNECTION, val)) goto error;

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *time_create(ov_sdp_time *node) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *obj = NULL;

    ov_json_value *repeat = NULL;
    ov_json_value *zone = NULL;

    out = ov_json_array();
    if (!out) goto error;

    ov_sdp_list *list = NULL;

    while (node) {

        repeat = NULL;
        zone = NULL;

        obj = ov_json_object();
        if (!ov_json_array_push(out, obj)) goto error;

        val = ov_json_number(node->start);
        if (!ov_json_object_set(obj, OV_KEY_START, val)) goto error;

        val = ov_json_number(node->stop);
        if (!ov_json_object_set(obj, OV_KEY_STOP, val)) goto error;

        if (node->repeat) {
            repeat = ov_json_array();

            if (!ov_json_object_set(obj, OV_KEY_REPEAT, repeat)) {
                repeat = ov_json_value_free(repeat);
                goto error;
            }
        }

        if (node->zone) {
            zone = ov_json_array();

            if (!ov_json_object_set(obj, OV_KEY_ZONE, zone)) {
                zone = ov_json_value_free(zone);
                goto error;
            }
        }

        obj = NULL;

        list = node->repeat;
        while (list) {

            if (!list->key || !list->value) {

                list = ov_node_next(list);
                continue;
            }

            obj = ov_json_object();
            val = ov_json_string(list->value);
            if (!ov_json_object_set(obj, list->key, val)) goto error;

            val = NULL;
            if (!ov_json_array_push(repeat, obj)) goto error;

            list = ov_node_next(list);
        }

        list = node->zone;
        while (list) {

            if (!list->key || !list->value) {

                list = ov_node_next(list);
                continue;
            }

            obj = ov_json_object();
            val = ov_json_string(list->value);
            if (!ov_json_object_set(obj, list->key, val)) goto error;

            val = NULL;
            if (!ov_json_array_push(zone, obj)) goto error;

            list = ov_node_next(list);
        }

        node = ov_node_next(node);
    }

    return out;
error:
    ov_json_value_free(obj);
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *list_array_create(const ov_sdp_list *list) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!list) return ov_json_null();

    out = ov_json_array();
    if (!out) goto error;

    ov_sdp_list *node = (ov_sdp_list *)list;

    while (node) {

        val = ov_json_string(node->value);
        if (!ov_json_array_push(out, val)) goto error;

        node = ov_node_next(node);
    }

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

struct bwd {

    ov_json_value *object;
};

/*----------------------------------------------------------------------------*/

static bool add_bandwidth_item(const void *key, void *item, void *data) {

    if (!key) return true;
    if (!data) goto error;

    struct bwd *container = (struct bwd *)data;

    char *k = (char *)key;
    intptr_t nbr = (intptr_t)item;

    ov_json_value *val = ov_json_number(nbr);
    if (!val) goto error;

    if (ov_json_object_set(container->object, k, val)) return true;

    val = ov_json_value_free(val);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *bandwidth_create(ov_dict *dict) {

    ov_json_value *out = NULL;

    if (!dict) return ov_json_null();

    out = ov_json_object();
    if (!out) goto error;

    struct bwd container = (struct bwd){.object = out};

    if (!ov_dict_for_each(dict, &container, add_bandwidth_item)) goto error;

    return out;
error:
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *attributes_create(const ov_sdp_list *list) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *arr = NULL;

    if (!list) return ov_json_null();

    out = ov_json_object();
    if (!out) goto error;

    ov_sdp_list *node = (ov_sdp_list *)list;

    ov_json_value *exists = NULL;

    while (node) {

        if (!node->key) {

            node = ov_node_next(node);
            continue;
        }

        exists = ov_json_object_get(out, node->key);

        if (exists) {

            if (!node->value) goto error;

            if (ov_json_is_null(exists)) goto error;

            if (ov_json_is_array(exists)) {

                val = ov_json_string(node->value);
                if (!ov_json_array_push(exists, val)) goto error;

            } else {

                if (!ov_json_value_unset_parent(exists)) goto error;

                /*
                 *      unplugged exists
                 */

                arr = ov_json_array();
                if (!arr) {
                    exists = ov_json_value_free(exists);
                    goto error;
                }

                if (!ov_json_array_push(arr, exists)) {
                    exists = ov_json_value_free(exists);
                    goto error;
                }

                val = ov_json_string(node->value);
                if (!ov_json_array_push(arr, val)) goto error;

                if (!ov_json_object_set(out, node->key, arr)) goto error;
            }

        } else if (node->value) {

            val = ov_json_string(node->value);
            if (!ov_json_object_set(out, node->key, val)) goto error;

        } else {

            val = ov_json_null();
            if (!ov_json_object_set(out, node->key, val)) goto error;
        }

        node = ov_node_next(node);
    }

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(arr);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *description_create(
    const ov_sdp_description *description) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *obj = NULL;
    ov_json_value *arr = NULL;

    if (!description) return ov_json_null();

    ov_sdp_description *desc = (ov_sdp_description *)description;
    ov_sdp_list *node = NULL;

    out = ov_json_array();
    if (!out) goto error;

    while (desc) {

        obj = ov_json_object();
        if (!obj) goto error;

        val = ov_json_string(desc->media.name);
        if (!ov_json_object_set(obj, OV_KEY_TYPE, val)) goto error;

        val = ov_json_string(desc->media.protocol);
        if (!ov_json_object_set(obj, OV_KEY_PROTOCOL, val)) goto error;

        val = ov_json_number(desc->media.port);
        if (!ov_json_object_set(obj, OV_KEY_PORT, val)) goto error;

        if (0 != desc->media.port2) {

            val = ov_json_number(desc->media.port2);
            if (!ov_json_object_set(obj, OV_KEY_PORT_RTCP, val)) goto error;
        }

        if (!desc->media.formats) {
            val = ov_json_null();
        } else {
            arr = ov_json_array();
            node = desc->media.formats;
            while (node) {
                val = ov_json_string(node->value);
                if (!ov_json_array_push(arr, val)) goto error;
                node = ov_node_next(node);
            }
            val = arr;
            arr = NULL;
        }

        if (!ov_json_object_set(obj, OV_KEY_FORMAT, val)) goto error;

        /*
         *      optional items
         */
        if (desc->info) {

            val = ov_json_string(desc->info);
            if (!ov_json_object_set(obj, OV_KEY_INFO, val)) goto error;
        }

        if (desc->key) {

            val = ov_json_string(desc->key);
            if (!ov_json_object_set(obj, OV_KEY_KEY, val)) goto error;
        }

        if (desc->connection) {

            arr = ov_json_array();
            if (!arr) goto error;

            ov_sdp_connection *c = desc->connection;
            while (c) {

                val = connection_create(c);
                if (!ov_json_array_push(arr, val)) goto error;

                c = ov_node_next(c);
            }

            if (!ov_json_object_set(obj, OV_KEY_CONNECTION, arr)) goto error;

            arr = NULL;
        }

        if (desc->bandwidth) {

            val = bandwidth_create(desc->bandwidth);
            if (!ov_json_object_set(obj, OV_KEY_BANDWIDTH, val)) goto error;
        }

        if (desc->attributes) {

            val = attributes_create(desc->attributes);
            if (!ov_json_object_set(obj, OV_KEY_ATTRIBUTES, val)) goto error;
        }

        if (!ov_json_array_push(out, obj)) goto error;
        desc = ov_node_next(desc);
    }

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(obj);
    ov_json_value_free(arr);
    return NULL;
}

/*----------------------------------------------------------------------------*/

struct json_parse_time_container {

    ov_json_value const *value;
    ov_sdp_time *head;
};

/*----------------------------------------------------------------------------*/

struct json_parse_list {

    ov_sdp_list *head;
    const char *key;
};

/*----------------------------------------------------------------------------*/

static bool json_parse_object_array_to_ov_sdp_list(void *item, void *data) {

    struct json_parse_list *container = NULL;
    ov_sdp_list *node = NULL;

    if (!item || !ov_json_is_string(item)) return false;

    container = (struct json_parse_list *)data;

    node = ov_sdp_list_create();
    if (!node) goto error;

    node->key = container->key;
    node->value = ov_json_string_get(item);

    if (ov_node_push((void **)&container->head, node)) return true;
error:
    node = ov_sdp_list_free(node);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool sdp_parse_object_to_ov_sdp_list(const void *key,
                                            void *item,
                                            void *data) {

    if (!key) return true;

    struct json_parse_list *container = NULL;
    ov_sdp_list *node = NULL;

    if (!data || !item) goto error;
    container = (struct json_parse_list *)data;

    if (ov_json_is_array(item)) {

        if (ov_json_array_is_empty(item)) return false;

        container->key = (const char *)key;

        return ov_json_array_for_each(
            item, container, json_parse_object_array_to_ov_sdp_list);
    }

    if (!ov_json_is_null(item))
        if (!ov_json_is_string(item)) goto error;

    node = ov_sdp_list_create();
    if (!node) goto error;

    node->key = (char *)key;

    ov_json_value *value = ov_json_value_cast(item);

    if (ov_json_is_null(value)) {

        if (!ov_node_push((void **)&container->head, node)) goto error;

        return true;
    }

    if (ov_json_is_string(value)) {

        if (!ov_node_push((void **)&container->head, node)) goto error;

        node->value = ov_json_string_get(value);
        return true;
    }

    ov_log_error("JSON to SDP cannot parse value at %s", node->key);
error:
    node = ov_sdp_list_free(node);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool json_parse_time_list(void *item, void *data) {

    if (!ov_json_is_object(item) || !data) goto error;

    return ov_json_object_for_each(item, data, sdp_parse_object_to_ov_sdp_list);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool json_parse_time(void *item, void *data) {

    ov_sdp_time *time = NULL;
    ov_sdp_list *rlist = NULL;
    ov_sdp_list *zlist = NULL;

    if (!ov_json_is_object(item) || !data) goto error;

    struct json_parse_time_container *container =
        (struct json_parse_time_container *)data;

    ov_json_value *source = (ov_json_value_cast(item));
    ov_json_value *start = NULL;
    ov_json_value *stop = NULL;
    ov_json_value *repeat = NULL;
    ov_json_value *zone = NULL;

    struct json_parse_list container2 = (struct json_parse_list){.head = NULL};

    start = ov_json_object_get(source, OV_KEY_START);
    if (!ov_json_is_number(start)) goto error;

    stop = ov_json_object_get(source, OV_KEY_STOP);
    if (!ov_json_is_number(stop)) goto error;

    repeat = ov_json_object_get(source, OV_KEY_REPEAT);
    if (repeat) {

        container2.head = NULL;
        container2.key = NULL;

        if (!ov_json_is_array(repeat)) goto error;

        if (!ov_json_array_for_each(repeat, &container2, json_parse_time_list))
            goto error;

        rlist = container2.head;
        container2.head = NULL;
    }

    zone = ov_json_object_get(source, OV_KEY_ZONE);
    if (zone) {

        container2.head = NULL;
        container2.key = NULL;

        if (!ov_json_is_array(repeat)) goto error;

        if (!ov_json_array_for_each(zone, &container2, json_parse_time_list))
            goto error;

        zlist = container2.head;
        container2.head = NULL;
    }

    time = ov_sdp_time_create();
    if (!time) goto error;

    time->start = ov_json_number_get(start);
    time->stop = ov_json_number_get(stop);
    time->repeat = rlist;
    time->zone = zlist;

    if (!ov_node_push((void **)&container->head, time)) goto error;

    return true;
error:
    if (rlist) rlist = ov_data_pointer_free(rlist);
    if (zlist) zlist = ov_data_pointer_free(zlist);
    if (time) time = ov_data_pointer_free(time);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool json_parse_connection(const ov_json_value *obj,
                                  ov_sdp_connection *connection) {

    if (!obj || !connection) goto error;

    ov_json_value *val = NULL;

    val = ov_json_object_get(obj, OV_KEY_TYPE);
    if (!val) goto error;

    connection->nettype = ov_json_string_get(val);

    val = ov_json_object_get(obj, OV_KEY_ADDRESS_TYPE);
    if (!val) goto error;

    connection->addrtype = ov_json_string_get(val);

    val = ov_json_object_get(obj, OV_KEY_ADDRESS);
    if (!val) goto error;

    connection->address = ov_json_string_get(val);
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool json_parse_connection_list(void *item, void *data) {

    ov_sdp_connection *conn = NULL;
    if (!ov_json_is_object(item) || !data) return false;

    conn = ov_sdp_connection_create();
    if (!conn) goto error;

    if (!json_parse_connection(item, conn)) goto error;

    if (!ov_node_push(data, conn)) goto error;

    return true;
error:
    conn = ov_sdp_connection_free(conn);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool json_parse_origin(const ov_json_value *obj,
                              ov_sdp_session *session) {

    if (!obj || !session) goto error;

    ov_json_value const *val = NULL;

    val = ov_json_get(obj, "/" OV_KEY_ORIGIN "/" OV_KEY_NAME);
    if (!val) goto error;

    session->origin.name = ov_json_string_get(val);

    val = ov_json_get(obj, "/" OV_KEY_ORIGIN "/" OV_KEY_ID);
    if (!val) goto error;

    session->origin.id = ov_json_number_get(val);

    val = ov_json_get(obj, "/" OV_KEY_ORIGIN "/" OV_KEY_VERSION);
    if (!val) goto error;

    session->origin.version = ov_json_number_get(val);

    val = ov_json_get(obj, "/" OV_KEY_ORIGIN "/" OV_KEY_CONNECTION);
    if (!val || ov_json_is_null(val)) goto error;

    if (!json_parse_connection(val, &session->origin.connection)) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool json_parse_string_list(void *item, void *data) {

    if (!ov_json_is_string(item) || !data) goto error;
    ov_json_value *val = ov_json_value_cast(item);

    const char *string = ov_json_string_get(val);
    if (!string) goto error;

    struct json_parse_list *container = (struct json_parse_list *)data;

    ov_sdp_list *node = ov_sdp_list_create();
    if (!node) goto error;

    node->value = string;
    if (ov_node_push((void **)&container->head, node)) return true;

    node->value = NULL;
    node = ov_sdp_list_free(node);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool json_parse_bandwidth_item(const void *key, void *item, void *data) {

    if (!key) return true;

    ov_dict *dict = ov_dict_cast(data);
    char *k = strdup((char *)key);
    if (!k) goto error;

    uint64_t nbr = (uint64_t)ov_json_number_get(item);
    intptr_t val = (intptr_t)nbr;

    if (ov_dict_set(dict, k, (void *)val, NULL)) return true;

error:
    if (k) free(k);
    return false;
}

/*----------------------------------------------------------------------------*/

static ov_dict *json_parse_bandwidth(ov_json_value const *val) {

    ov_dict *dict = NULL;
    if (!ov_json_is_object(val)) goto error;

    dict = ov_sdp_bandwidth_create();
    if (!dict) goto error;

    if (!ov_json_object_for_each(
            (ov_json_value *)val, dict, json_parse_bandwidth_item))
        goto error;

    return dict;
error:
    ov_dict_free(dict);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool sdp_parse_description_object(void *source, void *data) {

    ov_sdp_description *desc = NULL;
    ov_json_value *val = NULL;

    if (!ov_json_is_object(source) || !data) return false;

    struct json_parse_list container1 = (struct json_parse_list){.head = NULL};

    desc = ov_sdp_description_create();
    if (!desc) goto error;

    val = ov_json_object_get(source, OV_KEY_TYPE);
    if (!ov_json_is_string(val)) goto error;
    desc->media.name = ov_json_string_get(val);

    val = ov_json_object_get(source, OV_KEY_PROTOCOL);
    if (!ov_json_is_string(val)) goto error;
    desc->media.protocol = ov_json_string_get(val);

    val = ov_json_object_get(source, OV_KEY_PORT);
    if (!ov_json_is_number(val)) goto error;
    desc->media.port = ov_json_number_get(val);

    val = ov_json_object_get(source, OV_KEY_PORT_RTCP);
    if (val) {
        if (!ov_json_is_number(val)) goto error;
        desc->media.port2 = ov_json_number_get(val);
    }

    val = ov_json_object_get(source, OV_KEY_FORMAT);
    if (!ov_json_is_array(val) || ov_json_array_is_empty(val)) goto error;

    container1.head = NULL;

    if (!ov_json_array_for_each(val, &container1, json_parse_string_list))
        goto error;

    desc->media.formats = container1.head;
    container1.head = NULL;

    /*
     *      optional items
     */

    val = ov_json_object_get(source, OV_KEY_INFO);
    if (val) {
        if (!ov_json_is_string(val)) goto error;
        desc->info = ov_json_string_get(val);
    }

    val = ov_json_object_get(source, OV_KEY_KEY);
    if (val) {
        if (!ov_json_is_string(val)) goto error;
        desc->key = ov_json_string_get(val);
    }

    val = ov_json_object_get(source, OV_KEY_CONNECTION);

    if (val) {

        if (!ov_json_is_array(val) || ov_json_array_is_empty(val)) goto error;

        if (!ov_json_array_for_each(
                val, (void **)&desc->connection, json_parse_connection_list))
            goto error;
    }

    val = ov_json_object_get(source, OV_KEY_BANDWIDTH);
    if (val) {

        desc->bandwidth = json_parse_bandwidth(val);

        if (!desc->bandwidth) goto error;
    }

    val = ov_json_object_get(source, OV_KEY_ATTRIBUTES);
    if (val) {

        container1.head = NULL;
        container1.key = NULL;

        if (!ov_json_object_for_each(
                val, &container1, sdp_parse_object_to_ov_sdp_list))
            goto error;

        desc->attributes = container1.head;
        container1.head = NULL;
    }

    if (!ov_node_push(data, desc)) goto error;

    return true;

error:
    ov_sdp_description_free(desc);
    return false;
}

/*----------------------------------------------------------------------------*/

static ov_sdp_description *sdp_parse_description(ov_json_value const *val) {

    if (!ov_json_is_array(val)) return NULL;

    ov_sdp_description *desc = NULL;

    if (ov_json_array_for_each(
            (ov_json_value *)val, &desc, sdp_parse_description_object))
        return desc;

    ov_sdp_description_free(desc);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool sdp_parse_session(const char *string,
                              size_t length,
                              ov_sdp_session *input) {

    DefaultSession *session = AS_DEFAULT_SESSION(input);
    OV_ASSERT(session);

    if (!ov_sdp_session_clear(input)) goto error;

    session->buffer = ov_sdp_buffer_create();

    char *ptr = NULL;
    char *nxt = NULL;
    char *val = NULL;
    size_t len = 0;
    uint64_t nbr = 0;
    uint64_t size = 0;

    if (!string || length < 11) goto error;

    /*
     *      We copy the string to the internal buffer and
     *      use only the internal buffer for parsing.
     *
     *      We do now already the content is valid SDP.
     */
    size = session->buffer->length;
    if (!ov_buffer_set(session->buffer, string, length)) {
        ov_log_error("SDP failed to copy the input.");
        goto error;
    }

    ptr = (char *)session->buffer->start;

    /*
     *      Parse the sessions version number
     *      as actual number.
     */

    if (!parse_content_line(
            ptr, session->buffer->length, &val, &len, &ptr, 'v', NULL))
        goto error;

    ptr[-1] = 0;
    ptr[-2] = 0;

    if (!parse_number(val, len, &nbr)) goto error;
    if (nbr > 255) goto error;

    session->public.version = nbr;

    /*
     *      Parse the sessions origin within
     *      the content buffer.
     */

    if (!validate_line(
            ptr,
            session->buffer->length - (ptr - (char *)session->buffer->start),
            &nxt))
        goto error;

    if (!mask_origin(session, ptr, nxt - ptr)) goto error;

    /*
     *      mask the session name
     */

    ptr = nxt;
    ptr[-1] = 0;
    ptr[-2] = 0;

    if (!validate_line(
            ptr,
            session->buffer->length - (ptr - (char *)session->buffer->start),
            &nxt))
        goto error;

    if (!parse_content_line(ptr,
                            session->buffer->length,
                            &val,
                            &len,
                            &ptr,
                            's',
                            ov_sdp_is_text))
        goto error;

    ptr[-1] = 0;
    ptr[-2] = 0;

    session->public.name = val;
    val = (char *)session->public.name + len + 1;
    *val = 0;

    /*
     *      optional items
     */

    if (!validate_line(
            ptr,
            session->buffer->length - (ptr - (char *)session->buffer->start),
            &nxt))
        goto error;

    // OPTIONAL INFO

    if (*ptr == 'i') {

        if (!parse_information(ptr, (nxt - ptr), &val, &len)) goto error;

        session->public.info = val;
        val = (char *)session->public.info + len + 1;
        *val = 0;

        ptr = nxt;
        ptr[-1] = 0;
        ptr[-2] = 0;

        if (!validate_line(ptr,
                           session->buffer->length -
                               (ptr - (char *)session->buffer->start),
                           &nxt))
            goto error;
    }

    // OPTIONAL URI

    if (*ptr == 'u') {

        if (!parse_uri(ptr, (nxt - ptr), &val, &len)) goto error;

        session->public.uri = val;
        val = (char *)session->public.uri + len + 1;
        *val = 0;

        ptr = nxt;
        ptr[-1] = 0;
        ptr[-2] = 0;

        if (!validate_line(ptr,
                           session->buffer->length -
                               (ptr - (char *)session->buffer->start),
                           &nxt))
            goto error;
    }

    // OPTIONAL EMAIL

    if (*ptr == 'e') {

        if (!create_masked_list(&session->public.email,
                                ptr,
                                session->buffer->length -
                                    (ptr - (char *)session->buffer->start),
                                &ptr,
                                'e',
                                ov_sdp_is_email))
            goto error;

        ptr[-1] = 0;
        ptr[-2] = 0;

        if (!validate_line(ptr,
                           session->buffer->length -
                               (ptr - (char *)session->buffer->start),
                           &nxt))
            goto error;
    }

    // OPTIONAL PHONE

    if (*ptr == 'p') {

        if (!create_masked_list(&session->public.phone,
                                ptr,
                                session->buffer->length -
                                    (ptr - (char *)session->buffer->start),
                                &ptr,
                                'p',
                                ov_sdp_is_phone))
            goto error;

        ptr[-1] = 0;
        ptr[-2] = 0;

        if (!validate_line(ptr,
                           session->buffer->length -
                               (ptr - (char *)session->buffer->start),
                           &nxt))
            goto error;
    }

    // OPTIONAL CONNECTION

    if (*ptr == 'c') {

        if (!create_masked_connection(
                &session->public.connection, ptr, (nxt - ptr), true))
            goto error;

        ptr = nxt;
        ptr[-1] = 0;
        ptr[-2] = 0;

        if (!validate_line(ptr,
                           session->buffer->length -
                               (ptr - (char *)session->buffer->start),
                           &nxt))
            goto error;
    }

    // OPTIONAL BANDWIDTH

    if (*ptr == 'b') {

        if (!create_bandwith_dict(&session->public.bandwidth,
                                  ptr,
                                  session->buffer->length -
                                      (ptr - (char *)session->buffer->start),
                                  &ptr))
            goto error;

        ptr[-1] = 0;
        ptr[-2] = 0;

        if (!validate_line(ptr,
                           session->buffer->length -
                               (ptr - (char *)session->buffer->start),
                           &nxt))
            goto error;
    }

    // MANDATORY TIME

    if (!create_masked_time(
            &session->public.time,
            ptr,
            session->buffer->length - (ptr - (char *)session->buffer->start),
            &ptr))
        goto error;

    ptr[-1] = 0;
    ptr[-2] = 0;

    if (!validate_line(ptr, size - (ptr - string), &nxt)) goto done;

    // OPTIONAL KEY

    if (*ptr == 'k') {

        if (!parse_content_line(
                ptr, session->buffer->length, &val, &len, &ptr, 'k', NULL))
            goto error;

        if (!ov_sdp_is_key(val, len)) goto error;

        session->public.key = val;
        val = (char *)session->public.key + len + 1;
        *val = 0;

        ptr[-1] = 0;
        ptr[-2] = 0;
    }

    // OPTIONAL ATTRIBUTES

    if (*ptr == 'a') {

        if (!create_masked_attributes(
                &session->public.attributes,
                ptr,
                session->buffer->length -
                    (ptr - (char *)session->buffer->start),
                &ptr))
            goto error;

        ptr[-1] = 0;
        ptr[-2] = 0;

        if (!validate_line(ptr,
                           session->buffer->length -
                               (ptr - (char *)session->buffer->start),
                           &nxt))
            goto done;
    }

    // OPTIONAL MEDIA DESCRIPTIONS

    if (*ptr == 'm') {

        if (!create_masked_descriptions(
                &session->public.description,
                ptr,
                session->buffer->length -
                    (ptr - (char *)session->buffer->start),
                &ptr))
            goto error;

        ptr[-1] = 0;
        ptr[-2] = 0;
    }

done:
    return true;
error:
    if (session && length > IMPL_DEFAULT_BUFFER_SIZE)
        session->buffer = ov_buffer_free(session->buffer);
    return false;
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      #PUBLIC
 *
 *      ------------------------------------------------------------------------
 */

ov_sdp_session *ov_sdp_session_cast(const void *self) {

    if (!self) goto error;

    if (*(uint16_t *)self == OV_SDP_SESSION_MAGIC_BYTE)
        return (ov_sdp_session *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_sdp_session *ov_sdp_session_create() {

    ov_sdp_session *session = NULL;

    if (ov_sdp_cache.enabled) {

        session = ov_cache_get(ov_sdp_cache.session);
        if (session) OV_ASSERT(AS_DEFAULT_SESSION(session));

        ov_sdp_session_clear(session);
    }

    if (!session) session = calloc(1, sizeof(DefaultSession));

    if (!session) goto error;

    /*
     *  Init session
     */

    session->magic_byte = OV_SDP_SESSION_MAGIC_BYTE;
    session->type = IMPL_TYPE;

    DefaultSession *sess = AS_DEFAULT_SESSION(session);
    OV_ASSERT(sess);

    sess->buffer = ov_sdp_buffer_create();

    return session;
error:
    ov_sdp_session_free(session);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_sdp_session_clear(ov_sdp_session *self) {

    DefaultSession *session = AS_DEFAULT_SESSION(self);
    if (!session) goto error;

    /*
     *      unset the internal buffer
     */

    session->buffer = ov_sdp_buffer_free(session->buffer);

    /*
     *      unset external data
     */

    session->public.time = ov_sdp_time_free(session->public.time);
    session->public.connection =
        ov_sdp_connection_free(session->public.connection);
    session->public.email = ov_sdp_list_free(session->public.email);
    session->public.phone = ov_sdp_list_free(session->public.phone);
    session->public.bandwidth =
        ov_sdp_bandwidth_free(session->public.bandwidth);
    session->public.attributes = ov_sdp_list_free(session->public.attributes);
    session->public.description =
        ov_sdp_description_free(session->public.description);

    /*
     *      reinit session
     */

    *session = (DefaultSession){0};
    session->public.magic_byte = OV_SDP_SESSION_MAGIC_BYTE;
    session->public.type = IMPL_TYPE;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

void *ov_sdp_session_free(void *self) {

    ov_sdp_session *session = ov_sdp_session_cast(self);
    if (!session) goto error;

    if (!ov_sdp_session_clear(session)) return session;

    if (ov_sdp_cache.enabled)
        session = ov_cache_put(ov_sdp_cache.session, session);

    return ov_data_pointer_free(session);

error:
    return self;
}

/*----------------------------------------------------------------------------*/

bool ov_sdp_session_persist(ov_sdp_session *session) {

    bool result = false;
    char *string = NULL;

    if (!session) goto error;

    string = ov_sdp_stringify(session, false);
    if (!string) goto error;

    result = sdp_parse_session(string, strlen(string), session);

error:
    ov_data_pointer_free(string);
    return result;
}

/*----------------------------------------------------------------------------*/

ov_sdp_session *ov_sdp_session_copy(const ov_sdp_session *session) {

    ov_sdp_session *copy = NULL;
    char *string = NULL;

    if (!session) goto error;

    string = ov_sdp_stringify(session, false);
    if (!string) goto error;

    copy = ov_sdp_session_create();
    if (!copy) goto error;

    if (!sdp_parse_session(string, strlen(string), copy)) goto error;

    string = ov_data_pointer_free(string);
    return copy;

error:
    ov_data_pointer_free(string);
    ov_sdp_session_free(copy);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_sdp_validate(const char *start, size_t size) {

    if (!start || size < 32) goto error;

    char *ptr = (char *)start;
    char *next = NULL;

    /*
     *      Parse the session head,
     *      which MUST start with "v=0\r\n"
     */

    if (!validate_line(ptr, size - (ptr - start), &next)) goto error;
    if (!validate_version(ptr, (next - ptr))) goto error;

    ptr = next;
    if (!validate_line(ptr, size - (ptr - start), &next)) goto error;
    if (!validate_origin(ptr, (next - ptr))) goto error;

    ptr = next;
    if (!validate_line(ptr, size - (ptr - start), &next)) goto error;
    if (!validate_session_name(ptr, (next - ptr))) goto error;

    ptr = next;
    if (!validate_line(ptr, size - (ptr - start), &next)) goto error;

    // OPTIONAL INFO

    if (*ptr == 'i') {

        if (!validate_information(ptr, (next - ptr))) goto error;

        ptr = next;
        if (!validate_line(ptr, size - (ptr - start), &next)) goto error;
    }

    // OPTIONAL URI

    if (*ptr == 'u') {

        if (!validate_uri(ptr, (next - ptr))) goto error;

        ptr = next;
        if (!validate_line(ptr, size - (ptr - start), &next)) goto error;
    }

    // OPTIONAL EMAIL

    if (*ptr == 'e') {

        if (!validate_content_list(ptr, size, &ptr, 'e', ov_sdp_is_email))
            goto error;

        if (!validate_line(ptr, size - (ptr - start), &next)) goto error;
    }

    // OPTIONAL PHONE

    if (*ptr == 'p') {

        if (!validate_content_list(ptr, size, &ptr, 'p', ov_sdp_is_phone))
            goto error;

        if (!validate_line(ptr, size - (ptr - start), &next)) goto error;
    }

    // OPTIONAL CONNECTION

    if (*ptr == 'c') {

        if (!validate_connection(ptr, (next - ptr), true, true)) goto error;

        ptr = next;
        if (!validate_line(ptr, size - (ptr - start), &next)) goto error;
    }

    // OPTIONAL BANDWIDTH

    if (*ptr == 'b') {

        if (!validate_content_list(ptr, size, &ptr, 'b', ov_sdp_is_bandwidth))
            goto error;

        if (!validate_line(ptr, size - (ptr - start), &next)) goto error;
    }

    // MANDATORY TIME

    if (!validate_time(ptr, size, &ptr)) goto error;

    if (!validate_line(ptr, size - (ptr - start), &next)) goto done;

    // OPTIONAL KEY

    if (*ptr == 'k') {

        if (!validate_key(ptr, (next - ptr))) goto error;

        ptr = next;
        if (!validate_line(ptr, size - (ptr - start), &next)) goto done;
    }

    // OPTIONAL ATTRIBUTES

    if (*ptr == 'a') {

        if (!validate_attributes(ptr, size - (start - ptr), &next)) goto error;

        ptr = next;
        if (!validate_line(ptr, size - (ptr - start), &next)) goto done;
    }

    // OPTIONAL MEDIA DESCRIPTIONS

    if (*ptr == 'm') {

        if (!validate_media_descriptions(ptr, size - (start - ptr), &next))
            goto error;
    }

done:
    if ((ssize_t)size != next - start) {
        ov_log_debug("unparsed |%s|", next);
        goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_sdp_session *ov_sdp_parse(const char *string, size_t length) {

    ov_sdp_session *session = NULL;

    if (!string || length < 11) goto error;

    session = ov_sdp_session_create();
    DefaultSession *d = AS_DEFAULT_SESSION(session);

    if (!d) {
        ov_log_error("SDP failed to create SDP session.");
        goto error;
    }

    if (!sdp_parse_session(string, length, session)) goto error;

    return session;

error:
    session = ov_sdp_session_free(session);
    return NULL;
}

/*----------------------------------------------------------------------------*/

char *ov_sdp_stringify(const ov_sdp_session *session, bool escaped) {

    size_t size = IMPL_DEFAULT_BUFFER_SIZE;
    char *string = NULL;
    char *ptr = NULL;

    if (!session) goto error;

    string = calloc(size, sizeof(char));
    if (!string) goto error;

    /*
     *      Verify minimum required valid input
     */

    if (!validate_vos(session)) goto error;

    if (!validate_time_write(session)) goto error;

    size_t counter = 0;

    while (true) {

        ptr = string;

        if (!write_vos(session, ptr, size - (ptr - string), escaped, &ptr))
            goto reallocate;

        if (session->info) {

            if (!ov_sdp_is_text(session->info, strlen(session->info))) {
                ov_log_error(
                    "SDP session info %s"
                    "- NOT an SDP text",
                    session->info);
                goto error;
            }

            if (!write_line(ptr,
                            size - (ptr - string),
                            escaped,
                            &ptr,
                            'i',
                            session->info))
                goto reallocate;
        }

        if (session->uri) {

            // TBD add uri check

            if (!write_line(ptr,
                            size - (ptr - string),
                            escaped,
                            &ptr,
                            'u',
                            session->uri))
                goto reallocate;
        }

        if (session->email) {

            if (!validate_pointer_list(
                    session->email, "email", ov_sdp_is_email))
                goto error;

            if (!write_node_lines(ptr,
                                  size - (ptr - string),
                                  escaped,
                                  &ptr,
                                  'e',
                                  session->email))
                goto reallocate;
        }

        if (session->phone) {

            if (!validate_pointer_list(
                    session->phone, "phone", ov_sdp_is_phone))
                goto error;

            if (!write_node_lines(ptr,
                                  size - (ptr - string),
                                  escaped,
                                  &ptr,
                                  'p',
                                  session->phone))
                goto reallocate;
        }

        if (session->connection) {

            if (1 != ov_node_count(session->connection)) {

                ov_log_error(
                    "SDP session connection "
                    "MUST be one if set");
                goto error;
            }

            if (!validate_connection_pointers(session->connection)) goto error;

            if (!write_connections(session->connection,
                                   ptr,
                                   size - (ptr - string),
                                   escaped,
                                   &ptr))
                goto reallocate;
        }

        if (session->bandwidth) {

            if (!validate_bandwith(session->bandwidth)) goto error;

            if (!write_bandwith(session->bandwidth,
                                ptr,
                                size - (ptr - string),
                                escaped,
                                &ptr))
                goto reallocate;
        }

        if (!write_time(session, ptr, size - (ptr - string), escaped, &ptr))
            goto reallocate;

        if (session->key) {

            if (!ov_sdp_is_key(session->key, strlen(session->key))) {
                ov_log_error(
                    "SDP session key %s"
                    "- NOT an SDP key",
                    session->key);
                goto error;
            }

            if (!write_line(ptr,
                            size - (ptr - string),
                            escaped,
                            &ptr,
                            'k',
                            session->key))
                goto reallocate;
        }

        if (session->attributes) {

            if (!validate_attributes_write(session->attributes)) goto error;

            if (!write_attributes(session->attributes,
                                  ptr,
                                  size - (ptr - string),
                                  escaped,
                                  &ptr))
                goto reallocate;
        }

        if (session->description) {

            if (!validate_description_write(session->description)) goto error;

            if (!write_description(session->description,
                                   ptr,
                                   size - (ptr - string),
                                   escaped,
                                   &ptr))
                goto reallocate;
        }

        break;
    reallocate:
        counter++;
        if (100 == counter) goto error;

        string = ov_data_pointer_free(string);
        size = size + IMPL_DEFAULT_BUFFER_SIZE;
        string = calloc(size, sizeof(char));
        if (!string) goto error;
    }

    return string;
error:
    string = ov_data_pointer_free(string);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_sdp_to_json(const ov_sdp_session *session) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!session) goto error;

    out = ov_json_object();
    if (!out) goto error;

    /*
     *      set mandatory items, even if null
     */

    val = ov_json_number(session->version);
    if (!ov_json_object_set(out, OV_KEY_VERSION, val)) goto error;

    val = ov_json_string(session->name);
    if (!ov_json_object_set(out, OV_KEY_NAME, val)) goto error;

    val = origin_create(session);
    if (!ov_json_object_set(out, OV_KEY_ORIGIN, val)) goto error;

    val = time_create(session->time);
    if (!ov_json_object_set(out, OV_KEY_TIME, val)) goto error;

    /*
     *      optional items
     */

    if (session->info) {

        val = ov_json_string(session->info);
        if (!ov_json_object_set(out, OV_KEY_INFO, val)) goto error;
    }

    if (session->uri) {

        val = ov_json_string(session->uri);
        if (!ov_json_object_set(out, OV_KEY_URI, val)) goto error;
    }

    if (session->key) {

        val = ov_json_string(session->key);
        if (!ov_json_object_set(out, OV_KEY_KEY, val)) goto error;
    }

    if (session->connection) {

        val = connection_create(session->connection);
        if (!ov_json_object_set(out, OV_KEY_CONNECTION, val)) goto error;
    }

    if (session->email) {

        val = list_array_create(session->email);
        if (!ov_json_object_set(out, OV_KEY_EMAIL, val)) goto error;
    }

    if (session->phone) {

        val = list_array_create(session->phone);
        if (!ov_json_object_set(out, OV_KEY_PHONE, val)) goto error;
    }

    if (session->bandwidth) {

        val = bandwidth_create(session->bandwidth);
        if (!ov_json_object_set(out, OV_KEY_BANDWIDTH, val)) goto error;
    }

    if (session->attributes) {

        val = attributes_create(session->attributes);
        if (!ov_json_object_set(out, OV_KEY_ATTRIBUTES, val)) goto error;
    }

    if (session->description) {

        val = description_create(session->description);
        if (!ov_json_object_set(out, OV_KEY_DESCRIPTION, val)) goto error;
    }

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_sdp_session *ov_sdp_from_json(const ov_json_value *obj) {

    ov_sdp_session *session = NULL;
    char *valid = NULL;

    struct json_parse_list container = (struct json_parse_list){0};

    /*
     *      mandatory items
     */

    if (!ov_json_is_object(obj)) goto error;

    session = ov_sdp_session_create();

    ov_json_value const *val = NULL;

    val = ov_json_get(obj, "/" OV_KEY_VERSION);
    if (val) session->version = ov_json_number_get(val);

    val = ov_json_get(obj, "/" OV_KEY_NAME);
    if (!val) goto error;

    session->name = ov_json_string_get(val);

    if (!json_parse_origin(obj, session)) goto error;

    val = ov_json_get(obj, "/" OV_KEY_TIME);
    if (!ov_json_is_array(val)) goto error;

    struct json_parse_time_container container1 =
        (struct json_parse_time_container){

            .value = val, .head = NULL};

    if (!ov_json_array_for_each(
            (ov_json_value *)val, &container1, json_parse_time))
        goto error;

    session->time = container1.head;
    container1.head = NULL;

    /*
     *      optional items
     */

    val = ov_json_get(obj, "/" OV_KEY_INFO);
    if (val) session->info = ov_json_string_get(val);

    val = ov_json_get(obj, "/" OV_KEY_URI);
    if (val) session->uri = ov_json_string_get(val);

    val = ov_json_get(obj, "/" OV_KEY_KEY);
    if (val) session->key = ov_json_string_get(val);

    val = ov_json_get(obj, "/" OV_KEY_CONNECTION);
    if (val) {

        session->connection = ov_sdp_connection_create();
        if (!session->connection) goto error;

        if (!json_parse_connection(val, session->connection)) goto error;
    }

    val = ov_json_get(obj, "/" OV_KEY_EMAIL);
    if (val) {

        container.head = NULL;

        if (!ov_json_array_for_each(
                (ov_json_value *)val, &container, json_parse_string_list))
            goto error;

        session->email = container.head;
        container.head = NULL;
    }

    val = ov_json_get(obj, "/" OV_KEY_PHONE);
    if (val) {

        container.head = NULL;

        if (!ov_json_array_for_each(
                (ov_json_value *)val, &container, json_parse_string_list))
            goto error;

        session->phone = container.head;
        container.head = NULL;
    }

    val = ov_json_get(obj, "/" OV_KEY_BANDWIDTH);
    if (val) {

        session->bandwidth = json_parse_bandwidth(val);
        if (!session->bandwidth) goto error;
    }

    val = ov_json_get(obj, "/" OV_KEY_ATTRIBUTES);
    if (val) {

        container.head = NULL;
        container.key = NULL;

        if (!ov_json_object_for_each((ov_json_value *)val,
                                     &container,
                                     sdp_parse_object_to_ov_sdp_list))
            goto error;

        session->attributes = container.head;
        container.head = NULL;
    }

    val = ov_json_get(obj, "/" OV_KEY_DESCRIPTION);
    if (val) {

        session->description = sdp_parse_description(val);
        if (!session->description) goto error;
    }

    /*
     *      We use a volatile functionality to create the
     *      session out of JSON.
     *
     *      To validate the session and persist the values
     *      of the session we stringify and reparse.
     */

    valid = ov_sdp_stringify(session, false);
    if (!valid) goto error;

    size_t valid_length = strlen(valid);
    session = ov_sdp_session_free(session);
    session = ov_sdp_parse(valid, valid_length);
    valid = ov_data_pointer_free(valid);

    if (session) return session;
error:
    valid = ov_data_pointer_free(valid);
    session = ov_sdp_session_free(session);
    return NULL;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      CACHING
 *
 *      ------------------------------------------------------------------------
 */

void ov_sdp_enable_caching(size_t capacity) {

    if (NULL == ov_sdp_cache.buffer)
        ov_sdp_cache.buffer = ov_cache_extend(NULL, capacity);

    if (NULL == ov_sdp_cache.session)
        ov_sdp_cache.session = ov_cache_extend(NULL, capacity);

    if (NULL == ov_sdp_cache.description)
        ov_sdp_cache.description = ov_cache_extend(NULL, capacity);

    if (NULL == ov_sdp_cache.bandwidth)
        ov_sdp_cache.bandwidth = ov_cache_extend(NULL, capacity);

    if (NULL == ov_sdp_cache.list)
        ov_sdp_cache.list = ov_cache_extend(NULL, capacity);

    if (NULL == ov_sdp_cache.time)
        ov_sdp_cache.time = ov_cache_extend(NULL, capacity);

    if (NULL == ov_sdp_cache.connection)
        ov_sdp_cache.connection = ov_cache_extend(NULL, capacity);

    ov_sdp_cache.enabled = true;

    return;
}

/*----------------------------------------------------------------------------*/

void ov_sdp_disable_caching() {

    ov_sdp_cache.enabled = false;

    ov_sdp_cache.buffer = ov_cache_free(ov_sdp_cache.buffer, 0, ov_buffer_free);
    ov_sdp_cache.session =
        ov_cache_free(ov_sdp_cache.session, 0, ov_data_pointer_free);
    ov_sdp_cache.description =
        ov_cache_free(ov_sdp_cache.description, 0, ov_data_pointer_free);
    ov_sdp_cache.bandwidth =
        ov_cache_free(ov_sdp_cache.bandwidth, 0, ov_dict_free);
    ov_sdp_cache.list =
        ov_cache_free(ov_sdp_cache.list, 0, ov_data_pointer_free);
    ov_sdp_cache.time =
        ov_cache_free(ov_sdp_cache.time, 0, ov_data_pointer_free);
    ov_sdp_cache.connection =
        ov_cache_free(ov_sdp_cache.connection, 0, ov_data_pointer_free);

    return;
}

/*----------------------------------------------------------------------------*/

ov_buffer *ov_sdp_buffer_create() {

    ov_buffer *buffer = ov_cache_get(ov_sdp_cache.buffer);
    if (!buffer) buffer = ov_buffer_create(IMPL_DEFAULT_BUFFER_SIZE);

    return buffer;
}

/*----------------------------------------------------------------------------*/

ov_sdp_description *ov_sdp_description_create() {

    ov_sdp_description *description = ov_cache_get(ov_sdp_cache.description);
    if (!description) description = calloc(1, sizeof(ov_sdp_description));

    description->node.type = OV_SDP_DESCRIPTION_TYPE;
    description->node.prev = NULL;
    description->node.next = NULL;

    return description;
}

/*----------------------------------------------------------------------------*/

ov_dict *ov_sdp_bandwidth_create() {

    ov_dict_config config = ov_dict_string_key_config(255);

    ov_dict *bandwidth = ov_cache_get(ov_sdp_cache.bandwidth);
    if (!bandwidth) bandwidth = ov_dict_create(config);

    return bandwidth;
}

/*----------------------------------------------------------------------------*/

ov_sdp_list *ov_sdp_list_create() {

    ov_sdp_list *list = ov_cache_get(ov_sdp_cache.list);
    if (!list) list = calloc(1, sizeof(ov_sdp_list));

    list->node.type = OV_SDP_LIST_TYPE;
    list->node.prev = NULL;
    list->node.next = NULL;

    return list;
}

/*----------------------------------------------------------------------------*/

ov_sdp_time *ov_sdp_time_create() {

    ov_sdp_time *t = ov_cache_get(ov_sdp_cache.time);
    if (!t) t = calloc(1, sizeof(ov_sdp_time));

    t->node.type = OV_SDP_TIME_TYPE;
    t->node.prev = NULL;
    t->node.next = NULL;

    return t;
}

/*----------------------------------------------------------------------------*/

ov_sdp_connection *ov_sdp_connection_create() {

    ov_sdp_connection *c = ov_cache_get(ov_sdp_cache.connection);
    if (!c) c = calloc(1, sizeof(ov_sdp_connection));

    c->node.type = OV_SDP_CONNECTION_TYPE;
    c->node.prev = NULL;
    c->node.next = NULL;
    return c;
}

/*----------------------------------------------------------------------------*/

ov_buffer *ov_sdp_buffer_free(ov_buffer *buffer) {

    if (!buffer) return NULL;

    ov_buffer_clear(buffer);

    if (ov_sdp_cache.enabled)
        buffer = ov_cache_put(ov_sdp_cache.buffer, buffer);

    buffer = ov_buffer_free(buffer);
    return buffer;
}

/*----------------------------------------------------------------------------*/

ov_sdp_description *ov_sdp_description_free(ov_sdp_description *description) {

    if (!description) return NULL;

    void *head = description;

    ov_sdp_description *item = (ov_sdp_description *)ov_node_pop(&head);

    while (item) {

        item->media.name = NULL;
        item->media.protocol = NULL;
        item->media.port = 0;
        item->media.port2 = 0;
        item->media.formats = ov_sdp_list_free(item->media.formats);

        item->info = NULL;
        item->key = NULL;

        item->connection = ov_sdp_connection_free(item->connection);
        item->bandwidth = ov_sdp_bandwidth_free(item->bandwidth);
        item->attributes = ov_sdp_list_free(item->attributes);

        if (ov_sdp_cache.enabled)
            item = ov_cache_put(ov_sdp_cache.description, item);

        item = ov_data_pointer_free(item);
        item = (ov_sdp_description *)ov_node_pop(&head);
    }

    return ov_data_pointer_free(head);
}

/*----------------------------------------------------------------------------*/

ov_dict *ov_sdp_bandwidth_free(ov_dict *dict) {

    if (!dict) return NULL;

    ov_dict_clear(dict);

    if (ov_sdp_cache.enabled) dict = ov_cache_put(ov_sdp_cache.bandwidth, dict);

    return ov_dict_free(dict);
}

/*----------------------------------------------------------------------------*/

ov_sdp_list *ov_sdp_list_free(ov_sdp_list *list) {

    if (!list) return NULL;

    void *head = list;

    ov_sdp_list *item = (ov_sdp_list *)ov_node_pop(&head);

    while (item) {

        item->key = NULL;
        item->value = NULL;

        if (ov_sdp_cache.enabled) item = ov_cache_put(ov_sdp_cache.list, item);

        item = ov_data_pointer_free(item);
        item = (ov_sdp_list *)ov_node_pop(&head);
    }

    return ov_data_pointer_free(head);
}

/*----------------------------------------------------------------------------*/

ov_sdp_time *ov_sdp_time_free(ov_sdp_time *tme) {

    if (!tme) return NULL;

    void *head = tme;

    ov_sdp_time *item = (ov_sdp_time *)ov_node_pop(&head);

    while (item) {

        item->start = 0;
        item->stop = 0;

        item->repeat = ov_sdp_list_free(item->repeat);
        item->zone = ov_sdp_list_free(item->zone);

        if (ov_sdp_cache.enabled) item = ov_cache_put(ov_sdp_cache.time, item);

        item = ov_data_pointer_free(item);
        item = (ov_sdp_time *)ov_node_pop(&head);
    }

    return ov_data_pointer_free(head);
}

/*----------------------------------------------------------------------------*/

ov_sdp_connection *ov_sdp_connection_free(ov_sdp_connection *connection) {

    if (!connection) return NULL;

    void *head = connection;

    ov_sdp_connection *item = (ov_sdp_connection *)ov_node_pop(&head);

    while (item) {

        item->nettype = NULL;
        item->addrtype = NULL;
        item->address = NULL;

        if (ov_sdp_cache.enabled)
            item = ov_cache_put(ov_sdp_cache.connection, item);

        item = ov_data_pointer_free(item);
        item = (ov_sdp_connection *)ov_node_pop(&head);
    }

    return ov_data_pointer_free(head);
}

/*----------------------------------------------------------------------------*/

ov_sdp_session *ov_sdp_session_from_json(const ov_json_value *value) {

    char *sdp_string = NULL;
    size_t sdp_string_len = 0;

    ov_sdp_session *session = NULL;

    if (!value) goto error;

    const ov_json_value *src = ov_json_object_get(value, OV_KEY_SDP);
    if (!src) src = value;

    const char *src_string = ov_json_string_get(src);
    if (!src_string) goto error;

    if (!ov_string_replace_all(&sdp_string,
                               &sdp_string_len,
                               src_string,
                               strlen(src_string),
                               "\\r\\n",
                               4,
                               "\r\n",
                               2,
                               true))
        goto error;

    session = ov_sdp_parse(sdp_string, sdp_string_len);
    if (!session) goto error;

    sdp_string = ov_data_pointer_free(sdp_string);
    return session;

error:
    sdp_string = ov_data_pointer_free(sdp_string);
    session = ov_sdp_session_free(session);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_sdp_session_to_json(const ov_sdp_session *session) {

    ov_json_value *out = NULL;

    if (!session) goto error;

    char *string = ov_sdp_stringify(session, true);
    if (!string) goto error;

    out = ov_json_string(string);
    string = ov_data_pointer_free(string);
    return out;
error:
    ov_json_value_free(out);
    return NULL;
}
