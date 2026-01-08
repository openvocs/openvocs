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
        @file           ov_webserver_minimal_test.c
        @author         Markus Töpfer

        @date           2021-12-15


        ------------------------------------------------------------------------
*/
#include "ov_webserver_minimal.c"

#include "../include/ov_event_api.h"

#include <ov_base/ov_utils.h>

#include <ov_arch/ov_arch.h>

#include <ov_base/ov_convert.h>
#include <ov_base/ov_dump.h>
#include <ov_base/ov_file.h>
#include <ov_base/ov_random.h>

#include <ov_test/ov_test.h>
#include <sys/socket.h>
#include <unistd.h>

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/rand.h>
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

static bool set_resources_dir(char *dest) {

    OV_ASSERT(0 != dest);

    char *res = strncpy(dest, test_resource_dir, PATH_MAX - 1);

    if (0 == res)
        return false;

    /* Check for plausibility ... */

    return true;
}

/*----------------------------------------------------------------------------*/

static ov_socket_configuration EMPTY_SOCKET_CONFIGURATION = {0};

/*----------------------------------------------------------------------------*/

static bool socket_configs_equal(ov_socket_configuration const *s1,
                                 ov_socket_configuration const *s2) {

    if ((0 == s1) && (0 == s2))
        return true;

    if (0 == s1)
        return false;
    if (0 == s2)
        return false;

    if (0 != memcmp(s1->host, s2->host, OV_HOST_NAME_MAX))
        return false;
    if (s1->port != s2->port)
        return false;
    if (s1->type != s2->type)
        return false;

    return true;
}

/*----------------------------------------------------------------------------*/

int domains_deinit() {

    testrun(0 != domain_config_file);

    unlink(domain_config_file);
    free((char *)domain_config_file);
    domain_config_file = 0;

    testrun(0 != domain_config_file_one);
    unlink(domain_config_file_one);
    free((char *)domain_config_file_one);
    domain_config_file_one = 0;

    testrun(0 != domain_config_file_two);
    unlink(domain_config_file_two);
    free((char *)domain_config_file_two);
    domain_config_file_two = 0;

    free((char *)test_resource_dir);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int domains_init() {

    test_resource_dir = ov_test_get_resource_path("/resources");

    domain_config_file = ov_test_get_resource_path("resources"
                                                   "/" TEST_DOMAIN_NAME);
    domain_config_file_one =
        ov_test_get_resource_path("resources/" TEST_DOMAIN_NAME_ONE);
    domain_config_file_two =
        ov_test_get_resource_path("resources/" TEST_DOMAIN_NAME_TWO);

    /* Delete possibly remaining files from previous test run */
    domains_deinit();

    /* Since strings have been freed, reinit */
    test_resource_dir = ov_test_get_resource_path("/resources");

    domain_config_file = ov_test_get_resource_path("resources"
                                                   "/" TEST_DOMAIN_NAME);
    domain_config_file_one =
        ov_test_get_resource_path("resources/" TEST_DOMAIN_NAME_ONE);
    domain_config_file_two =
        ov_test_get_resource_path("resources/" TEST_DOMAIN_NAME_TWO);

    ov_json_value *conf = ov_json_object();
    ov_json_value *cert = ov_json_object();
    testrun(ov_json_object_set(conf, OV_KEY_CERTIFICATE, cert));

    ov_json_value *val = NULL;
    val = ov_json_string(TEST_DOMAIN_NAME);
    testrun(ov_json_object_set(conf, OV_KEY_NAME, val));
    val = ov_json_string(test_resource_dir);
    testrun(ov_json_object_set(conf, OV_KEY_PATH, val));
    val = ov_json_string(OV_TEST_CERT);
    testrun(ov_json_object_set(cert, OV_KEY_FILE, val));
    val = ov_json_string(OV_TEST_CERT_KEY);
    testrun(ov_json_object_set(cert, OV_KEY_KEY, val));
    testrun(ov_json_write_file(domain_config_file, conf));

    val = ov_json_string(TEST_DOMAIN_NAME_ONE);
    testrun(ov_json_object_set(conf, OV_KEY_NAME, val));
    val = ov_json_string(test_resource_dir);
    testrun(ov_json_object_set(conf, OV_KEY_PATH, val));
    val = ov_json_string(OV_TEST_CERT_ONE);
    testrun(ov_json_object_set(cert, OV_KEY_FILE, val));
    val = ov_json_string(OV_TEST_CERT_ONE_KEY);
    testrun(ov_json_object_set(cert, OV_KEY_KEY, val));
    testrun(ov_json_write_file(domain_config_file_one, conf));

    val = ov_json_string(TEST_DOMAIN_NAME_TWO);
    testrun(ov_json_object_set(conf, OV_KEY_NAME, val));
    val = ov_json_string(test_resource_dir);
    testrun(ov_json_object_set(conf, OV_KEY_PATH, val));
    val = ov_json_string(OV_TEST_CERT_TWO);
    testrun(ov_json_object_set(cert, OV_KEY_FILE, val));
    val = ov_json_string(OV_TEST_CERT_TWO_KEY);
    testrun(ov_json_object_set(cert, OV_KEY_KEY, val));
    testrun(ov_json_write_file(domain_config_file_two, conf));

    conf = ov_json_value_free(conf);

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

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_webserver_minimal_create() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_minimal_config config = {0};

    testrun(!ov_webserver_minimal_create(config));

    ov_webserver_minimal *webserver = NULL;

    config.base.loop = loop;
    config.base.http.secure = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
    config.base.http.redirect = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),

    set_resources_dir(config.base.domain_config_path);

    webserver = ov_webserver_minimal_create(config);
    testrun(webserver);

    testrun(NULL == ov_webserver_minimal_free(webserver));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_minimal_free() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_minimal_config config = (ov_webserver_minimal_config){
        .base.loop = loop,
        .base.http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .base.http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
    };

    set_resources_dir(config.base.domain_config_path);

    ov_webserver_minimal *webserver = ov_webserver_minimal_create(config);

    testrun(webserver);

    testrun(NULL == ov_webserver_minimal_free(NULL));
    testrun(NULL == ov_webserver_minimal_free(webserver));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool dummy_accept(void *userdata, int server_socket, int socket) {

    intptr_t *out = (intptr_t *)userdata;
    *out = socket;

    fprintf(stdout, "accept at %i conn %i\n", server_socket, socket);

    UNUSED(server_socket);
    return true;
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_minimal_close() {

    int err;
    int errorcode;

    intptr_t conn = -1;

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_minimal_config config = (ov_webserver_minimal_config){
        .base.loop = loop,
        .base.http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .base.http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .callback.userdata = &conn,
        .callback.accept = dummy_accept,
        .base.timer.accept_to_io_timeout_usec = 5000 * 1000};

    set_resources_dir(config.base.domain_config_path);

    ov_webserver_minimal *webserver = ov_webserver_minimal_create(config);

    testrun(webserver);

    int client =
        ov_socket_create(webserver->config.base.http.secure, true, NULL);
    SSL_CTX *ctx = NULL;
    SSL *ssl = NULL;

    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(ssl, TEST_DOMAIN_NAME));
    int r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    testrun(conn > 0);

    testrun(!ov_webserver_minimal_close(NULL, 0));
    testrun(!ov_webserver_minimal_close(webserver, 0));
    testrun(!ov_webserver_minimal_close(webserver, client));

    testrun(ov_webserver_minimal_close(webserver, conn));

    ssize_t bytes = -1;
    char buf[100] = {0};

    while (bytes == -1) {
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_read(ssl, buf, 100);
    }

    testrun(bytes == 0);

    SSL_free(ssl);
    SSL_CTX_free(ctx);
    testrun(NULL == ov_webserver_minimal_free(webserver));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

struct dummy_userdata {

    int socket;
    ov_event_parameter send;
    ov_json_value *value;
};

/*----------------------------------------------------------------------------*/

static void dummy_userdata_clear(struct dummy_userdata *data) {

    if (!data)
        return;

    data->socket = 0;
    data->send = (ov_event_parameter){0};
    data->value = ov_json_value_free(data->value);
    return;
}

/*----------------------------------------------------------------------------*/

static bool dummy_process(void *userdata, const int socket,
                          const ov_event_parameter *params,
                          ov_json_value *input) {

    struct dummy_userdata *data = (struct dummy_userdata *)userdata;
    dummy_userdata_clear(data);

    data->socket = socket;
    data->value = input;
    data->send = *params;

    return true;
}

/*----------------------------------------------------------------------------*/

static bool send_json_over_websocket(ov_event_loop *loop, SSL *ssl,
                                     ov_json_value *msg) {

    char *string = NULL;
    bool result = false;
    ov_websocket_frame *frame = NULL;

    if (!ssl || !msg)
        goto error;

    string = ov_json_value_to_string(msg);
    if (!string)
        goto error;

    frame = ov_websocket_frame_create((ov_websocket_frame_config){0});

    if (!frame)
        goto error;

    frame->buffer->start[0] = 0x80 | OV_WEBSOCKET_OPCODE_TEXT;
    if (!ov_websocket_set_data(frame, (uint8_t *)string, strlen(string), true))
        goto error;

    ssize_t bytes = -1;
    while (bytes == -1) {
        loop->run(loop, OV_RUN_ONCE);
        bytes = SSL_write(ssl, frame->buffer->start, frame->buffer->length);
    }

    if (bytes != (ssize_t)frame->buffer->length)
        goto error;

    result = true;

error:
    ov_websocket_frame_free(frame);
    ov_data_pointer_free(string);
    return result;
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_minimal_configure_uri_event_io() {

    int err;
    int errorcode;

    struct dummy_userdata userdata = {0};

    intptr_t conn = -1;

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_minimal_config config = (ov_webserver_minimal_config){
        .base.loop = loop,
        .base.http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .base.http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .callback.userdata = &conn,
        .callback.accept = dummy_accept,
        .base.timer.accept_to_io_timeout_usec = 5000 * 1000};

    set_resources_dir(config.base.domain_config_path);

    ov_webserver_minimal *webserver = ov_webserver_minimal_create(config);

    testrun(webserver);

    int client =
        ov_socket_create(webserver->config.base.http.secure, true, NULL);
    SSL_CTX *ctx = NULL;
    SSL *ssl = NULL;

    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(ssl, TEST_DOMAIN_NAME));
    int r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    testrun(conn > 0);

    testrun(!ov_webserver_minimal_configure_uri_event_io(
        NULL, (ov_memory_pointer){0}, (ov_event_io_config){0}));

    testrun(!ov_webserver_minimal_configure_uri_event_io(
        NULL,
        (ov_memory_pointer){.start = (uint8_t *)TEST_DOMAIN_NAME,
                            .length = strlen(TEST_DOMAIN_NAME)},
        (ov_event_io_config){.name = "uri",
                             .userdata = &userdata,
                             .callback.process = dummy_process}));

    testrun(!ov_webserver_minimal_configure_uri_event_io(
        webserver,
        (ov_memory_pointer){.start = NULL, .length = strlen(TEST_DOMAIN_NAME)},
        (ov_event_io_config){.name = "uri",
                             .userdata = &userdata,
                             .callback.process = dummy_process}));

    testrun(!ov_webserver_minimal_configure_uri_event_io(
        webserver,
        (ov_memory_pointer){.start = (uint8_t *)TEST_DOMAIN_NAME,
                            .length = strlen(TEST_DOMAIN_NAME)},
        (ov_event_io_config){.userdata = &userdata,
                             .callback.process = dummy_process}));

    // all required data set

    testrun(ov_webserver_minimal_configure_uri_event_io(
        webserver,
        (ov_memory_pointer){.start = (uint8_t *)TEST_DOMAIN_NAME,
                            .length = strlen(TEST_DOMAIN_NAME)},
        (ov_event_io_config){.name = "uri",
                             .userdata = &userdata,
                             .callback.process = dummy_process}));

    // TLS to openvocs.test opened previously, start websocket upgrade
    uint8_t key[OV_WEBSOCKET_SECURE_KEY_SIZE + 1];
    memset(key, 0, OV_WEBSOCKET_SECURE_KEY_SIZE + 1);
    testrun(ov_websocket_generate_secure_websocket_key(
        key, OV_WEBSOCKET_SECURE_KEY_SIZE));

    ov_http_message_config http_config = (ov_http_message_config){0};
    ov_http_version http_version = (ov_http_version){.major = 1, .minor = 1};

    ov_http_message *msg =
        ov_http_create_request_string(http_config, http_version, "GET", "/uri");
    testrun(msg);
    testrun(ov_http_message_add_header_string(msg, OV_HTTP_KEY_HOST,
                                              "openvocs.test"));
    testrun(ov_http_message_add_header_string(msg, OV_HTTP_KEY_UPGRADE,
                                              OV_WEBSOCKET_KEY));
    testrun(ov_http_message_add_header_string(msg, OV_HTTP_KEY_CONNECTION,
                                              OV_HTTP_KEY_UPGRADE));
    testrun(ov_http_message_add_header_string(msg, OV_WEBSOCKET_KEY_SECURE,
                                              (char *)key));
    testrun(ov_http_message_add_header_string(
        msg, OV_WEBSOCKET_KEY_SECURE_VERSION, "13"));
    testrun(ov_http_message_close_header(msg));
    testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_message(msg, NULL));

    ssize_t bytes = -1;

    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, msg->buffer->start, msg->buffer->length);
    }

    testrun(bytes == (ssize_t)msg->buffer->length);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    /* Websocket open, send some message */

    ov_json_value *out = ov_event_api_message_create("test", "uuid", 2);
    testrun(send_json_over_websocket(loop, ssl, out));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    testrun(userdata.socket == conn);
    testrun(userdata.value != NULL);
    testrun(ov_event_api_event_is(userdata.value, "test"));
    testrun(userdata.send.send.instance == webserver);
    testrun(userdata.send.send.send != NULL);
    dummy_userdata_clear(&userdata);

    out = ov_json_value_free(out);
    msg = ov_http_message_free(msg);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    testrun(NULL == ov_webserver_minimal_free(webserver));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_minimal_config_from_json() {

    ov_json_value *out = NULL;

    out = ov_json_object();
    testrun(out);

    ov_webserver_minimal_config config = {0};

    config = ov_webserver_minimal_config_from_json(out);
    testrun(config.base.name[0] == 0);
    testrun(config.base.domain_config_path[0] == 0);
    testrun(config.base.debug == false);
    testrun(config.base.ip4_only == false);
    testrun(config.base.timer.io_timeout_usec == 0);
    testrun(config.base.timer.accept_to_io_timeout_usec == 0);
    testrun(socket_configs_equal(&config.base.http.redirect,
                                 &EMPTY_SOCKET_CONFIGURATION));
    testrun(socket_configs_equal(&config.base.http.secure,
                                 &EMPTY_SOCKET_CONFIGURATION));
    for (size_t i = 0; i < OV_WEBSERVER_BASE_MAX_STUN_LISTENER; i++) {
        testrun(socket_configs_equal(&config.base.stun.socket[i],
                                     &EMPTY_SOCKET_CONFIGURATION));
    }

    out = ov_json_value_free(out);

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

OV_TEST_RUN("ov_webserver_minimal", domains_init,
            test_ov_webserver_minimal_create, test_ov_webserver_minimal_free,
            test_ov_webserver_minimal_close,
            test_ov_webserver_minimal_configure_uri_event_io,
            test_ov_webserver_minimal_config_from_json, domains_deinit);
