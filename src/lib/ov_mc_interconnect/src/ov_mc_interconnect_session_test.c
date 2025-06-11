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
        @file           ov_mc_interconnect_session_test.c
        @author         Markus

        @date           2023-12-13


        ------------------------------------------------------------------------
*/
#include "ov_mc_interconnect_session.c"

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
/*
static bool set_resources_dir(char *dest) {

    OV_ASSERT(0 != dest);

    char *res = strncpy(dest,
                        ov_mc_interconnect_test_common_get_test_resource_dir(),
                        PATH_MAX - 1);

    if (0 == res) return false;


    return true;
}
*/
/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_mc_interconnect_session_create() {
    /*
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

        ov_mc_interconnect *ic = ov_mc_interconnect_create(config);
        testrun(ic);

        ov_socket_data m = (ov_socket_data){
                .host = "localhost",
                .port = 12346
            };

        ov_mc_interconnect_session_config session_config = (
            ov_mc_interconnect_session_config){
            .base = ic,
            .loop = loop,
            .dtls = ov_mc_interconnect_get_dtls(ic),
            .signaling = 1,
            .internal = (ov_socket_configuration){
                .host = "localhost",
                .port = 0,
                .type = UDP
            },
            .remote.interface = "test",
            .remote.signaling = (ov_socket_data){
                .host = "localhost",
                .port = 12345
            },
            .remote.media = m
        };

        ov_mc_interconnect_session *session = ov_mc_interconnect_session_create(
            session_config);

        testrun(session);
        testrun(session->ssrcs);
        testrun(session->loops);
        testrun(ov_mc_interconnect_session_cast(session));
        testrun(OV_TIMER_INVALID != session->timer.keepalive);

        testrun(NULL == ov_mc_interconnect_free(ic));
        testrun(NULL == ov_event_loop_free(loop));
    */
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_interconnect_session_free() {
    /*
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

        ov_mc_interconnect *ic = ov_mc_interconnect_create(config);
        testrun(ic);

        ov_socket_data m = (ov_socket_data){
                .host = "localhost",
                .port = 12346
            };

        ov_mc_interconnect_session_config session_config = (
            ov_mc_interconnect_session_config){
            .base = ic,
            .loop = loop,
            .dtls = ov_mc_interconnect_get_dtls(ic),
            .signaling = 1,
            .internal = (ov_socket_configuration){
                .host = "localhost",
                .port = 0,
                .type = UDP
            },
            .remote.interface = "test",
            .remote.signaling = (ov_socket_data){
                .host = "localhost",
                .port = 12345
            },
            .remote.media = m
        };

        ov_mc_interconnect_session *session = ov_mc_interconnect_session_create(
            session_config);

        testrun(session);
        testrun(NULL == ov_mc_interconnect_session_free(session));

        testrun(NULL == ov_mc_interconnect_free(ic));
        testrun(NULL == ov_event_loop_free(loop));
    */
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

            test_ov_mc_interconnect_session_create,
            test_ov_mc_interconnect_session_free,

            ov_mc_interconnect_common_domains_deinit);
