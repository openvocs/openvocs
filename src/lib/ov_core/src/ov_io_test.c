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
        @file           ov_io_test.c
        @author         Markus Töpfer

        @date           2024-12-20


        ------------------------------------------------------------------------
*/
#include "ov_io.c"
#include <ov_test/testrun.h>

#include <ov_test/ov_test.h>

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

#define flag_accept 0x0001
#define flag_io 0x0010
#define flag_close 0x0100
#define flag_connected 0x1000

/*----------------------------------------------------------------------------*/

typedef struct dummy_userdata {

  int flag;
  int socket;
  int listener;
  char *domain;
  ov_buffer *data;

} dummy_userdata;

/*----------------------------------------------------------------------------*/

static void dummy_userdata_clear(dummy_userdata *data) {

  data->flag = 0;
  data->socket = 0;
  data->listener = 0;
  data->domain = ov_data_pointer_free(data->domain);
  data->data = ov_buffer_free(data->data);
  return;
}

/*----------------------------------------------------------------------------*/

static bool dummy_accept(void *userdata, int listener, int socket) {

  dummy_userdata *data = (dummy_userdata *)userdata;
  data->flag = flag_accept;
  data->listener = listener;
  data->socket = socket;
  return true;
}

/*----------------------------------------------------------------------------*/

static bool dummy_io(void *userdata, int connection, const char *domain,
                     const ov_memory_pointer buffer) {

  dummy_userdata *data = (dummy_userdata *)userdata;
  dummy_userdata_clear(data);

  data->flag = flag_io;

  data->socket = connection;
  if (domain)
    data->domain = ov_string_dup(domain);

  data->data = ov_buffer_create(buffer.length);
  ov_buffer_set(data->data, buffer.start, buffer.length);
  return true;
}

/*----------------------------------------------------------------------------*/

static void dummy_close(void *userdata, int socket) {

  dummy_userdata *data = (dummy_userdata *)userdata;
  dummy_userdata_clear(data);

  data->flag = flag_close;
  data->socket = socket;
  return;
}

/*----------------------------------------------------------------------------*/

static void dummy_connected(void *userdata, int socket) {

  dummy_userdata *data = (dummy_userdata *)userdata;
  dummy_userdata_clear(data);

  data->flag = flag_connected;
  data->socket = socket;
  return;
}

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
        *errorcode = ERR_get_error();
        run = false;
        break;

      case SSL_ERROR_WANT_ACCEPT:
        run = false;
        break;
      }
      break;
    }

    // fprintf(stdout, "\nr %i n %i\n", r, n);
  }

  *err = n;
  return r;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_io_create() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_io_config config = {.loop = loop};

  ov_io *io = ov_io_create(config);
  testrun(ov_io_cast(io));
  testrun(0 == io->domain.size);
  testrun(NULL == ov_io_free(io));

  strncpy(config.domain.path, test_resource_dir, PATH_MAX);

  io = ov_io_create(config);
  testrun(ov_io_cast(io));
  testrun(3 == io->domain.size);

  testrun(NULL == ov_io_free(io));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_io_free() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_io_config config = {.loop = loop};
  strncpy(config.domain.path, test_resource_dir, PATH_MAX);

  ov_io *io = ov_io_create(config);
  testrun(ov_io_cast(io));
  testrun(3 == io->domain.size);

  testrun(NULL == ov_io_free(io));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_io_open_listener() {

  dummy_userdata data = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_io_config config = {.loop = loop};
  strncpy(config.domain.path, test_resource_dir, PATH_MAX);

  ov_io *io = ov_io_create(config);
  testrun(ov_io_cast(io));
  testrun(3 == io->domain.size);

  // (1) check plain TCP socket

  ov_io_socket_config socket_config = {
      .socket = ov_socket_load_dynamic_port(
          (ov_socket_configuration){.type = TCP, .host = "localhost"}),
      .callbacks.userdata = &data,
      .callbacks.accept = dummy_accept,
      .callbacks.io = dummy_io,
      .callbacks.close = dummy_close,
      .callbacks.connected = dummy_connected};

  int server = ov_io_open_listener(io, socket_config);
  testrun(-1 != server);

  int client = ov_socket_create(socket_config.socket, true, NULL);
  ov_socket_ensure_nonblocking(client);
  ov_event_loop_run(loop, OV_RUN_ONCE);

  testrun(data.flag & flag_accept);
  testrun(data.listener = server);
  int conn = data.socket;

  // check working connection

  size_t size = 1000;
  char buf[size];
  memset(buf, 0, size);

  testrun(-1 == recv(client, buf, size, 0));

  testrun(ov_io_send(
      io, conn, (ov_memory_pointer){.start = (uint8_t *)"test", .length = 4}));

  ssize_t bytes = -1;
  while (bytes == -1) {
    ov_event_loop_run(loop, OV_RUN_ONCE);
    bytes = recv(client, buf, size, 0);
  }

  testrun(0 == strcmp("test", buf));

  // check close conn

  testrun(ov_io_close(io, conn));
  testrun(0 == recv(client, buf, size, 0));

  client = ov_socket_create(socket_config.socket, true, NULL);
  ov_socket_ensure_nonblocking(client);
  ov_event_loop_run(loop, OV_RUN_ONCE);

  testrun(data.flag & flag_accept);
  testrun(data.listener = server);
  conn = data.socket;

  // check client close
  close(client);
  testrun(0 == recv(conn, buf, size, 0));

  // (2) check TLS socket

  socket_config = (ov_io_socket_config){
      .socket = ov_socket_load_dynamic_port(
          (ov_socket_configuration){.type = TLS, .host = "localhost"}),
      .callbacks.userdata = &data,
      .callbacks.accept = dummy_accept,
      .callbacks.io = dummy_io,
      .callbacks.close = dummy_close,
      .callbacks.connected = dummy_connected};

  server = ov_io_open_listener(io, socket_config);
  testrun(-1 != server);

  client = ov_socket_create(socket_config.socket, true, NULL);
  ov_socket_ensure_nonblocking(client);
  ov_event_loop_run(loop, OV_RUN_ONCE);

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

  // check working connection

  memset(buf, 0, size);

  testrun(-1 == SSL_read(ssl, buf, size));

  testrun(4 == SSL_write(ssl, "test", 4));

  while (!data.data) {
    ov_event_loop_run(loop, OV_RUN_ONCE);
  }

  testrun(0 == memcmp("test", data.data->start, 4));

  SSL_CTX_free(ctx);
  SSL_free(ssl);

  // (2) check certificate based access socket

  socket_config = (ov_io_socket_config){
      .socket = ov_socket_load_dynamic_port(
          (ov_socket_configuration){.type = TLS, .host = "localhost"}),
      .callbacks.userdata = &data,
      .callbacks.accept = dummy_accept,
      .callbacks.io = dummy_io,
      .callbacks.close = dummy_close,
      .callbacks.connected = dummy_connected};

  strncpy(socket_config.ssl.domain, TEST_DOMAIN_NAME_ONE, PATH_MAX);
  strncpy(socket_config.ssl.certificate.cert, OV_TEST_CERT_ONE, PATH_MAX);
  strncpy(socket_config.ssl.certificate.key, OV_TEST_CERT_ONE_KEY, PATH_MAX);
  strncpy(socket_config.ssl.ca.file, OV_TEST_CERT_ONE, PATH_MAX);
  strncpy(socket_config.ssl.ca.client_ca, OV_TEST_CERT_ONE, PATH_MAX);

  server = ov_io_open_listener(io, socket_config);
  testrun(-1 != server);

  client = ov_socket_create(socket_config.socket, true, NULL);
  ov_socket_ensure_nonblocking(client);
  ov_event_loop_run(loop, OV_RUN_ONCE);

  ctx = SSL_CTX_new(TLS_client_method());
  SSL_CTX_set_client_hello_cb(ctx, dummy_client_hello_cb, NULL);
  testrun(1 == SSL_CTX_load_verify_locations(ctx, OV_TEST_CERT_ONE, NULL));
  testrun(1 == SSL_CTX_use_certificate_file(ctx, OV_TEST_CERT_ONE,
                                            SSL_FILETYPE_PEM));
  testrun(1 == SSL_CTX_use_PrivateKey_file(ctx, OV_TEST_CERT_ONE_KEY,
                                           SSL_FILETYPE_PEM));
  SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);
  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
  SSL_CTX_set_verify_depth(ctx, 1);
  SSL_CTX_set_client_hello_cb(ctx, tls_client_hello_cb, NULL);
  SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);

  ssl = SSL_new(ctx);
  testrun(1 == SSL_set_fd(ssl, client));
  SSL_set_connect_state(ssl);

  errorcode = -1;
  err = 0;
  r = run_client_handshake(loop, ssl, &err, &errorcode);
  testrun(r == 1);
  testrun(loop->run(loop, TEST_RUNTIME_USECS));

  // check working connection

  memset(buf, 0, size);

  testrun(-1 == SSL_read(ssl, buf, size));

  testrun(4 == SSL_write(ssl, "test", 4));

  while (!data.data) {
    ov_event_loop_run(loop, OV_RUN_ONCE);
  }

  testrun(0 == memcmp("test", data.data->start, 4));

  SSL_CTX_free(ctx);
  SSL_free(ssl);

  dummy_userdata_clear(&data);

  testrun(NULL == ov_io_free(io));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_io_open_connection() {

  dummy_userdata data = {0};
  dummy_userdata client_data = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_io_config config = {.loop = loop};
  strncpy(config.domain.path, test_resource_dir, PATH_MAX);

  ov_io *io = ov_io_create(config);
  testrun(ov_io_cast(io));
  testrun(3 == io->domain.size);

  // (1) check plain TCP socket

  ov_io_socket_config socket_config = {
      .socket = ov_socket_load_dynamic_port(
          (ov_socket_configuration){.type = TCP, .host = "localhost"}),
      .callbacks.userdata = &data,
      .callbacks.accept = dummy_accept,
      .callbacks.io = dummy_io,
      .callbacks.close = dummy_close,
      .callbacks.connected = dummy_connected};

  int server_tcp = ov_io_open_listener(io, socket_config);
  testrun(-1 != server_tcp);

  int client_tcp = ov_io_open_connection(
      io, (ov_io_socket_config){.socket = socket_config.socket,
                                .callbacks = socket_config.callbacks});

  ov_event_loop_run(loop, OV_RUN_ONCE);
  testrun(-1 != client_tcp);

  testrun(ov_io_close(io, client_tcp));

  // (2) check certificate based authentication

  socket_config = (ov_io_socket_config){
      .socket = ov_socket_load_dynamic_port(
          (ov_socket_configuration){.type = TLS, .host = "localhost"}),
      .callbacks.userdata = &data,
      .callbacks.accept = dummy_accept,
      .callbacks.io = dummy_io,
      .callbacks.close = dummy_close,
      .callbacks.connected = NULL};

  strncpy(socket_config.ssl.domain, TEST_DOMAIN_NAME_ONE, PATH_MAX);
  strncpy(socket_config.ssl.certificate.cert, OV_TEST_CERT_ONE, PATH_MAX);
  strncpy(socket_config.ssl.certificate.key, OV_TEST_CERT_ONE_KEY, PATH_MAX);
  strncpy(socket_config.ssl.ca.file, OV_TEST_CERT_ONE, PATH_MAX);
  strncpy(socket_config.ssl.ca.client_ca, OV_TEST_CERT_ONE, PATH_MAX);

  int server = ov_io_open_listener(io, socket_config);
  testrun(-1 != server);

  ov_io_socket_config client_config =
      (ov_io_socket_config){.auto_reconnect = true,
                            .socket = socket_config.socket,
                            .callbacks.userdata = &client_data,
                            .callbacks.accept = dummy_accept,
                            .callbacks.io = dummy_io,
                            .callbacks.close = dummy_close,
                            .callbacks.connected = dummy_connected};

  strncpy(client_config.ssl.domain, TEST_DOMAIN_NAME_ONE, PATH_MAX);
  strncpy(client_config.ssl.certificate.cert, OV_TEST_CERT_ONE, PATH_MAX);
  strncpy(client_config.ssl.certificate.key, OV_TEST_CERT_ONE_KEY, PATH_MAX);
  strncpy(client_config.ssl.ca.file, OV_TEST_CERT_ONE, PATH_MAX);

  int client = ov_io_open_connection(io, client_config);
  testrun(-1 != client);

  // check autoreconnect

  ov_io_close(io, server);
  ov_event_loop_run(loop, OV_RUN_ONCE);

  size_t size = 1000;
  char buf[size];
  memset(buf, 0, size);

  testrun(client_data.flag == flag_close);
  testrun(client_data.socket == client);
  dummy_userdata_clear(&client_data);

  // reopen server
  server = ov_io_open_listener(io, socket_config);
  testrun(-1 != server);

  while (!(flag_connected & client_data.flag)) {

    ov_event_loop_run(loop, OV_RUN_ONCE);
  }
  client = client_data.socket;

  Connection *conn = ov_dict_get(io->connections, (void *)(intptr_t)client);

  // handshaking is part of the connections,
  // but done after the initial callback
  while (!conn->tls.handshaked) {
    ov_event_loop_run(loop, OV_RUN_ONCE);
  }

  testrun(ov_io_send(
      io, client,
      (ov_memory_pointer){.start = (uint8_t *)"test1234", .length = 8}));

  while (!data.data) {
    ov_event_loop_run(loop, OV_RUN_ONCE);
  }

  testrun(0 == memcmp(data.data->start, "test1234", 8))

      dummy_userdata_clear(&data);

  testrun(NULL == ov_io_free(io));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST EXECUTION                                                  #EXEC
 *
 *      ------------------------------------------------------------------------
 */

OV_TEST_RUN("ov_io", domains_init,

            test_ov_io_create, test_ov_io_free,

            test_ov_io_open_listener, test_ov_io_open_connection,

            domains_deinit);