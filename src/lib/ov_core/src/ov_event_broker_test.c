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
        @file           ov_event_broker_test.c
        @author         Töpfer, Markus

        @date           2026-04-07


        ------------------------------------------------------------------------
*/
#include <ov_test/testrun.h>
#include "ov_event_broker.c"

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

#define TEST_RUNTIME_USECS 50 * 1000

#define TEST_DOMAIN_NAME "openvocs.test"
#define TEST_DOMAIN_NAME_ONE "one.test"
#define TEST_DOMAIN_NAME_TWO "two.test"

static const char *test_resource_dir = 0;
static const char *test_resource_password = 0;
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
    free((char *)test_resource_password);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int domains_init() {

    test_resource_dir = ov_test_get_resource_path("/resources");

    test_resource_password = ov_test_get_resource_path("resources/password");

    ov_file_write(test_resource_password, (uint8_t*) "password", 8, "wr");

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
    test_resource_password = ov_test_get_resource_path("resources/password");

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

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_event_broker_create(){

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);
    ov_io_config io_config = {.loop = loop};
    ov_io *io = ov_io_create(io_config);
    testrun(io);

    ov_event_broker_config config = {
        .loop = loop,
        .io = io
    };

    strncpy(config.password_path, test_resource_password, PATH_MAX);

    ov_event_broker *self = ov_event_broker_create(config);
    testrun(self);
    testrun(ov_event_broker_cast(self));
    testrun(self->events);
    testrun(self->json_io_buffer);
    testrun(self->clients);
    testrun(self->connections);

    testrun(NULL == ov_event_broker_free(self));
    testrun(NULL == ov_io_free(io));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

typedef struct Dummy {

    int socket;
    ov_json_value *msg;

} Dummy;

/*----------------------------------------------------------------------------*/

static void dummy_clear(Dummy *dummy){

    dummy->socket = 0;
    dummy->msg = ov_json_value_free(dummy->msg);
    return;
}

/*----------------------------------------------------------------------------*/

static void dummy_callback(void *userdata, const char *name, int socket, 
    const ov_json_value *input){

    UNUSED(name);

    Dummy *dummy = (Dummy*) userdata;
    dummy_clear(dummy);

    dummy->socket = socket;
    ov_json_value_copy((void**)&dummy->msg, input);
    return;
}

/*----------------------------------------------------------------------------*/

int test_ov_event_broker_free(){

    Dummy dummy1 = {0};
    Dummy dummy2 = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);
    ov_io_config io_config = {.loop = loop};
    ov_io *io = ov_io_create(io_config);
    testrun(io);

    ov_event_broker_config config = {
        .loop = loop,
        .io = io
    };

    strncpy(config.password_path, test_resource_password, PATH_MAX);

    ov_event_broker *self = ov_event_broker_create(config);
    testrun(self);

    ov_io_socket_config socket_config = {
        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TCP, .host = "localhost"})};

    int socket = ov_event_broker_open_listener(self, socket_config);
    testrun(0 < socket);

    testrun(ov_event_broker_register(self, "test1", &dummy1, dummy_callback));
    testrun(ov_event_broker_register(self, "test2", &dummy2, dummy_callback));

    ov_json_value *msg = ov_json_object();
    ov_json_value *par = ov_json_object();
    ov_json_object_set(msg, "event", ov_json_string("broker_login"));
    ov_json_object_set(msg, "parameter", par);
    ov_json_object_set(par, "password", ov_json_string("password"));

    testrun(ov_event_broker_push(self, socket, msg));

    testrun(NULL == ov_io_free(io));
    testrun(NULL == ov_event_broker_free(self));
    
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_broker_register(){

    Dummy dummy1 = {0};
    Dummy dummy2 = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_io_config io_config = {.loop = loop};
    ov_io *io = ov_io_create(io_config);
    
    testrun(io);

    ov_event_broker_config config = {
        .loop = loop,
        .io = io
    };

    strncpy(config.password_path, test_resource_password, PATH_MAX);

    ov_event_broker *self = ov_event_broker_create(config);
    testrun(self);

    testrun(!ov_event_broker_register(NULL, NULL, NULL, NULL));
    testrun(!ov_event_broker_register(NULL, "test1", &dummy1, dummy_callback));
    testrun(!ov_event_broker_register(self, NULL, &dummy1, dummy_callback));
    testrun(!ov_event_broker_register(self, "test1",NULL, dummy_callback));
    testrun(!ov_event_broker_register(self, "test1", &dummy1, NULL));
    testrun(ov_event_broker_register(self, "test1", &dummy1, dummy_callback));
    testrun(ov_event_broker_register(self, "test2", &dummy2, dummy_callback));

    testrun(NULL == ov_io_free(io));
    testrun(NULL == ov_event_broker_free(self));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_event_broker_push(){

    Dummy dummy1 = {0};
    Dummy dummy2 = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);
    ov_io_config io_config = {.loop = loop};
    ov_io *io = ov_io_create(io_config);
    testrun(io);

    ov_event_broker_config config = {
        .loop = loop,
        .io = io
    };

    strncpy(config.password_path, test_resource_password, PATH_MAX);

    ov_event_broker *self = ov_event_broker_create(config);
    testrun(self);

    ov_io_socket_config socket_config = {
        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TCP, .host = "localhost"})};

    int socket = ov_event_broker_open_listener(self, socket_config);
    testrun(0 < socket);

    int client = ov_socket_create(socket_config.socket, true, NULL);
    testrun(-1 != client);
    testrun(ov_socket_ensure_nonblocking(client));

    testrun(ov_event_broker_register(self, "test", &dummy1, dummy_callback));
    testrun(ov_event_broker_register(self, "test1", &dummy2, dummy_callback));
    testrun(6 == ov_dict_count(self->events));

    ov_json_value *msg = ov_json_object();
    ov_json_value *par = ov_json_object();
    ov_json_object_set(msg, "event", ov_json_string("broker_login"));
    ov_json_object_set(msg, "parameter", par);
    ov_json_object_set(par, "password", ov_json_string("password"));

    char *str = ov_json_value_to_string(msg);

    char buffer[2048] = {0};
    size_t size = 2028;

    ssize_t bytes = -1;
    while(-1 == bytes){

        bytes = send(client, str, strlen(str), 0);
        testrun(ov_event_loop_run(loop, OV_RUN_ONCE));
    }

    bytes = -1;
    while(-1 == bytes){

        bytes = recv(client, buffer, size, 0);
        testrun(ov_event_loop_run(loop, OV_RUN_ONCE));
    }

    msg = ov_json_value_free(msg);
    str = ov_data_pointer_free(str);
    memset(buffer, 0, 2048);

    msg = ov_json_object();
    par = ov_json_object();
    ov_json_object_set(msg, "event", ov_json_string("subscribe"));
    ov_json_object_set(msg, "parameter", par);
    ov_json_object_set(par, "event", ov_json_string("test"));

    str = ov_json_value_to_string(msg);

    bytes = -1;
    while(-1 == bytes){

        bytes = send(client, str, strlen(str), 0);
        testrun(ov_event_loop_run(loop, OV_RUN_ONCE));
    }

    bytes = -1;
    while(-1 == bytes){

        bytes = recv(client, buffer, size, 0);
        testrun(ov_event_loop_run(loop, OV_RUN_ONCE));
    }

    msg = ov_json_value_free(msg);
    str = ov_data_pointer_free(str);
    memset(buffer, 0, 2048);

    msg = ov_json_object();
    par = ov_json_object();
    ov_json_object_set(msg, "event", ov_json_string("test"));
    ov_json_object_set(msg, "parameter", par);
    ov_json_object_set(par, "event", ov_json_string("test"));

    testrun(0 == dummy1.msg);
    testrun(0 == dummy2.msg);

    testrun(ov_event_broker_push(self, 25, msg));

    testrun(0 != dummy1.msg);
    testrun(0 == dummy2.msg);

    testrun(ov_event_api_event_is(dummy1.msg, "test"));
    bytes = -1;

    while(bytes == -1){

        testrun(ov_event_loop_run(loop, OV_RUN_ONCE));
        bytes = recv(client, buffer, size, 0);

    }

    msg = ov_json_value_from_string(buffer, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, "test"));

    msg = ov_json_value_free(msg);
    dummy_clear(&dummy1);

    testrun(NULL == ov_io_free(io));
    testrun(NULL == ov_event_broker_free(self));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST EXECUTION                                                  #EXEC
 *
 *      ------------------------------------------------------------------------
 */

OV_TEST_RUN("ov_event_broker", domains_init,

            test_ov_event_broker_create,
            test_ov_event_broker_free,
            test_ov_event_broker_register,
            test_ov_event_broker_push,

            domains_deinit
            );
