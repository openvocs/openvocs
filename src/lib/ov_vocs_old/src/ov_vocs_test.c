/***
        ------------------------------------------------------------------------

        Copyright (c) 2025 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_vocs_test.c
        @author         Töpfer, Markus

        @date           2025-02-03


        ------------------------------------------------------------------------
*/
#include "ov_vocs.c"
#include <ov_test/testrun.h>

#include "../include/ov_vocs_msg.h"

struct userdata {

    ov_json_value *msg;
    int socket;
};

/*----------------------------------------------------------------------------*/

static bool userdata_clear(struct userdata *self) {

    self->msg = ov_json_value_free(self->msg);
    self->socket = 0;
    return true;
};

/*----------------------------------------------------------------------------*/

static bool cb_close(void *userdata, int socket) {

    struct userdata *data = (struct userdata *)userdata;
    userdata_clear(data);
    data->socket = socket;
    return true;
}

/*----------------------------------------------------------------------------*/

static bool cb_send(void *userdata, int socket, const ov_json_value *msg) {

    struct userdata *data = (struct userdata *)userdata;
    userdata_clear(data);
    data->socket = socket;
    data->msg = NULL;
    ov_json_value_copy((void **)&data->msg, msg);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool create_test_db(ov_vocs_db *db) {

    if (!db) goto error;

    if (!ov_vocs_db_create_entity(
            db, OV_VOCS_DB_DOMAIN, "domain1", OV_VOCS_DB_SCOPE_DOMAIN, NULL))
        goto error;

    if (!ov_vocs_db_create_entity(
            db, OV_VOCS_DB_DOMAIN, "domain2", OV_VOCS_DB_SCOPE_DOMAIN, NULL))
        goto error;

    if (!ov_vocs_db_create_entity(
            db, OV_VOCS_DB_DOMAIN, "domain3", OV_VOCS_DB_SCOPE_DOMAIN, NULL))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_PROJECT,
                                  "project1",
                                  OV_VOCS_DB_SCOPE_DOMAIN,
                                  "domain1"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_PROJECT,
                                  "project2",
                                  OV_VOCS_DB_SCOPE_DOMAIN,
                                  "domain2"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_PROJECT,
                                  "project3",
                                  OV_VOCS_DB_SCOPE_DOMAIN,
                                  "domain3"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop11",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project1"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop12",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project1"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop13",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project1"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop21",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project2"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop22",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project2"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop23",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project2"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop31",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project3"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop32",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project3"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop33",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project3"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role11",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project1"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role12",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project1"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role13",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project1"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role21",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project2"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role22",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project2"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role23",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project2"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role31",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project3"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role32",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project3"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role33",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project3"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user11",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project1"))
        goto error;

    if (!ov_vocs_db_set_password(db, "user11", "user11")) goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user12",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project1"))
        goto error;

    if (!ov_vocs_db_set_password(db, "user12", "user12")) goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user13",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project1"))
        goto error;

    if (!ov_vocs_db_set_password(db, "user13", "user13")) goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user21",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project2"))
        goto error;

    if (!ov_vocs_db_set_password(db, "user21", "user21")) goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user22",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project2"))
        goto error;

    if (!ov_vocs_db_set_password(db, "user22", "user22")) goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user23",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project2"))
        goto error;

    if (!ov_vocs_db_set_password(db, "user23", "user23")) goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user31",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project3"))
        goto error;

    if (!ov_vocs_db_set_password(db, "user31", "user31")) goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user32",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project3"))
        goto error;

    if (!ov_vocs_db_set_password(db, "user32", "user32")) goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user33",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project3"))
        goto error;

    if (!ov_vocs_db_set_password(db, "user33", "user33")) goto error;

    ov_json_value *out = ov_json_object();
    ov_json_object_set(out, "role11", ov_json_true());
    ov_json_object_set(out, "role21", ov_json_false());
    ov_json_object_set(out, "role31", ov_json_false());
    ov_json_object_set(out, "role12", ov_json_true());
    ov_json_object_set(out, "role13", ov_json_true());

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop11", OV_KEY_ROLES, out))
        goto error;

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop12", OV_KEY_ROLES, out))
        goto error;

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop13", OV_KEY_ROLES, out))
        goto error;

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop21", OV_KEY_ROLES, out))
        goto error;

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop22", OV_KEY_ROLES, out))
        goto error;

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop23", OV_KEY_ROLES, out))
        goto error;

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop31", OV_KEY_ROLES, out))
        goto error;

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop32", OV_KEY_ROLES, out))
        goto error;

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop33", OV_KEY_ROLES, out))
        goto error;

    ov_json_value *mc = NULL;

    ov_socket_configuration_to_json(
        (ov_socket_configuration){
            .type = UDP, .host = "229.0.0.1", .port = 10000},
        &mc);

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop11", OV_KEY_MULTICAST, mc))
        goto error;

    mc = ov_json_value_free(mc);

    ov_socket_configuration_to_json(
        (ov_socket_configuration){
            .type = UDP, .host = "229.0.0.2", .port = 10000},
        &mc);

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop12", OV_KEY_MULTICAST, mc))
        goto error;

    mc = ov_json_value_free(mc);

    ov_socket_configuration_to_json(
        (ov_socket_configuration){
            .type = UDP, .host = "229.0.0.2", .port = 10001},
        &mc);

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop13", OV_KEY_MULTICAST, mc))
        goto error;

    mc = ov_json_value_free(mc);

    ov_socket_configuration_to_json(
        (ov_socket_configuration){
            .type = UDP, .host = "229.0.0.2", .port = 10002},
        &mc);

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop21", OV_KEY_MULTICAST, mc))
        goto error;

    mc = ov_json_value_free(mc);

    ov_socket_configuration_to_json(
        (ov_socket_configuration){
            .type = UDP, .host = "229.0.0.2", .port = 10003},
        &mc);

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop22", OV_KEY_MULTICAST, mc))
        goto error;

    mc = ov_json_value_free(mc);
    ov_socket_configuration_to_json(
        (ov_socket_configuration){
            .type = UDP, .host = "229.0.0.2", .port = 10004},
        &mc);

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop23", OV_KEY_MULTICAST, mc))
        goto error;

    mc = ov_json_value_free(mc);
    ov_socket_configuration_to_json(
        (ov_socket_configuration){
            .type = UDP, .host = "229.0.0.2", .port = 10005},
        &mc);

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop31", OV_KEY_MULTICAST, mc))
        goto error;

    mc = ov_json_value_free(mc);
    ov_socket_configuration_to_json(
        (ov_socket_configuration){
            .type = UDP, .host = "229.0.0.2", .port = 10006},
        &mc);

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop32", OV_KEY_MULTICAST, mc))
        goto error;

    mc = ov_json_value_free(mc);
    ov_socket_configuration_to_json(
        (ov_socket_configuration){
            .type = UDP, .host = "229.0.0.2", .port = 10007},
        &mc);

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop33", OV_KEY_MULTICAST, mc))
        goto error;

    mc = ov_json_value_free(mc);

    out = ov_json_value_free(out);
    out = ov_json_object();

    ov_json_object_set(out, "user31", ov_json_null());
    ov_json_object_set(out, "user32", ov_json_null());
    ov_json_object_set(out, "user33", ov_json_null());

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_ROLE, "role31", OV_KEY_USERS, out))
        goto error;

    out = ov_json_value_free(out);
    out = ov_json_object();

    ov_json_object_set(out, "user11", ov_json_null());
    ov_json_object_set(out, "user12", ov_json_null());
    ov_json_object_set(out, "user13", ov_json_null());

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_ROLE, "role11", OV_KEY_USERS, out))
        goto error;

    out = ov_json_value_free(out);

    out = ov_json_value_free(out);
    out = ov_json_object();

    ov_json_object_set(out, "user21", ov_json_null());
    ov_json_object_set(out, "user22", ov_json_null());
    ov_json_object_set(out, "user23", ov_json_null());

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_ROLE, "role21", OV_KEY_USERS, out))
        goto error;

    out = ov_json_value_free(out);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static ov_vocs_config get_default_vocs_config(struct userdata *userdata) {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_socket_configuration ice_socket = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_socket_configuration mixer_socket = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_socket_configuration sip_socket = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_socket_configuration event_socket = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_socket_configuration rec_socket = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_socket_configuration vad_socket = ov_socket_load_dynamic_port(
        (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"});

    ov_vocs_config config = (ov_vocs_config){

        .loop = loop,
        .db = ov_vocs_db_create((ov_vocs_db_config){0}),
        .io = ov_io_create((ov_io_config){.loop = loop}),

        .env.userdata = userdata,
        .env.close = cb_close,
        .env.send = cb_send,

        .module.backend.socket.manager = mixer_socket,
        .module.frontend.socket.manager = ice_socket,
        .module.events.socket.manager = event_socket,
        .module.sip.socket.manager = sip_socket,
        .module.recorder.socket.manager = rec_socket,
        .module.vad.socket = vad_socket};

    return config;
}

/*----------------------------------------------------------------------------*/

static void clear_default_vocs_config(ov_vocs_config *cfg) {

    if (0 != cfg) {
        cfg->io = ov_io_free(cfg->io);
        cfg->db = ov_vocs_db_free(cfg->db);
        cfg->loop = ov_event_loop_free(cfg->loop);
    }

    return;
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_vocs_create() {

    struct userdata userdata = {0};

    ov_vocs_config config = get_default_vocs_config(&userdata);

    testrun(create_test_db(config.db));
    testrun(config.loop);

    ov_vocs *vocs = ov_vocs_create(config);
    testrun(vocs);
    testrun(ov_vocs_cast(vocs));
    testrun(vocs->backend);
    testrun(vocs->frontend);
    testrun(vocs->async);
    testrun(vocs->broadcasts);
    testrun(vocs->sessions);
    testrun(vocs->loops);
    testrun(vocs->io);

    testrun(NULL == ov_vocs_free(vocs));

    clear_default_vocs_config(&config);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_login_logout_cycle() {

    struct userdata userdata = {0};

    ov_vocs_config config = get_default_vocs_config(&userdata);

    testrun(create_test_db(config.db));
    testrun(config.loop);

    ov_vocs *vocs = ov_vocs_create(config);
    testrun(vocs);

    ov_json_value *in = ov_vocs_msg_login("user12", "user12", "user12");
    testrun(client_login(vocs, 5, NULL, in));
    testrun(ov_event_api_event_is(userdata.msg, OV_KEY_LOGIN));
    userdata_clear(&userdata);

    in = ov_vocs_msg_authorise("role11");
    testrun(client_authorize(vocs, 5, NULL, in));
    testrun(ov_event_api_event_is(userdata.msg, OV_KEY_AUTHORISE));
    userdata_clear(&userdata);

    in = ov_vocs_msg_get("user");
    testrun(client_get(vocs, 5, NULL, in));
    testrun(ov_event_api_event_is(userdata.msg, "get"));
    userdata_clear(&userdata);

    in = ov_vocs_msg_client_user_roles();
    testrun(client_user_roles(vocs, 5, NULL, in));
    testrun(ov_event_api_event_is(userdata.msg, "user_roles"));
    userdata_clear(&userdata);

    in = ov_vocs_msg_client_role_loops();
    testrun(client_user_roles(vocs, 5, NULL, in));
    testrun(ov_event_api_event_is(userdata.msg, "role_loops"));
    userdata_clear(&userdata);

    in = ov_vocs_msg_switch_loop_state("loop11", OV_VOCS_RECV);
    testrun(!client_switch_loop_state(vocs, 5, NULL, in));
    userdata_clear(&userdata);

    in = ov_vocs_msg_logout();
    testrun(client_logout(vocs, 5, NULL, in));

    testrun(NULL == ov_vocs_free(vocs));
    clear_default_vocs_config(&config);

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
    // testrun_test(test_ov_vocs_create);

    testrun_test(test_ov_vocs_login_logout_cycle);

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
