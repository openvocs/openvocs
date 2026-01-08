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
        @author         Michael Beer

        ------------------------------------------------------------------------
*/

#include "ov_signal_monitor.h"
#include <fcntl.h>
#include <ov_base/ov_string.h>

/*----------------------------------------------------------------------------*/

static ov_app *sapp = 0;

bool ov_signal_monitor_set_signaling_app(ov_app *new_sapp) {

  sapp = new_sapp;
  return true;
}

/*----------------------------------------------------------------------------*/

static int signaling_log_fd = -1;

/*----------------------------------------------------------------------------*/

static char const *get_log_path(char const *wanted_path) {

  static char file_name[PATH_MAX] = {0};

  if (0 != wanted_path) {

    return wanted_path;

  } else {

    snprintf(file_name, sizeof(file_name), "/tmp/%i_signaling.log", getpid());

    return file_name;
  }
}

/*----------------------------------------------------------------------------*/

static int open_log_fd(char const *path) {

  int fd =
      open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, S_IRUSR | S_IWUSR);

  if (0 > fd) {
    ov_log_error("Could not open serde log file %s", ov_string_sanitize(path));
    return -1;
  } else {
    ov_log_info("Opened serde log file %s", ov_string_sanitize(path));
    return fd;
  }
}

/*----------------------------------------------------------------------------*/

static void log_time(int i) {

  char now_str[30];
  ssize_t now_len = snprintf(now_str, 20, "%ld", time(0));

  if (now_len > -1) {
    write(i, now_str, (size_t)now_len);
  }
}

/*----------------------------------------------------------------------------*/

static void signaling_io_monitor(void *userdata, ov_direction direction,
                                 int signaling_fd,
                                 const ov_socket_data *signaling_fd_data,
                                 const ov_json_value *value) {

  UNUSED(userdata);
  UNUSED(signaling_fd);

  char const *prefix = "<RCV> from ";

  if (OV_OUT == direction) {
    prefix = "<SND> from ";
  }

  if (-1 < signaling_log_fd) {

    char *value_str = ov_json_value_to_string(value);

    write(signaling_log_fd, "\n\n\n", 3);
    log_time(signaling_log_fd);

    write(signaling_log_fd, prefix, strlen(prefix));

    if (0 != signaling_fd_data) {

      write(signaling_log_fd, signaling_fd_data->host,
            strlen(signaling_fd_data->host));

      char str[OV_HOST_NAME_MAX + 20] = {0};
      snprintf(str, sizeof(str), "%s:%" PRIu16, signaling_fd_data->host,
               signaling_fd_data->port);

      write(signaling_log_fd, str, strlen(str));
    }

    if (0 == value_str) {
      write(signaling_log_fd, "[NO DATA]\n", sizeof("[NO DATA]\n"));
    } else {
      write(signaling_log_fd, value_str, strlen(value_str));
    }

    value_str = ov_free(value_str);
  }
}

/*----------------------------------------------------------------------------*/

char const *ov_signal_monitor_enable(char const *log_file_path) {

  if (-1 < signaling_log_fd) {
    ov_log_warning("Not enabling signaling monitoring - already enabled");
    return 0;
  } else if (0 != sapp) {

    char const *path = get_log_path(log_file_path);
    signaling_log_fd = open_log_fd(path);

    if ((-1 < signaling_log_fd) &&
        ov_signaling_app_set_monitor(sapp, signaling_io_monitor, 0)) {

      ov_log_warning("Set signaling monitor to %s",
                     ov_string_sanitize(log_file_path));

      return path;

    } else {

      return 0;
    }

  } else {

    ov_log_error("Cannot enable signaling logging - no resmgr app");
    return 0;
  }
}

/*----------------------------------------------------------------------------*/

bool ov_signal_monitor_disable() {

  if (-1 < signaling_log_fd) {

    close(signaling_log_fd);
    ov_signaling_app_set_monitor(sapp, 0, 0);
    signaling_log_fd = -1;
    ov_log_warning("Signaling monitoring disabled");
  }

  return true;
}

/*----------------------------------------------------------------------------*/
