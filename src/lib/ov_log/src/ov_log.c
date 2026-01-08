/***
        ------------------------------------------------------------------------

        Copyright 2017 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the openvocs project. http://openvocs.org

        ------------------------------------------------------------------------
*//**
        @author         Michael Beer
        @date           2021-09-10

        ------------------------------------------------------------------------
*/
#include "../include/ov_log.h"
#include "../include/ov_log_rotate.h"

#include "log_assert.h"
#include <ov_arch/ov_arch.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/*----------------------------------------------------------------------------*/

#include "log_hashtable.h"

#define MESSAGE_MAX_CHARS 10000

/*----------------------------------------------------------------------------*/

static char const *level_strings[] = {

    "emergency", "alert", "critical", "error",   "warning", "notice",
    "info",      "debug", "dev",      "invalid", 0

};

/*----------------------------------------------------------------------------*/

static char *strdup_if_not_null(char const *str) {

  if (0 == str) {
    return 0;
  }

  return strdup(str);
}

/*----------------------------------------------------------------------------*/

ov_log_level ov_log_level_from_string(char const *level) {

  if (0 == level) {
    return OV_LOG_INVALID;
  }

  for (size_t i = 0; 0 != level_strings[i]; ++i) {

    if (0 == strcmp(level_strings[i], level)) {
      return i;
    }
  }

  return OV_LOG_INVALID;
}

/*----------------------------------------------------------------------------*/

char const *ov_log_level_to_string(ov_log_level level) {

  if ((level < 0) ||
      (level >= sizeof(level_strings) / sizeof(level_strings[0]))) {
    return 0;
  }

  return level_strings[level];
}

/*****************************************************************************
                               MESSAGE FORMATTERS
 ****************************************************************************/

typedef bool (*format_message_func)(char **msg_out, size_t *msg_len_out,
                                    int level, char const *const file,
                                    char const *const function, int line,
                                    char const *const format, va_list ap);

/*----------------------------------------------------------------------------*/

static int print_timestamp_nocheck(FILE *msg_out) {

  LOG_ASSERT(0 != msg_out);

  struct timespec ts = {0};

  if (!timespec_get(&ts, TIME_UTC)) {
    return -1;
  }

  struct tm broken_down_time = {0};

  if (0 == gmtime_r(&ts.tv_sec, &broken_down_time)) {
    return -1;
  }

  char time_str[25] = {0};

  if (!strftime(time_str, sizeof(time_str), "%FT%T", &broken_down_time)) {
    return -1;
  }

  return fprintf(msg_out, "%s.%.6ldZ", time_str, ts.tv_nsec);
}

/*----------------------------------------------------------------------------*/

static bool format_as_json(char **msg, size_t *msg_len, int level,
                           char const *const file, char const *const function,
                           int line, char const *const format, va_list ap) {

  LOG_ASSERT(0 != msg);
  LOG_ASSERT(0 != msg_len);
  LOG_ASSERT(0 != file);
  LOG_ASSERT(0 != function);
  LOG_ASSERT(0 != format);

  FILE *msg_out = 0;

  msg_out = open_memstream(msg, msg_len);

  if (0 == msg_out) {
    goto error;
  }

  if (0 > fprintf(msg_out, "\n{\n \"PRIO\" : %d,\n \"TIME\" : \"", level)) {
    goto error;
  }

  if (0 > print_timestamp_nocheck(msg_out)) {
    goto error;
  }

  if (0 > fprintf(msg_out,
                  "\",\n"
                  " \"FILE\" : \"%s\",\n"
                  " \"FUNC\" : \"%s\",\n"
                  " \"LINE\" : %d,\n"
                  " \"INFO\" : \"",
                  file, function, line)) {
    goto error;
  }

  if (0 > vfprintf(msg_out, format, ap)) {
    goto error;
  }

  if (0 > fprintf(msg_out, "\"\n}\n")) {
    goto error;
  }

  fclose(msg_out);

  return true;

error:

  if (0 != msg) {
    free(msg);
  }

  if (0 != msg_out) {
    fclose(msg_out);
  }

  return false;
}

/*----------------------------------------------------------------------------*/

static bool format_as_plain_text(char **msg, size_t *msg_len, int level,
                                 char const *const file,
                                 char const *const function, int line,
                                 char const *const format, va_list ap) {

  LOG_ASSERT(0 != msg);
  LOG_ASSERT(0 != msg_len);
  LOG_ASSERT(0 != file);
  LOG_ASSERT(0 != function);
  LOG_ASSERT(0 != format);

  FILE *msg_out = 0;

  msg_out = open_memstream(msg, msg_len);

  if (0 == msg_out) {
    goto error;
  }

  if (0 > print_timestamp_nocheck(msg_out)) {
    goto error;
  }

  if (0 > fprintf(msg_out, " [%s] - %s:%d (%s): ",
                  ov_log_level_to_string(level), file, line, function)) {
    goto error;
  }

  if (0 > vfprintf(msg_out, format, ap)) {
    goto error;
  }

  if (0 > fprintf(msg_out, "\n")) {
    goto error;
  }

  fclose(msg_out);

  return true;

error:

  if (0 != msg) {
    free(msg);
  }

  if (0 != msg_out) {
    fclose(msg_out);
  }

  return false;
}

/*****************************************************************************
                                    DEFAULTS
 ****************************************************************************/

/* FH 1 is stdout */
#define DEFAULT_FILEHANDLE 1
#define DEFAULT_LEVEL OV_LOG_DEBUG
#define DEFAULT_FORMATTER format_as_plain_text

/*****************************************************************************
                                  DATA STRUCTS
 ****************************************************************************/

#define GLOBAL_MAGIC_NUMBER 0xef010101
#define MODULE_MAGIC_NUMBER 0xab5c3dff
#define FUNC_MAGIC_NUMBER 0xcd6d4e00

typedef struct {

  uint32_t magic_number;
  ov_log_output output;
  ov_log_level level;
  format_message_func formatter;

  size_t message_counter;

  log_hashtable *func_outputs;

} module_output;

static module_output g_default_output = {
    .magic_number = GLOBAL_MAGIC_NUMBER,
    .level = DEFAULT_LEVEL,
    .formatter = DEFAULT_FORMATTER,
    .output.filehandle = DEFAULT_FILEHANDLE,
};

static bool g_muted = false;

/*****************************************************************************
                                     CREATE
 ****************************************************************************/

static module_output *func_output_create() {

  module_output *out = calloc(1, sizeof(module_output));
  out->magic_number = FUNC_MAGIC_NUMBER;
  out->level = DEFAULT_LEVEL;
  out->output.filehandle = DEFAULT_FILEHANDLE;
  return out;
}

/*----------------------------------------------------------------------------*/

static module_output *module_output_create() {

  module_output *mout = func_output_create();
  mout->magic_number = MODULE_MAGIC_NUMBER;

  /* Number of buckets should in the order of funcs per module on average
   */
  mout->func_outputs = log_hashtable_create(10);

  return mout;
}

/*----------------------------------------------------------------------------*/

bool ov_log_init() {

  /* Number of buckets should be in the order of number of modules */
  g_default_output.func_outputs = log_hashtable_create(100);
  return 0 != g_default_output.func_outputs;
}

/*****************************************************************************
                                   TEAR DOWN
 ****************************************************************************/

static bool close_output_nocheck(ov_log_output *out) {

  LOG_ASSERT(0 != out);

  // We do not want to close 1 == stdout or  2 == stderr
  if (2 < out->filehandle) {
    close(out->filehandle);
    out->filehandle = -1;
  }

  if (0 != out->log_rotation.path) {
    free(out->log_rotation.path);
    out->log_rotation.path = 0;
  }

  return true;
}

/*----------------------------------------------------------------------------*/

static log_hashtable *free_outputs(log_hashtable *table);

static bool free_output(void const *key, void const *value, void *arg) {

  UNUSED(key);
  UNUSED(arg);

  if (0 == value)
    return false;

  module_output *out = (module_output *)value;

  LOG_ASSERT((MODULE_MAGIC_NUMBER == out->magic_number) ||
             (FUNC_MAGIC_NUMBER == out->magic_number));

  close_output_nocheck(&out->output);

  if (MODULE_MAGIC_NUMBER == out->magic_number) {
    out->func_outputs = free_outputs(out->func_outputs);
  } else {
    LOG_ASSERT(FUNC_MAGIC_NUMBER == out->magic_number);
  }

  LOG_ASSERT(0 == out->func_outputs);

  free(out);

  return true;
}

/*----------------------------------------------------------------------------*/

static log_hashtable *free_outputs(log_hashtable *table) {

  log_hashtable_for_each(table, free_output, 0);
  return log_hashtable_free(table);
}

/*----------------------------------------------------------------------------*/

static bool module_output_close(module_output *mout) {

  if (0 == mout) {
    return false;
  }

  close_output_nocheck(&mout->output);

  mout->func_outputs = free_outputs(mout->func_outputs);
  LOG_ASSERT(0 == mout->func_outputs);

  return true;
}

/*----------------------------------------------------------------------------*/

bool ov_log_close() {

  bool retval = module_output_close(&g_default_output);

  /* Ensure we continue logging to stderr at least after log closure */
  g_default_output = (module_output){
      .magic_number = GLOBAL_MAGIC_NUMBER,
      .level = DEFAULT_LEVEL,
      .formatter = DEFAULT_FORMATTER,
      .output.filehandle = DEFAULT_FILEHANDLE,
  };

  return retval;
}

/*----------------------------------------------------------------------------*/

static format_message_func formatter_for_format(ov_log_format fmt) {

  switch (fmt) {

  case OV_LOG_JSON:
    return format_as_json;

  case OV_LOG_TEXT:
  default:

    return format_as_plain_text;
  };

  return format_as_plain_text;
}

/*----------------------------------------------------------------------------*/

static int set_module_output(module_output *mout, const ov_log_output output,
                             ov_log_level level) {

  LOG_ASSERT(0 != mout);

  int old = mout->output.filehandle;

  if (0 != mout->output.log_rotation.path) {
    free(mout->output.log_rotation.path);
    mout->output.log_rotation.path = 0;
  }

  mout->output = output;
  mout->level = level;
  mout->formatter = formatter_for_format(output.format);
  mout->output.log_rotation.path = strdup_if_not_null(output.log_rotation.path);

  mout->message_counter = 0;

  return old;
}

/*----------------------------------------------------------------------------*/

int ov_log_set_output(char const *module_name, char const *function_name,
                      ov_log_level level, const ov_log_output output) {

  if (0 == module_name) {
    return set_module_output(&g_default_output, output, level);
  }

  LOG_ASSERT(0 != module_name);

  module_output *mout =
      log_hashtable_get(g_default_output.func_outputs, module_name);

  if (0 == mout) {
    mout = module_output_create();
    log_hashtable_set(g_default_output.func_outputs, module_name, mout);
  }

  LOG_ASSERT(MODULE_MAGIC_NUMBER == mout->magic_number);
  LOG_ASSERT(0 != mout->func_outputs);

  if (0 == function_name) {
    return set_module_output(mout, output, level);
  }

  module_output *fout = log_hashtable_get(mout->func_outputs, function_name);

  if (0 == fout) {
    fout = func_output_create();
    log_hashtable_set(mout->func_outputs, function_name, fout);
  }

  LOG_ASSERT(0 != fout);
  LOG_ASSERT(FUNC_MAGIC_NUMBER == fout->magic_number);

  return set_module_output(fout, output, level);
}

/*----------------------------------------------------------------------------*/

static char const *filename(char const *path) {

  char const *fname = path;

  for (char const *p = path; *p != 0; ++p) {

    if ('/' == *p) {
      fname = p + 1;
    }
  }

  return fname;
}

/*----------------------------------------------------------------------------*/

static module_output *output_for(char const *module, char const *func) {

  if (0 == g_default_output.func_outputs) {
    return &g_default_output;
  }

  char const *fname = filename(module);

  module_output *mout = log_hashtable_get(g_default_output.func_outputs, fname);

  if (0 == mout) {
    return &g_default_output;
  }

  module_output *fout = log_hashtable_get(mout->func_outputs, func);

  if (0 == fout) {

    return mout;
  }

  return fout;
}

/*----------------------------------------------------------------------------*/

static bool log_to_stream_nocheck(int fh, format_message_func formatter,
                                  int level, char const *const file,
                                  char const *const function, int line,
                                  char const *const format, va_list ap) {

  FILE *msg_out = 0;

  LOG_ASSERT(0 < fh);
  LOG_ASSERT(0 != file);
  LOG_ASSERT(0 != function);
  LOG_ASSERT(0 != format);

  char *msg = 0;
  size_t msg_size = 0;

  if (0 == formatter) {
    formatter = format_as_plain_text;
  }

  if (!formatter(&msg, &msg_size, level, file, function, line, format, ap)) {
    goto error;
  }

  if (0 > write(fh, msg, msg_size)) {
    goto error;
  }

  free(msg);

  return true;

error:

  if (0 != msg) {
    free(msg);
  }

  if (0 != msg_out) {
    fclose(msg_out);
  }

  return false;
}

/*****************************************************************************
                               SYSTEMD SPECIFICS
 ****************************************************************************/

#if OV_ARCH == OV_LINUX
#include <systemd/sd-journal.h>
#endif

/*----------------------------------------------------------------------------*/

static bool format_for_systemd(char **msg, size_t *msg_len,
                               char const *const file,
                               char const *const function, int line,
                               char const *const format, va_list ap) {

  LOG_ASSERT(0 != msg);
  LOG_ASSERT(0 != msg_len);
  LOG_ASSERT(0 != file);
  LOG_ASSERT(0 != function);
  LOG_ASSERT(0 != format);

  FILE *msg_out = 0;

  msg_out = open_memstream(msg, msg_len);

  if (0 == msg_out) {
    goto error;
  }

  if (0 > fprintf(msg_out, "%s:%d (%s): ", file, line, function)) {
    goto error;
  }

  if (0 > vfprintf(msg_out, format, ap)) {
    goto error;
  }

  if (0 > fprintf(msg_out, "\n")) {
    goto error;
  }

  fclose(msg_out);

  return true;

error:

  if (0 != msg_out) {
    fclose(msg_out);
  }

  if (0 != msg) {
    free(msg);
  }

  return false;
}

/*----------------------------------------------------------------------------*/

static bool log_to_systemd(int level, char const *file, char const *func,
                           size_t line, char const *const format, va_list ap) {

#ifdef foosdjournalhfoo

  // We do not use sd_journal_print_with_location since it is too complicated
  // The arguments file, function etc. require a special format,
  // hence we would have first to create new strings wrapping file, function
  // etc...

  char *msg = 0;
  size_t msg_len_bytes = 0;

  if (format_for_systemd(&msg, &msg_len_bytes, file, func, line, format, ap)) {

    bool result = sd_journal_print(level, "%s", msg) >= 0;
    free(msg);
    return result;

  } else {

    // We could not create our log message for whatever reasong -
    // fallback: Log message without location info
    return sd_journal_printv(level, format, ap) >= 0;
  }

#else

  UNUSED(level);
  UNUSED(format);
  UNUSED(ap);
  UNUSED(file);
  UNUSED(func);
  UNUSED(line);
  UNUSED(format_for_systemd);

#endif

  return true;
}

/*****************************************************************************
                               LOG FILE ROTATION
 ****************************************************************************/

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

static bool rotate_log_if_required(module_output *mout) {

  LOG_ASSERT(0 != mout);

  char const *path = mout->output.log_rotation.path;

  if (0 == path) {
    return true;
  }

  LOG_ASSERT(0 != path);

  if (mout->output.log_rotation.messages_per_file > mout->message_counter) {
    return true;
  }

  mout->message_counter = 0;

  if (!ov_log_rotate_files(path, mout->output.log_rotation.max_num_files)) {
    return false;
  }

  if (0 < mout->output.filehandle) {
    close(mout->output.filehandle);
  }

  mout->output.filehandle = create_log_file(mout->output.log_rotation.path);

  return 0 < mout->output.filehandle;
}

/*****************************************************************************
                                      LOG
 ****************************************************************************/

bool ov_log_ng(ov_log_level level, char const *file, char const *function,
               size_t line, char const *format, ...) {

  va_list ap;

  if (g_muted && (OV_LOG_ERR < level)) {
    return true;
  }

  if ((0 == file) || (0 == format)) {
    goto error;
  }

  module_output *fout = output_for(file, function);

  if (level > fout->level) {
    return true;
  }

  if (0 == function) {
    function = "UNKNOWN";
  }

  LOG_ASSERT(0 != file);
  LOG_ASSERT(0 != function);
  LOG_ASSERT(0 != format);

  va_start(ap, format);

  bool logged_to_stream = false;
  bool should_log_to_stream = (0 < fout->output.filehandle);

  if (should_log_to_stream) {

    logged_to_stream =
        log_to_stream_nocheck(fout->output.filehandle, fout->formatter, level,
                              file, function, line, format, ap);
  }

  if (should_log_to_stream && (!logged_to_stream)) {
    goto error;
  }

  if (logged_to_stream) {
    ++fout->message_counter;
  }

  rotate_log_if_required(fout);

  va_end(ap);

  va_start(ap, format);

  if (fout->output.use.systemd &&
      (!log_to_systemd(level, file, function, line, format, ap))) {
    goto error;
  }

  va_end(ap);

  return true;

error:

  va_end(ap);
  return false;
}

/*----------------------------------------------------------------------------*/

void ov_log_mute() { g_muted = true; }

/*----------------------------------------------------------------------------*/

void ov_log_unmute() { g_muted = false; }

/*----------------------------------------------------------------------------*/
