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
        @file           ov_interconnect_test.c
        @author         Töpfer, Markus

        @date           2026-01-15


        ------------------------------------------------------------------------
*/
#include <ov_test/testrun.h>
#include "ov_interconnect.c"

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

static bool setup_interconnect(ov_event_loop **loop_out, 
    ov_io **io_out, ov_interconnect **interconnect_out){

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_io_config io_config = {.loop = loop};
    strncpy(io_config.domain.path, test_resource_dir, PATH_MAX);

    ov_io *io = ov_io_create(io_config);

    ov_interconnect_config config = (ov_interconnect_config){
        .loop = loop,
        .io = io,
        .socket.signaling = ov_socket_load_dynamic_port((ov_socket_configuration){
            .type = TCP,
            .host = "127.0.0.1"
        }),
        .socket.media = ov_socket_load_dynamic_port((ov_socket_configuration){
            .type = UDP,
            .host = "127.0.0.1"
        }),
        .socket.internal = ov_socket_load_dynamic_port((ov_socket_configuration){
            .type = TCP,
            .host = "127.0.0.1"
        }),
        .socket.mixer = ov_socket_load_dynamic_port((ov_socket_configuration){
            .type = TCP,
            .host = "127.0.0.1"
        }),

    };

    strncpy(config.name, "interconnect", OV_INTERCONNECT_INTERFACE_NAME_MAX);
    strncpy(config.password, "password", OV_INTERCONNECT_PASSWORD_MAX);
    strncpy(config.tls.domains, test_resource_dir, PATH_MAX);
    strncpy(config.tls.client.domain, "openvocs.test", PATH_MAX);
    strncpy(config.tls.client.ca.file, OV_TEST_CERT, PATH_MAX);

    strncpy(config.dtls.cert, OV_TEST_CERT, PATH_MAX);
    strncpy(config.dtls.key, OV_TEST_CERT_KEY, PATH_MAX);
    strncpy(config.dtls.ca.file, OV_TEST_CERT, PATH_MAX);
    strncpy(config.dtls.srtp.profile, OV_DTLS_SRTP_PROFILES, OV_DTLS_PROFILE_MAX);

    ov_interconnect *inter = ov_interconnect_create(config);

    *loop_out = loop;
    *io_out = io;
    *interconnect_out = inter;
    return true;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *generate_loop(ov_socket_configuration config){

    ov_json_value *out = NULL;
    ov_socket_configuration_to_json(config, &out);
    return out;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *generate_loop_config(){

    ov_json_value *out = ov_json_object();
    ov_json_value *interconnect = ov_json_object();
    ov_json_value *loops = ov_json_object();
    ov_json_object_set(out, "interconnect", interconnect);
    ov_json_object_set(interconnect, "loops", loops);

    ov_json_value *loop = generate_loop((ov_socket_configuration){
        .host = "224.0.0.1",
        .port = 60001,
        .type = UDP
    });
    ov_json_object_set(loops, "one", loop);

    loop = generate_loop((ov_socket_configuration){
        .host = "224.0.0.1",
        .port = 60002,
        .type = UDP
    });
    ov_json_object_set(loops, "two", loop);

    loop = generate_loop((ov_socket_configuration){
        .host = "224.0.0.1",
        .port = 60003,
        .type = UDP
    });
    ov_json_object_set(loops, "three", loop);

    return out;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_interconnect_create(){

    ov_event_loop *loop = NULL;
    ov_io *io = NULL;
    ov_interconnect *self = NULL;

    testrun(setup_interconnect(&loop, &io, &self));
    testrun(ov_interconnect_cast(self));

    testrun(self->dtls);
    testrun(self->app.signaling);
    testrun(self->app.mixer);
    testrun(self->session.by_signaling_remote);
    testrun(self->session.by_media_remote);
    testrun(self->loops);
    testrun(self->registered);
    testrun(self->mixers);
    testrun(0 < self->socket.signaling);
    testrun(0 < self->socket.media);
    testrun(0 < self->socket.mixer);
   
    testrun(NULL == ov_interconnect_free(self));
    testrun(NULL == ov_io_free(io));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_interconnect_load_loops(){

    ov_event_loop *loop = NULL;
    ov_io *io = NULL;
    ov_interconnect *self = NULL;

    testrun(setup_interconnect(&loop, &io, &self));
    testrun(ov_interconnect_cast(self));

    ov_json_value *loops = generate_loop_config();
    testrun(loops);

    char *str = ov_json_value_to_string(loops);
    fprintf(stdout, "%s\n", str);
    str = ov_data_pointer_free(str);

    testrun(ov_interconnect_load_loops(self, loops));
    testrun(3 == ov_dict_count(self->loops));
   
    loops = ov_json_value_free(loops);
    testrun(NULL == ov_interconnect_free(self));
    testrun(NULL == ov_io_free(io));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_interconnect_drop_mixer(){

    ov_event_loop *loop = NULL;
    ov_io *io = NULL;
    ov_interconnect *self = NULL;

    testrun(setup_interconnect(&loop, &io, &self));
    testrun(ov_interconnect_cast(self));

    int mixer = ov_socket_create(self->config.socket.mixer, true, NULL);
    ov_socket_ensure_nonblocking(mixer);

    ov_event_loop_run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_event_api_message_create("register", NULL, 0);
    char *str = ov_json_value_to_string(msg);
    msg = ov_json_value_free(msg);

    ov_socket_data dest = {0};
    ov_socket_fill_sockaddr_storage(&dest.sa, AF_INET,
        self->config.socket.mixer.host,
        self->config.socket.mixer.port);
    dest = ov_socket_data_from_sockaddr_storage(&dest.sa);
    socklen_t sa_len = sizeof(struct sockaddr_in);

    ssize_t bytes = -1;
    while(-1 == bytes){

        bytes = sendto(mixer, str, strlen(str), 0, (struct sockaddr*)&dest.sa, sa_len);
        ov_event_loop_run(loop, OV_RUN_ONCE);

    }

    str = ov_data_pointer_free(str);

    bytes = -1;

    ov_socket_data source = {0};

    char buffer[2048] = {0};
    size_t size = 2048;

    while (-1 == bytes){

        ov_event_loop_run(loop, OV_RUN_ONCE);
        bytes = recvfrom(mixer, buffer, size, 0, (struct sockaddr*)&source.sa, &sa_len);

    }

    msg = ov_json_value_from_string(buffer, bytes);
    testrun(ov_event_api_event_is(msg, "configure"));
    msg = ov_json_value_free(msg);

    // to get the mixer socket within the interconnect, we simply assign the mixer
    ov_mixer_data mdata = ov_interconnect_assign_mixer(self, "test");

    testrun(ov_interconnect_drop_mixer(self, mdata.socket));

    bytes = -1;

    while (-1 == bytes){

        ov_event_loop_run(loop, OV_RUN_ONCE);
        bytes = recvfrom(mixer, buffer, size, 0, (struct sockaddr*)&source.sa, &sa_len);

    }

    // close received as expected
    testrun(bytes == 0);
    
    testrun(NULL == ov_interconnect_free(self));
    testrun(NULL == ov_io_free(io));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int add_mixer(ov_interconnect *self, ov_event_loop *loop){

    int mixer = ov_socket_create(self->config.socket.mixer, true, NULL);
    ov_socket_ensure_nonblocking(mixer);

    ov_event_loop_run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_event_api_message_create("register", NULL, 0);
    char *str = ov_json_value_to_string(msg);
    msg = ov_json_value_free(msg);

    ov_socket_data dest = {0};
    ov_socket_fill_sockaddr_storage(&dest.sa, AF_INET,
        self->config.socket.mixer.host,
        self->config.socket.mixer.port);
    dest = ov_socket_data_from_sockaddr_storage(&dest.sa);
    socklen_t sa_len = sizeof(struct sockaddr_in);

    ssize_t bytes = -1;
    while(-1 == bytes){

        bytes = sendto(mixer, str, strlen(str), 0, (struct sockaddr*)&dest.sa, sa_len);
        ov_event_loop_run(loop, OV_RUN_ONCE);

    }

    str = ov_data_pointer_free(str);

    bytes = -1;

    ov_socket_data source = {0};

    char buffer[2048] = {0};
    size_t size = 2048;

    while (-1 == bytes){

        ov_event_loop_run(loop, OV_RUN_ONCE);
        bytes = recvfrom(mixer, buffer, size, 0, (struct sockaddr*)&source.sa, &sa_len);

    }

    msg = ov_json_value_from_string(buffer, bytes);
    testrun(ov_event_api_event_is(msg, "configure"));
    msg = ov_json_value_free(msg);

    return mixer;
}

/*----------------------------------------------------------------------------*/

int check_interconnect(){

    ov_event_loop *loop = NULL;
    ov_io *io = NULL;
    ov_interconnect *self = NULL;

    testrun(setup_interconnect(&loop, &io, &self));
    testrun(ov_interconnect_cast(self));

    ov_interconnect *client = NULL;
    ov_interconnect_config config = self->config;

    config.socket.client = true;
    config.socket.mixer = ov_socket_load_dynamic_port((ov_socket_configuration){
            .type = TCP,
            .host = "127.0.0.1"
        });
    config.socket.internal = ov_socket_load_dynamic_port((ov_socket_configuration){
            .type = UDP,
            .host = "127.0.0.1"
        });
    config.socket.media = ov_socket_load_dynamic_port((ov_socket_configuration){
            .type = UDP,
            .host = "127.0.0.1"
        });

    client = ov_interconnect_create(config);
    testrun(client);

    ov_json_value *loops = generate_loop_config();
    testrun(loops);

    testrun(ov_interconnect_load_loops(self, loops));
    testrun(ov_interconnect_load_loops(client, loops));

    for (int i = 0; i < 1000; i++){
        ov_event_loop_run(loop, OV_RUN_ONCE);
        usleep(5000);
    }

    testrun(1 == ov_dict_count(self->session.by_signaling_remote));
    testrun(1 == ov_dict_count(client->session.by_signaling_remote));
    
    int mixer1 = add_mixer(self, loop);
    int mixer2 = add_mixer(self, loop);
    int mixer3 = add_mixer(self, loop);

    int mixer4 = add_mixer(client, loop);
    int mixer5 = add_mixer(client, loop);
    int mixer6 = add_mixer(client, loop);

    testrun(mixer1);
    testrun(mixer2);
    testrun(mixer3);
    testrun(mixer4);
    testrun(mixer5);
    testrun(mixer6);

    testrun(NULL == ov_interconnect_free(self));
    testrun(NULL == ov_io_free(io));
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

OV_TEST_RUN("ov_interconnect_test", 

            domains_init,

            test_ov_interconnect_create,
            test_ov_interconnect_load_loops,
            test_ov_interconnect_drop_mixer,
            check_interconnect,

            domains_deinit);
