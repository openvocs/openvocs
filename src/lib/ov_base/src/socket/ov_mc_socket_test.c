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
        @file           ov_mc_socket_test.c
        @author         Markus Töpfer

        @date           2022-11-09


        ------------------------------------------------------------------------
*/
#include "ov_mc_socket.c"
#include <ov_test/testrun.h>

#include "../include/ov_mc_test_common.h"

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_mc_socket() {

  ov_socket_configuration config = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = UDP, .host = "239.255.255.255"});

  int server = ov_mc_test_common_open_socket(AF_INET);
  testrun(-1 != server);

  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(config.host);
  address.sin_port = htons(config.port);

  int client = ov_mc_socket(config);
  testrun(-1 != client);

  char *out = "test";
  char buffer[1024] = {0};

  ssize_t bytes = sendto(server, out, 4, 0, (struct sockaddr *)&address,
                         sizeof(struct sockaddr_in));

  testrun(4 == bytes);

  bytes = -1;

  struct sockaddr_in in = {0};
  socklen_t in_len;

  while (-1 == bytes) {

    in_len = sizeof(in);
    bytes = recvfrom(client, buffer, 1024, 0, (struct sockaddr *)&in, &in_len);
  }

  testrun(4 == bytes);
  testrun(0 == strcmp(buffer, out));

  testrun(ov_mc_socket_drop_membership(client));
  close(client);
  close(server);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

  testrun_init();
  testrun_test(test_ov_mc_socket);

  return testrun_counter;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST EXECUTION                                                  #EXEC
 *
 *      ------------------------------------------------------------------------
 */

testrun_run(all_tests);
