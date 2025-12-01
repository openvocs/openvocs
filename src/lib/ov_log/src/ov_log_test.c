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

#include "log_assert.h"
#include "ov_log.c"
#include "testrun.h"

#include <ftw.h>
#include <stdio.h>
#include <sys/stat.h>

/*----------------------------------------------------------------------------*/

static void lets_log_something(ov_log_level level) {

  ov_log_ng(level, __FILE__, __FUNCTION__, __LINE__, "LOGGED SOMETHIN");
}

/*****************************************************************************
                                    TEMP DIR
 ****************************************************************************/

#define OUR_TEMP_DIR "/tmp/ov_log"

static bool init_tmp_dir() {
  return 0 == mkdir(OUR_TEMP_DIR, S_IRUSR | S_IWUSR | S_IXUSR);
}

/*----------------------------------------------------------------------------*/

static int delete_node(char const *path, const struct stat *sb, int typeflag,
                       struct FTW *ftwbuf) {

  UNUSED(sb);
  UNUSED(typeflag);
  UNUSED(ftwbuf);

  int retval = remove(path);

  return retval;
}

/*----------------------------------------------------------------------------*/

static bool remove_tmp_dir() {

  struct stat statbuf = {0};
  errno = 0;

  char const *path = OUR_TEMP_DIR;

  // no stat not existing
  if (0 != stat(path, &statbuf))
    return true;

  if (0 !=
      nftw(path, delete_node, FOPEN_MAX, FTW_DEPTH | FTW_MOUNT | FTW_PHYS)) {
    return false;
  }

  return true;
}

/*****************************************************************************
                                     Files
 ****************************************************************************/

static FILE *get_file_ptr(int fh) {

  FILE *f = fdopen(dup(fh), "w+");
  return f;
}

/*----------------------------------------------------------------------------*/

static bool file_empty(FILE *f) {

  long old_pos = ftell(f);
  LOG_ASSERT(0 <= old_pos);

  int retval = fseek(f, 0, SEEK_END);
  LOG_ASSERT(0 <= retval);

  bool is_empty = 0 == ftell(f);

  retval = fseek(f, old_pos, SEEK_SET);
  LOG_ASSERT(0 <= retval);

  return is_empty;
}

/*----------------------------------------------------------------------------*/

static bool file_clear(FILE *f) {

  int retval = fseek(f, 0, SEEK_SET);
  LOG_ASSERT(0 <= retval);
  return 0 == ftruncate(fileno(f), 0);
}

/*----------------------------------------------------------------------------*/

static int file_temp() {
  char template[] = OUR_TEMP_DIR "/ov_log_XXXXXX";
  return mkstemp(template);
}

/*****************************************************************************
                                     STDOUT
 ****************************************************************************/

#define STDOUT_FILE OUR_TEMP_DIR "/ov_log_test_stdout"

/*----------------------------------------------------------------------------*/

static bool stdout_redirect() {
  return stdout == freopen(STDOUT_FILE, "w+", stdout);
}

/*----------------------------------------------------------------------------*/

static bool stdout_empty() { return file_empty(stdout); }

/*----------------------------------------------------------------------------*/

static bool stdout_clear() { return file_clear(stdout); }

/*****************************************************************************
                                     STDERR
 ****************************************************************************/

#define STDERR_FILE OUR_TEMP_DIR "/ov_log_test_stderr"

static bool stderr_empty() { return file_empty(stderr); }

/*----------------------------------------------------------------------------*/

static bool stderr_clear() { return file_clear(stderr); }

/*----------------------------------------------------------------------------*/

static bool stderr_redirect() {
  return stderr == freopen(STDERR_FILE, "w+", stderr);
}

/*****************************************************************************
                                     Tests
 ****************************************************************************/

static int init() {

  init_tmp_dir();
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_log() {

  testrun(stdout_redirect());
  testrun(stdout_empty());
  testrun(stderr_redirect());
  testrun(stderr_empty());

  // Should log stdout, up to level info
  ov_log_error("ERROR");
  ov_log_warning("WARNING");
  ov_log_info("INFO");
  ov_log_debug("DEBUG");

  testrun(!stdout_empty());
  testrun(stdout_clear());
  testrun(stderr_empty());
  testrun(stderr_clear());

  ov_log_init();

  // Should stdout, up to level info
  ov_log_error("ERROR");
  ov_log_warning("WARNING");
  ov_log_info("INFO");
  ov_log_debug("DEBUG");

  testrun(!stdout_empty());
  testrun(stdout_clear());
  testrun(stderr_empty());
  testrun(stderr_clear());

  ov_log_set_output(0, 0, OV_LOG_INFO,
                    (ov_log_output){
                        .use.systemd = false,
                        .filehandle = -1,
                    });

  // Should log nowhere
  ov_log_error("ERROR");
  ov_log_warning("WARNING");
  ov_log_info("INFO");
  ov_log_debug("DEBUG");

  testrun(stdout_empty());
  testrun(stderr_empty());

  int fh = file_temp();
  FILE *aaa = get_file_ptr(fh);
  testrun(0 != aaa);

  testrun(ov_log_set_output("aaa", 0, OV_LOG_DEBUG,
                            (ov_log_output){
                                .filehandle = fh,
                            }));

  // Since we are in module 'ov_log_test.c', we should still log nowhere

  ov_log_error("ERROR");

  testrun(stdout_empty());
  testrun(stderr_empty());
  testrun(file_empty(aaa));

  /* Now install an output handler for our own file -
   * `ov_log_test_c` file should receive our logs
   */

  fh = file_temp();
  FILE *ov_log_test_c = get_file_ptr(fh);
  testrun(0 != ov_log_test_c);

  testrun(ov_log_set_output("ov_log_test.c", 0, OV_LOG_WARNING,
                            (ov_log_output){
                                .filehandle = fh,
                            }));

  ov_log_error("ERROR");

  testrun(stdout_empty());
  testrun(stderr_empty());
  testrun(file_empty(aaa));
  testrun(!file_empty(ov_log_test_c));

  testrun(file_clear(ov_log_test_c));

  // Warning level should be logged as well

  ov_log_warning("WARNING");

  testrun(stdout_empty());
  testrun(stderr_empty());
  testrun(file_empty(aaa));
  testrun(!file_empty(ov_log_test_c));

  testrun(file_clear(ov_log_test_c));

  // INFO should not logged since the highest level was set to WARNING

  ov_log_info("INFO");

  testrun(stdout_empty());
  testrun(stderr_empty());
  testrun(file_empty(aaa));
  testrun(file_empty(ov_log_test_c));

  // DEBUG should not logged since the highest level was set to WARNING

  ov_log_debug("DEBUG");

  testrun(stdout_empty());
  testrun(stderr_empty());
  testrun(file_empty(aaa));
  testrun(file_empty(ov_log_test_c));

  // A function whose name has not been configured should log into
  // the output for our module
  lets_log_something(OV_LOG_ERR);

  testrun(stdout_empty());
  testrun(stderr_empty());
  testrun(file_empty(aaa));
  testrun(!file_empty(ov_log_test_c));

  testrun(file_clear(ov_log_test_c));

  // Install handler for another function that does not log

  fh = file_temp();
  FILE *ov_log_test_c_not_our_func = get_file_ptr(fh);
  testrun(0 != ov_log_test_c_not_our_func);

  testrun(ov_log_set_output("ov_log_test.c", "not_our_func", OV_LOG_WARNING,
                            (ov_log_output){
                                .filehandle = fh,
                            }));

  // Function should still log into our module log
  lets_log_something(OV_LOG_ERR);

  testrun(stdout_empty());
  testrun(stderr_empty());
  testrun(file_empty(aaa));
  testrun(!file_empty(ov_log_test_c));
  testrun(file_empty(ov_log_test_c_not_our_func));

  testrun(file_clear(ov_log_test_c));

  // Install handler for lets_log_something

  fh = file_temp();
  FILE *ov_log_test_c_lets_log_something = get_file_ptr(fh);
  testrun(0 != ov_log_test_c_lets_log_something);

  testrun(ov_log_set_output("ov_log_test.c", "lets_log_something",
                            OV_LOG_WARNING,
                            (ov_log_output){
                                .filehandle = fh,
                            }));

  lets_log_something(OV_LOG_ERR);

  testrun(stdout_empty());
  testrun(stderr_empty());
  testrun(file_empty(aaa));
  testrun(file_empty(ov_log_test_c));
  testrun(file_empty(ov_log_test_c_not_our_func));
  testrun(!file_empty(ov_log_test_c_lets_log_something));

  testrun(file_clear(ov_log_test_c_lets_log_something));

  /*************************************************************************
                                 Tear down
   ************************************************************************/

  fclose(aaa);
  aaa = 0;
  fclose(ov_log_test_c);
  ov_log_test_c = 0;
  fclose(ov_log_test_c_not_our_func);
  ov_log_test_c_not_our_func = 0;
  fclose(ov_log_test_c_lets_log_something);
  ov_log_test_c_lets_log_something = 0;

  ov_log_close();

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_log_level_from_string() {

  testrun(OV_LOG_INVALID == ov_log_level_from_string(0));
  testrun(OV_LOG_INVALID == ov_log_level_from_string("Drugstore from Hell"));

  testrun(OV_LOG_EMERG == ov_log_level_from_string("emergency"));
  testrun(OV_LOG_ALERT == ov_log_level_from_string("alert"));
  testrun(OV_LOG_CRIT == ov_log_level_from_string("critical"));
  testrun(OV_LOG_ERR == ov_log_level_from_string("error"));
  testrun(OV_LOG_WARNING == ov_log_level_from_string("warning"));
  testrun(OV_LOG_NOTICE == ov_log_level_from_string("notice"));
  testrun(OV_LOG_INFO == ov_log_level_from_string("info"));
  testrun(OV_LOG_DEBUG == ov_log_level_from_string("debug"));
  testrun(OV_LOG_DEV == ov_log_level_from_string("dev"));
  testrun(OV_LOG_INVALID == ov_log_level_from_string("invalid"));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_log_level_to_string() {

  testrun(0 == strcmp("emergency", ov_log_level_to_string(OV_LOG_EMERG)));
  testrun(0 == strcmp("alert", ov_log_level_to_string(OV_LOG_ALERT)));
  testrun(0 == strcmp("critical", ov_log_level_to_string(OV_LOG_CRIT)));
  testrun(0 == strcmp("error", ov_log_level_to_string(OV_LOG_ERR)));
  testrun(0 == strcmp("warning", ov_log_level_to_string(OV_LOG_WARNING)));
  testrun(0 == strcmp("notice", ov_log_level_to_string(OV_LOG_NOTICE)));
  testrun(0 == strcmp("info", ov_log_level_to_string(OV_LOG_INFO)));
  testrun(0 == strcmp("debug", ov_log_level_to_string(OV_LOG_DEBUG)));
  testrun(0 == strcmp("dev", ov_log_level_to_string(OV_LOG_DEV)));
  testrun(0 == strcmp("invalid", ov_log_level_to_string(OV_LOG_INVALID)));

  testrun(0 == ov_log_level_to_string(-1));
  testrun(0 == ov_log_level_to_string(UINT16_MAX));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int tear_down() {

  remove_tmp_dir();

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

RUN_TESTS("ov_log", init, test_ov_log, test_ov_log_level_from_string,
          test_ov_log_level_to_string, tear_down);
