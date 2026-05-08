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

#include "../include/ov_sip_message.h"
#include <../include/ov_sip_headers.h>
#include <ov_base/ov_registered_cache.h>
#include <ov_base/ov_string.h>

/*----------------------------------------------------------------------------*/

char const *ov_sip_message_type_to_string(ov_sip_message_type type) {

    switch (type) {

    case OV_SIP_INVALID:
        return "INVALID";

    case OV_SIP_REQUEST:
        return "REQUEST";

    case OV_SIP_RESPONSE:
        return "RESPONSE";

    default:
        ov_log_error("Never to happen");
        abort();
    };
}

/*----------------------------------------------------------------------------*/

static ov_registered_cache *g_sip_message_cache = 0;

/*----------------------------------------------------------------------------*/

typedef struct {

    char *method;
    char *uri;

} RequestLine;

typedef struct {
    int16_t code;
    char *reason;
} ResponseLine;

struct ov_sip_message {

    ov_sip_message_type type;

    union {
        RequestLine request;
        ResponseLine response;
    };

    ov_hashtable *headers;

    ov_buffer *body;
};

/*****************************************************************************
                                 ACCESS helpers
 ****************************************************************************/

static bool is_code_valid(int16_t code) {

    if (100 > code) {
        ov_log_error("Invalid SIP response code: %" PRIi16 " - too small",
                     code);
        return false;
    } else if (699 < code) {
        ov_log_error("Invalid SIP response code: %" PRIi16 " - too small",
                     code);
        return false;
    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool is_msg_valid(ov_sip_message const *self) {
    if (0 == self) {
        ov_log_error("No message (0 pointer)");
        return false;
    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool is_type(ov_sip_message const *self, ov_sip_message_type type) {

    if (!is_msg_valid(self)) {
        return false;
    } else if (type != self->type) {
        return false;
    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static char *request_method_get(ov_sip_message *self) {

    if (!is_type(self, OV_SIP_REQUEST)) {
        return 0;
    } else {
        return self->request.method;
    }
}

/*----------------------------------------------------------------------------*/

static char *request_uri_get(ov_sip_message *self) {

    if (!is_type(self, OV_SIP_REQUEST)) {
        return 0;
    } else {
        return self->request.uri;
    }
}

/*----------------------------------------------------------------------------*/

static char *response_reason_get(ov_sip_message *self) {

    if (!is_type(self, OV_SIP_RESPONSE)) {
        return 0;
    } else {
        return self->response.reason;
    }
}

/*----------------------------------------------------------------------------*/

static ov_sip_message *fresh_message() {

    ov_sip_message *msg = ov_registered_cache_get(g_sip_message_cache);

    if (0 == msg) {
        msg = calloc(1, sizeof(ov_sip_message));
        msg->type = OV_SIP_INVALID;
    }

    OV_ASSERT(OV_SIP_INVALID == msg->type);

    msg->headers = ov_hashtable_create_c_string(OV_DEFAULT_SIP_NUM_HEADER);

    ov_sip_message_header_set(msg, OV_SIP_HEADER_CONTENT_LENGTH, "0");

    return msg;
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_request_create(char const *request,
                                              char const *uri) {

    if (0 == request) {
        ov_log_error("Cannot create SIP request: Require method");
        return 0;
    } else if (0 == uri) {
        ov_log_error("Cannot create SIP request: Require uri");
        return 0;
    } else {

        ov_sip_message *msg = fresh_message();
        msg->type = OV_SIP_REQUEST;
        msg->request.method = ov_string_dup(request);
        msg->request.uri = ov_string_dup(uri);

        return msg;
    }
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_response_create(uint16_t code,
                                               char const *reason) {

    if (!is_code_valid(code)) {
        return 0;
    } else if (0 == reason) {
        ov_log_error("Cannot create SIP response: Require reason");
        return 0;
    } else {

        ov_sip_message *msg = fresh_message();

        msg->type = OV_SIP_RESPONSE;
        msg->response.code = code;
        msg->response.reason = ov_string_dup(reason);

        return msg;
    }
}

/*----------------------------------------------------------------------------*/

static bool start_line_clear(ov_sip_message *self) {

    ov_free(request_method_get(self));
    ov_free(request_uri_get(self));
    ov_free(response_reason_get(self));

    return true;
}

/*----------------------------------------------------------------------------*/

static bool free_hashtable_value(const void *key, const void *value,
                                 void *arg) {

    UNUSED(key);
    UNUSED(arg);

    ov_free((void *)value);

    return true;
}

/*----------------------------------------------------------------------------*/

static void clear_headers(ov_sip_message *self) {

    if (0 == self) {
        return;
    } else {
        ov_hashtable_for_each(self->headers, free_hashtable_value, 0);
        self->headers = ov_hashtable_free(self->headers);
    }
}

/*****************************************************************************
                                      COPY
 ****************************************************************************/

static ov_sip_message *basic_message_copy(ov_sip_message const *self) {

    switch (ov_sip_message_type_get(self)) {

    case OV_SIP_INVALID:

        ov_log_error("Cannot copy message - invalid message");
        return 0;

    case OV_SIP_RESPONSE:

        return ov_sip_message_response_create(
            ov_sip_message_response_code(self),
            ov_sip_message_response_reason(self));

    case OV_SIP_REQUEST:

        return ov_sip_message_request_create(ov_sip_message_method(self),
                                             ov_sip_message_uri(self));

    default:
        OV_PANIC("Invalid enum value");
    };
}

/*----------------------------------------------------------------------------*/

static bool copy_header(char const *header, char const *value, void *dest_msg) {

    ov_sip_message_header_set(dest_msg, header, value);
    return true;
}

/*----------------------------------------------------------------------------*/

static void copy_headers(ov_sip_message *dest, ov_sip_message const *src) {

    ov_sip_message_header_for_each(src, copy_header, dest);
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_copy(ov_sip_message const *self) {

    ov_sip_message *copy = basic_message_copy(self);
    copy_headers(copy, self);
    if (0 != copy) {
        copy->body = ov_buffer_copy(0, ov_sip_message_body(self));
    }
    return copy;
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_cast(void *ptr) {

    ov_sip_message *msg = ptr;

    switch (ov_sip_message_type_get(msg)) {

    case OV_SIP_REQUEST:

        return msg;

    case OV_SIP_RESPONSE:

        return msg;

    case OV_SIP_INVALID:
    default:

        return 0;
    };
}

/*----------------------------------------------------------------------------*/

static void message_clear(ov_sip_message *self) {

    if (0 != self) {

        start_line_clear(self);
        clear_headers(self);
        ov_buffer_free(self->body);

        memset(self, 0, sizeof(ov_sip_message));
        self->type = OV_SIP_INVALID;
    }
}

/*----------------------------------------------------------------------------*/

static void *message_free(void *msg_vptr) {

    ov_sip_message *msg = msg_vptr;
    message_clear(msg);
    return ov_free(msg);
}

/*----------------------------------------------------------------------------*/

ov_sip_message *ov_sip_message_free(ov_sip_message *self) {

    message_clear(self);
    message_free(ov_registered_cache_put(g_sip_message_cache, self));
    return 0;
}

/*----------------------------------------------------------------------------*/

ov_sip_message_type ov_sip_message_type_get(ov_sip_message const *self) {

    if (!is_msg_valid(self)) {
        return OV_SIP_INVALID;
    } else {
        return self->type;
    }
}

/*----------------------------------------------------------------------------*/

static char const *propagate_to_first_non_space(char const *in) {

    if (0 == in) {
        return 0;
    } else {

        while ((0 != *in) && isspace(*in)) {
            ++in;
        }
        return in;
    }
}

/*----------------------------------------------------------------------------*/

static bool is_non_zero(char value, char const *message) {

    if (0 == value) {

        ov_log_error("Is zero: %s", ov_string_sanitize(message));
        return false;

    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static uint32_t parse_cseq_header(char const *header, char const **method) {

    char const *endptr = 0;
    long cseq = 0;

    if (ov_ptr_valid(header, "Cannot parse CSEQ: No header")) {
        cseq = strtol(header, (char **)&endptr, 10);
        endptr = propagate_to_first_non_space(endptr);
    }

    if (!ov_ptr_valid(endptr,
                      "Could not parse CSEQ header - no valid number") ||
        !is_non_zero(*endptr, "No method given")) {
        return 0;
    } else if (0 != method) {

        *method = endptr;
        return cseq;

    } else {
        return cseq;
    }
}

/*----------------------------------------------------------------------------*/

uint32_t ov_sip_message_cseq(ov_sip_message const *self, char const **method) {

    return parse_cseq_header(ov_sip_message_header(self, OV_SIP_HEADER_CSEQ),
                             method);
}

/*----------------------------------------------------------------------------*/

static char *create_cseq_string(char const *method, uint32_t seq) {

    size_t len = 5 + 1 + ov_string_len(method) + 1;
    char *cseq = calloc(1, len);

    if (ov_ptr_valid(method, "Require valid method string")) {

        snprintf(cseq, len, "%" PRIu32 " %s", seq, method);
        return cseq;

    } else {

        cseq = ov_free(cseq);
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static bool method_fits_if_request(ov_sip_message const *self,
                                   char const *method, char const *message) {

    char const *request_method = request_method_get((ov_sip_message *)self);

    if ((0 != request_method) &&
        (0 != ov_string_compare(request_method, method))) {

        ov_log_warning("Method does not fit Request start line: %s",
                       ov_string_sanitize(message));
    }

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_sip_message_cseq_set(ov_sip_message *self, char const *method,
                             uint32_t seq) {

    char *cseq = create_cseq_string(method, seq);

    bool success =
        method_fits_if_request(self, method, "CSeq method does not fit") &&
        ov_sip_message_header_set(self, OV_SIP_HEADER_CSEQ, cseq);

    cseq = ov_free(cseq);

    return success;
}

/*----------------------------------------------------------------------------*/

char const *ov_sip_message_method(ov_sip_message const *self) {

    if (!(ov_cond_valid(is_type(self, OV_SIP_REQUEST),
                        "Cannot get SIP Message method - SIP message is not "
                        "Request"))) {
        return 0;
    } else {
        return self->request.method;
    }
}

/*----------------------------------------------------------------------------*/

char const *ov_sip_message_uri(ov_sip_message const *self) {

    if (!(ov_cond_valid(is_type(self, OV_SIP_REQUEST),
                        "Cannot get SIP Message URI - SIP message is not "
                        "Request"))) {
        return 0;
    } else {
        return self->request.uri;
    }
}

/*----------------------------------------------------------------------------*/

int16_t ov_sip_message_response_code(ov_sip_message const *self) {

    if (!(ov_cond_valid(is_type(self, OV_SIP_RESPONSE),
                        "Cannot get SIP Message response code - SIP message is "
                        "not Response"))) {
        return 0;
    } else {
        return self->response.code;
    }
}

/*----------------------------------------------------------------------------*/

char const *ov_sip_message_response_reason(ov_sip_message const *self) {

    if (!(ov_cond_valid(is_type(self, OV_SIP_RESPONSE),
                        "Cannot get SIP Message response reason - SIP message "
                        "is not Response"))) {
        return 0;
    } else {
        return self->response.reason;
    }
}

/*----------------------------------------------------------------------------*/

static bool is_header_value_valid(char const *value, char const *name) {

    if (0 == value) {
        ov_log_error("Cannot set SIP header '%s', value invalid",
                     ov_string_sanitize(name));
        return false;
    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_sip_message_header_set(ov_sip_message *self, char const *name,
                               char const *value) {

    if (!is_msg_valid(self) || (!ov_ptr_valid(name, "Missing header name")) ||
        (!is_header_value_valid(value, name))) {
        return false;
    } else {
        char *old = ov_hashtable_set(self->headers, name, ov_string_dup(value));
        old = ov_free(old);
        return true;
    }

    return false;
}

/*----------------------------------------------------------------------------*/

char const *ov_sip_message_header(ov_sip_message const *self,
                                  char const *name) {

    if (!is_msg_valid(self)) {
        return 0;
    } else {
        return ov_hashtable_get(self->headers, name);
    }
}

/*****************************************************************************
                                    FOR EACH
 ****************************************************************************/

struct apply_func_arg {
    void *additional;
    bool (*func)(char const *, char const *, void *);
};

static bool apply_func(void const *name, void const *value, void *additional) {

    struct apply_func_arg *farg = additional;
    OV_ASSERT(0 != farg->func);
    return farg->func(name, value, farg->additional);
}

/*----------------------------------------------------------------------------*/

bool ov_sip_message_header_for_each(ov_sip_message const *self,
                                    bool (*func)(char const *name,
                                                 char const *value,
                                                 void *additional),
                                    void *additional) {

    if ((!is_msg_valid(self)) || (0 == func)) {

        return false;

    } else {

        struct apply_func_arg arg = {
            .additional = additional,
            .func = func,
        };

        return ov_hashtable_for_each(self->headers, apply_func, &arg);
    }
}

/*****************************************************************************
                                      BODY
 ****************************************************************************/

ov_buffer const *ov_sip_message_body(ov_sip_message const *self) {

    if (!is_msg_valid(self)) {
        return 0;
    } else {
        return self->body;
    }
}

/*----------------------------------------------------------------------------*/

static bool ptr_valid(void const *ptr, char const *name) {

    if (0 == ptr) {
        ov_log_error("%s is invalid (0 pointer)", ov_string_sanitize(name));
        return false;
    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_sip_message_body_set(ov_sip_message *self, ov_buffer *body,
                             char const *content_type) {

    if ((!is_msg_valid(self)) || (!ptr_valid(body, "body")) ||
        (!ptr_valid(content_type, "content type"))) {
        return false;
    } else {

        ov_buffer_free(self->body);
        self->body = body;

        char content_length[30] = {0};
        snprintf(content_length, sizeof(content_length), "%zu", body->length);

        ov_sip_message_header_set(self, OV_SIP_HEADER_CONTENT_LENGTH,
                                  content_length);

        ov_sip_message_header_set(self, OV_SIP_HEADER_CONTENT_TYPE,
                                  content_type);

        return true;
    }
}

/*****************************************************************************
                                  SPECIAL DATA
 ****************************************************************************/

static char *our_strchr(char const *str, char c) {

    if (!ov_ptr_valid(str, "Invalid string")) {
        return 0;
    } else {
        return strchr(str, c);
    }
}

/*----------------------------------------------------------------------------*/

static ov_buffer *buffer_from_until(char const *str, char start, char end) {

    char *start_ptr = our_strchr(str, start);
    char *end_ptr = our_strchr(start_ptr, end);

    if ((0 == start_ptr) || (0 == end_ptr)) {
        return 0;
    } else if (start_ptr == end_ptr) {
        ov_log_error("Empty string");
        return 0;
    } else {
        size_t len = end_ptr - start_ptr;
        ov_buffer *buf = ov_buffer_create(len);
        memcpy(buf->start, start_ptr + 1, len);
        buf->start[len - 1] = 0;
        return buf;
    }
}

/*----------------------------------------------------------------------------*/

ov_buffer *ov_sip_message_get_caller(ov_sip_message const *self) {

    ov_buffer *from = buffer_from_until(
        ov_sip_message_header(self, OV_SIP_HEADER_FROM), '<', '>');

    if (0 == from) {
        ov_log_error("No 'From' in SIP message found or invalid format");
        return 0;
    } else {
        return from;
    }
}

/*****************************************************************************
                                    Inspect
 ****************************************************************************/

static bool dump_header(char const *name, char const *value, void *additional) {

    FILE *out = additional;

    if (0 == out) {
        return false;
    } else {

        fprintf(out, "    %s: %s\n", ov_string_sanitize(name),
                ov_string_sanitize(value));

        return true;
    }
}

/*----------------------------------------------------------------------------*/

static void dump_headers(FILE *out, ov_sip_message const *self) {

    if (ov_ptr_valid(
            out, "Tried to dump SIP message headers, but no output stream")) {

        ov_sip_message_header_for_each(self, dump_header, out);
    }
}

/*----------------------------------------------------------------------------*/

static void dump_body(FILE *out, ov_buffer const *body) {

    if (ov_ptr_valid(out, "Tried to dump SIP message, but no output stream")) {

        if (0 == body) {

            fprintf(out, "   --- NO BODY ---\n");

        } else {

            fprintf(out, "%.*s\n", (int)body->length,
                    ov_string_sanitize((char const *)body->start));
        }
    }
}

/*----------------------------------------------------------------------------*/

static void dump_request(FILE *out, ov_sip_message const *self) {

    if (ov_ptr_valid(out, "Tried to dump SIP message, but no output stream") &&
        ov_ptr_valid(self, "Tried to dump SIP message, but no message")) {

        fprintf(out, "\n--- BEGIN SIP MESSAGE ---\n");

        fprintf(out, "SIP Request message: %s - URi: %s\n",
                ov_string_sanitize(ov_sip_message_method(self)),
                ov_string_sanitize(ov_sip_message_uri(self)));

        dump_headers(out, self);
        dump_body(out, ov_sip_message_body(self));

        fprintf(out, "\n--- END SIP MESSAGE ---\n");
    }
}

/*----------------------------------------------------------------------------*/

static void dump_response(FILE *out, ov_sip_message const *self) {

    if ((ov_ptr_valid(out,
                      "Tried to dump SIP message, but no output stream")) &&
        (ov_ptr_valid(self, "Tried to dump SIP message, but no message"))) {

        fprintf(out, "\n--- BEGIN SIP MESSAGE ---\n");

        fprintf(out, "SIP Response message: %" PRIi16 "- %s\n",
                ov_sip_message_response_code(self),
                ov_sip_message_response_reason(self));

        dump_headers(out, self);
        dump_body(out, ov_sip_message_body(self));

        fprintf(out, "\n--- END SIP MESSAGE ---\n");
    }
}

/*----------------------------------------------------------------------------*/

static void dump_invalid(FILE *out) {

    ov_log_error("Tried to dump invalid SIP message");

    if (0 != out) {

        fprintf(out, "Tried to dump invalid SIP message\n");
    }
}

/*----------------------------------------------------------------------------*/

void ov_sip_message_dump(FILE *out, ov_sip_message const *self) {

    switch (ov_sip_message_type_get(self)) {

    case OV_SIP_REQUEST:

        dump_request(out, self);
        break;

    case OV_SIP_RESPONSE:

        dump_response(out, self);
        break;

    case OV_SIP_INVALID:

        dump_invalid(out);
        break;

    default:
        OV_ASSERT(!"MUST NEVER HAPPEN");
    };
}

/*----------------------------------------------------------------------------*/

void ov_sip_message_enable_caching(size_t capacity) {

    ov_registered_cache_config cfg = {
        .capacity = capacity,
        .item_free = message_free,
    };

    g_sip_message_cache = ov_registered_cache_extend("sip_message", cfg);

    ov_hashtable_enable_caching(capacity);
}

/*----------------------------------------------------------------------------*/
