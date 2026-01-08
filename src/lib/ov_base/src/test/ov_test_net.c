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

#include "../../include/ov_test_net.h"

#include "../../include/ov_socket.h"
#include "../../include/ov_utils.h"
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*----------------------------------------------------------------------------*/

bool ov_test_send_msg_to(int fd, ov_json_value *jmsg) {

  char *smsg = ov_json_value_to_string(jmsg);
  OV_ASSERT(0 != smsg);

  size_t len = strlen(smsg);

  ssize_t written = 0;

  while ((size_t)written < len) {

    written = write(fd, smsg + written, len - written);

    free(smsg);
    smsg = 0;

    if (0 > written) {
      ov_log_error("Could not write message: %s", strerror(errno));
      goto error;
    }
  }

  jmsg = ov_json_value_free(jmsg);
  return true;

error:

  jmsg = ov_json_value_free(jmsg);
  return false;
}

/*----------------------------------------------------------------------------*/

ssize_t ov_test_message_ok(char const *msg, size_t msglen, char const *event,
                           bool error_expected) {

  if ((0 == msg) || (0 == event)) {
    goto error;
  }

  OV_ASSERT(0 != msg);
  OV_ASSERT(0 != event);

  ssize_t count = 0;

  char const *error_indicator = "\"error\"";
  size_t err_pos = 0;

  size_t rt_pos = 0;

  bool trailing_junk = false;
  bool required_token_found = false;
  bool error_found = false;

  for (size_t pos = 0; pos < msglen; ++pos) {

    switch (msg[pos]) {
    case ' ':
      break;

    case '{':
      ++count;
      break;

    case '}':
      if (0 == --count) {
        trailing_junk = false;
      }

      break;

    default:

      trailing_junk = true;

      if (msg[pos] != event[rt_pos++]) {
        rt_pos = 0;
      }

      if (msg[pos] != error_indicator[err_pos++]) {
        err_pos = 0;
      }

      if (0 == count) {
        /* Outside of valid json does not count ;) */
        break;
      }

      if (0 == event[rt_pos]) {
        ov_log_info("Found event: %s", msg + pos - rt_pos);

        required_token_found = true;
      }

      if (0 == error_indicator[err_pos]) {

        ov_log_info("Error indicator found: %s", error_indicator);

        error_found = true;
      }
    };
  }

  if (trailing_junk) {
    ov_log_error("Trailing junk");
    goto error;
  }

  if (!required_token_found) {
    ov_log_error("Event not found: %s", event);
    goto error;
  }

  if (error_expected != error_found) {
    ov_log_error("Error: Expected: %s, found %s", error_expected ? "yes" : "no",
                 error_found ? "yes" : "no");
    goto error;
  }

  return count;

error:

  return -1;
}

/*----------------------------------------------------------------------------*/

bool ov_test_expect(int fd, char const *event_name, bool error_expected) {

  usleep(3 * 1000 * 1000);

  if (!ov_socket_ensure_nonblocking(fd)) {
    ov_log_error("Could not set fd non-blocking");
    return false;
  }

  char buffer[10 * 1024] = {0};

  ssize_t recvd = read(fd, buffer, sizeof(buffer) - 1);

  if (0 >= recvd) {
    ov_log_error("Could not read from socket: %s", strerror(errno));
    return false;
  }

  buffer[sizeof(buffer) - 1] = 0;

  ov_log_info("Received %s", buffer);

  return 0 ==
         ov_test_message_ok(buffer, (size_t)recvd, event_name, error_expected);
}
/*----------------------------------------------------------------------------*/
