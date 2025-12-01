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

#include "../../include/ov_config_log.h"

#include <fcntl.h>
#include <ov_log/ov_log.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../../include/ov_config_keys.h"
#include "../../include/ov_json.h"

/*----------------------------------------------------------------------------*/

static ov_log_level ov_log_level_from_json(ov_json_value const *jval) {

  if (0 == jval)
    goto error;

  char const *level = ov_json_string_get(ov_json_get(jval, "/" OV_KEY_LEVEL));

  return ov_log_level_from_string(level);

error:

  return OV_LOG_INVALID;
}

/*----------------------------------------------------------------------------*/

static int create_log_file(char const *path) {

  if (0 == path)
    return -1;

  int fh = open(path, O_RDWR | O_CREAT | O_APPEND, S_IRWXU);

  if (0 >= fh) {
    ov_log_warning("Could not open log file %s", path);
  }

  return fh;
}

/*----------------------------------------------------------------------------*/

static ov_log_format format_from_json(ov_json_value const *jval) {

  ov_log_format fmt = OV_LOG_TEXT;

  jval = ov_json_get(jval, "/" OV_KEY_FORMAT);

  if (0 == jval) {
    return fmt;
  }

  char const *fmt_str = ov_json_string_get(jval);

  if (0 == fmt_str) {
    ov_log_error("Invalid config: " OV_KEY_FORMAT " is not a string - "
                 "falling back to default "
                 "format");
    return fmt;
  }

  if (0 == strncmp(fmt_str, OV_KEY_JSON, strlen(OV_KEY_JSON))) {
    fmt = OV_LOG_JSON;
    goto finish;
  }

  if (0 != strncmp(fmt_str, OV_KEY_PLAIN_TEXT, strlen(OV_KEY_PLAIN_TEXT))) {
    ov_log_error("Invalid config: Unkown format '%s' - falling back to "
                 "default format",
                 fmt_str);
  }

finish:

  return fmt;
}

/*----------------------------------------------------------------------------*/

static bool configure_log_rotation(ov_log_output *out,
                                   ov_json_value const *jval) {

  ov_json_value const *max_num_messages_jval =
      ov_json_get(jval, "/" OV_KEY_ROTATE_AFTER_MESSAGES);

  ov_json_value const *max_num_files_jval =
      ov_json_get(jval, "/" OV_KEY_KEEP_FILES);

  if ((0 == max_num_messages_jval) && (0 == max_num_files_jval)) {
    goto finish;
  }

  if ((0 != max_num_messages_jval) &&
      (!ov_json_is_number(max_num_messages_jval))) {

    ov_log_error("Log config invalid: %s is not a number",
                 OV_KEY_ROTATE_AFTER_MESSAGES);
    goto error;
  }

  if ((0 != max_num_files_jval) && (!ov_json_is_number(max_num_files_jval))) {

    ov_log_error("Log config invalid: %s is not a number", OV_KEY_KEEP_FILES);
    goto error;
  }

  size_t messages_per_file = ov_json_number_get(max_num_messages_jval);
  size_t max_num_files = ov_json_number_get(max_num_files_jval);

  if (0 == messages_per_file) {
    messages_per_file = OV_DEFAULT_LOG_MESSAGES_PER_FILE;
  }

  if (0 == max_num_files) {
    max_num_files = OV_DEFAULT_LOG_MAX_NUM_FILES;
  }

  out->log_rotation.use = true;
  out->log_rotation.max_num_files = max_num_files;
  out->log_rotation.messages_per_file = messages_per_file;

finish:

  return true;

error:

  return false;
}

/*----------------------------------------------------------------------------*/

ov_log_output ov_log_output_from_json(ov_json_value const *jval) {

  ov_log_output out = {0};

  if (0 == jval) {
    ov_log_error("No JSON - 0 pointer");
    goto error;
  }

  if (ov_json_is_true(ov_json_get(jval, "/" OV_KEY_SYSTEMD))) {
    out.use.systemd = true;
  }

  ov_json_value const *fjval = ov_json_get(jval, "/" OV_KEY_FILE);

  if (0 == fjval) {
    goto finish;
  }

  ov_log_format format = format_from_json(jval);

  if (ov_json_is_object(fjval)) {
    jval = fjval;
  }

  char const *path = ov_json_string_get(ov_json_get(jval, "/" OV_KEY_FILE));

  if (0 == path) {
    goto finish;
  }

  if (0 == strcmp("stdout", path)) {
    out.filehandle = 1;
    goto finish;
  }

  if (0 == strcmp("stderr", path)) {
    out.filehandle = 2;
    goto finish;
  }

  out.filehandle = create_log_file(path);
  out.format = format;

  configure_log_rotation(&out, jval);

  if (out.log_rotation.use) {
    out.log_rotation.path = strdup(path);
  }

finish:
error:

  return out;
}

/*----------------------------------------------------------------------------*/

static int set_output_from_json(char const *module, char const *function,
                                ov_json_value const *jval) {

  ov_log_level level = ov_log_level_from_json(jval);
  ov_log_output out = ov_log_output_from_json(jval);

  int old_fh = ov_log_set_output(module, function, level, out);

  if (0 != out.log_rotation.path) {
    free(out.log_rotation.path);
    out.log_rotation.path = 0;
  }

  if (2 < old_fh) {
    ov_log_error("Overwriting open file handle ...");
    close(old_fh);
    return old_fh;
  }

  return old_fh;
}

/*----------------------------------------------------------------------------*/

bool configure_function(const void *key, void *value, void *module_name) {

  ov_log_info("Log: Configuring for module %s function %s", module_name, key);

  ov_json_value *jval = ov_json_value_cast(value);

  if (0 == jval) {
    ov_log_critical("Internal error: Expected json pointer, got something "
                    "else");
    return false;
  }

  set_output_from_json(module_name, key, value);

  return true;
}

/*----------------------------------------------------------------------------*/

bool configure_module(const void *key, void *value, void *data) {

  UNUSED(data);

  ov_log_info("Log: Configuring for module %s", key);

  ov_json_value *jval = ov_json_value_cast(value);

  if (0 == jval) {
    ov_log_critical("Internal error: Expected json pointer, got something "
                    "else");
    return false;
  }

  set_output_from_json(key, 0, value);

  ov_json_value const *functions = ov_json_get(jval, "/" OV_KEY_FUNCTIONS);
  ov_json_object_for_each((ov_json_value *)functions, (void *)key,
                          configure_function);

  return true;
}

/*----------------------------------------------------------------------------*/

bool ov_config_log_from_json(ov_json_value const *jval) {

  const ov_json_value *conf = ov_json_get(jval, "/" OV_KEY_LOGGING);

  if (0 == conf) {
    conf = jval;
  }

  set_output_from_json(0, 0, conf);

  ov_json_value const *modules = ov_json_get(conf, "/" OV_KEY_MODULES);

  ov_json_object_for_each((ov_json_value *)modules, 0, configure_module);

  return true;
}

/*----------------------------------------------------------------------------*/
