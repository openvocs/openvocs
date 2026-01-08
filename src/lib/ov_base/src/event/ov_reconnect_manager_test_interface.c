/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

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

#include "../../include/ov_utils.h"

#include <ov_test/ov_test_tcp.h>

#include "../../include/ov_reconnect_manager_test_interface.h"
#include <ov_test/testrun.h>

/******************************************************************************
 *                      ov_reconnect_manager_connect_test
 ******************************************************************************/

static int test_userdata_for_server = 23;
static int server_callback_called = 0;

static void serve_client_fd(int fd, void *userdata) {

  /* This will be executed in own process, thus we cannot share memory */

  OV_ASSERT(0 != userdata);
  OV_ASSERT(&test_userdata_for_server == userdata);

  ssize_t octets = 0;

  int recvd = 0;

  octets = ov_test_tcp_read_int(fd, &recvd);

  if (octets != sizeof(recvd)) {
    testrun_log("Server could not read:%s\n", strerror(-octets));
  }

  /* increase counter and bounce back */
  server_callback_called += 1;

  octets = write(fd, &server_callback_called, sizeof(server_callback_called));

  if (octets != sizeof(server_callback_called)) {
    testrun_log("Server could not write:%s\n", strerror(octets));
  }

  close(fd);
  testrun_log("Server closed %i", fd);
}

/*----------------------------------------------------------------------------*/

static int server_called_counter = 0;

static size_t io_callback_reads_counter = 0;

static size_t socket_reconnected_counter = 0;

static bool userdata_correct = true;

/*----------------------------------------------------------------------------*/

static void *test_userdata = ov_reconnect_manager_connect;

static bool io_callback(int fd, uint8_t events, void *userdata) {

  userdata_correct = userdata_correct && (test_userdata == userdata);

  if (OV_EVENT_IO_CLOSE & events) {

    socket_reconnected_counter += 1;
    return true;
  }

  if (0 == (OV_EVENT_IO_IN & events)) {

    testrun_log("io_callback: Unexpected event: %" PRIu8 "\n", events);

    return false;
  }

  int recvd = 0;

  int retval = ov_test_tcp_read_int(fd, &recvd);

  if (0 == retval) {

    return false;
  }

  if (sizeof(recvd) != retval) {

    testrun_log("Could not read from fd %i: "
                "retval: %i(%s), "
                "errno: %i(%s)\n",
                fd, retval, strerror(-retval), errno, strerror(errno));

    return false;
  }

  server_called_counter = recvd;
  io_callback_reads_counter += 1;

  testrun_log("server_called_counter: %i "
              "io_callback_reads_counter: %zu   "
              "socket_reconnected_counter: "
              "%zu\n",
              server_called_counter, io_callback_reads_counter,
              socket_reconnected_counter);

  return true;
}

/*----------------------------------------------------------------------------*/

static bool reconnected_callback(int fd, void *userdata) {

  UNUSED(userdata);

  OV_ASSERT(0 < fd);

  socket_reconnected_counter += 1;

  ov_test_tcp_disable_nagls_alg(fd);

  /* All we do is triggering an I/O write on the other end to
   * trigger a n io_callback  */

  int res = write(fd, &server_called_counter, sizeof(server_called_counter));

  if (sizeof(server_called_counter) != res) {

    testrun_log("Could not write on reconnect: %s\n", strerror(res));
  }

  userdata_correct = userdata_correct && (test_userdata == userdata);

  return true;
}

/*----------------------------------------------------------------------------*/

int ov_reconnect_manager_connect_test(
    ov_event_loop *(*loop_create)(ov_event_loop_config cfg)) {

  OV_ASSERT(0 != loop_create);

  int server_port = -1;
  pid_t server_pid;

  const size_t loop_runtime_secs = 30;
  const size_t reconnect_timer_interval_secs = 3;

  ov_test_tcp_server_config cfg = {
      .serve_client_callback = serve_client_fd,
      .userdata = &test_userdata_for_server,
  };

  testrun(ov_test_tcp_server(&server_pid, &server_port, cfg));

  testrun_log("Our test server listens on %i, with pid %i\n", server_port,
              server_pid);

  ov_socket_configuration server_scfg = {0};

  ov_reconnect_callbacks cbs = {0};

  testrun(!ov_reconnect_manager_connect(0, server_scfg, 0, cbs, 0));

  testrun(0 == io_callback_reads_counter);
  testrun(0 == socket_reconnected_counter);
  testrun(userdata_correct);

  server_scfg = (ov_socket_configuration){

      .host = "localhost",
      .port = server_port,
      .type = TCP,

  };

  testrun(!ov_reconnect_manager_connect(0, server_scfg, 0, cbs, 0));
  testrun(
      !ov_reconnect_manager_connect(0, server_scfg, OV_EVENT_IO_IN, cbs, 0));

  cbs.io = io_callback;

  testrun(!ov_reconnect_manager_connect(0, server_scfg, 0, cbs, 0));

  testrun(
      !ov_reconnect_manager_connect(0, server_scfg, OV_EVENT_IO_IN, cbs, 0));

  cbs.io = 0;
  cbs.reconnected = reconnected_callback;

  testrun(!ov_reconnect_manager_connect(0, server_scfg, 0, cbs, 0));

  testrun(
      !ov_reconnect_manager_connect(0, server_scfg, OV_EVENT_IO_IN, cbs, 0));

  cbs.io = io_callback;

  testrun(!ov_reconnect_manager_connect(0, server_scfg, 0, cbs, 0));

  testrun(
      !ov_reconnect_manager_connect(0, server_scfg, OV_EVENT_IO_IN, cbs, 0));

  cbs = (ov_reconnect_callbacks){0};

  testrun(!ov_reconnect_manager_connect(0, server_scfg, 0, cbs, test_userdata));

  testrun(!ov_reconnect_manager_connect(0, server_scfg, OV_EVENT_IO_IN, cbs,
                                        test_userdata));

  cbs.io = io_callback;

  testrun(!ov_reconnect_manager_connect(0, server_scfg, 0, cbs, test_userdata));

  cbs = (ov_reconnect_callbacks){
      .reconnected = reconnected_callback,
  };

  testrun(!ov_reconnect_manager_connect(0, server_scfg, 0, cbs, test_userdata));

  testrun(!ov_reconnect_manager_connect(0, server_scfg, OV_EVENT_IO_IN, cbs,
                                        test_userdata));

  cbs.io = io_callback;

  testrun(!ov_reconnect_manager_connect(0, server_scfg, OV_EVENT_IO_IN, cbs,
                                        test_userdata));

  testrun(0 == io_callback_reads_counter);
  testrun(0 == socket_reconnected_counter);
  testrun(userdata_correct);

  ov_event_loop *loop = loop_create(ov_event_loop_config_default());

  /* Explicite timer interval of 3 sec to be able to calculate
   * counters precisely */
  ov_reconnect_manager *rm =
      ov_reconnect_manager_create(loop, reconnect_timer_interval_secs, 5);

  cbs = (ov_reconnect_callbacks){
      .io = io_callback,
      .reconnected = reconnected_callback,
  };

  testrun(ov_reconnect_manager_connect(rm, server_scfg, OV_EVENT_IO_IN, cbs,
                                       test_userdata));

  testrun(0 == io_callback_reads_counter);
  testrun(0 == socket_reconnected_counter);
  testrun(userdata_correct);

  loop->run(loop, loop_runtime_secs * 1000 * 1000);

  testrun_log("server_called_counter: %i "
              "io_callback_reads_counter: %zu   "
              "socket_reconnected_counter: "
              "%zu\n",
              server_called_counter, io_callback_reads_counter,
              socket_reconnected_counter);

  size_t expected_count = loop_runtime_secs / reconnect_timer_interval_secs;

  testrun(-1 < server_called_counter);

  testrun(expected_count - 1 <= (unsigned)server_called_counter);
  testrun(expected_count - 1 <= io_callback_reads_counter);
  testrun(expected_count - 1 <= socket_reconnected_counter);

  testrun(expected_count + 1 >= (unsigned)server_called_counter);
  testrun(expected_count + 1 >= io_callback_reads_counter);
  testrun(expected_count + 1 >= socket_reconnected_counter);

  testrun(userdata_correct);

  /* Check again with 2 connections */

  /* Server needs to be stopped to reset counter on server side */

  rm = ov_reconnect_manager_free(rm);
  loop = ov_event_loop_free(loop);

  loop = loop_create(ov_event_loop_config_default());
  rm = ov_reconnect_manager_create(loop, reconnect_timer_interval_secs, 5);

  testrun(ov_test_tcp_stop_process(server_pid));
  testrun(ov_test_tcp_server(&server_pid, &server_port, cfg));

  testrun_log("Our test server listens on %i, with pid %i\n", server_port,
              server_pid);

  server_scfg.port = server_port;

  server_called_counter = 0;
  io_callback_reads_counter = 0;
  socket_reconnected_counter = 0;

  /* Connect 2 clients */
  testrun(ov_reconnect_manager_connect(rm, server_scfg, OV_EVENT_IO_IN, cbs,
                                       test_userdata));

  testrun(ov_reconnect_manager_connect(rm, server_scfg, OV_EVENT_IO_IN, cbs,
                                       test_userdata));

  loop->run(loop, loop_runtime_secs * 1000 * 1000);

  testrun_log("server_called_counter: %i "
              "io_callback_reads_counter: %zu   "
              "socket_reconnected_counter: "
              "%zu\n",
              server_called_counter, io_callback_reads_counter,
              socket_reconnected_counter);

  testrun(-1 < server_called_counter);

  /* twice as many connections */
  expected_count *= 2;

  testrun(expected_count - 1 <= (unsigned)server_called_counter);
  testrun(expected_count - 1 <= io_callback_reads_counter);
  testrun(expected_count - 1 <= socket_reconnected_counter);

  testrun(expected_count + 1 >= (unsigned)server_called_counter);
  testrun(expected_count + 1 >= io_callback_reads_counter);
  testrun(expected_count + 1 >= socket_reconnected_counter);

  testrun(ov_test_tcp_stop_process(server_pid));

  rm = ov_reconnect_manager_free(rm);

  loop = ov_event_loop_free(loop);

  testrun(0 == rm);
  testrun(0 == loop);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/
