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
        @file           ov_turn_channel_test.c
        @author         Markus Töpfer

        @date           2022-01-31


        ------------------------------------------------------------------------
*/
#include "ov_turn_channel.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_turn_channel_create() {

  ov_socket_data remote = (ov_socket_data){.host = "localhost", .port = 12345};

  ov_turn_channel *channel = NULL;

  channel = ov_turn_channel_create(0, (ov_socket_data){0});
  testrun(channel);
  testrun(channel->number == 0);
  testrun(channel->remote.host[0] == 0);
  testrun(channel->remote.port == 0);
  channel = ov_turn_channel_free(channel);

  channel = ov_turn_channel_create(1, remote);
  testrun(channel);
  testrun(channel->number == 1);
  testrun(channel->remote.host[0] != 0);
  testrun(channel->remote.port == 12345);
  testrun(0 == memcmp(channel->remote.host, remote.host, OV_HOST_NAME_MAX));
  channel = ov_turn_channel_free(channel);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_turn_channel_free() {

  ov_socket_data remote = (ov_socket_data){.host = "localhost", .port = 12345};

  ov_turn_channel *channel = NULL;

  testrun(NULL == ov_turn_channel_free(NULL));

  channel = ov_turn_channel_create(0, (ov_socket_data){0});
  testrun(channel);
  testrun(NULL == ov_turn_channel_free(channel));

  channel = ov_turn_channel_create(1, remote);
  testrun(channel);
  testrun(NULL == ov_turn_channel_free(channel));

  return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

  testrun_init();
  testrun_test(test_ov_turn_channel_create);
  testrun_test(test_ov_turn_channel_free);

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
