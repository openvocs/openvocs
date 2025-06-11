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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "../include/ov_sip_serde.h"
#include <ov_base/ov_string.h>
#include <stdarg.h>
#include <unistd.h>

/*----------------------------------------------------------------------------*/

#define PANIC(x)                                                               \
    do {                                                                       \
        ov_log_error(x);                                                       \
        abort();                                                               \
    } while (0)

#define RESULT_ERROR(res, msg) ov_result_set(res, OV_ERROR_INTERNAL_SERVER, msg)

/*----------------------------------------------------------------------------*/

// RFC 2822 - including terminal CRLF
#define MAX_LINE_LENGTH 1000

#define SP ' '
#define CR '\r'
#define LF '\n'
#define CRLF "\r\n"
#define SIP_VERSION "SIP/2.0"

/*****************************************************************************
                                    HELPERS
 ****************************************************************************/

static char *set_pointer_to(char *ptr, char value) {

    if (0 != ptr) {
        *ptr = value;
    }

    return ptr;
}

/*----------------------------------------------------------------------------*/

static char *propagate_by_one(char *ptr, char const *utmost_ptr_value) {

    if (0 == ptr) {
        return 0;
    } else if (ptr + 1 > utmost_ptr_value) {
        return 0;
    } else {
        return 1 + ptr;
    }
}

/*----------------------------------------------------------------------------*/

static char *find_char(char *str, char c) {

    if (0 == str) {
        return str;
    } else {
        return strchr(str, c);
    }
}

/*----------------------------------------------------------------------------*/

static ov_sip_message *generate_message(char const *el,
                                        char const *el2,
                                        char const *el3,
                                        ov_result *res) {

    if ((0 == el) || (0 == el2) || (0 == el3)) {
        RESULT_ERROR(res, "Null pointer encountered");
        return 0;
    } else if (0 == ov_string_compare(el, SIP_VERSION)) {
        bool ok = true;
        uint16_t code = ov_string_to_uint16(el2, &ok);
        if (!ok) {
            RESULT_ERROR(res, "SIP response code not numerical");
            return 0;
        } else {
            return ov_sip_message_response_create(code, el3);
        }
    } else {
        if (0 != ov_string_compare(SIP_VERSION, el3)) {
            RESULT_ERROR(res, "Invalid SIP request: No/invalid SIP version");
            return 0;
        } else {
            return ov_sip_message_request_create(el, el2);
        }
    }
}

/*****************************************************************************
                                    SipSerde
 ****************************************************************************/

typedef enum {
    P_OK,
    P_ERROR,
    P_CONTINUE,
    P_OK_MORE_ELEMENTS
} ElementParseState;

typedef enum {

    REQUEST,
    RESPONSE,

} MessageType;

typedef enum {

    M_START_LINE,
    M_HEADERS,
    M_BODY,
    M_DONE

} MessageParseState;

typedef struct {

    ov_serde public;

    MessageParseState pstate;

    struct line_buffer {
        char buffer[MAX_LINE_LENGTH + 1];
        size_t pos;
    } line_buffer;

    ov_buffer *residuum;

    ov_sip_message *message;

} SipSerde;

/*****************************************************************************
                                  LINE BUFFER
 ****************************************************************************/

static void lb_reset(struct line_buffer *lbuf) {

    if (0 != lbuf) {
        lbuf->pos = 0;
        lbuf->buffer[0] = 0;
    }
}

/*----------------------------------------------------------------------------*/

static ElementParseState lb_copy_line(struct line_buffer *lbuf,
                                      ov_buffer *buf,
                                      ov_result *res) {

    if ((0 == lbuf) || (0 == buf) || (0 == buf->start)) {
        RESULT_ERROR(res, "0 Pointer");
        return P_ERROR;

    } else if (0 == buf->length) {

        return P_CONTINUE;

    } else {

        char last_char = 0;

        if (lbuf->pos > 0) {
            last_char = lbuf->buffer[lbuf->pos - 1];
        }

        while (true) {

            if ((LF == buf->start[0]) && (CR == last_char)) {

                ++buf->start;
                --buf->length;

                OV_ASSERT(lbuf->pos > 0);
                lbuf->buffer[lbuf->pos - 1] = 0;
                return P_OK;

            } else {

                last_char = buf->start[0];
                lbuf->buffer[lbuf->pos] = last_char;

                ++lbuf->pos;
                --buf->length;
                ++buf->start;

                if (lbuf->pos > MAX_LINE_LENGTH) {
                    RESULT_ERROR(res, "Line too long");
                    lbuf->buffer[lbuf->pos] = 0;
                    return P_ERROR;
                } else if (0 == buf->length) {
                    lbuf->buffer[lbuf->pos] = 0;
                    return P_CONTINUE;
                }
            }
        }
    }
}

/*----------------------------------------------------------------------------*/

static char *lb_get_line(struct line_buffer *lbuf) {

    if (0 == lbuf) {
        return 0;
    } else {
        lbuf->buffer[lbuf->pos] = 0;
        return lbuf->buffer;
    }
}

/*****************************************************************************
                                     SERDE
 ****************************************************************************/

static SipSerde *as_sip_serde(ov_serde *ss, ov_result *res) {

    if (0 == ss) {
        ov_result_set(
            res, OV_ERROR_INTERNAL_SERVER, "No SIP Serde (0 pointer)");
        return 0;
    } else if (OV_SIP_SERDE_TYPE != ss->magic_bytes) {
        ov_result_set(
            res, OV_ERROR_INTERNAL_SERVER, "Wrong Serde (Not a SIP Serde)");
        return 0;
    } else {
        return (SipSerde *)ss;
    }
}

/*----------------------------------------------------------------------------*/

static bool reset_serde(ov_serde *self, ov_result *res) {

    SipSerde *sip = as_sip_serde(self, res);

    if (0 == sip) {
        return false;
    } else if (M_DONE == sip->pstate) {
        ov_log_info("Resetting SIP serde%s",
                    (0 == sip->message) ? "" : " SIP Message lost");
        sip->message = ov_sip_message_free(sip->message);
        sip->pstate = M_START_LINE;
        return true;
    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static ov_sip_message *get_message(ov_serde *serde, ov_result *res) {

    SipSerde *sip = as_sip_serde(serde, res);

    if (0 == sip) {
        return 0;
    } else {
        return sip->message;
    }
}

/*----------------------------------------------------------------------------*/

static uint32_t get_content_length_from_headers(ov_sip_message const *msg) {

    bool ok = 0;
    return ov_string_to_uint32(
        ov_sip_message_header(msg, OV_SIP_HEADER_CONTENT_LENGTH), &ok);
}

/*----------------------------------------------------------------------------*/

static char const *get_content_type_from_headers(ov_sip_message const *msg) {
    return ov_sip_message_header(msg, OV_SIP_HEADER_CONTENT_TYPE);
}

/*----------------------------------------------------------------------------*/

static bool is_data_valid(ov_serde_data data, ov_result *res) {

    if (0 == data.data) {
        ov_result_set(res, OV_ERROR_INTERNAL_SERVER, "No data given");
        return false;
    } else if (OV_SIP_SERDE_TYPE != data.data_type) {
        ov_result_set(res, OV_ERROR_INTERNAL_SERVER, "Data not a SIP message");
        return false;
    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool is_buf_valid(ov_buffer const *buf, ov_result *res) {
    if (0 == buf) {
        ov_result_set(res, OV_ERROR_INTERNAL_SERVER, "No raw data given");
        return false;
    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool buffer_empty(ov_buffer const *buf) {

    if ((0 == buf) || (0 == buf->start) || (0 == buf->length)) {
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool is_ptr_valid(void const *ptr, ov_result *res) {
    if (0 == ptr) {
        RESULT_ERROR(res, "Invalid pointer (0 pointer)");
        return false;
    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static void reset_sip_serde(SipSerde *self) {

    if (0 != self) {
        self->message = ov_sip_message_free(self->message);
        self->pstate = M_START_LINE;
        lb_reset(&self->line_buffer);
        self->residuum = ov_buffer_free(self->residuum);
    }
}

/*----------------------------------------------------------------------------*/

static ov_serde *impl_free(ov_serde *self) {

    SipSerde *sip = as_sip_serde(self, 0);

    if (0 != sip) {
        reset_sip_serde(sip);
        ov_buffer_free(sip->residuum);
        return ov_free(sip);
    } else {
        return self;
    }
}

/*****************************************************************************
                                    PARSING
 ****************************************************************************/

static ElementParseState message_from_line(SipSerde *self,
                                           char *line,
                                           char const *end_ptr,
                                           ov_result *res) {

    if (0 == line[0]) {
        return P_CONTINUE;
    } else {

        char *el2 = find_char(line, SP);
        set_pointer_to(el2, 0);
        el2 = propagate_by_one(el2, end_ptr);

        char *el3 = find_char(el2, SP);
        set_pointer_to(el3, 0);
        el3 = propagate_by_one(el3, end_ptr);

        self->message = generate_message(line, el2, el3, res);

        if (0 == self->message) {
            RESULT_ERROR(res, "Malformed SIP message: Start line corrupt");
            return P_ERROR;
        } else {
            return P_OK;
        }
    }
}

/*----------------------------------------------------------------------------*/

static ElementParseState parse_start_line(SipSerde *self,
                                          ov_buffer *buf,
                                          ov_result *res) {

    OV_ASSERT(self);

    struct line_buffer *lb = &self->line_buffer;

    ElementParseState state = lb_copy_line(lb, buf, res);

    if (P_OK != state) {
        return state;
    } else {

        char *line = lb_get_line(lb);
        OV_ASSERT(0 != line);

        ElementParseState state =
            message_from_line(self, line, lb->buffer + MAX_LINE_LENGTH, res);

        lb_reset(lb);

        return state;
    }
}

/*----------------------------------------------------------------------------*/

static ElementParseState add_header_to_message(ov_sip_message *msg,
                                               char const *header,
                                               char const *value,
                                               ov_result *res) {
    if (0 == msg) {

        RESULT_ERROR(res,
                     "Malformed SIP message: Header before end of "
                     "start-line");
        return P_ERROR;

    } else if (!ov_sip_message_header_set(msg, header, value)) {

        ov_log_error("Malformed SIP header: %s:%s",
                     ov_string_sanitize(header),
                     ov_string_sanitize(value));
        RESULT_ERROR(res, "Malformed SIP header");
        return P_ERROR;

    } else {

        return P_OK;
    }
}

/*----------------------------------------------------------------------------*/

static ElementParseState add_header_to_sip_serde(SipSerde *self,
                                                 char *header,
                                                 char *value,
                                                 ov_result *res) {
    char const WHITESPACE_CHARS[] = " ";

    char *trimmed_header = (char *)ov_string_ltrim(
        ov_string_rtrim(header, WHITESPACE_CHARS), WHITESPACE_CHARS);

    char *trimmed_value = ov_string_rtrim(
        (char *)ov_string_ltrim(value, WHITESPACE_CHARS), WHITESPACE_CHARS);

    ElementParseState state = add_header_to_message(
        get_message(&self->public, res), trimmed_header, trimmed_value, res);

    return state;
}

/*----------------------------------------------------------------------------*/

static ElementParseState parse_header(SipSerde *self,
                                      char *line,
                                      size_t line_length,
                                      ov_result *res) {
    char WHITESPACE_CHARS[] = " ";

    char *value = find_char(line, ':');
    set_pointer_to(value, 0);
    value = (char *)ov_string_ltrim(
        propagate_by_one(value, line + line_length), WHITESPACE_CHARS);

    ov_string_rtrim(line, WHITESPACE_CHARS);

    ElementParseState state = add_header_to_sip_serde(self, line, value, res);

    return state;
}

/*----------------------------------------------------------------------------*/

static ElementParseState parse_headers(SipSerde *self,
                                       ov_buffer *buf,
                                       ov_result *res) {
    OV_ASSERT(self);

    struct line_buffer *lb = &self->line_buffer;

    ElementParseState state = lb_copy_line(lb, buf, res);

    if (P_OK != state) {
        return state;
    } else {

        char *line = lb_get_line(lb);

        size_t line_length = strnlen(line, MAX_LINE_LENGTH);

        if (0 == line_length) {

            return P_OK;

        } else {

            ElementParseState state =
                parse_header(self, line, line_length, res);

            lb_reset(lb);

            if (P_OK == state) {
                return P_OK_MORE_ELEMENTS;
            } else {
                return P_ERROR;
            }
        }
    }
}

/*----------------------------------------------------------------------------*/

static ElementParseState set_message_body(ov_sip_message *msg,
                                          ov_buffer *buf,
                                          size_t len,
                                          char const *type,
                                          ov_result *res) {

    if ((!is_ptr_valid(msg, res)) || (!is_buf_valid(buf, res)) ||
        (!is_ptr_valid(type, res))) {
        return P_ERROR;
    } else if (buf->length < len) {
        return P_CONTINUE;
    } else {
        ov_buffer *body = 0;

        size_t old_len = buf->length;
        buf->length = len;

        ov_buffer_copy((void **)&body, buf);

        buf->start += len;
        buf->length = old_len - len;

        ov_sip_message_body_set(msg, body, type);
        return P_OK;
    }
}

/*----------------------------------------------------------------------------*/

static ElementParseState parse_body(SipSerde *self,
                                    ov_buffer *buf,
                                    ov_result *res) {
    OV_ASSERT(self);

    ov_sip_message *msg = get_message((ov_serde *)self, 0);
    uint32_t len = get_content_length_from_headers(msg);
    char const *type = get_content_type_from_headers(msg);

    OV_ASSERT(0 < len);

    if (buf->length < len) {
        return P_CONTINUE;
    } else {
        return set_message_body(msg, buf, len, type, res);
    }
}

/*----------------------------------------------------------------------------*/

static bool body_expected(SipSerde *self) {
    return 0 != get_content_length_from_headers(get_message(&self->public, 0));
}

/*----------------------------------------------------------------------------*/

static ElementParseState parse_raw_data(SipSerde *self,
                                        ov_buffer *buf,
                                        ov_result *res) {
    if (0 == self) {
        RESULT_ERROR(res, "No SIP Serde given");
        return P_ERROR;
    } else {

        ElementParseState state = P_OK;
        switch (self->pstate) {

            case M_START_LINE:

                state = parse_start_line(self, buf, res);
                if (P_OK == state) {
                    self->pstate = M_HEADERS;
                    state = P_OK_MORE_ELEMENTS;
                }
                return state;

            case M_HEADERS:
                state = parse_headers(self, buf, res);
                if ((P_OK == state) && body_expected(self)) {
                    self->pstate = M_BODY;
                    state = P_OK_MORE_ELEMENTS;
                }
                return state;

            case M_BODY:

                return parse_body(self, buf, res);

            default:
                PANIC("Unknown enum value");
        }
    }
}

/*----------------------------------------------------------------------------*/

static ov_serde_state parse(SipSerde *sip, ov_buffer buffer, ov_result *res) {

    if (0 == sip) {
        return OV_SERDE_ERROR;

    } else if (buffer_empty(&buffer)) {
        return OV_SERDE_PROGRESS;

    } else {

        OV_ASSERT(0 == sip->residuum);

        ElementParseState state = P_OK_MORE_ELEMENTS;

        while (P_OK_MORE_ELEMENTS == state) {
            state = parse_raw_data(sip, &buffer, res);
        };

        sip->residuum = ov_buffer_copy((void **)&sip->residuum, &buffer);

        switch (state) {

            case P_OK_MORE_ELEMENTS:
                PANIC("Unexpected state P_OK");

            case P_CONTINUE:
                return OV_SERDE_PROGRESS;

            case P_OK:
                sip->pstate = M_DONE;
                lb_reset(&sip->line_buffer);
                return OV_SERDE_END;

            case P_ERROR:
                reset_sip_serde(sip);
                return OV_SERDE_ERROR;

            default:
                PANIC("Invalid enum value");
        }
    }
}

/*----------------------------------------------------------------------------*/

static ov_serde_state impl_add_raw(ov_serde *self,
                                   ov_buffer const *buf,
                                   ov_result *res) {
    UNUSED(buf);
    UNUSED(res);

    SipSerde *sip = as_sip_serde(self, res);

    if ((!reset_serde(self, res)) || (0 == sip) || (!is_buf_valid(buf, res))) {

        return OV_SERDE_ERROR;

    } else {

        ov_buffer *pbuf = ov_buffer_concat(sip->residuum, buf);
        sip->residuum = 0;

        ov_buffer parse_buffer = {
            .start = pbuf->start,
            .length = pbuf->length,
            .magic_byte = pbuf->magic_byte,
        };

        ov_serde_state state = parse(sip, parse_buffer, res);
        pbuf = ov_buffer_free(pbuf);

        return state;
    }
}

/*****************************************************************************
                                   POP DATUM
 ****************************************************************************/

static ov_sip_message *get_next_message(SipSerde *sip, ov_result *res) {

    if (0 == sip) {
        return 0;
    } else if (M_DONE == sip->pstate) {
        return sip->message;

    } else if (0 == sip->residuum) {
        return 0;
    } else {

        ov_buffer *residuum = sip->residuum;
        sip->residuum = 0;

        ov_serde_state state = impl_add_raw(&sip->public, residuum, res);
        residuum = ov_buffer_free(residuum);

        switch (state) {

            case OV_SERDE_END:
                return sip->message;

            case OV_SERDE_ERROR:
                return 0;

            case OV_SERDE_PROGRESS:
                return 0;

            default:
                PANIC("UNKNOWN ENUM STATE");
        };
    }
}

/*----------------------------------------------------------------------------*/

static ov_serde_data impl_pop_datum(ov_serde *self, ov_result *res) {

    SipSerde *sip = as_sip_serde(self, 0);

    ov_serde_data data = {
        .data_type = OV_SIP_SERDE_TYPE,
        .data = get_next_message(sip, res),
    };

    if (0 != data.data) {
        sip->message = 0;
        sip->pstate = M_START_LINE;
    } else {
        ov_log_info("No message ready");
    }

    return data;
}

/*****************************************************************************
                                   SERIALIZE
 ****************************************************************************/

static ssize_t print_formatted(
    char *write_ptr, size_t capacity, ov_result *res, char const *format, ...) {
    va_list ap;

    if (0 == write_ptr) {
        ov_result_set(res,
                      OV_ERROR_INTERNAL_SERVER,
                      "Could not serialize SIP message: Invalid write buffer");
        return -1;
    } else {

        va_start(ap, format);
        int retval = vsnprintf(write_ptr, capacity, format, ap);
        va_end(ap);

        if ((retval > -1) && ((size_t)retval <= capacity)) {
            return retval;
        } else {
            ov_result_set(res,
                          OV_ERROR_INTERNAL_SERVER,
                          "Could not serialize SIP message: write buffer too "
                          "small");
            return -1;
        }
    }
}

/*----------------------------------------------------------------------------*/

static ssize_t serialize_request_start_line(ov_sip_message *msg,
                                            char *write_ptr,
                                            size_t capacity,
                                            ov_result *res) {

    char const *method = ov_sip_message_method(msg);
    char const *uri = ov_sip_message_uri(msg);

    if ((0 == method) || (0 == uri)) {

        ov_result_set(res, OV_ERROR_INTERNAL_SERVER, "Method or URI missing");
        return -1;

    } else {

        return print_formatted(write_ptr,
                               capacity,
                               res,
                               "%s %s %s%s",
                               method,
                               uri,
                               SIP_VERSION,
                               CRLF);
    }
}

/*----------------------------------------------------------------------------*/

static ssize_t serialize_response_start_line(ov_sip_message *msg,
                                             char *write_ptr,
                                             size_t capacity,
                                             ov_result *res) {

    int16_t code = ov_sip_message_response_code(msg);
    char const *reason = ov_sip_message_response_reason(msg);

    if (0 == write_ptr) {

        ov_result_set(res,
                      OV_ERROR_INTERNAL_SERVER,
                      "Could not serialize SIP start line: Invalid write "
                      "buffer");
        return -1;

    } else if (0 > code) {

        ov_result_set(res,
                      OV_ERROR_INTERNAL_SERVER,
                      "Could not serialize SIP start line: Could not get "
                      "code");
        return -1;

    } else if (0 == reason) {

        ov_result_set(res, OV_ERROR_INTERNAL_SERVER, "Reason string missing");
        return -1;

    } else {

        return print_formatted(write_ptr,
                               capacity,
                               res,
                               "%s %3" PRIi16 " %s%s",
                               SIP_VERSION,
                               code,
                               reason,
                               CRLF);
    }
}

/*----------------------------------------------------------------------------*/

static ssize_t serialize_start_line(ov_serde *self,
                                    ov_sip_message *msg,
                                    char *write_ptr,
                                    size_t capacity,
                                    ov_result *res) {
    UNUSED(self);

    switch (ov_sip_message_type_get(msg)) {

        case OV_SIP_INVALID:
            ov_result_set(res,
                          OV_ERROR_INTERNAL_SERVER,
                          "Cannot serialize SIP message: Invalid type");
            return -1;

        case OV_SIP_REQUEST:
            return serialize_request_start_line(msg, write_ptr, capacity, res);

        case OV_SIP_RESPONSE:
            return serialize_response_start_line(msg, write_ptr, capacity, res);

        default:
            PANIC("INVALID enum value");
    };
}

/*----------------------------------------------------------------------------*/

struct serialize_header_struct {
    char *write_ptr;
    ssize_t written;
    size_t capacity;
    ov_result *res;
};

/*----------------------------------------------------------------------------*/

static struct serialize_header_struct *as_serialize_header_struct(void *arg) {

    struct serialize_header_struct *hs = arg;

    if ((0 == hs) || (0 == hs->write_ptr)) {
        ov_log_error("Cannot serialize SIP headers: invalid argument");
        return 0;
    } else {
        return hs;
    }
}

/*----------------------------------------------------------------------------*/

static bool serialize_header(char const *name, char const *value, void *arg) {

    struct serialize_header_struct *hs = as_serialize_header_struct(arg);

    if ((0 == name) || (0 == value) || (0 == hs)) {
        return false;
    } else if (0 > hs->written) {

        // Error occured before
        return false;
    } else {

        int retval =
            snprintf(hs->write_ptr, hs->capacity, "%s: %s" CRLF, name, value);

        if ((retval > -1) && ((size_t)retval <= hs->capacity)) {
            hs->written += retval;
            hs->capacity -= retval;
            hs->write_ptr += retval;
        } else {
            hs->written = -1;
        }

        return true;
    }
}

/*----------------------------------------------------------------------------*/

static ssize_t serialize_headers(ov_serde *self,
                                 ov_sip_message *msg,
                                 char *write_ptr,
                                 size_t capacity,
                                 ov_result *res) {

    SipSerde *sip = as_sip_serde(self, res);

    if ((0 == sip) || (0 == write_ptr)) {

        ov_result_set(res,
                      1000,
                      "Cannot serialize SIP headers: Invalid SIP object or "
                      "write buffer too small");
        return -1;
    }

    struct serialize_header_struct arg = {
        .write_ptr = write_ptr,
        .written = 0,
        .capacity = capacity,
        .res = res,
    };

    ov_sip_message_header_for_each(msg, serialize_header, &arg);

    if (0 > arg.written) {

        ov_result_set(res,
                      1000,
                      "Could not serialize SIP message: Could not "
                      "serialize headers");
    }

    return arg.written;
}

/*----------------------------------------------------------------------------*/

static ssize_t serialize_body(ov_serde *self,
                              ov_sip_message *msg,
                              char *write_ptr,
                              size_t capacity,
                              ov_result *res) {

    ov_buffer const *body = ov_sip_message_body(msg);

    if ((0 == as_sip_serde(self, res)) || (0 == write_ptr)) {

        ov_result_set(res,
                      1000,
                      "Cannot serialize SIP message body: Invalid SIP "
                      "object "
                      "or no write buffer");
        return -1;

    } else if (0 == body) {
        return 0;

    } else if (capacity < body->length) {

        ov_result_set(res,
                      1000,
                      "Cannot serialize SIP message body: Write buffer too "
                      "small");
        return -1;

    } else {
        memcpy(write_ptr, body->start, body->length);
        return body->length;
    }
}

/*----------------------------------------------------------------------------*/

static bool impl_serialize(ov_serde *self,
                           int fh,
                           ov_serde_data data,
                           ov_result *res) {
    if ((0 == as_sip_serde(self, res)) || (!is_data_valid(data, res))) {

        return false;

    } else {

        ov_sip_message *msg = data.data;

        char buffer[10000] = {0};
        size_t capacity = sizeof(buffer);

        char *write_ptr = buffer;
        size_t written_total = 0;

        ssize_t written =
            serialize_start_line(self, msg, write_ptr, capacity, res);

        if (-1 < written) {

            capacity -= written;
            written_total += written;
            write_ptr += written;

            written = serialize_headers(self, msg, write_ptr, capacity, res);

            if (-1 < written) {
                capacity -= written;
                written_total += written;
                write_ptr += written;

                if ((-1 < written) && (capacity > 1)) {
                    written = 2;
                    memcpy(write_ptr, CRLF, written);
                    capacity -= written;
                    written_total += written;
                    write_ptr += written;

                    written =
                        serialize_body(self, msg, write_ptr, capacity, res);
                    capacity -= written;
                    written_total += written;
                    write_ptr += written;

                    if (-1 < written) {

                        written = write(fh, buffer, written_total);
                    }

                } else {

                    ov_log_error(
                        "Cannot serialize SIP message: write buffer too "
                        "small");
                }
            }
        }

        return -1 < written;
    }
}

/*****************************************************************************
                                  CLEAR BUFFER
 ****************************************************************************/

static bool impl_clear_buffer(ov_serde *self) {

    SipSerde *sip = as_sip_serde(self, 0);

    if (0 != sip) {
        reset_sip_serde(sip);
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

ov_serde *ov_sip_serde() {
    SipSerde *sip = calloc(1, sizeof(SipSerde));

    ov_serde *serde = &sip->public;

    serde->magic_bytes = OV_SIP_SERDE_TYPE;

    serde->add_raw = impl_add_raw;
    serde->pop_datum = impl_pop_datum;
    serde->clear_buffer = impl_clear_buffer;
    serde->serialize = impl_serialize;
    serde->free = impl_free;

    // We start off with new message -> Start line expected, nothing
    // parsed yet
    sip->pstate = M_START_LINE;
    sip->residuum = 0;

    lb_reset(&sip->line_buffer);

    return serde;
}

/*----------------------------------------------------------------------------*/

ov_serde_state ov_sip_serde_add_raw(ov_serde *self, ov_buffer const *buffer) {
    ov_result res = {0};

    ov_serde_state state = ov_serde_add_raw(self, buffer, &res);

    if (OV_SERDE_ERROR == state) {
        ov_log_error(
            "Error in parsing SIP input: %s", ov_string_sanitize(res.message));
    }

    ov_result_clear(&res);

    return state;
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_serde_pop_datum(ov_serde *self, ov_result *res) {
    if (0 == as_sip_serde(self, 0)) {
        ov_log_error("Not a SIP Serde object");
        return 0;
    } else {

        ov_serde_data data = ov_serde_pop_datum(self, res);

        OV_ASSERT(OV_SIP_SERDE_TYPE == data.data_type);

        return data.data;
    }
}

/*----------------------------------------------------------------------------*/

char *ov_sip_message_to_string(ov_serde *self, ov_sip_message *msg) {
    ov_serde_data data = {
        .data_type = OV_SIP_SERDE_TYPE,
        .data = msg,
    };

    ov_result result = {0};

    char *str = ov_serde_to_string(self, data, &result);

    if (0 == str) {
        ov_log_error("Could not stringify sip message: %s",
                     ov_string_sanitize(result.message));
    }

    ov_result_clear(&result);

    return str;
}

/*----------------------------------------------------------------------------*/
