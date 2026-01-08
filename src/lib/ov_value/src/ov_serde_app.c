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

#include "../include/ov_serde_app.h"

#include <fcntl.h>
#include <ov_base/ov_string.h>
#include <ov_core/ov_io_base.h>

#include "ov_base/ov_socket.h"

/*----------------------------------------------------------------------------*/

#define MAGIC_BYTES 0x41e3ab68

typedef struct {
  uint32_t reconnect_interval_secs;

  void *additional;

  void (*cb_closed)(int sckt, void *additional);
  void (*cb_reconnected)(int sckt, void *additional);
  void (*cb_accepted)(int sckt, void *additional);

  uint64_t data_type;
  void (*handler)(void *data, int socket, void *additional);

  ov_serde *serde;

  ov_list *socket_parameters;

  int log_fd;

} SerdeData;

/*----------------------------------------------------------------------------*/

struct ov_serde_app {
  uint32_t magic_bytes;

  SerdeData data;
  ov_io_base *io_base;
};

/*----------------------------------------------------------------------------*/

static ov_serde_app *as_serde_app(void *vptr) {
  ov_serde_app *app = vptr;

  if ((!ov_ptr_valid(app, "No serde app - 0 pointer")) ||
      (!ov_cond_valid(MAGIC_BYTES == app->magic_bytes,
                      "Not a serde app - wrong magic bytes"))) {
    return 0;

  } else {
    return app;
  }
}

/*----------------------------------------------------------------------------*/

static ov_io_base *get_io_base(ov_serde_app *self) {
  if (!ov_ptr_valid(as_serde_app(self), "No Serde app")) {
    return 0;
  } else {
    return self->io_base;
  }
}

/*----------------------------------------------------------------------------*/

static bool initialize_serde_data(SerdeData *data,
                                  ov_serde_app_configuration cfg) {
  if ((!ov_ptr_valid(cfg.serde, "No serde object")) ||
      (!ov_ptr_valid(data, "No serde data to initialize"))) {
    return 0;

  } else {
    data->serde = cfg.serde;

    data->cb_closed = cfg.cb_closed;
    data->cb_reconnected = cfg.cb_reconnected;
    data->cb_accepted = cfg.cb_accepted;
    data->additional = cfg.additional;
    data->reconnect_interval_secs = cfg.reconnect_interval_secs;

    return data;
  }
}

/*----------------------------------------------------------------------------*/

static int open_log_fd_at(char const *path) {
  if (0 == path) {
    ov_log_error("Cannot open serde log file - path is invalid");
    return -1;
  } else {
    int fd =
        open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, S_IRUSR | S_IWUSR);

    if (0 > fd) {
      ov_log_error("Could not open serde log file %s: %s",
                   ov_string_sanitize(path), strerror(errno));

      return -1;
    } else {
      ov_log_info("Opened serde log file %s", ov_string_sanitize(path));
      return fd;
    }
  }
}

/*----------------------------------------------------------------------------*/

static int open_log_fd(char const *app_name, bool enable_logging) {
  if (enable_logging) {
    char file_name[PATH_MAX] = {0};

    snprintf(file_name, sizeof(file_name), "/tmp/%s-%i_serde_in.log",
             ov_string_sanitize(app_name), getpid());

    return open_log_fd_at(file_name);

  } else {
    UNUSED(app_name);
    return -1;
  }
}

/*----------------------------------------------------------------------------*/

ov_serde_app *ov_serde_app_free(ov_serde_app *self);

static ov_serde_app *
create_serde_app_from_config(ov_io_base_config io_base_cfg,
                             ov_serde_app_configuration serde_app_config) {

  ov_serde_app *app = calloc(1, sizeof(ov_serde_app));
  app->magic_bytes = MAGIC_BYTES;
  app->io_base = ov_io_base_create(io_base_cfg);

  if ((!initialize_serde_data(&app->data, serde_app_config)) ||
      (!ov_ptr_valid(app->io_base, "Could not create io_base object"))) {
    return ov_serde_app_free(app);
  } else {
    app->data.log_fd = open_log_fd(io_base_cfg.name, serde_app_config.log_io);
    return app;
  }
}

/*----------------------------------------------------------------------------*/

static bool initialize_app_name(char *dest, char const *name) {
  size_t name_length = ov_string_len(name);

  if (0 == dest) {
    ov_log_error("Null pointer: no dest given");
    return false;
  } else if (0 == name) {
    ov_log_error("No app name given");
    return false;
  } else if (OV_IO_BASE_NAME_MAX < name_length + 1) {
    ov_log_error("app name exceeds %i characters\n", OV_IO_BASE_NAME_MAX);
    return false;
  } else {
    strncpy(dest, name, OV_IO_BASE_NAME_MAX);
    dest[OV_IO_BASE_NAME_MAX - 1] = 0;
    return true;
  }
}

/*----------------------------------------------------------------------------*/

ov_serde_app *ov_serde_app_create(char const *name, ov_event_loop *loop,
                                  ov_serde_app_configuration cfg) {
  ov_io_base_config io_base_cfg = {

      .debug = false,

      // We dont need socket buffering - Serde buffers if need be
      .dont_buffer = true,

      // .name initialized later
      .loop = loop,

      .limit.max_sockets = 1000,
      .limit.max_send = 0,
      .limit.thread_lock_timeout_usec = 0,

      .timer.io_timeout_usec = 0,
      .timer.accept_to_io_timeout_usec =
          cfg.accept_to_io_timeout_secs * 1000 * 1000,
      .timer.reconnect_interval_usec =
          cfg.reconnect_interval_secs * 1000 * 1000,

  };

  if (!initialize_app_name(io_base_cfg.name, name)) {
    return 0;
  } else {
    ov_serde_app *app = create_serde_app_from_config(io_base_cfg, cfg);
    return app;
  }
}

/*----------------------------------------------------------------------------*/

ov_serde_app *ov_serde_app_free(ov_serde_app *self) {
  if (0 == self) {
    return self;
  } else {
    self->io_base = ov_io_base_free(self->io_base);
    return ov_free(self);
  }
}

/*****************************************************************************
                                REGISTER HANDLER
 ****************************************************************************/

static bool is_handler_already_set(ov_serde_app *self) {
  if (0 == self) {
    return false;
  } else if (0 != self->data.handler) {
    ov_log_warning("Overwriting handler for parsed Serde data");
    return true;
  } else {
    return false;
  }
}

/*----------------------------------------------------------------------------*/

bool ov_serde_app_register_handler(ov_serde_app *self, uint64_t data_type,
                                   void (*handler)(void *data, int socket,
                                                   void *additional)) {
  if (0 == as_serde_app(self)) {
    return false;
  } else {
    is_handler_already_set(self);

    self->data.data_type = data_type;
    self->data.handler = handler;

    return true;
  }
}

/*****************************************************************************
                           Control logging SERDE I/O
 ****************************************************************************/

bool ov_serde_app_enable_logging(ov_serde_app *self, char const *path) {
  //            open_log_fd(io_base_cfg.name, serde_app_config.log_io);
  if (0 == as_serde_app(self)) {
    ov_log_error("Cannot enable logging for SERDE app - invalid serde app");
    return false;
  } else if (0 == path) {
    ov_log_error("Cannot enable logging for SERDE app - invalid path given");
    return false;
  } else if (-1 < self->data.log_fd) {
    ov_log_error("Cannot enable logging for SERDE app - already enabled");
    return false;
  } else {
    self->data.log_fd = open_log_fd_at(path);
    return -1 < self->data.log_fd;
  }
}

/*----------------------------------------------------------------------------*/

bool ov_serde_app_disable_logging(ov_serde_app *self) {
  if (0 != as_serde_app(self)) {
    if (-1 < self->data.log_fd) {
      close(self->data.log_fd);
      self->data.log_fd = -1;
    }

    return true;

  } else {
    ov_log_error("Cannot disable logging for SERDE app - invalid serde app");
    return false;
  }
}

/*****************************************************************************
                                       IO
 ****************************************************************************/

static void set_bool(bool *b, bool value) {
  if (0 == b) {
    return;
  } else {
    *b = value;
  }
}

/*----------------------------------------------------------------------------*/

static void *get_more_data(ov_serde *serde, uint64_t expected_data_type,
                           bool *ok_p) {
  ov_result res = {0};
  ov_serde_data datum = ov_serde_pop_datum(serde, &res);

  if (OV_SERDE_DATA_NO_MORE.data_type == datum.data_type) {
    ov_log_info("No more parsed elements");

    ov_result_clear(&res);
    set_bool(ok_p, true);
    return 0;

  } else if (OV_ERROR_NOERROR != res.error_code) {
    ov_log_error("Could not parse input: %s", ov_result_get_message(res));

    ov_result_clear(&res);
    set_bool(ok_p, false);
    return 0;

  } else if (expected_data_type != datum.data_type) {
    ov_log_error("Expected data of type %" PRIu64 " but got %" PRIu64
                 " - potential mem leak: We cannot free the "
                 "datum we got",
                 expected_data_type, datum.data_type);

    ov_result_clear(&res);
    set_bool(ok_p, false);
    return 0;

  } else {
    ov_result_clear(&res);
    set_bool(ok_p, true);
    return datum.data;
  }
}

/*----------------------------------------------------------------------------*/

static bool process_parsed_data(ov_serde *serde,
                                void (*handler)(void *, int, void *), int s,
                                void *additional, uint64_t expected_data_type) {
  void *data = 0;

  if (0 == handler) {
    ov_log_error("No handler given");
    return false;
  } else {
    bool ok_p = true;

    data = get_more_data(serde, expected_data_type, &ok_p);

    while (0 != data) {
      handler(data, s, additional);
      data = get_more_data(serde, expected_data_type, &ok_p);
    };

    return ok_p;
  }
}

/*----------------------------------------------------------------------------*/

static bool parse_data(ov_serde *serde, ov_buffer const *buffer,
                       void (*handler)(void *, int, void *), int s,
                       void *additional, uint64_t expected_data_type) {
  ov_result res = {0};

  switch (ov_serde_add_raw(serde, buffer, &res)) {
  case OV_SERDE_ERROR:
    ov_log_error("Error parsing input: %s", ov_result_get_message(res));
    ov_result_clear(&res);
    return false;

  case OV_SERDE_PROGRESS:
    ov_result_clear(&res);
    return true;

  case OV_SERDE_END:
    ov_result_clear(&res);
    return process_parsed_data(serde, handler, s, additional,
                               expected_data_type);

  default:
    OV_PANIC("INvalid ov_serde_state");
  };
}

/*****************************************************************************
                                LOG SERDE IN/OUT
 ****************************************************************************/

static void log_time(int i) {
  char now_str[30];
  ssize_t now_len = snprintf(now_str, 20, "%ld", time(0));

  if (now_len > -1) {
    write(i, now_str, (size_t)now_len);
  }
}

/*----------------------------------------------------------------------------*/

static void log_buffer(ov_serde_app *self, ov_buffer const *buffer,
                       char const *prefix) {
  if ((!ov_ptr_valid(self, "Require valid SerdeData")) ||
      (0 == ov_buffer_cast(buffer))) {
    ov_log_error("Cannot log serde data: Invalid args received: serde: "
                 "%p, "
                 "buffer: %s",
                 self, buffer);
    return;

  } else if (0 > self->data.log_fd) {
    return;

  } else {
    write(self->data.log_fd, "\n\n\n", 3);
    log_time(self->data.log_fd);

    if (0 != prefix) {
      write(self->data.log_fd, prefix, strlen(prefix));
    }

    write(self->data.log_fd, buffer->start, buffer->length);
  }
}

/*----------------------------------------------------------------------------*/

static void log_recv(ov_serde_app *self, ov_buffer const *buffer) {
  log_buffer(self, buffer, "<RCV>\n");
}

/*----------------------------------------------------------------------------*/

static void log_serde_data(ov_serde_app *self, ov_serde_data data) {
  ov_result res = {0};

  if (ov_ptr_valid(self, "No serde app") && (-1 < self->data.log_fd)) {
    write(self->data.log_fd, "\n\n\n", 3);
    log_time(self->data.log_fd);
    write(self->data.log_fd, "<SND>\n", 6);
    ov_serde_serialize(self->data.serde, self->data.log_fd, data, &res);
    ov_result_clear(&res);
  }
}

/*****************************************************************************
                                      I/O
 ****************************************************************************/

static bool impl_socket_accepted(void *self, int accepted_socket,
                                 int server_socket) {
  UNUSED(server_socket);

  ov_serde_app *app = as_serde_app(self);

  if ((0 != app) && (0 != app->data.cb_accepted)) {
    app->data.cb_accepted(accepted_socket, app->data.additional);
  }

  return true;
}

/*----------------------------------------------------------------------------*/

// static ov_buffer *read_from_socket(int s, size_t len) {
//
//     ov_buffer *buffer = ov_buffer_create(len);
//
//     ssize_t recv_result = recv(s, buffer->start, len, 0);
//
//     if (0 > recv_result) {
//
//         ov_log_error("Could not read from socket %i: %s", s,
//         strerror(-s)); return ov_buffer_free(buffer);
//
//     } else if (0 == recv_result) {
//
//         ov_log_warning("Could not receive any data");
//         return ov_buffer_free(buffer);
//
//     } else {
//
//         buffer->length = (size_t)recv_result;
//         return buffer;
//     }
// }

/*----------------------------------------------------------------------------*/

static bool process_read_data(ov_serde_app *self, int fd,
                              ov_buffer const *buffer) {
  if ((!ov_ptr_valid(buffer, "No data to process")) ||
      (!ov_ptr_valid(as_serde_app(self), "No serde app"))) {
    return false;

  } else {
    log_recv(self, buffer);
    return parse_data(self->data.serde, buffer, self->data.handler, fd,
                      self->data.additional, self->data.data_type);
  }
}

/*----------------------------------------------------------------------------*/

static bool impl_socket_io(void *self, int fd,
                           const ov_memory_pointer recv_data) {
  ov_serde_app *app = as_serde_app(self);

  if (0 == app) {
    ov_serde_app_close(self, fd);
  } else if (!ov_cond_valid(0 < recv_data.length, "No data ready")) {
    ov_serde_app_close(self, fd);
  } else {
    ov_buffer const buffer = {
        .magic_byte = OV_BUFFER_MAGIC_BYTE,
        .length = recv_data.length,
        .start = (uint8_t *)recv_data.start,
    };

    if (!process_read_data(self, fd, &buffer)) {
      ov_serde_app_close(self, fd);
    }
  }
  return true;
}

/*----------------------------------------------------------------------------*/

static void impl_socket_closed(void *self, int sckt) {
  ov_serde_app *app = as_serde_app(self);

  if ((0 != app) && (0 != app->data.cb_closed)) {
    app->data.cb_closed(sckt, app->data.additional);
  }
}

/*----------------------------------------------------------------------------*/

static void impl_socket_connected(void *self, int fd, bool result) {
  ov_serde_app *app = as_serde_app(self);

  if (!result) {
    ov_log_error("Could not reconnect");
  } else if ((0 != app) && (0 != app->data.cb_reconnected)) {
    app->data.cb_reconnected(fd, app->data.additional);
  }
}

/*----------------------------------------------------------------------------*/

bool ov_serde_app_connect(ov_serde_app *self, ov_socket_configuration config) {
  ov_io_base *io_base = get_io_base(self);

  if ((!ov_ptr_valid(io_base, "No valid Serde app")) ||
      (!ov_cond_valid(config.type != NETWORK_TRANSPORT_TYPE_ERROR,
                      "Cannot connect - network type is invalid"))) {
    return false;

  } else {
    ov_io_base_listener_config cfg = {

        .socket = config,
        .callback.userdata = self,
        .callback.accept = 0,
        .callback.io = impl_socket_io,
        .callback.close = impl_socket_closed,

    };

    ov_io_base_connection_config ccfg = {
        .connection = cfg,
        .auto_reconnect = self->data.reconnect_interval_secs > 0,
        .connected = impl_socket_connected,
    };

    ov_io_base_create_connection(io_base, ccfg);

    // don't care whether or not io_base_connect
    // created a connection immediately since we configured it
    // to try reconnects - thus if we could not connect
    // immediately, do not care

    return true;
  }
}

/*----------------------------------------------------------------------------*/

bool ov_serde_app_open_server_socket(ov_serde_app *self,
                                     ov_socket_configuration config) {
  ov_io_base_listener_config cfg = {

      .socket = config,
      .callback.userdata = self,
      .callback.accept = impl_socket_accepted,
      .callback.io = impl_socket_io,
      .callback.close = impl_socket_closed,

  };

  return -1 < ov_io_base_create_listener(get_io_base(self), cfg);
}

/*----------------------------------------------------------------------------*/

int ov_serde_app_close(ov_serde_app *self, int fd) {
  if (ov_io_base_close(get_io_base(self), fd)) {
    return -1;
  } else {
    return fd;
  }
}

/*****************************************************************************
                                      SEND
 ****************************************************************************/

bool ov_serde_app_send(ov_serde_app *self, int fd, ov_serde_data data) {
  UNUSED(self);

  if (0 == as_serde_app(self)) {
    return false;

  } else {
    ov_result res = {0};

    log_serde_data(self, data);

    if (!ov_serde_serialize(self->data.serde, fd, data, &res)) {
      ov_log_error("Could not serialize data: %s", ov_result_get_message(res));
      ov_result_clear(&res);
      return false;

    } else {
      ov_result_clear(&res);
      return true;
    }
  }
}

/*----------------------------------------------------------------------------*/
