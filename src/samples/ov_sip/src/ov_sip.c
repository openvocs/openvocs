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
        @author         Michael J. Beer

        ------------------------------------------------------------------------
*/
#include <ov_base/ov_socket.h>
#include <ov_base/ov_utils.h>
#include <ov_os/ov_os_event_loop.h>
#include <ov_sip/ov_sip_app.h>
#include <stdarg.h>
#include <stdio.h>

/*----------------------------------------------------------------------------*/

FILE *outstream = 0;

/*----------------------------------------------------------------------------*/

static void print(char const *msg, ...) {

  va_list args;
  va_start(args, msg);

  fprintf(outstream, "%lld: ", (long long)time(0));
  vfprintf(outstream, msg, args);

  va_end(args);
}

/*----------------------------------------------------------------------------*/

void cb_closed(int fd, void *additional) {

  UNUSED(fd);

  char **constringptr = additional;
  char *constring = *constringptr;
  print("Connection %s closed\n", constring);

  constring = ov_free(constring);
  constring = strdup("UNCONNECTED");
  *constringptr = constring;
}

/*----------------------------------------------------------------------------*/

char *constring_from_fd(int fd) {

  ov_socket_state state = {0};

  if (ov_socket_state_from_handle(fd, &state)) {
    //              "Peer:"  hostname  + ":"  + port number + '\0'
    size_t bufsize = 5 + OV_HOST_NAME_MAX + 1 + 6 + 1;
    char *con = calloc(1, bufsize);
    snprintf(con, bufsize, "Peer:%s:%" PRIu16, state.peer.host,
             state.peer.port);
    return con;

  } else {
    print("Could not get socket state from handle\n");
    return strdup("New connection");
  }
}

/*----------------------------------------------------------------------------*/

void cb_reconnected(int fd, void *additional) {
  char **constringptr = additional;
  char *constring = constring_from_fd(fd);
  print("Reconnected successfully %s\n", constring);

  ov_free(*constringptr);
  *constringptr = constring;
}

/*----------------------------------------------------------------------------*/

void cb_accepted(int fd, void *additional) {

  char **constringptr = additional;
  char *constring = constring_from_fd(fd);
  print("Accepted new connection %s - beware: We assume only ONE client "
        "connection - if there are more, the logging will be wrong\n",
        constring);

  ov_free(*constringptr);
  *constringptr = constring;
}

/*****************************************************************************
                                  SIP HANDLERS
 ****************************************************************************/

static void cb_sip_response(ov_sip_message const *message, int fd,
                            void *additional) {
  UNUSED(fd);
  UNUSED(message);

  char **constringptr = additional;

  print("Received new SIP response from %s\n", *constringptr);
  ov_sip_message_dump(outstream, message);
  fprintf(outstream,
          "------------------------------------------------------------\n");
}

/*----------------------------------------------------------------------------*/

static void cb_sip_request(ov_sip_message const *message, int fd,
                           void *additional) {
  UNUSED(fd);
  UNUSED(message);

  char **constringptr = additional;

  print("Received new SIP request from %s:\n", *constringptr);
  ov_sip_message_dump(outstream, message);
  fprintf(outstream,
          "------------------------------------------------------------\n");
}

/*----------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  outstream = stderr;

  ov_socket_configuration server_socket_cfg = {
      .host = "127.0.0.1",
      .port = 1111,
      .type = TCP,
  };

  char *constring = strdup("UNCONNECTED");

  ov_sip_app_configuration cfg = {

      .log_io = true,
      .reconnect_interval_secs = 20,
      .accept_to_io_timeout_secs = 20,

      .cb_closed = cb_closed,
      .cb_reconnected = cb_reconnected,
      .cb_accepted = cb_accepted,

      .additional = &constring,
      .are_methods_case_sensitive = false,

  };

  ov_event_loop *loop = ov_os_event_loop(ov_event_loop_config_default());
  ov_sip_app *app = ov_sip_app_create("SIP Test", loop, cfg);

  int exit_value = EXIT_FAILURE;

  if (ov_sip_app_register_handler(app, "BYE", cb_sip_request) &&
      ov_sip_app_register_handler(app, "ACK", cb_sip_request) &&
      ov_sip_app_register_handler(app, "CANCEL", cb_sip_request) &&
      ov_sip_app_register_handler(app, "INVITE", cb_sip_request) &&
      ov_sip_app_register_response_handler(app, cb_sip_response) &&
      ov_sip_app_open_server_socket(app, server_socket_cfg)) {
    print("Listening on %s:%" PRIu16 "\n", server_socket_cfg.host,
          server_socket_cfg.port);
    ov_event_loop_run(loop, OV_RUN_MAX);
    exit_value = EXIT_SUCCESS;

  } else {
    print("Failed to open server socket on %s:%" PRIu16 "\n",
          server_socket_cfg.host, server_socket_cfg.port);
    exit_value = EXIT_FAILURE;
  }

  app = ov_sip_app_free(app);
  loop = ov_event_loop_free(loop);

  constring = ov_free(constring);

  return exit_value;
}
