/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_websocket_message_test.c
        @author         Markus Töpfer

        @date           2021-01-12


        ------------------------------------------------------------------------
*/
#include "ov_websocket_message.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_websocket_upgrade_request() {

    uint8_t key[OV_WEBSOCKET_SECURE_KEY_SIZE + 1];
    memset(key, 0, OV_WEBSOCKET_SECURE_KEY_SIZE + 1);

    testrun(ov_websocket_generate_secure_websocket_key(
        key, OV_WEBSOCKET_SECURE_KEY_SIZE));

    ov_memory_pointer sec_key = {0};

    ov_http_message *msg = NULL;

    const char *host = "host";
    const char *uri = "uri";
    const ov_http_header *header = NULL;

    testrun(!ov_websocket_upgrade_request(NULL, NULL, sec_key));
    testrun(!ov_websocket_upgrade_request(host, uri, sec_key));
    sec_key.start = key;
    sec_key.length = OV_WEBSOCKET_SECURE_KEY_SIZE;
    testrun(!ov_websocket_upgrade_request(NULL, uri, sec_key));
    testrun(!ov_websocket_upgrade_request(host, NULL, sec_key));

    msg = ov_websocket_upgrade_request(host, uri, sec_key);
    testrun(msg);
    header = ov_http_header_get_unique(
        msg->header, msg->config.header.capacity, OV_HTTP_KEY_UPGRADE);
    testrun(header);
    testrun(0 == memcmp(OV_WEBSOCKET_KEY,
                        header->value.start,
                        header->value.length));

    header = ov_http_header_get_unique(
        msg->header, msg->config.header.capacity, OV_HTTP_KEY_CONNECTION);
    testrun(header);
    testrun(0 == memcmp(OV_WEBSOCKET_KEY_UPGRADE,
                        header->value.start,
                        header->value.length));

    header = ov_http_header_get_unique(
        msg->header, msg->config.header.capacity, OV_HTTP_KEY_HOST);
    testrun(header);
    testrun(0 == memcmp(host, header->value.start, header->value.length));

    header = ov_http_header_get_unique(
        msg->header, msg->config.header.capacity, OV_WEBSOCKET_KEY_SECURE);
    testrun(header);
    testrun(0 == memcmp(key, header->value.start, header->value.length));

    header = ov_http_header_get_unique(msg->header,
                                       msg->config.header.capacity,
                                       OV_WEBSOCKET_KEY_SECURE_VERSION);
    testrun(header);
    testrun(0 == memcmp(OV_WEBSOCKET_VERSION,
                        header->value.start,
                        header->value.length));

    testrun(NULL == ov_http_message_free(msg));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_websocket_is_upgrade_response() {

    uint8_t key[OV_WEBSOCKET_SECURE_KEY_SIZE + 1];
    memset(key, 0, OV_WEBSOCKET_SECURE_KEY_SIZE + 1);

    uint8_t *accept_key = NULL;
    size_t accept_key_length = 0;

    testrun(ov_websocket_generate_secure_websocket_key(
        key, OV_WEBSOCKET_SECURE_KEY_SIZE));

    testrun(ov_websocket_generate_secure_accept_key(
        key, OV_WEBSOCKET_SECURE_KEY_SIZE, &accept_key, &accept_key_length));

    ov_memory_pointer sec_key = {
        .start = key, .length = OV_WEBSOCKET_SECURE_KEY_SIZE};

    ov_http_message *msg =
        ov_http_create_status_string((ov_http_message_config){0},
                                     (ov_http_version){.major = 1, .minor = 1},
                                     101,
                                     OV_HTTP_SWITCH_PROTOCOLS);
    testrun(msg);

    testrun(ov_http_message_add_header_string(
        msg, OV_HTTP_KEY_UPGRADE, OV_WEBSOCKET_KEY));

    testrun(ov_http_message_add_header_string(
        msg, OV_HTTP_KEY_CONNECTION, OV_HTTP_KEY_UPGRADE));

    testrun(ov_http_message_add_header_string(
        msg, OV_WEBSOCKET_KEY_SECURE_ACCEPT, (char *)accept_key));

    testrun(ov_http_message_close_header(msg));
    testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_message(msg, NULL));

    testrun(ov_websocket_is_upgrade_response(msg, sec_key));

    // change sec key
    testrun(ov_websocket_generate_secure_websocket_key(
        key, OV_WEBSOCKET_SECURE_KEY_SIZE));

    testrun(!ov_websocket_is_upgrade_response(msg, sec_key));

    accept_key = ov_data_pointer_free(accept_key);
    testrun(NULL == ov_http_message_free(msg));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

    testrun_init();
    testrun_test(test_ov_websocket_upgrade_request);
    testrun_test(test_ov_websocket_is_upgrade_response);

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
