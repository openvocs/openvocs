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
        @file           ov_mc_mixer_app_test.c
        @author         Markus Töpfer

        @date           2022-11-11


        ------------------------------------------------------------------------
*/
#include "ov_mc_mixer_app.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_mc_mixer_app_create() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_socket_configuration m = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    int manager = ov_socket_create(m, false, NULL);
    testrun(manager);
    testrun(ov_socket_ensure_nonblocking(manager));

    ov_mc_mixer_app_config config =
        (ov_mc_mixer_app_config){.loop = loop, .manager = m};

    ov_mc_mixer_app *app = ov_mc_mixer_app_create(config);
    testrun(app);
    testrun(ov_mc_mixer_app_cast(app));
    testrun(app->app);
    testrun(app->mixer);

    char buf[1024] = {0};
    ssize_t bytes = -1;

    struct sockaddr_storage sa = {0};
    socklen_t sa_len = sizeof(sa);

    int conn = accept(manager, (struct sockaddr *)&sa, &sa_len);
    testrun(ov_socket_ensure_nonblocking(conn));

    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    ov_json_value *msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_REGISTER));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_UUID));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));
    msg = ov_json_value_free(msg);

    testrun(NULL == ov_mc_mixer_app_free(app));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_mixer_app_free() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_socket_configuration m = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    int manager = ov_socket_create(m, false, NULL);
    testrun(manager);
    testrun(ov_socket_ensure_nonblocking(manager));

    ov_mc_mixer_app_config config =
        (ov_mc_mixer_app_config){.loop = loop, .manager = m};

    ov_mc_mixer_app *app = ov_mc_mixer_app_create(config);
    testrun(app);

    testrun(NULL == ov_mc_mixer_app_free(app));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_mixer_app_cast() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_socket_configuration m = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    int manager = ov_socket_create(m, false, NULL);
    testrun(manager);
    testrun(ov_socket_ensure_nonblocking(manager));

    ov_mc_mixer_app_config config =
        (ov_mc_mixer_app_config){.loop = loop, .manager = m};

    ov_mc_mixer_app *app = ov_mc_mixer_app_create(config);
    testrun(app);

    for (size_t i = 0; i < 0xffff; i++) {

        app->magic_bytes = i;

        if (i == OV_MC_MIXER_APP_MAGIC_BYTES) {
            testrun(ov_mc_mixer_app_cast(app));
        } else {
            testrun(!ov_mc_mixer_app_cast(app));
        }
    }

    app->magic_bytes = OV_MC_MIXER_APP_MAGIC_BYTES;

    testrun(NULL == ov_mc_mixer_app_free(app));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_mixer_app_config_from_json() {

    char *str = "{"
                "\"app\" :"
                "{"
                "\"resource_manager\":"
                "{"
                "\"host\" : \"127.0.0.1\","
                "\"port\" : 12346,"
                "\"type\" : \"TCP\""
                "},"
                "\"limit\" :"
                "{"
                "\"reconnect_interval_secs\": 3"
                "}"
                "}"
                "}";

    ov_json_value *in = ov_json_value_from_string(str, strlen(str));
    testrun(in);

    ov_mc_mixer_app_config config = ov_mc_mixer_app_config_from_json(in);
    testrun(3 == config.limit.client_connect_sec);
    testrun(0 == strcmp(config.manager.host, "127.0.0.1"));
    testrun(TCP == config.manager.type);
    testrun(12346 == config.manager.port);

    testrun(NULL == ov_json_value_free(in));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_configure() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_socket_configuration m = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    int manager = ov_socket_create(m, false, NULL);
    testrun(manager);
    testrun(ov_socket_ensure_nonblocking(manager));

    ov_mc_mixer_app_config config =
        (ov_mc_mixer_app_config){.loop = loop, .manager = m};

    ov_mc_mixer_app *app = ov_mc_mixer_app_create(config);
    testrun(app);

    char buf[1024] = {0};
    ssize_t bytes = -1;

    struct sockaddr_storage sa = {0};
    socklen_t sa_len = sizeof(sa);

    int conn = accept(manager, (struct sockaddr *)&sa, &sa_len);
    testrun(ov_socket_ensure_nonblocking(conn));

    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    ov_json_value *msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_REGISTER));
    msg = ov_json_value_free(msg);

    ov_json_value *in = ov_mc_mixer_msg_configure((ov_mc_mixer_core_config){0});
    testrun(in);

    cb_event_configure(app, "name", app->socket, in);

    testrun(NULL == ov_mc_mixer_app_free(app));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_acquire() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_socket_configuration m = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    int manager = ov_socket_create(m, false, NULL);
    testrun(manager);
    testrun(ov_socket_ensure_nonblocking(manager));

    ov_mc_mixer_app_config config =
        (ov_mc_mixer_app_config){.loop = loop, .manager = m};

    ov_mc_mixer_app *app = ov_mc_mixer_app_create(config);
    testrun(app);

    char buf[1024] = {0};
    ssize_t bytes = -1;

    struct sockaddr_storage sa = {0};
    socklen_t sa_len = sizeof(sa);

    int conn = accept(manager, (struct sockaddr *)&sa, &sa_len);
    testrun(ov_socket_ensure_nonblocking(conn));

    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    ov_json_value *msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_REGISTER));
    msg = ov_json_value_free(msg);

    ov_json_value *in = ov_mc_mixer_msg_acquire(
        "username", (ov_mc_mixer_core_forward){.ssrc = 12345,
                                               .payload_type = 1,
                                               .socket.host = "127.0.0.1",
                                               .socket.port = 12345,
                                               .socket.type = UDP});
    testrun(in);

    cb_event_acquire(app, "name", app->socket, in);

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_ACQUIRE));
    testrun(0 == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    // check success
    testrun(0 == strcmp(ov_mc_mixer_core_get_name(app->mixer), "username"));
    ov_mc_mixer_core_forward forward = ov_mc_mixer_core_get_forward(app->mixer);
    testrun(12345 == forward.ssrc);
    testrun(0 == strcmp("127.0.0.1", forward.socket.host));
    testrun(UDP == forward.socket.type);
    testrun(12345 == forward.socket.port);

    // reacquire to different user

    in = ov_mc_mixer_msg_acquire(
        "name", (ov_mc_mixer_core_forward){.ssrc = 12345,
                                           .payload_type = 1,
                                           .socket.host = "127.0.0.1",
                                           .socket.port = 12345,
                                           .socket.type = UDP});
    testrun(in);

    cb_event_acquire(app, "name", app->socket, in);

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_ACQUIRE));
    testrun(0 == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    // check success
    testrun(0 == strcmp(ov_mc_mixer_core_get_name(app->mixer), "name"));
    forward = ov_mc_mixer_core_get_forward(app->mixer);
    testrun(12345 == forward.ssrc);
    testrun(0 == strcmp("127.0.0.1", forward.socket.host));
    testrun(UDP == forward.socket.type);
    testrun(12345 == forward.socket.port);

    // input error

    in = ov_mc_mixer_msg_acquire(
        "name", (ov_mc_mixer_core_forward){
                    .ssrc = 12345, .socket.port = 12345, .socket.type = UDP});
    testrun(in);

    cb_event_acquire(app, "name", app->socket, in);

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_ACQUIRE));
    testrun(OV_ERROR_CODE_PROCESSING_ERROR == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    // check nothing changed in acquisition
    testrun(0 == strcmp(ov_mc_mixer_core_get_name(app->mixer), "name"));
    forward = ov_mc_mixer_core_get_forward(app->mixer);
    testrun(12345 == forward.ssrc);
    testrun(0 == strcmp("127.0.0.1", forward.socket.host));
    testrun(UDP == forward.socket.type);
    testrun(12345 == forward.socket.port);

    testrun(NULL == ov_mc_mixer_app_free(app));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_release() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_socket_configuration m = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    int manager = ov_socket_create(m, false, NULL);
    testrun(manager);
    testrun(ov_socket_ensure_nonblocking(manager));

    ov_mc_mixer_app_config config =
        (ov_mc_mixer_app_config){.loop = loop, .manager = m};

    ov_mc_mixer_app *app = ov_mc_mixer_app_create(config);
    testrun(app);

    char buf[1024] = {0};
    ssize_t bytes = -1;

    struct sockaddr_storage sa = {0};
    socklen_t sa_len = sizeof(sa);

    int conn = accept(manager, (struct sockaddr *)&sa, &sa_len);
    testrun(ov_socket_ensure_nonblocking(conn));

    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    ov_json_value *msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_REGISTER));
    msg = ov_json_value_free(msg);

    ov_json_value *in = ov_mc_mixer_msg_release("username");
    testrun(in);

    cb_event_release(app, "name", app->socket, in);

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_RELEASE));
    testrun(0 == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    // acquire to user

    in = ov_mc_mixer_msg_acquire(
        "username", (ov_mc_mixer_core_forward){.ssrc = 12345,
                                               .payload_type = 1,
                                               .socket.host = "127.0.0.1",
                                               .socket.port = 12345,
                                               .socket.type = UDP});
    testrun(in);

    cb_event_acquire(app, "name", app->socket, in);

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_ACQUIRE));
    testrun(0 == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    // check success
    testrun(0 == strcmp(ov_mc_mixer_core_get_name(app->mixer), "username"));
    ov_mc_mixer_core_forward forward = ov_mc_mixer_core_get_forward(app->mixer);
    testrun(12345 == forward.ssrc);
    testrun(0 == strcmp("127.0.0.1", forward.socket.host));
    testrun(UDP == forward.socket.type);
    testrun(12345 == forward.socket.port);

    in = ov_mc_mixer_msg_release("username");
    testrun(in);

    cb_event_release(app, "name", app->socket, in);

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // check release
    testrun(NULL == ov_mc_mixer_core_get_name(app->mixer));

    // fprintf(stdout, "%s", buf);

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_RELEASE));
    testrun(0 == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    testrun(NULL == ov_mc_mixer_app_free(app));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_join() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_socket_configuration m = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    int manager = ov_socket_create(m, false, NULL);
    testrun(manager);
    testrun(ov_socket_ensure_nonblocking(manager));

    ov_mc_mixer_app_config config =
        (ov_mc_mixer_app_config){.loop = loop, .manager = m};

    ov_mc_mixer_app *app = ov_mc_mixer_app_create(config);
    testrun(app);

    char buf[1024] = {0};
    ssize_t bytes = -1;

    struct sockaddr_storage sa = {0};
    socklen_t sa_len = sizeof(sa);

    int conn = accept(manager, (struct sockaddr *)&sa, &sa_len);
    testrun(ov_socket_ensure_nonblocking(conn));

    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    ov_json_value *msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_REGISTER));
    msg = ov_json_value_free(msg);

    // join without acquisition

    ov_json_value *in =
        ov_mc_mixer_msg_join((ov_mc_loop_data){.socket.host = "229.0.0.1",
                                               .socket.port = 12345,
                                               .socket.type = UDP,
                                               .name = "loop1",
                                               .volume = 50});
    testrun(in);

    cb_event_join(app, "name", app->socket, in);

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_JOIN));
    testrun(OV_ERROR_CODE_PROCESSING_ERROR == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    // acquire to user

    in = ov_mc_mixer_msg_acquire(
        "username", (ov_mc_mixer_core_forward){.ssrc = 12345,
                                               .payload_type = 1,
                                               .socket.host = "127.0.0.1",
                                               .socket.port = 12345,
                                               .socket.type = UDP});
    testrun(in);

    cb_event_acquire(app, "name", app->socket, in);

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_ACQUIRE));
    testrun(0 == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    in = ov_mc_mixer_msg_join((ov_mc_loop_data){.socket.host = "229.0.0.1",
                                                .socket.port = 12345,
                                                .socket.type = UDP,
                                                .name = "loop1",
                                                .volume = 50});
    testrun(in);

    cb_event_join(app, "name", app->socket, in);

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_JOIN));
    testrun(0 == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    testrun(NULL == ov_mc_mixer_app_free(app));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_leave() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_socket_configuration m = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    int manager = ov_socket_create(m, false, NULL);
    testrun(manager);
    testrun(ov_socket_ensure_nonblocking(manager));

    ov_mc_mixer_app_config config =
        (ov_mc_mixer_app_config){.loop = loop, .manager = m};

    ov_mc_mixer_app *app = ov_mc_mixer_app_create(config);
    testrun(app);

    char buf[1024] = {0};
    ssize_t bytes = -1;

    struct sockaddr_storage sa = {0};
    socklen_t sa_len = sizeof(sa);

    int conn = accept(manager, (struct sockaddr *)&sa, &sa_len);
    testrun(ov_socket_ensure_nonblocking(conn));

    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    ov_json_value *msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_REGISTER));
    msg = ov_json_value_free(msg);

    // leave without acquisition

    ov_json_value *in = ov_mc_mixer_msg_leave("loop1");
    testrun(in);

    cb_event_leave(app, "name", app->socket, in);

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_LEAVE));
    testrun(0 == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    // acquire to user

    in = ov_mc_mixer_msg_acquire(
        "username", (ov_mc_mixer_core_forward){.ssrc = 12345,
                                               .payload_type = 1,
                                               .socket.host = "127.0.0.1",
                                               .socket.port = 12345,
                                               .socket.type = UDP});
    testrun(in);

    cb_event_acquire(app, "name", app->socket, in);

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_ACQUIRE));
    testrun(0 == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    in = ov_mc_mixer_msg_join((ov_mc_loop_data){.socket.host = "229.0.0.1",
                                                .socket.port = 12345,
                                                .socket.type = UDP,
                                                .name = "loop1",
                                                .volume = 50});
    testrun(in);

    cb_event_join(app, "name", app->socket, in);

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_JOIN));
    testrun(0 == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    in = ov_mc_mixer_msg_leave("loop1");
    testrun(in);

    cb_event_leave(app, "name", app->socket, in);

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_LEAVE));
    testrun(0 == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    testrun(NULL == ov_mc_mixer_app_free(app));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_volume() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_socket_configuration m = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    int manager = ov_socket_create(m, false, NULL);
    testrun(manager);
    testrun(ov_socket_ensure_nonblocking(manager));

    ov_mc_mixer_app_config config =
        (ov_mc_mixer_app_config){.loop = loop, .manager = m};

    ov_mc_mixer_app *app = ov_mc_mixer_app_create(config);
    testrun(app);

    char buf[1024] = {0};
    ssize_t bytes = -1;

    struct sockaddr_storage sa = {0};
    socklen_t sa_len = sizeof(sa);

    int conn = accept(manager, (struct sockaddr *)&sa, &sa_len);
    testrun(ov_socket_ensure_nonblocking(conn));

    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    ov_json_value *msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_REGISTER));
    msg = ov_json_value_free(msg);

    // leave without acquisition

    ov_json_value *in = ov_mc_mixer_msg_volume("loop1", 50);
    testrun(in);

    cb_event_volume(app, "name", app->socket, in);

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_VOLUME));
    testrun(OV_ERROR_CODE_PROCESSING_ERROR == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    // acquire to user

    in = ov_mc_mixer_msg_acquire(
        "username", (ov_mc_mixer_core_forward){.ssrc = 12345,
                                               .payload_type = 1,
                                               .socket.host = "127.0.0.1",
                                               .socket.port = 12345,
                                               .socket.type = UDP});
    testrun(in);

    cb_event_acquire(app, "name", app->socket, in);

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_ACQUIRE));
    testrun(0 == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    in = ov_mc_mixer_msg_join((ov_mc_loop_data){.socket.host = "229.0.0.1",
                                                .socket.port = 12345,
                                                .socket.type = UDP,
                                                .name = "loop1",
                                                .volume = 50});
    testrun(in);

    cb_event_join(app, "name", app->socket, in);

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_JOIN));
    testrun(0 == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    in = ov_mc_mixer_msg_volume("loop1", 50);
    testrun(in);

    cb_event_volume(app, "name", app->socket, in);

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_VOLUME));
    testrun(0 == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    testrun(NULL == ov_mc_mixer_app_free(app));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_shutdown() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_socket_configuration m = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    int manager = ov_socket_create(m, false, NULL);
    testrun(manager);
    testrun(ov_socket_ensure_nonblocking(manager));

    ov_mc_mixer_app_config config =
        (ov_mc_mixer_app_config){.loop = loop, .manager = m};

    ov_mc_mixer_app *app = ov_mc_mixer_app_create(config);
    testrun(app);

    char buf[1024] = {0};
    ssize_t bytes = -1;

    struct sockaddr_storage sa = {0};
    socklen_t sa_len = sizeof(sa);

    int conn = accept(manager, (struct sockaddr *)&sa, &sa_len);
    testrun(ov_socket_ensure_nonblocking(conn));

    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);

    ov_json_value *msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_REGISTER));
    msg = ov_json_value_free(msg);

    // shutdown without acquisition

    ov_json_value *in = ov_mc_mixer_msg_shutdown();
    testrun(in);

    cb_event_shutdown(app, "name", app->socket, in);

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(conn, buf, 1024, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    testrun(0 == bytes);
    testrun(NULL == ov_mc_mixer_app_free(app));
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

int all_tests() {

    testrun_init();
    testrun_test(test_ov_mc_mixer_app_create);
    testrun_test(test_ov_mc_mixer_app_free);
    testrun_test(test_ov_mc_mixer_app_cast);
    testrun_test(test_ov_mc_mixer_app_config_from_json);

    testrun_test(check_cb_event_configure);
    testrun_test(check_cb_event_acquire);
    testrun_test(check_cb_event_release);
    testrun_test(check_cb_event_join);
    testrun_test(check_cb_event_leave);
    testrun_test(check_cb_event_volume);
    // testrun_test(check_cb_event_shutdown);

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
