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
        @file           ov_vocs_recorder.c
        @author         Markus

        @date           2024-01-09


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_recorder.h"
#include "../include/ov_vocs_record.h"

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_error_codes.h>
#include <ov_base/ov_event_keys.h>
#include <ov_base/ov_string.h>

#include <ov_core/ov_callback.h>
#include <ov_core/ov_event_connection.h>
#include <ov_core/ov_event_engine.h>
#include <ov_core/ov_event_socket.h>
#include <ov_core/ov_notify.h>
#include <ov_core/ov_recorder_events.h>
#include <ov_core/ov_recording.h>

#include <ov_database/ov_database_events.h>

/*----------------------------------------------------------------------------*/

#define OV_VOCS_RECORDER_MAGIC_BYTES 0x5ec0
#define OV_RECORDER_STARTUP_DELAY                                              \
    15000000 // 15 sekunden
             //
#define MAX_RESULTS_PER_QUERY 50

/*----------------------------------------------------------------------------*/

struct ov_vocs_recorder {

    uint16_t magic_bytes;
    ov_vocs_recorder_config config;

    int socket;

    struct {

        ov_event_socket *socket;
        ov_event_engine *engine;

    } event;

    struct {

        uint32_t startup_delay;

    } timer;

    ov_dict *connections;
    ov_dict *recorder;
    ov_dict *recordings;

    ov_database *db;

    ov_callback_registry *callbacks;
};

/*----------------------------------------------------------------------------*/

static ov_database *get_database_mut(ov_vocs_recorder *self) {

    if (ov_ptr_valid(self, "Cannot get database - no recorder_manager")) {
        return self->db;
    } else {
        return 0;
    }
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #EVENT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

bool event_recorder_register(void *userdata, const int socket,
                             const ov_event_parameter *params,
                             ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_vocs_recorder *self = ov_vocs_recorder_cast(userdata);
    if (!self || !socket || !params || !input)
        goto error;

    const char *uuid = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_UUID));

    if (!uuid) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_event_connection *conn = ov_event_connection_create(
        (ov_event_connection_config){.socket = socket, .params = *params});

    if (!conn) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_PROCESSING_ERROR,
            OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    intptr_t key = socket;

    if (!ov_dict_set(self->recorder, (void *)key, conn, NULL)) {

        conn = ov_event_connection_free(conn);

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_PROCESSING_ERROR,
            OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    ov_event_connection_set(conn, OV_KEY_UUID, uuid);
    out = ov_event_api_create_success_response(input);

    ov_log_debug("Recorder %s connected", uuid);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);

    ov_json_value_free(input);
    return true;
error:
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

bool event_recorder_unregister(void *userdata, const int socket,
                               const ov_event_parameter *params,
                               ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_vocs_recorder *self = ov_vocs_recorder_cast(userdata);
    if (!self || !socket || !params || !input)
        goto error;

    const char *uuid = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_UUID));

    if (!uuid) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    intptr_t key = socket;
    ov_dict_del(self->recorder, (void *)key);

    out = ov_event_api_create_success_response(input);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);

    ov_json_value_free(input);
    return true;
error:
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool start_record(void *userdata, const int fh,
                         const ov_event_parameter *parameter,
                         ov_json_value *input) {

    ov_recorder_response_start resp = {0};

    uint64_t code = 0;
    const char *desc = 0;

    ov_vocs_recorder *self = ov_vocs_recorder_cast(userdata);
    if (!self || fh < 0 || !parameter || !input)
        goto error;

    ov_json_value *res = ov_event_api_get_response(input);
    if (!res)
        goto error;

    const char *uuid = ov_event_api_get_uuid(input);

    ov_event_api_get_error_parameter(input, &code, &desc);

    ov_callback cb = ov_callback_registry_unregister(self->callbacks, uuid);
    void (*function)(void *, int, const char *, const char *, ov_result) =
        cb.function;

    ov_event_api_get_error_parameter(input, &code, &desc);

    if (!ov_recorder_response_start_from_json(res, &resp))
        goto error;

    const char *loop = ov_json_string_get((ov_json_get(
        input, "/" OV_KEY_REQUEST "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP)));

    if (!loop)
        goto error;

    ov_vocs_record *record = ov_dict_get(self->recordings, loop);
    if (!record) {
        // loop recoding not enabled
        goto done;
    }

    switch (code) {

    case OV_ERROR_NOERROR:

        ov_vocs_record_set_active(record, resp.id, loop, resp.filename, fh);

        ov_log_debug("activated recording of loop %s", loop);

        function(cb.userdata, cb.socket, uuid, loop,
                 (ov_result){.error_code = OV_ERROR_NOERROR, .message = NULL});

        break;

    default:

        ov_log_error("Start recording error %i|%s", code, desc);

        function(cb.userdata, cb.socket, uuid, loop,
                 (ov_result){.error_code = code, .message = (char *)desc});
    }

done:
    ov_recorder_response_start_clear(&resp);
    ov_json_value_free(input);
    return true;
error:
    ov_json_value_free(input);
    ov_recorder_response_start_clear(&resp);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool stop_record(void *userdata, const int socket,
                        const ov_event_parameter *parameter,
                        ov_json_value *input) {

    ov_json_value *t = NULL;

    uint64_t code = 0;
    const char *desc = 0;

    ov_vocs_recorder *self = ov_vocs_recorder_cast(userdata);
    if (!self || socket < 0 || !parameter || !input)
        goto error;

    const char *uuid = ov_event_api_get_uuid(input);

    ov_event_api_get_error_parameter(input, &code, &desc);

    ov_callback cb = ov_callback_registry_unregister(self->callbacks, uuid);
    void (*function)(void *, int, const char *, const char *, ov_result) =
        cb.function;

    ov_event_connection *conn =
        ov_dict_get(self->recorder, (void *)(intptr_t)socket);

    ov_json_value *res = ov_event_api_get_response(input);
    if (!res)
        goto error;

    const char *loop = ov_json_string_get((ov_json_get(
        input, "/" OV_KEY_REQUEST "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP)));

    if (!loop)
        goto error;

    ov_vocs_record *record = ov_dict_get(self->recordings, loop);
    if (!record)
        goto unblock;

    switch (code) {

    case OV_ERROR_NOERROR:
    case OV_ERROR_NOT_RECORDING:

        // NO - The recorder will send a notification event when it
        // wrote the new recording - here we don't know the file name yet.
        // Moreover, if we register the recording here and when receiving
        // the notification, we end up with 2 entries in the database.
        // And we need to use the notification in case of automatically
        // starting/stopping recordings (like in case of rolling recordings
        // or using a VAD
        // ov_db_recordings_add(get_database_mut(self),
        //             record->active.id,
        //             loop,
        //             record->active.uri,
        //             record->active.started_at_epoch_secs,
        //             time(0));

        ov_vocs_record_reset_active(record);

        function(cb.userdata, cb.socket, uuid, loop,
                 (ov_result){.error_code = OV_ERROR_NOERROR, .message = NULL});

        break;

        break;

    default:

        function(cb.userdata, cb.socket, uuid, loop,
                 (ov_result){.error_code = code, .message = (char *)desc});

        self->config.callbacks.stop_record(
            self->config.callbacks.userdata, uuid,
            (ov_result){.error_code = code, .message = (char *)desc});

        goto done;
    }

    // we unblock the recorder
unblock:

    t = ov_json_true();
    ov_event_connection_set_json(conn, OV_KEY_EMPTY, t);
    t = ov_json_value_free(t);

    ov_log_debug("deactivated recording of loop %s", loop);

done:
    input = ov_json_value_free(input);
    return true;

error:
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static void handle_new_recording(ov_vocs_recorder *self, ov_recording record) {

    if (!self)
        goto error;

    ov_db_recordings_add(get_database_mut(self), record.id, record.loop,
                         record.uri, record.start_epoch_secs,
                         record.end_epoch_secs);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void handle_error(ov_vocs_recorder *self, ov_json_value *input) {

    UNUSED(self);

    char *str = ov_json_value_to_string(input);
    ov_log_error("Error response notify unhandled %s", str);
    str = ov_data_pointer_free(str);
    return;
}

/*----------------------------------------------------------------------------*/

static void handle_unexpected(ov_vocs_recorder *self, ov_json_value *input) {

    UNUSED(self);

    char *str = ov_json_value_to_string(input);
    ov_log_error("Error response notify unexpected %s", str);
    str = ov_data_pointer_free(str);
    return;
}

/*----------------------------------------------------------------------------*/

static bool cb_event_notify(void *userdata, const int socket,
                            const ov_event_parameter *parameter,
                            ov_json_value *input) {

    ov_vocs_recorder *self = ov_vocs_recorder_cast(userdata);
    if (!self || socket < 0 || !parameter || !input)
        goto error;

    ov_json_value *par = ov_event_api_get_parameter(input);
    if (!par)
        goto error;

    ov_notify_parameters params = {0};

    switch (ov_notify_parse(par, &params)) {

    case NOTIFY_NEW_RECORDING:

        handle_new_recording(self, params.recording);
        ov_recording_clear(&params.recording);
        break;

    case NOTIFY_INVALID:

        handle_error(self, input);
        break;

    default:

        handle_unexpected(self, input);
    };

    ov_json_value_free(input);
    return true;

error:
    ov_json_value_free(input);
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool check_config(ov_vocs_recorder_config *config) {

    if (!config)
        goto error;
    if (!config->loop)
        goto error;
    if (!config->socket.manager.host[0])
        goto error;

    if (0 == config->vad.zero_crossings_rate_threshold_hertz)
        config->vad.zero_crossings_rate_threshold_hertz = 5000;

    if (0 == config->limits.silence_cutoff_interval_msec)
        config->limits.silence_cutoff_interval_msec = 2000;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool register_events(ov_vocs_recorder *self) {

    if (!self)
        goto error;

    if (!ov_event_engine_register(self->event.engine, OV_KEY_REGISTER, self,
                                  event_recorder_register))
        goto error;

    if (!ov_event_engine_register(self->event.engine, OV_KEY_UNREGISTER, self,
                                  event_recorder_unregister))
        goto error;

    if (!ov_event_engine_register(self->event.engine, OV_EVENT_START_RECORD,
                                  self, start_record))
        goto error;

    if (!ov_event_engine_register(self->event.engine, OV_EVENT_STOP_RECORD,
                                  self, stop_record))
        goto error;

    if (!ov_event_engine_register(self->event.engine, OV_EVENT_NOTIFY, self,
                                  cb_event_notify))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

struct container_rec {

    ov_vocs_record *rec;
    int recorder;
};

/*----------------------------------------------------------------------------*/

static bool find_record(const void *key, void *val, void *data) {

    if (!key)
        return true;

    ov_vocs_record *record = (ov_vocs_record *)val;
    struct container_rec *rec = (struct container_rec *)data;

    if (!record || !rec)
        return false;

    if (record->active.recorder == rec->recorder)
        rec->rec = record;

    return true;
}

/*----------------------------------------------------------------------------*/

static ov_vocs_record *find_recording_of_recorder(ov_vocs_recorder *self,
                                                  int socket) {

    if (!self || !socket)
        goto error;

    struct container_rec rec = (struct container_rec){.recorder = socket};

    if (!ov_dict_for_each(self->recordings, &rec, find_record))
        goto error;

    return rec.rec;

error:
    return NULL;
}

struct container1 {

    bool found_empty;
    ov_event_connection *conn;
};

/*----------------------------------------------------------------------------*/

static bool find_empty_recorder_entry(const void *key, void *val, void *data) {

    if (!key)
        return true;
    struct container1 *container = (struct container1 *)data;
    ov_event_connection *conn = (ov_event_connection *)val;

    if (container->found_empty)
        return true;

    const ov_json_value *empty =
        ov_event_connection_get_json(conn, OV_KEY_EMPTY);
    if (!empty || ov_json_is_true(empty)) {

        container->found_empty = true;
        container->conn = conn;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static ov_event_connection *find_empty_recorder(ov_vocs_recorder *self) {

    struct container1 container = (struct container1){

        .found_empty = false, .conn = NULL};

    if (!ov_dict_for_each(self->recorder, &container,
                          find_empty_recorder_entry))
        goto error;

    if (!container.found_empty) {
        ov_log_error("No recorder available for recording.");
        goto error;
    }
error:
    return container.conn;
}

/*----------------------------------------------------------------------------*/

static bool request_new_recording(ov_vocs_recorder *self, const char *loop) {

    if (!self || !loop)
        goto error;

    ov_socket_configuration socket =
        ov_vocs_db_get_multicast_group(self->config.vocs_db, loop);

    ov_recorder_event_start event = (ov_recorder_event_start){
        .loop = (char *)loop,
        .mc_ip = socket.host,
        .mc_port = socket.port,
        .silence_cutoff_interval_msecs =
            self->config.limits.silence_cutoff_interval_msec,
        .vad = self->config.vad};

    ov_event_connection *conn = find_empty_recorder(self);
    if (!conn) {

        ov_log_error("No recorder avaliable for loop %s", loop);
        goto error;
    }

    // we block the recorder here
    ov_json_value *f = ov_json_false();
    ov_event_connection_set_json(conn, OV_KEY_EMPTY, f);
    f = ov_json_value_free(f);

    ov_vocs_record_config conf = {0};
    strncpy(conf.loopname, loop, OV_MC_LOOP_NAME_MAX);

    ov_vocs_record *rec = ov_vocs_record_create(conf);

    if (!rec)
        goto error;

    if (!ov_dict_set(self->recordings, ov_string_dup(loop), rec, NULL))
        goto error;

    ov_json_value *out =
        ov_event_api_message_create(OV_EVENT_START_RECORD, 0, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);
    if (!ov_recorder_event_start_to_json(par, &event)) {
        out = ov_json_value_free(out);
        goto error;
    }

    rec->active.recorder = ov_event_connection_get_socket(conn);

    char *str = ov_json_value_to_string(out);
    ov_log_debug("Activated recording %s", str);
    str = ov_data_pointer_free(str);

    ov_event_connection_send(conn, out);
    out = ov_json_value_free(out);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void cb_recorder_socket_close(void *userdata, int socket) {

    ov_vocs_recorder *self = ov_vocs_recorder_cast(userdata);
    if (!self)
        return;

    ov_vocs_record *rec = find_recording_of_recorder(self, socket);
    if (rec) {

        rec->active.running = false;
        rec->active.recorder = 0;

        request_new_recording(self, rec->config.loopname);
    }

    ov_dict_del(self->recorder, (void *)(intptr_t)socket);
    return;
}

/*----------------------------------------------------------------------------*/

static bool start_recording(void *item, void *data) {

    ov_json_value *value = ov_json_value_cast(item);
    ov_vocs_recorder *self = ov_vocs_recorder_cast(data);
    if (!value || !self)
        goto error;

    const char *name =
        ov_json_string_get(ov_json_object_get(value, OV_KEY_LOOP));

    return request_new_recording(self, name);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool start_all_recordings(uint32_t id, void *userdata) {

    UNUSED(id);

    ov_json_value *loops = NULL;

    ov_vocs_recorder *self = ov_vocs_recorder_cast(userdata);
    if (!self)
        goto error;

    self->timer.startup_delay = OV_TIMER_INVALID;

    loops = ov_vocs_db_get_recorded_loops(self->config.vocs_db);
    if (!loops) {
        ov_log_error("No recording to start.");
        goto done;
    }

    if (!ov_json_array_for_each(loops, self, start_recording)) {

        ov_log_error("Failed to start all recordings.");
    }

done:
    loops = ov_json_value_free(loops);
    return true;

error:
    ov_json_value_free(loops);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_vocs_recorder *ov_vocs_recorder_create(ov_vocs_recorder_config config) {

    ov_vocs_recorder *self = NULL;

    if (!check_config(&config))
        goto error;

    self = calloc(1, sizeof(ov_vocs_recorder));
    if (!self)
        goto error;

    self->magic_bytes = OV_VOCS_RECORDER_MAGIC_BYTES;
    self->config = config;

    ov_dict_config d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = ov_event_connection_free_void;

    self->recorder = ov_dict_create(d_config);
    if (!self->recorder)
        goto error;

    d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = ov_vocs_record_free_void;
    self->recordings = ov_dict_create(d_config);
    if (!self->recordings)
        goto error;

    self->event.engine = ov_event_engine_create();

    self->event.socket = ov_event_socket_create(
        (ov_event_socket_config){.loop = config.loop,
                                 .engine = self->event.engine,
                                 .callback.userdata = self,
                                 .callback.close = cb_recorder_socket_close});

    if (!self->event.socket)
        goto error;

    self->socket = ov_event_socket_create_listener(
        self->event.socket, (ov_event_socket_server_config){
                                .socket = config.socket.manager,

                            });

    if (-1 == self->socket) {

        ov_log_error("Failed to create socket %s:%i",
                     config.socket.manager.host, config.socket.manager.port);

        goto error;

    } else {

        ov_log_debug("created socket %i %s:%i", self->socket,
                     config.socket.manager.host, config.socket.manager.port);
    }

    if (!register_events(self))
        goto error;

    ov_database_info dbcfg = {
        .type = OV_OR_DEFAULT(config.db.type, OV_DB_SQLITE),
        .dbname = OV_OR_DEFAULT(config.db.dbname, "ov_recorder_events"),
        .host = OV_OR_DEFAULT(config.db.host, "localhost"),
        .port = OV_OR_DEFAULT(config.db.port, 5432),
        .password = OV_OR_DEFAULT(config.db.password, "ov"),
        .user = OV_OR_DEFAULT(config.db.user, "ov"),
    };

    self->db = ov_database_connect_singleton(dbcfg);

    if (0 == self->db) {
        ov_log_error("Could not connect to database at %s:%s:%" PRIu16 "/%"
                     "s",
                     dbcfg.type, dbcfg.host, dbcfg.port, dbcfg.dbname);
    }

    if (!ov_db_prepare(self->db)) {
        ov_log_error("Could not initialize event database");
    }

    // add startup delay for recordings to let recorders connect before
    self->timer.startup_delay =
        ov_event_loop_timer_set(self->config.loop, OV_RECORDER_STARTUP_DELAY,
                                self, start_all_recordings);

    self->callbacks = ov_callback_registry_create((ov_callback_registry_config){
        .loop = self->config.loop,
        .timeout_usec = self->config.timeout.response_usec});

    if (!self->callbacks)
        goto error;

    return self;
error:
    ov_vocs_recorder_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_recorder *ov_vocs_recorder_free(ov_vocs_recorder *self) {

    if (!ov_vocs_recorder_cast(self))
        goto error;

    self->event.socket = ov_event_socket_free(self->event.socket);
    self->event.engine = ov_event_engine_free(self->event.engine);

    self->connections = ov_dict_free(self->connections);
    self->recorder = ov_dict_free(self->recorder);
    self->recordings = ov_dict_free(self->recordings);
    self->callbacks = ov_callback_registry_free(self->callbacks);

    self = ov_data_pointer_free(self);

error:
    return self;
}

/*----------------------------------------------------------------------------*/

ov_vocs_recorder *ov_vocs_recorder_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_VOCS_RECORDER_MAGIC_BYTES)
        return NULL;

    return (ov_vocs_recorder *)data;
}

/*----------------------------------------------------------------------------*/

static void cb_socket_close(void *userdata, int socket) {

    ov_vocs_recorder *self = ov_vocs_recorder_cast(userdata);
    if (!self || socket < 0)
        goto error;

    intptr_t key = socket;
    ov_dict_del(self->connections, (void *)key);

    ov_vocs_record *record = find_recording_of_recorder(self, socket);

    if (record) {
        ov_vocs_record_reset_active(record);
    }

error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool cb_client_process(void *userdata, const int socket,
                              const ov_event_parameter *params,
                              ov_json_value *input) {

    ov_vocs_recorder *self = ov_vocs_recorder_cast(userdata);
    if (!self || (0 > socket) || !input) {
        goto error;
    }

    if (!ov_event_engine_push(self->event.engine, socket, *params, input))
        goto error;

    return true;

error:
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_event_io_config ov_vocs_recorder_io_uri_config(ov_vocs_recorder *self) {

    return (ov_event_io_config){.name = OV_KEY_RECORDER,
                                .userdata = self,
                                .callback.close = cb_socket_close,
                                .callback.process = cb_client_process};
}

/*----------------------------------------------------------------------------*/

ov_vocs_recorder_config
ov_vocs_recorder_config_from_json(const ov_json_value *value) {

    ov_vocs_recorder_config config = {0};

    if (!value)
        goto error;
    const ov_json_value *conf = ov_json_get(value, "/" OV_KEY_RECORDER);
    if (!conf)
        conf = value;

    config.socket.manager = ov_socket_configuration_from_json(
        ov_json_get(conf, "/" OV_KEY_SOCKET "/" OV_KEY_MANAGER),
        (ov_socket_configuration){0});

    config.db = ov_database_info_from_json(ov_json_get(conf, "/" OV_KEY_DB));

    config.vad = ov_vad_config_from_json(conf);

    config.limits.silence_cutoff_interval_msec =
        ov_json_number_get(ov_json_get(conf, "/vad/silence_interval_msec"));

    return config;

error:
    return (ov_vocs_recorder_config){0};
}

/*----------------------------------------------------------------------------*/

void ov_vocs_recorder_join_loop(ov_vocs_recorder *self, const char *user,
                                const char *role, const char *loop) {

    if (!self || !user || !role || !loop)
        goto error;

    time_t now;
    time(&now);

    ov_db_events_add_participation_state(get_database_mut(self), user, role,
                                         loop, OV_PARTICIPATION_STATE_RECV,
                                         now);

error:
    return;
}

/*----------------------------------------------------------------------------*/

void ov_vocs_recorder_leave_loop(ov_vocs_recorder *self, const char *user,
                                 const char *role, const char *loop) {

    if (!self || !user || !role || !loop)
        goto error;

    time_t now;
    time(&now);

    ov_db_events_add_participation_state(self->db, user, role, loop,
                                         OV_PARTICIPATION_STATE_RECV_OFF, now);

error:
    return;
}

/*----------------------------------------------------------------------------*/

void ov_vocs_recorder_talk_on_loop(ov_vocs_recorder *self, const char *user,
                                   const char *role, const char *loop) {

    if (!self || !user || !role || !loop)
        goto error;

    time_t now;
    time(&now);

    ov_db_events_add_participation_state(self->db, user, role, loop,
                                         OV_PARTICIPATION_STATE_SEND, now);

error:
    return;
}

/*----------------------------------------------------------------------------*/

void ov_vocs_recorder_talk_off_loop(ov_vocs_recorder *self, const char *user,
                                    const char *role, const char *loop) {

    if (!self || !user || !role || !loop)
        goto error;

    time_t now;
    time(&now);

    ov_db_events_add_participation_state(self->db, user, role, loop,
                                         OV_PARTICIPATION_STATE_SEND_OFF, now);

error:
    return;
}

/*----------------------------------------------------------------------------*/

void ov_vocs_recorder_ptt(ov_vocs_recorder *self, const char *user,
                          const char *role, const char *loop, bool off) {

    if (!self || !user || !role || !loop)
        goto error;

    time_t now;
    time(&now);

    ov_participation_state state = OV_PARTICIPATION_STATE_PTT_OFF;
    if (!off)
        state = OV_PARTICIPATION_STATE_PTT;

    ov_db_events_add_participation_state(self->db, user, role, loop, state,
                                         now);

error:
    return;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_recorder_start_recording(ov_vocs_recorder *self, const char *loop,
                                      const char *uuid) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;

    ov_vocs_record *record = NULL;

    if (!self || !loop)
        goto error;

    record = ov_dict_get(self->recordings, loop);
    if (!record) {

        ov_vocs_record_config conf = {0};
        strncpy(conf.loopname, loop, OV_MC_LOOP_NAME_MAX);

        record = ov_vocs_record_create(conf);

        if (!ov_dict_set(self->recordings, ov_string_dup(loop), record, NULL))
            goto error;
    }

    if (record->active.running) {

        self->config.callbacks.start_record(
            self->config.callbacks.userdata, uuid,
            (ov_result){.error_code = OV_ERROR_CODE_ALREADY_SET,
                        .message = "Loop already recorded."});

        goto error;
    }

    ov_socket_configuration socket =
        ov_vocs_db_get_multicast_group(self->config.vocs_db, loop);

    if (0 == socket.host[0]) {

        self->config.callbacks.start_record(
            self->config.callbacks.userdata, uuid,
            (ov_result){.error_code = OV_ERROR_CODE_DESTINATION_UNKNOWN,
                        .message = "No multicast socket avaliable for loop"});

        goto error;
    }

    ov_recorder_event_start event = (ov_recorder_event_start){
        .loop = (char *)loop,
        .mc_ip = socket.host,
        .mc_port = socket.port,
        .silence_cutoff_interval_msecs =
            self->config.limits.silence_cutoff_interval_msec,
        .vad = self->config.vad};

    ov_event_connection *conn = find_empty_recorder(self);
    if (!conn) {

        self->config.callbacks.start_record(
            self->config.callbacks.userdata, uuid,
            (ov_result){.error_code = OV_ERROR_NO_RESOURCE,
                        .message = "No recorder avaliable for loop"});

        goto error;
    }

    // we block the recorder here
    ov_json_value *f = ov_json_false();
    ov_event_connection_set_json(conn, OV_KEY_EMPTY, f);
    f = ov_json_value_free(f);

    out = ov_event_api_message_create(OV_EVENT_START_RECORD, uuid, 0);
    par = ov_event_api_set_parameter(out);

    if (!ov_recorder_event_start_to_json(par, &event)) {
        out = ov_json_value_free(out);

        self->config.callbacks.start_record(
            self->config.callbacks.userdata, uuid,
            (ov_result){.error_code = OV_ERROR_CODE_PROCESSING_ERROR,
                        .message = OV_ERROR_DESC_PROCESSING_ERROR});

        goto error;
    }

    record->active.recorder = ov_event_connection_get_socket(conn);

    char *str = ov_json_value_to_string(out);
    ov_log_debug("Activated recording %s", str);
    str = ov_data_pointer_free(str);

    ov_event_connection_send(conn, out);
    out = ov_json_value_free(out);

    ov_vocs_db_set_recorded(self->config.vocs_db, loop, true);

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_recorder_stop_recording(ov_vocs_recorder *self, const char *loop,
                                     const char *uuid) {

    if (!self || !loop)
        goto error;

    ov_vocs_record *rec = ov_dict_get(self->recordings, loop);
    if (!rec) {

        self->config.callbacks.stop_record(
            self->config.callbacks.userdata, uuid,
            (ov_result){.error_code = OV_ERROR_NOT_RECORDING,
                        .message = OV_ERROR_DESC_NOT_RECORDING});

        goto error;
    }

    ov_recorder_event_stop stop_event = {0};

    ov_event_connection *conn =
        ov_dict_get(self->recorder, (void *)(intptr_t)rec->active.recorder);

    if (!conn) {

        if (rec->active.running)
            rec->active.running = false;

        self->config.callbacks.stop_record(
            self->config.callbacks.userdata, uuid,
            (ov_result){.error_code = OV_ERROR_NOT_RECORDING,
                        .message = "No recorder connected."});

        goto error;
    }

    memcpy(stop_event.id, rec->active.id, 36);

    ov_json_value *msg =
        ov_event_api_message_create(OV_EVENT_STOP_RECORD, uuid, 0);
    ov_json_value *params = ov_event_api_set_parameter(msg);
    ov_recorder_event_stop_to_json(params, &stop_event);

    if (!ov_json_object_set(params, OV_KEY_LOOP, ov_json_string(loop)))
        goto error;

    ov_event_connection_send(conn, msg);
    msg = ov_json_value_free(msg);

    ov_log_debug("Deactivated recording for Loop %s", loop);

    ov_vocs_db_set_recorded(self->config.vocs_db, loop, false);

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *
ov_vocs_recorder_get_recording(ov_vocs_recorder *self,
                               ov_db_recordings_get_params params) {

    if (!self)
        goto error;

    return ov_db_recordings_get_struct(self->db, MAX_RESULTS_PER_QUERY, params);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool add_recorded_loop(const void *key, void *val, void *data) {

    if (!key)
        return true;

    ov_vocs_record *record = (ov_vocs_record *)val;
    ov_json_value *array = ov_json_value_cast(data);

    if (record->active.running) {

        ov_json_value *out = ov_json_object();
        ov_json_value *v = ov_json_string((char *)key);
        ov_json_object_set(out, OV_KEY_LOOP, v);
        ov_json_array_push(array, out);
    }

    return true;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_recorder_get_recorded_loops(ov_vocs_recorder *self) {

    ov_json_value *out = NULL;

    if (!self)
        goto error;

    out = ov_json_array();

    ov_dict_for_each(self->recordings, out, add_recorded_loop);

    return out;

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_recorder_start_loop_recording(
    ov_vocs_recorder *self, const char *uuid, const char *loop, void *userdata,
    int socket,
    void (*callback)(void *, int, const char *, const char *, ov_result)) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;

    ov_vocs_record *record = NULL;

    if (!self || !uuid || !loop || !userdata || !callback)
        goto error;

    ov_callback cb = (ov_callback){
        .userdata = userdata, .function = callback, .socket = socket};

    record = ov_dict_get(self->recordings, loop);
    if (!record) {

        ov_vocs_record_config conf = {0};
        strncpy(conf.loopname, loop, OV_MC_LOOP_NAME_MAX);
        record = ov_vocs_record_create(conf);

        if (!ov_dict_set(self->recordings, ov_string_dup(loop), record, NULL))
            goto error;
    }

    if (record->active.running) {

        callback(userdata, socket, uuid, loop,
                 (ov_result){.error_code = OV_ERROR_CODE_ALREADY_SET,
                             .message = "Loop already recorded."});

        goto error;
    }

    ov_socket_configuration socket_config =
        ov_vocs_db_get_multicast_group(self->config.vocs_db, loop);

    if (0 == socket_config.host[0]) {

        callback(
            userdata, socket, uuid, loop,
            (ov_result){.error_code = OV_ERROR_CODE_DESTINATION_UNKNOWN,
                        .message = "No multicast socket avaliable for loop"});

        goto error;
    }

    ov_recorder_event_start event =
        (ov_recorder_event_start){.loop = (char *)loop,
                                  .mc_ip = socket_config.host,
                                  .mc_port = socket_config.port};

    ov_event_connection *conn = find_empty_recorder(self);
    if (!conn) {

        callback(userdata, socket, uuid, loop,
                 (ov_result){.error_code = OV_ERROR_NO_RESOURCE,
                             .message = "No recorder avaliable for loop"});

        goto error;
    }

    // we block the recorder here
    ov_json_value *f = ov_json_false();
    ov_event_connection_set_json(conn, OV_KEY_EMPTY, f);
    f = ov_json_value_free(f);

    out = ov_event_api_message_create(OV_EVENT_START_RECORD, uuid, 0);
    par = ov_event_api_set_parameter(out);

    if (!ov_recorder_event_start_to_json(par, &event)) {
        out = ov_json_value_free(out);

        callback(userdata, socket, uuid, loop,
                 (ov_result){.error_code = OV_ERROR_CODE_PROCESSING_ERROR,
                             .message = OV_ERROR_DESC_PROCESSING_ERROR});

        goto error;
    }

    if (!ov_callback_registry_register(self->callbacks, uuid, cb,
                                       self->config.timeout.response_usec))
        goto error;

    record->active.recorder = ov_event_connection_get_socket(conn);

    ov_log_debug("Activated recording for Loop %s", loop);

    ov_event_connection_send(conn, out);
    out = ov_json_value_free(out);

    ov_vocs_db_set_recorded(self->config.vocs_db, loop, true);

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_recorder_stop_loop_recording(
    ov_vocs_recorder *self, const char *uuid, const char *loop, void *userdata,
    int socket,
    void (*callback)(void *, int, const char *, const char *, ov_result)) {

    if (!self || !loop || !callback || !userdata)
        goto error;

    ov_callback cb = (ov_callback){
        .userdata = userdata, .function = callback, .socket = socket};

    ov_vocs_record *rec = ov_dict_get(self->recordings, loop);
    if (!rec) {

        callback(userdata, socket, uuid, loop,
                 (ov_result){.error_code = OV_ERROR_NOT_RECORDING,
                             .message = OV_ERROR_DESC_NOT_RECORDING});

        goto error;
    }

    ov_recorder_event_stop stop_event = {0};

    ov_event_connection *conn =
        ov_dict_get(self->recorder, (void *)(intptr_t)rec->active.recorder);

    if (!conn) {

        if (rec->active.running)
            rec->active.running = false;

        callback(userdata, socket, uuid, loop,
                 (ov_result){.error_code = OV_ERROR_NOT_RECORDING,
                             .message = "No recorder connected."});

        goto error;
    }

    memcpy(stop_event.id, rec->active.id, 36);

    ov_json_value *msg =
        ov_event_api_message_create(OV_EVENT_STOP_RECORD, uuid, 0);
    ov_json_value *params = ov_event_api_set_parameter(msg);
    ov_recorder_event_stop_to_json(params, &stop_event);

    if (!ov_json_object_set(params, OV_KEY_LOOP, ov_json_string(loop)))
        goto error;

    ov_event_connection_send(conn, msg);
    msg = ov_json_value_free(msg);

    ov_log_debug("Deactivated recording for Loop %s", loop);

    if (!ov_callback_registry_register(self->callbacks, uuid, cb,
                                       self->config.timeout.response_usec))
        goto error;

    ov_vocs_db_set_recorded(self->config.vocs_db, loop, false);

    return true;

error:
    return false;
}
