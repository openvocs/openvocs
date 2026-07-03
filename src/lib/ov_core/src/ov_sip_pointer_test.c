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
        @file           ov_sip_pointer_test.c
        @author         Töpfer, Markus

        @date           2026-06-30


        ------------------------------------------------------------------------
*/
#include <ov_test/testrun.h>
#include "ov_sip_pointer.c"

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_sip_message_cast() {

    for (uint16_t i = 0; i < 0xFFFF; i++) {

        if (i == OV_SIP_MESSAGE_MAGIC_BYTE) {
            testrun(ov_sip_message_cast(&i));
        } else {
            testrun(!ov_sip_message_cast(&i));
        }
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sip_message_create(){

    ov_sip_message *msg = ov_sip_message_create((ov_sip_message_config){0});
    testrun(msg);
    testrun(ov_sip_message_cast(msg));
    testrun(msg->buffer);
    testrun(msg->config.header.capacity == IMPL_DEFAULT_HEADER_CAPACITY);
    testrun(msg->config.buffer.default_size == IMPL_DEFAULT_BUFFER_SIZE);
    testrun(msg->body.start == NULL);
    testrun(msg->body.length == 0);
    testrun(msg->version.major == 0);
    testrun(msg->version.minor == 0);
    testrun(msg->request.method.start == NULL);
    testrun(msg->request.method.length == 0);
    testrun(msg->request.uri.start == NULL);
    testrun(msg->request.uri.length == 0);
    testrun(msg->status.phrase.start == NULL);
    testrun(msg->status.phrase.length == 0);
    testrun(msg->status.code == 0);
    for (size_t i = 0; i < msg->config.header.capacity; i++) {
        testrun(msg->header[i].name.start == NULL);
        testrun(msg->header[i].name.length == 0);
        testrun(msg->header[i].value.start == NULL);
        testrun(msg->header[i].value.length == 0);
    }
    testrun(msg->buffer->capacity == msg->config.buffer.default_size);
    testrun(msg->buffer->length == 0);
    msg = ov_sip_message_free(msg);

    ov_sip_message_config config = { .header.capacity = 10,
                                     .buffer.default_size = 100};

    msg = ov_sip_message_create(config);
    testrun(ov_sip_message_cast(msg));
    testrun(msg->buffer);
    testrun(msg->config.header.capacity == 10);
    testrun(msg->config.buffer.default_size == 100);
    testrun(msg->buffer->capacity == 100);
    msg = ov_sip_message_free(msg);

    // check with caching
    ov_sip_enable_caching(1);
    testrun(g_cache != NULL);

    msg = ov_sip_message_create((ov_sip_message_config){0});
    testrun(msg);
    testrun(ov_sip_message_cast(msg));
    ov_sip_message *msg2 = ov_sip_message_create((ov_sip_message_config){0});
    testrun(msg2);
    testrun(ov_sip_message_cast(msg2));

    ov_sip_message_free(msg);
    ov_sip_message *msg3 = ov_sip_message_create((ov_sip_message_config){0});
    testrun(msg3);
    testrun(ov_sip_message_cast(msg3));
    testrun(msg3 == msg);

    ov_sip_message_free(msg2);
    ov_sip_message *msg4 = ov_sip_message_create((ov_sip_message_config){0});
    testrun(msg4);
    testrun(ov_sip_message_cast(msg4));
    testrun(msg4 == msg2);

    ov_sip_message_free(msg3);
    ov_sip_message_free(msg4);

    // we check what we get from cache
    msg3 = ov_registered_cache_get(g_cache);
    testrun(msg3);
    testrun(msg == msg3);
    msg4 = ov_registered_cache_get(g_cache);
    testrun(!msg4);

    msg3 = ov_sip_message_free_uncached(msg3);
    testrun(!ov_registered_cache_get(g_cache));

    msg = NULL;
    msg2 = NULL;
    msg3 = NULL;
    msg4 = NULL;

    config = (ov_sip_message_config){ .header.capacity = 10,
                                      .buffer.default_size = 100};

    msg = ov_sip_message_create(config);
    testrun(msg);
    testrun(msg->config.header.capacity == 10);
    ov_sip_message_free(msg);

    // we request a message with a bigger header size
    msg2 = ov_sip_message_create(
        (ov_sip_message_config){.header.capacity = 100});
    testrun(msg2 != msg);
    // msg will point to the message wihin the cache
    testrun(msg->config.header.capacity == 10);
    testrun(msg2->config.header.capacity == 100);

    // we request a message with the same header size
    msg3 = ov_sip_message_create(config);
    testrun(msg);
    testrun(msg->config.header.capacity == 10);
    testrun(msg == msg3);

    // order for free is import! (cache size 1)
    ov_sip_message_free(msg3);
    ov_sip_message_free(msg2);

    // we request a bigger buffer size
    msg4 = ov_sip_message_create((ov_sip_message_config){
        .header.capacity = 10, .buffer.default_size = 1000});
    testrun(msg4);
    testrun(msg == msg4);
    testrun(msg4->buffer->capacity == 1000);

    // we request a smaller buffer size
    ov_sip_message_free(msg);
    msg4 = ov_sip_message_create((ov_sip_message_config){
        .header.capacity = 10, .buffer.default_size = 100});
    testrun(msg4);
    testrun(msg == msg4);
    testrun(msg4->buffer->capacity == 1000);

    // we request a smaller buffer size and bigger header size
    // this will calloc a new msg
    ov_sip_message_free(msg);
    msg4 = ov_sip_message_create((ov_sip_message_config){
        .header.capacity = 100, .buffer.default_size = 100});
    testrun(msg4);
    testrun(msg != msg4);
    testrun(msg4->buffer->capacity == 100);
    msg4 = ov_sip_message_free(msg4);

    ov_registered_cache_free_all();
    g_cache = NULL;

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sip_message_clear() {

    ov_sip_message *msg = ov_sip_message_create((ov_sip_message_config){0});
    testrun(msg);
    testrun(ov_sip_message_cast(msg));
    testrun(msg->buffer);
    testrun(msg->config.header.capacity == IMPL_DEFAULT_HEADER_CAPACITY);
    testrun(msg->config.buffer.default_size == IMPL_DEFAULT_BUFFER_SIZE);
    testrun(msg->body.start == NULL);
    testrun(msg->body.length == 0);
    testrun(msg->version.major == 0);
    testrun(msg->version.minor == 0);
    testrun(msg->request.method.start == NULL);
    testrun(msg->request.method.length == 0);
    testrun(msg->request.uri.start == NULL);
    testrun(msg->request.uri.length == 0);
    testrun(msg->status.phrase.start == NULL);
    testrun(msg->status.phrase.length == 0);
    testrun(msg->status.code == 0);
    for (size_t i = 0; i < msg->config.header.capacity; i++) {
        testrun(msg->header[i].name.start == NULL);
        testrun(msg->header[i].name.length == 0);
        testrun(msg->header[i].value.start == NULL);
        testrun(msg->header[i].value.length == 0);
    }
    testrun(msg->buffer->capacity == msg->config.buffer.default_size);
    testrun(msg->buffer->length == 0);

    char *string = "GET / SIP/1.1\r\nfield:value\r\n\r\n";
    testrun(ov_buffer_set(msg->buffer, string, strlen(string)));
    testrun(OV_SIP_PARSER_SUCCESS == ov_sip_parse_message(msg, NULL));

    testrun(msg->body.start == NULL);
    testrun(msg->version.major == 1);
    testrun(msg->version.minor == 1);
    testrun(0 == strncmp("GET", (char *)msg->request.method.start,
                         msg->request.method.length));
    testrun(0 == strncmp("/", (char *)msg->request.uri.start,
                         msg->request.uri.length));
    testrun(msg->status.phrase.start == NULL);
    testrun(msg->status.phrase.length == 0);
    testrun(msg->status.code == 0);
    testrun(0 == strncmp("field", (char *)msg->header[0].name.start,
                         msg->header[0].name.length));
    testrun(0 == strncmp("value", (char *)msg->header[0].value.start,
                         msg->header[0].value.length));
    testrun(0 == msg->header[1].name.start);
    testrun(0 == msg->header[1].value.start);
    testrun(msg->buffer->capacity == msg->config.buffer.default_size);
    testrun(msg->buffer->length == strlen(string));

    testrun(!ov_sip_message_clear(NULL));

    testrun(ov_sip_message_clear(msg));
    testrun(ov_sip_message_cast(msg));
    testrun(msg->buffer);
    testrun(msg->config.header.capacity == IMPL_DEFAULT_HEADER_CAPACITY);
    testrun(msg->config.buffer.default_size == IMPL_DEFAULT_BUFFER_SIZE);
    testrun(msg->body.start == NULL);
    testrun(msg->body.length == 0);
    testrun(msg->version.major == 0);
    testrun(msg->version.minor == 0);
    testrun(msg->request.method.start == NULL);
    testrun(msg->request.method.length == 0);
    testrun(msg->request.uri.start == NULL);
    testrun(msg->request.uri.length == 0);
    testrun(msg->status.phrase.start == NULL);
    testrun(msg->status.phrase.length == 0);
    testrun(msg->status.code == 0);
    for (size_t i = 0; i < msg->config.header.capacity; i++) {
        testrun(msg->header[i].name.start == NULL);
        testrun(msg->header[i].name.length == 0);
        testrun(msg->header[i].value.start == NULL);
        testrun(msg->header[i].value.length == 0);
    }
    testrun(msg->buffer->capacity == msg->config.buffer.default_size);
    testrun(msg->buffer->length == 0);

    msg = ov_sip_message_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sip_message_free() {

    ov_sip_message *msg = ov_sip_message_create((ov_sip_message_config){0});
    testrun(msg);

    testrun(NULL == ov_sip_message_free(NULL));
    testrun(NULL == ov_sip_message_free(msg));

    msg = ov_sip_message_create((ov_sip_message_config){0});

    testrun(g_cache == NULL);
    ov_sip_enable_caching(1);
    testrun(g_cache != NULL);

    testrun(NULL == ov_sip_message_free(msg));

    ov_sip_message *msg2 = ov_registered_cache_get(g_cache);
    testrun(msg == msg2);

    testrun(NULL == ov_sip_message_free(msg2));
    msg2 = ov_sip_message_create((ov_sip_message_config){0});
    testrun(msg == msg2);
    testrun(NULL == ov_registered_cache_get(g_cache));

    testrun(NULL == ov_sip_message_free(msg2));

    ov_registered_cache_free_all();
    g_cache = NULL;

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sip_message_free_uncached() {

    ov_sip_message *msg = ov_sip_message_create((ov_sip_message_config){0});
    testrun(msg);

    testrun(NULL == ov_sip_message_free_uncached(NULL));
    testrun(NULL == ov_sip_message_free_uncached(msg));

    msg = ov_sip_message_create((ov_sip_message_config){0});

    testrun(g_cache == NULL);
    ov_sip_enable_caching(1);
    testrun(g_cache != NULL);

    testrun(NULL == ov_sip_message_free_uncached(msg));
    testrun(NULL == ov_registered_cache_get(g_cache));

    ov_registered_cache_free_all();
    g_cache = NULL;

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sip_enable_caching() {

    ov_sip_message *msg = ov_sip_message_create((ov_sip_message_config){0});
    testrun(msg);

    testrun(g_cache == NULL);
    ov_sip_enable_caching(1);
    testrun(g_cache != NULL);

    testrun(NULL == ov_sip_message_free(msg));

    ov_sip_message *msg2 = ov_registered_cache_get(g_cache);
    testrun(msg == msg2);

    ov_registered_cache_free_all();
    g_cache = NULL;

    ov_sip_message_free(msg2);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sip_message_config_init() {

    ov_sip_message_config config =
        ov_sip_message_config_init((ov_sip_message_config){0});

    testrun(config.header.capacity == IMPL_DEFAULT_HEADER_CAPACITY);
    testrun(config.header.max_bytes_method_name ==
            IMPL_DEFAULT_MAX_METHOD_NAME);
    testrun(config.header.max_bytes_line == IMPL_DEFAULT_MAX_HEADER_LINE);
    testrun(config.buffer.default_size == IMPL_DEFAULT_BUFFER_SIZE);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sip_parse_message_buffer() {

    ov_sip_message *out = NULL;

    char * start =  "INVITE sip:+14155552222@example.pstn.twilio.com SIP/2.0\r\n"
                    "Via: SIP/2.0/UDP 192.168.10.10:5060;branch=z9hG4bK776asdhds\r\n"
                    "Max-Forwards: 70\r\n"
                    "To: \"Bob\" <sip:+14155552222@example.pstn.twilio.com>\r\n"
                    "From: \"Alice\" <sip:+14155551111@example.pstn.twilio.com>;tag=1\r\n"
                    "Call-ID: a84b4c76e66710\r\n"
                    "CSeq: 1 INVITE\r\n"
                    "Contact: \"Alice\" <sip:+14155551111@192.168.10.10:5060>\r\n"
                    "Diversion: \"Sales\" <sip:+14155550000@example.pstn.twilio.com>\r\n"
                    "P-Asserted-Identity: \"Alice\" <sip:+14155551111@example.pstn.twilio.com>\r\n"
                    "Content-Length: 0\r\n"
                    "\r\n"
                    "NEXT";

    size_t len = strlen(start);
    uint8_t *next = NULL;

    ov_sip_parser_state state = ov_sip_parse_message_buffer(
        (uint8_t*) start, len, &next, &out);

    testrun(state == OV_SIP_PARSER_SUCCESS);
    testrun(out);
    testrun(next[0] == 'N');
    testrun(next == (uint8_t*) start + out->buffer->length);

    out = ov_sip_message_free(out);

    return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

    testrun_init();

    testrun_test(test_ov_sip_message_cast);
    testrun_test(test_ov_sip_message_create);
    testrun_test(test_ov_sip_message_clear);
    testrun_test(test_ov_sip_message_free);
    testrun_test(test_ov_sip_message_free_uncached);
    testrun_test(test_ov_sip_enable_caching);
    testrun_test(test_ov_sip_message_config_init);

    testrun_test(test_ov_sip_parse_message_buffer);

    return testrun_counter;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST EXECUTION                                                  #EXEC
 *
 *      ------------------------------------------------------------------------
 */

testrun_run(all_tests);
