/***
        ------------------------------------------------------------------------

        Copyright (c) 2025 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the opendtn project. https://opendtn.com

        ------------------------------------------------------------------------
*//**
        @file           ov_webserver_test.c
        @author         Töpfer, Markus

        @date           2025-12-17


        ------------------------------------------------------------------------
*/
#include "ov_webserver.c"
#include <ov_test/testrun.h>

#include "../include/ov_event_api.h"
#include "../include/ov_websocket_message.h"
#include <ov_base/ov_dump.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#ifndef OV_TEST_CERT
#error "Must provide -D OV_TEST_CERT=value while compiling this file."
#endif

#ifndef OV_TEST_CERT_KEY
#error "Must provide -D OV_TEST_CERT_KEY=value while compiling this file."
#endif

#ifndef OV_TEST_CERT_ONE
#error "Must provide -D OV_TEST_CERT_ONE=value while compiling this file."
#endif

#ifndef OV_TEST_CERT_ONE_KEY
#error "Must provide -D OV_TEST_CERT_ONE_KEY=value while compiling this file."
#endif

#ifndef OV_TEST_CERT_TWO
#error "Must provide -D OV_TEST_CERT_TWO=value while compiling this file."
#endif

#ifndef OV_TEST_CERT_TWO_KEY
#error "Must provide -D OV_TEST_CERT_TWO_KEY=value while compiling this file."
#endif

#define TEST_RUNTIME_USECS 50 * 1000

#define TEST_DOMAIN_NAME "openvocs.test"
#define TEST_DOMAIN_NAME_ONE "one.test"
#define TEST_DOMAIN_NAME_TWO "two.test"

static const char *test_resource_dir = 0;
static const char *domain_config_file = 0;
static const char *domain_config_file_one = 0;
static const char *domain_config_file_two = 0;

/*----------------------------------------------------------------------------*/

int domains_deinit() {

    testrun(0 != domain_config_file);

    unlink(domain_config_file);
    domain_config_file = 0;

    testrun(0 != domain_config_file_one);
    unlink(domain_config_file_one);
    domain_config_file_one = 0;

    testrun(0 != domain_config_file_two);
    unlink(domain_config_file_two);
    domain_config_file_two = 0;

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int domains_init() {

    test_resource_dir = OV_TEST_RESOURCE_DIR;

    domain_config_file = OV_TEST_RESOURCE_DIR "/" TEST_DOMAIN_NAME;
    domain_config_file_one = OV_TEST_RESOURCE_DIR "/" TEST_DOMAIN_NAME_ONE;
    domain_config_file_two = OV_TEST_RESOURCE_DIR "/" TEST_DOMAIN_NAME_TWO;

    /* Delete possibly remaining files from previous test run */
    domains_deinit();

    /* Since strings have been freed, reinit */
    test_resource_dir = OV_TEST_RESOURCE_DIR;

    domain_config_file = OV_TEST_RESOURCE_DIR "/" TEST_DOMAIN_NAME;
    domain_config_file_one = OV_TEST_RESOURCE_DIR "/" TEST_DOMAIN_NAME_ONE;
    domain_config_file_two = OV_TEST_RESOURCE_DIR "/" TEST_DOMAIN_NAME_TWO;

    ov_json_value *conf = ov_json_object();
    ov_json_value *cert = ov_json_object();
    testrun(ov_json_object_set(conf, "certificate", cert));

    ov_json_value *val = NULL;
    val = ov_json_string(TEST_DOMAIN_NAME);
    testrun(ov_json_object_set(conf, "name", val));
    val = ov_json_string(test_resource_dir);
    testrun(ov_json_object_set(conf, "path", val));
    val = ov_json_string(OV_TEST_CERT);
    testrun(ov_json_object_set(cert, "file", val));
    val = ov_json_string(OV_TEST_CERT_KEY);
    testrun(ov_json_object_set(cert, "key", val));
    testrun(ov_json_write_file(domain_config_file, conf));

    val = ov_json_string(TEST_DOMAIN_NAME_ONE);
    testrun(ov_json_object_set(conf, "name", val));
    val = ov_json_string(test_resource_dir);
    testrun(ov_json_object_set(conf, "path", val));
    val = ov_json_string(OV_TEST_CERT_ONE);
    testrun(ov_json_object_set(cert, "file", val));
    val = ov_json_string(OV_TEST_CERT_ONE_KEY);
    testrun(ov_json_object_set(cert, "key", val));
    testrun(ov_json_write_file(domain_config_file_one, conf));

    val = ov_json_string(TEST_DOMAIN_NAME_TWO);
    testrun(ov_json_object_set(conf, "name", val));
    val = ov_json_string(test_resource_dir);
    testrun(ov_json_object_set(conf, "path", val));
    val = ov_json_string(OV_TEST_CERT_TWO);
    testrun(ov_json_object_set(cert, "file", val));
    val = ov_json_string(OV_TEST_CERT_TWO_KEY);
    testrun(ov_json_object_set(cert, "key", val));
    testrun(ov_json_write_file(domain_config_file_two, conf));

    conf = ov_json_value_free(conf);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_webserver_create() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});
    testrun(loop);

    // server is running on TLS only, so we need some TLS IO config
    ov_io_config io_config = (ov_io_config){.loop = loop};
    strncpy(io_config.domain.path, test_resource_dir, PATH_MAX);

    ov_io *io = ov_io_create(io_config);
    testrun(io);

    ov_webserver_config config = (ov_webserver_config){
        .loop = loop,
        .io = io,
        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "localhost", .type = TCP})};

    ov_webserver *self = ov_webserver_create(config);

    testrun(self);
    testrun(self->connections);
    testrun(self->domains);
    testrun(self->callbacks);
    testrun(self->json_io_buffer);

    testrun(NULL == ov_io_free(io));
    testrun(NULL == ov_webserver_free(self));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_free() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});
    testrun(loop);

    // server is running on TLS only, so we need some TLS IO config
    ov_io_config io_config = (ov_io_config){.loop = loop};
    strncpy(io_config.domain.path, test_resource_dir, PATH_MAX);

    ov_io *io = ov_io_create(io_config);
    testrun(io);

    ov_webserver_config config = (ov_webserver_config){
        .loop = loop,
        .io = io,
        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "localhost", .type = TCP})};

    ov_webserver *self = ov_webserver_create(config);

    testrun(NULL == ov_io_free(io));
    testrun(NULL == ov_webserver_free(self));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_set_debug() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});
    testrun(loop);

    // server is running on TLS only, so we need some TLS IO config
    ov_io_config io_config = (ov_io_config){.loop = loop};
    strncpy(io_config.domain.path, test_resource_dir, PATH_MAX);

    ov_io *io = ov_io_create(io_config);
    testrun(io);

    ov_webserver_config config = (ov_webserver_config){
        .loop = loop,
        .io = io,
        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "localhost", .type = TCP})};

    ov_webserver *self = ov_webserver_create(config);

    testrun(ov_webserver_set_debug(self, true));
    testrun(self->debug == true);
    testrun(ov_webserver_set_debug(self, false));
    testrun(self->debug == false);

    testrun(NULL == ov_io_free(io));
    testrun(NULL == ov_webserver_free(self));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

struct dummy {

    int socket;
    ov_json_value *item;
};

/*----------------------------------------------------------------------------*/

static void dummy_callback(void *userdata, int socket, ov_json_value *msg) {

    struct dummy *dummy = (struct dummy *)userdata;
    dummy->socket = socket;
    dummy->item = msg;
    return;
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_enable_callback() {

    struct dummy dummy = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});
    testrun(loop);

    // server is running on TLS only, so we need some TLS IO config
    ov_io_config io_config = (ov_io_config){.loop = loop};
    strncpy(io_config.domain.path, test_resource_dir, PATH_MAX);

    ov_io *io = ov_io_create(io_config);
    testrun(io);

    ov_webserver_config config = (ov_webserver_config){
        .loop = loop,
        .io = io,
        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "localhost", .type = TCP})};

    ov_webserver *self = ov_webserver_create(config);

    testrun(0 == ov_dict_count(self->callbacks));
    testrun(ov_webserver_enable_callback(self, "one", &dummy, dummy_callback));
    testrun(1 == ov_dict_count(self->callbacks));

    testrun(NULL == ov_io_free(io));
    testrun(NULL == ov_webserver_free(self));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_enable_domains() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});
    testrun(loop);

    // server is running on TLS only, so we need some TLS IO config
    ov_io_config io_config = (ov_io_config){.loop = loop};
    strncpy(io_config.domain.path, test_resource_dir, PATH_MAX);

    ov_io *io = ov_io_create(io_config);
    testrun(io);

    ov_webserver_config config = (ov_webserver_config){
        .loop = loop,
        .io = io,
        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "localhost", .type = TCP})};

    ov_webserver *self = ov_webserver_create(config);

    ov_json_value *overall_config = ov_json_object();
    ov_json_value *web_server_config = ov_json_object();
    ov_json_value *domain_config = ov_json_array();
    testrun(
        ov_json_object_set(overall_config, "webserver", web_server_config));
    testrun(ov_json_object_set(web_server_config, "domains", domain_config));
    ov_json_value *domain1 = ov_json_object();
    ov_json_value *domain2 = ov_json_object();
    testrun(ov_json_array_push(domain_config, domain1));
    testrun(ov_json_array_push(domain_config, domain2));
    testrun(ov_json_object_set(domain1, "name", ov_json_string("name1")));
    testrun(ov_json_object_set(domain1, "path", ov_json_string("path1")));
    testrun(ov_json_object_set(domain2, "name", ov_json_string("name2")));
    testrun(ov_json_object_set(domain2, "path", ov_json_string("path2")));

    testrun(ov_webserver_enable_domains(self, overall_config));
    testrun(2 == ov_dict_count(self->domains));
    testrun(NULL == ov_json_value_free(overall_config));

    testrun(NULL == ov_io_free(io));
    testrun(NULL == ov_webserver_free(self));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int run_client_handshake(ov_event_loop *loop, SSL *ssl, int *err,
                                int *errorcode) {

    testrun(loop);
    testrun(ssl);
    testrun(err);
    testrun(errorcode);

    int r = 0;
    int n = 0;

    bool run = true;
    while (run) {

        loop->run(loop, TEST_RUNTIME_USECS);

        n = 0;
        r = SSL_connect(ssl);

        switch (r) {

        case 1:
            /* SUCCESS */
            run = false;
            break;

        default:

            n = SSL_get_error(ssl, r);

            switch (n) {

            case SSL_ERROR_NONE:
                /* SHOULD not be returned in 0 */
                break;

            case SSL_ERROR_ZERO_RETURN:
                /* close */
                run = false;
                break;

            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_CONNECT:
            case SSL_ERROR_WANT_X509_LOOKUP:
            case SSL_ERROR_WANT_ASYNC:
            case SSL_ERROR_WANT_ASYNC_JOB:
            case SSL_ERROR_WANT_CLIENT_HELLO_CB:
                /* try async again */
                break;

            case SSL_ERROR_SYSCALL:
                /* nonrecoverable IO error */
                *errorcode = ERR_get_error();
                run = false;
                break;

            case SSL_ERROR_SSL:
                *errorcode = ERR_get_error();
                run = false;
                break;

            case SSL_ERROR_WANT_ACCEPT:
                run = false;
                break;
            }
            break;
        }

        // fprintf(stdout, "\nr %i n %i\n", r, n);
    }

    *err = n;
    return r;
}

/*----------------------------------------------------------------------------*/

static int dummy_client_hello_cb(SSL *s, int *al, void *arg) {

    /*
     *      At some point @perform_ssl_client_handshake
     *      may stop with SSL_ERROR_WANT_CLIENT_HELLO_CB,
     *      so we add some dummy callback, to resume
     *      standard SSL operation.
     */
    if (!s)
        return SSL_CLIENT_HELLO_ERROR;

    if (al || arg) { /* ignored */
    };
    return SSL_CLIENT_HELLO_SUCCESS;
}

/*----------------------------------------------------------------------------*/

int check_websocket() {

    struct dummy dummy = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});
    testrun(loop);

    // server is running on TLS only, so we need some TLS IO config
    ov_io_config io_config = (ov_io_config){.loop = loop};
    strncpy(io_config.domain.path, test_resource_dir, PATH_MAX);

    ov_io *io = ov_io_create(io_config);
    testrun(io);

    ov_webserver_config config = (ov_webserver_config){
        .loop = loop,
        .io = io,
        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "localhost", .type = TCP})};

    ov_webserver *self = ov_webserver_create(config);
    testrun(ov_webserver_set_debug(self, true));

    int client = ov_socket_create(config.socket, true, NULL);
    testrun(client > 0);
    testrun(ov_socket_ensure_nonblocking(client));

    ov_event_loop_run(loop, OV_RUN_ONCE);

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL *ssl = SSL_new(ctx);
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);

    int errorcode = -1;
    int err = 0;
    int r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // SSL handshake done, client connected

    uint8_t secure_key[1024] = {0};
    testrun(ov_websocket_generate_secure_websocket_key(secure_key, 1024));

    ov_http_message *upgrade = ov_websocket_upgrade_request(
        "one", "/",
        (ov_memory_pointer){.start = secure_key,
                             .length = strlen((char *)secure_key)});
    testrun(upgrade);

    ssize_t bytes =
        SSL_write(ssl, upgrade->buffer->start, upgrade->buffer->length);
    testrun(bytes == (int64_t)upgrade->buffer->length);
    upgrade = ov_http_message_free(upgrade);

    uint8_t buffer[2028] = {0};

    bytes = -1;

    while (-1 == bytes) {

        bytes = SSL_read(ssl, buffer, 2048);
        ov_event_loop_run(loop, OV_RUN_ONCE);
    }

    ov_http_message_config http_config =
        ov_http_message_config_init((ov_http_message_config){0});

    upgrade = ov_http_message_create(http_config);
    testrun(upgrade);
    memcpy(upgrade->buffer->start, buffer, bytes);
    upgrade->buffer->length = bytes;
    uint8_t *next = NULL;

    testrun(OV_HTTP_PARSER_SUCCESS ==
            ov_http_pointer_parse_message(upgrade, &next));
    testrun(ov_websocket_is_upgrade_response(
        upgrade, (ov_memory_pointer){.start = secure_key,
                                      .length = strlen((char *)secure_key)}));

    upgrade = ov_http_message_free(upgrade);

    // websocket connection enabled

    ov_websocket_frame *frame =
        ov_websocket_frame_create((ov_websocket_frame_config){0});
    testrun(frame);

    frame->buffer->start[0] = 0x80 | OV_WEBSOCKET_OPCODE_PING;
    frame->buffer->start[1] = 0x00;

    bytes = -1;

    while (-1 == bytes) {

        bytes = SSL_write(ssl, frame->buffer->start, 2);
        ov_event_loop_run(loop, OV_RUN_ONCE);
    }

    ov_event_loop_run(loop, OV_RUN_ONCE);

    bytes = -1;
    memset(buffer, 0, 2028);
    while (-1 == bytes) {

        bytes = SSL_read(ssl, buffer, 2028);
        ov_event_loop_run(loop, OV_RUN_ONCE);
    }

    // check pong received
    // ov_dump_binary_as_hex(stdout, (uint8_t*)buffer, 10);
    testrun(2 == bytes);
    testrun(0x8a == buffer[0]);
    testrun(0x00 == buffer[1]);

    // check websocket reception with JSON messages

    testrun(ov_webserver_enable_callback(self, "openvocs.test", &dummy,
                                          dummy_callback));

    ov_json_value *json = ov_json_object();
    testrun(ov_json_object_set(json, "event", ov_json_string("test")));
    char *string = ov_json_value_to_string(json);
    testrun(string);

    frame->buffer->start[0] =
        0x80 | OV_WEBSOCKET_FRAGMENTATION_NONE | OV_WEBSOCKET_OPCODE_TEXT;
    testrun(
        ov_websocket_set_data(frame, (uint8_t *)string, strlen(string), true));

    json = ov_json_value_free(json);
    string = ov_data_pointer_free(string);

    bytes = -1;
    while (-1 == bytes) {

        bytes = SSL_write(ssl, frame->buffer->start, frame->buffer->length);
        ov_event_loop_run(loop, OV_RUN_ONCE);
    }

    testrun(dummy.socket > 0);
    testrun(dummy.item);
    testrun(0 == ov_string_compare("test", ov_event_api_get_event(dummy.item)));
    dummy.item = ov_json_value_free(dummy.item);

    // check incomplete JSON delivery

    json = ov_json_object();
    testrun(ov_json_object_set(json, "event", ov_json_string("test")));
    string = ov_json_value_to_string(json);
    testrun(string);

    frame->buffer->start[0] =
        0x80 | OV_WEBSOCKET_FRAGMENTATION_NONE | OV_WEBSOCKET_OPCODE_TEXT;
    testrun(ov_websocket_set_data(frame, (uint8_t *)string, 5, true));

    bytes = -1;
    while (-1 == bytes) {

        bytes = SSL_write(ssl, frame->buffer->start, frame->buffer->length);
        ov_event_loop_run(loop, OV_RUN_ONCE);
    }

    testrun(!dummy.item);

    frame->buffer->start[0] =
        0x80 | OV_WEBSOCKET_FRAGMENTATION_NONE | OV_WEBSOCKET_OPCODE_TEXT;
    testrun(ov_websocket_set_data(frame, (uint8_t *)string + 5,
                                   strlen(string) - 5, true));

    dummy.socket = 0;

    bytes = -1;
    while (-1 == bytes) {

        bytes = SSL_write(ssl, frame->buffer->start, frame->buffer->length);
        ov_event_loop_run(loop, OV_RUN_ONCE);
    }

    // we need to run the loop a while after writing (dont know exactely why)
    while (dummy.socket == 0) {

        ov_event_loop_run(loop, OV_RUN_ONCE);
    }

    testrun(dummy.socket > 0);
    testrun(dummy.item);
    testrun(0 == ov_string_compare("test", ov_event_api_get_event(dummy.item)));
    dummy.item = ov_json_value_free(dummy.item);

    // check fragmented websocket delivery

    frame->buffer->start[0] = 0x00 | OV_WEBSOCKET_OPCODE_TEXT;
    testrun(ov_websocket_set_data(frame, (uint8_t *)string, 4, true));

    bytes = -1;
    while (-1 == bytes) {

        bytes = SSL_write(ssl, frame->buffer->start, frame->buffer->length);
        ov_event_loop_run(loop, OV_RUN_ONCE);
    }

    testrun(!dummy.item);

    testrun(ov_websocket_frame_clear(frame));
    frame->buffer->start[0] = 0x00;
    testrun(ov_websocket_set_data(frame, (uint8_t *)string + 4, 4, true));

    bytes = -1;
    while (-1 == bytes) {

        bytes = SSL_write(ssl, frame->buffer->start, frame->buffer->length);
        ov_event_loop_run(loop, OV_RUN_ONCE);
    }

    testrun(!dummy.item);

    testrun(ov_websocket_frame_clear(frame));
    frame->buffer->start[0] = 0x00;
    testrun(ov_websocket_set_data(frame, (uint8_t *)string + 8, 4, true));

    bytes = -1;
    while (-1 == bytes) {

        bytes = SSL_write(ssl, frame->buffer->start, frame->buffer->length);
        ov_event_loop_run(loop, OV_RUN_ONCE);
    }

    testrun(!dummy.item);

    testrun(ov_websocket_frame_clear(frame));
    frame->buffer->start[0] = 0x80;
    testrun(ov_websocket_set_data(frame, (uint8_t *)string + 12,
                                   strlen(string) - 12, true));

    dummy.socket = 0;

    bytes = -1;
    while (-1 == bytes) {

        bytes = SSL_write(ssl, frame->buffer->start, frame->buffer->length);
        ov_event_loop_run(loop, OV_RUN_ONCE);
    }

    // we need to run the loop a while after writing (dont know exactely why)
    while (dummy.socket == 0) {

        ov_event_loop_run(loop, OV_RUN_ONCE);
    }

    testrun(dummy.socket > 0);
    testrun(dummy.item);
    testrun(0 == ov_string_compare("test", ov_event_api_get_event(dummy.item)));
    dummy.item = ov_json_value_free(dummy.item);

    frame = ov_websocket_frame_free(frame);
    json = ov_json_value_free(json);
    string = ov_data_pointer_free(string);

    SSL_CTX_free(ctx);
    SSL_free(ssl);

    testrun(NULL == ov_io_free(io));
    testrun(NULL == ov_webserver_free(self));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_config_from_item() {

    ov_json_value *overall_config = ov_json_object();
    ov_json_value *config = ov_json_object();
    testrun(ov_json_object_set(overall_config, "webserver", config));
    testrun(ov_json_object_set(config, "name", ov_json_string("name")));

    ov_json_value *socket = NULL;
    testrun(ov_socket_configuration_to_json(
        (ov_socket_configuration){
            .host = "localhost", .port = 443, .type = TLS},
        &socket));

    testrun(ov_json_object_set(config, "socket", socket));

    ov_webserver_config conf = ov_webserver_config_from_item(overall_config);
    testrun(0 == strcmp(conf.name, "name"));
    testrun(0 == strcmp(conf.socket.host, "localhost"));
    testrun(TLS == conf.socket.type);
    testrun(443 == conf.socket.port);

    ov_json_value_free(overall_config);

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

    testrun_test(domains_init);
    testrun_test(test_ov_webserver_create);
    testrun_test(test_ov_webserver_free);
    testrun_test(test_ov_webserver_set_debug);
    testrun_test(test_ov_webserver_enable_callback);
    testrun_test(test_ov_webserver_enable_domains);
    testrun_test(test_ov_webserver_config_from_item);
    testrun_test(check_websocket);
    testrun_test(domains_deinit);

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
