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
        @file           ov_mc_backend_sip_test.c
        @author         Markus

        @date           2023-03-24


        ------------------------------------------------------------------------
*/
#include "ov_mc_backend_sip.c"
#include <ov_test/testrun.h>

#include "../include/ov_mc_mixer_msg.h"

struct userdata {

    char *uuid;
    char *user;
    char *loop;
    char *peer;
    uint64_t error_code;
    char *error_desc;

    ov_sip_permission permission;
};

/*----------------------------------------------------------------------------*/

static bool dummy_userdata_clear(struct userdata *u) {

    if (!u)
        return false;

    u->uuid = ov_data_pointer_free(u->uuid);
    u->user = ov_data_pointer_free(u->user);
    u->loop = ov_data_pointer_free(u->loop);
    u->peer = ov_data_pointer_free(u->peer);
    u->error_desc = ov_data_pointer_free(u->error_desc);
    u->error_code = 0;
    u->permission.caller = ov_data_pointer_free((char *)u->permission.caller);
    u->permission.loop = ov_data_pointer_free((char *)u->permission.loop);
    u->permission.from_epoch = 0;
    u->permission.until_epoch = 0;

    return true;
}

/*----------------------------------------------------------------------------*/

static void dummy_mixer_acquired(void *userdata, const char *uuid,
                                 const char *user_uuid, uint64_t error_code,
                                 const char *error_desc) {

    struct userdata *data = (struct userdata *)userdata;
    dummy_userdata_clear(data);

    data->uuid = ov_string_dup(uuid);
    data->user = ov_string_dup(user_uuid);
    data->error_code = error_code;
    data->error_desc = ov_string_dup(error_desc);
    return;
}

/*----------------------------------------------------------------------------*/

static void new_call(void *userdata, const char *loop, const char *id,
                     const char *peer) {

    struct userdata *data = (struct userdata *)userdata;
    dummy_userdata_clear(data);

    data->uuid = ov_string_dup(id);
    data->peer = ov_string_dup(peer);
    data->loop = ov_string_dup(loop);
    return;
}

/*----------------------------------------------------------------------------*/

static void terminated_call(void *userdata, const char *id,
                            const char *loopname) {

    struct userdata *data = (struct userdata *)userdata;
    dummy_userdata_clear(data);

    data->uuid = ov_string_dup(id);
    data->loop = ov_string_dup(loopname);
    return;
}

/*----------------------------------------------------------------------------*/

static void permit_call(void *userdata, const ov_sip_permission permission,
                        uint64_t error_code, const char *error_desc) {

    struct userdata *data = (struct userdata *)userdata;
    dummy_userdata_clear(data);

    data->error_code = error_code;
    data->error_desc = ov_string_dup(error_desc);
    data->permission.caller = ov_string_dup(permission.caller);
    data->permission.loop = ov_string_dup(permission.loop);
    data->permission.from_epoch = permission.from_epoch;
    data->permission.until_epoch = permission.until_epoch;

    return;
}

/*----------------------------------------------------------------------------*/

static void revoke_call(void *userdata, const ov_sip_permission permission,
                        uint64_t error_code, const char *error_desc) {

    struct userdata *data = (struct userdata *)userdata;
    dummy_userdata_clear(data);

    data->error_code = error_code;
    data->error_desc = ov_string_dup(error_desc);
    data->permission.caller = ov_string_dup(permission.caller);
    data->permission.loop = ov_string_dup(permission.loop);
    data->permission.from_epoch = permission.from_epoch;
    data->permission.until_epoch = permission.until_epoch;
    return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_mc_backend_sip_create() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig = (ov_mc_backend_sip_config){
        .loop = loop, .db = db, .backend = backend, .socket.manager = manager};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);
    testrun(ov_mc_backend_sip_cast(sip));
    testrun(sip->app);
    testrun(sip->async);
    testrun(-1 != sip->socket.manager);

    testrun(NULL == ov_mc_backend_sip_free(sip));

    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_sip_free() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig = (ov_mc_backend_sip_config){
        .loop = loop, .db = db, .backend = backend, .socket.manager = manager};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);
    testrun(ov_mc_backend_sip_cast(sip));
    testrun(sip->app);
    testrun(sip->async);
    testrun(-1 != sip->socket.manager);

    testrun(NULL == ov_mc_backend_sip_free(sip));

    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_sip_cast() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig = (ov_mc_backend_sip_config){
        .loop = loop, .db = db, .backend = backend, .socket.manager = manager};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);
    testrun(ov_mc_backend_sip_cast(sip));
    testrun(sip->magic_bytes == OV_MC_BACKEND_SIP_MAGIC_BYTES);
    sip->magic_bytes = OV_MC_BACKEND_SIP_MAGIC_BYTES + 1;
    testrun(!ov_mc_backend_sip_cast(sip));
    sip->magic_bytes = OV_MC_BACKEND_SIP_MAGIC_BYTES;

    testrun(NULL == ov_mc_backend_sip_free(sip));

    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_register() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig = (ov_mc_backend_sip_config){
        .loop = loop, .db = db, .backend = backend, .socket.manager = manager};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);

    int client = ov_socket_create(manager, true, false);
    testrun(-1 != client);
    testrun(ov_socket_ensure_nonblocking(client));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_mc_sip_msg_register("1-2-3-4");
    testrun(msg);
    char *str = ov_json_value_to_string(msg);

    ssize_t bytes = -1;

    while (-1 == bytes) {

        bytes = send(client, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    loop->run(loop, OV_RUN_ONCE);
    str = ov_data_pointer_free(str);

    int connection = ov_mc_backend_sip_registry_get_proxy_socket(sip->registry);
    testrun(connection > 0);

    char buf[2048] = {0};
    bytes = -1;

    for (int i = 0; i < 10; i++) {

        bytes = recv(client, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    testrun(-1 == bytes);

    // call manually, will free msg
    cb_event_register(sip, "name", 15, msg);
    // nothing happend

    testrun(NULL == ov_mc_backend_sip_free(sip));

    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_mixer_released() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig = (ov_mc_backend_sip_config){
        .loop = loop, .db = db, .backend = backend, .socket.manager = manager};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);

    int client = ov_socket_create(manager, true, false);
    testrun(-1 != client);
    testrun(ov_socket_ensure_nonblocking(client));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_mc_sip_msg_register("1-2-3-4");
    testrun(msg);
    char *str = ov_json_value_to_string(msg);

    ssize_t bytes = -1;

    while (-1 == bytes) {

        bytes = send(client, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    loop->run(loop, OV_RUN_ONCE);

    str = ov_data_pointer_free(str);
    msg = ov_json_value_free(msg);

    int connection = ov_mc_backend_sip_registry_get_proxy_socket(sip->registry);
    testrun(connection > 0);

    msg = ov_mc_sip_msg_release("1-2-3-4", "user");
    testrun(msg);

    testrun(ov_event_async_set(
        sip->async, "1-2-3-4",
        (ov_event_async_data){.socket = connection,
                              .value = msg,
                              .timedout.userdata = sip,
                              .timedout.callback = cb_async_timedout},
        sip->config.timeout.response_usec));

    cb_mixer_released(sip, "1-2-3-4", "user", 0, NULL);

    // check client get success response

    bytes = -1;
    char buf[2048] = {0};

    while (-1 == bytes) {

        bytes = recv(client, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_RELEASE));
    testrun(0 == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    msg = ov_mc_sip_msg_release("1-2-3-4", "user");
    testrun(msg);

    testrun(ov_event_async_set(
        sip->async, "1-2-3-4",
        (ov_event_async_data){.socket = connection,
                              .value = msg,
                              .timedout.userdata = sip,
                              .timedout.callback = cb_async_timedout},
        sip->config.timeout.response_usec));

    cb_mixer_released(sip, "1-2-3-4", "user", 100, "some error");

    // check client get error response

    memset(buf, 0, 2048);
    bytes = -1;

    while (-1 == bytes) {

        bytes = recv(client, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_RELEASE));
    testrun(100 == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    testrun(NULL == ov_mc_backend_sip_free(sip));

    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_mixer_acquired() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    int mixer = ov_socket_create(manager, true, false);
    testrun(-1 != mixer);
    testrun(ov_socket_ensure_nonblocking(mixer));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_mc_mixer_msg_register("1-2-3-4", "audio");
    char *str = ov_json_value_to_string(msg);
    msg = ov_json_value_free(msg);

    ssize_t bytes = -1;
    while (-1 == bytes) {

        bytes = send(mixer, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    str = ov_data_pointer_free(str);

    // expect configure response

    char buf[2048] = {0};

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(mixer, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    testrun(ov_mc_backend_acquire_mixer(
        backend, "uuid", "user",
        (ov_mc_mixer_core_forward){.socket.host = "127.0.0.1",
                                   .socket.port = 12345,
                                   .socket.type = UDP,
                                   .ssrc = 12345},
        &userdata, dummy_mixer_acquired));

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(mixer, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    msg = ov_json_value_from_string(buf, bytes);
    ov_json_value *out = ov_event_api_create_success_response(msg);
    str = ov_json_value_to_string(out);

    bytes = -1;
    while (-1 == bytes) {

        bytes = send(mixer, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    while (NULL == userdata.uuid) {
        loop->run(loop, OV_RUN_ONCE);
    }

    // expect some registered mixer for user "user"
    testrun(0 == strcmp(userdata.user, "user"));
    dummy_userdata_clear(&userdata);

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig = (ov_mc_backend_sip_config){
        .loop = loop, .db = db, .backend = backend, .socket.manager = manager};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);

    int client = ov_socket_create(manager, true, false);
    testrun(-1 != client);
    testrun(ov_socket_ensure_nonblocking(client));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    msg = ov_mc_sip_msg_register("1-2-3-4");
    testrun(msg);
    str = ov_json_value_to_string(msg);

    bytes = -1;

    while (-1 == bytes) {

        bytes = send(client, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    loop->run(loop, OV_RUN_ONCE);

    str = ov_data_pointer_free(str);
    msg = ov_json_value_free(msg);

    int connection = ov_mc_backend_sip_registry_get_proxy_socket(sip->registry);
    testrun(connection > 0);

    // no async set, check we get a release response

    msg = ov_mc_sip_msg_acquire("1-2-3-4", "user");

    cb_mixer_aquired(sip, "1-2-3-4", "user", 0, NULL);

    bytes = -1;

    while (-1 == bytes) {

        bytes = recv(mixer, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);
    msg = ov_json_value_from_string(buf, bytes);
    testrun(ov_event_api_event_is(msg, OV_KEY_RELEASE));
    testrun(0 == strcmp("user",
                        ov_json_string_get(ov_json_get(msg, "/" OV_KEY_PARAMETER
                                                            "/" OV_KEY_NAME))));
    msg = ov_json_value_free(msg);

    msg = ov_mc_sip_msg_acquire("1-2-3-4", "user");
    testrun(msg);

    testrun(ov_event_async_set(
        sip->async, "1-2-3-4",
        (ov_event_async_data){.socket = connection,
                              .value = msg,
                              .timedout.userdata = sip,
                              .timedout.callback = cb_async_timedout},
        sip->config.timeout.response_usec));

    cb_mixer_aquired(sip, "1-2-3-4", "user", 0, NULL);

    // check client get success response

    memset(buf, 0, 2048);
    bytes = -1;

    while (-1 == bytes) {

        bytes = recv(client, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_ACQUIRE));
    testrun(0 == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    msg = ov_mc_sip_msg_acquire("1-2-3-4", "user");
    testrun(msg);

    testrun(ov_event_async_set(
        sip->async, "1-2-3-4",
        (ov_event_async_data){.socket = connection,
                              .value = msg,
                              .timedout.userdata = sip,
                              .timedout.callback = cb_async_timedout},
        sip->config.timeout.response_usec));

    cb_mixer_aquired(sip, "1-2-3-4", "user", 100, "some error");

    memset(buf, 0, 2048);
    bytes = -1;

    while (-1 == bytes) {

        bytes = recv(client, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    msg = ov_json_value_from_string(buf, bytes);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_ACQUIRE));
    testrun(100 == ov_event_api_get_error_code(msg));
    msg = ov_json_value_free(msg);

    testrun(NULL == ov_mc_backend_sip_free(sip));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_acquire() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    int mixer = ov_socket_create(manager, true, false);
    testrun(-1 != mixer);
    testrun(ov_socket_ensure_nonblocking(mixer));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_mc_mixer_msg_register("1-2-3-4", "audio");
    char *str = ov_json_value_to_string(msg);
    msg = ov_json_value_free(msg);

    ssize_t bytes = -1;
    while (-1 == bytes) {

        bytes = send(mixer, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    str = ov_data_pointer_free(str);

    // expect configure response

    char buf[2048] = {0};

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(mixer, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig = (ov_mc_backend_sip_config){
        .loop = loop, .db = db, .backend = backend, .socket.manager = manager};

    testrun(ov_mc_backend_acquire_mixer(
        backend, "uuid", "user",
        (ov_mc_mixer_core_forward){.socket.host = "127.0.0.1",
                                   .socket.port = 12345,
                                   .socket.type = UDP,
                                   .ssrc = 12345},
        &userdata, dummy_mixer_acquired));

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(mixer, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    msg = ov_json_value_from_string(buf, bytes);
    ov_json_value *out = ov_event_api_create_success_response(msg);
    str = ov_json_value_to_string(out);

    bytes = -1;
    while (-1 == bytes) {

        bytes = send(mixer, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    while (NULL == userdata.uuid) {
        loop->run(loop, OV_RUN_ONCE);
    }

    // expect some registered mixer for user "user"
    testrun(0 == strcmp(userdata.user, "user"));
    dummy_userdata_clear(&userdata);

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);

    int client = ov_socket_create(manager, true, false);
    testrun(-1 != client);
    testrun(ov_socket_ensure_nonblocking(client));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    msg = ov_mc_sip_msg_register("1-2-3-4");
    testrun(msg);
    str = ov_json_value_to_string(msg);

    bytes = -1;

    while (-1 == bytes) {

        bytes = send(client, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    loop->run(loop, OV_RUN_ONCE);

    str = ov_data_pointer_free(str);
    msg = ov_json_value_free(msg);

    int connection = ov_mc_backend_sip_registry_get_proxy_socket(sip->registry);
    testrun(connection > 0);

    msg = ov_mc_sip_msg_release("1-2-3-4", "user");

    cb_event_release(sip, "name", connection, msg);

    // check acquire is send to mixer

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(mixer, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    ov_event_async_data adata = ov_event_async_unset(sip->async, "1-2-3-4");
    testrun(adata.value == msg);
    testrun(adata.socket == connection);
    msg = ov_json_value_free(msg);

    // fprintf(stdout, "%s", buf);
    msg = ov_json_value_from_string(buf, bytes);
    testrun(ov_event_api_event_is(msg, OV_KEY_RELEASE));
    testrun(!ov_event_api_get_response(msg));
    msg = ov_json_value_free(msg);

    testrun(NULL == ov_mc_backend_sip_free(sip));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_release() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    int mixer = ov_socket_create(manager, true, false);
    testrun(-1 != mixer);
    testrun(ov_socket_ensure_nonblocking(mixer));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_mc_mixer_msg_register("1-2-3-4", "audio");
    char *str = ov_json_value_to_string(msg);
    msg = ov_json_value_free(msg);

    ssize_t bytes = -1;
    while (-1 == bytes) {

        bytes = send(mixer, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    str = ov_data_pointer_free(str);

    // expect configure response

    char buf[2048] = {0};

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(mixer, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig = (ov_mc_backend_sip_config){
        .loop = loop, .db = db, .backend = backend, .socket.manager = manager};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);

    int client = ov_socket_create(manager, true, false);
    testrun(-1 != client);
    testrun(ov_socket_ensure_nonblocking(client));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    msg = ov_mc_sip_msg_register("1-2-3-4");
    testrun(msg);
    str = ov_json_value_to_string(msg);

    bytes = -1;

    while (-1 == bytes) {

        bytes = send(client, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    loop->run(loop, OV_RUN_ONCE);

    str = ov_data_pointer_free(str);
    msg = ov_json_value_free(msg);

    int connection = ov_mc_backend_sip_registry_get_proxy_socket(sip->registry);
    testrun(connection > 0);

    msg = ov_mc_sip_msg_acquire("1-2-3-4", "user");

    cb_event_acquire(sip, "name", connection, msg);

    // check acquire is send to mixer

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(mixer, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    ov_event_async_data adata = ov_event_async_unset(sip->async, "1-2-3-4");
    testrun(adata.value == msg);
    testrun(adata.socket == connection);
    msg = ov_json_value_free(msg);

    // fprintf(stdout, "%s", buf);
    msg = ov_json_value_from_string(buf, bytes);
    testrun(ov_event_api_event_is(msg, OV_KEY_ACQUIRE));
    testrun(!ov_event_api_get_response(msg));
    msg = ov_json_value_free(msg);

    testrun(NULL == ov_mc_backend_sip_free(sip));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_mixer_forwarded() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    int mixer = ov_socket_create(manager, true, false);
    testrun(-1 != mixer);
    testrun(ov_socket_ensure_nonblocking(mixer));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_mc_mixer_msg_register("1-2-3-4", "audio");
    char *str = ov_json_value_to_string(msg);
    msg = ov_json_value_free(msg);

    ssize_t bytes = -1;
    while (-1 == bytes) {

        bytes = send(mixer, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    str = ov_data_pointer_free(str);

    // expect configure response

    char buf[2048] = {0};

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(mixer, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    testrun(ov_mc_backend_acquire_mixer(
        backend, "uuid", "user",
        (ov_mc_mixer_core_forward){.socket.host = "127.0.0.1",
                                   .socket.port = 12345,
                                   .socket.type = UDP,
                                   .ssrc = 12345},
        &userdata, dummy_mixer_acquired));

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(mixer, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    msg = ov_json_value_from_string(buf, bytes);
    ov_json_value *out = ov_event_api_create_success_response(msg);
    str = ov_json_value_to_string(out);

    bytes = -1;
    while (-1 == bytes) {

        bytes = send(mixer, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    while (NULL == userdata.uuid) {
        loop->run(loop, OV_RUN_ONCE);
    }

    // expect some registered mixer for user "user"
    testrun(0 == strcmp(userdata.user, "user"));
    dummy_userdata_clear(&userdata);

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig = (ov_mc_backend_sip_config){
        .loop = loop, .db = db, .backend = backend, .socket.manager = manager};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);

    int client = ov_socket_create(manager, true, false);
    testrun(-1 != client);
    testrun(ov_socket_ensure_nonblocking(client));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    msg = ov_mc_sip_msg_register("1-2-3-4");
    testrun(msg);
    str = ov_json_value_to_string(msg);

    bytes = -1;

    while (-1 == bytes) {

        bytes = send(client, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    loop->run(loop, OV_RUN_ONCE);

    str = ov_data_pointer_free(str);
    msg = ov_json_value_free(msg);

    int connection = ov_mc_backend_sip_registry_get_proxy_socket(sip->registry);
    testrun(connection > 0);

    // no async set expect mixer release
    cb_mixer_forwarded(sip, "1-2-3-4", "user", 0, NULL);

    // check release is send to mixer

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(mixer, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);
    msg = ov_json_value_from_string(buf, bytes);
    testrun(ov_event_api_event_is(msg, OV_KEY_RELEASE));
    testrun(!ov_event_api_get_response(msg));
    msg = ov_json_value_free(msg);

    msg = ov_mc_sip_msg_set_sc("1-2-3-4", "user", "loop",
                               (ov_mc_mixer_core_forward){.socket.host = "127."
                                                                         "0.0."
                                                                         "1",
                                                          .socket.port = 12345,
                                                          .socket.type = UDP,
                                                          .ssrc = 12345});
    testrun(msg);

    testrun(ov_event_async_set(
        sip->async, "1-2-3-4",
        (ov_event_async_data){.socket = connection,
                              .value = msg,
                              .timedout.userdata = sip,
                              .timedout.callback = cb_async_timedout},
        sip->config.timeout.response_usec));

    cb_mixer_forwarded(sip, "1-2-3-4", "user", 0, NULL);

    bytes = -1;
    memset(buf, 0, 2048);
    while (-1 == bytes) {

        bytes = recv(client, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);
    msg = ov_json_value_from_string(buf, bytes);
    testrun(ov_event_api_event_is(msg, OV_SIP_EVENT_SET_SINGLECAST));
    testrun(0 == ov_event_api_get_error_code(msg));
    testrun(ov_event_api_get_response(msg));
    msg = ov_json_value_free(msg);

    msg = ov_mc_sip_msg_set_sc("1-2-3-4", "user", "loop",
                               (ov_mc_mixer_core_forward){.socket.host = "127."
                                                                         "0.0."
                                                                         "1",
                                                          .socket.port = 12345,
                                                          .socket.type = UDP,
                                                          .ssrc = 12345});
    testrun(msg);

    testrun(ov_event_async_set(
        sip->async, "1-2-3-4",
        (ov_event_async_data){.socket = connection,
                              .value = msg,
                              .timedout.userdata = sip,
                              .timedout.callback = cb_async_timedout},
        sip->config.timeout.response_usec));

    cb_mixer_forwarded(sip, "1-2-3-4", "user", 100, "some error");

    bytes = -1;
    memset(buf, 0, 2048);
    while (-1 == bytes) {

        bytes = recv(client, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);
    msg = ov_json_value_from_string(buf, bytes);
    testrun(ov_event_api_event_is(msg, OV_SIP_EVENT_SET_SINGLECAST));
    testrun(100 == ov_event_api_get_error_code(msg));
    testrun(ov_event_api_get_response(msg));
    msg = ov_json_value_free(msg);

    testrun(NULL == ov_mc_backend_sip_free(sip));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_set_singlecast() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    int mixer = ov_socket_create(manager, true, false);
    testrun(-1 != mixer);
    testrun(ov_socket_ensure_nonblocking(mixer));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_mc_mixer_msg_register("1-2-3-4", "audio");
    char *str = ov_json_value_to_string(msg);
    msg = ov_json_value_free(msg);

    ssize_t bytes = -1;
    while (-1 == bytes) {

        bytes = send(mixer, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    str = ov_data_pointer_free(str);

    // expect configure response

    char buf[2048] = {0};

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(mixer, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    testrun(ov_mc_backend_acquire_mixer(
        backend, "uuid", "user",
        (ov_mc_mixer_core_forward){.socket.host = "127.0.0.1",
                                   .socket.port = 12345,
                                   .socket.type = UDP,
                                   .ssrc = 12345},
        &userdata, dummy_mixer_acquired));

    bytes = -1;
    while (-1 == bytes) {

        bytes = recv(mixer, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    msg = ov_json_value_from_string(buf, bytes);
    ov_json_value *out = ov_event_api_create_success_response(msg);
    str = ov_json_value_to_string(out);

    bytes = -1;
    while (-1 == bytes) {

        bytes = send(mixer, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    while (NULL == userdata.uuid) {
        loop->run(loop, OV_RUN_ONCE);
    }

    // expect some registered mixer for user "user"
    testrun(0 == strcmp(userdata.user, "user"));
    dummy_userdata_clear(&userdata);

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig = (ov_mc_backend_sip_config){
        .loop = loop, .db = db, .backend = backend, .socket.manager = manager};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);

    int client = ov_socket_create(manager, true, false);
    testrun(-1 != client);
    testrun(ov_socket_ensure_nonblocking(client));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    msg = ov_mc_sip_msg_register("1-2-3-4");
    testrun(msg);
    str = ov_json_value_to_string(msg);

    bytes = -1;

    while (-1 == bytes) {

        bytes = send(client, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    loop->run(loop, OV_RUN_ONCE);

    str = ov_data_pointer_free(str);
    msg = ov_json_value_free(msg);

    int connection = ov_mc_backend_sip_registry_get_proxy_socket(sip->registry);
    testrun(connection > 0);

    msg = ov_mc_sip_msg_set_sc("1-2-3-4", "user", "loop",
                               (ov_mc_mixer_core_forward){.socket.host = "127."
                                                                         "0.0."
                                                                         "1",
                                                          .socket.port = 12345,
                                                          .socket.type = UDP,
                                                          .payload_type = 100,
                                                          .ssrc = 12345});

    testrun(msg);

    ov_mc_mixer_core_forward forward = ov_mc_sip_msg_get_loop_socket(msg);
    testrun(ov_mc_mixer_core_forward_data_is_valid(&forward));

    ov_json_value *par = ov_event_api_get_parameter(msg);
    ov_json_object_del(par, OV_KEY_PAYLOAD_TYPE);

    forward = ov_mc_sip_msg_get_loop_socket(msg);
    testrun(!ov_mc_mixer_core_forward_data_is_valid(&forward));

    testrun(NULL == ov_mc_backend_sip_free(sip));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_notify(const char *type, const char *call_id,
                                 const char *peer, const char *loop) {

    ov_json_value *out = ov_event_api_message_create(OV_KEY_NOTIFY, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_value *val = ov_json_string(type);
    ov_json_object_set(par, OV_KEY_TYPE, val);
    val = ov_json_string(call_id);
    ov_json_object_set(par, OV_KEY_ID, val);

    if (peer) {

        val = ov_json_string(peer);
        ov_json_object_set(par, OV_KEY_PEER, val);
    }

    if (loop) {

        val = ov_json_string(loop);
        ov_json_object_set(par, OV_KEY_LOOP, val);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_notify() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig =
        (ov_mc_backend_sip_config){.loop = loop,
                                   .db = db,
                                   .backend = backend,
                                   .socket.manager = manager,
                                   .callback.userdata = &userdata,
                                   .callback.call.new = new_call,
                                   .callback.call.terminated = terminated_call};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);

    int client = ov_socket_create(manager, true, false);
    testrun(-1 != client);
    testrun(ov_socket_ensure_nonblocking(client));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_mc_sip_msg_register("1-2-3-4");
    testrun(msg);
    char *str = ov_json_value_to_string(msg);

    ssize_t bytes = -1;

    while (-1 == bytes) {

        bytes = send(client, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    loop->run(loop, OV_RUN_ONCE);

    str = ov_data_pointer_free(str);
    msg = ov_json_value_free(msg);

    int connection = ov_mc_backend_sip_registry_get_proxy_socket(sip->registry);
    testrun(connection > 0);

    msg = msg_notify("new_call", "id", "peer", "loop");

    cb_event_notify(sip, "name", connection, msg);

    testrun(connection ==
            ov_mc_backend_sip_registry_get_call_proxy(sip->registry, "id"));
    testrun(0 == strcmp(userdata.uuid, "id"));
    testrun(0 == strcmp(userdata.peer, "peer"));
    testrun(0 == strcmp(userdata.loop, "loop"));
    dummy_userdata_clear(&userdata);

    msg = msg_notify("call_terminated", "id", "peer", "loop");

    cb_event_notify(sip, "name", connection, msg);

    testrun(-1 ==
            ov_mc_backend_sip_registry_get_call_proxy(sip->registry, "id"));
    testrun(0 == strcmp(userdata.uuid, "id"));
    dummy_userdata_clear(&userdata);

    msg = msg_notify("unknown", "id", "peer", "loop");

    cb_event_notify(sip, "name", connection, msg);
    // nothing happens

    testrun(NULL == ov_mc_backend_sip_free(sip));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_permit() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig =
        (ov_mc_backend_sip_config){.loop = loop,
                                   .db = db,
                                   .backend = backend,
                                   .socket.manager = manager,
                                   .callback.userdata = &userdata,
                                   .callback.call.new = new_call,
                                   .callback.call.terminated = terminated_call,
                                   .callback.call.permit = permit_call,
                                   .callback.call.revoke = revoke_call};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);

    int client = ov_socket_create(manager, true, false);
    testrun(-1 != client);
    testrun(ov_socket_ensure_nonblocking(client));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_mc_sip_msg_register("1-2-3-4");
    testrun(msg);
    char *str = ov_json_value_to_string(msg);

    ssize_t bytes = -1;

    while (-1 == bytes) {

        bytes = send(client, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    loop->run(loop, OV_RUN_ONCE);

    str = ov_data_pointer_free(str);
    msg = ov_json_value_free(msg);

    int connection = ov_mc_backend_sip_registry_get_proxy_socket(sip->registry);
    testrun(connection > 0);

    msg = ov_mc_sip_msg_permit((ov_sip_permission){
        .caller = "caller", .loop = "loop", .from_epoch = 1, .until_epoch = 2});

    ov_json_value *res = ov_event_api_create_success_response(msg);
    msg = ov_json_value_free(msg);

    cb_event_permit_response(sip, connection, res);

    testrun(0 == strcmp(userdata.permission.caller, "caller"));
    testrun(0 == strcmp(userdata.permission.loop, "loop"));
    dummy_userdata_clear(&userdata);

    testrun(NULL == ov_mc_backend_sip_free(sip));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_cb_event_revoke() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig =
        (ov_mc_backend_sip_config){.loop = loop,
                                   .db = db,
                                   .backend = backend,
                                   .socket.manager = manager,
                                   .callback.userdata = &userdata,
                                   .callback.call.new = new_call,
                                   .callback.call.terminated = terminated_call,
                                   .callback.call.permit = permit_call,
                                   .callback.call.revoke = revoke_call};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);

    int client = ov_socket_create(manager, true, false);
    testrun(-1 != client);
    testrun(ov_socket_ensure_nonblocking(client));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_mc_sip_msg_register("1-2-3-4");
    testrun(msg);
    char *str = ov_json_value_to_string(msg);

    ssize_t bytes = -1;

    while (-1 == bytes) {

        bytes = send(client, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    loop->run(loop, OV_RUN_ONCE);

    str = ov_data_pointer_free(str);
    msg = ov_json_value_free(msg);

    int connection = ov_mc_backend_sip_registry_get_proxy_socket(sip->registry);
    testrun(connection > 0);

    msg = ov_mc_sip_msg_revoke((ov_sip_permission){
        .caller = "caller", .loop = "loop", .from_epoch = 1, .until_epoch = 2});

    ov_json_value *res = ov_event_api_create_success_response(msg);
    msg = ov_json_value_free(msg);

    cb_event_revoke_response(sip, connection, res);

    testrun(0 == strcmp(userdata.permission.caller, "caller"));
    testrun(0 == strcmp(userdata.permission.loop, "loop"));
    dummy_userdata_clear(&userdata);

    testrun(NULL == ov_mc_backend_sip_free(sip));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_sip_create_call() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig =
        (ov_mc_backend_sip_config){.loop = loop,
                                   .db = db,
                                   .backend = backend,
                                   .socket.manager = manager,
                                   .callback.userdata = &userdata,
                                   .callback.call.new = new_call,
                                   .callback.call.terminated = terminated_call};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);

    int client = ov_socket_create(manager, true, false);
    testrun(-1 != client);
    testrun(ov_socket_ensure_nonblocking(client));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_mc_sip_msg_register("1-2-3-4");
    testrun(msg);
    char *str = ov_json_value_to_string(msg);

    ssize_t bytes = -1;

    while (-1 == bytes) {

        bytes = send(client, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    loop->run(loop, OV_RUN_ONCE);

    str = ov_data_pointer_free(str);
    msg = ov_json_value_free(msg);

    int connection = ov_mc_backend_sip_registry_get_proxy_socket(sip->registry);
    testrun(connection > 0);

    testrun(!ov_mc_backend_sip_create_call(NULL, NULL, NULL, NULL));
    testrun(
        !ov_mc_backend_sip_create_call(NULL, "loop", "destination", "from"));
    testrun(!ov_mc_backend_sip_create_call(sip, NULL, "destination", "from"));
    testrun(!ov_mc_backend_sip_create_call(sip, "loop", NULL, "from"));

    testrun(ov_mc_backend_sip_create_call(sip, "loop", "destination", "from"));

    char buf[2048] = {0};
    bytes = -1;

    while (-1 == bytes) {

        bytes = recv(client, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);
    msg = ov_json_value_from_string(buf, bytes);
    testrun(ov_event_api_event_is(msg, OV_SIP_EVENT_CALL));
    msg = ov_json_value_free(msg);

    testrun(NULL == ov_mc_backend_sip_free(sip));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_sip_terminate_call() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig =
        (ov_mc_backend_sip_config){.loop = loop,
                                   .db = db,
                                   .backend = backend,
                                   .socket.manager = manager,
                                   .callback.userdata = &userdata,
                                   .callback.call.new = new_call,
                                   .callback.call.terminated = terminated_call};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);

    int client = ov_socket_create(manager, true, false);
    testrun(-1 != client);
    testrun(ov_socket_ensure_nonblocking(client));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_mc_sip_msg_register("1-2-3-4");
    testrun(msg);
    char *str = ov_json_value_to_string(msg);

    ssize_t bytes = -1;

    while (-1 == bytes) {

        bytes = send(client, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    loop->run(loop, OV_RUN_ONCE);

    str = ov_data_pointer_free(str);
    msg = ov_json_value_free(msg);

    int connection = ov_mc_backend_sip_registry_get_proxy_socket(sip->registry);
    testrun(connection > 0);

    testrun(!ov_mc_backend_sip_terminate_call(NULL, NULL));
    testrun(!ov_mc_backend_sip_terminate_call(NULL, "id"));
    testrun(!ov_mc_backend_sip_terminate_call(sip, NULL));
    testrun(!ov_mc_backend_sip_terminate_call(sip, "id"));

    // register a call
    testrun(ov_mc_backend_sip_registry_register_call(sip->registry, connection,
                                                     "id", "loop", "peer"));

    testrun(ov_mc_backend_sip_terminate_call(sip, "id"));

    char buf[2048] = {0};
    bytes = -1;

    while (-1 == bytes) {

        bytes = recv(client, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);
    msg = ov_json_value_from_string(buf, bytes);
    testrun(ov_event_api_event_is(msg, OV_KEY_HANGUP));
    msg = ov_json_value_free(msg);

    testrun(NULL == ov_mc_backend_sip_free(sip));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_sip_create_permission() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig =
        (ov_mc_backend_sip_config){.loop = loop,
                                   .db = db,
                                   .backend = backend,
                                   .socket.manager = manager,
                                   .callback.userdata = &userdata,
                                   .callback.call.new = new_call,
                                   .callback.call.terminated = terminated_call,
                                   .callback.call.permit = permit_call,
                                   .callback.call.revoke = revoke_call};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);

    int client = ov_socket_create(manager, true, false);
    testrun(-1 != client);
    testrun(ov_socket_ensure_nonblocking(client));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_mc_sip_msg_register("1-2-3-4");
    testrun(msg);
    char *str = ov_json_value_to_string(msg);

    ssize_t bytes = -1;

    while (-1 == bytes) {

        bytes = send(client, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    loop->run(loop, OV_RUN_ONCE);

    str = ov_data_pointer_free(str);
    msg = ov_json_value_free(msg);

    int connection = ov_mc_backend_sip_registry_get_proxy_socket(sip->registry);
    testrun(connection > 0);

    testrun(!ov_mc_backend_sip_create_permission(NULL, (ov_sip_permission){0}));
    testrun(!ov_mc_backend_sip_create_permission(sip, (ov_sip_permission){0}));

    testrun(ov_mc_backend_sip_create_permission(
        sip, (ov_sip_permission){.caller = "caller",
                                 .loop = "loop",
                                 .from_epoch = 1,
                                 .until_epoch = 2}));

    char buf[2048] = {0};
    bytes = -1;

    while (-1 == bytes) {

        bytes = recv(client, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);
    msg = ov_json_value_from_string(buf, bytes);
    testrun(ov_event_api_event_is(msg, OV_KEY_PERMIT));
    msg = ov_json_value_free(msg);

    testrun(NULL == ov_mc_backend_sip_free(sip));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_sip_terminate_permission() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig =
        (ov_mc_backend_sip_config){.loop = loop,
                                   .db = db,
                                   .backend = backend,
                                   .socket.manager = manager,
                                   .callback.userdata = &userdata,
                                   .callback.call.new = new_call,
                                   .callback.call.terminated = terminated_call,
                                   .callback.call.permit = permit_call,
                                   .callback.call.revoke = revoke_call};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);

    int client = ov_socket_create(manager, true, false);
    testrun(-1 != client);
    testrun(ov_socket_ensure_nonblocking(client));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_mc_sip_msg_register("1-2-3-4");
    testrun(msg);
    char *str = ov_json_value_to_string(msg);

    ssize_t bytes = -1;

    while (-1 == bytes) {

        bytes = send(client, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    loop->run(loop, OV_RUN_ONCE);

    str = ov_data_pointer_free(str);
    msg = ov_json_value_free(msg);

    int connection = ov_mc_backend_sip_registry_get_proxy_socket(sip->registry);
    testrun(connection > 0);

    testrun(
        !ov_mc_backend_sip_terminate_permission(NULL, (ov_sip_permission){0}));
    testrun(
        !ov_mc_backend_sip_terminate_permission(sip, (ov_sip_permission){0}));

    testrun(ov_mc_backend_sip_terminate_permission(
        sip, (ov_sip_permission){.caller = "caller",
                                 .loop = "loop",
                                 .from_epoch = 1,
                                 .until_epoch = 2}));

    char buf[2048] = {0};
    bytes = -1;

    while (-1 == bytes) {

        bytes = recv(client, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);
    msg = ov_json_value_from_string(buf, bytes);
    testrun(ov_event_api_event_is(msg, OV_KEY_REVOKE));
    msg = ov_json_value_free(msg);

    testrun(NULL == ov_mc_backend_sip_free(sip));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_sip_list_calls() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig =
        (ov_mc_backend_sip_config){.loop = loop,
                                   .db = db,
                                   .backend = backend,
                                   .socket.manager = manager,
                                   .callback.userdata = &userdata,
                                   .callback.call.new = new_call,
                                   .callback.call.terminated = terminated_call,
                                   .callback.call.permit = permit_call,
                                   .callback.call.revoke = revoke_call};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);

    int client = ov_socket_create(manager, true, false);
    testrun(-1 != client);
    testrun(ov_socket_ensure_nonblocking(client));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_mc_sip_msg_register("1-2-3-4");
    testrun(msg);
    char *str = ov_json_value_to_string(msg);

    ssize_t bytes = -1;

    while (-1 == bytes) {

        bytes = send(client, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    loop->run(loop, OV_RUN_ONCE);

    str = ov_data_pointer_free(str);
    msg = ov_json_value_free(msg);

    int connection = ov_mc_backend_sip_registry_get_proxy_socket(sip->registry);
    testrun(connection > 0);

    testrun(!ov_mc_backend_sip_list_calls(NULL, NULL));
    testrun(!ov_mc_backend_sip_list_calls(NULL, "1.2.3"));

    testrun(ov_mc_backend_sip_list_calls(sip, "1.2.3"));

    char buf[2048] = {0};
    bytes = -1;

    while (-1 == bytes) {

        bytes = recv(client, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);
    msg = ov_json_value_from_string(buf, bytes);
    testrun(ov_event_api_event_is(msg, OV_KEY_LIST_CALLS));
    testrun(0 == strcmp("1.2.3", ov_event_api_get_uuid(msg)));
    msg = ov_json_value_free(msg);

    testrun(NULL == ov_mc_backend_sip_free(sip));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_sip_list_permissions() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig =
        (ov_mc_backend_sip_config){.loop = loop,
                                   .db = db,
                                   .backend = backend,
                                   .socket.manager = manager,
                                   .callback.userdata = &userdata,
                                   .callback.call.new = new_call,
                                   .callback.call.terminated = terminated_call,
                                   .callback.call.permit = permit_call,
                                   .callback.call.revoke = revoke_call};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);

    int client = ov_socket_create(manager, true, false);
    testrun(-1 != client);
    testrun(ov_socket_ensure_nonblocking(client));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_mc_sip_msg_register("1-2-3-4");
    testrun(msg);
    char *str = ov_json_value_to_string(msg);

    ssize_t bytes = -1;

    while (-1 == bytes) {

        bytes = send(client, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    loop->run(loop, OV_RUN_ONCE);

    str = ov_data_pointer_free(str);
    msg = ov_json_value_free(msg);

    int connection = ov_mc_backend_sip_registry_get_proxy_socket(sip->registry);
    testrun(connection > 0);

    testrun(!ov_mc_backend_sip_list_permissions(NULL, NULL));
    testrun(!ov_mc_backend_sip_list_permissions(NULL, "1.2.3"));

    testrun(ov_mc_backend_sip_list_permissions(sip, "1.2.3"));

    char buf[2048] = {0};
    bytes = -1;

    while (-1 == bytes) {

        bytes = recv(client, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);
    msg = ov_json_value_from_string(buf, bytes);
    testrun(ov_event_api_event_is(msg, OV_KEY_LIST_PERMISSIONS));
    testrun(0 == strcmp("1.2.3", ov_event_api_get_uuid(msg)));
    msg = ov_json_value_free(msg);

    testrun(NULL == ov_mc_backend_sip_free(sip));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_backend_sip_get_status() {

    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    ov_socket_configuration manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_config bconfig = (ov_mc_backend_config){
        .loop = loop,
        .socket.manager = manager,
    };

    ov_mc_backend *backend = ov_mc_backend_create(bconfig);
    testrun(backend);

    manager = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_mc_backend_sip_config sconfig =
        (ov_mc_backend_sip_config){.loop = loop,
                                   .db = db,
                                   .backend = backend,
                                   .socket.manager = manager,
                                   .callback.userdata = &userdata,
                                   .callback.call.new = new_call,
                                   .callback.call.terminated = terminated_call,
                                   .callback.call.permit = permit_call,
                                   .callback.call.revoke = revoke_call};

    ov_mc_backend_sip *sip = ov_mc_backend_sip_create(sconfig);
    testrun(sip);

    int client = ov_socket_create(manager, true, false);
    testrun(-1 != client);
    testrun(ov_socket_ensure_nonblocking(client));
    loop->run(loop, OV_RUN_ONCE);
    usleep(5000);
    loop->run(loop, OV_RUN_ONCE);

    ov_json_value *msg = ov_mc_sip_msg_register("1-2-3-4");
    testrun(msg);
    char *str = ov_json_value_to_string(msg);

    ssize_t bytes = -1;

    while (-1 == bytes) {

        bytes = send(client, str, strlen(str), 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    loop->run(loop, OV_RUN_ONCE);

    str = ov_data_pointer_free(str);
    msg = ov_json_value_free(msg);

    int connection = ov_mc_backend_sip_registry_get_proxy_socket(sip->registry);
    testrun(connection > 0);

    testrun(!ov_mc_backend_sip_get_status(NULL, NULL));
    testrun(!ov_mc_backend_sip_get_status(NULL, "1.2.3"));

    testrun(ov_mc_backend_sip_get_status(sip, "1.2.3"));

    char buf[2048] = {0};
    bytes = -1;

    while (-1 == bytes) {

        bytes = recv(client, buf, 2048, 0);
        loop->run(loop, OV_RUN_ONCE);
    }

    // fprintf(stdout, "%s", buf);
    msg = ov_json_value_from_string(buf, bytes);
    testrun(ov_event_api_event_is(msg, OV_KEY_GET_STATUS));
    testrun(0 == strcmp("1.2.3", ov_event_api_get_uuid(msg)));
    msg = ov_json_value_free(msg);

    testrun(NULL == ov_mc_backend_sip_free(sip));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_mc_backend_free(backend));
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
    testrun_test(test_ov_mc_backend_sip_create);
    testrun_test(test_ov_mc_backend_sip_free);
    testrun_test(test_ov_mc_backend_sip_cast);

    testrun_test(check_cb_event_register);
    testrun_test(check_cb_mixer_released);
    testrun_test(check_cb_mixer_acquired);
    testrun_test(check_cb_event_acquire);
    testrun_test(check_cb_event_release);
    testrun_test(check_cb_mixer_forwarded);
    testrun_test(check_cb_event_set_singlecast);
    testrun_test(check_cb_event_notify);
    testrun_test(check_cb_event_permit);
    testrun_test(check_cb_event_revoke);

    testrun_test(test_ov_mc_backend_sip_create_call);
    testrun_test(test_ov_mc_backend_sip_terminate_call);
    testrun_test(test_ov_mc_backend_sip_create_permission);
    testrun_test(test_ov_mc_backend_sip_terminate_permission);

    testrun_test(test_ov_mc_backend_sip_list_calls);
    testrun_test(test_ov_mc_backend_sip_list_permissions);
    testrun_test(test_ov_mc_backend_sip_get_status);

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
