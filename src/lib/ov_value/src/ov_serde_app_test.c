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

#include "ov_serde_app.c"
#include <ov_base/ov_socket.h>
#include <ov_base/ov_test_event_loop.h>
#include <ov_test/ov_test.h>
#include <ov_test/ov_test_tcp.h>

/*----------------------------------------------------------------------------*/

static bool error_result(ov_result *res, char const *msg) {
    return ov_result_set(res, OV_ERROR_INTERNAL_SERVER, msg);
}

/*****************************************************************************
                                  SERDE dummy
 ****************************************************************************/

#define SERDE_MAGIC_BYTES 0x1232345456

typedef struct {

    ov_serde public;

    size_t state;
    size_t max_state;

    ov_buffer *buffer;

} TestSerde;

/*----------------------------------------------------------------------------*/

static TestSerde *as_test_serde(ov_serde *serde) {

    if (0 == serde) {
        return 0;
    } else if (SERDE_MAGIC_BYTES != serde->magic_bytes) {
        return 0;
    } else {
        return (TestSerde *)serde;
    }
}

/*----------------------------------------------------------------------------*/

static bool serde_state_equals(ov_serde *serde, size_t state) {

    TestSerde *ts = as_test_serde(serde);

    if (0 == ts) {
        return false;
    } else if (state == ts->state) {
        testrun_log(
            "Serde state %zu fits expected state %zu", ts->state, state);
        return true;
    } else {
        testrun_log("Serde state %zu does not fit expected state %zu",
                    ts->state,
                    state);
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static ov_serde_state serde_inc(TestSerde *serde,
                                char const *msg,
                                ov_result *res) {

    if ((0 == serde) || (0 != ov_string_compare(msg, "inc"))) {

        error_result(res, "invalid opcode");
        return OV_SERDE_ERROR;

    } else {

        ++serde->max_state;
        return OV_SERDE_PROGRESS;
    }
}

/*----------------------------------------------------------------------------*/

static ov_serde_state serde_end(TestSerde *serde,
                                char const *msg,
                                ov_result *res) {

    UNUSED(serde);

    if (0 != ov_string_compare(msg, "end")) {
        error_result(res, "invalid opcode");
        return OV_SERDE_ERROR;
    } else {
        return OV_SERDE_END;
    }
}

/*----------------------------------------------------------------------------*/

static ov_serde_state serde_reset(TestSerde *serde,
                                  char const *msg,
                                  ov_result *res) {

    UNUSED(serde);

    if (0 != ov_string_compare(msg, "rst")) {
        error_result(res, "invalid opcode");
        return OV_SERDE_ERROR;
    } else {
        serde->max_state = 0;
        serde->state = 0;
        return OV_SERDE_PROGRESS;
    }
}

/*----------------------------------------------------------------------------*/

static ov_serde_state serde_error(TestSerde *serde,
                                  char const *msg,
                                  ov_result *res) {

    UNUSED(serde);

    if (0 != ov_string_compare(msg, "err")) {
        error_result(res, "invalid opcode");
        return OV_SERDE_ERROR;
    } else {
        error_result(res, "Error triggered by protocol");
        return OV_SERDE_ERROR;
    }
}

/*----------------------------------------------------------------------------*/

// That's heck of slow, but here we only care for correctness
static bool get_next_token(ov_buffer *buf, char *dest) {

    if ((0 == dest) || (0 == buf) || (0 == buf->start) || (3 > buf->length)) {
        return false;
    } else {

        memcpy(dest, buf->start, 3);
        dest[3] = 0;

        fprintf(stderr, "Next token: %s\n", dest);

        ov_buffer *new = ov_buffer_create(buf->length - 3);

        uint8_t *data = buf->start;
        size_t capacity = buf->capacity;

        buf->start += 3;
        buf->length -= 3;
        ov_buffer_copy((void **)&new, buf);
        buf->start = new->start;
        buf->capacity = new->capacity;

        new->capacity = capacity;
        new->start = data;

        ov_buffer_free(new);

        return true;
    }
}

/*----------------------------------------------------------------------------*/

static ov_serde_state execute_token(TestSerde *serde,
                                    char const *token,
                                    ov_result *res) {

    switch (token[2]) {
        case 'c':
            return serde_inc(serde, token, res);

        case 'd':
            return serde_end(serde, token, res);

        case 't':
            return serde_reset(serde, token, res);

        case 'r':
            return serde_error(serde, token, res);

        default:

            return OV_SERDE_PROGRESS;
    };
}

/*----------------------------------------------------------------------------*/

static ov_serde_state serde_parse_data(TestSerde *serde,
                                       ov_buffer const *msg,
                                       ov_result *res) {

    if (0 == serde) {
        error_result(res, "Serde invalid");
        return OV_SERDE_ERROR;
    } else {

        serde->buffer = ov_buffer_concat(serde->buffer, msg);

        while (true) {

            char token[4];
            if (!get_next_token(serde->buffer, token)) {
                return OV_SERDE_PROGRESS;
            } else {

                switch (execute_token(serde, token, res)) {

                    case OV_SERDE_PROGRESS:
                        break;
                    case OV_SERDE_END:
                        return OV_SERDE_END;
                    case OV_SERDE_ERROR:
                        return OV_SERDE_ERROR;
                }
            }
        }
    }
}

/*----------------------------------------------------------------------------*/

// simple protocol:
// Any string with length != 3 -> ingore
// inc => increase max-state by 1 and proceed
// end => end, emit new parsed datum
// rst => reset max-state to 0
// err => Trigger error
// anything else: ignore
ov_serde_state serde_add_raw(ov_serde *self,
                             ov_buffer const *raw,
                             ov_result *res) {

    TestSerde *serde = as_test_serde(self);

    if (0 == serde) {
        error_result(res, "Invalid Serde");
        return OV_SERDE_ERROR;
    } else if (0 == raw->length) {
        error_result(res, "No input");
        return OV_SERDE_ERROR;
    } else {
        return serde_parse_data(serde, raw, res);
    }
}

/*----------------------------------------------------------------------------*/

ov_serde_data serde_pop_datum(ov_serde *self, ov_result *res) {

    TestSerde *serde = as_test_serde(self);

    testrun_log("Requested next parsed data from serde");

    if (0 == serde) {
        error_result(res, "invalid serde");
        return OV_SERDE_DATA_NO_MORE;
    } else if (serde->max_state <= serde->state) {
        testrun_log("max_state reached: %zu    current: %zu",
                    serde->max_state,
                    serde->state);
        return OV_SERDE_DATA_NO_MORE;
    } else {
        testrun_log("Increasing current state: max_state: %zu    current: %zu",
                    serde->max_state,
                    serde->state);
        ++serde->state;
        return (ov_serde_data){
            .data_type = self->magic_bytes,
            .data = &serde->state,
        };
    }
}

/*----------------------------------------------------------------------------*/

static bool serde_clear_buffer(ov_serde *self) {
    TestSerde *serde = as_test_serde(self);

    if (0 == serde) {
        return false;
    } else {
        serde->max_state = 0;
        serde->state = 0;
        serde->buffer = ov_buffer_free(serde->buffer);
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static void write_to_fh(int fh, char const *str) {

    if ((-1 < fh) && (0 != str)) {
        write(fh, str, strlen(str));
    }
}

/*----------------------------------------------------------------------------*/

static bool datum_to_fh(int fh, size_t *datum, ov_result *res) {

    if (0 > fh) {

        ov_result_set(res, OV_ERROR_INTERNAL_SERVER, "Invalid file handle");
        testrun_log_error("Invalid file handle");
        return false;

    } else if (0 == datum) {

        ov_result_set(res, OV_ERROR_INTERNAL_SERVER, "No datum (0 pointer)");
        testrun_log_error("No datum (0 pointer)");
        return false;

    } else if (2 > *datum) {

        write_to_fh(fh, "LOW\n");
        return true;

    } else if (5 > *datum) {

        write_to_fh(fh, "MED\n");
        return true;

    } else {

        write_to_fh(fh, "HIG\n");
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool serde_serialize(ov_serde *self,
                            int fh,
                            ov_serde_data data,
                            ov_result *res) {

    UNUSED(self);

    if (SERDE_MAGIC_BYTES != data.data_type) {

        ov_result_set(res, OV_ERROR_INTERNAL_SERVER, "Wrong data type");
        testrun_log_error("Wrong data type");
        return false;

    } else {

        return datum_to_fh(fh, data.data, res);
    }
}

/*----------------------------------------------------------------------------*/

ov_serde *serde_free(ov_serde *self) {

    TestSerde *serde = as_test_serde(self);

    if (0 != serde) {
        ov_buffer_free(serde->buffer);
        free(serde);
        return 0;
    } else {
        return self;
    }
}

/*----------------------------------------------------------------------------*/

static ov_serde *get_test_serde() {

    ov_serde *serde = calloc(1, sizeof(TestSerde));

    serde->magic_bytes = SERDE_MAGIC_BYTES;
    serde->add_raw = serde_add_raw;
    serde->clear_buffer = serde_clear_buffer;
    serde->free = serde_free;
    serde->pop_datum = serde_pop_datum;
    serde->serialize = serde_serialize;

    return serde;
}

/*****************************************************************************
                                    Handler
 ****************************************************************************/

struct handler_data {
    int sckt;
    void *additional;
} closed_handler_data;

static void reset_closed_handler_data() {
    closed_handler_data.sckt = -1;
    closed_handler_data.additional = 0;
}

static void closed_handler(int sckt, void *additional) {

    closed_handler_data.sckt = sckt;
    closed_handler_data.additional = additional;
}

/*----------------------------------------------------------------------------*/

struct handler_data reconnected_handler_data;

static void reset_reconnected_handler_data() {
    reconnected_handler_data.sckt = -1;
    reconnected_handler_data.additional = 0;
}

static void reconnected_handler(int sckt, void *additional) {

    reconnected_handler_data.sckt = sckt;
    reconnected_handler_data.additional = additional;
}

/*----------------------------------------------------------------------------*/

struct handler_data accepted_handler_data;

static void reset_accepted_handler_data() {
    accepted_handler_data.sckt = -1;
    accepted_handler_data.additional = 0;
}

static void accepted_handler(int sckt, void *additional) {

    accepted_handler_data.sckt = sckt;
    accepted_handler_data.additional = additional;
}

/*----------------------------------------------------------------------------*/

static void dump_handler_data(char const *kind, struct handler_data *hd) {

    fprintf(stderr,
            "%s_data socket: %i   additional: %p\n",
            kind,
            hd->sckt,
            hd->additional);
}

/*----------------------------------------------------------------------------*/

static void dump_all_handler_data() {

    dump_handler_data("closed", &closed_handler_data);
    dump_handler_data("reconnected", &reconnected_handler_data);
    dump_handler_data("accepted", &accepted_handler_data);
}

/*----------------------------------------------------------------------------*/

static void reset_all_handler_data() {

    reset_closed_handler_data();
    reset_reconnected_handler_data();
    reset_accepted_handler_data();
}

/*****************************************************************************
                               SerdeAppResources
 ****************************************************************************/

typedef struct {

    ov_serde *serde;
    ov_event_loop *loop;
    ov_serde_app *app;

    bool thread_running;
    pthread_t loop_thread;

} SerdeAppResources;

/*----------------------------------------------------------------------------*/

static SerdeAppResources get_resources(char const *name,
                                       void *additional,
                                       uint64_t reconnect_interval_secs) {

    SerdeAppResources res = {
        .serde = get_test_serde(),
        .loop = ov_event_loop_default(ov_event_loop_config_default()),

    };

    ov_serde_app_configuration cfg = {

        .reconnect_interval_secs = reconnect_interval_secs,
        .accept_to_io_timeout_secs = 5,

        .serde = res.serde,
        .cb_accepted = accepted_handler,
        .cb_closed = closed_handler,
        .cb_reconnected = reconnected_handler,
        .additional = additional,
    };

    res.app = ov_serde_app_create(name, res.loop, cfg);

    return res;
}

/*----------------------------------------------------------------------------*/

static void run_for(SerdeAppResources *res, uint64_t max_run_interval_sec) {

    ov_event_loop_run(res->loop, max_run_interval_sec * 1000 * 1000);
}

/*----------------------------------------------------------------------------*/

static void release_resources(SerdeAppResources res) {

    if (res.thread_running) {
        ov_test_event_loop_stop_thread(res.loop, res.loop_thread);
    }

    ov_serde_app_free(res.app);
    ov_event_loop_free(res.loop);
    ov_serde_free(res.serde);
}

/*****************************************************************************
                                     TESTS
 ****************************************************************************/

static int test_ov_serde_app_create() {

    ov_serde_app_configuration cfg = {0};

    testrun(0 == ov_serde_app_create(0, 0, cfg));
    testrun(0 == ov_serde_app_create("abrakadabra", 0, cfg));

    ov_event_loop *loop = ov_event_loop_default(ov_event_loop_config_default());
    testrun(0 != loop);

    testrun(0 == ov_serde_app_create(0, loop, cfg));
    testrun(0 == ov_serde_app_create("abrakadabra", loop, cfg));

    cfg.serde = get_test_serde();
    ov_serde_app *app = ov_serde_app_create("abrakadabra", loop, cfg);

    testrun(0 != app);

    app = ov_serde_app_free(app);
    testrun(0 == app);

    cfg.serde = ov_serde_free(cfg.serde);
    testrun(0 == cfg.serde);

    loop = ov_event_loop_free(loop);
    testrun(0 == loop);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_serde_app_free() {

    testrun(0 == ov_serde_app_free(0));

    SerdeAppResources res = get_resources("OurApp", 0, 0);

    res.app = ov_serde_app_free(res.app);

    testrun(0 == res.app);

    release_resources(res);

    return testrun_log_success();
}

/*****************************************************************************
                         ov_serde_app_register_handler
 ****************************************************************************/

static size_t counter = 0;
static bool handler1_called = false;

// We expect data to be a size_t which tells us how much to increase the
// counter
static void handler1(void *data, int socket, void *additional) {

    UNUSED(data);
    UNUSED(socket);

    handler1_called = true;

    size_t inc = 0;

    if (0 != data) {
        inc = *(size_t *)data;
    }

    testrun_log("Got data: %p, inc is %zu", data, inc);

    counter += inc;

    if (0 != additional) {
        size_t *dyn_counter = additional;
        *dyn_counter += inc;
    }

    return;
}

/*----------------------------------------------------------------------------*/

static void dump_handler1_data() {

    fprintf(stderr,
            "handler1 was %scalled: %zu\n",
            handler1_called ? "" : "not",
            counter);
}

/*----------------------------------------------------------------------------*/

static ov_socket_configuration open_server_socket(SerdeAppResources res) {

    ov_socket_configuration cfg = {
        .host = "localhost",
        .port = 21331,
        .type = TCP,
    };

    ov_serde_app_open_server_socket(res.app, cfg);

    return cfg;
}

/*----------------------------------------------------------------------------*/

static bool start_app_thread(SerdeAppResources *res) {

    TEST_ASSERT(0 != res);

    res->thread_running =
        ov_test_event_loop_run_in_thread(res->loop, &res->loop_thread);

    return res->thread_running;
}

/*----------------------------------------------------------------------------*/

static int connect_with_app(ov_socket_configuration cfg) {

    int fd = ov_socket_create(cfg, true, 0);

    // Shrink send buffer this much that data is ALWAYS sent immediately
    int buflen = 1;
    if (0 == setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buflen, sizeof(buflen))) {
        return fd;
    } else {
        close(fd);
        return -1;
    }
}

/*----------------------------------------------------------------------------*/

static bool send_string(int s, char const *str) {

    char padded[3] = {0};

    ssize_t len = ov_string_len(str);

    if (len < 3) {
        len = 3;
        ov_string_copy(padded, str, 3);
    }

    testrun_log("Sending %s", str);

    return len == write(s, str, len);
}

/*----------------------------------------------------------------------------*/

static bool send_to_app(int s, char const **data) {

    while (true) {

        if (0 == data[0]) {
            return true;
        } else if (!send_string(s, *data)) {
            return false;
        } else {
            ++data;
        }
    };
}

/*----------------------------------------------------------------------------*/

static int test_ov_serde_app_register_handler() {

    int dummy = 1;

    testrun(!ov_serde_app_register_handler(0, 1234, 0));

    // Without reconnect - wrong data type

    SerdeAppResources res = get_resources("register_handler", &dummy, 0);

    testrun(ov_serde_app_register_handler(res.app, 1234, 0));
    testrun(!ov_serde_app_register_handler(0, 1234, handler1));

    testrun(ov_serde_app_register_handler(res.app, 1234, handler1));

    ov_socket_configuration server = open_server_socket(res);
    testrun(TCP == server.type);

    testrun(start_app_thread(&res));

    int s = connect_with_app(server);
    testrun(0 < s);

    testrun(send_to_app(s, (char const *[]){"inc", "inc", "end", 0}));

    sleep(10);

    testrun(serde_state_equals(res.serde, 1));

    // Serde delivered data of type SERDE_MAGIC_BYTES, but our handler
    // only accepted data of type 1234.
    // Thus there was no handler registered, and hence, the app closed the
    // connection due to unknown data received.

    testrun(0 < closed_handler_data.sckt);
    testrun(&dummy == closed_handler_data.additional);
    reset_closed_handler_data();

    /*-----------------------------------------------------------------------*/
    // "Unset" handler

    testrun(ov_serde_app_register_handler(res.app, 1234, 0));

    s = connect_with_app(server);
    testrun(0 < s);

    testrun(send_to_app(s, (char const *[]){"inc", "inc", "end", 0}));

    sleep(30);

    // No handler registered -> no further 'incs' parsed
    testrun(serde_state_equals(res.serde, 1));

    // Same as before - no handler at all -> app closes connection

    testrun(0 < closed_handler_data.sckt);
    testrun(&dummy == closed_handler_data.additional);
    reset_closed_handler_data();

    /*-----------------------------------------------------------------------*/
    // Set handler with proper data type

    testrun(
        ov_serde_app_register_handler(res.app, SERDE_MAGIC_BYTES, handler1));

    s = connect_with_app(server);
    testrun(0 < s);

    testrun(send_to_app(s, (char const *[]){"inc", "inc", "end", 0}));

    sleep(10);

    testrun(serde_state_equals(res.serde, 6));

    testrun(-1 == closed_handler_data.sckt);
    testrun(0 == closed_handler_data.additional);

    dump_all_handler_data();
    dump_handler1_data();

    reset_closed_handler_data();

    // Clean up

    close(s);
    s = -1;

    release_resources(res);

    reset_all_handler_data();

    return testrun_log_success();
}

/*****************************************************************************
                                    connect
 ****************************************************************************/

static int test_ov_serde_app_connect() {

    int *dummy = calloc(1, sizeof(int));
    *dummy = 21;

    ov_test_tcp_server_config tcp_srv_cfg = {.serve_client_callback =
                                                 ov_test_tcp_server_loopback};

    pid_t server_pid = 0;
    int server_port = 0;

    testrun(ov_test_tcp_server(&server_pid, &server_port, tcp_srv_cfg));

    ov_socket_configuration cfg = {
        .host = "localhost",
        .port = server_port,
        .type = NETWORK_TRANSPORT_TYPE_ERROR,
    };

    testrun_log("Test server on port: %i\n", server_port);

    testrun(!ov_serde_app_connect(0, cfg));

    SerdeAppResources res = get_resources("connect", dummy, 1);

    testrun(!ov_serde_app_connect(res.app, cfg));

    cfg.type = TCP;

    testrun(ov_serde_app_connect(res.app, cfg));

    run_for(&res, 3);

    testrun(reconnected_handler_data.sckt != -1); // reconnect shouldve happnd
    testrun(closed_handler_data.sckt == -1);
    testrun(accepted_handler_data.sckt == -1);

    // Try if reconnect works...
    ov_test_tcp_send(reconnected_handler_data.sckt, "PlaPla");
    sleep(1);
    testrun(ov_test_tcp_received(reconnected_handler_data.sckt, "Pla"));
    testrun(ov_test_tcp_received(reconnected_handler_data.sckt, "Pla"));
    ov_test_tcp_send(reconnected_handler_data.sckt, "quit");
    sleep(1);
    testrun(ov_test_tcp_received(reconnected_handler_data.sckt, "quit"));

    reset_all_handler_data();

    run_for(&res, 10);

    testrun(reconnected_handler_data.sckt != -1); // reconnect shouldve happnd
    testrun(closed_handler_data.sckt != -1);      // Server terminated the conn.
    testrun(accepted_handler_data.sckt == -1);

    testrun(ov_test_tcp_stop_process(server_pid));

    release_resources(res);

    reset_all_handler_data();

    free(dummy);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_serde_app_open_server_socket() {

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_serde_app_send() {

    int *dummy = calloc(1, sizeof(int));
    *dummy = 21;

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

    testrun_log("123 Test server on port: %i\n", server_port);

    SerdeAppResources res = get_resources("HandlingApp", dummy, 1);

    testrun(ov_serde_app_connect(res.app, cfg));

    run_for(&res, 5);

    int send_fd = reconnected_handler_data.sckt;
    testrun(send_fd > -1); // reconnect shouldve happnd

    ov_serde_data invalid_data = {0};

    size_t datum = 1; // Corresponds to "LOW"
    ov_serde_data data = {.data_type = SERDE_MAGIC_BYTES, .data = &datum};

    // So we are good to go

    testrun(!ov_serde_app_send(0, -1, invalid_data));
    testrun(!ov_serde_app_send(res.app, -1, invalid_data));
    testrun(!ov_serde_app_send(0, send_fd, invalid_data));
    testrun(!ov_serde_app_send(0, -1, data));
    testrun(!ov_serde_app_send(res.app, -1, data));
    testrun(ov_serde_app_send(res.app, send_fd, data));

    // If we sent it, echo server should reply with 'a' as well
    testrun(ov_test_tcp_received(send_fd, "LOW"));
    // run_for(&res, 4);

    testrun(ov_test_tcp_stop_process(server_pid));

    release_resources(res);

    free(dummy);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_serde_app",
            test_ov_serde_app_create,
            test_ov_serde_app_free,
            test_ov_serde_app_register_handler,
            test_ov_serde_app_connect,
            test_ov_serde_app_open_server_socket,
            test_ov_serde_app_send);

/*----------------------------------------------------------------------------*/
