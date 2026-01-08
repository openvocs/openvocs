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
        @file           ov_io_base_test.c
        @author         Markus Töpfer

        @date           2021-01-30


        ------------------------------------------------------------------------
*/
#include "ov_io_base.c"
#include <ov_test/ov_test.h>

#include <ov_base/ov_convert.h>

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

static bool test_debug_log = false;

static const char *test_resource_dir = 0;
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

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int domains_init() {

    test_resource_dir = ov_test_get_resource_path("/resources");

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

/*----------------------------------------------------------------------------*/

struct dummy_userdata {

    int listener;
    int connection;
    ov_buffer *buffer;
    bool result;
};

/*----------------------------------------------------------------------------*/

static bool dummy_accept(void *userdata, int listener, int connection) {

    struct dummy_userdata *data = (struct dummy_userdata *)userdata;
    data->connection = connection;
    data->listener = listener;

    if (test_debug_log)
        fprintf(stdout, "dummy_accept at %i for %i\n", listener, connection);

    return true;
}

/*----------------------------------------------------------------------------*/

static bool dummy_io(void *userdata, int connection,
                     const ov_memory_pointer buffer) {

    struct dummy_userdata *data = (struct dummy_userdata *)userdata;

    data->connection = connection;
    if (!data->buffer)
        data->buffer = ov_buffer_create(buffer.length);

    if (!ov_buffer_set(data->buffer, buffer.start, buffer.length))
        goto error;

    if (test_debug_log)
        fprintf(stdout, "dummy_io at %i bytes %zu content\n%.*s\n", connection,
                buffer.length, (int)buffer.length, buffer.start);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void dummy_close(void *userdata, int connection) {

    struct dummy_userdata *data = (struct dummy_userdata *)userdata;
    data->connection = connection;

    if (test_debug_log)
        fprintf(stdout, "dummy_close at %i\n", connection);

    return;
}

/*----------------------------------------------------------------------------*/
/*
static void dummy_connected(void *userdata, int connection, bool result) {

    struct dummy_userdata *data = (struct dummy_userdata *)userdata;
    data->connection = connection;
    data->result = result;

    if (test_debug_log) fprintf(stdout, "dummy_connected at %i\n", connection);

    return;
}
*/
/*----------------------------------------------------------------------------*/

static int dummy_client_hello_cb(SSL *s, int *al, void *arg) {

    /*
     *      At some point @perform_ssl_client_handshake
     *      may stop with SSL_ERROR_WANT_CLIENT_HELLO_CB,
     *      so we add some dummy callback, to resume
     *      standard SSL operation.
     */
    if (!s)
        return SSL_CLIENT_HELLO_ERROR;

    if (al || arg) { /* ignored */
    };
    return SSL_CLIENT_HELLO_SUCCESS;
}

/*----------------------------------------------------------------------------*/

static int run_client_handshake(ov_event_loop *loop, SSL *ssl, int *err,
                                int *errorcode) {

    testrun(loop);
    testrun(ssl);
    testrun(err);
    testrun(errorcode);

    int r = 0;
    int n = 0;

    bool run = true;
    while (run) {

        loop->run(loop, TEST_RUNTIME_USECS);

        n = 0;
        r = SSL_connect(ssl);

        switch (r) {

        case 1:
            /* SUCCESS */
            run = false;
            break;

        default:

            n = SSL_get_error(ssl, r);

            switch (n) {

            case SSL_ERROR_NONE:
                /* SHOULD not be returned in 0 */
                break;

            case SSL_ERROR_ZERO_RETURN:
                /* close */
                run = false;
                break;

            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_CONNECT:
            case SSL_ERROR_WANT_X509_LOOKUP:
            case SSL_ERROR_WANT_ASYNC:
            case SSL_ERROR_WANT_ASYNC_JOB:
            case SSL_ERROR_WANT_CLIENT_HELLO_CB:
                /* try async again */
                break;

            case SSL_ERROR_SYSCALL:
                /* nonrecoverable IO error */
                *errorcode = ERR_get_error();
                run = false;
                break;

            case SSL_ERROR_SSL:
                run = false;
                break;

            case SSL_ERROR_WANT_ACCEPT:
                run = false;
                break;
            }
            break;
        }

        fprintf(stdout, "\nr %i n %i\n", r, n);
    }

    *err = n;
    return r;
}

/*----------------------------------------------------------------------------*/

static char *fingerprint_format_RFC8122(const char *source, size_t length) {

    /*
    hash-func         =  "sha-1" / "sha-224" / "sha-256" /
                         "sha-384" / "sha-512" /
                         "md5" / "md2" / token
                         ; Additional hash functions can only come
                         ; from updates to RFC 3279

    fingerprint            =  2UHEX *(":" 2UHEX)
                         ; Each byte in upper-case hex, separated
                         ; by colons.

    UHEX                   =  DIGIT / %x41-46 ; A-F uppercase
    */

    char *fingerprint = NULL;

    if (!source)
        return NULL;

    size_t hex_len = 2 * length + 1;
    char hex[hex_len + 1];
    memset(hex, 0, hex_len);
    uint8_t *ptr = (uint8_t *)hex;

    if (!ov_convert_binary_to_hex((uint8_t *)source, length, &ptr, &hex_len))
        goto error;

    size_t size = hex_len + length;
    fingerprint = calloc(size + 1, sizeof(char));

    for (size_t i = 0; i < length; i++) {

        fingerprint[(i * 3) + 0] = toupper(hex[(i * 2) + 0]);
        fingerprint[(i * 3) + 1] = toupper(hex[(i * 2) + 1]);
        if (i < length - 1)
            fingerprint[(i * 3) + 2] = ':';
    }

    return fingerprint;

error:
    if (fingerprint)
        free(fingerprint);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static char *fingerprint_from_cert(const X509 *cert, const EVP_MD *func) {

    if (!cert || !func)
        return NULL;

    unsigned char mdigest[EVP_MAX_MD_SIZE] = {0};
    unsigned int mdigest_size = 0;

    if (0 == X509_digest(cert, func, mdigest, &mdigest_size))
        return NULL;

    return fingerprint_format_RFC8122((char *)mdigest, mdigest_size);
}

/*----------------------------------------------------------------------------*/

static char *fingerprint_from_path(const char *path, const EVP_MD *func) {

    char *fingerprint = NULL;
    FILE *fp = NULL;
    X509 *x = NULL;

    if (!path || !func)
        goto error;

    fp = fopen(path, "r");

    if (!PEM_read_X509(fp, &x, NULL, NULL))
        goto error;

    fingerprint = fingerprint_from_cert(x, func);

    fclose(fp);
    X509_free(x);
    return fingerprint;
error:
    if (x)
        X509_free(x);
    if (fp)
        fclose(fp);

    if (fingerprint)
        ov_data_pointer_free(fingerprint);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool check_cert(SSL *ssl, const EVP_MD *func, const char *fingerprint) {

    if (!ssl || !func || !fingerprint)
        return false;

    char *finger = NULL;

    X509 *cert = SSL_get_peer_certificate(ssl);
    if (!cert)
        goto error;

    finger = fingerprint_from_cert(cert, func);
    if (!finger)
        goto error;

    if (0 != strcmp(finger, fingerprint))
        goto error;

    finger = ov_data_pointer_free(finger);
    X509_free(cert);

    return true;
error:
    if (cert)
        X509_free(cert);

    if (finger)
        ov_data_pointer_free(finger);
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_io_base_create() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_io_base_config config = {0};

    ov_io_base *base = ov_io_base_create(config);
    testrun(!base);

    uint32_t max_sockets = ov_socket_get_max_supported_runtime_sockets(0);

    config.loop = loop;
    base = ov_io_base_create(config);
    testrun(base);
    testrun(ov_io_base_cast(base));

    /* check config */
    testrun(base->config.debug == false);
    testrun(base->config.loop == loop);
    testrun(base->config.limit.max_sockets == max_sockets);
    testrun(base->config.timer.io_timeout_usec == 0);
    testrun(base->config.timer.accept_to_io_timeout_usec ==
            OV_IO_BASE_DEFAULT_ACCEPT_TIMEOUT_USEC);

    /* check struct init */
    testrun(base->connections_max == max_sockets);
    testrun(OV_TIMER_INVALID != base->timer.io_timeout_check);
    testrun(NULL == base->domain.array);
    testrun(0 == base->domain.size);

    for (uint32_t i = 0; i < base->connections_max; i++) {
        testrun(base->conn[i].base == base);
        testrun(base->conn[i].fd == -1);
        testrun(ov_list_cast(base->conn[i].out.queue));
    }

    testrun(NULL == ov_io_base_free(base));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_io_base_free() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_io_base *base = ov_io_base_create(
        (ov_io_base_config){.loop = loop, .debug = test_debug_log});

    testrun(base);

    testrun(NULL == ov_io_base_free(base));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_io_base_debug() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_io_base *base =
        ov_io_base_create((ov_io_base_config){.loop = loop, .debug = false});

    testrun(base);

    testrun(!ov_io_base_debug(NULL, false));

    testrun(ov_io_base_debug(base, false));
    testrun(base->config.debug == false);
    testrun(ov_io_base_debug(base, true));
    testrun(base->config.debug == true);

    testrun(NULL == ov_io_base_free(base));
    testrun(NULL == ov_event_loop_free(loop));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_io_base_load_ssl_config() {

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_io_base *base = ov_io_base_create(
        (ov_io_base_config){.loop = loop, .debug = test_debug_log});

    testrun(base);

    testrun(!ov_io_base_load_ssl_config(NULL, NULL));
    testrun(!ov_io_base_load_ssl_config(base, NULL));
    testrun(!ov_io_base_load_ssl_config(NULL, test_resource_dir));

    testrun(ov_io_base_load_ssl_config(base, test_resource_dir));
    testrun(base);
    testrun(0 != base->domain.size);
    testrun(NULL != base->domain.array);

    testrun(find_domain(base, (uint8_t *)TEST_DOMAIN_NAME,
                        strlen(TEST_DOMAIN_NAME)));
    testrun(find_domain(base, (uint8_t *)TEST_DOMAIN_NAME_ONE,
                        strlen(TEST_DOMAIN_NAME_ONE)));
    testrun(find_domain(base, (uint8_t *)TEST_DOMAIN_NAME_TWO,
                        strlen(TEST_DOMAIN_NAME_TWO)));

    testrun(NULL == ov_io_base_free(base));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_io_base_create_listener() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_io_base *base = ov_io_base_create(
        (ov_io_base_config){.loop = loop, .debug = test_debug_log});

    testrun(base);

    testrun(ov_io_base_load_ssl_config(base, test_resource_dir));
    testrun(0 != base->domain.size);
    testrun(NULL != base->domain.array);

    ov_io_base_listener_config config = (ov_io_base_listener_config){

        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TCP, .host = "127.0.0.1"}),

        .callback.userdata = &userdata,
        .callback.accept = dummy_accept,
        .callback.io = dummy_io,
        .callback.close = dummy_close};

    testrun(-1 ==
            ov_io_base_create_listener(base, (ov_io_base_listener_config){0}));
    testrun(-1 == ov_io_base_create_listener(NULL, config));

    fprintf(stdout, "Check %s:%i|%i\n", config.socket.host, config.socket.port,
            config.socket.type);
    int tcp = ov_io_base_create_listener(base, config);
    testrun(-1 != tcp);

    ov_socket_configuration local_config =
        (ov_socket_configuration){.type = LOCAL};

    ssize_t bytes =
        snprintf(local_config.host, OV_HOST_NAME_MAX, "/tmp/local12313");
    testrun(bytes > 0);
    testrun(bytes < OV_HOST_NAME_MAX);

    unlink(local_config.host);

    config = (ov_io_base_listener_config){

        .socket = local_config,

        .callback.userdata = &userdata,
        .callback.accept = dummy_accept,
        .callback.io = dummy_io,
        .callback.close = dummy_close};

    int local = ov_io_base_create_listener(base, config);
    testrun(-1 != local);

    config = (ov_io_base_listener_config){

        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.type = TLS, .host = "127.0.0.1"}),

        .callback.userdata = &userdata,
        .callback.accept = dummy_accept,
        .callback.io = dummy_io,
        .callback.close = dummy_close};

    int tls = ov_io_base_create_listener(base, config);
    testrun(-1 != tls);

    config = (ov_io_base_listener_config){

        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = DTLS}),

        .callback.userdata = &userdata,
        .callback.accept = dummy_accept,
        .callback.io = dummy_io,
        .callback.close = dummy_close};

    testrun(-1 == ov_io_base_create_listener(base, config));

    config = (ov_io_base_listener_config){

        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = UDP}),

        .callback.userdata = &userdata,
        .callback.accept = dummy_accept,
        .callback.io = dummy_io,
        .callback.close = dummy_close};

    testrun(-1 == ov_io_base_create_listener(base, config));

    testrun(NULL == ov_io_base_free(base));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_io_base_create_connection() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_io_base *base = ov_io_base_create((ov_io_base_config){
        .loop = loop,
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 10 * 1000 * 1000});

    testrun(base);

    testrun(ov_io_base_load_ssl_config(base, test_resource_dir));
    testrun(0 != base->domain.size);
    testrun(NULL != base->domain.array);

    ov_io_base_listener_config tcp_config = (ov_io_base_listener_config){

        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),

        .callback.userdata = &userdata,
        .callback.accept = dummy_accept,
        .callback.io = dummy_io,
        .callback.close = dummy_close};

    ov_io_base_listener_config tls_config = (ov_io_base_listener_config){

        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TLS}),

        .callback.userdata = &userdata,
        .callback.accept = dummy_accept,
        .callback.io = dummy_io,
        .callback.close = dummy_close};

    tls_config.socket.type = TLS;

    ov_socket_configuration local_socket_config =
        (ov_socket_configuration){.type = LOCAL};

    ssize_t bytes =
        snprintf(local_socket_config.host, OV_HOST_NAME_MAX, "/tmp/local12313");
    unlink(local_socket_config.host);

    testrun(bytes > 0);
    testrun(bytes < OV_HOST_NAME_MAX);

    ov_io_base_listener_config local_config = (ov_io_base_listener_config){

        .socket = local_socket_config,
        .callback.userdata = &userdata,
        .callback.accept = dummy_accept,
        .callback.io = dummy_io,
        .callback.close = dummy_close};

    int tcp = ov_io_base_create_listener(base, tcp_config);
    testrun(-1 != tcp);

    int tls = ov_io_base_create_listener(base, tls_config);
    testrun(-1 != tls);

    int local = ov_io_base_create_listener(base, local_config);
    testrun(-1 != local);

    /* some listeners created */

    struct dummy_userdata client_userdata = {0};

    ov_io_base_connection_config client_config =
        (ov_io_base_connection_config){.connection = tcp_config};

    client_config.connection.callback.userdata = &client_userdata;

    testrun(-1 == ov_io_base_create_connection(
                      NULL, (ov_io_base_connection_config){0}));
    testrun(-1 == ov_io_base_create_connection(
                      base, (ov_io_base_connection_config){0}));
    testrun(-1 == ov_io_base_create_connection(NULL, client_config));

    int client = ov_io_base_create_connection(base, client_config);
    testrun(0 < client);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    int connection = userdata.connection;
    testrun(0 < connection);

    /* CHECK STATE IN SERVER */

    Connection *conn = &base->conn[client];
    testrun(conn->base == base);
    testrun(conn->fd == client);
    testrun(conn->type == CLIENT);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.close == dummy_close);
    testrun(conn->config.connection.callback.io == dummy_io);
    testrun(conn->config.connection.callback.userdata == &client_userdata);
    testrun(conn->out.queue != NULL);
    testrun(conn->stats.type == TCP);
    testrun(conn->in.recv_buffer_size > 0);

    conn = &base->conn[connection];
    testrun(conn->base == base);
    testrun(conn->type == CONNECTION);
    testrun(conn->fd == connection);
    testrun(conn->listener.fd == tcp);
    testrun(conn->config.connection.callback.close == dummy_close);
    testrun(conn->config.connection.callback.io == dummy_io);
    testrun(conn->config.connection.callback.userdata == &userdata);
    testrun(conn->out.queue != NULL);

    conn = &base->conn[tcp];
    testrun(conn->base == base);
    testrun(conn->type == LISTENER);
    testrun(conn->fd == tcp);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.close == NULL);
    testrun(conn->config.connection.callback.io == NULL);
    testrun(conn->config.connection.callback.userdata == NULL);
    testrun(conn->listener.config.callback.accept == dummy_accept);
    testrun(conn->listener.config.callback.close == dummy_close);
    testrun(conn->listener.config.callback.io == dummy_io);
    testrun(conn->listener.config.callback.userdata == &userdata);
    testrun(conn->out.queue != NULL);

    // check send
    bytes = -1;
    while (bytes == -1) {
        bytes = send(client, "test", 4, 0);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
    }
    testrun(bytes == 4);

    // check recv over dummy io
    testrun(userdata.connection == connection);
    testrun(0 ==
            memcmp("test", userdata.buffer->start, userdata.buffer->length));
    userdata.buffer = ov_buffer_free(userdata.buffer);

    bytes = -1;
    while (bytes == -1) {
        bytes = send(connection, "test", 4, 0);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
    }
    testrun(bytes == 4);

    // check recv over dummy io
    testrun(client_userdata.connection == client);
    testrun(0 == memcmp("test", client_userdata.buffer->start,
                        client_userdata.buffer->length));
    client_userdata.buffer = ov_buffer_free(client_userdata.buffer);

    // check close from server side
    client_userdata.connection = 0;
    testrun(ov_io_base_close(base, connection));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    // receive over close callback, socket is closed in base -> no recv possible
    testrun(client_userdata.connection == client);

    // check state in server client and connection must become unused
    conn = &base->conn[client];
    testrun(conn->base == base);
    testrun(conn->fd == -1);
    testrun(conn->type == CONNECTION_UNUSED);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.close == NULL);
    testrun(conn->config.connection.callback.io == NULL);
    testrun(conn->config.connection.callback.userdata == NULL);
    testrun(conn->out.queue != NULL);
    testrun(conn->stats.type == 0);
    testrun(conn->in.recv_buffer_size == 0);

    conn = &base->conn[connection];
    testrun(conn->base == base);
    testrun(conn->fd == -1);
    testrun(conn->type == CONNECTION_UNUSED);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.close == NULL);
    testrun(conn->config.connection.callback.io == NULL);
    testrun(conn->config.connection.callback.userdata == NULL);
    testrun(conn->out.queue != NULL);
    testrun(conn->stats.type == 0);
    testrun(conn->in.recv_buffer_size == 0);

    // reconnect
    client = ov_io_base_create_connection(base, client_config);
    testrun(0 < client);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    connection = userdata.connection;
    testrun(0 < connection);

    // check close from client side
    testrun(ov_io_base_close(base, client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    // receive over close callback, socket is closed in base -> no recv possible
    testrun(userdata.connection == connection);

    conn = &base->conn[client];
    testrun(conn->base == base);
    testrun(conn->fd == -1);
    testrun(conn->type == CONNECTION_UNUSED);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.close == NULL);
    testrun(conn->config.connection.callback.io == NULL);
    testrun(conn->config.connection.callback.userdata == NULL);
    testrun(conn->out.queue != NULL);
    testrun(conn->stats.type == 0);
    testrun(conn->in.recv_buffer_size == 0);

    conn = &base->conn[connection];
    testrun(conn->base == base);
    testrun(conn->fd == -1);
    testrun(conn->type == CONNECTION_UNUSED);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.close == NULL);
    testrun(conn->config.connection.callback.io == NULL);
    testrun(conn->config.connection.callback.userdata == NULL);
    testrun(conn->out.queue != NULL);
    testrun(conn->stats.type == 0);
    testrun(conn->in.recv_buffer_size == 0);

    // reconnect
    client = ov_io_base_create_connection(base, client_config);
    testrun(0 < client);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    connection = userdata.connection;
    testrun(0 < connection);

    // check listener close
    testrun(ov_io_base_close(base, tcp));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    // receive over close callback, socket is closed in base -> no recv possible
    testrun(userdata.connection == connection);

    conn = &base->conn[client];
    testrun(conn->base == base);
    testrun(conn->fd == -1);
    testrun(conn->type == CONNECTION_UNUSED);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.close == NULL);
    testrun(conn->config.connection.callback.io == NULL);
    testrun(conn->config.connection.callback.userdata == NULL);
    testrun(conn->out.queue != NULL);
    testrun(conn->stats.type == 0);
    testrun(conn->in.recv_buffer_size == 0);

    conn = &base->conn[connection];
    testrun(conn->base == base);
    testrun(conn->fd == -1);
    testrun(conn->type == CONNECTION_UNUSED);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.close == NULL);
    testrun(conn->config.connection.callback.io == NULL);
    testrun(conn->config.connection.callback.userdata == NULL);
    testrun(conn->out.queue != NULL);
    testrun(conn->stats.type == 0);
    testrun(conn->in.recv_buffer_size == 0);

    conn = &base->conn[tcp];
    testrun(conn->base == base);
    testrun(conn->fd == -1);
    testrun(conn->type == CONNECTION_UNUSED);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.close == NULL);
    testrun(conn->config.connection.callback.io == NULL);
    testrun(conn->config.connection.callback.userdata == NULL);
    testrun(conn->out.queue != NULL);
    testrun(conn->stats.type == 0);
    testrun(conn->in.recv_buffer_size == 0);

    /* <-- TCP DONE */

    client_config = (ov_io_base_connection_config){.connection = local_config};

    client_config.connection.callback.userdata = &client_userdata;

    client = ov_io_base_create_connection(base, client_config);
    testrun(0 < client);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    connection = userdata.connection;
    testrun(0 < connection);

    /* CHECK STATE IN SERVER */

    conn = &base->conn[client];
    testrun(conn->base == base);
    testrun(conn->fd == client);
    testrun(conn->type == CLIENT);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.close == dummy_close);
    testrun(conn->config.connection.callback.io == dummy_io);
    testrun(conn->config.connection.callback.userdata == &client_userdata);
    testrun(conn->out.queue != NULL);
    testrun(conn->stats.type == LOCAL);
    testrun(conn->in.recv_buffer_size > 0);

    conn = &base->conn[connection];
    testrun(conn->base == base);
    testrun(conn->type == CONNECTION);
    testrun(conn->fd == connection);
    testrun(conn->listener.fd == local);
    testrun(conn->config.connection.callback.close == dummy_close);
    testrun(conn->config.connection.callback.io == dummy_io);
    testrun(conn->config.connection.callback.userdata == &userdata);
    testrun(conn->out.queue != NULL);

    conn = &base->conn[local];
    testrun(conn->base == base);
    testrun(conn->type == LISTENER);
    testrun(conn->fd == local);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.close == NULL);
    testrun(conn->config.connection.callback.io == NULL);
    testrun(conn->config.connection.callback.userdata == NULL);
    testrun(conn->listener.config.callback.accept == dummy_accept);
    testrun(conn->listener.config.callback.close == dummy_close);
    testrun(conn->listener.config.callback.io == dummy_io);
    testrun(conn->listener.config.callback.userdata == &userdata);
    testrun(conn->out.queue != NULL);

    // check send
    bytes = -1;
    while (bytes == -1) {
        bytes = send(client, "test", 4, 0);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
    }
    testrun(bytes == 4);

    // check recv over dummy io
    testrun(userdata.connection == connection);
    testrun(0 ==
            memcmp("test", userdata.buffer->start, userdata.buffer->length));
    userdata.buffer = ov_buffer_free(userdata.buffer);

    bytes = -1;
    while (bytes == -1) {
        bytes = send(connection, "test", 4, 0);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
    }
    testrun(bytes == 4);

    // check recv over dummy io
    testrun(client_userdata.connection == client);
    testrun(0 == memcmp("test", client_userdata.buffer->start,
                        client_userdata.buffer->length));
    client_userdata.buffer = ov_buffer_free(client_userdata.buffer);

    // check close from server side
    client_userdata.connection = 0;
    testrun(ov_io_base_close(base, connection));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    // receive over close callback, socket is closed in base -> no recv possible
    testrun(client_userdata.connection == client);

    // check state in server client and connection must become unused
    conn = &base->conn[client];
    testrun(conn->base == base);
    testrun(conn->fd == -1);
    testrun(conn->type == CONNECTION_UNUSED);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.close == NULL);
    testrun(conn->config.connection.callback.io == NULL);
    testrun(conn->config.connection.callback.userdata == NULL);
    testrun(conn->out.queue != NULL);
    testrun(conn->stats.type == 0);
    testrun(conn->in.recv_buffer_size == 0);

    conn = &base->conn[connection];
    testrun(conn->base == base);
    testrun(conn->fd == -1);
    testrun(conn->type == CONNECTION_UNUSED);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.close == NULL);
    testrun(conn->config.connection.callback.io == NULL);
    testrun(conn->config.connection.callback.userdata == NULL);
    testrun(conn->out.queue != NULL);
    testrun(conn->stats.type == 0);
    testrun(conn->in.recv_buffer_size == 0);

    // reconnect
    client = ov_io_base_create_connection(base, client_config);
    testrun(0 < client);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    connection = userdata.connection;
    testrun(0 < connection);

    // check close from client side
    testrun(ov_io_base_close(base, client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    // receive over close callback, socket is closed in base -> no recv possible
    testrun(userdata.connection == connection);

    conn = &base->conn[client];
    testrun(conn->base == base);
    testrun(conn->fd == -1);
    testrun(conn->type == CONNECTION_UNUSED);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.close == NULL);
    testrun(conn->config.connection.callback.io == NULL);
    testrun(conn->config.connection.callback.userdata == NULL);
    testrun(conn->out.queue != NULL);
    testrun(conn->stats.type == 0);
    testrun(conn->in.recv_buffer_size == 0);

    conn = &base->conn[connection];
    testrun(conn->base == base);
    testrun(conn->fd == -1);
    testrun(conn->type == CONNECTION_UNUSED);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.close == NULL);
    testrun(conn->config.connection.callback.io == NULL);
    testrun(conn->config.connection.callback.userdata == NULL);
    testrun(conn->out.queue != NULL);
    testrun(conn->stats.type == 0);
    testrun(conn->in.recv_buffer_size == 0);

    // reconnect
    client = ov_io_base_create_connection(base, client_config);
    testrun(0 < client);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    connection = userdata.connection;
    testrun(0 < connection);

    // check listener close
    testrun(ov_io_base_close(base, local));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    // receive over close callback, socket is closed in base -> no recv possible
    testrun(userdata.connection == connection);

    conn = &base->conn[client];
    testrun(conn->base == base);
    testrun(conn->fd == -1);
    testrun(conn->type == CONNECTION_UNUSED);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.close == NULL);
    testrun(conn->config.connection.callback.io == NULL);
    testrun(conn->config.connection.callback.userdata == NULL);
    testrun(conn->out.queue != NULL);
    testrun(conn->stats.type == 0);
    testrun(conn->in.recv_buffer_size == 0);

    conn = &base->conn[connection];
    testrun(conn->base == base);
    testrun(conn->fd == -1);
    testrun(conn->type == CONNECTION_UNUSED);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.close == NULL);
    testrun(conn->config.connection.callback.io == NULL);
    testrun(conn->config.connection.callback.userdata == NULL);
    testrun(conn->out.queue != NULL);
    testrun(conn->stats.type == 0);
    testrun(conn->in.recv_buffer_size == 0);

    conn = &base->conn[local];
    testrun(conn->base == base);
    testrun(conn->fd == -1);
    testrun(conn->type == CONNECTION_UNUSED);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.close == NULL);
    testrun(conn->config.connection.callback.io == NULL);
    testrun(conn->config.connection.callback.userdata == NULL);
    testrun(conn->out.queue != NULL);
    testrun(conn->stats.type == 0);
    testrun(conn->in.recv_buffer_size == 0);
    unlink(local_config.socket.host);

    /* <-- LOCAL DONE */
    /*
        client_config = (ov_io_base_connection_config){
            .connection = tls_config, .connected = dummy_connected};

        client_config.connection.callback.userdata = &client_userdata;
        client_userdata.result = false;

        client = ov_io_base_create_connection(base, client_config);
        testrun(-1 == client);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));

        strcat(client_config.ssl.ca.file, OV_TEST_CERT);
        client = ov_io_base_create_connection(base, client_config);
        testrun(0 < client);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));

        connection = userdata.connection;
        testrun(0 < connection);
    */
    /* CHECK STATE IN SERVER */
    /*
        conn = &base->conn[client];
        testrun(conn->base == base);
        testrun(conn->fd == client);
        testrun(conn->type == CLIENT);
        testrun(conn->listener.fd == -1);
        testrun(conn->config.connection.callback.close == dummy_close);
        testrun(conn->config.connection.callback.io == dummy_io);
        testrun(conn->config.connection.callback.userdata == &client_userdata);
        testrun(conn->out.queue != NULL);
        testrun(conn->stats.type == TLS);
        testrun(conn->in.recv_buffer_size > 0);
        testrun(conn->tls.ctx != NULL);
        testrun(conn->tls.ssl != NULL);

        conn = &base->conn[connection];
        testrun(conn->base == base);
        testrun(conn->type == CONNECTION);
        testrun(conn->fd == connection);
        testrun(conn->listener.fd == tls);
        testrun(conn->config.connection.callback.close == dummy_close);
        testrun(conn->config.connection.callback.io == dummy_io);
        testrun(conn->config.connection.callback.userdata == &userdata);
        testrun(conn->out.queue != NULL);
        testrun(conn->tls.ssl != NULL);
        testrun(conn->tls.ctx == NULL);

        conn = &base->conn[tls];
        testrun(conn->base == base);
        testrun(conn->type == LISTENER);
        testrun(conn->fd == tls);
        testrun(conn->listener.fd == -1);
        testrun(conn->config.connection.callback.close == NULL);
        testrun(conn->config.connection.callback.io == NULL);
        testrun(conn->config.connection.callback.userdata == NULL);
        testrun(conn->listener.config.callback.accept == dummy_accept);
        testrun(conn->listener.config.callback.close == dummy_close);
        testrun(conn->listener.config.callback.io == dummy_io);
        testrun(conn->listener.config.callback.userdata == &userdata);
        testrun(conn->out.queue != NULL);
        testrun(conn->tls.ssl == NULL);
        testrun(conn->tls.ctx == NULL);

        // let SSL handshake finish
        while (!client_userdata.result) {
            testrun(loop->run(loop, TEST_RUNTIME_USECS));
        }
        testrun(loop->run(loop, TEST_RUNTIME_USECS));

        conn = &base->conn[client];
        testrun(conn->tls.handshaked == true);
        testrun(conn->timer_id == OV_TIMER_INVALID);

        conn = &base->conn[connection];
        testrun(conn->tls.handshaked == true);

        // send client to server (using the standard send, as the connection is
       SSL) testrun(ov_io_base_send( base, client, (ov_memory_pointer){.start =
       (uint8_t *)"test", .length = 4}));

        testrun(loop->run(loop, TEST_RUNTIME_USECS));

        // check recv over dummy io
        testrun(userdata.connection == connection);
        testrun(0 ==
                memcmp("test", userdata.buffer->start,
       userdata.buffer->length)); userdata.buffer =
       ov_buffer_free(userdata.buffer);

        // send server to client
        testrun(ov_io_base_send(
            base,
            connection,
            (ov_memory_pointer){.start = (uint8_t *)"test", .length = 4}));

        testrun(loop->run(loop, TEST_RUNTIME_USECS));

        // check recv over dummy io
        testrun(client_userdata.connection == client);
        testrun(0 == memcmp("test",
                            client_userdata.buffer->start,
                            client_userdata.buffer->length));
        client_userdata.buffer = ov_buffer_free(client_userdata.buffer);

        // check close from server side
        client_userdata.connection = 0;
        testrun(ov_io_base_close(base, connection));
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        // receive over close callback, socket is closed in base -> no recv
       possible testrun(client_userdata.connection == client);

        // check state in server client and connection must become unused
        conn = &base->conn[client];
        testrun(conn->base == base);
        testrun(conn->fd == -1);
        testrun(conn->type == CONNECTION_UNUSED);
        testrun(conn->listener.fd == -1);
        testrun(conn->config.connection.callback.close == NULL);
        testrun(conn->config.connection.callback.io == NULL);
        testrun(conn->config.connection.callback.userdata == NULL);
        testrun(conn->out.queue != NULL);
        testrun(conn->stats.type == 0);
        testrun(conn->in.recv_buffer_size == 0);

        conn = &base->conn[connection];
        testrun(conn->base == base);
        testrun(conn->fd == -1);
        testrun(conn->type == CONNECTION_UNUSED);
        testrun(conn->listener.fd == -1);
        testrun(conn->config.connection.callback.close == NULL);
        testrun(conn->config.connection.callback.io == NULL);
        testrun(conn->config.connection.callback.userdata == NULL);
        testrun(conn->out.queue != NULL);
        testrun(conn->stats.type == 0);
        testrun(conn->in.recv_buffer_size == 0);

        // reconnect
        client_userdata.result = false;
        client = ov_io_base_create_connection(base, client_config);
        testrun(0 < client);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        connection = userdata.connection;
        testrun(0 < connection);

        // let SSL handshake finish
        while (!client_userdata.result) {
            testrun(loop->run(loop, TEST_RUNTIME_USECS));
        }
        testrun(loop->run(loop, TEST_RUNTIME_USECS));

        conn = &base->conn[client];
        testrun(conn->tls.handshaked == true);
        testrun(conn->timer_id == OV_TIMER_INVALID);
        conn = &base->conn[connection];
        testrun(conn->tls.handshaked == true);

        // check close from client side
        testrun(ov_io_base_close(base, client));
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        // receive over close callback, socket is closed in base -> no recv
       possible testrun(userdata.connection == connection);

        conn = &base->conn[client];
        testrun(conn->base == base);
        testrun(conn->fd == -1);
        testrun(conn->type == CONNECTION_UNUSED);
        testrun(conn->listener.fd == -1);
        testrun(conn->config.connection.callback.close == NULL);
        testrun(conn->config.connection.callback.io == NULL);
        testrun(conn->config.connection.callback.userdata == NULL);
        testrun(conn->out.queue != NULL);
        testrun(conn->stats.type == 0);
        testrun(conn->in.recv_buffer_size == 0);

        conn = &base->conn[connection];
        testrun(conn->base == base);
        testrun(conn->fd == -1);
        testrun(conn->type == CONNECTION_UNUSED);
        testrun(conn->listener.fd == -1);
        testrun(conn->config.connection.callback.close == NULL);
        testrun(conn->config.connection.callback.io == NULL);
        testrun(conn->config.connection.callback.userdata == NULL);
        testrun(conn->out.queue != NULL);
        testrun(conn->stats.type == 0);
        testrun(conn->in.recv_buffer_size == 0);

        // reconnect
        client_userdata.result = false;
        client = ov_io_base_create_connection(base, client_config);
        testrun(0 < client);
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        connection = userdata.connection;
        testrun(0 < connection);

        // let SSL handshake finish
        while (!client_userdata.result) {
            testrun(loop->run(loop, TEST_RUNTIME_USECS));
        }
        testrun(loop->run(loop, TEST_RUNTIME_USECS));

        conn = &base->conn[client];
        testrun(conn->tls.handshaked == true);
        testrun(conn->timer_id == OV_TIMER_INVALID);
        conn = &base->conn[connection];
        testrun(conn->tls.handshaked == true);

        // check listener close
        testrun(ov_io_base_close(base, tcp));
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        // receive over close callback, socket is closed in base -> no recv
       possible testrun(userdata.connection == connection);

        conn = &base->conn[client];
        testrun(conn->base == base);
        testrun(conn->fd == -1);
        testrun(conn->type == CONNECTION_UNUSED);
        testrun(conn->listener.fd == -1);
        testrun(conn->config.connection.callback.close == NULL);
        testrun(conn->config.connection.callback.io == NULL);
        testrun(conn->config.connection.callback.userdata == NULL);
        testrun(conn->out.queue != NULL);
        testrun(conn->stats.type == 0);
        testrun(conn->in.recv_buffer_size == 0);

        conn = &base->conn[connection];
        testrun(conn->base == base);
        testrun(conn->fd == -1);
        testrun(conn->type == CONNECTION_UNUSED);
        testrun(conn->listener.fd == -1);
        testrun(conn->config.connection.callback.close == NULL);
        testrun(conn->config.connection.callback.io == NULL);
        testrun(conn->config.connection.callback.userdata == NULL);
        testrun(conn->out.queue != NULL);
        testrun(conn->stats.type == 0);
        testrun(conn->in.recv_buffer_size == 0);

        conn = &base->conn[tcp];
        testrun(conn->base == base);
        testrun(conn->fd == -1);
        testrun(conn->type == CONNECTION_UNUSED);
        testrun(conn->listener.fd == -1);
        testrun(conn->config.connection.callback.close == NULL);
        testrun(conn->config.connection.callback.io == NULL);
        testrun(conn->config.connection.callback.userdata == NULL);
        testrun(conn->out.queue != NULL);
        testrun(conn->stats.type == 0);
        testrun(conn->in.recv_buffer_size == 0);
    */
    testrun(NULL == ov_io_base_free(base));
    testrun(NULL == ov_event_loop_free(loop));
    unlink(local_config.socket.host);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_io_base_close() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_io_base *base = ov_io_base_create((ov_io_base_config){
        .loop = loop,
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 1000 * 1000 * 10,
    });

    testrun(base);

    testrun(ov_io_base_load_ssl_config(base, test_resource_dir));
    testrun(0 != base->domain.size);
    testrun(NULL != base->domain.array);

    ov_io_base_listener_config config = (ov_io_base_listener_config){

        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),

        .callback.userdata = &userdata,
        .callback.accept = dummy_accept,
        .callback.io = dummy_io,
        .callback.close = dummy_close};

    int server = ov_io_base_create_listener(base, config);
    testrun(-1 != server);

    size_t items = 5;
    int client[items];
    int conn[items];
    memset(client, 0, sizeof(int) * items);
    memset(conn, 0, sizeof(int) * items);

    for (size_t i = 0; i < items; i++) {

        client[i] = ov_socket_create(config.socket, true, NULL);
        testrun(0 < client[i]);
        testrun(ov_socket_ensure_nonblocking(client[i]));
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        conn[i] = userdata.connection;
    }

    testrun(!ov_io_base_close(NULL, 0));

    // not contained
    testrun(ov_io_base_close(base, client[0]));

    // check client
    ssize_t bytes = -1;
    size_t size = 1000;
    char buf[size];
    memset(buf, 0, size);

    for (size_t i = 0; i < items; i++) {

        bytes = recv(client[i], buf, size, 0);
        testrun(bytes == -1);
    }

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    testrun(ov_io_base_close(base, conn[0]));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    for (size_t i = 0; i < items; i++) {

        bytes = recv(client[i], buf, size, 0);

        if (i == 0) {
            testrun(bytes == 0);
        } else {
            testrun(bytes == -1);
        }
    }

    // check server state
    for (size_t i = 0; i < items; i++) {

        int c = conn[i];
        Connection *conn = &base->conn[c];

        switch (i) {

        case 0:
            testrun(conn->fd == -1);
            testrun(conn->listener.fd == -1);
            testrun(conn->config.connection.callback.userdata == NULL);
            testrun(conn->config.connection.callback.close == NULL);
            testrun(conn->config.connection.callback.io == NULL);
            break;
        default:
            testrun(conn->fd == c);
            testrun(conn->listener.fd == server);
            testrun(conn->config.connection.callback.userdata == &userdata);
            testrun(conn->config.connection.callback.close == dummy_close);
            testrun(conn->config.connection.callback.io == dummy_io);
        }
    }

    testrun(ov_io_base_close(base, conn[1]));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    for (size_t i = 0; i < items; i++) {

        bytes = recv(client[i], buf, size, 0);

        switch (i) {

        case 0:
        case 1:
            testrun(bytes == 0);
            break;
        default:
            testrun(bytes == -1);
        }
    }

    // check server state
    for (size_t i = 0; i < items; i++) {

        int c = conn[i];
        Connection *conn = &base->conn[c];

        switch (i) {

        case 0:
        case 1:
            testrun(conn->fd == -1);
            testrun(conn->listener.fd == -1);
            testrun(conn->config.connection.callback.userdata == NULL);
            testrun(conn->config.connection.callback.close == NULL);
            testrun(conn->config.connection.callback.io == NULL);
            break;
        default:
            testrun(conn->fd == c);
            testrun(conn->listener.fd == server);
            testrun(conn->config.connection.callback.userdata == &userdata);
            testrun(conn->config.connection.callback.close == dummy_close);
            testrun(conn->config.connection.callback.io == dummy_io);
        }
    }

    // close client socket
    close(client[2]);
    userdata.connection = 0;
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(userdata.connection == conn[2]);

    // check server state
    for (size_t i = 0; i < items; i++) {

        int c = conn[i];
        Connection *conn = &base->conn[c];

        switch (i) {

        case 0:
        case 1:
        case 2:
            testrun(conn->fd == -1);
            testrun(conn->listener.fd == -1);
            testrun(conn->config.connection.callback.userdata == NULL);
            testrun(conn->config.connection.callback.close == NULL);
            testrun(conn->config.connection.callback.io == NULL);
            break;
        default:
            testrun(conn->fd == c);
            testrun(conn->listener.fd == server);
            testrun(conn->config.connection.callback.userdata == &userdata);
            testrun(conn->config.connection.callback.close == dummy_close);
            testrun(conn->config.connection.callback.io == dummy_io);
        }
    }

    // close listener socket

    testrun(ov_io_base_close(base, server));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    for (size_t i = 0; i < items; i++) {

        // we closed client 2 above
        if (i == 2)
            continue;

        bytes = recv(client[i], buf, size, 0);
        testrun(bytes == 0);
    }

    // check server state
    for (size_t i = 0; i < items; i++) {

        int c = conn[i];
        Connection *conn = &base->conn[c];
        testrun(conn->fd == -1);
        testrun(conn->listener.fd == -1);
        testrun(conn->config.connection.callback.userdata == NULL);
        testrun(conn->config.connection.callback.close == NULL);
        testrun(conn->config.connection.callback.io == NULL);
    }

    testrun(NULL == ov_io_base_free(base));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_io_base_get_statistics() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_io_base *base = ov_io_base_create((ov_io_base_config){
        .loop = loop,
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 1000 * 1000 * 10,
    });

    testrun(base);

    testrun(ov_io_base_load_ssl_config(base, test_resource_dir));
    testrun(0 != base->domain.size);
    testrun(NULL != base->domain.array);

    ov_io_base_listener_config config = (ov_io_base_listener_config){

        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),

        .callback.userdata = &userdata,
        .callback.accept = dummy_accept,
        .callback.io = dummy_io,
        .callback.close = dummy_close};

    int server = ov_io_base_create_listener(base, config);
    testrun(-1 != server);

    ov_io_base_statistics stats = ov_io_base_get_statistics(NULL, 0);
    testrun(stats.created_usec == 0);
    stats = ov_io_base_get_statistics(base, 0);
    testrun(stats.created_usec == 0);
    stats = ov_io_base_get_statistics(base, server);
    testrun(stats.created_usec == 0);

    size_t items = 5;
    int client[items];
    int conn[items];
    memset(client, 0, items);
    memset(conn, 0, items);

    for (size_t i = 0; i < items; i++) {

        client[i] = ov_socket_create(config.socket, true, NULL);
        testrun(0 < client[i]);
        testrun(ov_socket_ensure_nonblocking(client[i]));
        testrun(loop->run(loop, TEST_RUNTIME_USECS));
        conn[i] = userdata.connection;
    }

    for (size_t i = 0; i < items; i++) {

        int c = conn[i];

        stats = ov_io_base_get_statistics(base, c);
        testrun(stats.created_usec > 0);
        testrun(stats.type == TCP);
        testrun(0 == strcmp(stats.remote.host, config.socket.host));
        testrun(stats.remote.port > 0);
        testrun(stats.recv.bytes == 0);
        testrun(stats.recv.last_usec == 0);
        testrun(stats.send.bytes == 0);
        testrun(stats.send.last_usec == 0);
    }

    char *str = "test";

    // check recv stats

    for (size_t i = 0; i < items; i++) {

        testrun((ssize_t)strlen(str) == send(client[i], str, strlen(str), 0));
        testrun(loop->run(loop, TEST_RUNTIME_USECS));

        int c = conn[i];

        stats = ov_io_base_get_statistics(base, c);
        testrun(stats.created_usec > 0);
        testrun(stats.type == TCP);
        testrun(0 == strcmp(stats.remote.host, config.socket.host));
        testrun(stats.remote.port > 0);
        testrun(stats.recv.bytes == strlen(str));
        testrun(stats.recv.last_usec > 0);
        testrun(stats.send.bytes == 0);
        testrun(stats.send.last_usec == 0);

        // check content
        testrun(0 ==
                memcmp(str, userdata.buffer->start, userdata.buffer->length));
        userdata.buffer = ov_buffer_free(userdata.buffer);
    }

    // check send stats

    size_t size = 1000;
    char buf[size];
    memset(buf, 0, size);

    ssize_t bytes = 0;

    for (size_t i = 0; i < items; i++) {

        int c = conn[i];

        testrun(ov_io_base_send(base, c,
                                (ov_memory_pointer){.start = (uint8_t *)str,
                                                    .length = strlen(str)}));

        testrun(loop->run(loop, TEST_RUNTIME_USECS));

        // check recv client
        memset(buf, 0, size);
        bytes = recv(client[i], buf, size, 0);
        testrun(bytes == (ssize_t)strlen(str));

        // check content
        testrun(0 == memcmp(buf, str, strlen(str)));

        // check stats
        stats = ov_io_base_get_statistics(base, c);
        testrun(stats.created_usec > 0);
        testrun(stats.type == TCP);
        testrun(0 == strcmp(stats.remote.host, config.socket.host));
        testrun(stats.remote.port > 0);
        testrun(stats.recv.bytes == strlen(str));
        testrun(stats.recv.last_usec > 0);
        testrun(stats.send.bytes == strlen(str));
        testrun(stats.send.last_usec > 0);
    }

    testrun(NULL == ov_io_base_free(base));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_io_base_send() {

    struct dummy_userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_io_base *base = ov_io_base_create((ov_io_base_config){
        .loop = loop,
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 1000 * 1000 * 1000});

    testrun(base);

    testrun(ov_io_base_load_ssl_config(base, test_resource_dir));
    testrun(0 != base->domain.size);
    testrun(NULL != base->domain.array);

    ov_io_base_listener_config tcp_config = (ov_io_base_listener_config){

        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),

        .callback.userdata = &userdata,
        .callback.accept = dummy_accept,
        .callback.io = dummy_io,
        .callback.close = dummy_close};

    ov_io_base_listener_config tls_config = (ov_io_base_listener_config){

        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TLS}),

        .callback.userdata = &userdata,
        .callback.accept = dummy_accept,
        .callback.io = dummy_io,
        .callback.close = dummy_close};

    tls_config.socket.type = TLS;

    ov_socket_configuration local_socket_config =
        (ov_socket_configuration){.type = LOCAL};

    ssize_t bytes = snprintf(local_socket_config.host, OV_HOST_NAME_MAX,
                             "%s/local12313", test_resource_dir);
    unlink(local_socket_config.host);

    testrun(bytes > 0);
    testrun(bytes < OV_HOST_NAME_MAX);

    ov_io_base_listener_config local_config = (ov_io_base_listener_config){

        .socket = local_socket_config,
        .callback.userdata = &userdata,
        .callback.accept = dummy_accept,
        .callback.io = dummy_io,
        .callback.close = dummy_close};

    int tcp = ov_io_base_create_listener(base, tcp_config);
    testrun(-1 != tcp);

    int tls = ov_io_base_create_listener(base, tls_config);
    testrun(-1 != tls);

    int local = ov_io_base_create_listener(base, local_config);
    testrun(-1 != local);

    // check tcp

    int conn = 0;
    int client = ov_socket_create(tcp_config.socket, true, NULL);
    testrun(0 < client);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    conn = userdata.connection;

    char *str = "test";
    size_t size = 1000;
    char buf[size];
    memset(buf, 0, size);

    // state before send
    testrun(-1 == recv(client, buf, size, 0));

    // send
    testrun(ov_io_base_send(
        base, conn,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // check recv client
    memset(buf, 0, size);
    bytes = recv(client, buf, size, 0);
    testrun(bytes == (ssize_t)strlen(str));
    // check content
    testrun(0 == memcmp(buf, str, strlen(str)));

    // send on unkown socket
    testrun(!ov_io_base_send(
        base, client,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));

    // send on listener socket
    testrun(!ov_io_base_send(
        base, tcp,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));

    // close tcp conn
    testrun(ov_io_base_close(base, conn));
    close(client);

    // check local

    conn = 0;
    client = ov_socket_create(local_config.socket, true, NULL);
    testrun(0 < client);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    conn = userdata.connection;

    // state before send
    testrun(-1 == recv(client, buf, size, 0));

    // send
    testrun(ov_io_base_send(
        base, conn,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // check recv client
    memset(buf, 0, size);
    bytes = recv(client, buf, size, 0);
    testrun(bytes == (ssize_t)strlen(str));
    // check content
    testrun(0 == memcmp(buf, str, strlen(str)));

    // send on unkown socket
    testrun(!ov_io_base_send(
        base, client,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));

    // send on listener socket
    testrun(!ov_io_base_send(
        base, tcp,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));

    // close local conn
    testrun(ov_io_base_close(base, conn));
    close(client);

    // check TLS

    conn = 0;
    client = ov_socket_create(tls_config.socket, true, NULL);
    testrun(0 < client);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    conn = userdata.connection;

    // state before send
    testrun(-1 == recv(client, buf, size, 0));

    // send not handshaked (ignore)
    testrun(!ov_io_base_send(
        base, conn,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // handshake SSL

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL *ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);

    /* start handshaking */
    int errorcode = -1;
    int err = 0;
    int r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // state before send, we get some handshake response and read SSL empty

    bytes = 0;
    while (bytes != -1) {

        memset(buf, 0, size);
        bytes = SSL_read(ssl, buf, size);
        if (bytes != -1)
            fprintf(stdout, "READ HANDSHAKE %.*s", (int)bytes, buf);
    }

    testrun(-1 == recv(client, buf, size, 0));

    // send handshaked
    testrun(ov_io_base_send(
        base, conn,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // check recv client
    memset(buf, 0, size);
    bytes = SSL_read(ssl, buf, size);
    testrun(bytes == (ssize_t)strlen(str));
    // check content
    testrun(0 == memcmp(buf, str, strlen(str)));

    // send on unkown socket
    testrun(!ov_io_base_send(
        base, client,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));

    // send on listener socket
    testrun(!ov_io_base_send(
        base, tcp,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));

    // repeat send
    testrun(ov_io_base_send(
        base, conn,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // check recv client
    memset(buf, 0, size);
    bytes = SSL_read(ssl, buf, size);
    testrun(bytes == (ssize_t)strlen(str));
    // check content
    testrun(0 == memcmp(buf, str, strlen(str)));

    // limit send
    base->config.limit.max_send = 2;

    // repeat send
    testrun(ov_io_base_send(
        base, conn,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // check recv client
    memset(buf, 0, size);
    bytes = SSL_read(ssl, buf, size);
    testrun(bytes == 2);
    // check content
    testrun(0 == memcmp(buf, "te", 2));

    // check recv client
    memset(buf, 0, size);
    bytes = SSL_read(ssl, buf, size);
    testrun(bytes == 2);
    // check content
    testrun(0 == memcmp(buf, "st", 2));

    // check recv client
    memset(buf, 0, size);
    bytes = SSL_read(ssl, buf, size);
    testrun(bytes == -1);

    // close local conn
    testrun(ov_io_base_close(base, conn));

    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    testrun(NULL == ov_io_base_free(base));
    testrun(NULL == ov_event_loop_free(loop));
    unlink(local_socket_config.host);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_sni() {

    struct dummy_userdata userdata = {0};

    const EVP_MD *digest_func = EVP_sha256();

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_io_base *base = ov_io_base_create((ov_io_base_config){
        .loop = loop,
        .debug = test_debug_log,
        .timer.accept_to_io_timeout_usec = 1000 * 1000 * 1000});

    testrun(base);

    testrun(ov_io_base_load_ssl_config(base, test_resource_dir));
    testrun(0 != base->domain.size);
    testrun(NULL != base->domain.array);

    fprintf(stdout, "Domains %zu\n", base->domain.size);

    testrun(find_domain(base, (uint8_t *)TEST_DOMAIN_NAME,
                        strlen(TEST_DOMAIN_NAME)));
    testrun(find_domain(base, (uint8_t *)TEST_DOMAIN_NAME_ONE,
                        strlen(TEST_DOMAIN_NAME_ONE)));
    testrun(find_domain(base, (uint8_t *)TEST_DOMAIN_NAME_TWO,
                        strlen(TEST_DOMAIN_NAME_TWO)));

    ov_io_base_listener_config config = (ov_io_base_listener_config){

        .socket = ov_socket_load_dynamic_port(
            (ov_socket_configuration){.host = "127.0.0.1", .type = TCP}),

        .callback.userdata = &userdata,
        .callback.accept = dummy_accept,
        .callback.io = dummy_io,
        .callback.close = dummy_close};

    config.socket.type = TLS;

    int server = ov_io_base_create_listener(base, config);
    testrun(-1 != server);

    char *fingerprint[base->domain.size];
    memset(fingerprint, 0, sizeof(char *));

    for (size_t i = 0; i < base->domain.size; i++) {
        fingerprint[i] = fingerprint_from_path(
            base->domain.array[i].config.certificate.cert, digest_func);
        testrun(fingerprint[i]);
    }

    /* create some client connection */
    int client = ov_socket_create(config.socket, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    int connection = userdata.connection;

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL *ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);

    /* start handshaking */
    int errorcode = -1;
    int err = 0;
    int r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    /* STATE
     *  - no default set
     *  - request without domain header send
     * EXPECT
     *  - certificate of domain.array[0] returned
     */

    testrun(check_cert(ssl, digest_func, fingerprint[0]));

    if (test_debug_log)
        fprintf(stdout, "request without header without default - done");

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    // reset
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    close(client);

    testrun(loop->run(loop, TEST_RUNTIME_USECS));

    Connection *conn = &base->conn[connection];
    testrun(conn->fd == -1);
    testrun(conn->listener.fd == -1);
    testrun(conn->config.connection.callback.userdata == NULL);
    testrun(conn->config.connection.callback.close == NULL);
    testrun(conn->config.connection.callback.io == NULL);

    // set default domain to two.test
    base->domain.standard = 2;

    // reconnect
    client = ov_socket_create(config.socket, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);

    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    /* STATE
     *  - default set
     *  - request without domain header send
     * EXPECT
     *  - certificate of domain.array[2] returned
     */

    testrun(check_cert(ssl, digest_func, fingerprint[2]));

    if (test_debug_log)
        fprintf(stdout, "request without header but default set - done");

    // reset
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    // reconnect
    client = ov_socket_create(config.socket, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(
                     ssl, (char *)base->domain.array[0].config.name.start));
    const char *ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 == strcmp(ptr, "openvocs.test"));
    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    /* STATE
     *  - default set
     *  - request with domain header for openvocs.test
     * EXPECT
     *  - certificate of domain.array[0] returned
     */

    testrun(check_cert(ssl, digest_func, fingerprint[0]));

    if (test_debug_log)
        fprintf(stdout, "request with header openvocs.test - done");

    // reset
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    // reconnect
    client = ov_socket_create(config.socket, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(
                     ssl, (char *)base->domain.array[1].config.name.start));
    ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 == strcmp(ptr, "one.test"));
    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    /* STATE
     *  - default set
     *  - request with domain header for one.test
     * EXPECT
     *  - certificate of domain.array[1] returned
     */

    testrun(check_cert(ssl, digest_func, fingerprint[1]));

    if (test_debug_log)
        fprintf(stdout, "request with header one.test - done");

    // reset
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    client = ov_socket_create(config.socket, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(
                     ssl, (char *)base->domain.array[2].config.name.start));
    ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 == strcmp(ptr, "two.test"));
    r = run_client_handshake(loop, ssl, &err, &errorcode);
    testrun(r == 1);

    /* STATE
     *  - default set
     *  - request with domain header for two.test
     * EXPECT
     *  - certificate of domain.array[2] returned
     */

    testrun(check_cert(ssl, digest_func, fingerprint[2]));

    if (test_debug_log)
        fprintf(stdout, "request with header two.test - done");

    // reset
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    client = ov_socket_create(config.socket, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(ssl, "unconfigured.test"));
    ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 == strcmp(ptr, "unconfigured.test"));
    r = run_client_handshake(loop, ssl, &err, &errorcode);

    if (test_debug_log)
        fprintf(stdout, "request with header unconfigured.test - done");

    // expect abort with close
    testrun(r == -1);

    // reset
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    // check with non ascii name
    // here we do change the host name, but NOT the cerificate

    ov_domain *domain = &base->domain.array[1];
    memset(domain->config.name.start, 0, domain->config.name.length);

    // mix of different length unicode chars

    domain->config.name.start[0] = 0xCE; // 2 byte UTF8 alpha 0xCE91
    domain->config.name.start[1] = 0x91;
    domain->config.name.start[2] = 0xCE; // 2 byte UTF8 beta 0xCE92
    domain->config.name.start[3] = 0x92;
    domain->config.name.start[4] = 0xCE; // 2 byte UTF8 gamma 0xCE93
    domain->config.name.start[5] = 0x93;
    domain->config.name.start[6] =
        0xF0; // 4 byte UTF8 Gothic Letter Ahsa U+10330
    domain->config.name.start[7] = 0x90;
    domain->config.name.start[8] = 0x8C;
    domain->config.name.start[9] = 0xB0;
    domain->config.name.start[10] =
        0xF0; // 4 byte UTF8 Gothic Letter Bairkan U+10331
    domain->config.name.start[11] = 0x90;
    domain->config.name.start[12] = 0x8C;
    domain->config.name.start[13] = 0xb1;
    domain->config.name.start[14] =
        0xE3; // 3 byte UTF8 Cjk unified ideograph U+3400
    domain->config.name.start[15] = 0x90;
    domain->config.name.start[16] = 0x80;
    domain->config.name.start[17] = 0xCE; // 2 byte UTF8 delta 0xCE94
    domain->config.name.start[18] = 0x94;
    domain->config.name.start[19] = 'x'; // 1 byte char x
    domain->config.name.length = 20;

    client = ov_socket_create(config.socket, true, NULL);
    testrun(client > -1);
    testrun(ov_socket_ensure_nonblocking(client));
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    testrun(1 == SSL_set_fd(ssl, client));
    SSL_set_connect_state(ssl);
    SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
    testrun(1 == SSL_set_tlsext_host_name(ssl, domain->config.name.start));
    ptr = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    testrun(0 ==
            memcmp(ptr, domain->config.name.start, domain->config.name.length));
    r = run_client_handshake(loop, ssl, &err, &errorcode);

    testrun(r == 1);

    /* STATE
     *  - default set
     *  - request with domain header for utf8 valid domain
     * EXPECT
     *  - certificate of domain.array[1] returned
     */

    testrun(check_cert(ssl, digest_func, fingerprint[1]));

    // reset
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    close(client);

    for (size_t i = 0; i < base->domain.size; i++) {
        fingerprint[i] = ov_data_pointer_free(fingerprint[i]);
    }

    testrun(loop->run(loop, TEST_RUNTIME_USECS));
    testrun(NULL == ov_io_base_free(base));
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

OV_TEST_RUN("ov_io_base", domains_init, test_ov_io_base_create,
            test_ov_io_base_free, test_ov_io_base_debug,
            test_ov_io_base_load_ssl_config, test_ov_io_base_create_listener,
            test_ov_io_base_create_connection, test_ov_io_base_close,
            test_ov_io_base_get_statistics, test_ov_io_base_send, check_sni,
            domains_deinit);