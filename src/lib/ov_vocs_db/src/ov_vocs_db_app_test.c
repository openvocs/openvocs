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
        @file           ov_vocs_db_app_test.c
        @author         Markus Töpfer

        @date           2023-11-16


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_test_db.h"
#include "ov_vocs_db_app.c"
#include <ov_test/testrun.h>

#include <ov_base/ov_dir.h>

struct dummy_userdata {

    int socket;
    ov_json_value *value;
};

/*----------------------------------------------------------------------------*/

static bool dummy_userdata_clear(struct dummy_userdata *u) {

    if (!u) return false;
    u->socket = 0;
    u->value = ov_json_value_free(u->value);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool dummy_send(void *userdata, int socket, const ov_json_value *val) {

    struct dummy_userdata *u = (struct dummy_userdata *)userdata;
    dummy_userdata_clear(u);
    u->socket = socket;
    u->value = ov_json_value_copy((void **)&u->value, val);

    return true;
}

/*----------------------------------------------------------------------------*/

static bool dummy_close(void *userdata, int socket) {

    struct dummy_userdata *u = (struct dummy_userdata *)userdata;
    dummy_userdata_clear(u);
    u->socket = socket;

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_vocs_db_app_create() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance = ov_vocs_db_persistance_create(
        (ov_vocs_db_persistance_config){.loop = loop, .db = db});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);
    testrun(ov_vocs_db_app_cast(app));
    testrun(app->async);
    testrun(app->engine);
    testrun(app->connections);
    testrun(!app->ldap);

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_app_free() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance = ov_vocs_db_persistance_create(
        (ov_vocs_db_persistance_config){.loop = loop, .db = db});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_connection_create() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance = ov_vocs_db_persistance_create(
        (ov_vocs_db_persistance_config){.loop = loop, .db = db});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    Connection *c = connection_create(app, 1, "user", "client");
    testrun(c);
    testrun(c == ov_dict_get(app->connections, (void *)(intptr_t)1));
    testrun(0 == strcmp(get_connection_user(c), "user"));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_authorize_connection_for_id() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance = ov_vocs_db_persistance_create(
        (ov_vocs_db_persistance_config){.loop = loop, .db = db});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    Connection *c1 = connection_create(app, 1, "user1", "client1");
    Connection *c2 = connection_create(app, 2, "user2", "client2");
    Connection *c3 = connection_create(app, 3, "user3", "client3");
    testrun(c1);
    testrun(c2);
    testrun(c3);

    // check config
    testrun(ov_vocs_db_authorize_domain_admin(db, "user1", "localhost"));
    testrun(
        ov_vocs_db_authorize_project_admin(db, "user2", "project@localhost"));
    testrun(
        ov_vocs_db_authorize_project_admin(db, "user1", "project@localhost"));

    // check autorization
    testrun(
        authorize_connection_for_id(app, 1, OV_VOCS_DB_DOMAIN, "localhost"));
    testrun(
        !authorize_connection_for_id(app, 2, OV_VOCS_DB_DOMAIN, "localhost"));

    testrun(authorize_connection_for_id(
        app, 1, OV_VOCS_DB_PROJECT, "project@localhost"));
    testrun(!authorize_connection_for_id(
        app, 2, OV_VOCS_DB_PROJECT, "project@localhost"));
    testrun(!authorize_connection_for_id(
        app, 3, OV_VOCS_DB_PROJECT, "project@localhost"));

    testrun(authorize_connection_for_id(app, 1, OV_VOCS_DB_LOOP, "loop3"));
    testrun(authorize_connection_for_id(app, 2, OV_VOCS_DB_LOOP, "loop3"));
    testrun(!authorize_connection_for_id(app, 3, OV_VOCS_DB_LOOP, "loop3"));

    testrun(authorize_connection_for_id(app, 1, OV_VOCS_DB_USER, "user2"));
    testrun(authorize_connection_for_id(app, 2, OV_VOCS_DB_USER, "user2"));
    testrun(!authorize_connection_for_id(app, 3, OV_VOCS_DB_USER, "user2"));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_login(const char *user, const char *pass) {

    ov_json_value *out = ov_event_api_message_create(OV_KEY_LOGIN, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_value *val = NULL;

    val = ov_json_string(user);
    ov_json_object_set(out, OV_KEY_CLIENT, val);

    if (user) {

        val = ov_json_string(user);
        ov_json_object_set(par, OV_KEY_USER, val);
    }

    if (pass) {

        val = ov_json_string(pass);
        ov_json_object_set(par, OV_KEY_PASSWORD, val);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_login() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance = ov_vocs_db_persistance_create(
        (ov_vocs_db_persistance_config){.loop = loop, .db = db});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    // test1 parameter error
    ov_json_value *msg = msg_login("user1", NULL);
    testrun(msg);

    testrun(cb_event_login(app, 1, &params, msg));
    testrun(userdata.value);
    testrun(ov_event_api_event_is(userdata.value, OV_KEY_LOGIN));
    testrun(OV_ERROR_CODE_PARAMETER_ERROR ==
            ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    // test2 password error
    msg = msg_login("user1", "wrong_password");
    testrun(msg);

    testrun(cb_event_login(app, 1, &params, msg));
    testrun(userdata.value);
    testrun(ov_event_api_event_is(userdata.value, OV_KEY_LOGIN));
    testrun(OV_ERROR_CODE_AUTH == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    // check no connection setup yet
    testrun(0 == ov_dict_count(app->connections));

    // test3 correct login
    msg = msg_login("user1", "user1");
    testrun(msg);

    testrun(cb_event_login(app, 1, &params, msg));
    testrun(userdata.value);
    testrun(ov_event_api_event_is(userdata.value, OV_KEY_LOGIN));
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(ov_event_api_get_response(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    // check 1 connection setup yet
    testrun(1 == ov_dict_count(app->connections));

    Connection *c = get_connection(app, 1);
    const char *u = get_connection_user(c);
    testrun(0 == strcmp(u, "user1"));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_logout() {

    return ov_event_api_message_create(OV_KEY_LOGOUT, NULL, 0);
}

/*----------------------------------------------------------------------------*/

int check_cb_event_logout() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance = ov_vocs_db_persistance_create(
        (ov_vocs_db_persistance_config){.loop = loop, .db = db});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    // login user
    ov_json_value *msg = msg_login("user1", "user1");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(userdata.value);
    testrun(ov_event_api_event_is(userdata.value, OV_KEY_LOGIN));
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // logout user
    msg = msg_logout();
    testrun(msg);
    testrun(cb_event_logout(app, 1, &params, msg));
    // NOTE drop_connection will clear the msg in dummy_userdata
    testrun(1 == userdata.socket);
    testrun(dummy_userdata_clear(&userdata));
    testrun(0 == ov_dict_count(app->connections));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_update_password(const char *user, const char *pass) {

    ov_json_value *out =
        ov_event_api_message_create(OV_VOCS_DB_UPDATE_PASSWORD, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_value *val = NULL;

    if (user) {

        val = ov_json_string(user);
        ov_json_object_set(par, OV_KEY_USER, val);
    }

    if (pass) {

        val = ov_json_string(pass);
        ov_json_object_set(par, OV_KEY_PASSWORD, val);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_update_password() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance = ov_vocs_db_persistance_create(
        (ov_vocs_db_persistance_config){.loop = loop, .db = db});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    ov_json_value *msg = msg_update_password("user1", "password");
    testrun(msg);

    // no user logged in
    testrun(!cb_event_update_password(app, 1, &params, msg));

    // login user
    msg = msg_login("user1", "user1");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check parameter error
    msg = msg_update_password("user1", NULL);
    testrun(msg);
    testrun(cb_event_update_password(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(OV_ERROR_CODE_PARAMETER_ERROR ==
            ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // check ldap enabled
    app->config.ldap.enable = true;
    msg = msg_update_password("user1", "new password");
    testrun(msg);
    testrun(cb_event_update_password(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(OV_ERROR_CODE_AUTH_LDAP_USED ==
            ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));
    app->config.ldap.enable = false;

    // check own password update no admin
    msg = msg_login("user3", "user3");
    testrun(ov_vocs_db_authenticate(db, "user3", "user3"));
    testrun(msg);
    testrun(cb_event_login(app, 2, &params, msg));
    testrun(2 == ov_dict_count(app->connections));
    msg = msg_update_password("user3", "new password");
    testrun(msg);
    testrun(cb_event_update_password(app, 2, &params, msg));
    testrun(2 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));
    // check updated password
    testrun(ov_vocs_db_authenticate(db, "user3", "new password"));

    // check password update as project admin
    msg = msg_login("user2", "user2");
    testrun(ov_vocs_db_authenticate(db, "user3", "new password"));
    testrun(msg);
    testrun(cb_event_login(app, 3, &params, msg));
    testrun(3 == ov_dict_count(app->connections));
    msg = msg_update_password("user3", "password reset");
    testrun(msg);
    testrun(cb_event_update_password(app, 3, &params, msg));
    testrun(3 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));
    // check updated password
    testrun(ov_vocs_db_authenticate(db, "user3", "password reset"));

    // check password reset as domain admin
    testrun(ov_vocs_db_authenticate(db, "user3", "password reset"));
    testrun(msg);
    msg = msg_update_password("user3", "user3");
    testrun(msg);
    testrun(cb_event_update_password(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));
    // check updated password
    testrun(ov_vocs_db_authenticate(db, "user3", "user3"));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_get_admin_domains(const char *user) {

    ov_json_value *out =
        ov_event_api_message_create(OV_VOCS_DB_ADMIN_DOMAINS, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_value *val = NULL;

    if (user) {

        val = ov_json_string(user);
        ov_json_object_set(par, OV_KEY_USER, val);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_get_admin_domains() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance = ov_vocs_db_persistance_create(
        (ov_vocs_db_persistance_config){.loop = loop, .db = db});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    ov_json_value *msg = msg_get_admin_domains("user1");
    testrun(msg);

    // no user logged in
    testrun(!cb_event_get_admin_domains(app, 1, &params, msg));

    // login user
    msg = msg_login("user1", "user1");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check parameter error
    msg = msg_get_admin_domains(NULL);
    testrun(msg);
    testrun(cb_event_get_admin_domains(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // testrun(OV_ERROR_CODE_PARAMETER_ERROR ==
    //         ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // check success
    msg = msg_get_admin_domains("user1");
    testrun(msg);
    testrun(cb_event_get_admin_domains(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    const ov_json_value *domains =
        ov_json_get(userdata.value, "/" OV_KEY_RESPONSE "/" OV_KEY_DOMAINS);
    testrun(domains);
    testrun(1 == ov_json_array_count(domains));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // check success
    msg = msg_get_admin_domains("user3");
    testrun(msg);
    testrun(cb_event_get_admin_domains(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    domains =
        ov_json_get(userdata.value, "/" OV_KEY_RESPONSE "/" OV_KEY_DOMAINS);
    testrun(domains);
    testrun(0 == ov_json_array_count(domains));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_get_admin_projects(const char *user) {

    ov_json_value *out =
        ov_event_api_message_create(OV_VOCS_DB_ADMIN_PROJECTS, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_value *val = NULL;

    if (user) {

        val = ov_json_string(user);
        ov_json_object_set(par, OV_KEY_USER, val);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_get_admin_projects() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance = ov_vocs_db_persistance_create(
        (ov_vocs_db_persistance_config){.loop = loop, .db = db});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    ov_json_value *msg = msg_get_admin_projects("user1");
    testrun(msg);

    // no user logged in
    testrun(!cb_event_get_admin_projects(app, 1, &params, msg));

    // login user
    msg = msg_login("user1", "user1");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check parameter error
    msg = msg_get_admin_projects(NULL);
    testrun(msg);
    testrun(cb_event_get_admin_projects(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // testrun(OV_ERROR_CODE_PARAMETER_ERROR ==
    //         ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // check success domain admin
    msg = msg_get_admin_projects("user1");
    testrun(msg);
    testrun(cb_event_get_admin_projects(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    const ov_json_value *projects =
        ov_json_get(userdata.value, "/" OV_KEY_RESPONSE "/" OV_KEY_PROJECTS);
    testrun(projects);
    testrun(1 == ov_json_object_count(projects));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // check success project admin
    msg = msg_get_admin_projects("user2");
    testrun(msg);
    testrun(cb_event_get_admin_projects(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    projects =
        ov_json_get(userdata.value, "/" OV_KEY_RESPONSE "/" OV_KEY_PROJECTS);
    testrun(projects);
    testrun(1 == ov_json_object_count(projects));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // check success no admin
    msg = msg_get_admin_projects("user3");
    testrun(msg);
    testrun(cb_event_get_admin_projects(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    projects =
        ov_json_get(userdata.value, "/" OV_KEY_RESPONSE "/" OV_KEY_PROJECTS);
    testrun(projects);
    testrun(0 == ov_json_object_count(projects));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_check_id_exists(const char *id, const char *scope) {

    ov_json_value *out =
        ov_event_api_message_create(OV_VOCS_DB_ID_EXISTS, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_value *val = NULL;

    if (id) {

        val = ov_json_string(id);
        ov_json_object_set(par, OV_KEY_ID, val);
    }

    if (scope) {

        val = ov_json_string(scope);
        ov_json_object_set(par, OV_KEY_SCOPE, val);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_check_id_exists() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance = ov_vocs_db_persistance_create(
        (ov_vocs_db_persistance_config){.loop = loop, .db = db});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    ov_json_value *msg = msg_check_id_exists("user1", "user");
    testrun(msg);

    // no user logged in
    testrun(!cb_event_check_id_exists(app, 1, &params, msg));

    // login user
    msg = msg_login("user1", "user1");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check parameter error
    msg = msg_check_id_exists(NULL, NULL);
    testrun(msg);
    testrun(cb_event_check_id_exists(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(OV_ERROR_CODE_PARAMETER_ERROR ==
            ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // check success without scope
    msg = msg_check_id_exists("user1", NULL);
    testrun(msg);
    testrun(cb_event_check_id_exists(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(ov_json_is_true(
        ov_json_get(userdata.value, "/" OV_KEY_RESPONSE "/" OV_KEY_RESULT)));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // check success with scope
    msg = msg_check_id_exists("user1", "user");
    testrun(msg);
    testrun(cb_event_check_id_exists(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(ov_json_is_true(
        ov_json_get(userdata.value, "/" OV_KEY_RESPONSE "/" OV_KEY_RESULT)));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // check success with different scope
    msg = msg_check_id_exists("user1", "role");
    testrun(msg);
    testrun(cb_event_check_id_exists(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(ov_json_is_false(
        ov_json_get(userdata.value, "/" OV_KEY_RESPONSE "/" OV_KEY_RESULT)));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // check success with non existing id
    msg = msg_check_id_exists("id which is not set yet", NULL);
    testrun(msg);
    testrun(cb_event_check_id_exists(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(ov_json_is_false(
        ov_json_get(userdata.value, "/" OV_KEY_RESPONSE "/" OV_KEY_RESULT)));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_get(const char *id, const char *type) {

    ov_json_value *out = ov_event_api_message_create(OV_VOCS_DB_GET, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_value *val = NULL;

    if (id) {

        val = ov_json_string(id);
        ov_json_object_set(par, OV_KEY_ID, val);
    }

    if (type) {

        val = ov_json_string(type);
        ov_json_object_set(par, OV_KEY_TYPE, val);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_get() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance = ov_vocs_db_persistance_create(
        (ov_vocs_db_persistance_config){.loop = loop, .db = db});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    ov_json_value *msg = msg_get("localhost", "domain");
    testrun(msg);

    // no user logged in
    testrun(!cb_event_get(app, 1, &params, msg));

    // login user
    msg = msg_login("user1", "user1");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check parameter error
    msg = msg_get(NULL, NULL);
    testrun(msg);
    testrun(cb_event_get(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(OV_ERROR_CODE_PARAMETER_ERROR ==
            ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // check success
    msg = msg_get("localhost", "domain");
    testrun(msg);
    testrun(cb_event_get(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(ov_json_get(userdata.value, "/" OV_KEY_RESPONSE "/" OV_KEY_RESULT));
    // ov_json_value_dump(stdout, userdata.value);
    testrun(dummy_userdata_clear(&userdata));

    msg = msg_get("project@localhost", "project");
    testrun(msg);
    testrun(cb_event_get(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(ov_json_get(userdata.value, "/" OV_KEY_RESPONSE "/" OV_KEY_RESULT));
    // ov_json_value_dump(stdout, userdata.value);
    testrun(dummy_userdata_clear(&userdata));

    msg = msg_get("loop1", "loop");
    testrun(msg);
    testrun(cb_event_get(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(ov_json_get(userdata.value, "/" OV_KEY_RESPONSE "/" OV_KEY_RESULT));
    // ov_json_value_dump(stdout, userdata.value);
    testrun(dummy_userdata_clear(&userdata));

    // check error get result null
    msg = msg_get("loop123", "loop");
    testrun(msg);
    testrun(cb_event_get(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(ov_json_is_null(
        ov_json_get(userdata.value, "/" OV_KEY_RESPONSE "/" OV_KEY_RESULT)));
    // ov_json_value_dump(stdout, userdata.value);
    testrun(dummy_userdata_clear(&userdata));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_delete(const char *id, const char *type) {

    ov_json_value *out =
        ov_event_api_message_create(OV_VOCS_DB_DELETE, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_value *val = NULL;

    if (id) {

        val = ov_json_string(id);
        ov_json_object_set(par, OV_KEY_ID, val);
    }

    if (type) {

        val = ov_json_string(type);
        ov_json_object_set(par, OV_KEY_TYPE, val);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_delete() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance = ov_vocs_db_persistance_create(
        (ov_vocs_db_persistance_config){.loop = loop, .db = db});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    ov_json_value *msg = msg_delete("loop3", "loop");
    testrun(msg);

    // no user logged in
    testrun(!cb_event_delete(app, 1, &params, msg));

    // login standard user
    msg = msg_login("user3", "user3");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check parameter error
    msg = msg_delete(NULL, NULL);
    testrun(msg);
    testrun(cb_event_delete(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(OV_ERROR_CODE_PARAMETER_ERROR ==
            ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // check permission error
    msg = msg_delete("loop3", "loop");
    testrun(msg);
    testrun(cb_event_delete(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(OV_ERROR_CODE_AUTH == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    // login admin user
    msg = msg_login("user1", "user1");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    msg = msg_delete("loop3", "loop");
    testrun(msg);
    testrun(cb_event_delete(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_create(const char *id,
                                 const char *type,
                                 const char *scope_type,
                                 const char *scope_id) {

    ov_json_value *out =
        ov_event_api_message_create(OV_VOCS_DB_CREATE, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_value *val = NULL;

    if (id) {

        val = ov_json_string(id);
        ov_json_object_set(par, OV_KEY_ID, val);
    }

    if (type) {

        val = ov_json_string(type);
        ov_json_object_set(par, OV_KEY_TYPE, val);
    }

    ov_json_value *scope = ov_json_object();
    ov_json_object_set(par, OV_KEY_SCOPE, scope);

    if (scope_type) {

        val = ov_json_string(scope_type);
        ov_json_object_set(scope, OV_KEY_TYPE, val);
    }

    if (scope_id) {

        val = ov_json_string(scope_id);
        ov_json_object_set(scope, OV_KEY_ID, val);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_create() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance = ov_vocs_db_persistance_create(
        (ov_vocs_db_persistance_config){.loop = loop, .db = db});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    ov_json_value *msg =
        msg_create("loop3", "loop", "project", "project@localhost");
    testrun(msg);

    // no user logged in
    testrun(!cb_event_create(app, 1, &params, msg));

    // login standard user
    msg = msg_login("user3", "user3");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check parameter error
    msg = msg_create("loop3", NULL, "project", "project@localhost");
    testrun(msg);
    testrun(cb_event_create(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(OV_ERROR_CODE_PARAMETER_ERROR ==
            ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // check permission error
    msg = msg_create("loop3", "loop", "project", "project@localhost");
    testrun(msg);
    testrun(cb_event_create(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(OV_ERROR_CODE_AUTH == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    // login admin user
    msg = msg_login("user1", "user1");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    msg = msg_create("loop3", "loop", "project", "project@localhost");
    testrun(msg);
    testrun(cb_event_create(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    msg = msg_create("unkown", "loop", "project", "project@localhost");
    testrun(msg);
    testrun(cb_event_create(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    msg = msg_create("user42", "user", "project", "project@localhost");
    testrun(msg);
    testrun(cb_event_create(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    ov_vocs_db_dump(stdout, db);
    OV_ASSERT(1 == 0);

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_get_key(const char *id,
                                  const char *type,
                                  const char *key) {

    ov_json_value *out =
        ov_event_api_message_create(OV_VOCS_DB_GET_KEY, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_value *val = NULL;

    if (id) {

        val = ov_json_string(id);
        ov_json_object_set(par, OV_KEY_ID, val);
    }

    if (type) {

        val = ov_json_string(type);
        ov_json_object_set(par, OV_KEY_TYPE, val);
    }

    if (key) {

        val = ov_json_string(key);
        ov_json_object_set(par, OV_KEY_KEY, val);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_get_key() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance = ov_vocs_db_persistance_create(
        (ov_vocs_db_persistance_config){.loop = loop, .db = db});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    ov_json_value *msg = msg_get_key("loop3", "loop", "id");
    testrun(msg);

    // no user logged in
    testrun(!cb_event_get_key(app, 1, &params, msg));

    // login standard user
    msg = msg_login("user3", "user3");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check parameter error
    msg = msg_get_key("loop3", "loop", NULL);
    testrun(msg);
    testrun(cb_event_get_key(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(OV_ERROR_CODE_PARAMETER_ERROR ==
            ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // check permission error
    msg = msg_get_key("loop3", "loop", "id");
    testrun(msg);
    testrun(cb_event_get_key(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_update_key(const char *id,
                                     const char *type,
                                     const char *key,
                                     ov_json_value *data) {

    ov_json_value *out =
        ov_event_api_message_create(OV_VOCS_DB_UPDATE_KEY, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_value *val = NULL;

    if (id) {

        val = ov_json_string(id);
        ov_json_object_set(par, OV_KEY_ID, val);
    }

    if (type) {

        val = ov_json_string(type);
        ov_json_object_set(par, OV_KEY_TYPE, val);
    }

    if (key) {

        val = ov_json_string(key);
        ov_json_object_set(par, OV_KEY_KEY, val);
    }

    if (data) {

        ov_json_object_set(par, OV_KEY_DATA, data);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_update_key() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance = ov_vocs_db_persistance_create(
        (ov_vocs_db_persistance_config){.loop = loop, .db = db});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    ov_json_value *msg =
        msg_update_key("loop3", "loop", "name", ov_json_string("loopname"));
    testrun(msg);

    // no user logged in
    testrun(!cb_event_update_key(app, 1, &params, msg));

    // login standard user
    msg = msg_login("user3", "user3");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check parameter error
    msg = msg_update_key("loop3", "loop", NULL, ov_json_string("loopname"));
    testrun(msg);
    testrun(cb_event_update_key(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(OV_ERROR_CODE_PARAMETER_ERROR ==
            ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // check permission error
    msg = msg_update_key("loop3", "loop", "name", ov_json_string("loopname"));
    testrun(msg);
    testrun(cb_event_update_key(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(OV_ERROR_CODE_AUTH == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    // login admin user
    msg = msg_login("user1", "user1");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check no permission error
    ov_json_value *out =
        ov_vocs_db_get_entity_key(db, OV_VOCS_DB_LOOP, "loop3", "name");
    testrun(out);
    testrun(ov_json_is_null(out));
    out = ov_json_value_free(out);
    msg = msg_update_key("loop3", "loop", "name", ov_json_string("loopname"));
    testrun(msg);
    testrun(cb_event_update_key(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));
    out = ov_vocs_db_get_entity_key(db, OV_VOCS_DB_LOOP, "loop3", "name");
    testrun(out);
    testrun(ov_json_is_string(out));
    testrun(0 == strcmp("loopname", ov_json_string_get(out)));
    out = ov_json_value_free(out);

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_delete_key(const char *id,
                                     const char *type,
                                     const char *key) {

    ov_json_value *out =
        ov_event_api_message_create(OV_VOCS_DB_DELETE_KEY, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_value *val = NULL;

    if (id) {

        val = ov_json_string(id);
        ov_json_object_set(par, OV_KEY_ID, val);
    }

    if (type) {

        val = ov_json_string(type);
        ov_json_object_set(par, OV_KEY_TYPE, val);
    }

    if (key) {

        val = ov_json_string(key);
        ov_json_object_set(par, OV_KEY_KEY, val);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_delete_key() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance = ov_vocs_db_persistance_create(
        (ov_vocs_db_persistance_config){.loop = loop, .db = db});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    // prep add some key

    ov_json_value *msg = NULL;

    // login admin user
    msg = msg_login("user1", "user1");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));
    testrun(dummy_userdata_clear(&userdata));
    // add key
    msg = msg_update_key("loop3", "loop", "name", ov_json_string("loopname"));
    testrun(cb_event_update_key(app, 1, &params, msg));
    // ov_json_value_dump(stdout, userdata.value);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    // no user logged in
    msg = msg_delete_key("loop3", "loop", "name");
    testrun(!cb_event_delete_key(app, 2, &params, msg));

    // login standard user
    msg = msg_login("user3", "user3");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check parameter error
    msg = msg_delete_key("loop3", "loop", NULL);
    testrun(msg);
    testrun(cb_event_delete_key(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(OV_ERROR_CODE_PARAMETER_ERROR ==
            ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // check permission error
    msg = msg_delete_key("loop3", "loop", "name");
    testrun(msg);
    testrun(cb_event_delete_key(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(OV_ERROR_CODE_AUTH == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_verify(const char *id,
                                 const char *type,
                                 ov_json_value *data) {

    ov_json_value *out =
        ov_event_api_message_create(OV_VOCS_DB_VERIFY, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_value *val = NULL;

    if (id) {

        val = ov_json_string(id);
        ov_json_object_set(par, OV_KEY_ID, val);
    }

    if (type) {

        val = ov_json_string(type);
        ov_json_object_set(par, OV_KEY_TYPE, val);
    }

    if (data) {

        ov_json_object_set(par, OV_KEY_DATA, data);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *create_loop_object(const char *name, const char *color) {

    ov_json_value *out = ov_json_object();
    ov_json_value *val = ov_json_string(name);
    ov_json_object_set(out, OV_KEY_NAME, val);
    val = ov_json_string(color);
    ov_json_object_set(out, OV_KEY_COLOR, val);
    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_verify() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance = ov_vocs_db_persistance_create(
        (ov_vocs_db_persistance_config){.loop = loop, .db = db});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    // prep add some key

    ov_json_value *msg = create_loop_object("name", "red");
    ov_json_value *err = NULL;
    testrun(
        ov_vocs_db_verify_entity_item(db, OV_VOCS_DB_LOOP, "loop3", msg, &err));
    testrun(NULL == err);
    msg = ov_json_value_free(msg);

    // no user logged in
    msg = msg_verify("loop3", "loop", create_loop_object("name", "red"));
    testrun(!cb_event_verify(app, 2, &params, msg))

        // login standard user
        msg = msg_login("user3", "user3");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check parameter error
    msg = msg_verify("loop3", "loop", NULL);
    testrun(msg);
    testrun(cb_event_verify(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(OV_ERROR_CODE_PARAMETER_ERROR ==
            ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // check permission error
    msg = msg_verify("loop3", "loop", create_loop_object("name", "red"));
    testrun(msg);
    testrun(cb_event_verify(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(OV_ERROR_CODE_AUTH == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    // login admin user
    msg = msg_login("user1", "user1");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check no permission error
    msg = msg_verify("loop3", "loop", create_loop_object("name", "red"));
    testrun(msg);
    testrun(cb_event_verify(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    // check verify error
    msg = create_loop_object("name", "red");
    ov_json_object_set(msg, OV_KEY_ID, ov_json_string("loop5"));
    // ov_json_value_dump(stdout, msg);
    testrun(!ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_LOOP, "loop3", msg, &err));
    testrun(NULL != err);
    ov_json_value_free(err);

    err = msg;
    msg = msg_verify("loop3", "loop", err);
    testrun(cb_event_verify(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(OV_ERROR_CODE_PROCESSING_ERROR ==
            ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_update(const char *id,
                                 const char *type,
                                 ov_json_value *data) {

    ov_json_value *out =
        ov_event_api_message_create(OV_VOCS_DB_UPDATE, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_value *val = NULL;

    if (id) {

        val = ov_json_string(id);
        ov_json_object_set(par, OV_KEY_ID, val);
    }

    if (type) {

        val = ov_json_string(type);
        ov_json_object_set(par, OV_KEY_TYPE, val);
    }

    if (data) {

        ov_json_object_set(par, OV_KEY_DATA, data);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_update() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance = ov_vocs_db_persistance_create(
        (ov_vocs_db_persistance_config){.loop = loop, .db = db});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    // prep add some key

    ov_json_value *msg = NULL;

    // no user logged in
    msg = msg_update("loop3", "loop", create_loop_object("name", "red"));
    testrun(!cb_event_update(app, 2, &params, msg))

        // login standard user
        msg = msg_login("user3", "user3");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check parameter error
    msg = msg_update("loop3", "loop", NULL);
    testrun(msg);
    testrun(cb_event_update(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    testrun(OV_ERROR_CODE_PARAMETER_ERROR ==
            ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));
    testrun(1 == ov_dict_count(app->connections));

    // check permission error
    msg = msg_update("loop3", "loop", create_loop_object("name", "red"));
    testrun(msg);
    testrun(cb_event_update(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(OV_ERROR_CODE_AUTH == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    // login admin user
    msg = msg_login("user1", "user1");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check no permission error
    msg = msg_update("loop3", "loop", create_loop_object("name", "red"));
    testrun(msg);
    testrun(cb_event_update(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    // check verify error
    msg = create_loop_object("name", "red");
    ov_json_object_set(msg, OV_KEY_ID, ov_json_string("loop5"));
    ov_json_value *err = msg;
    msg = msg_update("loop3", "loop", err);
    testrun(cb_event_update(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(OV_ERROR_CODE_PROCESSING_ERROR ==
            ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_load() {

    ov_json_value *out = ov_event_api_message_create(OV_VOCS_DB_LOAD, NULL, 0);

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_load() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance =
        ov_vocs_db_persistance_create((ov_vocs_db_persistance_config){
            .loop = loop, .db = db, .path = "./build/test/ov_vocs/db"});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    // create db in filesystem
    testrun(ov_vocs_db_persistance_save(persistance));
    testrun(ov_vocs_db_persistance_load(persistance));

    // prep add some key

    ov_json_value *msg = NULL;

    // no user logged in
    msg = msg_load();
    testrun(!cb_event_load(app, 2, &params, msg))

        // login standard user
        msg = msg_login("user3", "user3");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check permission error
    msg = msg_load();
    testrun(msg);
    testrun(cb_event_load(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(OV_ERROR_CODE_AUTH == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    // login admin user
    msg = msg_login("user1", "user1");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check no permission error
    msg = msg_load();
    testrun(msg);
    testrun(cb_event_load(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_save() {

    ov_json_value *out = ov_event_api_message_create(OV_VOCS_DB_SAVE, NULL, 0);

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_save() {

    struct dummy_userdata userdata = {0};

    const char *dir = "./build/test/ov_vocs/db";

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance =
        ov_vocs_db_persistance_create((ov_vocs_db_persistance_config){
            .loop = loop, .db = db, .path = "./build/test/ov_vocs/db"});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    // remove dir in path
    ov_dir_tree_remove(dir);
    testrun(!ov_dir_access_to_path(dir));

    // prep add some key

    ov_json_value *msg = NULL;

    // no user logged in
    msg = msg_save();
    testrun(!cb_event_save(app, 2, &params, msg))

        // login standard user
        msg = msg_login("user3", "user3");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check permission error
    msg = msg_save();
    testrun(msg);
    testrun(cb_event_save(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(OV_ERROR_CODE_AUTH == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    // login admin user
    msg = msg_login("user1", "user1");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check no permission error
    msg = msg_save();
    testrun(msg);
    testrun(cb_event_save(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    testrun(ov_dir_access_to_path(dir));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_set_layout(const char *role, ov_json_value *layout) {

    ov_json_value *val = NULL;
    ov_json_value *out =
        ov_event_api_message_create(OV_VOCS_DB_SET_LAYOUT, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);

    if (role) {

        val = ov_json_string(role);
        ov_json_object_set(par, OV_KEY_ROLE, val);
    }

    if (layout) {

        ov_json_object_set(par, OV_KEY_LAYOUT, layout);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *create_layout() {

    ov_json_value *out = ov_json_object();

    ov_json_object_set(out, "loop11", ov_json_number(1));
    ov_json_object_set(out, "loop21", ov_json_number(2));
    ov_json_object_set(out, "loop31", ov_json_number(3));

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_set_layout() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance =
        ov_vocs_db_persistance_create((ov_vocs_db_persistance_config){
            .loop = loop, .db = db, .path = "./build/test/ov_vocs/db"});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    ov_json_value *msg = NULL;

    // no user logged in
    msg = msg_set_layout("role1", create_layout());
    testrun(!cb_event_set_layout(app, 2, &params, msg))

        // login standard user
        msg = msg_login("user3", "user3");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check permission error
    msg = msg_set_layout("role1", create_layout());
    testrun(msg);
    testrun(cb_event_set_layout(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(OV_ERROR_CODE_AUTH == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    // login admin user
    msg = msg_login("user1", "user1");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check no permission error
    msg = msg_set_layout("role1", create_layout());
    testrun(msg);
    testrun(cb_event_set_layout(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_get_layout(const char *role) {

    ov_json_value *val = NULL;
    ov_json_value *out =
        ov_event_api_message_create(OV_VOCS_DB_GET_LAYOUT, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);

    if (role) {

        val = ov_json_string(role);
        ov_json_object_set(par, OV_KEY_ROLE, val);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_get_layout() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance =
        ov_vocs_db_persistance_create((ov_vocs_db_persistance_config){
            .loop = loop, .db = db, .path = "./build/test/ov_vocs/db"});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    ov_json_value *msg = NULL;

    // no user logged in
    msg = msg_get_layout("role1");
    testrun(!cb_event_set_layout(app, 2, &params, msg))

        // login standard user
        msg = msg_login("user3", "user3");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check no permission error with standard user
    msg = msg_get_layout("role1");
    testrun(msg);
    testrun(cb_event_get_layout(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    // login admin user
    msg = msg_login("user1", "user1");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check no permission error
    msg = msg_set_layout("role1", create_layout());
    testrun(msg);
    testrun(cb_event_set_layout(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    msg = msg_get_layout("role1");
    testrun(msg);
    testrun(cb_event_get_layout(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_add_domain_admin(const char *domain,
                                           const char *user) {

    ov_json_value *val = NULL;
    ov_json_value *out =
        ov_event_api_message_create(OV_VOCS_DB_ADD_DOMAIN_ADMIN, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);

    if (domain) {

        val = ov_json_string(domain);
        ov_json_object_set(par, OV_KEY_DOMAIN, val);
    }

    if (user) {

        val = ov_json_string(user);
        ov_json_object_set(par, OV_KEY_USER, val);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_add_domain_admin() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance =
        ov_vocs_db_persistance_create((ov_vocs_db_persistance_config){
            .loop = loop, .db = db, .path = "./build/test/ov_vocs/db"});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    ov_json_value *msg = NULL;

    // no user logged in
    msg = msg_add_domain_admin("localhost", "user2");
    testrun(!cb_event_add_domain_admin(app, 2, &params, msg))

        // login standard user
        msg = msg_login("user3", "user3");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check permission error with standard user
    msg = msg_add_domain_admin("localhost", "user2");
    testrun(msg);
    testrun(cb_event_add_domain_admin(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(OV_ERROR_CODE_AUTH == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    // login project admin user
    msg = msg_login("user2", "user2");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check permission error with project admin user
    msg = msg_add_domain_admin("localhost", "user2");
    testrun(msg);
    testrun(cb_event_add_domain_admin(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(OV_ERROR_CODE_AUTH == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    // login admin user
    msg = msg_login("user1", "user1");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    testrun(!ov_vocs_db_authorize_domain_admin(db, "user2", "localhost"));

    // check no permission error
    msg = msg_add_domain_admin("localhost", "user2");
    testrun(msg);
    testrun(cb_event_add_domain_admin(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    testrun(ov_vocs_db_authorize_domain_admin(db, "user2", "localhost"));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_add_project_admin(const char *project,
                                            const char *user) {

    ov_json_value *val = NULL;
    ov_json_value *out =
        ov_event_api_message_create(OV_VOCS_DB_ADD_DOMAIN_ADMIN, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);

    if (project) {

        val = ov_json_string(project);
        ov_json_object_set(par, OV_KEY_PROJECT, val);
    }

    if (user) {

        val = ov_json_string(user);
        ov_json_object_set(par, OV_KEY_USER, val);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

int check_cb_event_add_project_admin() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    ov_vocs_db_persistance *persistance =
        ov_vocs_db_persistance_create((ov_vocs_db_persistance_config){
            .loop = loop, .db = db, .path = "./build/test/ov_vocs/db"});
    testrun(persistance);

    ov_vocs_db_app *app = ov_vocs_db_app_create(
        (ov_vocs_db_app_config){.loop = loop,
                                .db = db,
                                .persistance = persistance,
                                .env.userdata = &userdata,
                                .env.send = dummy_send,
                                .env.close = dummy_close});

    testrun(app);

    ov_event_parameter params = (ov_event_parameter){
        .send.instance = &userdata, .send.send = dummy_send};

    ov_json_value *msg = NULL;

    // no user logged in
    msg = msg_add_project_admin("project@localhost", "user3");
    testrun(!cb_event_add_project_admin(app, 2, &params, msg))

        // login standard user
        msg = msg_login("user3", "user3");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    // check permission error with standard user
    msg = msg_add_project_admin("project@localhost", "user3");
    testrun(msg);
    testrun(cb_event_add_project_admin(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(OV_ERROR_CODE_AUTH == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    // login project admin user
    msg = msg_login("user2", "user2");
    testrun(msg);
    testrun(cb_event_login(app, 1, &params, msg));
    testrun(1 == ov_dict_count(app->connections));

    testrun(
        !ov_vocs_db_authorize_project_admin(db, "user3", "project@localhost"));

    // check no permission error with project admin user
    msg = msg_add_project_admin("project@localhost", "user3");
    testrun(msg);
    testrun(cb_event_add_project_admin(app, 1, &params, msg));
    testrun(1 == userdata.socket);
    // ov_json_value_dump(stdout, userdata.value);
    testrun(0 == ov_event_api_get_error_code(userdata.value));
    testrun(dummy_userdata_clear(&userdata));

    testrun(
        ov_vocs_db_authorize_project_admin(db, "user3", "project@localhost"));

    testrun(NULL == ov_vocs_db_app_free(app));
    testrun(NULL == ov_vocs_db_persistance_free(persistance));
    testrun(NULL == ov_vocs_db_free(db));
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
    /*
    testrun_test(test_ov_vocs_db_app_create);
    testrun_test(test_ov_vocs_db_app_free);

    testrun_test(check_connection_create);
    testrun_test(check_authorize_connection_for_id);

    testrun_test(check_cb_event_login);
    testrun_test(check_cb_event_logout);

    testrun_test(check_cb_event_update_password);
    testrun_test(check_cb_event_get_admin_domains);
    testrun_test(check_cb_event_get_admin_projects);
    testrun_test(check_cb_event_check_id_exists);

    testrun_test(check_cb_event_get);
    testrun_test(check_cb_event_delete);
    */

    testrun_test(check_cb_event_create);
    /*
    testrun_test(check_cb_event_verify);
    testrun_test(check_cb_event_update);

    testrun_test(check_cb_event_get_key);
    testrun_test(check_cb_event_update_key);
    testrun_test(check_cb_event_delete_key);

    testrun_test(check_cb_event_load);
    testrun_test(check_cb_event_save);

    testrun_test(check_cb_event_set_layout);
    testrun_test(check_cb_event_get_layout);

    testrun_test(check_cb_event_add_domain_admin);
    testrun_test(check_cb_event_add_project_admin);
*/
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
