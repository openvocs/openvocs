/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_vocs_events_test.c
        @author         Markus Töpfer

        @date           2024-01-22


        ------------------------------------------------------------------------
*/
#include "ov_vocs_events.c"
#include "ov_vocs_events.h"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_vocs_events_create() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_events *events = ov_vocs_events_create((ov_vocs_events_config){
        .loop = loop,
        .socket.manager = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TCP, .host = "localhost"})});

    testrun(events);
    testrun(ov_vocs_events_cast(events));
    testrun(events->event.engine);
    testrun(events->event.socket);

    testrun(NULL == ov_vocs_events_free(events));

    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_events_free() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_events *events = ov_vocs_events_create((ov_vocs_events_config){
        .loop = loop,
        .socket.manager = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TCP, .host = "localhost"})});

    testrun(events);

    testrun(NULL == ov_vocs_events_free(events));

    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_register() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_events *events = ov_vocs_events_create((ov_vocs_events_config){
        .loop = loop,
        .socket.manager = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TCP, .host = "localhost"})});

    testrun(events);

    int socket = ov_socket_create(events->config.socket.manager, true, NULL);
    testrun(ov_socket_ensure_nonblocking(socket));

    ov_json_value *out = ov_event_api_message_create(OV_KEY_REGISTER, NULL, 1);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_object_set(par, OV_KEY_UUID, ov_json_string("1-2-3-4"));
    ov_json_object_set(par, OV_KEY_TYPE, ov_json_string("test"));

    char *str = ov_json_value_to_string(out);
    out = ov_json_value_free(out);

    ssize_t bytes = -1;
    errno = EAGAIN;

    while ((-1 == bytes) && (errno == EAGAIN)) {

        bytes = send(socket, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    str = ov_data_pointer_free(str);

    char buf[1000] = {0};

    bytes = -1;
    errno = EAGAIN;

    while ((-1 == bytes) && (errno == EAGAIN)) {

        loop->run(loop, OV_RUN_ONCE);
        bytes = recv(socket, buf, 1000, 0);
    }

    out = ov_json_value_from_string(buf, bytes);
    testrun(out);
    testrun(ov_event_api_event_is(out, OV_KEY_REGISTER));
    testrun(0 == ov_event_api_get_error_code(out));
    out = ov_json_value_free(out);

    testrun(1 == ov_dict_count(events->connections));

    testrun(NULL == ov_vocs_events_free(events));

    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int register_connection(ov_vocs_events *events) {

    int conn = ov_socket_create(events->config.socket.manager, true, NULL);
    testrun(ov_socket_ensure_nonblocking(conn));
    ov_event_loop *loop = events->config.loop;

    ov_json_value *out = ov_event_api_message_create(OV_KEY_REGISTER, NULL, 1);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_object_set(par, OV_KEY_UUID, ov_json_string("1-2-3-4"));
    ov_json_object_set(par, OV_KEY_TYPE, ov_json_string("test"));

    char *str = ov_json_value_to_string(out);
    out = ov_json_value_free(out);

    ssize_t bytes = -1;
    errno = EAGAIN;

    while ((-1 == bytes) && (errno == EAGAIN)) {

        bytes = send(conn, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    str = ov_data_pointer_free(str);
    char buf[1000] = {0};

    bytes = -1;
    errno = EAGAIN;

    while ((-1 == bytes) && (errno == EAGAIN)) {

        loop->run(loop, OV_RUN_ONCE);
        bytes = recv(conn, buf, 1000, 0);
    }

    out = ov_json_value_from_string(buf, bytes);
    testrun(out);
    testrun(ov_event_api_event_is(out, OV_KEY_REGISTER));
    testrun(0 == ov_event_api_get_error_code(out));
    out = ov_json_value_free(out);

    return conn;
}

/*----------------------------------------------------------------------------*/

static bool read_conn_with_session(int conn,
                                   ov_event_loop *loop,
                                   const char *event) {

    char buf[1000] = {0};

    ov_json_value *out = NULL;
    bool result = false;

    ssize_t bytes = -1;
    errno = EAGAIN;

    while ((-1 == bytes) && (errno == EAGAIN)) {

        loop->run(loop, OV_RUN_ONCE);
        bytes = recv(conn, buf, 1000, 0);
    }

    out = ov_json_value_from_string(buf, bytes);
    if (!out) goto error;

    if (!ov_event_api_event_is(out, event)) goto error;
    if (!ov_json_string_get(
            ov_json_get(out, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION)))
        goto error;

    result = true;
error:
    ov_json_value_free(out);
    return result;
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_events_emit_media_ready() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_events *events = ov_vocs_events_create((ov_vocs_events_config){
        .loop = loop,
        .socket.manager = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TCP, .host = "localhost"})});

    testrun(events);

    int conn1 = register_connection(events);
    int conn2 = register_connection(events);
    int conn3 = register_connection(events);

    testrun(3 == ov_dict_count(events->connections));

    ov_vocs_events_emit_media_ready(events, "session_id");

    testrun(read_conn_with_session(conn2, loop, OV_KEY_MEDIA_READY));
    testrun(read_conn_with_session(conn1, loop, OV_KEY_MEDIA_READY));
    testrun(read_conn_with_session(conn3, loop, OV_KEY_MEDIA_READY));

    testrun(NULL == ov_vocs_events_free(events));

    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_events_emit_mixer_aquired() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_events *events = ov_vocs_events_create((ov_vocs_events_config){
        .loop = loop,
        .socket.manager = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TCP, .host = "localhost"})});

    testrun(events);

    int conn1 = register_connection(events);
    int conn2 = register_connection(events);
    int conn3 = register_connection(events);

    testrun(3 == ov_dict_count(events->connections));

    ov_vocs_events_emit_mixer_aquired(events, "session_id");

    testrun(read_conn_with_session(conn2, loop, OV_KEY_MIXER_AQUIRED));
    testrun(read_conn_with_session(conn1, loop, OV_KEY_MIXER_AQUIRED));
    testrun(read_conn_with_session(conn3, loop, OV_KEY_MIXER_AQUIRED));

    testrun(NULL == ov_vocs_events_free(events));

    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_events_emit_mixer_lost() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_events *events = ov_vocs_events_create((ov_vocs_events_config){
        .loop = loop,
        .socket.manager = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TCP, .host = "localhost"})});

    testrun(events);

    int conn1 = register_connection(events);
    int conn2 = register_connection(events);
    int conn3 = register_connection(events);

    testrun(3 == ov_dict_count(events->connections));

    ov_vocs_events_emit_mixer_lost(events, "session_id");

    testrun(read_conn_with_session(conn2, loop, OV_KEY_MIXER_LOST));
    testrun(read_conn_with_session(conn1, loop, OV_KEY_MIXER_LOST));
    testrun(read_conn_with_session(conn3, loop, OV_KEY_MIXER_LOST));

    testrun(NULL == ov_vocs_events_free(events));

    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_events_emit_mixer_released() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_events *events = ov_vocs_events_create((ov_vocs_events_config){
        .loop = loop,
        .socket.manager = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TCP, .host = "localhost"})});

    testrun(events);

    int conn1 = register_connection(events);
    int conn2 = register_connection(events);
    int conn3 = register_connection(events);

    testrun(3 == ov_dict_count(events->connections));

    ov_vocs_events_emit_mixer_released(events, "session_id");

    testrun(read_conn_with_session(conn2, loop, OV_KEY_MIXER_RELEASED));
    testrun(read_conn_with_session(conn1, loop, OV_KEY_MIXER_RELEASED));
    testrun(read_conn_with_session(conn3, loop, OV_KEY_MIXER_RELEASED));

    testrun(NULL == ov_vocs_events_free(events));

    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool read_conn_login(int conn,
                            ov_event_loop *loop,
                            const char *client_id,
                            const char *user_id) {

    char buf[1000] = {0};

    ov_json_value *out = NULL;
    bool result = false;

    ssize_t bytes = -1;
    errno = EAGAIN;

    while ((-1 == bytes) && (errno == EAGAIN)) {

        loop->run(loop, OV_RUN_ONCE);
        bytes = recv(conn, buf, 1000, 0);
    }

    out = ov_json_value_from_string(buf, bytes);
    if (!out) goto error;

    if (!ov_event_api_event_is(out, OV_KEY_CLIENT_LOGIN)) goto error;

    if (0 != strcmp(client_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_CLIENT))))
        goto error;

    if (0 != strcmp(user_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_USER))))
        goto error;

    result = true;
error:
    ov_json_value_free(out);
    return result;
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_events_emit_client_login() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_events *events = ov_vocs_events_create((ov_vocs_events_config){
        .loop = loop,
        .socket.manager = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TCP, .host = "localhost"})});

    testrun(events);

    int conn1 = register_connection(events);
    int conn2 = register_connection(events);
    int conn3 = register_connection(events);

    testrun(3 == ov_dict_count(events->connections));

    const char *client_id = "1234";
    const char *user_id = "user";

    ov_vocs_events_emit_client_login(events, client_id, user_id);

    testrun(read_conn_login(conn2, loop, client_id, user_id));
    testrun(read_conn_login(conn1, loop, client_id, user_id));
    testrun(read_conn_login(conn3, loop, client_id, user_id));

    testrun(NULL == ov_vocs_events_free(events));

    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool read_conn_auth(int conn,
                           ov_event_loop *loop,
                           const char *sess_id,
                           const char *user_id,
                           const char *role_id) {

    char buf[1000] = {0};

    ov_json_value *out = NULL;
    bool result = false;

    ssize_t bytes = -1;
    errno = EAGAIN;

    while ((-1 == bytes) && (errno == EAGAIN)) {

        loop->run(loop, OV_RUN_ONCE);
        bytes = recv(conn, buf, 1000, 0);
    }

    out = ov_json_value_from_string(buf, bytes);
    if (!out) goto error;

    if (!ov_event_api_event_is(out, OV_KEY_AUTHORIZE)) goto error;

    if (0 != strcmp(sess_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION))))
        goto error;

    if (0 != strcmp(user_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_USER))))
        goto error;

    if (0 != strcmp(role_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_ROLE))))
        goto error;

    result = true;
error:
    ov_json_value_free(out);
    return result;
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_events_emit_client_authorize() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_events *events = ov_vocs_events_create((ov_vocs_events_config){
        .loop = loop,
        .socket.manager = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TCP, .host = "localhost"})});

    testrun(events);

    int conn1 = register_connection(events);
    int conn2 = register_connection(events);
    int conn3 = register_connection(events);

    testrun(3 == ov_dict_count(events->connections));

    const char *sess_id = "1234";
    const char *user_id = "user";
    const char *role_id = "role";

    ov_vocs_events_emit_client_authorize(events, sess_id, user_id, role_id);

    testrun(read_conn_auth(conn2, loop, sess_id, user_id, role_id));
    testrun(read_conn_auth(conn1, loop, sess_id, user_id, role_id));
    testrun(read_conn_auth(conn3, loop, sess_id, user_id, role_id));

    testrun(NULL == ov_vocs_events_free(events));

    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_events_emit_client_logout() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_events *events = ov_vocs_events_create((ov_vocs_events_config){
        .loop = loop,
        .socket.manager = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TCP, .host = "localhost"})});

    testrun(events);

    int conn1 = register_connection(events);
    int conn2 = register_connection(events);
    int conn3 = register_connection(events);

    testrun(3 == ov_dict_count(events->connections));

    ov_vocs_events_emit_client_logout(events, "session_id");

    testrun(read_conn_with_session(conn2, loop, OV_KEY_LOGOUT));
    testrun(read_conn_with_session(conn1, loop, OV_KEY_LOGOUT));
    testrun(read_conn_with_session(conn3, loop, OV_KEY_LOGOUT));

    testrun(NULL == ov_vocs_events_free(events));

    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool read_conn_user_session(int conn,
                                   ov_event_loop *loop,
                                   const char *event,
                                   const char *sess_id,
                                   const char *client_id,
                                   const char *user_id) {

    char buf[1000] = {0};

    ov_json_value *out = NULL;
    bool result = false;

    ssize_t bytes = -1;
    errno = EAGAIN;

    while ((-1 == bytes) && (errno == EAGAIN)) {

        loop->run(loop, OV_RUN_ONCE);
        bytes = recv(conn, buf, 1000, 0);
    }

    out = ov_json_value_from_string(buf, bytes);
    if (!out) goto error;

    if (!ov_event_api_event_is(out, event)) goto error;

    if (0 != strcmp(sess_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION))))
        goto error;

    if (0 != strcmp(user_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_USER))))
        goto error;

    if (0 != strcmp(client_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_CLIENT))))
        goto error;

    result = true;
error:
    ov_json_value_free(out);
    return result;
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_events_emit_user_session_created() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_events *events = ov_vocs_events_create((ov_vocs_events_config){
        .loop = loop,
        .socket.manager = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TCP, .host = "localhost"})});

    testrun(events);

    int conn1 = register_connection(events);
    int conn2 = register_connection(events);
    int conn3 = register_connection(events);

    testrun(3 == ov_dict_count(events->connections));

    const char *sess_id = "1234";
    const char *user_id = "user";
    const char *client_id = "client";

    ov_vocs_events_emit_user_session_created(
        events, sess_id, client_id, user_id);

    testrun(read_conn_user_session(
        conn2, loop, OV_KEY_SESSION_CREATED, sess_id, client_id, user_id));
    testrun(read_conn_user_session(
        conn1, loop, OV_KEY_SESSION_CREATED, sess_id, client_id, user_id));
    testrun(read_conn_user_session(
        conn3, loop, OV_KEY_SESSION_CREATED, sess_id, client_id, user_id));

    testrun(NULL == ov_vocs_events_free(events));

    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_events_emit_user_session_closed() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_events *events = ov_vocs_events_create((ov_vocs_events_config){
        .loop = loop,
        .socket.manager = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TCP, .host = "localhost"})});

    testrun(events);

    int conn1 = register_connection(events);
    int conn2 = register_connection(events);
    int conn3 = register_connection(events);

    testrun(3 == ov_dict_count(events->connections));

    const char *sess_id = "1234";
    const char *user_id = "user";
    const char *client_id = "client";

    ov_vocs_events_emit_user_session_closed(
        events, sess_id, client_id, user_id);

    testrun(read_conn_user_session(
        conn2, loop, OV_KEY_SESSION_CLOSED, sess_id, client_id, user_id));
    testrun(read_conn_user_session(
        conn1, loop, OV_KEY_SESSION_CLOSED, sess_id, client_id, user_id));
    testrun(read_conn_user_session(
        conn3, loop, OV_KEY_SESSION_CLOSED, sess_id, client_id, user_id));

    testrun(NULL == ov_vocs_events_free(events));

    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool read_participation(int conn,
                               ov_event_loop *loop,
                               const char *sess_id,
                               const char *client_id,
                               const char *user_id,
                               const char *role_id,
                               const char *loop_id,
                               const char *state) {

    char buf[1000] = {0};

    ov_json_value *out = NULL;
    bool result = false;

    ssize_t bytes = -1;
    errno = EAGAIN;

    while ((-1 == bytes) && (errno == EAGAIN)) {

        loop->run(loop, OV_RUN_ONCE);
        bytes = recv(conn, buf, 1000, 0);
    }

    out = ov_json_value_from_string(buf, bytes);
    if (!out) goto error;

    if (!ov_event_api_event_is(out, OV_KEY_STATE)) goto error;

    if (0 != strcmp(sess_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION))))
        goto error;

    if (0 != strcmp(user_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_USER))))
        goto error;

    if (0 != strcmp(client_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_CLIENT))))
        goto error;

    if (0 != strcmp(role_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_ROLE))))
        goto error;

    if (0 != strcmp(loop_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP))))
        goto error;

    if (0 != strcmp(state,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE))))
        goto error;

    result = true;
error:
    ov_json_value_free(out);
    return result;
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_events_emit_participation_state() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_events *events = ov_vocs_events_create((ov_vocs_events_config){
        .loop = loop,
        .socket.manager = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TCP, .host = "localhost"})});

    testrun(events);

    int conn1 = register_connection(events);
    int conn2 = register_connection(events);
    int conn3 = register_connection(events);

    testrun(3 == ov_dict_count(events->connections));

    const char *sess_id = "1234";
    const char *user_id = "user";
    const char *role_id = "role";
    const char *loop_id = "loop";
    const char *client_id = "client";
    ov_socket_configuration mcast = (ov_socket_configuration){
        .host = "224.0.0.1", .port = 12345, .type = UDP};

    ov_vocs_events_emit_participation_state(events,
                                            sess_id,
                                            client_id,
                                            user_id,
                                            role_id,
                                            loop_id,
                                            mcast,
                                            OV_PARTICIPATION_STATE_RECV);

    char const *state =
        ov_participation_state_to_string(OV_PARTICIPATION_STATE_RECV);

    testrun(read_participation(
        conn2, loop, sess_id, client_id, user_id, role_id, loop_id, state));
    testrun(read_participation(
        conn1, loop, sess_id, client_id, user_id, role_id, loop_id, state));
    testrun(read_participation(
        conn3, loop, sess_id, client_id, user_id, role_id, loop_id, state));

    testrun(NULL == ov_vocs_events_free(events));

    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool read_volume(int conn,
                        ov_event_loop *loop,
                        const char *sess_id,
                        const char *client_id,
                        const char *user_id,
                        const char *role_id,
                        const char *loop_id,
                        uint8_t volume) {

    char buf[1000] = {0};

    ov_json_value *out = NULL;
    bool result = false;

    ssize_t bytes = -1;
    errno = EAGAIN;

    while ((-1 == bytes) && (errno == EAGAIN)) {

        loop->run(loop, OV_RUN_ONCE);
        bytes = recv(conn, buf, 1000, 0);
    }

    out = ov_json_value_from_string(buf, bytes);
    if (!out) goto error;

    if (!ov_event_api_event_is(out, OV_KEY_VOLUME)) goto error;

    if (0 != strcmp(sess_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION))))
        goto error;

    if (0 != strcmp(user_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_USER))))
        goto error;

    if (0 != strcmp(client_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_CLIENT))))
        goto error;

    if (0 != strcmp(role_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_ROLE))))
        goto error;

    if (0 != strcmp(loop_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP))))
        goto error;

    if (volume != ov_json_number_get(
                      ov_json_get(out, "/" OV_KEY_PARAMETER "/" OV_KEY_VOLUME)))
        goto error;

    result = true;
error:
    ov_json_value_free(out);
    return result;
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_events_emit_loop_volume() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_events *events = ov_vocs_events_create((ov_vocs_events_config){
        .loop = loop,
        .socket.manager = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TCP, .host = "localhost"})});

    testrun(events);

    int conn1 = register_connection(events);
    int conn2 = register_connection(events);
    int conn3 = register_connection(events);

    testrun(3 == ov_dict_count(events->connections));

    const char *sess_id = "1234";
    const char *user_id = "user";
    const char *role_id = "role";
    const char *loop_id = "loop";
    uint8_t volume = 10;
    const char *client_id = "client";
    ov_socket_configuration mcast = (ov_socket_configuration){
        .host = "224.0.0.1", .port = 12345, .type = UDP};

    ov_vocs_events_emit_loop_volume(
        events, sess_id, client_id, user_id, role_id, loop_id, mcast, volume);

    testrun(read_volume(
        conn2, loop, sess_id, client_id, user_id, role_id, loop_id, volume));
    testrun(read_volume(
        conn1, loop, sess_id, client_id, user_id, role_id, loop_id, volume));
    testrun(read_volume(
        conn3, loop, sess_id, client_id, user_id, role_id, loop_id, volume));

    testrun(NULL == ov_vocs_events_free(events));

    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool read_ptt(int conn,
                     ov_event_loop *loop,
                     const char *sess_id,
                     const char *client_id,
                     const char *user_id,
                     const char *role_id,
                     const char *loop_id) {

    char buf[1000] = {0};

    ov_json_value *out = NULL;
    bool result = false;

    ssize_t bytes = -1;
    errno = EAGAIN;

    while ((-1 == bytes) && (errno == EAGAIN)) {

        loop->run(loop, OV_RUN_ONCE);
        bytes = recv(conn, buf, 1000, 0);
    }

    out = ov_json_value_from_string(buf, bytes);
    if (!out) goto error;

    if (!ov_event_api_event_is(out, OV_KEY_PTT)) goto error;

    if (0 != strcmp(sess_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION))))
        goto error;

    if (0 != strcmp(user_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_USER))))
        goto error;

    if (0 != strcmp(client_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_CLIENT))))
        goto error;

    if (0 != strcmp(role_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_ROLE))))
        goto error;

    if (0 != strcmp(loop_id,
                    ov_json_string_get(ov_json_get(
                        out, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP))))
        goto error;

    if (!ov_json_get(out, "/" OV_KEY_PARAMETER "/" OV_KEY_PTT)) goto error;

    result = true;
error:
    ov_json_value_free(out);
    return result;
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_events_emit_ptt() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_events *events = ov_vocs_events_create((ov_vocs_events_config){
        .loop = loop,
        .socket.manager = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TCP, .host = "localhost"})});

    testrun(events);

    int conn1 = register_connection(events);
    int conn2 = register_connection(events);
    int conn3 = register_connection(events);

    testrun(3 == ov_dict_count(events->connections));

    const char *sess_id = "1234";
    const char *user_id = "user";
    const char *role_id = "role";
    const char *loop_id = "loop";
    const char *client_id = "client";
    ov_socket_configuration mcast = (ov_socket_configuration){
        .host = "224.0.0.1", .port = 12345, .type = UDP};

    ov_vocs_events_emit_ptt(
        events, sess_id, client_id, user_id, role_id, loop_id, mcast, true);

    testrun(
        read_ptt(conn2, loop, sess_id, client_id, user_id, role_id, loop_id));
    testrun(
        read_ptt(conn1, loop, sess_id, client_id, user_id, role_id, loop_id));
    testrun(
        read_ptt(conn3, loop, sess_id, client_id, user_id, role_id, loop_id));

    testrun(NULL == ov_vocs_events_free(events));

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

int all_tests() {

    testrun_init();
    testrun_test(test_ov_vocs_events_create);
    testrun_test(test_ov_vocs_events_free);

    testrun_test(check_register);

    testrun_test(test_ov_vocs_events_emit_media_ready);
    testrun_test(test_ov_vocs_events_emit_mixer_aquired);
    testrun_test(test_ov_vocs_events_emit_mixer_lost);
    testrun_test(test_ov_vocs_events_emit_mixer_released);

    testrun_test(test_ov_vocs_events_emit_client_login);
    testrun_test(test_ov_vocs_events_emit_client_authorize);
    testrun_test(test_ov_vocs_events_emit_client_logout);

    testrun_test(test_ov_vocs_events_emit_user_session_created);
    testrun_test(test_ov_vocs_events_emit_user_session_closed);

    testrun_test(test_ov_vocs_events_emit_participation_state);
    testrun_test(test_ov_vocs_events_emit_loop_volume);
    testrun_test(test_ov_vocs_events_emit_ptt);

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
