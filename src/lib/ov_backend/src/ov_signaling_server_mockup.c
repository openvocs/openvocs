/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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

#include "../include/ov_signaling_server_mockup.h"
#include <ov_base/ov_dict.h>
#include <ov_base/ov_parser_json.h>
#include <ov_base/ov_thread_lock.h>
#include <ov_base/ov_utils.h>
#include <pthread.h>

#define MAGIC_BYTES 0x2e55ff1b

typedef struct {

    uint32_t magic_bytes;

    pthread_t thread_id;
    bool running;

    int client_socket;

    void *userdata;

    ov_dict *event_trackers;
    ov_thread_lock lock;

    ov_app *(*old_free)(ov_app *);

} mockup;

/*----------------------------------------------------------------------------*/

static mockup *as_mockup(void *vptr) {

    if (0 == vptr) return 0;

    mockup *m = vptr;

    if (MAGIC_BYTES != m->magic_bytes) return 0;

    return m;
}

/*----------------------------------------------------------------------------*/

static mockup *get_mockup_from_app(ov_app *app) {

    return as_mockup(ov_signaling_app_get_userdata(app));
}

/*----------------------------------------------------------------------------*/

static void close_cb(ov_app *app,
                     int socket,
                     const char *uuid,
                     void *userdata) {

    UNUSED(app);
    UNUSED(socket);
    UNUSED(uuid);
    UNUSED(userdata);

    mockup *m = get_mockup_from_app(app);

    if (0 == m) {

        ov_log_error("No/wrong app supplied");
        return;
    }

    if (m->client_socket != socket) {

        ov_log_error("Close: Sockets do not match");
        return;
    }

    m->client_socket = -1;
}

/*----------------------------------------------------------------------------*/

static bool connected_cb(ov_app *app,
                         int server_socket,
                         int accepted_socket,
                         void *userdata) {

    UNUSED(server_socket);
    UNUSED(accepted_socket);
    UNUSED(userdata);

    fprintf(stderr, "connected called \n");

    mockup *m = get_mockup_from_app(app);

    if (0 == m) {

        ov_log_error("No/wrong app supplied");
        return false;
    }

    if (0 < m->client_socket) {

        ov_log_warning("Connected: ALready connected");
        return false;
    }

    m->client_socket = accepted_socket;

    return true;
}

/*----------------------------------------------------------------------------*/

static ov_app *impl_free(ov_app *self) {

    fprintf(stderr, "Close called\n");

    mockup *m = get_mockup_from_app(self);

    if (0 == m) return 0;

    ov_signaling_app_set_userdata(self, 0);

    self = m->old_free(self);

    ov_thread_lock_clear(&m->lock);

    m->event_trackers = ov_dict_free(m->event_trackers);
    OV_ASSERT(0 == m->event_trackers);

    free(m);

    return self;
}

/*----------------------------------------------------------------------------*/

static ov_dict *event_tracker_dict_create();

/*----------------------------------------------------------------------------*/

ov_app *ov_signaling_server_mockup_create(ov_event_loop *loop,
                                          ov_socket_configuration server_socket,
                                          uint32_t timeout_secs) {

    ov_app *app = 0;
    mockup *m = 0;

    if (0 == loop) {
        ov_log_error("Require event loop");
        goto error;
    }

    m = calloc(1, sizeof(mockup));
    m->magic_bytes = MAGIC_BYTES;

    m->running = false;

    ov_thread_lock_init(&m->lock, timeout_secs * 1000 * 1000);

    m->event_trackers = event_tracker_dict_create();

    ov_app_config config = {.loop = loop, .name = "signaling_server_mockup"};

    app = ov_signaling_app_create(config);
    OV_ASSERT(0 != app);

    m->old_free = app->free;
    app->free = impl_free;

    ov_signaling_app_set_userdata(app, m);

    ov_app_socket_config app_socket_config = {
        .socket_config = server_socket,

        .parser.create = ov_parser_json_create_default,
        .parser.config = (ov_parser_config){0},

        .callback.io = ov_signaling_app_io_signaling,
        .callback.accepted = connected_cb,
        .callback.close = close_cb,

    };

    if (!ov_app_open_socket(app, app_socket_config, 0, 0)) {

        goto error;
    }

    return app;

error:

    if (0 != app) {
        app = app->free(app);
    }

    OV_ASSERT(0 == app);

    return 0;
}

/*****************************************************************************
                                    Userdata
 ****************************************************************************/

void *ov_signaling_server_mockup_set_userdata(ov_app *self, void *userdata) {

    mockup *m = get_mockup_from_app(self);

    if (0 == m) return 0;

    void *old = m->userdata;
    m->userdata = userdata;

    return old;
}

/*----------------------------------------------------------------------------*/

void *ov_signaling_server_mockup_get_userdata(ov_app *self) {

    mockup *m = get_mockup_from_app(self);

    if (0 == m) return 0;

    return m->userdata;
}

/*****************************************************************************
                                   Run Mockup
 ****************************************************************************/

static void *run_mockup(void *arg) {

    ov_app *app = arg;

    OV_ASSERT(0 != app);

    ov_event_loop *loop = app->config.loop;

    OV_ASSERT(0 != loop);

    loop->run(loop, UINT64_MAX);

    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_signaling_server_mockup_run(ov_app *self) {

    mockup *m = get_mockup_from_app(self);

    if (0 == m) {

        ov_log_error("No minion mockup to run");
        goto error;
    }

    pthread_create(&m->thread_id, 0, run_mockup, self);

    m->running = true;

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_signaling_server_mockup_stop(ov_app *self) {

    mockup *m = get_mockup_from_app(self);

    if (0 == m) {

        ov_log_error("Require minion mockup, got a different app");
        goto error;
    }

    if (!m->running) {

        goto finish;
    }

    ov_event_loop *loop = self->config.loop;

    if (0 == loop) return false;

    OV_ASSERT(0 != loop);

    loop->stop(loop);

    void *retval = 0;

    if (0 != pthread_join(m->thread_id, &retval)) {

        testrun_log_error("Could not stop loop thread");
        goto error;
    }

    m->running = false;

finish:

    return true;

error:

    return false;
}

/*****************************************************************************
                               SENDING TO MOCKUP
 ****************************************************************************/

bool ov_signaling_server_mockup_send(ov_app *self, ov_json_value *msg) {

    mockup *m = get_mockup_from_app(self);

    if ((0 == m) || (0 == msg)) {

        ov_log_error("Got 0 pointer");
        goto error;
    }

    if (0 >= m->client_socket) {

        ov_log_error("Not connected");
        goto error;
    }

    return ov_app_send(self, m->client_socket, 0, (void *)msg);

error:

    return false;
}

/*****************************************************************************
                                 EVENT TRACKING
 ****************************************************************************/

typedef struct {
    bool called;
    ov_json_value *parameters;

} event_tracker;

/*----------------------------------------------------------------------------*/

static void *event_tracker_free(void *vptr) {

    if (0 == vptr) return 0;

    event_tracker *et = vptr;

    if (0 != et->parameters) {
        et->parameters = ov_json_value_free(et->parameters);
    }

    OV_ASSERT(0 == et->parameters);

    free(et);

    return 0;
}

/*----------------------------------------------------------------------------*/

static ov_dict *event_tracker_dict_create() {

    ov_dict_config cfg = ov_dict_string_key_config(30);

    cfg.value.data_function.free = event_tracker_free;

    return ov_dict_create(cfg);
}

/*----------------------------------------------------------------------------*/

static bool get_lock(ov_thread_lock *lock) {

    if (0 == lock) return false;

    bool success_p = ov_thread_lock_try_lock(lock);

    if (!success_p) return false;

    success_p = ov_thread_lock_wait(lock);

    // Mutex should still be locked ?

    return true;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *track_event_cb(ov_app *app,
                                     const char *name,
                                     const ov_json_value *msg,
                                     int socket,
                                     const ov_socket_data *remote) {

    UNUSED(app);
    UNUSED(socket);
    UNUSED(remote);

    fprintf(stderr, "Received %s\n", name);

    bool locked = 0;

    mockup *m = get_mockup_from_app(app);

    if (0 == m) {

        ov_log_error("Require signaling server app, got 0 pointer");
        goto error;
    }

    locked = ov_thread_lock_try_lock(&m->lock);

    if (!locked) goto error;

    event_tracker *et = ov_dict_get(m->event_trackers, name);

    if (0 == et) {

        ov_log_error("No event tracker for %s", name);
        goto error;
    }

    et->called = true;

    ov_json_value *params = ov_event_api_get_parameter(msg);

    OV_ASSERT(0 != params);

    if (0 != et->parameters) {

        et->parameters = ov_json_value_free(et->parameters);
    }

    ov_json_value_copy((void **)&et->parameters, params);

    ov_thread_lock_unlock(&m->lock);

    ov_thread_lock_notify(&m->lock);

    return 0;

error:

    if (locked) {
        ov_thread_lock_unlock(&m->lock);
    }

    return (ov_json_value *)FAILURE_CLOSE_SOCKET;
}

/*----------------------------------------------------------------------------*/

void ov_signaling_server_mockup_track_event(ov_app *self,
                                            char const *event_name) {

    bool locked = false;

    event_tracker *old_et = 0;

    mockup *m = get_mockup_from_app(self);

    if (0 == m) goto finish;

    locked = ov_thread_lock_try_lock(&m->lock);

    if (!locked) goto finish;

    event_tracker *et = calloc(1, sizeof(event_tracker));

    char *event_name_copy = strdup(event_name);

    bool success_p =
        ov_dict_set(m->event_trackers, event_name_copy, et, (void **)&old_et);

    OV_ASSERT(success_p);

    et = 0;

    success_p =
        ov_signaling_app_register_command(self, event_name, 0, track_event_cb);

    OV_ASSERT(success_p);

    ov_thread_lock_unlock(&m->lock);

    locked = false;

    if (0 == old_et) {

        goto finish;
    }

    if (0 != old_et->parameters) {

        old_et->parameters = ov_json_value_free(old_et->parameters);
    }

    OV_ASSERT(0 == old_et->parameters);

    free(old_et);

    old_et = 0;

finish:

    OV_ASSERT(0 == old_et);
}

/*----------------------------------------------------------------------------*/

bool ov_signaling_server_mockup_event_received(ov_app *self,
                                               char const *event_name) {

    bool called = false;

    mockup *m = get_mockup_from_app(self);

    if (0 == m) return false;

    bool locked = get_lock(&m->lock);

    if (!locked) return false;

    event_tracker *t = ov_dict_get(m->event_trackers, event_name);

    if (0 == t) goto error;

    called = t->called;
    t->called = false;

error:

    if (locked) ov_thread_lock_unlock(&m->lock);

    return called;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_signaling_server_mockup_event_parameters(
    ov_app *self, char const *event_name) {

    ov_json_value *params = 0;

    mockup *m = get_mockup_from_app(self);

    if (0 == m) return 0;

    bool locked = get_lock(&m->lock);

    if (!locked) return false;

    event_tracker *t = ov_dict_get(m->event_trackers, event_name);

    if (0 == t) goto error;

    params = t->parameters;
    t->parameters = 0;

    ov_thread_lock_unlock(&m->lock);

    return params;

error:

    if (locked) ov_thread_lock_unlock(&m->lock);

    return 0;
}

/*----------------------------------------------------------------------------*/
