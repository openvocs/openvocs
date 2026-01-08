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
        @file           ov_webserver_base_test.c
        @author         Markus Töpfer
        @author         Michael Beer

        @date           2020-12-07


        ------------------------------------------------------------------------
*/
#include "ov_webserver_base.c"

#include <ov_arch/ov_arch.h>

#include <ov_base/ov_convert.h>
#include <ov_base/ov_dump.h>
#include <ov_base/ov_file.h>
#include <ov_base/ov_random.h>

#include <ov_test/ov_test.h>
#include <sys/socket.h>
#include <unistd.h>

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

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

#define TEST_RUNTIME_USECS 50 * 1000

#define TEST_DOMAIN_NAME "openvocs.test"
#define TEST_DOMAIN_NAME_ONE "one.test"
#define TEST_DOMAIN_NAME_TWO "two.test"

static bool test_debug_log = false;

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

int test_ov_webserver_base_cast() {

    uint16_t magic_byte = 0;

    for (size_t i = 0; i < 0xffff; i++) {

        magic_byte = i;
        if (i != OV_WEBSERVER_BASE_MAGIC_BYTE) {
            testrun(!ov_webserver_base_cast(&magic_byte));
        } else {
            testrun(ov_webserver_base_cast(&magic_byte));
        }
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_base_create() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = {0};

    testrun(!ov_webserver_base_create(config));

    ov_webserver_base *webserver = NULL;

    config.loop = loop;

    set_resources_dir(config.domain_config_path);

    // this will create a port at socket 80 and 443!
    testrun_log("Trying to open socket at port 80 and 443,"
                " expect success of IPv6 in IP6 environments.");

    webserver = ov_webserver_base_create(config);
    if (webserver) {
        testrun_log("Webserver created in default mode");
    } else {
        testrun_log("Webserver creation in default mode failed, root may be "
                    "required");
    }
    webserver = ov_webserver_base_free(webserver);

    config.ip4_only = true;

    testrun_log("Trying to open socket at port 80 and 443,"
                " expect success of IPv4 in IPv4 environments.");

    webserver = ov_webserver_base_create(config);
    if (webserver) {
        testrun_log("Webserver created in default mode");
    } else {
        testrun_log("Webserver creation in default mode failed, root mey be "
                    "required");
    }
    webserver = ov_webserver_base_free(webserver);

    // to support testing in unpriviledged mode we use dynamic sockets!
    config.http.secure = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.host = "127.0.0.1", .type = TCP});

    webserver = ov_webserver_base_create(config);
    testrun(webserver);

    testrun(NULL == ov_webserver_base_free(webserver));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_base_free() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = (ov_webserver_base_config){
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 10 * 1000 * 1000,
        .callback.userdata = NULL,
        .callback.accept = NULL,
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0}};

    set_resources_dir(config.domain_config_path);

    ov_webserver_base *webserver = ov_webserver_base_create(config);
    testrun(webserver);

    /* Actual content free is done within the whole testset */

    testrun(NULL == ov_webserver_base_free(NULL));
    testrun(NULL == ov_webserver_base_free(webserver));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_stun() {

    size_t size = 1000;
    uint8_t buf1[size];
    uint8_t buf2[size];
    memset(buf1, 0, size);
    memset(buf2, 0, size);

    size_t attr_size = IMPL_MAX_STUN_ATTRIBUTES * 2;
    uint8_t *attr[attr_size];
    memset(attr, 0, attr_size);

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = {
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0}};

    set_resources_dir(config.domain_config_path);

    for (size_t i = 0; i < OV_WEBSERVER_BASE_MAX_STUN_LISTENER; i++) {

        config.stun.socket[i] = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = UDP});
    };

    int client = ov_socket_create(
        (ov_socket_configuration){.type = UDP, .host = "127.0.0.1", .port = 0},
        false, NULL);

    testrun(ov_socket_ensure_nonblocking(client));

    ov_webserver_base *srv = ov_webserver_base_create(config);
    testrun(srv);

    // check we opened OV_WEBSERVER_BASE_MAX_STUN_LISTENER UDP sockets

    for (size_t i = 0; i < OV_WEBSERVER_BASE_MAX_STUN_LISTENER; i++) {
        testrun(-1 != srv->sockets.stun[i]);
        testrun(ov_socket_is_dgram(srv->sockets.stun[i]));
    }

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // create some valid min STUN buffer
    testrun(ov_stun_frame_set_method(buf1, size, STUN_BINDING));
    testrun(ov_stun_frame_set_magic_cookie(buf1, size));
    testrun(ov_stun_frame_generate_transaction_id(buf1 + 8));
    testrun(ov_stun_frame_set_request(buf1, size));
    testrun(ov_stun_frame_set_length(buf1, size, 0));

    // check test buffer
    testrun(ov_stun_frame_is_valid(buf1, 20));
    testrun(ov_stun_method_is_binding(buf1, 20));
    testrun(ov_stun_frame_class_is_request(buf1, 20));
    testrun(ov_stun_frame_slice(buf1, 20, attr, size));
    // we dont have any attributes set
    testrun(attr[0] == NULL);
    testrun(ov_stun_check_fingerprint(buf1, 20, attr, attr_size, false));

    // send the request to server socket 0
    ssize_t out = -1;
    ssize_t in = -1;

    uint32_t counter = 0;

    struct sockaddr_storage sa_out = {0};
    struct sockaddr_storage sa_in = {0};
    struct sockaddr_storage sa_xor = {0};
    struct sockaddr_storage * xor = &sa_xor;
    socklen_t in_len = sizeof(sa_in);

    ov_socket_data expect = {0};
    ov_socket_data result = {0};
    testrun(ov_socket_get_data(client, &expect, NULL));

    // let the eventloop process the request for each server socket
    for (size_t i = 0; i < OV_WEBSERVER_BASE_MAX_STUN_LISTENER; i++) {

        // clean read buffer
        memset(buf2, 0, size);

        testrun(ov_socket_get_sockaddr_storage(srv->sockets.stun[i], &sa_out,
                                               NULL, NULL));

        out = -1;
        counter = 0;
        while (out == -1) {
            out = sendto(client, buf1, 20, 0, (struct sockaddr *)&sa_out,
                         sizeof(struct sockaddr_storage));
            counter++;
            testrun(counter < 10);
        }

        counter = 0;
        in = -1;
        while (in == -1) {
            testrun(loop->run(loop, TEST_RUNTIME_USECS));
            in = recvfrom(client, buf2, size, 0, (struct sockaddr *)&sa_in,
                          &in_len);
            counter++;
            testrun(counter < 10);
        }

        testrun(in > 20);

        // ov_dump_binary_as_hex(stdout, buf2, in);
        if (test_debug_log)
            fprintf(stdout, "check STUN %zu\n", i);

        // check response
        testrun(ov_stun_frame_is_valid(buf2, in));
        testrun(ov_stun_method_is_binding(buf2, in));
        testrun(ov_stun_frame_class_is_success_response(buf2, in));
        testrun(ov_stun_frame_slice(buf2, in, attr, attr_size));
        // we expect a xor mapped address response
        testrun(attr[0] != NULL);
        testrun(attr[1] == NULL);
        testrun(
            ov_stun_attribute_frame_is_xor_mapped_address(attr[0], in - 20));

        memset(xor, 0, sizeof(struct sockaddr_storage));
        testrun(ov_stun_xor_mapped_address_decode(
            attr[0], in - (attr[0] - buf2), buf2, &xor));

        result = ov_socket_data_from_sockaddr_storage(xor);
        testrun(0 == strncmp(result.host, expect.host, OV_HOST_NAME_MAX));
        testrun(result.port == expect.port);
    }

    // check invalid frame
    memset(buf1, 0, size);

    // create some valid min STUN buffer
    testrun(ov_stun_frame_set_method(buf1, size, STUN_BINDING));
    testrun(ov_stun_frame_set_magic_cookie(buf1, size));
    testrun(ov_stun_frame_generate_transaction_id(buf1 + 8));
    testrun(ov_stun_frame_set_request(buf1, size));
    testrun(ov_stun_frame_set_length(buf1, size, 0));

    // check test buffer
    testrun(ov_stun_frame_is_valid(buf1, 20));
    // destroy magic_cookie
    buf1[4] = 0x22;
    testrun(!ov_stun_frame_is_valid(buf1, 20));

    // clean read buffer
    memset(buf2, 0, size);

    testrun(ov_socket_get_sockaddr_storage(srv->sockets.stun[0], &sa_out, NULL,
                                           NULL));

    // send invalid frame
    out = -1;
    counter = 0;
    while (out == -1) {
        out = sendto(client, buf1, 20, 0, (struct sockaddr *)&sa_out,
                     sizeof(struct sockaddr_storage));
        counter++;
        testrun(counter < 10);
    }

    // read at client expect nothing in return
    counter = 0;
    in = -1;
    while (in == -1) {
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        in =
            recvfrom(client, buf2, size, 0, (struct sockaddr *)&sa_in, &in_len);
        counter++;

        if (counter == 20)
            break;
    }

    testrun(counter == 20);
    testrun(in == -1);

    // check non binding frame
    memset(buf1, 0, size);

    // create some valid min STUN buffer
    testrun(ov_stun_frame_set_method(buf1, size, STUN_BINDING + 1));
    testrun(ov_stun_frame_set_magic_cookie(buf1, size));
    testrun(ov_stun_frame_generate_transaction_id(buf1 + 8));
    testrun(ov_stun_frame_set_request(buf1, size));
    testrun(ov_stun_frame_set_length(buf1, size, 0));

    // check test buffer
    testrun(ov_stun_frame_is_valid(buf1, 20));

    // clean read buffer
    memset(buf2, 0, size);

    testrun(ov_socket_get_sockaddr_storage(srv->sockets.stun[0], &sa_out, NULL,
                                           NULL));

    // send invalid frame
    out = -1;
    counter = 0;
    while (out == -1) {
        out = sendto(client, buf1, 20, 0, (struct sockaddr *)&sa_out,
                     sizeof(struct sockaddr_storage));
        counter++;
        testrun(counter < 10);
    }

    // read at client expect nothing in return
    counter = 0;
    in = -1;
    while (in == -1) {
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        in =
            recvfrom(client, buf2, size, 0, (struct sockaddr *)&sa_in, &in_len);
        counter++;

        if (counter == 20)
            break;
    }

    testrun(counter == 20);
    testrun(in == -1);

    // check non request frame (indication)
    memset(buf1, 0, size);

    // create some valid min STUN buffer
    testrun(ov_stun_frame_set_method(buf1, size, STUN_BINDING));
    testrun(ov_stun_frame_set_magic_cookie(buf1, size));
    testrun(ov_stun_frame_generate_transaction_id(buf1 + 8));
    testrun(ov_stun_frame_set_indication(buf1, size));
    testrun(ov_stun_frame_set_length(buf1, size, 0));

    // check test buffer
    testrun(ov_stun_frame_is_valid(buf1, 20));

    // clean read buffer
    memset(buf2, 0, size);

    testrun(ov_socket_get_sockaddr_storage(srv->sockets.stun[0], &sa_out, NULL,
                                           NULL));

    // send invalid frame
    out = -1;
    counter = 0;
    while (out == -1) {
        out = sendto(client, buf1, 20, 0, (struct sockaddr *)&sa_out,
                     sizeof(struct sockaddr_storage));
        counter++;
        testrun(counter < 10);
    }

    // read at client expect nothing in return
    counter = 0;
    in = -1;
    while (in == -1) {
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        in =
            recvfrom(client, buf2, size, 0, (struct sockaddr *)&sa_in, &in_len);
        counter++;

        if (counter == 20)
            break;
    }

    testrun(counter == 20);
    testrun(in == -1);

    // check non request frame (response)
    memset(buf1, 0, size);

    // create some valid min STUN buffer
    testrun(ov_stun_frame_set_method(buf1, size, STUN_BINDING));
    testrun(ov_stun_frame_set_magic_cookie(buf1, size));
    testrun(ov_stun_frame_generate_transaction_id(buf1 + 8));
    testrun(ov_stun_frame_set_success_response(buf1, size));
    testrun(ov_stun_frame_set_length(buf1, size, 0));

    // check test buffer
    testrun(ov_stun_frame_is_valid(buf1, 20));

    // clean read buffer
    memset(buf2, 0, size);

    testrun(ov_socket_get_sockaddr_storage(srv->sockets.stun[0], &sa_out, NULL,
                                           NULL));

    // send invalid frame
    out = -1;
    counter = 0;
    while (out == -1) {
        out = sendto(client, buf1, 20, 0, (struct sockaddr *)&sa_out,
                     sizeof(struct sockaddr_storage));
        counter++;
        testrun(counter < 10);
    }

    // read at client expect nothing in return
    counter = 0;
    in = -1;
    while (in == -1) {
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        in =
            recvfrom(client, buf2, size, 0, (struct sockaddr *)&sa_in, &in_len);
        counter++;

        if (counter == 20)
            break;
    }

    testrun(counter == 20);
    testrun(in == -1);

    // check software attribute (ignore and respond)
    memset(buf1, 0, size);

    // create some valid min STUN buffer
    testrun(ov_stun_frame_set_method(buf1, size, STUN_BINDING));
    testrun(ov_stun_frame_set_magic_cookie(buf1, size));
    testrun(ov_stun_frame_generate_transaction_id(buf1 + 8));
    testrun(ov_stun_frame_set_request(buf1, size));
    const char *software = "abc";
    testrun(ov_stun_software_encode(buf1 + 20, size - 20, NULL,
                                    (uint8_t *)software, strlen(software)));
    testrun(
        ov_stun_frame_set_length(buf1, size,
                                 ov_stun_software_encoding_length(
                                     (uint8_t *)software, strlen(software))));

    size_t len = 20 + ov_stun_software_encoding_length((uint8_t *)software,
                                                       strlen(software));

    // check test buffer
    testrun(ov_stun_frame_is_valid(buf1, len));
    testrun(ov_stun_method_is_binding(buf1, len));
    testrun(ov_stun_frame_class_is_request(buf1, len));
    testrun(ov_stun_frame_slice(buf1, len, attr, size));
    // we have the software attribute set
    testrun(attr[0] != NULL);
    testrun(attr[1] == NULL);
    testrun(ov_stun_check_fingerprint(buf1, len, attr, attr_size, false));
    testrun(STUN_SOFTWARE == ov_stun_attribute_get_type(attr[0], len - 20));

    // clean read buffer
    memset(buf2, 0, size);

    testrun(ov_socket_get_sockaddr_storage(srv->sockets.stun[0], &sa_out, NULL,
                                           NULL));

    // send invalid frame
    out = -1;
    counter = 0;
    while (out == -1) {
        out = sendto(client, buf1, len, 0, (struct sockaddr *)&sa_out,
                     sizeof(struct sockaddr_storage));
        counter++;
        testrun(counter < 10);
    }

    // read at client expect response
    counter = 0;
    in = -1;
    while (in == -1) {
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        in =
            recvfrom(client, buf2, size, 0, (struct sockaddr *)&sa_in, &in_len);
        counter++;
        testrun(counter < 10);
    }

    // ov_dump_binary_as_hex(stdout, buf2, in);
    // fprintf(stdout, "\n");

    // check response
    testrun(ov_stun_frame_is_valid(buf2, in));
    testrun(ov_stun_method_is_binding(buf2, in));
    testrun(ov_stun_frame_class_is_success_response(buf2, in));
    testrun(ov_stun_frame_slice(buf2, in, attr, attr_size));
    // we expect a xor mapped address response
    testrun(attr[0] != NULL);
    testrun(attr[1] == NULL);
    testrun(ov_stun_attribute_frame_is_xor_mapped_address(attr[0], in - 20));

    memset(xor, 0, sizeof(struct sockaddr_storage));
    testrun(ov_stun_xor_mapped_address_decode(attr[0], in - (attr[0] - buf2),
                                              buf2, &xor));

    result = ov_socket_data_from_sockaddr_storage(xor);
    testrun(0 == strncmp(result.host, expect.host, OV_HOST_NAME_MAX));
    testrun(result.port == expect.port);

    // check fingerpint attribute (valid)
    memset(buf1, 0, size);

    // create some valid min STUN buffer
    testrun(ov_stun_frame_set_method(buf1, size, STUN_BINDING));
    testrun(ov_stun_frame_set_magic_cookie(buf1, size));
    testrun(ov_stun_frame_generate_transaction_id(buf1 + 8));
    testrun(ov_stun_frame_set_request(buf1, size));
    testrun(ov_stun_add_fingerprint(buf1, size - 20, buf1 + 20, NULL));
    testrun(ov_stun_frame_set_length(buf1, size,
                                     ov_stun_fingerprint_encoding_length()));

    len = 20 + ov_stun_fingerprint_encoding_length();

    // check test buffer
    testrun(ov_stun_frame_is_valid(buf1, len));
    testrun(ov_stun_method_is_binding(buf1, len));
    testrun(ov_stun_frame_class_is_request(buf1, len));
    testrun(ov_stun_frame_slice(buf1, len, attr, size));
    // we have the fingerprint attribute set
    testrun(attr[0] != NULL);
    testrun(attr[1] == NULL);
    testrun(ov_stun_check_fingerprint(buf1, len, attr, attr_size, false));
    testrun(STUN_FINGERPRINT == ov_stun_attribute_get_type(attr[0], len - 20));
    // actually check with MUST be present
    testrun(ov_stun_check_fingerprint(buf1, len, attr, attr_size, true));

    // clean read buffer
    memset(buf2, 0, size);

    testrun(ov_socket_get_sockaddr_storage(srv->sockets.stun[0], &sa_out, NULL,
                                           NULL));

    // send frame
    out = -1;
    counter = 0;
    while (out == -1) {
        out = sendto(client, buf1, len, 0, (struct sockaddr *)&sa_out,
                     sizeof(struct sockaddr_storage));
        counter++;
        testrun(counter < 10);
    }

    // read at client expect response
    counter = 0;
    in = -1;
    while (in == -1) {
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        in =
            recvfrom(client, buf2, size, 0, (struct sockaddr *)&sa_in, &in_len);
        counter++;
        testrun(counter < 10);
    }

    // ov_dump_binary_as_hex(stdout, buf2, in);
    // fprintf(stdout, "\n");

    // check response
    testrun(ov_stun_frame_is_valid(buf2, in));
    testrun(ov_stun_method_is_binding(buf2, in));
    testrun(ov_stun_frame_class_is_success_response(buf2, in));
    testrun(ov_stun_frame_slice(buf2, in, attr, attr_size));
    // we expect a xor mapped address response
    testrun(attr[0] != NULL);
    testrun(attr[1] == NULL);
    testrun(ov_stun_attribute_frame_is_xor_mapped_address(attr[0], in - 20));

    memset(xor, 0, sizeof(struct sockaddr_storage));
    testrun(ov_stun_xor_mapped_address_decode(attr[0], in - (attr[0] - buf2),
                                              buf2, &xor));

    result = ov_socket_data_from_sockaddr_storage(xor);
    testrun(0 == strncmp(result.host, expect.host, OV_HOST_NAME_MAX));
    testrun(result.port == expect.port);

    // invalidate the fingerprint
    testrun(ov_stun_frame_slice(buf1, len, attr, attr_size));
    testrun(ov_stun_check_fingerprint(buf1, len, attr, attr_size, true));
    buf1[26] = buf1[26] + 1;
    testrun(!ov_stun_check_fingerprint(buf1, len, attr, attr_size, true));

    // send frame
    out = -1;
    counter = 0;
    while (out == -1) {
        out = sendto(client, buf1, len, 0, (struct sockaddr *)&sa_out,
                     sizeof(struct sockaddr_storage));
        counter++;
        testrun(counter < 10);
    }

    // read at client expect no response
    counter = 0;
    in = -1;
    while (in == -1) {
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        in =
            recvfrom(client, buf2, size, 0, (struct sockaddr *)&sa_in, &in_len);
        counter++;
        if (counter == 20)
            break;
    }

    testrun(counter == 20);
    testrun(in == -1);

    // check unsupported attribute (ignore)
    memset(buf1, 0, size);

    // create some valid min STUN buffer
    testrun(ov_stun_frame_set_method(buf1, size, STUN_BINDING));
    testrun(ov_stun_frame_set_magic_cookie(buf1, size));
    testrun(ov_stun_frame_generate_transaction_id(buf1 + 8));
    testrun(ov_stun_frame_set_request(buf1, size));
    testrun(ov_stun_realm_encode(buf1 + 20, size - 20, NULL,
                                 (uint8_t *)software, strlen(software)));
    testrun(ov_stun_frame_set_length(
        buf1, size,
        ov_stun_realm_encoding_length((uint8_t *)software, strlen(software))));

    len = 20 +
          ov_stun_realm_encoding_length((uint8_t *)software, strlen(software));

    // check test buffer
    testrun(ov_stun_frame_is_valid(buf1, len));
    testrun(ov_stun_method_is_binding(buf1, len));
    testrun(ov_stun_frame_class_is_request(buf1, len));
    testrun(ov_stun_frame_slice(buf1, len, attr, size));
    // we have the software attribute set
    testrun(attr[0] != NULL);
    testrun(attr[1] == NULL);
    testrun(ov_stun_check_fingerprint(buf1, len, attr, attr_size, false));
    testrun(STUN_REALM == ov_stun_attribute_get_type(attr[0], len - 20));

    // clean read buffer
    memset(buf2, 0, size);

    testrun(ov_socket_get_sockaddr_storage(srv->sockets.stun[0], &sa_out, NULL,
                                           NULL));

    // send frame
    out = -1;
    counter = 0;
    while (out == -1) {
        out = sendto(client, buf1, len, 0, (struct sockaddr *)&sa_out,
                     sizeof(struct sockaddr_storage));
        counter++;
        testrun(counter < 10);
    }

    // read at client expect no response
    counter = 0;
    in = -1;
    while (in == -1) {
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        in =
            recvfrom(client, buf2, size, 0, (struct sockaddr *)&sa_in, &in_len);
        counter++;
        if (counter == 20)
            break;
    }

    testrun(counter == 20);
    testrun(in == -1);

    // send some non STUN

    uint32_t rlen = 0;

    if (test_debug_log)
        fprintf(stdout, "\nSending RANDOM buffers\n");

    for (size_t i = 0; i < 10; i++) {

        rlen = ov_random_uint32();
        if (rlen > size)
            rlen = size;

        memset(buf1, 0, size);
        testrun(ov_random_bytes_with_zeros(buf1, rlen));

        // ov_dump_binary_as_hex(stdout, buf1, rlen);
        if (test_debug_log)
            fprintf(stdout, "Check random %zu\n", i);

        // send frame
        out = -1;
        counter = 0;
        while (out == -1) {
            out = sendto(client, buf1, rlen, 0, (struct sockaddr *)&sa_out,
                         sizeof(struct sockaddr_storage));
            counter++;
            testrun(counter < 10);
        }

        // read at client expect no response
        counter = 0;
        in = -1;
        while (in == -1) {
            testrun(loop->run(loop, TEST_RUNTIME_USECS));
            in = recvfrom(client, buf2, size, 0, (struct sockaddr *)&sa_in,
                          &in_len);
            counter++;
            if (counter == 20)
                break;
        }

        testrun(counter == 20);
        testrun(in == -1);
    }

    close(client);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(NULL == ov_webserver_base_free(srv));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_redirect() {

    size_t size = 1000;
    uint8_t buf1[size];
    uint8_t buf2[size];
    memset(buf1, 0, size);
    memset(buf2, 0, size);

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = (ov_webserver_base_config){
        .debug = test_debug_log,
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0}};

    set_resources_dir(config.domain_config_path);

    ov_webserver_base *srv = ov_webserver_base_create(config);
    testrun(srv);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    int client = ov_socket_create(config.http.redirect, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // check redirect min

    char *str = "GET / HTTP/1.1\r\nHost:x\r\n\r\n";
    ssize_t bytes = send(client, str, strlen(str), 0);
    testrun(bytes == (ssize_t)strlen(str));
    bytes = -1;
    uint64_t start = ov_time_get_current_time_usecs();
    while (bytes == -1) {
        testrun(loop->run(loop, OV_RUN_ONCE));
        bytes = recv(client, buf1, size, 0);
    }
    uint64_t end = ov_time_get_current_time_usecs();
    if (test_debug_log)
        fprintf(stdout, "Roundtrip %" PRIu64 " (usec) %zi bytes\n", end - start,
                bytes);
    // fprintf(stdout, "|%s|\n", buf1);
    testrun(snprintf((char *)buf2, size,
                     "HTTP/1.1 301 Moved "
                     "Permanently\r\nLocation:https://x:%i\r\n\r\n",
                     config.http.secure.port))

        testrun(0 == strcmp((char *)buf1, (char *)buf2));
    // expect the connection to be closed
    bytes = send(client, str, strlen(str), 0);
    testrun(bytes == (ssize_t)strlen(str));
    bytes = -1;
    start = ov_time_get_current_time_usecs();
    while (bytes == -1) {
        testrun(loop->run(loop, OV_RUN_ONCE));
        bytes = recv(client, buf1, size, 0);
    }
    end = ov_time_get_current_time_usecs();
    if (test_debug_log)
        fprintf(stdout, "Roundtrip %" PRIu64 " (usec) %zi bytes\n", end - start,
                bytes);
    testrun(bytes == 0);

    // reconnect client
    client = ov_socket_create(config.http.redirect, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // check request without HOST header, expect close
    memset(buf1, 0, size);
    memset(buf2, 0, size);
    str = "GET / HTTP/1.1\r\n\r\n";
    bytes = send(client, str, strlen(str), 0);
    testrun(bytes == (ssize_t)strlen(str));
    bytes = -1;
    start = ov_time_get_current_time_usecs();
    while (bytes == -1) {
        testrun(loop->run(loop, OV_RUN_ONCE));
        bytes = recv(client, buf1, size, 0);
    }
    end = ov_time_get_current_time_usecs();
    if (test_debug_log)
        fprintf(stdout, "Roundtrip %" PRIu64 " (usec) %zi bytes\n", end - start,
                bytes);
    testrun(bytes == 0);

    // check state in srv
    for (size_t i = 0; i < srv->config.limit.max_sockets; i++) {
        testrun(srv->conn[i].fd == -1);
    }

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    if (test_debug_log)
        fprintf(stdout, "--> reconnect client");

    // reconnect client
    client = ov_socket_create(config.http.redirect, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // check with absolute request URI
    memset(buf1, 0, size);
    memset(buf2, 0, size);
    str = "GET openvocs.org:1234/ HTTP/1.1\r\nHost:openvocs.org\r\n\r\n";
    bytes = send(client, str, strlen(str), 0);
    // fprintf(stdout, "client %i sent %zi bytes|%s\n", client, bytes, str);
    testrun(bytes == (ssize_t)strlen(str));
    bytes = -1;
    start = ov_time_get_current_time_usecs();
    while (bytes == -1) {
        testrun(loop->run(loop, OV_RUN_ONCE));
        bytes = recv(client, buf1, size, 0);
    }
    end = ov_time_get_current_time_usecs();
    if (test_debug_log)
        fprintf(stdout, "Roundtrip %" PRIu64 " (usec) %zi bytes\n", end - start,
                bytes);
    // fprintf(stdout, "|%s|\n", buf1);
    testrun(snprintf((char *)buf2, size,
                     "HTTP/1.1 301 Moved "
                     "Permanently\r\nLocation:https://openvocs.org:%i\r\n\r\n",
                     config.http.secure.port))

        testrun(0 == strcmp((char *)buf1, (char *)buf2));

    // expect the connection to be closed
    bytes = send(client, str, strlen(str), 0);
    testrun(bytes == (ssize_t)strlen(str));
    bytes = -1;
    start = ov_time_get_current_time_usecs();
    while (bytes == -1) {
        testrun(loop->run(loop, OV_RUN_ONCE));
        bytes = recv(client, buf1, size, 0);
    }
    end = ov_time_get_current_time_usecs();
    if (test_debug_log)
        fprintf(stdout, "Roundtrip %" PRIu64 " (usec) %zi bytes\n", end - start,
                bytes);
    testrun(bytes == 0);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // check state in srv
    for (size_t i = 0; i < srv->config.limit.max_sockets; i++) {
        testrun(srv->conn[i].fd == -1);
    }

    // check with random buffer
    size_t count = 0;
    srv->config.debug = false;

    for (size_t i = 0; i < 25; i++) {

        memset(buf1, 0, size);
        memset(buf2, 0, size);

        // reconnect client
        client = ov_socket_create(config.http.redirect, true, NULL);
        testrun(client > 0);
        testrun(ov_socket_ensure_nonblocking(client));
        testrun(loop->run(loop, TEST_RUNTIME_USECS));

        // check state in srv (one client SHOULD be open)
        // NOTE accepted FD is not client fd!

        count = 0;
        for (size_t x = 0; x < srv->config.limit.max_sockets; x++) {
            if (srv->conn[x].fd != -1)
                count++;
        }
        testrun(count == 1);

        testrun(ov_random_bytes_with_zeros(buf1, size));
        // ov_dump_binary_as_hex(stdout, buf1, len);
        bytes = send(client, buf1, size, 0);
        testrun(bytes == (ssize_t)size);
        // run recv and some spare time for TCP ack
        testrun(loop->run(loop, TEST_RUNTIME_USECS));

        for (size_t x = 0; x < 100; x++) {

            bytes = -1;
            bytes = recv(client, buf2, size, 0);
            if (bytes != -1)
                break;

            usleep(TEST_RUNTIME_USECS);

            /*
             *  To avoid any timeout issue of the underlaying TCP stack,
             *  we had back to the system and let TCP stack resolve any
             *  retransmissions and acknowledgements.
             *
             *  We limit this to 100 retrys before we finaly fail the test.
             *  TCP handling is NOT part of this test method
             *  (THIS IS NOT WHAT WE WANT TO TEST HERE!),
             *  so we just try to avoid any issues here.
             *
             *  It may happen anyway!
             *
             *  So this test is with a bit of salt and limited to some
             *  retries.
             */

            if (errno == ETIMEDOUT) {
                testrun(loop->run(loop, TEST_RUNTIME_USECS));
                usleep(TEST_RUNTIME_USECS);
            }
        }

        if (test_debug_log)
            fprintf(stdout,
                    "%zu| client %i send %zu random bytes read %zi bytes\n", i,
                    client, size, bytes);

        if (errno == ETIMEDOUT) {

            if (test_debug_log)
                fprintf(stdout, "---> %zu| client %i TCP TIMEDOUT\n", i,
                        client);

            /*
             *  We may reach this point, but it is not the intended test case,
             *  so we close the client and let the server process the close
             *  with over an eventloop run.
             */
            testrun(-1 == bytes);
            close(client);
            testrun(loop->run(loop, TEST_RUNTIME_USECS));

        } else {
            testrun(0 == bytes);
        }

        // check state in srv (all clients SHOULD be closed)
        count = 0;
        for (size_t x = 0; x < srv->config.limit.max_sockets; x++) {
            if (srv->conn[x].fd != -1)
                count++;
        }
        testrun(count == 0);

        // Close client, to free up the socket assignments within the OS
        close(client);
    }

    if (test_debug_log)
        fprintf(stdout, "-------------------- cleanup\n");
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(NULL == ov_webserver_base_free(srv));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_accept_timeout() {

    size_t size = 1000;
    uint8_t buf[size];
    memset(buf, 0, size);

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = (ov_webserver_base_config){
        .debug = test_debug_log,
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0},
        .timer.accept_to_io_timeout_usec = 2 * TEST_RUNTIME_USECS};

    set_resources_dir(config.domain_config_path);

    ov_webserver_base *srv = ov_webserver_base_create(config);
    testrun(srv);

    // check accept timeout
    srv->config.timer.accept_to_io_timeout_usec = 2 * TEST_RUNTIME_USECS;

    // connect client to HTTP
    int client = ov_socket_create(config.http.redirect, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    ssize_t bytes = -1;
    memset(buf, 0, size);
    size_t count = 0;
    while (bytes == -1) {
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = recv(client, buf, size, 0);
        testrun(bytes < 1);
        count++;
    }
    testrun(bytes == 0);
    testrun(count >= 1);
    testrun(count <= 3);
    close(client);

    // connect client to HTTPS
    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    bytes = -1;
    memset(buf, 0, size);
    count = 0;
    while (bytes == -1) {
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = recv(client, buf, size, 0);
        testrun(bytes < 1);
        count++;
    }
    testrun(bytes == 0);
    testrun(count >= 1);
    testrun(count <= 3);
    close(client);

    if (test_debug_log)
        fprintf(stdout, "-------------------- cleanup\n");
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(NULL == ov_webserver_base_free(srv));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
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

static char *fingerprint_format_RFC8122(const char *source, size_t length) {

    /*
    hash-func         =  "sha-1" / "sha-224" / "sha-256" /
                         "sha-384" / "sha-512" /
                         "md5" / "md2" / token
                         ; Additional hash functions can only come
                         ; from updates to RFC 3279

    fingerprint            =  2UHEX *(":" 2UHEX)
                         ; Each byte in upper-case hex, separated
                         ; by colons.

    UHEX                   =  DIGIT / %x41-46 ; A-F uppercase
    */

    char *fingerprint = NULL;

    if (!source)
        return NULL;

    size_t hex_len = 2 * length + 1;
    char hex[hex_len + 1];
    memset(hex, 0, hex_len);
    uint8_t *ptr = (uint8_t *)hex;

    if (!ov_convert_binary_to_hex((uint8_t *)source, length, &ptr, &hex_len))
        goto error;

    size_t size = hex_len + length;
    fingerprint = calloc(size + 1, sizeof(char));

    for (size_t i = 0; i < length; i++) {

        fingerprint[(i * 3) + 0] = toupper(hex[(i * 2) + 0]);
        fingerprint[(i * 3) + 1] = toupper(hex[(i * 2) + 1]);
        if (i < length - 1)
            fingerprint[(i * 3) + 2] = ':';
    }

    return fingerprint;

error:
    if (fingerprint)
        free(fingerprint);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static char *fingerprint_from_cert(const X509 *cert, const EVP_MD *func) {

    if (!cert || !func)
        return NULL;

    unsigned char mdigest[EVP_MAX_MD_SIZE] = {0};
    unsigned int mdigest_size = 0;

    if (0 == X509_digest(cert, func, mdigest, &mdigest_size))
        return NULL;

    return fingerprint_format_RFC8122((char *)mdigest, mdigest_size);
}

/*----------------------------------------------------------------------------*/

static char *fingerprint_from_path(const char *path, const EVP_MD *func) {

    char *fingerprint = NULL;
    FILE *fp = NULL;
    X509 *x = NULL;

    if (!path || !func)
        goto error;

    fp = fopen(path, "r");

    if (!PEM_read_X509(fp, &x, NULL, NULL))
        goto error;

    fingerprint = fingerprint_from_cert(x, func);

    fclose(fp);
    X509_free(x);
    return fingerprint;
error:
    if (x)
        X509_free(x);
    if (fp)
        fclose(fp);

    if (fingerprint)
        ov_data_pointer_free(fingerprint);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool check_cert(SSL *ssl, const EVP_MD *func, const char *fingerprint) {

    if (!ssl || !func || !fingerprint)
        return false;

    char *finger = NULL;

    X509 *cert = SSL_get_peer_certificate(ssl);
    if (!cert)
        goto error;

    finger = fingerprint_from_cert(cert, func);
    if (!finger)
        goto error;

    if (0 != strcmp(finger, fingerprint))
        goto error;

    finger = ov_data_pointer_free(finger);
    X509_free(cert);

    return true;
error:
    if (cert)
        X509_free(cert);

    if (finger)
        ov_data_pointer_free(finger);
    return false;
}

/*----------------------------------------------------------------------------*/

int check_sni() {

    const EVP_MD *digest_func = EVP_sha256();

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = (ov_webserver_base_config){
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 10 * 1000 * 1000,
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0}};

    set_resources_dir(config.domain_config_path);

    ov_webserver_base *srv = ov_webserver_base_create(config);
    testrun(srv);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    /* check domains configured */
    testrun(3 == srv->domain.size);
    testrun(0 == memcmp(TEST_DOMAIN_NAME,
                        srv->domain.array[0].config.name.start,
                        srv->domain.array[0].config.name.length));
    testrun(0 == memcmp(TEST_DOMAIN_NAME_ONE,
                        srv->domain.array[1].config.name.start,
                        srv->domain.array[1].config.name.length));
    testrun(0 == memcmp(TEST_DOMAIN_NAME_TWO,
                        srv->domain.array[2].config.name.start,
                        srv->domain.array[2].config.name.length));

    char *fingerprint[srv->domain.size];
    memset(fingerprint, 0, sizeof(char *));

    for (size_t i = 0; i < srv->domain.size; i++) {
        fingerprint[i] = fingerprint_from_path(
            srv->domain.array[i].config.certificate.cert, digest_func);
        testrun(fingerprint[i]);
    }

    /* create some client connection */
    int client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL *ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);

    /* start handshaking */
    int errorcode = -1;
    int err = 0;

    if (test_debug_log)
        fprintf(stdout, "\nstart client handshake without hostname ext\n");

    int r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    /* STATE
     *  - no default set
     *  - request without domain header send
     * EXPECT
     *  - certificate of domain.array[0] returned
     */

    testrun(check_cert(ssl, digest_func, fingerprint[0]));

    // reset
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // set default domain to two.test
    srv->domain.default_domain = 2;

    if (test_debug_log)
        fprintf(stdout,
                "\nstart client handshake without hostname at two.test\n");

    // reconnect
    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);

    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    /* STATE
     *  - default set
     *  - request without domain header send
     * EXPECT
     *  - certificate of domain.array[2] returned
     */

    testrun(check_cert(ssl, digest_func, fingerprint[2]));

    // reset
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    if (test_debug_log)
        fprintf(stdout, "\nstart client handshake with hostname ext at "
                        "openvocs.test\n");

    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));

    // reconnect
    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(
                     ssl, (char *)srv->domain.array[0].config.name.start));
    const char *ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 == strcmp(ptr, "openvocs.test"));
    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    /* STATE
     *  - default set
     *  - request with domain header for openvocs.test
     * EXPECT
     *  - certificate of domain.array[0] returned
     */

    testrun(check_cert(ssl, digest_func, fingerprint[0]));

    // reset
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    if (test_debug_log)
        fprintf(stdout,
                "\nstart client handshake with hostname ext at one.test\n");

    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));

    // reconnect
    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(
                     ssl, (char *)srv->domain.array[1].config.name.start));
    ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 == strcmp(ptr, "one.test"));
    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    /* STATE
     *  - default set
     *  - request with domain header for one.test
     * EXPECT
     *  - certificate of domain.array[1] returned
     */

    testrun(check_cert(ssl, digest_func, fingerprint[1]));

    // reset
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));

    if (test_debug_log)
        fprintf(stdout,
                "\nstart client handshake with hostname ext at two.test\n");

    // reconnect
    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(
                     ssl, (char *)srv->domain.array[2].config.name.start));
    ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 == strcmp(ptr, "two.test"));
    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    /* STATE
     *  - default set
     *  - request with domain header for two.test
     * EXPECT
     *  - certificate of domain.array[2] returned
     */

    testrun(check_cert(ssl, digest_func, fingerprint[2]));

    // reset
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));

    if (test_debug_log)
        fprintf(stdout, "\nstart client handshake with hostname ext at "
                        "unconfigured.test\n");

    // reconnect
    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(ssl, "unconfigured.test"));
    ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 == strcmp(ptr, "unconfigured.test"));
    r = run_client_handshake(loop, ssl, &err, &errorcode);

    // expect abort with close
    testrun(r == -1);

    // reset
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    if (test_debug_log)
        fprintf(stdout, "\nstart client handshake with non ascii domain\n");

    // check with non ascii name
    // here we do change the host name, but NOT the cerificate

    ov_domain *domain = &srv->domain.array[1];
    memset(domain->config.name.start, 0, domain->config.name.length);

    // mix of different length unicode chars

    domain->config.name.start[0] = 0xCE; // 2 byte UTF8 alpha 0xCE91
    domain->config.name.start[1] = 0x91;
    domain->config.name.start[2] = 0xCE; // 2 byte UTF8 beta 0xCE92
    domain->config.name.start[3] = 0x92;
    domain->config.name.start[4] = 0xCE; // 2 byte UTF8 gamma 0xCE93
    domain->config.name.start[5] = 0x93;
    domain->config.name.start[6] =
        0xF0; // 4 byte UTF8 Gothic Letter Ahsa U+10330
    domain->config.name.start[7] = 0x90;
    domain->config.name.start[8] = 0x8C;
    domain->config.name.start[9] = 0xB0;
    domain->config.name.start[10] =
        0xF0; // 4 byte UTF8 Gothic Letter Bairkan U+10331
    domain->config.name.start[11] = 0x90;
    domain->config.name.start[12] = 0x8C;
    domain->config.name.start[13] = 0xb1;
    domain->config.name.start[14] =
        0xE3; // 3 byte UTF8 Cjk unified ideograph U+3400
    domain->config.name.start[15] = 0x90;
    domain->config.name.start[16] = 0x80;
    domain->config.name.start[17] = 0xCE; // 2 byte UTF8 delta 0xCE94
    domain->config.name.start[18] = 0x94;
    domain->config.name.start[19] = 'x'; // 1 byte char x
    domain->config.name.length = 20;

    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));

    // reconnect
    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(ssl, domain->config.name.start));
    ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 ==
            memcmp(ptr, domain->config.name.start, domain->config.name.length));
    r = run_client_handshake(loop, ssl, &err, &errorcode);

    testrun(r == 1);

    /* STATE
     *  - default set
     *  - request with domain header for utf8 valid domain
     * EXPECT
     *  - certificate of domain.array[1] returned
     */

    testrun(check_cert(ssl, digest_func, fingerprint[1]));

    // reset
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    if (test_debug_log)
        fprintf(stdout, "-------------------- cleanup\n");

    for (size_t i = 0; i < srv->domain.size; i++) {
        fingerprint[i] = ov_data_pointer_free(fingerprint[i]);
    }

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(NULL == ov_webserver_base_free(srv));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_secure_socket() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = (ov_webserver_base_config){
        .debug = test_debug_log,
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0}};

    set_resources_dir(config.domain_config_path);

    ov_webserver_base *srv = ov_webserver_base_create(config);
    testrun(srv);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    /* create some client connection */
    int client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL *ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);

    /* start handshaking */
    int errorcode = -1;
    int err = 0;

    int r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    char *string = "non secure";
    ssize_t bytes = send(client, string, strlen(string), 0);
    testrun(bytes == (ssize_t)strlen(string));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    char buf[1000] = {0};

    bytes = -1;
    while (bytes == -1) {
        bytes = recv(client, buf, 1000, 0);
    }

    testrun(bytes != 0);

    bytes = -1;
    while (bytes == -1) {
        bytes = recv(client, buf, 1000, 0);
    }

    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    if (test_debug_log)
        fprintf(stdout, "-------------------- cleanup\n");

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(NULL == ov_webserver_base_free(srv));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_tls_send_buffer() {

    char errorstring[OV_SSL_ERROR_STRING_BUFFER_SIZE] = {0};
    int errorcode = -1, n = 0;

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = (ov_webserver_base_config){
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 100 * 1000 * 1000,
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0}};

    set_resources_dir(config.domain_config_path);

    ov_webserver_base *srv = ov_webserver_base_create(config);
    testrun(srv);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    /* create some client connection */
    int client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL *ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(
                     ssl, (char *)srv->domain.array[0].config.name.start));

    Connection *conn = NULL;

    for (size_t i = 0; i < srv->connections_max; i++) {

        if (srv->conn[i].fd != -1) {
            conn = &srv->conn[i];
            break;
        }
    }

    testrun(conn);
    if (test_debug_log)
        fprintf(stdout, "... using connection with socket %i\n", conn->fd);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    char *expect = "send me secured please";
    ov_buffer *buffer = ov_buffer_from_string(expect);
    testrun(buffer);

    size_t size = 1000;
    char buf[size];
    memset(buf, 0, size);

    // conn not handshaked expect connection close
    testrun(!tls_send_buffer(srv, conn, buffer));
    ssize_t bytes = -1;
    n = 0;
    bool retry = true;

    while (retry) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        usleep(TEST_RUNTIME_USECS);
        bytes = SSL_read(ssl, buf, size);

        if (bytes < 1) {

            n = SSL_get_error(ssl, bytes);

            fprintf(stdout, "SSL failed bytes %zi err %i\n", bytes, n);

            switch (n) {

            case SSL_ERROR_NONE:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_NONE - retry\n");
                break;
            case SSL_ERROR_WANT_READ:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_READ - retry\n");
                break;
            case SSL_ERROR_WANT_WRITE:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_WRITE - retry\n");
                break;
            case SSL_ERROR_WANT_CONNECT:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_CONNECT - retry\n");
                break;
            case SSL_ERROR_WANT_ACCEPT:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_ACCEPT - retry\n");
                break;
            case SSL_ERROR_WANT_X509_LOOKUP:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_X509_LOOKUP - retry\n");
                break;

            case SSL_ERROR_ZERO_RETURN:
                fprintf(stdout, "SSL_ERROR_ZERO_RETURN - close\n");
                retry = false;
                break;

            case SSL_ERROR_SYSCALL:

                errorcode = ERR_get_error();
                ERR_error_string_n(errorcode, errorstring,
                                   OV_SSL_ERROR_STRING_BUFFER_SIZE);

                fprintf(stdout, "SSL_ERROR_SYSCALL - stop %i|%s errno %i|%s\n",
                        errorcode, errorstring, errno, strerror(errno));

                retry = false;
                break;

            case SSL_ERROR_SSL:

                errorcode = ERR_get_error();
                ERR_error_string_n(errorcode, errorstring,
                                   OV_SSL_ERROR_STRING_BUFFER_SIZE);

                fprintf(stdout, "SSL_ERROR_SSL - stop %i|%s errno %i|%s\n",
                        errorcode, errorstring, errno, strerror(errno));

                retry = false;
                break;

            default:
                fprintf(stdout, "DEFAULT REACHED");
                testrun(1 == 0);
                break;
            }
        }
    }

    // not handshaked yet, will not send
    testrun((n == SSL_ERROR_SYSCALL) || (n == SSL_ERROR_SSL));

    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // check conn
    for (size_t i = 0; i < srv->connections_max; i++) {
        testrun(-1 == srv->conn[i].fd);
    }

    // reconnect client
    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(
                     ssl, (char *)srv->domain.array[0].config.name.start));

    /* start handshaking */
    errorcode = -1;
    int err = 0;

    int r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    if (test_debug_log)
        fprintf(stdout, "-------------------- handshake done\n");

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // search new conn
    for (size_t i = 0; i < srv->connections_max; i++) {

        if (srv->conn[i].fd != -1) {
            conn = &srv->conn[i];
            break;
        }
    }

    // we set the connection handshake in NOT done to test the function
    testrun(conn->tls.handshaked == true);
    conn->tls.handshaked = false;

    // conn not handshaked expect connection close
    testrun(!tls_send_buffer(srv, conn, buffer));
    bytes = -1;
    n = 0;
    retry = true;

    while (retry) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        usleep(TEST_RUNTIME_USECS);
        bytes = SSL_read(ssl, buf, size);

        if (bytes < 1) {

            n = SSL_get_error(ssl, bytes);

            fprintf(stdout, "SSL failed bytes %zi err %i\n", bytes, n);

            switch (n) {

            case SSL_ERROR_NONE:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_NONE - retry\n");
                break;
            case SSL_ERROR_WANT_READ:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_READ - retry\n");
                break;
            case SSL_ERROR_WANT_WRITE:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_WRITE - retry\n");
                break;
            case SSL_ERROR_WANT_CONNECT:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_CONNECT - retry\n");
                break;
            case SSL_ERROR_WANT_ACCEPT:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_ACCEPT - retry\n");
                break;
            case SSL_ERROR_WANT_X509_LOOKUP:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_X509_LOOKUP - retry\n");
                break;

            case SSL_ERROR_ZERO_RETURN:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_ZERO_RETURN - close\n");
                retry = false;
                break;

            case SSL_ERROR_SYSCALL:

                errorcode = ERR_get_error();
                ERR_error_string_n(errorcode, errorstring,
                                   OV_SSL_ERROR_STRING_BUFFER_SIZE);

                fprintf(stdout, "SSL_ERROR_SYSCALL - stop %i|%s errno %i|%s\n",
                        errorcode, errorstring, errno, strerror(errno));

                retry = false;
                break;

            case SSL_ERROR_SSL:

                errorcode = ERR_get_error();
                ERR_error_string_n(errorcode, errorstring,
                                   OV_SSL_ERROR_STRING_BUFFER_SIZE);

                fprintf(stdout, "SSL_ERROR_SSL - stop %i|%s errno %i|%s\n",
                        errorcode, errorstring, errno, strerror(errno));

                retry = false;
                break;

            default:
                fprintf(stdout, "DEFAULT REACHED");
                testrun(1 == 0);
                break;
            }
        }
    }

    // not handshaked yet, will return with close
    testrun(n == SSL_ERROR_ZERO_RETURN);
    testrun(bytes == 0);

    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // check conn
    for (size_t i = 0; i < srv->connections_max; i++) {
        testrun(-1 == srv->conn[i].fd);
    }

    // reconnect client
    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(
                     ssl, (char *)srv->domain.array[0].config.name.start));

    /* start handshaking */
    errorcode = -1;
    err = 0;
    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);
    if (test_debug_log)
        fprintf(stdout, "-------------------- handshake done\n");
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    // search new conn
    for (size_t i = 0; i < srv->connections_max; i++) {

        if (srv->conn[i].fd != -1) {
            conn = &srv->conn[i];
            break;
        }
    }
    // we set the connection handshake in NOT done to test the function
    testrun(conn->tls.handshaked == true);

    if (test_debug_log)
        fprintf(stdout, "... using connection with socket %i\n", conn->fd);

    // No conn input and no srv input is assert guarded and will terminate
    // the process, so we do not test it here, as it want close the conn
    // on failure anyway, as the conn is no input to the function

    // positive test
    testrun(tls_send_buffer(srv, conn, buffer));
    bytes = -1;
    n = 0;
    retry = true;

    while (retry) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        usleep(TEST_RUNTIME_USECS);
        bytes = SSL_read(ssl, buf, size);

        if (bytes < 1) {

            n = SSL_get_error(ssl, bytes);

            fprintf(stdout, "SSL failed bytes %zi err %i\n", bytes, n);

            switch (n) {

            case SSL_ERROR_NONE:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_NONE - retry\n");
                break;
            case SSL_ERROR_WANT_READ:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_READ - retry\n");
                break;
            case SSL_ERROR_WANT_WRITE:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_WRITE - retry\n");
                break;
            case SSL_ERROR_WANT_CONNECT:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_CONNECT - retry\n");
                break;
            case SSL_ERROR_WANT_ACCEPT:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_ACCEPT - retry\n");
                break;
            case SSL_ERROR_WANT_X509_LOOKUP:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_X509_LOOKUP - retry\n");
                break;

            case SSL_ERROR_ZERO_RETURN:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_ZERO_RETURN - close\n");
                retry = false;
                break;

            case SSL_ERROR_SYSCALL:

                errorcode = ERR_get_error();
                ERR_error_string_n(errorcode, errorstring,
                                   OV_SSL_ERROR_STRING_BUFFER_SIZE);

                fprintf(stdout, "SSL_ERROR_SYSCALL - stop %i|%s errno %i|%s\n",
                        errorcode, errorstring, errno, strerror(errno));

                retry = false;
                break;

            case SSL_ERROR_SSL:

                errorcode = ERR_get_error();
                ERR_error_string_n(errorcode, errorstring,
                                   OV_SSL_ERROR_STRING_BUFFER_SIZE);

                fprintf(stdout, "SSL_ERROR_SSL - stop %i|%s errno %i|%s\n",
                        errorcode, errorstring, errno, strerror(errno));

                retry = false;
                break;

            default:
                fprintf(stdout, "DEFAULT REACHED\n");
                testrun(1 == 0);
                break;
            }

        } else {
            break;
        }
    }

    // expect we read something
    testrun(bytes > 0);
    testrun(bytes == (ssize_t)strlen(expect));
    testrun(0 == strncmp(expect, (char *)buf, bytes));
    testrun(buffer);

    if (test_debug_log)
        fprintf(stdout,
                "--------------------------------------------------- x");
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

#if (OV_ARCH == OV_MACOS)

    /*
     *  We limit the send buffer of the connection socket to 4 byte, to test
     *  partial message sent.
     *
     *  NOTE setsockopt will not work as requiered for this test
     *  using LINUX as standard user, so we do the test only when running
     *  on the MAC.
     */

    expect = "0000111122223333444455";

    size = 4;
    testrun(0 == setsockopt(conn->fd, SOL_SOCKET, SO_SNDBUF,
                            (const void *)&size, sizeof(size)));

    testrun(ov_buffer_set(buffer, expect, strlen(expect)));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // check state
    ov_buffer *out = NULL;
    testrun(NULL == conn->io.out.buffer);
    out = ov_list_queue_pop(conn->io.out.queue);
    testrun(out == NULL);

    fprintf(stdout, "\ntry to send on %i\n", conn->fd);

    testrun(tls_send_buffer(srv, conn, buffer));

    testrun(NULL == conn->io.out.buffer);

    bytes = -1;
    n = 0;
    retry = true;

    /*
     *  We expect the data is NOT send (no partial write enabled in SSL)
     *  and the buffer is queued within the send queue
     */

    testrun(NULL != buffer);

    buffer = ov_buffer_free(buffer);
    buffer = NULL;

    buffer = ov_list_queue_pop(conn->io.out.queue);
    testrun(ov_buffer_cast(buffer));
    testrun(4 == buffer->length);
    testrun(0 == strncmp(expect, (char *)buffer->start, buffer->length));
    testrun(0 == strncmp("0000", (char *)buffer->start, buffer->length));

    /*
     *  Now we let the eventloop run and transmit the rest of the buffer.
     */

    for (size_t i = 0; i < 5; i++) {

        memset(buf, 0, size);

        retry = true;

        while (retry) {

            testrun(loop->run(loop, TEST_RUNTIME_USECS));
            usleep(TEST_RUNTIME_USECS);
            bytes = SSL_read(ssl, buf, size);

            if (bytes < 1) {

                n = SSL_get_error(ssl, bytes);
                switch (n) {

                case SSL_ERROR_NONE:
                case SSL_ERROR_WANT_READ:
                case SSL_ERROR_WANT_WRITE:
                case SSL_ERROR_WANT_CONNECT:
                case SSL_ERROR_WANT_ACCEPT:
                case SSL_ERROR_WANT_X509_LOOKUP:
                    // try againg
                    break;

                case SSL_ERROR_ZERO_RETURN:
                case SSL_ERROR_SYSCALL:
                case SSL_ERROR_SSL:

                    errorcode = ERR_get_error();
                    ERR_error_string_n(errorcode, errorstring,
                                       OV_SSL_ERROR_STRING_BUFFER_SIZE);

                    fprintf(stdout, "Stop %i|%s errno %i|%s\n", errorcode,
                            errorstring, errno, strerror(errno));

                    retry = false;
                    break;

                default:
                    testrun(1 == 0);
                    break;
                }

            } else {
                break;
            }
        }

        switch (i) {

        case 0:
            testrun(bytes == 4);
            testrun(0 == strncmp("1111", (char *)buf, bytes));
            break;

        case 1:
            testrun(bytes == 4);
            testrun(0 == strncmp("2222", (char *)buf, bytes));
            break;

        case 2:
            testrun(bytes == 4);
            testrun(0 == strncmp("3333", (char *)buf, bytes));
            break;

        case 3:
            testrun(bytes == 4);
            testrun(0 == strncmp("4444", (char *)buf, bytes));
            break;

        case 4:
            testrun(bytes == 2);
            testrun(0 == strncmp("55", (char *)buf, bytes));
            break;
        }
    }

    /* Check we we not receive any more data now */

    for (size_t i = 0; i < 5; i++) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        usleep(TEST_RUNTIME_USECS);
        bytes = SSL_read(ssl, buf, size);
        testrun(bytes == -1);
    }

#endif

    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    buffer = ov_buffer_free(buffer);

    // check conn
    for (size_t i = 0; i < srv->connections_max; i++) {
        testrun(-1 == srv->conn[i].fd);
    }

    if (test_debug_log)
        fprintf(stdout, "-------------------- cleanup\n");

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(NULL == ov_webserver_base_free(srv));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool dummy_ws_callback(void *userdata, int socket,
                              const ov_memory_pointer domain, const char *uri,
                              ov_memory_pointer content, bool text) {

    if (!userdata || socket < 1)
        goto error;

    if (domain.start) { /* unused */
    };
    if (uri) { /* unused */
    };

    int x = *(int *)userdata;

    if (test_debug_log)
        fprintf(stdout, "dummy_ws_callback at %i | %i text %s bytes %zu\n",
                socket, x, text ? "true" : "false", content.length);

    x++;

    *(int *)userdata = x;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool dummy_ws_callback_fragmented_copy_to_userdata(
    void *userdata, int socket, const ov_memory_pointer domain, const char *uri,
    ov_websocket_frame *frame) {

    if (!userdata || socket < 1 || !frame)
        goto error;

    if (domain.start) { /* unused */
    };
    if (uri) { /* unused */
    };

    ov_buffer *buffer = ov_buffer_cast(userdata);
    if (!ov_buffer_clear(buffer))
        goto error;

    if (!ov_buffer_set(buffer, frame->buffer->start, frame->buffer->length))
        goto error;

    ov_websocket_frame_free(frame);

    if (test_debug_log)
        fprintf(stdout, "fragmented_copy_to_userdata at %i\n", socket);
    return true;
error:
    ov_websocket_frame_free(frame);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool dummy_ws_callback_copy_to_userdata(void *userdata, int socket,
                                               const ov_memory_pointer domain,
                                               const char *uri,
                                               ov_memory_pointer content,
                                               bool text) {

    if (!userdata || socket < 1)
        goto error;

    if (domain.start) { /* unused */
    };
    if (uri) { /* unused */
    };

    ov_buffer *buffer = ov_buffer_cast(userdata);
    if (!ov_buffer_clear(buffer))
        goto error;

    if (!ov_buffer_set(buffer, content.start, content.length))
        goto error;

    if (text) { /* ignore */
    };

    if (test_debug_log)
        fprintf(stdout, "copy_to_userdata at %i\n", socket);
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_base_configure_websocket_callback() {

    int data = 0;
    void *userdata = &data;

    int other = 0;
    void *otherdata = &other;

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = (ov_webserver_base_config){
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 10 * 1000 * 1000,
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0}};

    set_resources_dir(config.domain_config_path);

    ov_webserver_base *srv = ov_webserver_base_create(config);
    testrun(srv);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    ov_domain *domain = &srv->domain.array[0];

    testrun(0 == memcmp("openvocs.test", domain->config.name.start,
                        domain->config.name.length));

    ov_memory_pointer hostname = {
        .start = srv->domain.array[0].config.name.start,
        .length = srv->domain.array[0].config.name.length};

    ov_websocket_message_config ws_config = {0};

    testrun(!ov_webserver_base_configure_websocket_callback(NULL, hostname,
                                                            ws_config));

    hostname.start = NULL;

    testrun(!ov_webserver_base_configure_websocket_callback(srv, hostname,
                                                            ws_config));

    hostname.start = domain->config.name.start;
    hostname.length = 0;

    testrun(!ov_webserver_base_configure_websocket_callback(srv, hostname,
                                                            ws_config));

    hostname.length = domain->config.name.length;

    // min input == reset
    testrun(ov_webserver_base_configure_websocket_callback(srv, hostname,
                                                           ws_config));

    testrun(NULL == domain->websocket.config.userdata);
    testrun(NULL == domain->websocket.config.callback);
    testrun(NULL == domain->websocket.uri);

    ws_config.callback = dummy_ws_callback;

    // unset all callback
    testrun(ov_webserver_base_configure_websocket_callback(srv, hostname,
                                                           ws_config));

    testrun(NULL == domain->websocket.config.userdata);
    testrun(NULL == domain->websocket.config.callback);
    testrun(NULL == domain->websocket.uri);

    // set generic callback
    ws_config.userdata = userdata;
    testrun(ov_webserver_base_configure_websocket_callback(srv, hostname,
                                                           ws_config));

    testrun(userdata == domain->websocket.config.userdata);
    testrun(dummy_ws_callback == domain->websocket.config.callback);
    testrun(NULL == domain->websocket.uri);

    // set uri callback
    memset(ws_config.uri, 0, PATH_MAX);
    strcat(ws_config.uri, "/abc");
    testrun(ov_webserver_base_configure_websocket_callback(srv, hostname,
                                                           ws_config));

    testrun(userdata == domain->websocket.config.userdata);
    testrun(dummy_ws_callback == domain->websocket.config.callback);
    testrun(!ov_dict_is_empty(domain->websocket.uri));
    testrun(1 == ov_dict_count(domain->websocket.uri));
    ov_websocket_message_config *out =
        ov_dict_get(domain->websocket.uri, "/abc");
    testrun(out);
    testrun(dummy_ws_callback == out->callback);

    // try to set uri callback with different userdata
    ws_config.userdata = otherdata;
    testrun(!ov_webserver_base_configure_websocket_callback(srv, hostname,
                                                            ws_config));

    testrun(userdata == domain->websocket.config.userdata);
    testrun(dummy_ws_callback == domain->websocket.config.callback);
    testrun(!ov_dict_is_empty(domain->websocket.uri));
    testrun(1 == ov_dict_count(domain->websocket.uri));
    out = ov_dict_get(domain->websocket.uri, "/abc");
    testrun(out);
    testrun(dummy_ws_callback == out->callback);

    // set uri callback
    ws_config.userdata = userdata;
    memset(ws_config.uri, 0, PATH_MAX);
    strcat(ws_config.uri, "/chat");
    testrun(ov_webserver_base_configure_websocket_callback(srv, hostname,
                                                           ws_config));

    testrun(userdata == domain->websocket.config.userdata);
    testrun(dummy_ws_callback == domain->websocket.config.callback);
    testrun(!ov_dict_is_empty(domain->websocket.uri));
    testrun(2 == ov_dict_count(domain->websocket.uri));
    out = ov_dict_get(domain->websocket.uri, "/abc");
    testrun(out);
    testrun(dummy_ws_callback == out->callback);
    out = ov_dict_get(domain->websocket.uri, "/chat");
    testrun(out);
    testrun(dummy_ws_callback == out->callback);

    // reset
    ws_config = (ov_websocket_message_config){0};
    memset(ws_config.uri, 0, PATH_MAX);
    testrun(ov_webserver_base_configure_websocket_callback(srv, hostname,
                                                           ws_config));

    testrun(NULL == domain->websocket.config.userdata);
    testrun(NULL == domain->websocket.config.callback);
    testrun(NULL == domain->websocket.uri);

    // try to set to unenabled domain
    hostname.start = (uint8_t *)"test.com";
    hostname.length = 8;

    testrun(!ov_webserver_base_configure_websocket_callback(srv, hostname,
                                                            ws_config));

    memset(ws_config.uri, 0, PATH_MAX);
    strcat(ws_config.uri, "/chat");
    testrun(!ov_webserver_base_configure_websocket_callback(srv, hostname,
                                                            ws_config));

    testrun(!ov_webserver_base_configure_websocket_callback(srv, hostname,
                                                            ws_config));

    /* Check URI only usage */

    hostname.start = domain->config.name.start;
    hostname.length = domain->config.name.length;

    ws_config = (ov_websocket_message_config){.userdata = userdata,
                                              .callback = dummy_ws_callback};

    memset(ws_config.uri, 0, PATH_MAX);
    strcat(ws_config.uri, "/1");
    testrun(ov_webserver_base_configure_websocket_callback(srv, hostname,
                                                           ws_config));

    memset(ws_config.uri, 0, PATH_MAX);
    strcat(ws_config.uri, "/2");
    testrun(ov_webserver_base_configure_websocket_callback(srv, hostname,
                                                           ws_config));

    memset(ws_config.uri, 0, PATH_MAX);
    strcat(ws_config.uri, "/3");
    testrun(ov_webserver_base_configure_websocket_callback(srv, hostname,
                                                           ws_config));

    testrun(NULL == domain->websocket.config.userdata);
    testrun(NULL == domain->websocket.config.callback);
    testrun(3 == ov_dict_count(domain->websocket.uri));
    testrun(ov_dict_get(domain->websocket.uri, "/1"));
    testrun(ov_dict_get(domain->websocket.uri, "/2"));
    testrun(ov_dict_get(domain->websocket.uri, "/3"));

    // clear uri specific callback

    ws_config = (ov_websocket_message_config){.userdata = userdata};

    memset(ws_config.uri, 0, PATH_MAX);
    strcat(ws_config.uri, "/2");
    testrun(ov_webserver_base_configure_websocket_callback(srv, hostname,
                                                           ws_config));

    testrun(NULL == domain->websocket.config.userdata);
    testrun(NULL == domain->websocket.config.callback);
    testrun(2 == ov_dict_count(domain->websocket.uri));
    testrun(ov_dict_get(domain->websocket.uri, "/1"));
    testrun(NULL == ov_dict_get(domain->websocket.uri, "/2"));
    testrun(ov_dict_get(domain->websocket.uri, "/3"));

    fprintf(stdout, "-------------------- cleanup\n");

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(NULL == ov_webserver_base_free(srv));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}
/*----------------------------------------------------------------------------*/

struct dummy_event_data {

    int socket;
    ov_json_value *value;
};

/*----------------------------------------------------------------------------*/

static bool dummy_event_callback(void *userdata, const int socket,
                                 const ov_event_parameter *para,
                                 ov_json_value *input) {

    if (!userdata)
        return false;

    UNUSED(para);

    struct dummy_event_data *data = (struct dummy_event_data *)userdata;
    data->socket = socket;
    data->value = input;
    return true;
}

/*----------------------------------------------------------------------------*/

static void dummy_event_close(void *userdata, const int socket) {

    if (!userdata)
        return;

    struct dummy_event_data *data = (struct dummy_event_data *)userdata;
    data->socket = socket;
    return;
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_base_configure_uri_event_io() {

    struct dummy_event_data data = {0};
    void *userdata = &data;

    struct dummy_event_data other = {0};
    void *otherdata = &other;

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = (ov_webserver_base_config){
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 10 * 1000 * 1000,
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0}};

    set_resources_dir(config.domain_config_path);

    ov_webserver_base *srv = ov_webserver_base_create(config);
    testrun(srv);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    ov_domain *domain = &srv->domain.array[0];

    testrun(0 == memcmp("openvocs.test", domain->config.name.start,
                        domain->config.name.length));

    ov_memory_pointer hostname = {
        .start = srv->domain.array[0].config.name.start,
        .length = srv->domain.array[0].config.name.length};

    ov_event_io_config event_config = {0};

    testrun(!ov_webserver_base_configure_uri_event_io(NULL, hostname,
                                                      event_config, NULL));

    hostname.start = NULL;

    testrun(!ov_webserver_base_configure_uri_event_io(srv, hostname,
                                                      event_config, NULL));

    hostname.start = domain->config.name.start;
    hostname.length = 0;

    testrun(!ov_webserver_base_configure_uri_event_io(srv, hostname,
                                                      event_config, NULL));

    hostname.length = domain->config.name.length;

    testrun(NULL == domain->websocket.config.userdata);
    testrun(NULL == domain->websocket.config.callback);
    testrun(NULL == domain->websocket.uri);

    // reset NO userdata set
    testrun(ov_webserver_base_configure_uri_event_io(srv, hostname,
                                                     event_config, NULL));

    testrun(NULL == domain->websocket.config.userdata);
    testrun(NULL == domain->websocket.config.callback);
    testrun(NULL == domain->websocket.uri);

    event_config.callback.process = dummy_event_callback;
    event_config.callback.close = dummy_event_close;

    // reset NO userdata set
    testrun(ov_webserver_base_configure_uri_event_io(srv, hostname,
                                                     event_config, NULL));

    testrun(NULL == domain->websocket.config.userdata);
    testrun(NULL == domain->websocket.config.callback);
    testrun(NULL == domain->websocket.uri);
    testrun(NULL == domain->event_handler.uri);

    // set generic callback
    event_config.userdata = userdata;
    testrun(!ov_webserver_base_configure_uri_event_io(srv, hostname,
                                                      event_config, NULL));

    testrun(NULL == domain->websocket.config.userdata);
    testrun(NULL == domain->websocket.config.callback);
    testrun(NULL == domain->websocket.uri);
    testrun(NULL == domain->event_handler.uri);

    // min valid input
    strcat(event_config.name, "/");
    testrun(ov_webserver_base_configure_uri_event_io(srv, hostname,
                                                     event_config, NULL));

    testrun(NULL == domain->websocket.config.close);
    testrun(NULL == domain->websocket.config.callback);
    testrun(NULL != domain->websocket.uri);
    testrun(NULL != domain->event_handler.uri);

    testrun(!ov_dict_is_empty(domain->websocket.uri));
    testrun(1 == ov_dict_count(domain->websocket.uri));
    testrun(!ov_dict_is_empty(domain->event_handler.uri));
    testrun(1 == ov_dict_count(domain->event_handler.uri));
    ov_websocket_message_config *out = ov_dict_get(domain->websocket.uri, "/");
    testrun(out);
    testrun(cb_wss_io_to_json == out->callback);
    testrun(dummy_event_close == out->close);
    ov_event_io_config *out_event = ov_dict_get(domain->event_handler.uri, "/");
    testrun(out_event);
    testrun(dummy_event_callback == out_event->callback.process);
    testrun(userdata == out_event->userdata);

    // try to override with different userdata
    event_config.userdata = otherdata;
    testrun(!ov_webserver_base_configure_uri_event_io(srv, hostname,
                                                      event_config, NULL));

    testrun(NULL == domain->websocket.config.userdata);
    testrun(NULL == domain->websocket.config.callback);
    testrun(1 == ov_dict_count(domain->websocket.uri));
    testrun(!ov_dict_is_empty(domain->event_handler.uri));
    testrun(1 == ov_dict_count(domain->event_handler.uri));
    out = ov_dict_get(domain->websocket.uri, "/");
    testrun(out);
    testrun(cb_wss_io_to_json == out->callback);
    testrun(dummy_event_close == out->close);
    out_event = ov_dict_get(domain->event_handler.uri, "/");
    testrun(out_event);
    testrun(dummy_event_callback == out_event->callback.process);
    testrun(userdata == out_event->userdata);

    // set uri callback
    event_config.userdata = userdata;
    memset(event_config.name, 0, PATH_MAX);
    strcat(event_config.name, "/chat");
    testrun(ov_webserver_base_configure_uri_event_io(srv, hostname,
                                                     event_config, NULL));

    testrun(NULL == domain->websocket.config.userdata);
    testrun(NULL == domain->websocket.config.callback);
    testrun(2 == ov_dict_count(domain->websocket.uri));
    testrun(!ov_dict_is_empty(domain->event_handler.uri));
    testrun(2 == ov_dict_count(domain->event_handler.uri));

    out = ov_dict_get(domain->websocket.uri, "/");
    testrun(out);
    testrun(cb_wss_io_to_json == out->callback);
    out = ov_dict_get(domain->websocket.uri, "/chat");
    testrun(out);
    testrun(cb_wss_io_to_json == out->callback);

    out_event = ov_dict_get(domain->event_handler.uri, "/");
    testrun(out_event);
    testrun(dummy_event_callback == out_event->callback.process);
    testrun(userdata == out_event->userdata);

    out_event = ov_dict_get(domain->event_handler.uri, "/chat");
    testrun(out_event);
    testrun(dummy_event_callback == out_event->callback.process);
    testrun(userdata == out_event->userdata);

    // reset
    event_config = (ov_event_io_config){0};
    memset(event_config.name, 0, PATH_MAX);
    testrun(ov_webserver_base_configure_uri_event_io(srv, hostname,
                                                     event_config, NULL));

    testrun(NULL == domain->websocket.config.userdata);
    testrun(NULL == domain->websocket.config.callback);
    testrun(NULL == domain->websocket.uri);

    // try to set to unenabled domain
    hostname.start = (uint8_t *)"test.com";
    hostname.length = 8;

    testrun(!ov_webserver_base_configure_uri_event_io(srv, hostname,
                                                      event_config, NULL));

    memset(event_config.name, 0, PATH_MAX);
    strcat(event_config.name, "/chat");
    testrun(!ov_webserver_base_configure_uri_event_io(srv, hostname,
                                                      event_config, NULL));

    testrun(!ov_webserver_base_configure_uri_event_io(srv, hostname,
                                                      event_config, NULL));

    /* Check URI only usage */

    hostname.start = domain->config.name.start;
    hostname.length = domain->config.name.length;

    event_config =
        (ov_event_io_config){.userdata = userdata,
                             .callback.process = dummy_event_callback,
                             .callback.close = dummy_event_close};

    memset(event_config.name, 0, PATH_MAX);
    strcat(event_config.name, "/1");
    testrun(ov_webserver_base_configure_uri_event_io(srv, hostname,
                                                     event_config, NULL));

    memset(event_config.name, 0, PATH_MAX);
    strcat(event_config.name, "/2");
    testrun(ov_webserver_base_configure_uri_event_io(srv, hostname,
                                                     event_config, NULL));

    memset(event_config.name, 0, PATH_MAX);
    strcat(event_config.name, "/3");
    testrun(ov_webserver_base_configure_uri_event_io(srv, hostname,
                                                     event_config, NULL));

    testrun(NULL == domain->websocket.config.userdata);
    testrun(NULL == domain->websocket.config.callback);
    testrun(3 == ov_dict_count(domain->websocket.uri));
    testrun(ov_dict_get(domain->websocket.uri, "/1"));
    testrun(ov_dict_get(domain->websocket.uri, "/2"));
    testrun(ov_dict_get(domain->websocket.uri, "/3"));
    testrun(3 == ov_dict_count(domain->event_handler.uri));
    testrun(ov_dict_get(domain->event_handler.uri, "/1"));
    testrun(ov_dict_get(domain->event_handler.uri, "/2"));
    testrun(ov_dict_get(domain->event_handler.uri, "/3"));

    // clear uri specific callback (not allowed)

    event_config = (ov_event_io_config){.userdata = userdata};

    memset(event_config.name, 0, PATH_MAX);
    strcat(event_config.name, "/2");
    testrun(!ov_webserver_base_configure_uri_event_io(srv, hostname,
                                                      event_config, NULL));

    testrun(NULL == domain->websocket.config.userdata);
    testrun(NULL == domain->websocket.config.callback);
    testrun(3 == ov_dict_count(domain->websocket.uri));
    testrun(ov_dict_get(domain->websocket.uri, "/1"));
    testrun(ov_dict_get(domain->websocket.uri, "/2"));
    testrun(ov_dict_get(domain->websocket.uri, "/3"));
    testrun(3 == ov_dict_count(domain->event_handler.uri));
    testrun(ov_dict_get(domain->event_handler.uri, "/1"));
    testrun(ov_dict_get(domain->event_handler.uri, "/2"));
    testrun(ov_dict_get(domain->event_handler.uri, "/3"));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(NULL == ov_webserver_base_free(srv));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_websocket_callback_fragmentation() {

    ov_buffer *buffer = ov_buffer_create(1000);
    testrun(buffer);

    uint8_t key[OV_WEBSOCKET_SECURE_KEY_SIZE + 1];
    memset(key, 0, OV_WEBSOCKET_SECURE_KEY_SIZE + 1);

    testrun(ov_websocket_generate_secure_websocket_key(
        key, OV_WEBSOCKET_SECURE_KEY_SIZE));

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = (ov_webserver_base_config){
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 1000 * 1000,
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0}};

    set_resources_dir(config.domain_config_path);

    ov_webserver_base *srv = ov_webserver_base_create(config);
    testrun(srv);

    /* prepare env */

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    ov_domain *openvocs = &srv->domain.array[0];

    ov_websocket_message_config ws_config = (ov_websocket_message_config){
        .userdata = buffer,
        .callback = dummy_ws_callback_copy_to_userdata,
        .fragmented = dummy_ws_callback_fragmented_copy_to_userdata};

    testrun(ov_webserver_base_configure_websocket_callback(
        srv,
        (ov_memory_pointer){.start = openvocs->config.name.start,
                            .length = openvocs->config.name.length},
        ws_config));

    /* create some client connection */
    int errorcode = -1;
    int err = 0;
    int r = 0;

    int client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL *ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 ==
            SSL_set_tlsext_host_name(ssl, (char *)openvocs->config.name.start));
    const char *ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 == strcmp(ptr, "openvocs.test"));
    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    // TLS to openvocs.test opened start websocket upgrade
    ov_http_message_config http_config = (ov_http_message_config){0};
    ov_http_version http_version = (ov_http_version){.major = 1, .minor = 1};

    ov_http_message *msg =
        ov_http_create_request_string(http_config, http_version, "GET", "/");
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

    if (test_debug_log)
        fprintf(stdout, "CHECK openvocs.test with request /\n");
    ssize_t bytes = -1;

    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, msg->buffer->start, msg->buffer->length);
    }

    testrun(bytes == (ssize_t)msg->buffer->length);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    Connection *conn = NULL;
    for (size_t i = 0; i < srv->connections_max; i++) {

        if (srv->conn[i].fd != -1) {
            conn = &srv->conn[i];
            break;
        }
    }

    /* Read the reponse empty to check for close later */

    char buf[10000] = {0};
    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_read(ssl, buf, 10000);
    }

    testrun(bytes > 0);
    char *expect = "HTTP/1.1 101 Switching Protocols";
    testrun(0 == strncmp(buf, expect, strlen(expect)));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    bytes = SSL_read(ssl, buf, 10000);
    testrun(bytes == -1);

    testrun(conn->websocket.queue == NULL);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_NONE);
    testrun(conn->websocket.counter == 0);

    /* Send some NONE fragmented message */

    memset(buf, 0, 1000);
    buf[0] = 0x81;
    buf[1] = 4;
    buf[2] = 't';
    buf[3] = 'e';
    buf[4] = 's';
    buf[5] = 't';

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    testrun(6 == buffer->length);
    testrun(0 == memcmp(buf, buffer->start, buffer->length));

    /* Send some FRAGMENTED START */

    buf[0] = 0x01;
    buf[1] = 4;
    buf[2] = 't';
    buf[3] = 'e';
    buf[4] = 's';
    buf[5] = 't';

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    testrun(6 == buffer->length);
    testrun(0 == memcmp(buf, buffer->start, buffer->length));

    /* Resend some FRAGMENTED START,
     * expect delivery, as non fragmented mode is used */
    ov_buffer_clear(buffer);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    testrun(6 == buffer->length);
    testrun(0 == memcmp(buf, buffer->start, buffer->length));

    /* send some NONE fragmented after start */

    buf[0] = 0x81;
    buf[1] = 4;
    buf[2] = 't';
    buf[3] = 'e';
    buf[4] = 's';
    buf[5] = 't';

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    testrun(6 == buffer->length);
    testrun(0 == memcmp(buf, buffer->start, buffer->length));

    /* Send some CONT */
    ov_buffer_clear(buffer);

    buf[0] = 0x00;
    buf[1] = 4;
    buf[2] = 't';
    buf[3] = 'e';
    buf[4] = 's';
    buf[5] = 't';

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // ov_dump_binary_as_hex(stdout, buffer->start, buffer->length);

    testrun(6 == buffer->length);
    testrun(0 == memcmp(buf, buffer->start, buffer->length));

    // check state of conn websocket data
    testrun(conn->websocket.queue == NULL);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_NONE);
    testrun(conn->websocket.counter == 0);

    /* change mode of config */
    conn->websocket.config.fragmented = NULL;

    /* send some NONE fragmented message */

    buf[0] = 0x81;
    buf[1] = 4;
    buf[2] = 't';
    buf[3] = 'e';
    buf[4] = 's';
    buf[5] = 't';

    ov_buffer_clear(buffer);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // content only delivery !
    testrun(4 == buffer->length);
    testrun(0 == memcmp(buf + 2, buffer->start, buffer->length));

    // check state at conn
    testrun(conn->websocket.queue == NULL);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_NONE);
    testrun(conn->websocket.counter == 0);

    /* send some fragmented start */

    ov_buffer_clear(buffer);

    buf[0] = 0x01;
    buf[1] = 4;
    buf[2] = 't';
    buf[3] = 'e';
    buf[4] = 's';
    buf[5] = 't';

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // no callback yet
    testrun(buffer->length == 0);

    // check state
    testrun(conn->websocket.queue != NULL);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_START);
    testrun(conn->websocket.counter == 1);

    /* send some fragment count */

    buf[0] = 0x00;
    buf[1] = 4;
    buf[2] = 't';
    buf[3] = 'e';
    buf[4] = 's';
    buf[5] = 't';

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // no callback yet
    testrun(buffer->length == 0);
    // check state
    testrun(conn->websocket.queue != NULL);
    testrun(conn->websocket.counter == 2);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);

    /* send some fragment count */

    buf[0] = 0x00;
    buf[1] = 4;
    buf[2] = 'A';
    buf[3] = 'B';
    buf[4] = 'C';
    buf[5] = 'D';

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // no callback yet
    testrun(buffer->length == 0);
    // check state
    testrun(conn->websocket.queue != NULL);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
    testrun(conn->websocket.counter == 3);

    /* send some NONE fragment LAST */

    buf[0] = 0x80;
    buf[1] = 4;
    buf[2] = '1';
    buf[3] = '2';
    buf[4] = '3';
    buf[5] = '4';

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // no callback yet
    testrun(buffer->length == 16);
    testrun(0 ==
            memcmp((char *)"testtestABCD1234", buffer->start, buffer->length));
    // check state
    testrun(conn->websocket.queue != NULL);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_LAST);
    testrun(conn->websocket.counter == 0);

    /* send some NONE fragmented message */

    buf[0] = 0x81;
    buf[1] = 4;
    buf[2] = 't';
    buf[3] = 'e';
    buf[4] = 's';
    buf[5] = 't';

    ov_buffer_clear(buffer);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // content only delivery !
    testrun(4 == buffer->length);
    testrun(0 == memcmp(buf + 2, buffer->start, buffer->length));

    // check state at conn
    testrun(conn->websocket.queue != NULL);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_NONE);
    testrun(conn->websocket.counter == 0);

    /* check close for non matching order */

    /* send some fragmented start */

    ov_buffer_clear(buffer);

    buf[0] = 0x01;
    buf[1] = 4;
    buf[2] = 't';
    buf[3] = 'e';
    buf[4] = 's';
    buf[5] = 't';

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // no callback yet
    testrun(buffer->length == 0);

    // check state
    testrun(conn->websocket.queue != NULL);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_START);
    testrun(conn->websocket.counter == 1);

    /* send some fragment count */

    buf[0] = 0x00;
    buf[1] = 4;
    buf[2] = 't';
    buf[3] = 'e';
    buf[4] = 's';
    buf[5] = 't';

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // no callback yet
    testrun(buffer->length == 0);
    // check state
    testrun(conn->websocket.queue != NULL);
    testrun(conn->websocket.counter == 2);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);

    /* send some NONE fragmented message */

    buf[0] = 0x81;
    buf[1] = 4;
    buf[2] = 't';
    buf[3] = 'e';
    buf[4] = 's';
    buf[5] = 't';

    ov_buffer_clear(buffer);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    bytes = -1;
    memset(buf, 0, 10000);
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_read(ssl, buf, 10000);
    }
    testrun(bytes > 4);
    // expect a wss close frame
    buf[2] = 0x03;
    buf[3] = 0xEB;

    // reset
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    while (0 != conn->websocket.close.wait_for_response) {
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
    }

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    for (size_t i = 0; i < srv->connections_max; i++) {

        testrun(srv->conn[i].fd == -1);
    }

    // reconnect
    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 ==
            SSL_set_tlsext_host_name(ssl, (char *)openvocs->config.name.start));
    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, msg->buffer->start, msg->buffer->length);
    }

    testrun(bytes == (ssize_t)msg->buffer->length);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    conn = NULL;
    for (size_t i = 0; i < srv->connections_max; i++) {

        if (srv->conn[i].fd != -1) {
            testrun(!conn);
            conn = &srv->conn[i];
        }
    }

    /* Read the reponse empty to check for close later */

    memset(buf, 0, 10000);
    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_read(ssl, buf, 10000);
    }

    testrun(bytes > 0);
    expect = "HTTP/1.1 101 Switching Protocols";
    testrun(0 == strncmp(buf, expect, strlen(expect)));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    bytes = SSL_read(ssl, buf, 10000);
    testrun(bytes == -1);

    /* new client connected */

    // check state at conn
    testrun(conn->websocket.queue == NULL);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_NONE);
    testrun(conn->websocket.counter == 0);
    testrun(conn->websocket.config.userdata == buffer);
    testrun(conn->websocket.config.max_frames == 0);
    testrun(conn->websocket.config.callback ==
            dummy_ws_callback_copy_to_userdata);
    testrun(conn->websocket.config.fragmented ==
            dummy_ws_callback_fragmented_copy_to_userdata);

    /* test max_frames */

    conn->websocket.config.max_frames = 5;
    conn->websocket.config.fragmented = NULL;

    ov_buffer_clear(buffer);

    buf[0] = 0x01;
    buf[1] = 4;
    buf[2] = 't';
    buf[3] = 'e';
    buf[4] = 's';
    buf[5] = 't';

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // no callback yet
    testrun(buffer->length == 0);

    // check state
    testrun(conn->websocket.queue != NULL);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_START);
    testrun(conn->websocket.counter == 1);

    /* send some fragment count */

    buf[0] = 0x00;
    buf[1] = 4;
    buf[2] = 't';
    buf[3] = 'e';
    buf[4] = 's';
    buf[5] = 't';

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // no callback yet
    testrun(buffer->length == 0);
    // check state
    testrun(conn->websocket.queue != NULL);
    testrun(conn->websocket.counter == 2);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // no callback yet
    testrun(buffer->length == 0);
    // check state
    testrun(conn->websocket.queue != NULL);
    testrun(conn->websocket.counter == 3);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // no callback yet
    testrun(buffer->length == 0);
    // check state
    testrun(conn->websocket.queue != NULL);
    testrun(conn->websocket.counter == 4);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);

    /* send some fragment close */

    buf[0] = 0x80;
    buf[1] = 4;
    buf[2] = 't';
    buf[3] = 'e';
    buf[4] = 's';
    buf[5] = 't';

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // callback done
    testrun(buffer->length != 0);
    expect = "testtesttesttesttest";
    testrun(0 == memcmp(expect, buffer->start, buffer->length));
    // check state
    testrun(conn->websocket.queue != NULL);
    testrun(conn->websocket.counter == 0);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_LAST);

    ov_buffer_clear(buffer);

    buf[0] = 0x01;
    buf[1] = 4;
    buf[2] = 't';
    buf[3] = 'e';
    buf[4] = 's';
    buf[5] = 't';

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // no callback yet
    testrun(buffer->length == 0);

    // check state
    testrun(conn->websocket.queue != NULL);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_START);
    testrun(conn->websocket.counter == 1);

    buf[0] = 0x00;
    buf[1] = 4;
    buf[2] = 't';
    buf[3] = 'e';
    buf[4] = 's';
    buf[5] = 't';

    for (size_t i = 0; i < 3; i++) {

        bytes = -1;
        while (bytes == -1) {

            testrun(loop->run(loop, TEST_RUNTIME_USECS));
            bytes = SSL_write(ssl, buf, 6);
        }

        testrun(bytes == 6);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));

        // no callback yet
        testrun(buffer->length == 0);
    }

    testrun(conn->websocket.queue != NULL);
    testrun(conn->websocket.counter == 4);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);

    // VERIFY STILL CONNECTED
    memset(buf, 0, 1000);
    for (size_t i = 0; i < 10; i++) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        testrun(-1 == SSL_read(ssl, buf, 1000));
    }

    memset(buf, 0, 1000);

    buf[0] = 0x00;
    buf[1] = 4;
    buf[2] = 't';
    buf[3] = 'e';
    buf[4] = 's';
    buf[5] = 't';

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // testrun(conn->websocket.close.send);

    bytes = -1;
    memset(buf, 0, 10000);
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_read(ssl, buf, 10000);
    }
    testrun(bytes > 4);
    // expect a wss close frame
    buf[2] = 0x03;
    buf[3] = 0xEB;

    /* wait for automatic closure of the connection */
    while (0 != conn->websocket.close.wait_for_response) {
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
    }

    testrun(conn->websocket.queue == NULL);
    testrun(conn->websocket.counter == 0);
    testrun(conn->websocket.last == OV_WEBSOCKET_FRAGMENTATION_NONE);

    // VERIFY CLOSE
    memset(buf, 0, 1000);
    bytes = -1;
    while (bytes == -1) {
        bytes = SSL_read(ssl, buf, 1000);
    }
    testrun(bytes == 0);

    // reset
    msg = ov_http_message_free(msg);
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    testrun(NULL == ov_buffer_free(buffer));
    testrun(NULL == ov_webserver_base_free(srv));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_websocket_callback() {

    int data[10] = {0};
    for (size_t i = 0; i < 10; i++) {
        data[i] = 1;
    };

    uint8_t key[OV_WEBSOCKET_SECURE_KEY_SIZE + 1];
    memset(key, 0, OV_WEBSOCKET_SECURE_KEY_SIZE + 1);

    testrun(ov_websocket_generate_secure_websocket_key(
        key, OV_WEBSOCKET_SECURE_KEY_SIZE));

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = (ov_webserver_base_config){
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 10 * 1000 * 1000,
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0}};

    set_resources_dir(config.domain_config_path);

    ov_webserver_base *srv = ov_webserver_base_create(config);
    testrun(srv);

    /* prepare env */

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    ov_domain *openvocs = &srv->domain.array[0];
    ov_domain *one = &srv->domain.array[1];
    ov_domain *two = &srv->domain.array[2];

    testrun(0 == memcmp("openvocs.test", openvocs->config.name.start,
                        openvocs->config.name.length));

    testrun(0 == memcmp("one.test", one->config.name.start,
                        one->config.name.length));

    testrun(0 == memcmp("two.test", two->config.name.start,
                        two->config.name.length));

    ov_websocket_message_config ws_config = (ov_websocket_message_config){
        .userdata = (void *)&data[0], .callback = dummy_ws_callback};

    testrun(ov_webserver_base_configure_websocket_callback(
        srv,
        (ov_memory_pointer){.start = openvocs->config.name.start,
                            .length = openvocs->config.name.length},
        ws_config));

    ws_config.userdata = (void *)&data[1];
    memset(ws_config.uri, 0, PATH_MAX);
    strcat(ws_config.uri, "/chat");
    testrun(ov_webserver_base_configure_websocket_callback(
        srv,
        (ov_memory_pointer){.start = one->config.name.start,
                            .length = one->config.name.length},
        ws_config));

    ws_config.userdata = (void *)&data[2];
    memset(ws_config.uri, 0, PATH_MAX);
    testrun(ov_webserver_base_configure_websocket_callback(
        srv,
        (ov_memory_pointer){.start = two->config.name.start,
                            .length = two->config.name.length},
        ws_config));

    memset(ws_config.uri, 0, PATH_MAX);
    strcat(ws_config.uri, "/1");
    testrun(ov_webserver_base_configure_websocket_callback(
        srv,
        (ov_memory_pointer){.start = two->config.name.start,
                            .length = two->config.name.length},
        ws_config));

    memset(ws_config.uri, 0, PATH_MAX);
    strcat(ws_config.uri, "/2");
    testrun(ov_webserver_base_configure_websocket_callback(
        srv,
        (ov_memory_pointer){.start = two->config.name.start,
                            .length = two->config.name.length},
        ws_config));

    memset(ws_config.uri, 0, PATH_MAX);
    strcat(ws_config.uri, "/3");
    testrun(ov_webserver_base_configure_websocket_callback(
        srv,
        (ov_memory_pointer){.start = two->config.name.start,
                            .length = two->config.name.length},
        ws_config));

    testrun((void *)&data[0] == openvocs->websocket.config.userdata);
    testrun(dummy_ws_callback == openvocs->websocket.config.callback);
    testrun(0 == openvocs->websocket.uri);

    testrun(NULL == one->websocket.config.userdata);
    testrun(NULL == one->websocket.config.callback);
    testrun(1 == ov_dict_count(one->websocket.uri));

    ov_websocket_message_config *ws_out =
        ov_dict_get(one->websocket.uri, "/chat");
    testrun(ws_out);
    testrun(ws_out->userdata == (void *)&data[1]);
    testrun(ws_out->callback == dummy_ws_callback);

    testrun((void *)&data[2] == two->websocket.config.userdata);
    testrun(dummy_ws_callback == two->websocket.config.callback);
    testrun(3 == ov_dict_count(two->websocket.uri));
    ws_out = ov_dict_get(two->websocket.uri, "/1");
    testrun(ws_out->userdata == (void *)&data[2]);
    testrun(ws_out->callback == dummy_ws_callback);
    ws_out = ov_dict_get(two->websocket.uri, "/2");
    testrun(ws_out->userdata == (void *)&data[2]);
    testrun(ws_out->callback == dummy_ws_callback);
    ws_out = ov_dict_get(two->websocket.uri, "/3");
    testrun(ws_out->userdata == (void *)&data[2]);
    testrun(ws_out->callback == dummy_ws_callback);

    /* env prepared */

    /* create some client connection */
    int errorcode = -1;
    int err = 0;
    int r = 0;

    int client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL *ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 ==
            SSL_set_tlsext_host_name(ssl, (char *)openvocs->config.name.start));
    const char *ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 == strcmp(ptr, "openvocs.test"));
    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    // TLS to openvocs.test opened start websocket upgrade
    ov_http_message_config http_config = (ov_http_message_config){0};
    ov_http_version http_version = (ov_http_version){.major = 1, .minor = 1};

    ov_http_message *msg =
        ov_http_create_request_string(http_config, http_version, "GET", "/");
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

    if (test_debug_log)
        fprintf(stdout, "CHECK openvocs.test with request /\n");

    ssize_t bytes = -1;

    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, msg->buffer->start, msg->buffer->length);
    }

    testrun(bytes == (ssize_t)msg->buffer->length);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // send some websocket frame
    char buf[100] = {0};
    buf[0] = 0x81;
    buf[1] = 4;
    buf[2] = 't';
    buf[3] = 'e';
    buf[4] = 's';
    buf[5] = 't';

    testrun(1 == data[0]);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(2 == data[0]);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(3 == data[0]);

    // reset
    msg = ov_http_message_free(msg);
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    // restart TLS
    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(ssl, (char *)one->config.name.start));
    ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 == strcmp(ptr, "one.test"));
    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    // We send to one.test which has no WSS callback,
    // but a callback at /chat, so it will reject any other upgrade

    msg = ov_http_create_request_string(http_config, http_version, "GET",
                                        "/unenabled");
    testrun(msg);
    testrun(
        ov_http_message_add_header_string(msg, OV_HTTP_KEY_HOST, "one.test"));
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

    if (test_debug_log)
        fprintf(stdout, "CHECK one.test with request /unenabled\n");

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, msg->buffer->start, msg->buffer->length);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
    }

    testrun(bytes == (ssize_t)msg->buffer->length);

    char read_buf[1000] = {0};
    bytes = SSL_read(ssl, read_buf, 1000);
    testrun(bytes == 0);

    // reset
    msg = ov_http_message_free(msg);
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    // restart TLS
    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(ssl, (char *)one->config.name.start));
    ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 == strcmp(ptr, "one.test"));
    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    // Check /chat is working at domain one

    msg = ov_http_create_request_string(http_config, http_version, "GET",
                                        "/chat");
    testrun(msg);
    testrun(
        ov_http_message_add_header_string(msg, OV_HTTP_KEY_HOST, "one.test"));
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

    if (test_debug_log)
        fprintf(stdout, "CHECK one.test with request /chat\n");

    testrun(1 == data[1]);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, msg->buffer->start, msg->buffer->length);
    }

    testrun(bytes == (ssize_t)msg->buffer->length);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(2 == data[1]);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(3 == data[1]);

    // reset
    msg = ov_http_message_free(msg);
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    // check different domain at TLS and Websocket level

    // restart TLS
    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(ssl, (char *)one->config.name.start));
    ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 == strcmp(ptr, "one.test"));
    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    msg = ov_http_create_request_string(http_config, http_version, "GET", "/");
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

    if (test_debug_log)
        fprintf(stdout, "CHECK one.test with request / at openvocs.test\n");

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, msg->buffer->start, msg->buffer->length);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
    }

    testrun(bytes == (ssize_t)msg->buffer->length);

    memset(read_buf, 0, 1000);
    bytes = SSL_read(ssl, read_buf, 1000);
    testrun(bytes == 0);

    // reset
    msg = ov_http_message_free(msg);
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    /* Here we test access request to / as well as spefific domains.
     *
     * After this we will test 3 different connections to /1 /2 /3 with
     * the same userdata pointer and check the usage of the same userdata
     * over 3 different URIs */

    // restart TLS
    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(ssl, (char *)two->config.name.start));
    ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 == strcmp(ptr, "two.test"));
    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    msg = ov_http_create_request_string(http_config, http_version, "GET", "/");
    testrun(msg);
    testrun(
        ov_http_message_add_header_string(msg, OV_HTTP_KEY_HOST, "two.test"));
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

    if (test_debug_log)
        fprintf(stdout, "CHECK two.test with request / at two.test\n");

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, msg->buffer->start, msg->buffer->length);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
    }

    testrun(bytes == (ssize_t)msg->buffer->length);

    testrun(1 == data[2]);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(2 == data[2]);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(3 == data[2]);

    // reset
    msg = ov_http_message_free(msg);
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    // restart TLS
    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(ssl, (char *)two->config.name.start));
    ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 == strcmp(ptr, "two.test"));
    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    msg = ov_http_create_request_string(http_config, http_version, "GET", "/1");
    testrun(msg);
    testrun(
        ov_http_message_add_header_string(msg, OV_HTTP_KEY_HOST, "two.test"));
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

    if (test_debug_log)
        fprintf(stdout, "CHECK two.test with request /1 at two.test\n");

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, msg->buffer->start, msg->buffer->length);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
    }

    testrun(bytes == (ssize_t)msg->buffer->length);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(4 == data[2]);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(5 == data[2]);

    // reset
    msg = ov_http_message_free(msg);
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    // restart TLS
    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(ssl, (char *)two->config.name.start));
    ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 == strcmp(ptr, "two.test"));
    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    msg = ov_http_create_request_string(http_config, http_version, "GET", "/2");
    testrun(msg);
    testrun(
        ov_http_message_add_header_string(msg, OV_HTTP_KEY_HOST, "two.test"));
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

    if (test_debug_log)
        fprintf(stdout, "CHECK two.test with request /2 at two.test\n");

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, msg->buffer->start, msg->buffer->length);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
    }

    testrun(bytes == (ssize_t)msg->buffer->length);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    testrun(5 == data[2]);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(6 == data[2]);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(7 == data[2]);

    // reset
    msg = ov_http_message_free(msg);
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    // restart TLS
    client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(ssl, (char *)two->config.name.start));
    ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 == strcmp(ptr, "two.test"));
    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    msg = ov_http_create_request_string(http_config, http_version, "GET", "/3");
    testrun(msg);
    testrun(
        ov_http_message_add_header_string(msg, OV_HTTP_KEY_HOST, "two.test"));
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

    if (test_debug_log)
        fprintf(stdout, "CHECK two.test with request /3 at two.test\n");

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, msg->buffer->start, msg->buffer->length);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
    }

    testrun(bytes == (ssize_t)msg->buffer->length);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    testrun(7 == data[2]);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(8 == data[2]);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_write(ssl, buf, 6);
    }

    testrun(bytes == 6);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(9 == data[2]);

    // reset
    msg = ov_http_message_free(msg);
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    if (test_debug_log)
        fprintf(stdout, "-------------------- cleanup\n");

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(NULL == ov_webserver_base_free(srv));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool accept_socket(void *userdata, int server, int connection) {

    if (test_debug_log)
        fprintf(stdout, "Connection at %i accepted %i\n", server, connection);

    int *data = userdata;

    for (size_t i = 0; i < 100; i++) {

        if (data[i] == 0) {
            data[i] = connection;
            break;
        }
    }

    return true;
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_base_send_secure() {

    int data[100] = {0};
    void *userdata = &data;

    uint8_t key[OV_WEBSOCKET_SECURE_KEY_SIZE + 1];
    memset(key, 0, OV_WEBSOCKET_SECURE_KEY_SIZE + 1);

    testrun(ov_websocket_generate_secure_websocket_key(
        key, OV_WEBSOCKET_SECURE_KEY_SIZE));

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = (ov_webserver_base_config){
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 10 * 1000 * 1000,
        .callback.userdata = userdata,
        .callback.accept = accept_socket,
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0}};

    set_resources_dir(config.domain_config_path);

    ov_webserver_base *srv = ov_webserver_base_create(config);
    testrun(srv);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    /* create some client connection */
    int errorcode = -1;
    int err = 0;
    int r = 0;

    int client[5] = {0};
    SSL_CTX *ctx[5] = {0};
    SSL *ssl[5] = {0};

    for (size_t i = 0; i < 5; i++) {

        client[i] = ov_socket_create(config.http.secure, true, NULL);
        testrun(client[i] > -1);
        testrun(ov_socket_ensure_nonblocking(client[i]));
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        ctx[i] = SSL_CTX_new(TLS_client_method());
        ssl[i] = SSL_new(ctx[i]);
        testrun(1 == SSL_set_fd(ssl[i], client[i]));
        SSL_set_connect_state(ssl[i]);
        SSL_CTX_set_client_hello_cb(ctx[i], dummy_client_hello_cb, NULL);
        testrun(1 == SSL_set_tlsext_host_name(
                         ssl[i], srv->domain.array[0].config.name.start));
        r = run_client_handshake(loop, ssl[i], &err, &errorcode);
        testrun(r == 1);

        // check connection socket accepted
        testrun(data[i] > 0);
    }

    /* 5 client connections opened */

    ov_buffer *buffer = ov_buffer_from_string("test1");

    testrun(!ov_webserver_base_send_secure(NULL, data[0], NULL));
    testrun(!ov_webserver_base_send_secure(NULL, data[0], buffer));

    testrun(ov_webserver_base_send_secure(srv, data[0], buffer));
    testrun(NULL != buffer);

    uint8_t buf[1000] = {0};
    ssize_t bytes = -1;

    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        usleep(TEST_RUNTIME_USECS);
        bytes = SSL_read(ssl[0], buf, 1000);
    }

    testrun(bytes == 5);
    testrun(0 == memcmp(buf, "test1", 5));
    memset(buf, 0, 1000);

    testrun(ov_buffer_set(buffer, "test2", 5));

    testrun(ov_webserver_base_send_secure(srv, data[1], buffer));
    testrun(NULL != buffer);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        usleep(TEST_RUNTIME_USECS);
        bytes = SSL_read(ssl[1], buf, 1000);
    }

    testrun(bytes == 5);
    testrun(0 == memcmp(buf, "test2", 5));
    memset(buf, 0, 1000);

    testrun(ov_buffer_set(buffer, "test3", 5));

    testrun(ov_webserver_base_send_secure(srv, data[0], buffer));
    testrun(NULL != buffer);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        usleep(TEST_RUNTIME_USECS);
        bytes = SSL_read(ssl[0], buf, 1000);
    }

    testrun(bytes == 5);
    testrun(0 == memcmp(buf, "test3", 5));
    memset(buf, 0, 1000);

    testrun(ov_buffer_set(buffer, "test4", 5));

    testrun(ov_webserver_base_send_secure(srv, data[4], buffer));
    testrun(NULL != buffer);

    bytes = -1;
    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        usleep(TEST_RUNTIME_USECS);
        bytes = SSL_read(ssl[4], buf, 1000);
    }

    testrun(bytes == 5);
    testrun(0 == memcmp(buf, "test4", 5));
    memset(buf, 0, 1000);

    // check send on unknown socket

    testrun(ov_buffer_set(buffer, "test5", 5));
    testrun(!ov_webserver_base_send_secure(srv, 12345, buffer));
    testrun(NULL != buffer);
    testrun(0 != buffer->start[0]);
    testrun(0 == memcmp("test5", buffer->start, buffer->length));

    if (test_debug_log)
        fprintf(stdout, "-------------------- cleanup\n");

    for (size_t i = 0; i < 5; i++) {

        SSL_CTX_free(ctx[i]);
        SSL_free(ssl[i]);
        close(client[i]);

        ctx[i] = NULL;
        ssl[i] = NULL;
    }

    testrun(NULL == ov_buffer_free(buffer));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(NULL == ov_webserver_base_free(srv));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_base_close() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = (ov_webserver_base_config){
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 10 * 1000 * 1000,
        .callback.userdata = NULL,
        .callback.accept = accept_socket,
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0}};

    set_resources_dir(config.domain_config_path);

    ov_webserver_base *srv = ov_webserver_base_create(config);
    testrun(srv);

    testrun(!ov_webserver_base_close(NULL, 0));
    testrun(!ov_webserver_base_close(srv, 0));
    testrun(!ov_webserver_base_close(NULL, 1));

    // socket not found
    testrun(!ov_webserver_base_close(srv, 1));

    // check close over socket close. we set some socket here
    int client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));

    srv->conn[client].fd = client;
    testrun(ov_webserver_base_close(srv, client));
    testrun(srv->conn[client].fd == -1);

    srv = ov_webserver_base_free(srv);
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_base_config_from_json() {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *itm = NULL;

    ov_socket_configuration http =
        (ov_socket_configuration){.type = TCP, .host = "localhost", .port = 80};

    ov_socket_configuration https = (ov_socket_configuration){
        .type = TCP, .host = "localhost", .port = 443};

    ov_socket_configuration stun_configs[] = {

        (ov_socket_configuration){.type = UDP, .host = "localhost", .port = 1},

        (ov_socket_configuration){.type = UDP, .host = "localhost", .port = 2},

        (ov_socket_configuration){.type = UDP, .host = "localhost", .port = 3}

    };

    size_t stun_config_items = sizeof(stun_configs) / sizeof(stun_configs[0]);

    out = ov_json_object();
    testrun(out);

    ov_webserver_base_config config = ov_webserver_base_config_from_json(NULL);
    testrun(config.name[0] == 0);
    testrun(config.domain_config_path[0] == 0);
    testrun(config.debug == false);
    testrun(config.ip4_only == false);
    testrun(socket_configs_equal(&config.http.redirect,
                                 &EMPTY_SOCKET_CONFIGURATION));
    testrun(
        socket_configs_equal(&config.http.secure, &EMPTY_SOCKET_CONFIGURATION));
    testrun(config.timer.io_timeout_usec == 0);
    testrun(config.timer.accept_to_io_timeout_usec == 0);
    for (size_t i = 0; i < OV_WEBSERVER_BASE_MAX_STUN_LISTENER; i++) {
        testrun(socket_configs_equal(&config.stun.socket[i],
                                     &EMPTY_SOCKET_CONFIGURATION));
    }

    config = ov_webserver_base_config_from_json(out);
    testrun(config.name[0] == 0);
    testrun(config.domain_config_path[0] == 0);
    testrun(config.debug == false);
    testrun(config.ip4_only == false);
    testrun(config.timer.io_timeout_usec == 0);
    testrun(config.timer.accept_to_io_timeout_usec == 0);
    testrun(socket_configs_equal(&config.http.redirect,
                                 &EMPTY_SOCKET_CONFIGURATION));
    testrun(
        socket_configs_equal(&config.http.secure, &EMPTY_SOCKET_CONFIGURATION));
    for (size_t i = 0; i < OV_WEBSERVER_BASE_MAX_STUN_LISTENER; i++) {
        testrun(socket_configs_equal(&config.stun.socket[i],
                                     &EMPTY_SOCKET_CONFIGURATION));
    }

    testrun(ov_json_object_set(out, OV_KEY_NAME, ov_json_string("name")));
    testrun(ov_json_object_set(out, OV_KEY_DEBUG, ov_json_true()));
    testrun(ov_json_object_set(out, OV_KEY_IP4_ONLY, ov_json_true()));
    testrun(ov_json_object_set(out, OV_KEY_DOMAINS, ov_json_string("path")));

    val = ov_json_object();
    testrun(ov_json_object_set(out, OV_KEY_TIMER, val));
    testrun(ov_json_object_set(val, OV_KEY_IO, ov_json_number(1)));
    testrun(ov_json_object_set(val, OV_KEY_ACCEPT, ov_json_number(2)));

    val = ov_json_object();
    testrun(ov_json_object_set(out, OV_KEY_SOCKETS, val));
    itm = NULL;
    testrun(ov_socket_configuration_to_json(http, &itm));
    testrun(ov_json_object_set(val, OV_KEY_HTTP, itm));
    itm = NULL;
    testrun(ov_socket_configuration_to_json(https, &itm));
    testrun(ov_json_object_set(val, OV_KEY_HTTPS, itm));
    ov_json_value *arr = ov_json_array();
    testrun(ov_json_object_set(val, OV_KEY_STUN, arr));
    for (size_t i = 0; i < stun_config_items; i++) {
        itm = NULL;
        testrun(ov_socket_configuration_to_json(stun_configs[i], &itm));
        testrun(ov_json_array_push(arr, itm));
    }

    char *str = ov_json_value_to_string(out);
    // fprintf(stdout, "%s\n", str);
    str = ov_data_pointer_free(str);

    config = ov_webserver_base_config_from_json(out);
    testrun(0 == strcmp("name", config.name));
    testrun(0 == strcmp("path", config.domain_config_path));
    testrun(config.debug == true);
    testrun(config.ip4_only == true);
    testrun(config.timer.io_timeout_usec == 1);
    testrun(config.timer.accept_to_io_timeout_usec == 2);
    testrun(socket_configs_equal(&config.http.redirect, &http));
    testrun(socket_configs_equal(&config.http.secure, &https));
    for (size_t i = 0; i < OV_WEBSERVER_BASE_MAX_STUN_LISTENER; i++) {
        if (i < stun_config_items) {
            testrun(
                socket_configs_equal(&config.stun.socket[i], &stun_configs[i]));
        } else {
            testrun(socket_configs_equal(&config.stun.socket[i],
                                         &EMPTY_SOCKET_CONFIGURATION));
        }
    }

    // same result if config is includes as key OV_KEY_WEBSERVER
    itm = ov_json_object();
    testrun(ov_json_object_set(itm, OV_KEY_WEBSERVER, out));
    config = ov_webserver_base_config_from_json(itm);
    testrun(0 == strcmp("name", config.name));
    testrun(0 == strcmp("path", config.domain_config_path));
    testrun(config.debug == true);
    testrun(config.ip4_only == true);
    testrun(config.timer.io_timeout_usec == 1);
    testrun(config.timer.accept_to_io_timeout_usec == 2);
    testrun(socket_configs_equal(&config.http.redirect, &http));
    testrun(socket_configs_equal(&config.http.secure, &https));
    for (size_t i = 0; i < OV_WEBSERVER_BASE_MAX_STUN_LISTENER; i++) {
        if (i < stun_config_items) {
            testrun(
                socket_configs_equal(&config.stun.socket[i], &stun_configs[i]));
        } else {
            testrun(socket_configs_equal(&config.stun.socket[i],
                                         &EMPTY_SOCKET_CONFIGURATION));
        }
    }

    testrun(0 == config.http_message.header.capacity);
    testrun(0 == config.http_message.header.max_bytes_method_name);
    testrun(0 == config.http_message.header.max_bytes_line);
    testrun(0 == config.http_message.buffer.default_size);
    testrun(0 == config.http_message.buffer.max_bytes_recache);
    testrun(0 == config.http_message.transfer.max);
    testrun(0 == config.http_message.chunk.max_bytes);
    testrun(0 == config.websocket_frame.buffer.default_size);
    testrun(0 == config.websocket_frame.buffer.max_bytes_recache);

    // add http_message and websocket config

    val = ov_websocket_frame_config_to_json((ov_websocket_frame_config){
        .buffer.default_size = 1, .buffer.max_bytes_recache = 2});
    testrun(ov_json_object_set(out, OV_KEY_WEBSOCKET, val));

    val = ov_http_message_config_to_json(
        (ov_http_message_config){.header.capacity = 1,
                                 .header.max_bytes_method_name = 2,
                                 .header.max_bytes_line = 3,
                                 .buffer.default_size = 4,
                                 .buffer.max_bytes_recache = 5,
                                 .transfer.max = 6,
                                 .chunk.max_bytes = 7});
    testrun(ov_json_object_set(out, OV_KEY_HTTP_MESSAGE, val));

    config = ov_webserver_base_config_from_json(itm);
    testrun(0 == strcmp("name", config.name));
    testrun(0 == strcmp("path", config.domain_config_path));
    testrun(config.debug == true);
    testrun(config.ip4_only == true);
    testrun(config.timer.io_timeout_usec == 1);
    testrun(config.timer.accept_to_io_timeout_usec == 2);
    testrun(socket_configs_equal(&config.http.redirect, &http));
    testrun(socket_configs_equal(&config.http.secure, &https));
    for (size_t i = 0; i < OV_WEBSERVER_BASE_MAX_STUN_LISTENER; i++) {
        if (i < stun_config_items) {
            testrun(
                socket_configs_equal(&config.stun.socket[i], &stun_configs[i]));
        } else {
            testrun(socket_configs_equal(&config.stun.socket[i],
                                         &EMPTY_SOCKET_CONFIGURATION));
        }
    }
    testrun(1 == config.http_message.header.capacity);
    testrun(2 == config.http_message.header.max_bytes_method_name);
    testrun(3 == config.http_message.header.max_bytes_line);
    testrun(4 == config.http_message.buffer.default_size);
    testrun(5 == config.http_message.buffer.max_bytes_recache);
    testrun(6 == config.http_message.transfer.max);
    testrun(7 == config.http_message.chunk.max_bytes);

    testrun(1 == config.websocket_frame.buffer.default_size);
    testrun(2 == config.websocket_frame.buffer.max_bytes_recache);

    testrun(NULL == ov_json_value_free(itm));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_base_config_to_json() {

    ov_webserver_base_config config = ov_webserver_base_config_from_json(NULL);

    ov_json_value *val = NULL;
    ov_json_value *out = ov_webserver_base_config_to_json(config);
    testrun(out);

    char *str = ov_json_value_to_string(out);
    // fprintf(stdout, "%s\n", str);
    str = ov_data_pointer_free(str);

    testrun(0 != ov_json_object_count(out));
    testrun(NULL == ov_json_object_get(out, OV_KEY_NAME));
    testrun(NULL == ov_json_object_get(out, OV_KEY_DOMAINS));
    testrun(NULL == ov_json_object_get(out, OV_KEY_DEBUG));
    testrun(NULL == ov_json_object_get(out, OV_KEY_IP4_ONLY));
    val = ov_json_object_get(out, OV_KEY_SOCKETS);
    testrun(val);
    testrun(NULL == ov_json_object_get(val, OV_KEY_HTTP));
    testrun(NULL == ov_json_object_get(val, OV_KEY_HTTPS));
    testrun(NULL != ov_json_object_get(val, OV_KEY_STUN));
    testrun(0 == ov_json_array_count(ov_json_object_get(val, OV_KEY_STUN)));
    val = ov_json_object_get(out, OV_KEY_TIMER);
    testrun(val);
    testrun(0 == ov_json_number_get(ov_json_object_get(val, OV_KEY_ACCEPT)));
    testrun(0 == ov_json_number_get(ov_json_object_get(val, OV_KEY_IO)));
    val = ov_json_object_get(out, OV_KEY_HTTP_MESSAGE);
    testrun(ov_json_get(val, "/" OV_KEY_BUFFER));
    testrun(ov_json_get(val, "/" OV_KEY_BUFFER "/" OV_KEY_SIZE));
    testrun(ov_json_get(val, "/" OV_KEY_BUFFER "/" OV_KEY_MAX_CACHE));
    testrun(ov_json_get(val, "/" OV_KEY_CHUNK));
    testrun(ov_json_get(val, "/" OV_KEY_CHUNK "/" OV_KEY_MAX));
    testrun(ov_json_get(val, "/" OV_KEY_TRANSFER "/" OV_KEY_MAX));
    testrun(ov_json_get(val, "/" OV_KEY_HEADER));
    testrun(ov_json_get(val, "/" OV_KEY_HEADER "/" OV_KEY_CAPACITY));
    testrun(ov_json_get(val, "/" OV_KEY_HEADER "/" OV_KEY_METHOD));
    testrun(ov_json_get(val, "/" OV_KEY_HEADER "/" OV_KEY_LINES));
    testrun(0 == ov_json_number_get(
                     ov_json_get(val, "/" OV_KEY_BUFFER "/" OV_KEY_SIZE)));
    testrun(0 == ov_json_number_get(
                     ov_json_get(val, "/" OV_KEY_BUFFER "/" OV_KEY_MAX_CACHE)));
    testrun(0 == ov_json_number_get(
                     ov_json_get(val, "/" OV_KEY_CHUNK "/" OV_KEY_MAX)));
    testrun(0 == ov_json_number_get(
                     ov_json_get(val, "/" OV_KEY_TRANSFER "/" OV_KEY_MAX)));
    testrun(0 == ov_json_number_get(
                     ov_json_get(val, "/" OV_KEY_HEADER "/" OV_KEY_CAPACITY)));
    testrun(0 == ov_json_number_get(
                     ov_json_get(val, "/" OV_KEY_HEADER "/" OV_KEY_METHOD)));
    testrun(0 == ov_json_number_get(
                     ov_json_get(val, "/" OV_KEY_HEADER "/" OV_KEY_LINES)));

    out = ov_json_value_free(out);

    config = (ov_webserver_base_config){

        .debug = true,
        .ip4_only = true,

        .name = "name",
        .domain_config_path = "path",

        .limit.max_sockets = 1,

        .http_message =
            (ov_http_message_config){.header.capacity = 1,
                                     .header.max_bytes_method_name = 2,
                                     .header.max_bytes_line = 3,
                                     .buffer.default_size = 4,
                                     .buffer.max_bytes_recache = 5,
                                     .transfer.max = 6,
                                     .chunk.max_bytes = 7},

        .websocket_frame =
            (ov_websocket_frame_config){.buffer.default_size = 1,
                                        .buffer.max_bytes_recache = 2},

        .http = {.redirect = (ov_socket_configuration){.type = TCP,
                                                       .host = "localhost",
                                                       .port = 80},

                 .secure = (ov_socket_configuration){.type = TCP,
                                                     .host = "localhost",
                                                     .port = 443}},

        .stun.socket[0] = (ov_socket_configuration){.type = UDP,
                                                    .host = "localhost",
                                                    .port = 1},

        .stun.socket[1] = (ov_socket_configuration){.type = UDP,
                                                    .host = "localhost",
                                                    .port = 1},

        .stun.socket[2] = (ov_socket_configuration){.type = UDP,
                                                    .host = "localhost",
                                                    .port = 1},

        .timer.io_timeout_usec = 1,
        .timer.accept_to_io_timeout_usec = 2

    };

    out = ov_webserver_base_config_to_json(config);
    testrun(out);

    str = ov_json_value_to_string(out);
    // fprintf(stdout, "%s\n", str);
    str = ov_data_pointer_free(str);

    testrun(0 == strcmp("name", ov_json_string_get(
                                    ov_json_object_get(out, OV_KEY_NAME))));
    testrun(0 == strcmp("path", ov_json_string_get(
                                    ov_json_object_get(out, OV_KEY_DOMAINS))));
    testrun(ov_json_is_true(ov_json_object_get(out, OV_KEY_DEBUG)));
    testrun(ov_json_is_true(ov_json_object_get(out, OV_KEY_IP4_ONLY)));

    val = ov_json_object_get(out, OV_KEY_SOCKETS);
    testrun(val);
    testrun(NULL != ov_json_object_get(val, OV_KEY_HTTP));
    testrun(NULL != ov_json_object_get(val, OV_KEY_HTTPS));
    testrun(NULL != ov_json_object_get(val, OV_KEY_STUN));
    testrun(3 == ov_json_array_count(ov_json_object_get(val, OV_KEY_STUN)));
    val = ov_json_object_get(out, OV_KEY_TIMER);
    testrun(val);
    testrun(2 == ov_json_number_get(ov_json_object_get(val, OV_KEY_ACCEPT)));
    testrun(1 == ov_json_number_get(ov_json_object_get(val, OV_KEY_IO)));

    val = ov_json_object_get(out, OV_KEY_HTTP_MESSAGE);
    testrun(4 == ov_json_number_get(
                     ov_json_get(val, "/" OV_KEY_BUFFER "/" OV_KEY_SIZE)));
    testrun(5 == ov_json_number_get(
                     ov_json_get(val, "/" OV_KEY_BUFFER "/" OV_KEY_MAX_CACHE)));
    testrun(7 == ov_json_number_get(
                     ov_json_get(val, "/" OV_KEY_CHUNK "/" OV_KEY_MAX)));
    testrun(6 == ov_json_number_get(
                     ov_json_get(val, "/" OV_KEY_TRANSFER "/" OV_KEY_MAX)));
    testrun(1 == ov_json_number_get(
                     ov_json_get(val, "/" OV_KEY_HEADER "/" OV_KEY_CAPACITY)));
    testrun(2 == ov_json_number_get(
                     ov_json_get(val, "/" OV_KEY_HEADER "/" OV_KEY_METHOD)));
    testrun(3 == ov_json_number_get(
                     ov_json_get(val, "/" OV_KEY_HEADER "/" OV_KEY_LINES)));

    val = ov_json_object_get(out, OV_KEY_WEBSOCKET);
    testrun(val);
    testrun(1 == ov_json_number_get(
                     ov_json_get(val, "/" OV_KEY_BUFFER "/" OV_KEY_SIZE)));
    testrun(2 == ov_json_number_get(
                     ov_json_get(val, "/" OV_KEY_BUFFER "/" OV_KEY_MAX_CACHE)));

    out = ov_json_value_free(out);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_base_answer_get() {

    char errorstring[OV_SSL_ERROR_STRING_BUFFER_SIZE] = {0};
    int errorcode = -1, n = 0;

    int data[100] = {0};
    void *userdata = &data;

    const char *filename = "test.file";
    char *file = ov_test_get_resource_path("/resources/test.file");
    const char *mime = "some/file";
    const char *mime_with_charset = "some/file;charset=utf-8";

    unlink(file);

    size_t size = 4000;
    uint8_t buf[size];
    memset(buf, 0, size);

    uint8_t *content = buf;
    size_t content_bytes = size;

    testrun(ov_utf8_generate_random_buffer(&content, &content_bytes, 950));
    if (content_bytes < size) {

        testrun(ov_random_bytes(buf + content_bytes, size - content_bytes));

        for (size_t i = content_bytes; i < size; i++) {

            if (!isalpha(buf[i]))
                buf[i] = 'x';
        }
    }

    testrun(ov_utf8_validate_sequence(buf, size));
    content_bytes = size;

    testrun(OV_FILE_SUCCESS ==
            ov_file_write(file, content, content_bytes, "w"));

    ov_file_format_desc format = (ov_file_format_desc){0};
    format.desc = ov_file_desc_from_path(file);
    testrun(format.desc.bytes == (ssize_t)content_bytes);
    testrun(0 == memcmp("file", format.desc.ext[0], 4));
    strcat(format.mime, mime);

    ov_http_message *request = ov_http_create_request_string(
        (ov_http_message_config){0}, (ov_http_version){.major = 1, .minor = 1},
        "GET", filename);

    testrun(ov_http_message_close_header(request));
    testrun(request);
    testrun(OV_HTTP_PARSER_SUCCESS ==
            ov_http_pointer_parse_message(request, NULL));

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = (ov_webserver_base_config){
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 100 * 1000 * 1000,
        .callback.userdata = userdata,
        .callback.accept = accept_socket,
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0}};

    set_resources_dir(config.domain_config_path);

    ov_webserver_base *srv = ov_webserver_base_create(config);
    testrun(srv);

    testrun(
        !ov_webserver_base_answer_get(NULL, 0, (ov_file_format_desc){0}, NULL));

    int client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    int conn_fd = -1;

    for (size_t i = 0; i < 100; i++) {
        if (data[i] == 0)
            continue;

        conn_fd = data[i];
        break;
    }

    // check state of connection is invalid
    testrun(srv->conn[conn_fd].fd == conn_fd);
    testrun(srv->conn[conn_fd].type == CONNECTION_TYPE_HTTP_1_1);
    testrun(srv->conn[conn_fd].tls.handshaked == false);

    // check state of format is valid
    testrun((ssize_t)content_bytes == format.desc.bytes);
    testrun(0 == strcmp(mime, format.mime));

    // check state of request is valid
    testrun(ov_http_is_request(request, OV_HTTP_METHOD_GET));

    testrun(!ov_webserver_base_answer_get(srv, conn_fd, format, request));

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL *ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(
                     ssl, (char *)srv->domain.array[0].config.name.start));
    int err = 0;
    ssize_t bytes = 0;
    int r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // check state of connection is valid
    testrun(srv->conn[conn_fd].fd == conn_fd);
    testrun(srv->conn[conn_fd].type == CONNECTION_TYPE_HTTP_1_1);
    testrun(srv->conn[conn_fd].tls.handshaked == true);

    // check state of format is valid
    testrun(content_bytes == (size_t)format.desc.bytes);
    testrun(0 == strcmp(mime, format.mime));

    // check state of request is valid
    testrun(ov_http_is_request(request, OV_HTTP_METHOD_GET));

    // test with non connection socket
    testrun(!ov_webserver_base_answer_get(srv, client, format, request));

    bytes = -1;
    size_t count = 0;

    bool retry = true;

    const ov_http_header *header = NULL;

    ov_http_message *response = ov_http_message_create(
        (ov_http_message_config){.buffer.default_size = 5000});

    // check start ready to read
    bytes = SSL_read(ssl, response->buffer->start, response->buffer->capacity);
    testrun(bytes == -1);

    // test with connected socket
    testrun(ov_webserver_base_answer_get(srv, conn_fd, format, request));

    ov_buffer_clear(response->buffer);

    retry = true;
    while (retry) {

        testrun(loop->run(loop, OV_RUN_ONCE));

        bytes =
            SSL_read(ssl, response->buffer->start + response->buffer->length,
                     response->buffer->capacity);

        if (bytes > 0) {

            count++;
            response->buffer->length += bytes;

            if (OV_HTTP_PARSER_SUCCESS ==
                ov_http_pointer_parse_message(response, NULL))
                break;

        } else if (bytes < 1) {

            n = SSL_get_error(ssl, bytes);
            switch (n) {

            case SSL_ERROR_NONE:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_NONE - retry\n");
                break;
            case SSL_ERROR_WANT_READ:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_READ - retry\n");
                break;
            case SSL_ERROR_WANT_WRITE:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_WRITE - retry\n");
                break;
            case SSL_ERROR_WANT_CONNECT:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_CONNECT - retry\n");
                break;
            case SSL_ERROR_WANT_ACCEPT:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_ACCEPT - retry\n");
                break;
            case SSL_ERROR_WANT_X509_LOOKUP:
                if (test_debug_log)
                    fprintf(stdout, "SSL_ERROR_WANT_X509_LOOKUP - retry\n");
                break;

            case SSL_ERROR_ZERO_RETURN:
                fprintf(stdout, "SSL_ERROR_ZERO_RETURN - close\n");
                retry = false;
                break;

            case SSL_ERROR_SYSCALL:

                errorcode = ERR_get_error();
                ERR_error_string_n(errorcode, errorstring,
                                   OV_SSL_ERROR_STRING_BUFFER_SIZE);

                fprintf(stdout, "SSL_ERROR_SYSCALL - stop %i|%s errno %i|%s\n",
                        errorcode, errorstring, errno, strerror(errno));

                retry = false;
                break;

            case SSL_ERROR_SSL:

                errorcode = ERR_get_error();
                ERR_error_string_n(errorcode, errorstring,
                                   OV_SSL_ERROR_STRING_BUFFER_SIZE);

                fprintf(stdout, "SSL_ERROR_SSL - stop %i|%s errno %i|%s\n",
                        errorcode, errorstring, errno, strerror(errno));

                retry = false;
                break;

            default:
                fprintf(stdout, "DEFAULT REACHED");
                testrun(1 == 0);
                break;
            }
        }
    }

    // fprintf(stdout, "received %zu messages\n", count);
    testrun(count == 1);
    testrun(response->body.length == 4000);

    // check header data
    testrun(ov_http_header_get_unique(response->header,
                                      response->config.header.capacity,
                                      OV_HTTP_KEY_SERVER));

    testrun(ov_http_header_get_unique(
        response->header, response->config.header.capacity, OV_HTTP_KEY_DATE));

    testrun(!ov_http_header_get_unique(response->header,
                                       response->config.header.capacity,
                                       OV_HTTP_KEY_TRANSFER_ENCODING));

    header = ov_http_header_get_unique(response->header,
                                       response->config.header.capacity,
                                       OV_HTTP_KEY_CONTENT_TYPE);

    testrun(header);
    testrun(0 == memcmp(mime_with_charset, header->value.start,
                        header->value.length));

    testrun(0 == memcmp(buf, response->body.start, response->body.length));

    testrun(loop->run(loop, OV_RUN_ONCE));

    // reset client
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    unlink(file);

    testrun(NULL == ov_http_message_free(request));
    testrun(NULL == ov_http_message_free(response));

    testrun(NULL == ov_webserver_base_free(srv));
    testrun(NULL == ov_event_loop_free(loop));

    free(file);
    file = 0;

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_wss_events() {

    struct dummy_event_data data[5] = {0};

    size_t items = 10;
    int client[items];
    memset(client, 0, sizeof(int) * items);

    int conn[items];
    memset(conn, 0, sizeof(int) * items);

    SSL_CTX *ctx[items];
    memset(ctx, 0, items);

    SSL *ssl[items];
    memset(ssl, 0, items);

    uint8_t key[OV_WEBSOCKET_SECURE_KEY_SIZE + 1];
    memset(key, 0, OV_WEBSOCKET_SECURE_KEY_SIZE + 1);

    testrun(ov_websocket_generate_secure_websocket_key(
        key, OV_WEBSOCKET_SECURE_KEY_SIZE));

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = (ov_webserver_base_config){
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 100 * 1000 * 1000,
        .callback.userdata = NULL,
        .callback.accept = NULL,
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0}};

    set_resources_dir(config.domain_config_path);

    ov_webserver_base *srv = ov_webserver_base_create(config);
    testrun(srv);

    /* prepare env */

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ov_domain *openvocs = &srv->domain.array[0];
    ov_domain *one = &srv->domain.array[1];
    ov_domain *two = &srv->domain.array[2];

    testrun(0 == memcmp("openvocs.test", openvocs->config.name.start,
                        openvocs->config.name.length));

    testrun(0 == memcmp("one.test", one->config.name.start,
                        one->config.name.length));

    testrun(0 == memcmp("two.test", two->config.name.start,
                        two->config.name.length));

    testrun(ov_webserver_base_configure_uri_event_io(
        srv,
        (ov_memory_pointer){.start = openvocs->config.name.start,
                            .length = openvocs->config.name.length},
        (ov_event_io_config){.userdata = &data[0],
                             .name = "/",
                             .callback.process = dummy_event_callback,
                             .callback.close = dummy_event_close},
        NULL));

    testrun(ov_webserver_base_configure_uri_event_io(
        srv,
        (ov_memory_pointer){.start = one->config.name.start,
                            .length = one->config.name.length},
        (ov_event_io_config){.userdata = &data[1],
                             .name = "/",
                             .callback.process = dummy_event_callback,
                             .callback.close = dummy_event_close},
        NULL));

    testrun(ov_webserver_base_configure_uri_event_io(
        srv,
        (ov_memory_pointer){.start = two->config.name.start,
                            .length = two->config.name.length},
        (ov_event_io_config){.userdata = &data[2],
                             .name = "/",
                             .callback.process = dummy_event_callback,
                             .callback.close = dummy_event_close},
        NULL));
    // check state
    testrun(1 == ov_dict_count(openvocs->event_handler.uri));
    testrun(1 == ov_dict_count(one->event_handler.uri));
    testrun(1 == ov_dict_count(two->event_handler.uri));

    ov_http_message_config http_config = (ov_http_message_config){0};
    ov_http_version http_version = (ov_http_version){.major = 1, .minor = 1};

    ov_http_message *msg = NULL;

    size_t size = 1000;
    char buf[size];
    memset(buf, 0, size);

    int r = 0;
    int err = 0;
    int errorcode = 0;
    ssize_t bytes = -1;

    char *expect = "HTTP/1.1 101 Switching Protocols";

    for (size_t i = 0; i < 10; i++) {

        client[i] = ov_socket_create(config.http.secure, true, NULL);
        testrun(client[i] > -1);
        testrun(ov_socket_ensure_nonblocking(client[i]));

        ctx[i] = SSL_CTX_new(TLS_client_method());
        ssl[i] = SSL_new(ctx[i]);
        testrun(1 == SSL_set_fd(ssl[i], client[i]));
        SSL_CTX_set_client_hello_cb(ctx[i], dummy_client_hello_cb, NULL);

        ov_domain *domain = NULL;

        switch (i) {

        case 0:
        case 1:
        case 2:
        case 3:
            testrun(1 == SSL_set_tlsext_host_name(
                             ssl[i], (char *)openvocs->config.name.start));

            domain = openvocs;
            break;

        case 4:
        case 5:
        case 6:
            testrun(1 == SSL_set_tlsext_host_name(
                             ssl[i], (char *)one->config.name.start));

            domain = one;
            break;

        case 7:
        case 8:
        case 9:
            testrun(1 == SSL_set_tlsext_host_name(
                             ssl[i], (char *)two->config.name.start));

            domain = two;
            break;

        default:
            OV_ASSERT(1 == 0);
        }

        r = run_client_handshake(loop, ssl[i], &err, &errorcode);
        testrun(r == 1);

        // TLS handshake done

        testrun(loop->run(loop, TEST_RUNTIME_USECS));

        // find connection at server

        bool contained = false;
        int new_conn = -1;

        for (size_t x = 0; x < srv->connections_max; x++) {

            if (srv->conn[x].fd == -1)
                continue;

            contained = false;
            for (size_t y = 0; y < items; y++) {

                if (0 == conn[y])
                    break;

                if (conn[y] == srv->conn[x].fd) {
                    contained = true;
                    break;
                }
            }
            if (false == contained) {
                new_conn = srv->conn[x].fd;
                break;
            }
        }

        conn[i] = new_conn;
        testrun(0 < conn[i]);

        msg = ov_http_create_request_string(http_config, http_version, "GET",
                                            "/");
        testrun(msg);
        testrun(ov_http_message_add_header_string(
            msg, OV_HTTP_KEY_HOST, (char *)domain->config.name.start));
        testrun(ov_http_message_add_header_string(msg, OV_HTTP_KEY_UPGRADE,
                                                  OV_WEBSOCKET_KEY));
        testrun(ov_http_message_add_header_string(msg, OV_HTTP_KEY_CONNECTION,
                                                  OV_HTTP_KEY_UPGRADE));
        testrun(ov_http_message_add_header_string(msg, OV_WEBSOCKET_KEY_SECURE,
                                                  (char *)key));
        testrun(ov_http_message_add_header_string(
            msg, OV_WEBSOCKET_KEY_SECURE_VERSION, "13"));
        testrun(ov_http_message_close_header(msg));
        testrun(OV_HTTP_PARSER_SUCCESS ==
                ov_http_pointer_parse_message(msg, NULL));

        bytes = -1;
        while (bytes == -1) {

            testrun(loop->run(loop, TEST_RUNTIME_USECS));
            bytes = SSL_write(ssl[i], msg->buffer->start, msg->buffer->length);
        }

        testrun(bytes == (ssize_t)msg->buffer->length);
        msg = ov_http_message_free(msg);

        // read response

        memset(buf, 0, size);

        bytes = -1;
        while (bytes == -1) {

            testrun(loop->run(loop, TEST_RUNTIME_USECS));
            bytes = SSL_read(ssl[i], buf, size);
        }

        testrun(bytes > 0);
        testrun(0 == strncmp(buf, expect, strlen(expect)));
        testrun(loop->run(loop, TEST_RUNTIME_USECS));

        // WSS upgrade done
    }

    /* (1) send from client to server */

    char *content = "{\"some\":\"json\"}";
    ov_json_value *check = ov_json_value_from_string(content, strlen(content));
    testrun(check);
    check = ov_json_value_free(check);

    ov_websocket_frame *frame =
        ov_websocket_frame_create((ov_websocket_frame_config){0});

    frame->buffer->start[0] = 0x80 | OV_WEBSOCKET_OPCODE_TEXT;
    testrun(ov_websocket_set_data(frame, (uint8_t *)content, strlen(content),
                                  true));

    for (size_t i = 0; i < items; i++) {

        bytes = -1;
        while (bytes == -1) {

            bytes =
                SSL_write(ssl[i], frame->buffer->start, frame->buffer->length);
            testrun(loop->run(loop, TEST_RUNTIME_USECS));
        }

        testrun(bytes == (ssize_t)frame->buffer->length);

        // check data

        switch (i) {

        case 0:
        case 1:
        case 2:
        case 3:

            testrun(data[1].socket == 0);
            testrun(data[1].value == NULL);

            testrun(data[2].socket == 0);
            testrun(data[2].value == NULL);

            testrun(data[0].socket == conn[i]);
            testrun(data[0].value != NULL);

            testrun(0 == strcmp("json", ov_json_string_get(ov_json_object_get(
                                            data[0].value, "some"))));

            data[0].socket = 0;
            data[0].value = ov_json_value_free(data[0].value);
            break;

        case 4:
        case 5:
        case 6:

            testrun(data[0].socket == 0);
            testrun(data[0].value == NULL);

            testrun(data[2].socket == 0);
            testrun(data[2].value == NULL);

            testrun(data[1].socket == conn[i]);
            testrun(data[1].value != NULL);

            testrun(0 == strcmp("json", ov_json_string_get(ov_json_object_get(
                                            data[1].value, "some"))));

            data[1].socket = 0;
            data[1].value = ov_json_value_free(data[1].value);
            break;

        case 7:
        case 8:
        case 9:

            testrun(data[0].socket == 0);
            testrun(data[0].value == NULL);

            testrun(data[1].socket == 0);
            testrun(data[1].value == NULL);

            testrun(data[2].socket == conn[i]);
            testrun(data[2].value != NULL);

            testrun(0 == strcmp("json", ov_json_string_get(ov_json_object_get(
                                            data[2].value, "some"))));

            data[2].socket = 0;
            data[2].value = ov_json_value_free(data[2].value);
            break;

        default:
            OV_ASSERT(1 == 0);
        }
    }

    /* (2) send from server to client */
    ov_json_value *val = NULL;

    for (size_t i = 0; i < items; i++) {

        check = ov_json_value_free(check);
        check = ov_json_value_from_string(content, strlen(content));
        testrun(check);

        testrun(cb_wss_send_json(srv, conn[i], check));
        testrun(loop->run(loop, TEST_RUNTIME_USECS));

        ov_websocket_frame_clear(frame);

        bytes = -1;
        while (bytes == -1) {

            bytes =
                SSL_read(ssl[i], frame->buffer->start, frame->buffer->capacity);
            testrun(loop->run(loop, TEST_RUNTIME_USECS));
        }
        testrun(bytes > 0);
        frame->buffer->length = bytes;
        testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
                ov_websocket_parse_frame(frame, NULL));

        // check non fragmented text
        testrun(frame->buffer->start[0] == (0x80 | OV_WEBSOCKET_OPCODE_TEXT));
        // check not masked
        testrun(!(frame->buffer->start[1] & 0x80));
        // check content
        val = ov_json_value_from_string((char *)frame->content.start,
                                        frame->content.length);
        testrun(val);
        testrun(1 == ov_json_object_count(val));
        testrun(0 == strcmp("json", ov_json_string_get(
                                        ov_json_object_get(val, "some"))));
        val = ov_json_value_free(val);
        check = ov_json_value_free(check);
    }

    // (3) check state send

    Connection *c = &srv->conn[conn[5]];
    testrun(c->type == CONNECTION_TYPE_WEBSOCKET);
    c->type = CONNECTION_TYPE_HTTP_1_1;
    check = ov_json_value_from_string(content, strlen(content));
    testrun(check);
    testrun(!cb_wss_send_json(srv, conn[5], check));
    c->type = CONNECTION_TYPE_WEBSOCKET;
    check = ov_json_value_free(check);

    c->fd = -1;
    check = ov_json_value_from_string(content, strlen(content));
    testrun(check);
    testrun(!cb_wss_send_json(srv, conn[5], check));
    c->fd = conn[5];
    check = ov_json_value_free(check);

    // recheck success
    ov_websocket_frame_clear(frame);
    testrun(-1 ==
            SSL_read(ssl[5], frame->buffer->start, frame->buffer->capacity));

    check = ov_json_value_from_string(content, strlen(content));
    testrun(check);
    testrun(cb_wss_send_json(srv, conn[5], check));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    check = ov_json_value_free(check);

    ov_websocket_frame_clear(frame);

    bytes = -1;
    while (bytes == -1) {

        bytes = SSL_read(ssl[5], frame->buffer->start, frame->buffer->capacity);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
    }
    testrun(bytes > 0);
    frame->buffer->length = bytes;
    testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
            ov_websocket_parse_frame(frame, NULL));

    // check non fragmented text
    testrun(frame->buffer->start[0] == (0x80 | OV_WEBSOCKET_OPCODE_TEXT));
    // check not masked
    testrun(!(frame->buffer->start[1] & 0x80));
    // check content
    val = ov_json_value_from_string((char *)frame->content.start,
                                    frame->content.length);
    testrun(val);
    testrun(1 == ov_json_object_count(val));
    testrun(0 == strcmp("json",
                        ov_json_string_get(ov_json_object_get(val, "some"))));
    val = ov_json_value_free(val);

    // (4) check framed send

    check = ov_json_value_from_string(content, strlen(content));
    testrun(check);
    char *str = ov_json_value_to_string(check);
    testrun(str);
    testrun(strlen(str) > 10);
    testrun(strlen(str) < 20);
    str = ov_data_pointer_free(str);

    srv->config.limit.max_content_bytes_per_websocket_frame = 10;

    testrun(cb_wss_send_json(srv, conn[5], check));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    ov_buffer *content_buffer = ov_buffer_create(100);

    bytes = -1;
    ov_websocket_frame_clear(frame);
    while (bytes == -1) {

        bytes = SSL_read(ssl[5], frame->buffer->start, frame->buffer->capacity);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
    }
    testrun(bytes > 0);
    frame->buffer->length = bytes;
    testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
            ov_websocket_parse_frame(frame, NULL));

    // check fragmented start text
    testrun(frame->buffer->start[0] == OV_WEBSOCKET_OPCODE_TEXT);
    // check not masked
    testrun(!(frame->buffer->start[1] & 0x80));
    testrun(ov_buffer_push(content_buffer, (void *)frame->content.start,
                           frame->content.length));

    bytes = -1;
    ov_websocket_frame_clear(frame);
    while (bytes == -1) {

        bytes = SSL_read(ssl[5], frame->buffer->start, frame->buffer->capacity);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
    }
    testrun(bytes > 0);
    frame->buffer->length = bytes;
    testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
            ov_websocket_parse_frame(frame, NULL));

    // check fragmented end
    testrun(frame->buffer->start[0] == 0x80);
    // check not masked
    testrun(!(frame->buffer->start[1] & 0x80));
    testrun(ov_buffer_push(content_buffer, (void *)frame->content.start,
                           frame->content.length));

    // check content reveived
    val = ov_json_value_from_string((char *)content_buffer->start,
                                    content_buffer->length);
    testrun(val);
    testrun(1 == ov_json_object_count(val));
    testrun(0 == strcmp("json",
                        ov_json_string_get(ov_json_object_get(val, "some"))));
    val = ov_json_value_free(val);
    content_buffer = ov_buffer_free(content_buffer);
    check = ov_json_value_free(check);

    frame = ov_websocket_frame_free(frame);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    testrun(NULL == ov_webserver_base_free(srv));
    testrun(NULL == ov_event_loop_free(loop));

    for (size_t i = 0; i < items; i++) {

        SSL_CTX_free(ctx[i]);
        SSL_free(ssl[i]);
    }

    return testrun_log_success();
}

#include "../include/ov_event_api.h"

/*----------------------------------------------------------------------------*/

int test_ov_webserver_base_send_json() {

    int data[100] = {0};
    void *userdata = &data;

    int errorcode = -1;
    int err = 0;

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = (ov_webserver_base_config){
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 10 * 1000 * 1000,
        .callback.userdata = userdata,
        .callback.accept = accept_socket,
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0}};

    set_resources_dir(config.domain_config_path);

    ov_webserver_base *server = ov_webserver_base_create(config);
    testrun(server);

    ov_json_value *msg = ov_event_api_message_create("test", "uuid", 1);
    testrun(msg);

    int client = ov_socket_create(server->config.http.secure, true, NULL);
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
    testrun(1 == SSL_set_tlsext_host_name(
                     ssl, server->domain.array[0].config.name.start));
    int r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // check connection socket accepted
    int conn = 0;
    for (size_t i = 0; i < 100; i++) {

        if (data[i] != 0) {
            testrun(0 == conn);
            conn = data[i];
            break;
        }
    }

    testrun(conn > 0);
    testrun(server->conn[conn].fd == conn);
    testrun(server->conn[conn].tls.ssl);
    testrun(server->conn[conn].tls.handshaked);
    testrun(!ov_webserver_base_send_json(NULL, 0, NULL));
    testrun(!ov_webserver_base_send_json(server, 0, msg));
    testrun(!ov_webserver_base_send_json(NULL, conn, msg));
    testrun(!ov_webserver_base_send_json(server, conn, NULL));

    // connection not upgraded to websocket yet
    testrun(!ov_webserver_base_send_json(server, conn, msg));

    // fake upgrade to websocket
    Connection *connection = &server->conn[conn];
    connection->type = CONNECTION_TYPE_WEBSOCKET;

    testrun(ov_webserver_base_send_json(server, conn, msg));

    ov_websocket_frame *frame = ov_websocket_frame_create(
        (ov_websocket_frame_config){.buffer.default_size = OV_SSL_MAX_BUFFER});
    testrun(frame);

    uint8_t *next = NULL;
    ssize_t bytes = -1;

    while (bytes == -1) {

        bytes = SSL_read(ssl, frame->buffer->start, frame->buffer->capacity);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
    }

    frame->buffer->length = bytes;
    testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
            ov_websocket_parse_frame(frame, &next));

    ov_json_value *out = ov_json_value_from_string(
        (const char *)frame->content.start, frame->content.length);

    size_t len = bytes;

    testrun(out);
    testrun(ov_event_api_event_is(out, "test"));
    out = ov_json_value_free(out);
    ov_websocket_frame_clear(frame);

    server->config.limit.max_content_bytes_per_websocket_frame = (len / 2);
    testrun(ov_webserver_base_send_json(server, conn, msg));

    bytes = -1;

    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_read(ssl, frame->buffer->start, frame->buffer->capacity);
    }
    frame->buffer->length = bytes;

    testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
            ov_websocket_parse_frame(frame, &next));
    testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_START);

    ov_buffer *buffer = ov_buffer_create(1000);
    testrun(ov_buffer_push(buffer, (void *)frame->content.start,
                           frame->content.length));
    ov_websocket_frame_clear(frame);

    bytes = -1;

    while (bytes == -1) {

        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        bytes = SSL_read(ssl, frame->buffer->start, frame->buffer->capacity);
    }
    frame->buffer->length = bytes;

    testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
            ov_websocket_parse_frame(frame, &next));
    fprintf(stdout, "%i\n", frame->state);
    testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_LAST);
    testrun(ov_buffer_push(buffer, (void *)frame->content.start,
                           frame->content.length));

    out =
        ov_json_value_from_string((const char *)buffer->start, buffer->length);
    testrun(ov_event_api_event_is(out, "test"));
    out = ov_json_value_free(out);
    ov_websocket_frame_clear(frame);
    frame = ov_websocket_frame_free(frame);
    buffer = ov_buffer_free(buffer);

    msg = ov_json_value_free(msg);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    testrun(NULL == ov_webserver_base_free(server));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_base_uri_file_path() {

    int data[100] = {0};
    void *userdata = &data;

    int errorcode = -1;
    int err = 0;

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = (ov_webserver_base_config){
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 10 * 1000 * 1000,
        .callback.userdata = userdata,
        .callback.accept = accept_socket,
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0}};

    set_resources_dir(config.domain_config_path);

    ov_webserver_base *server = ov_webserver_base_create(config);
    testrun(server);

    int client = ov_socket_create(server->config.http.secure, true, NULL);
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
    testrun(1 == SSL_set_tlsext_host_name(
                     ssl, server->domain.array[0].config.name.start));
    int r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // check connection socket accepted
    int conn = 0;
    for (size_t i = 0; i < 100; i++) {

        if (data[i] != 0) {
            testrun(0 == conn);
            conn = data[i];
            break;
        }
    }

    Connection *connection = &server->conn[conn];

    char root_path[PATH_MAX] = {0};
    testrun(ov_uri_path_remove_dot_segments(connection->domain->config.path,
                                            root_path));

    ov_http_message *msg = ov_http_create_request(
        (ov_http_message_config){0}, (ov_http_version){0},
        (ov_http_request){.method.start = (uint8_t *)"method",
                          .method.length = 6,
                          .uri.start = (uint8_t *)"/",
                          .uri.length = 1});

    testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_request_line(
                                          msg->buffer->start,
                                          msg->buffer->length, &msg->request,
                                          &msg->version, 100, NULL));

    char path[PATH_MAX] = {0};
    char check[PATH_MAX] = {0};

    testrun(0 < snprintf(check, PATH_MAX, "%s/index.html", root_path));
    testrun(ov_webserver_base_uri_file_path(server, conn, msg, PATH_MAX, path));
    testrun(0 == strcmp(check, path));
    msg = ov_http_message_free(msg);

    char *uri = "/some.html";

    msg = ov_http_create_request(
        (ov_http_message_config){0}, (ov_http_version){0},
        (ov_http_request){.method.start = (uint8_t *)"get",
                          .method.length = 6,
                          .uri.start = (const uint8_t *)uri,
                          .uri.length = strlen(uri)});

    testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_request_line(
                                          msg->buffer->start,
                                          msg->buffer->length, &msg->request,
                                          &msg->version, 100, NULL));

    memset(path, 0, PATH_MAX);
    memset(check, 0, PATH_MAX);

    testrun(0 < snprintf(check, PATH_MAX, "%s/some.html", root_path));
    testrun(ov_webserver_base_uri_file_path(server, conn, msg, PATH_MAX, path));
    testrun(0 == strcmp(check, path));
    msg = ov_http_message_free(msg);

    uri = "/a/some.html";

    msg = ov_http_create_request(
        (ov_http_message_config){0}, (ov_http_version){0},
        (ov_http_request){.method.start = (uint8_t *)"get",
                          .method.length = 6,
                          .uri.start = (const uint8_t *)uri,
                          .uri.length = strlen(uri)});

    testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_request_line(
                                          msg->buffer->start,
                                          msg->buffer->length, &msg->request,
                                          &msg->version, 100, NULL));

    memset(path, 0, PATH_MAX);
    memset(check, 0, PATH_MAX);

    testrun(0 < snprintf(check, PATH_MAX, "%s/a/some.html", root_path));
    testrun(ov_webserver_base_uri_file_path(server, conn, msg, PATH_MAX, path));
    testrun(0 == strcmp(check, path));
    msg = ov_http_message_free(msg);

    SSL_free(ssl);
    SSL_CTX_free(ctx);
    testrun(NULL == ov_webserver_base_free(server));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_webserver_base_find_domain() {

    int data[100] = {0};
    void *userdata = &data;

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = (ov_webserver_base_config){
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 10 * 1000 * 1000,
        .callback.userdata = userdata,
        .callback.accept = accept_socket,
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0}};

    set_resources_dir(config.domain_config_path);

    ov_webserver_base *server = ov_webserver_base_create(config);
    testrun(server);

    char *domain = TEST_DOMAIN_NAME;
    ov_memory_pointer p = {.start = (uint8_t *)domain,
                           .length = strlen(domain)};

    testrun(!ov_webserver_base_find_domain(NULL, p));
    testrun(!ov_webserver_base_find_domain(server, (ov_memory_pointer){0}));
    testrun(ov_webserver_base_find_domain(server, p));

    testrun(NULL == ov_webserver_base_free(server));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_process_https() {

    int data[100] = {0};
    void *userdata = &data;

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_webserver_base_config config = (ov_webserver_base_config){
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 10 * 1000 * 1000,
        .callback.userdata = userdata,
        .callback.accept = accept_socket,
        .loop = loop,
        .http.secure = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .http.redirect = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),
        .stun.socket = {0}};

    set_resources_dir(config.domain_config_path);

    ov_webserver_base *server = ov_webserver_base_create(config);
    testrun(server);

    /* create some client connection */
    int client = ov_socket_create(config.http.secure, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    struct Connection *conn = NULL;

    for (size_t i = 0; i < server->connections_max; i++) {

        if (server->conn[i].fd != -1) {
            conn = &server->conn[i];
            break;
        }
    }

    testrun(conn);

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL *ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);

    /* start handshaking */
    int errorcode = -1;
    int err = 0;

    int r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    ov_http_message_config http_config = (ov_http_message_config){0};
    ov_http_version http_version = (ov_http_version){.major = 1, .minor = 1};

    ov_http_message *msg =
        ov_http_create_request_string(http_config, http_version, "GET", "/");
    testrun(msg);
    testrun(ov_http_message_add_header_string(msg, OV_HTTP_KEY_HOST,
                                              "openvocs.test"));
    testrun(ov_http_message_close_header(msg));
    testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_message(msg, NULL));

    conn->data = msg;

    testrun(process_https(server, conn));
    testrun(conn->data);
    msg = ov_http_message_cast(conn->data);
    testrun(msg);
    // fprintf(stdout, "%i|%s\n", (int)msg->buffer->length,
    // (char*)msg->buffer->start);

    char *str = "GET /wiki/Katzen HTTP/1.1\r\nHost: openvocs.test\r\n\r\n";
    char *expect = "GET /wiki/Katzen HTTP/1.1\r\nHost: ";
    testrun(ov_buffer_set(msg->buffer, str, strlen(str)));
    testrun(ov_buffer_push(msg->buffer, expect, strlen(expect)));

    // fprintf(stdout, "%i|%s\n", (int)msg->buffer->length,
    // (char*)msg->buffer->start);
    testrun(process_https(server, conn));
    testrun(conn->data);
    msg = ov_http_message_cast(conn->data);
    testrun(msg);
    // fprintf(stdout, "expect %i %i|%s\n", (int) strlen(expect),
    // (int)msg->buffer->length, (char*)msg->buffer->start);
    testrun(msg->buffer->length == strlen(expect));
    testrun(0 == memcmp(expect, msg->buffer->start, msg->buffer->length));

    SSL_free(ssl);
    SSL_CTX_free(ctx);
    testrun(NULL == ov_webserver_base_free(server));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

OV_TEST_RUN("ov_webserver_base", domains_init,

            test_ov_webserver_base_cast, test_ov_webserver_base_create,
            test_ov_webserver_base_free,

            test_ov_webserver_base_close,

            test_ov_webserver_base_send_secure,
            test_ov_webserver_base_send_json, test_ov_webserver_base_answer_get,

            test_ov_webserver_base_uri_file_path,
            test_ov_webserver_base_find_domain,

            test_ov_webserver_base_configure_websocket_callback,
            test_ov_webserver_base_configure_uri_event_io,

            test_ov_webserver_base_config_from_json,
            test_ov_webserver_base_config_to_json,

            check_stun, check_redirect, check_tls_send_buffer, check_sni,
            check_wss_events, check_websocket_callback,
            check_websocket_callback_fragmentation, check_process_https,
            domains_deinit);
