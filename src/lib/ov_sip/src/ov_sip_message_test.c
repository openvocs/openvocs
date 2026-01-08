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

#include "ov_sip_headers.h"
#include "ov_sip_message.c"
#include "ov_sip_message.h"
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

static bool count_headers_func(char const *name, char const *value, void *add) {

    ov_log_info("Saw header %s:%s", name, value);

    size_t *num = add;
    ++(*num);

    return true;
}

/*----------------------------------------------------------------------------*/

static size_t count_headers(ov_sip_message const *self) {

    size_t num_headers = 0;
    ov_sip_message_header_for_each(self, count_headers_func, &num_headers);
    return num_headers;
}

/*----------------------------------------------------------------------------*/

static char const *string_from_buffer(ov_buffer const *buf) {

    if (!ov_ptr_valid(buf, "Invalid buffer")) {
        return 0;
    } else {
        return (char const *)buf->start;
    }
}

/*----------------------------------------------------------------------------*/

static bool buffer_equals_string(ov_buffer const *buf, char const *ref) {

    return 0 == ov_string_compare(string_from_buffer(buf), ref);
}

/*----------------------------------------------------------------------------*/

static int test_ov_sip_message_request_create() {

    testrun(0 == ov_sip_message_request_create(0, 0));
    testrun(0 == ov_sip_message_request_create("INVITE", 0));
    testrun(0 == ov_sip_message_request_create(0, "sip:honk.danube.org"));

    ov_sip_message *msg =
        ov_sip_message_request_create("INVITE", "sip:alburga.org");

    testrun(OV_SIP_REQUEST == ov_sip_message_type_get(msg));
    testrun(0 == ov_string_compare("INVITE", ov_sip_message_method(msg)));
    testrun(0 == ov_string_compare("sip:alburga.org", ov_sip_message_uri(msg)));

    testrun(1 == count_headers(msg));

    msg = ov_sip_message_free(msg);
    testrun(0 == msg);

    // Caching

    const size_t num_cached = 10;

    ov_sip_message_enable_caching(num_cached);

    testrun(0 == ov_sip_message_request_create(0, 0));
    testrun(0 == ov_sip_message_request_create("INVITE", 0));
    testrun(0 == ov_sip_message_request_create(0, "sip:honk.danube.org"));

    ov_sip_message *cached[num_cached];

    // First get all messages to ensure they are not the SAME message
    for (size_t i = 0; i < num_cached; ++i) {
        cached[i] = ov_sip_message_request_create("INVITE", "sip:alburga.org");
        testrun(0 != cached[i]);
    }

    for (size_t i = 0; i < num_cached; ++i) {
        testrun(0 == ov_sip_message_free(cached[i]));
    }

    ov_sip_message *msgs[num_cached];
    memset(msgs, 0, sizeof(msgs));

    for (size_t i = 0; i < num_cached; ++i) {
        msg = ov_sip_message_request_create("REGISTER", "a.b.c");
        testrun(OV_SIP_REQUEST == ov_sip_message_type_get(msg));
        testrun(0 == ov_string_compare("REGISTER", ov_sip_message_method(msg)));
        testrun(0 == ov_string_compare("a.b.c", ov_sip_message_uri(msg)));
        testrun(1 == count_headers(msg));

        testrun(!ov_utils_is_in_array(msgs, num_cached, msg));

        // But it should be one of the messages we created & freed before
        testrun(ov_utils_is_in_array(cached, num_cached, msg));

        msgs[i] = msg;
    }

    for (size_t i = 0; i < num_cached; ++i) {
        msgs[i] = ov_sip_message_free(msgs[i]);
        testrun(0 == msgs[i]);
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_sip_message_response_create() {

    testrun(0 == ov_sip_message_response_create(0, 0));
    testrun(0 == ov_sip_message_response_create(110, 0));

    char const *reason = "I ain't got no reason";

    testrun(0 == ov_sip_message_response_create(0, reason));
    testrun(0 == ov_sip_message_response_create(1, reason));
    testrun(0 == ov_sip_message_response_create(700, reason));

    ov_sip_message *msg = ov_sip_message_response_create(110, reason);

    testrun(OV_SIP_RESPONSE == ov_sip_message_type_get(msg));
    testrun(0 == ov_sip_message_method(msg));
    testrun(0 == ov_sip_message_uri(msg));
    testrun(110 == ov_sip_message_response_code(msg));
    testrun(1 == count_headers(msg));

    testrun(0 ==
            ov_string_compare(reason, ov_sip_message_response_reason(msg)));

    msg = ov_sip_message_free(msg);
    testrun(0 == msg);

    msg = ov_sip_message_response_create(699, reason);

    testrun(OV_SIP_RESPONSE == ov_sip_message_type_get(msg));
    testrun(0 == ov_sip_message_method(msg));
    testrun(0 == ov_sip_message_uri(msg));
    testrun(699 == ov_sip_message_response_code(msg));
    testrun(1 == count_headers(msg));
    testrun(0 ==
            ov_string_compare(reason, ov_sip_message_response_reason(msg)));

    msg = ov_sip_message_free(msg);
    testrun(0 == msg);

    // Check caching

    const size_t num_cached = 10;

    ov_sip_message *cached[num_cached];

    // First get all messages to ensure they are not the SAME message
    for (size_t i = 0; i < num_cached; ++i) {
        cached[i] = ov_sip_message_response_create(100 + i, reason);
        testrun(0 != cached[i]);
    }

    for (size_t i = 0; i < num_cached; ++i) {
        testrun(0 == ov_sip_message_free(cached[i]));
    }

    ov_sip_message *msgs[num_cached];
    memset(msgs, 0, sizeof(msgs));

    for (size_t i = 0; i < num_cached; ++i) {
        msg = ov_sip_message_response_create(100 + i, reason);
        testrun(OV_SIP_RESPONSE == ov_sip_message_type_get(msg));
        testrun(0 ==
                ov_string_compare(reason, ov_sip_message_response_reason(msg)));
        testrun(1 == count_headers(msg));

        testrun(!ov_utils_is_in_array(msgs, num_cached, msg));

        // But it should be one of the messages we created & freed before
        testrun(ov_utils_is_in_array(cached, num_cached, msg));

        msgs[i] = msg;
    }

    for (size_t i = 0; i < num_cached; ++i) {
        msgs[i] = ov_sip_message_free(msgs[i]);
        testrun(0 == msgs[i]);
    }

    return testrun_log_success();
}

/*****************************************************************************
                              ov_sip_message_copy
 ****************************************************************************/

struct find_compare_header_arg {

    bool found_and_equals;
    ov_sip_message const *other_msg;
};

/*----------------------------------------------------------------------------*/

static bool find_compare_header(char const *header, char const *value,
                                void *additional) {

    struct find_compare_header_arg *arg = additional;

    if ((0 == header) || (0 == value) || (0 == arg)) {
        return false;
    } else {

        arg->found_and_equals =
            (0 == ov_string_compare(
                      value, ov_sip_message_header(arg->other_msg, header)));

        return arg->found_and_equals;
    }
}

/*----------------------------------------------------------------------------*/

static bool all_headers_from_msg1_in_msg2(ov_sip_message const *msg1,
                                          ov_sip_message const *msg2) {

    struct find_compare_header_arg arg = {
        .found_and_equals = false,
        .other_msg = msg2,
    };

    if ((0 == msg1) || (0 == msg2)) {
        return (msg1 == msg2);
    } else {

        ov_sip_message_header_for_each(msg1, find_compare_header, &arg);
        return arg.found_and_equals;
    }
}

/*----------------------------------------------------------------------------*/

static bool message_headers_equal(ov_sip_message const *msg1,
                                  ov_sip_message const *msg2) {

    // The comparison method here is incredibly slow but straight forward,
    // and we are not dealing with time-critical code or messages that contain
    // 100s of headers here
    return all_headers_from_msg1_in_msg2(msg1, msg2) &&
           all_headers_from_msg1_in_msg2(msg2, msg1);
}

/*----------------------------------------------------------------------------*/

static bool message_bodies_equal(ov_buffer const *b1, ov_buffer const *b2) {

    if ((0 == b1) || (0 == b2)) {
        return b1 == b2;
    } else if ((0 == b1->start) || (0 == b2->start) ||
               (b1->length != b2->length)) {
        return false;
    } else {
        return 0 == memcmp(b1->start, b2->start, b1->length);
    }
}

/*----------------------------------------------------------------------------*/

static bool messages_equal(ov_sip_message const *msg1,
                           ov_sip_message const *msg2) {

    switch (ov_sip_message_type_get(msg1)) {

    case OV_SIP_REQUEST:

        return (OV_SIP_REQUEST == ov_sip_message_type_get(msg2)) &&
               (0 == ov_string_compare(ov_sip_message_uri(msg1),
                                       ov_sip_message_uri(msg2))) &&
               message_headers_equal(msg1, msg2) &&
               message_bodies_equal(ov_sip_message_body(msg1),
                                    ov_sip_message_body(msg2));

    case OV_SIP_RESPONSE:

        return (OV_SIP_REQUEST == ov_sip_message_type_get(msg2)) &&
               (0 == ov_string_compare(ov_sip_message_response_reason(msg1),
                                       ov_sip_message_response_reason(msg2))) &&
               message_headers_equal(msg1, msg2) &&
               message_bodies_equal(ov_sip_message_body(msg1),
                                    ov_sip_message_body(msg2));

    case OV_SIP_INVALID:
        return (0 == msg1) && (0 == msg2);

    default:
        OV_PANIC("Invalid enum value");
    };
}

/*----------------------------------------------------------------------------*/

static int test_ov_sip_message_copy() {

    char const *method = "ABRA";
    char const *uri = "CADABRA";

    testrun(0 == ov_sip_message_copy(0));

    ov_sip_message *msg = ov_sip_message_request_create(method, uri);
    ov_sip_message *copy = ov_sip_message_copy(msg);
    testrun(0 != copy);
    testrun(messages_equal(msg, copy));
    copy = ov_sip_message_free(copy);

    testrun(ov_sip_message_header_set(msg, "header1", "1"));
    testrun(ov_sip_message_header_set(msg, "header2", "2"));
    copy = ov_sip_message_copy(msg);
    testrun(messages_equal(msg, copy));

    copy = ov_sip_message_free(copy);

    ov_buffer *body = ov_buffer_from_string("abcd");
    testrun(ov_sip_message_body_set(msg, body, "Loaded"));
    body = 0;
    copy = ov_sip_message_copy(msg);
    testrun(messages_equal(msg, copy));

    copy = ov_sip_message_free(copy);

    msg = ov_sip_message_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_sip_message_cseq() {

    testrun(0 == ov_sip_message_cseq(0, 0));

    ov_sip_message *msg = ov_sip_message_request_create("REGISTER", "rapa_nui");

    testrun(0 == ov_sip_message_cseq(msg, 0));

    char const *method = 0;
    testrun(0 == ov_sip_message_cseq(msg, &method));
    testrun(0 == method);

    testrun(0 == ov_sip_message_cseq(0, &method));

    testrun(ov_sip_message_cseq_set(msg, "REGISTER", 1324));
    testrun(1324 == ov_sip_message_cseq(msg, 0));

    testrun(1324 == ov_sip_message_cseq(msg, &method));
    testrun(0 != method);
    testrun(0 == ov_string_compare(method, "REGISTER"));

    msg = ov_sip_message_free(msg);
    testrun(0 == msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_sip_message_cseq_set() {

    testrun(!ov_sip_message_cseq_set(0, 0, 0));

    ov_sip_message *msg = ov_sip_message_request_create("REGISTER", "rapa_nui");

    testrun(!ov_sip_message_cseq_set(msg, 0, 0));
    testrun(!ov_sip_message_cseq_set(0, 0, 1));
    testrun(!ov_sip_message_cseq_set(0, "REGISTER", 0));
    // Method does not fit method in start line of message - must be possible
    // since e.g. the ACK must use the 'CSeq' header of the message it
    // acknowledges
    testrun(ov_sip_message_cseq_set(msg, "INVITE", 1));

    char const *method = 0;
    testrun(1 == ov_sip_message_cseq(msg, &method));
    testrun(0 == ov_string_compare(method, "INVITE"));

    // Metohd DOES fit method in start line of message
    testrun(ov_sip_message_cseq_set(msg, "REGISTER", 0));

    testrun(0 == ov_sip_message_cseq(msg, &method));
    testrun(0 == ov_string_compare(method, "REGISTER"));
    method = 0;

    testrun(0 ==
            ov_string_compare(ov_sip_message_header(msg, OV_SIP_HEADER_CSEQ),
                              "0 REGISTER"));

    testrun(ov_sip_message_cseq_set(msg, "REGISTER", 17));
    testrun(17 == ov_sip_message_cseq(msg, &method));
    testrun(0 == ov_string_compare(method, "REGISTER"));

    testrun(0 ==
            ov_string_compare(ov_sip_message_header(msg, OV_SIP_HEADER_CSEQ),
                              "17 REGISTER"));

    msg = ov_sip_message_free(msg);
    testrun(0 == msg);

    /*************************************************************************
                             Same with SIP response
     ************************************************************************/

    msg = ov_sip_message_response_create(110, "Applaus");
    testrun(0 != msg);

    testrun(!ov_sip_message_cseq_set(msg, 0, 0));
    testrun(!ov_sip_message_cseq_set(0, 0, 1));
    testrun(!ov_sip_message_cseq_set(0, "REGISTER", 0));
    testrun(ov_sip_message_cseq_set(msg, "REGISTER", 0));

    testrun(0 == ov_sip_message_cseq(msg, &method));
    testrun(0 == ov_string_compare(method, "REGISTER"));
    method = 0;

    testrun(0 ==
            ov_string_compare(ov_sip_message_header(msg, OV_SIP_HEADER_CSEQ),
                              "0 REGISTER"));

    testrun(ov_sip_message_cseq_set(msg, "INVITE", 17));
    testrun(17 == ov_sip_message_cseq(msg, &method));
    testrun(0 == ov_string_compare(method, "INVITE"));

    testrun(0 ==
            ov_string_compare(ov_sip_message_header(msg, OV_SIP_HEADER_CSEQ),
                              "17 INVITE"));

    msg = ov_sip_message_free(msg);
    testrun(0 == msg);

    return testrun_log_success();
}
/*----------------------------------------------------------------------------*/

static int test_ov_sip_message_header_set() {

    testrun(!ov_sip_message_header_set(0, 0, 0));

    ov_sip_message *msg = ov_sip_message_request_create("REGISTER", "rapa_nui");
    testrun(0 != msg);

    testrun(!ov_sip_message_header_set(msg, 0, 0));
    testrun(!ov_sip_message_header_set(0, "h1", 0));
    testrun(!ov_sip_message_header_set(msg, "h1", 0));
    testrun(!ov_sip_message_header_set(0, 0, "v1"));
    testrun(!ov_sip_message_header_set(msg, 0, "v1"));
    testrun(ov_sip_message_header_set(msg, "h1", "v1"));

    msg = ov_sip_message_free(msg);
    testrun(0 == msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_sip_message_header() {

    testrun(0 == ov_sip_message_header(0, 0));

    ov_sip_message *msg = ov_sip_message_request_create("REGISTER", "rapa_nui");
    testrun(0 != msg);

    testrun(0 == ov_sip_message_header(msg, 0));
    testrun(0 == ov_sip_message_header(0, "h2"));
    testrun(0 == ov_sip_message_header(msg, "h2"));

    testrun(ov_sip_message_header_set(msg, "h2", "v2"));

    char const *value = ov_sip_message_header(msg, "h2");

    testrun(0 != value);

    testrun(0 == ov_string_compare(value, "v2"));

    msg = ov_sip_message_free(msg);
    testrun(0 == msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_sip_message_header_for_each() {

    testrun(!ov_sip_message_header_for_each(0, 0, 0));

    ov_sip_message *msg = ov_sip_message_response_create(140, "aha");
    testrun(0 != msg);

    ov_sip_message_header_set(msg, "h1", "v1");
    ov_sip_message_header_set(msg, "h2", "v2");
    ov_sip_message_header_set(msg, "h3", "v3");

    testrun(!ov_sip_message_header_for_each(msg, 0, 0));
    testrun(!ov_sip_message_header_for_each(0, count_headers_func, 0));

    // The last argument might be 0, thus we won't test
    size_t counter = 0;

    testrun(!ov_sip_message_header_for_each(0, 0, &counter));
    testrun(!ov_sip_message_header_for_each(msg, 0, &counter));
    testrun(ov_sip_message_header_for_each(msg, count_headers_func, &counter));

    testrun(4 == counter);

    msg = ov_sip_message_free(msg);
    testrun(0 == msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_sip_message_body() {

    testrun(0 == ov_sip_message_body(0));

    ov_sip_message *msg =
        ov_sip_message_request_create("INVITE", "obldadi.oblada");
    testrun(0 != msg);

    testrun(0 == ov_sip_message_body(0));

    ov_buffer *body = ov_buffer_from_string("live goes on - bra");
    testrun(0 != body);

    testrun(ov_sip_message_body_set(msg, body, "ASCII"));

    testrun(body == ov_sip_message_body(msg));
    body = 0;

    msg = ov_sip_message_free(msg);
    testrun(0 == msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_sip_message_body_set() {

    testrun(!ov_sip_message_body_set(0, 0, 0));

    ov_sip_message *msg =
        ov_sip_message_request_create("INVITE", "obldadi.oblada");
    testrun(0 != msg);

    char const *content_length =
        ov_sip_message_header(msg, OV_SIP_HEADER_CONTENT_LENGTH);

    testrun(0 != content_length);
    testrun(0 == atoi(content_length));

    testrun(!ov_sip_message_body_set(msg, 0, 0));

    ov_buffer *body = ov_buffer_from_string("live goes on - bra");
    testrun(0 != body);

    testrun(!ov_sip_message_body_set(0, body, 0));
    testrun(!ov_sip_message_body_set(msg, body, 0));
    testrun(!ov_sip_message_body_set(0, 0, "ASCII"));
    testrun(!ov_sip_message_body_set(msg, 0, "ASCII"));

    testrun(ov_sip_message_body_set(msg, body, "ASCII"));

    testrun(body == ov_sip_message_body(msg));

    content_length = ov_sip_message_header(msg, OV_SIP_HEADER_CONTENT_LENGTH);

    testrun(body->length == (size_t)atoi(content_length));
    body = 0;

    char const *content_type =
        ov_sip_message_header(msg, OV_SIP_HEADER_CONTENT_TYPE);

    testrun(0 == ov_string_compare("ASCII", content_type));

    msg = ov_sip_message_free(msg);
    testrun(0 == msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool caller_equals(ov_sip_message const *msg, char const *str) {

    ov_buffer *caller = ov_sip_message_get_caller(msg);
    bool equals = buffer_equals_string(caller, str);
    ov_buffer_free(caller);
    return equals;
}

/*----------------------------------------------------------------------------*/

static int test_ov_sip_message_get_caller() {

    // ov_buffer *ov_sip_message_get_caller(ov_sip_message const *self);
    testrun(0 == ov_sip_message_get_caller(0));

    ov_sip_message *msg = ov_sip_message_request_create(
        "INVITE", "sip:001234567890@10.135.0.1:5060;user=phone");
    testrun(0 != msg);
    ov_sip_message_header_set(msg, OV_SIP_HEADER_FROM,
                              " \"Calling User\" "
                              "<sip:151@10.135.0.1:5060>;tag=m3l2hbp");

    testrun(caller_equals(msg, "sip:151@10.135.0.1:5060"));
    testrun(!caller_equals(msg, "pip:151@10.135.0.1:5060"));
    testrun(!caller_equals(msg, "sip:151@10.135.0.1:5061"));
    testrun(!caller_equals(msg, "sip:151@10.145.0.1:5060"));

    ov_sip_message_header_set(msg, OV_SIP_HEADER_FROM,
                              " \"Calling User\" "
                              "<sip:151@10.135.0.1:5060;tag=m3l2hbp");

    testrun(0 == ov_sip_message_get_caller(msg));

    ov_sip_message_header_set(msg, OV_SIP_HEADER_FROM,
                              " \"Calling User\" "
                              "sip:151@10.135.0.1:5060>;tag=m3l2hbp");

    testrun(0 == ov_sip_message_get_caller(msg));

    ov_sip_message_header_set(msg, OV_SIP_HEADER_FROM,
                              " \"Calling User\" "
                              "<sip:151@10.135.0.1:5060>;tag=m3l2hbp");

    testrun(caller_equals(msg, "sip:151@10.135.0.1:5060"));

    msg = ov_sip_message_free(msg);
    testrun(0 == msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_sip_message_free() {

    testrun(0 == ov_sip_message_free(0));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int tear_down() {

    ov_registered_cache_free_all();
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_sip_message", test_ov_sip_message_request_create,
            test_ov_sip_message_response_create, test_ov_sip_message_copy,
            test_ov_sip_message_cseq, test_ov_sip_message_cseq_set,
            test_ov_sip_message_header_set, test_ov_sip_message_header,
            test_ov_sip_message_header_for_each, test_ov_sip_message_body,
            test_ov_sip_message_body_set, test_ov_sip_message_get_caller,
            test_ov_sip_message_free, tear_down);

/*----------------------------------------------------------------------------*/
