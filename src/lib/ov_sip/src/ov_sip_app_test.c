/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include <ov_test/ov_test.h>

#include "ov_sip_app.c"
#include <ov_test/ov_test_tcp.h>

/*----------------------------------------------------------------------------*/

static ov_event_loop *get_loop() {

    return ov_event_loop_default(ov_event_loop_config_default());
}

/*----------------------------------------------------------------------------*/

static int test_ov_sip_app_create() {

    ov_sip_app_configuration cfg = {0};

    testrun(0 == ov_sip_app_create(0, 0, cfg));
    testrun(0 == ov_sip_app_create("SIP", 0, cfg));

    ov_event_loop *loop = get_loop();

    testrun(0 == ov_sip_app_create(0, loop, cfg));
    ov_sip_app *app = ov_sip_app_create("SIP", loop, cfg);

    testrun(0 != app);

    app = ov_sip_app_free(app);

    testrun(0 == app);

    loop = ov_event_loop_free(loop);

    testrun(0 == loop);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sip_app_free() {

    // ov_sip_app *ov_sip_app_free(ov_sip_app *app);

    testrun(0 == ov_sip_app_free(0));

    ov_sip_app_configuration cfg = {0};

    ov_event_loop *loop = get_loop();

    ov_sip_app *app = ov_sip_app_create("SIP free", loop, cfg);
    testrun(0 != app);
    testrun(0 == ov_sip_app_free(app));

    ov_event_loop_free(loop);

    return testrun_log_success();
}

/*****************************************************************************
                                REGISTER HANDLER
 ****************************************************************************/

void cb_invite_dummy(ov_sip_message const *msg, int fd, void *additional) {

    UNUSED(msg);
    UNUSED(fd);
    UNUSED(additional);
}

/*----------------------------------------------------------------------------*/

int test_ov_sip_app_register_handler() {

    testrun(!ov_sip_app_register_handler(0, 0, 0));

    ov_event_loop *loop = get_loop();
    ov_sip_app_configuration cfg = {0};

    ov_sip_app *app = ov_sip_app_create("SIP register handler", loop, cfg);
    testrun(0 != app);

    testrun(!ov_sip_app_register_handler(app, 0, 0));
    testrun(!ov_sip_app_register_handler(0, "INVITE", 0));
    testrun(!ov_sip_app_register_handler(app, "INVITE", 0));
    testrun(!ov_sip_app_register_handler(0, 0, cb_invite_dummy));
    testrun(!ov_sip_app_register_handler(app, 0, cb_invite_dummy));
    testrun(ov_sip_app_register_handler(app, "INVITE", cb_invite_dummy));

    app = ov_sip_app_free(app);

    loop = ov_event_loop_free(loop);

    testrun(0 == app);
    testrun(0 == loop);

    return testrun_log_success();
}

/*****************************************************************************
                                    Connect
 ****************************************************************************/

static void run_for(ov_event_loop *loop, uint64_t max_run_interval_sec) {

    ov_event_loop_run(loop, max_run_interval_sec * 1000 * 1000);
}

/*----------------------------------------------------------------------------*/

struct handler_data {

    int listen_socket;

    char *method;
    char *uri;

    void (*handler_called)(ov_sip_message const *, int, void *);
};

/*----------------------------------------------------------------------------*/

static void cb_reconnected(int sckt, void *additional) {

    TEST_ASSERT(0 != additional);
    TEST_ASSERT(-1 < sckt);

    struct handler_data *data = additional;
    data->listen_socket = sckt;

    data->handler_called = 0;
}

/*----------------------------------------------------------------------------*/

static void cb_register(ov_sip_message const *msg, int fh, void *additional) {

    UNUSED(fh);

    TEST_ASSERT(0 != msg);
    TEST_ASSERT(0 != additional);

    struct handler_data *data = additional;

    ov_free(data->method);
    ov_free(data->uri);

    data->method = strdup(ov_sip_message_method(msg));
    data->uri = strdup(ov_sip_message_uri(msg));

    data->handler_called = cb_register;
}

/*----------------------------------------------------------------------------*/

static void cb_invite(ov_sip_message const *msg, int fh, void *additional) {

    UNUSED(fh);

    TEST_ASSERT(0 != msg);
    TEST_ASSERT(0 != additional);

    struct handler_data *data = additional;

    ov_free(data->method);
    ov_free(data->uri);

    data->method = strdup(ov_sip_message_method(msg));
    data->uri = strdup(ov_sip_message_uri(msg));

    data->handler_called = cb_invite;
}

/*----------------------------------------------------------------------------*/

int test_ov_sip_app_connect() {

    // Start echoing test server

    ov_test_tcp_server_config tcp_srv_cfg = {
        .serve_client_callback = ov_test_tcp_server_loopback,
    };

    pid_t server_pid = 0;
    int server_port = -1;

    testrun(ov_test_tcp_server(&server_pid, &server_port, tcp_srv_cfg));

    ov_socket_configuration cfg = {
        .host = "localhost",
        .port = server_port,
        .type = TCP,
    };

    testrun(!ov_sip_app_connect(0, cfg));

    struct handler_data data = {
        .listen_socket = -1,
    };

    ov_sip_app_configuration sip_cfg = {
        .reconnect_interval_secs = 2,
        .cb_reconnected = cb_reconnected,
        .additional = &data,
    };

    ov_event_loop *loop = get_loop();
    ov_sip_app *app = ov_sip_app_create("SIP connect", loop, sip_cfg);

    testrun(0 != app);
    testrun(ov_sip_app_register_handler(app, "REGISTER", cb_register));
    testrun(ov_sip_app_register_handler(app, "INVITE", cb_invite));

    // bool ov_sip_app_connect(ov_sip_app *self, ov_socket_configuration
    // config);

    testrun(ov_sip_app_connect(app, cfg));

    run_for(loop, 3);

    // Reconnect should have happened -> cb_reconnected called ->server_socket
    testrun(-1 < data.listen_socket);

    ov_sip_message *msg =
        ov_sip_message_request_create("REGISTER", "hinter:tutlingen");

    testrun(ov_sip_app_send(app, data.listen_socket, msg));

    run_for(loop, 3);

    // Test TCP server should have echoed back the serialized message,
    // the handler for "REGISTER" should have been called

    testrun(0 != data.method);
    testrun(0 != data.uri);
    testrun(0 == strcmp("hinter:tutlingen", data.uri));
    testrun(0 == strcmp("REGISTER", data.method));
    testrun(cb_register == data.handler_called);

    msg = ov_sip_message_free(msg);

    msg = ov_sip_message_request_create("INVITE", "vorder:echingen");

    testrun(ov_sip_app_send(app, data.listen_socket, msg));

    run_for(loop, 3);

    // Test TCP server should have echoed back the serialized message,
    // the handler for "REGISTER" should have been called

    testrun(0 != data.method);
    testrun(0 != data.uri);
    testrun(0 == strcmp("vorder:echingen", data.uri));
    testrun(0 == strcmp("INVITE", data.method));
    testrun(cb_invite == data.handler_called);

    ov_free(data.method);
    ov_free(data.uri);

    msg = ov_sip_message_free(msg);
    testrun(0 == msg);

    testrun(ov_test_tcp_stop_process(server_pid));

    app = ov_sip_app_free(app);
    loop = ov_event_loop_free(loop);

    testrun(0 == app);
    testrun(0 == loop);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sip_app_open_server_socket() { return testrun_log_success(); }

/*----------------------------------------------------------------------------*/

int test_ov_sip_app_send() {

    // Just check error cases - actual functionality checked in
    // test_ov_sip_app_connect

    // bool ov_sip_app_send(ov_sip_app *self, int fd, ov_sip_message *msg);

    testrun(!ov_sip_app_send(0, -1, 0));

    ov_event_loop *loop = get_loop();
    ov_sip_app_configuration cfg = {0};

    ov_sip_app *app = ov_sip_app_create("SIP register handler", loop, cfg);
    testrun(0 != app);

    testrun(!ov_sip_app_send(app, -1, 0));

    int sockets[2] = {0};

    testrun(0 == socketpair(AF_UNIX, SOCK_STREAM, 0, sockets));

    testrun(!ov_sip_app_send(0, sockets[0], 0));
    testrun(!ov_sip_app_send(app, sockets[0], 0));

    ov_sip_message *msg =
        ov_sip_message_request_create("REGISTER", "hinter:tutlingen");

    testrun(!ov_sip_app_send(0, -1, msg));
    testrun(!ov_sip_app_send(app, -1, msg));

    testrun(ov_sip_app_send(app, sockets[0], msg));

    // Read from other end of pipe and check whether the msg was properly
    // serialized

    ov_serde *serde = ov_sip_serde();

    char *msg_str = ov_sip_message_to_string(serde, msg);
    testrun(0 != msg_str);

    serde = ov_serde_free(serde);

    testrun(ov_test_tcp_received(sockets[1], msg_str));

    free(msg_str);
    msg_str = 0;

    close(sockets[0]);
    close(sockets[1]);
    sockets[0] = -1;
    sockets[1] = -1;

    msg = ov_sip_message_free(msg);

    app = ov_sip_app_free(app);
    loop = ov_event_loop_free(loop);

    testrun(0 == serde);
    testrun(0 == msg);
    testrun(0 == app);
    testrun(0 == loop);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int tear_down() {

    ov_registered_cache_free_all();
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_sip_app", test_ov_sip_app_create, test_ov_sip_app_free,
            test_ov_sip_app_register_handler, test_ov_sip_app_connect,
            test_ov_sip_app_open_server_socket, test_ov_sip_app_send,
            tear_down);

/*----------------------------------------------------------------------------*/
