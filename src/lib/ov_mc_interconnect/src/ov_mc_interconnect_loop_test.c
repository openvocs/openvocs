/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_mc_interconnect_loop_test.c
        @author         Markus Töpfer

        @date           2023-12-11


        ------------------------------------------------------------------------
*/
#include "ov_mc_interconnect_loop.c"

#include <ov_test/ov_test.h>

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

#include "../include/ov_mc_interconnect_test_common.h"

/*----------------------------------------------------------------------------*/

static bool set_resources_dir(char *dest) {

    OV_ASSERT(0 != dest);

    char *res = strncpy(dest,
                        ov_mc_interconnect_test_common_get_test_resource_dir(),
                        PATH_MAX - 1);

    if (0 == res) return false;

    /* Check for plausibility ... */

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_mc_interconnect_loop_create() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_socket_configuration signaling = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TLS, .host = "localhost"});

    ov_socket_configuration media = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = UDP, .host = "localhost"});

    ov_socket_configuration internal = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = UDP, .host = "localhost"});

    ov_mc_interconnect_config config = (ov_mc_interconnect_config){

        .loop = loop,
        .name = "name",
        .password = "password",
        .socket.client = false,
        .socket.signaling = signaling,
        .socket.media = media,
        .socket.internal = internal,
        .dtls.cert = OV_TEST_CERT,
        .dtls.key = OV_TEST_CERT_KEY,
        .dtls.ca.file = OV_TEST_CERT

    };

    set_resources_dir(config.tls.domains);

    ov_mc_interconnect *base = ov_mc_interconnect_create(config);
    testrun(base);

    ov_mc_interconnect_loop *mc_loop =
        ov_mc_interconnect_loop_create((ov_mc_interconnect_loop_config){
            .loop = loop,
            .base = base,
            .name = "loop1",
            .socket = (ov_socket_configuration){
                .host = "224.0.0.1", .port = 12345, .type = UDP}});
    testrun(mc_loop);
    testrun(ov_mc_interconnect_loop_cast(mc_loop));
    testrun(-1 != mc_loop->socket);
    testrun(-1 != mc_loop->sender);
    testrun(0 != mc_loop->ssrc);

    testrun(mc_loop->ssrc == ov_mc_interconnect_loop_get_ssrc(mc_loop));

    testrun(0 == strcmp("loop1", ov_mc_interconnect_loop_get_name(mc_loop)));

    testrun(NULL == ov_mc_interconnect_loop_free(mc_loop));
    testrun(NULL == ov_mc_interconnect_free(base));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_interconnect_loop_free() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_socket_configuration signaling = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TLS, .host = "localhost"});

    ov_socket_configuration media = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = UDP, .host = "localhost"});

    ov_socket_configuration internal = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = UDP, .host = "localhost"});

    ov_mc_interconnect_config config = (ov_mc_interconnect_config){

        .loop = loop,
        .name = "name",
        .password = "password",
        .socket.signaling = signaling,
        .socket.media = media,
        .socket.internal = internal,
        .dtls.cert = OV_TEST_CERT,
        .dtls.key = OV_TEST_CERT_KEY,
        .dtls.ca.file = OV_TEST_CERT

    };

    set_resources_dir(config.tls.domains);

    ov_mc_interconnect *base = ov_mc_interconnect_create(config);
    testrun(base);

    ov_mc_interconnect_loop *mc_loop =
        ov_mc_interconnect_loop_create((ov_mc_interconnect_loop_config){
            .loop = loop,
            .base = base,
            .name = "loop1",
            .socket = (ov_socket_configuration){
                .host = "224.0.0.1", .port = 12345, .type = UDP}});
    testrun(mc_loop);
    testrun(ov_mc_interconnect_loop_cast(mc_loop));
    testrun(-1 != mc_loop->socket);
    testrun(-1 != mc_loop->sender);
    testrun(0 != mc_loop->ssrc);

    testrun(loop == (ov_event_loop *)ov_mc_interconnect_loop_free(
                        (ov_mc_interconnect_loop *)loop));

    testrun(NULL == ov_mc_interconnect_loop_free(mc_loop));
    testrun(NULL == ov_mc_interconnect_free(base));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_interconnect_loop_send() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_socket_configuration signaling = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TLS, .host = "localhost"});

    ov_socket_configuration media = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = UDP, .host = "localhost"});

    ov_socket_configuration internal = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = UDP, .host = "localhost"});

    ov_mc_interconnect_config config = (ov_mc_interconnect_config){

        .loop = loop,
        .name = "name",
        .password = "password",
        .socket.signaling = signaling,
        .socket.media = media,
        .socket.internal = internal,

        .dtls.cert = OV_TEST_CERT,
        .dtls.key = OV_TEST_CERT_KEY,
        .dtls.ca.file = OV_TEST_CERT

    };

    set_resources_dir(config.tls.domains);

    ov_mc_interconnect *base = ov_mc_interconnect_create(config);
    testrun(base);

    ov_mc_interconnect_loop *mc_loop =
        ov_mc_interconnect_loop_create((ov_mc_interconnect_loop_config){
            .loop = loop,
            .base = base,
            .name = "loop1",
            .socket = (ov_socket_configuration){
                .host = "224.0.0.1", .port = 12345, .type = UDP}});
    testrun(mc_loop);
    testrun(ov_mc_interconnect_loop_cast(mc_loop));

    ov_socket_configuration socket_config = (ov_socket_configuration){
        .type = UDP, .host = "224.0.0.1", .port = 12345};

    int socket = ov_mc_socket(socket_config);
    testrun(-1 != socket);

    testrun(ov_mc_interconnect_loop_send(mc_loop, (uint8_t *)"test", 4));

    char buf[OV_UDP_PAYLOAD_OCTETS] = {0};

    ov_socket_data remote = {};
    socklen_t src_addr_len = sizeof(remote.sa);

    ssize_t bytes = -1;

    while (-1 == bytes) {

        bytes = recvfrom(socket,
                         (char *)buf,
                         OV_UDP_PAYLOAD_OCTETS,
                         0,
                         (struct sockaddr *)&remote.sa,
                         &src_addr_len);
    }

    testrun(bytes == 4);
    testrun(0 == strcmp("test", buf));

    testrun(NULL == ov_mc_interconnect_loop_free(mc_loop));
    testrun(NULL == ov_mc_interconnect_free(base));
    testrun(NULL == ov_event_loop_free(loop));

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

OV_TEST_RUN("ov_mc_interconnect",
            ov_mc_interconnect_common_domains_init,

            test_ov_mc_interconnect_loop_create,
            test_ov_mc_interconnect_loop_free,
            test_ov_mc_interconnect_loop_send,

            ov_mc_interconnect_common_domains_deinit);
