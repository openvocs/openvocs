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
        @file           ov_websocket_message.c
        @author         Markus Töpfer

        @date           2021-01-12


        ------------------------------------------------------------------------
*/
#include "../include/ov_websocket_message.h"

ov_http_message *ov_websocket_upgrade_request(const char *host,
                                              const char *uri,
                                              ov_memory_pointer sec_key) {

    ov_http_message *msg = NULL;

    if (!uri || !host || !sec_key.start) goto error;

    msg =
        ov_http_create_request_string((ov_http_message_config){0},
                                      (ov_http_version){.major = 1, .minor = 1},
                                      OV_HTTP_METHOD_GET,
                                      uri);

    if (!msg) goto error;

    if (!ov_http_message_add_header_string(
            msg, OV_HTTP_KEY_UPGRADE, OV_WEBSOCKET_KEY))
        goto error;

    if (!ov_http_message_add_header_string(
            msg, OV_HTTP_KEY_CONNECTION, OV_WEBSOCKET_KEY_UPGRADE))
        goto error;

    if (!ov_http_message_add_header_string(msg, OV_HTTP_KEY_HOST, host))
        goto error;

    if (!ov_http_message_add_header(
            msg,
            (ov_http_header){

                .name =
                    (ov_memory_pointer){
                        .start = (uint8_t *)OV_WEBSOCKET_KEY_SECURE,
                        .length = strlen(OV_WEBSOCKET_KEY_SECURE)},

                .value = sec_key}))
        goto error;

    if (!ov_http_message_add_header_string(
            msg, OV_WEBSOCKET_KEY_SECURE_VERSION, OV_WEBSOCKET_VERSION))
        goto error;

    if (!ov_http_message_close_header(msg)) goto error;

    if (OV_HTTP_PARSER_SUCCESS != ov_http_pointer_parse_message(msg, NULL))
        goto error;

    return msg;
error:
    ov_http_message_free(msg);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_websocket_is_upgrade_response(const ov_http_message *msg,
                                      ov_memory_pointer sec_key) {

    uint8_t *accept_key = NULL;
    size_t accept_key_length = 0;

    if (!msg || !sec_key.start) goto error;

    if (msg->status.code != 101) goto error;

    const ov_http_header *header = ov_http_header_get_unique(
        msg->header, msg->config.header.capacity, OV_HTTP_KEY_UPGRADE);

    if (!header) goto error;

    if (header->value.length != strlen(OV_WEBSOCKET_KEY)) goto error;

    if (0 !=
        memcmp(OV_WEBSOCKET_KEY, header->value.start, header->value.length))
        goto error;

    header = ov_http_header_get_unique(
        msg->header, msg->config.header.capacity, OV_HTTP_KEY_CONNECTION);

    if (!header) goto error;

    if (header->value.length != strlen(OV_HTTP_KEY_UPGRADE)) goto error;

    if (0 !=
        memcmp(OV_HTTP_KEY_UPGRADE, header->value.start, header->value.length))
        goto error;

    header = ov_http_header_get_unique(msg->header,
                                       msg->config.header.capacity,
                                       OV_WEBSOCKET_KEY_SECURE_ACCEPT);

    if (!header) goto error;

    if (!ov_websocket_generate_secure_accept_key(
            sec_key.start, sec_key.length, &accept_key, &accept_key_length))
        goto error;

    if (accept_key_length != header->value.length) goto error;

    if (0 != memcmp(accept_key, header->value.start, header->value.length))
        goto error;

    ov_data_pointer_free(accept_key);
    return true;
error:
    ov_data_pointer_free(accept_key);
    return false;
}
