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
        @file           ov_vocs_loop_test.c
        @author         Markus Töpfer

        @date           2022-11-08


        ------------------------------------------------------------------------
*/
#include "ov_vocs_loop.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_vocs_loop_create() {

  ov_vocs_loop *loop = ov_vocs_loop_create(NULL);
  testrun(!loop);

  loop = ov_vocs_loop_create("name");
  testrun(loop);
  testrun(ov_vocs_loop_cast(loop));
  testrun(0 == strcmp(loop->name, "name"));
  testrun(loop->participants);
  testrun(0 == ov_dict_count(loop->participants));

  testrun(NULL == ov_vocs_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_loop_cast() {

  ov_vocs_loop *loop = ov_vocs_loop_create("name");
  testrun(loop);
  testrun(ov_vocs_loop_cast(loop));

  for (size_t i = 0; i < 0xFFFF; i++) {

    loop->magic_bytes = i;
    if (i == OV_VOCS_LOOP_MAGIC_BYTES) {
      testrun(ov_vocs_loop_cast(loop));
    } else {
      testrun(!ov_vocs_loop_cast(loop));
    }
  }

  loop->magic_bytes = OV_VOCS_LOOP_MAGIC_BYTES;
  testrun(NULL == ov_vocs_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_loop_free() {

  ov_vocs_loop *loop = ov_vocs_loop_create("name");
  testrun(loop);
  testrun(NULL == ov_vocs_loop_free(loop));

  loop = ov_vocs_loop_create("name");
  testrun(ov_vocs_loop_add_participant(loop, 1, "client1", "user1", "role1"));
  testrun(ov_vocs_loop_add_participant(loop, 2, "client2", "user2", "role2"));
  testrun(ov_vocs_loop_add_participant(loop, 3, "client3", "user3", "role3"));
  testrun(ov_vocs_loop_add_participant(loop, 4, "client4", "user4", "role4"));

  testrun(NULL == ov_vocs_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_loop_get_participants_count() {

  ov_vocs_loop *loop = ov_vocs_loop_create("name");
  testrun(loop);

  testrun(-1 == ov_vocs_loop_get_participants_count(NULL));
  testrun(0 == ov_vocs_loop_get_participants_count(loop));

  testrun(ov_vocs_loop_add_participant(loop, 1, "client1", "user1", "role1"));
  testrun(1 == ov_vocs_loop_get_participants_count(loop));
  testrun(ov_vocs_loop_add_participant(loop, 2, "client2", "user2", "role2"));
  testrun(2 == ov_vocs_loop_get_participants_count(loop));
  testrun(ov_vocs_loop_add_participant(loop, 3, "client3", "user3", "role3"));
  testrun(3 == ov_vocs_loop_get_participants_count(loop));
  testrun(ov_vocs_loop_add_participant(loop, 4, "client4", "user4", "role4"));
  testrun(4 == ov_vocs_loop_get_participants_count(loop));

  testrun(NULL == ov_vocs_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_loop_add_participant() {

  ov_vocs_loop *loop = ov_vocs_loop_create("name");
  testrun(loop);

  testrun(!ov_vocs_loop_add_participant(NULL, 1, "client1", "user1", "role1"));
  testrun(!ov_vocs_loop_add_participant(loop, 1, "client1", NULL, "role1"));
  testrun(!ov_vocs_loop_add_participant(loop, 1, "client1", "user1", NULL));
  testrun(0 == ov_vocs_loop_get_participants_count(loop));

  testrun(!ov_vocs_loop_add_participant(loop, 0, "client0", "user0", "role0"));
  testrun(0 == ov_vocs_loop_get_participants_count(loop));

  // override
  testrun(ov_vocs_loop_add_participant(loop, 1, "client0", "user0", "role0"));
  testrun(1 == ov_vocs_loop_get_participants_count(loop));
  testrun(ov_vocs_loop_add_participant(loop, 1, NULL, "user1", "role1"));
  testrun(1 == ov_vocs_loop_get_participants_count(loop));
  Participant *p = ov_dict_get(loop->participants, (void *)(intptr_t)1);
  testrun(NULL == p->client);

  testrun(ov_vocs_loop_add_participant(loop, 1, "client1", "user1", "role1"));
  testrun(1 == ov_vocs_loop_get_participants_count(loop));
  p = ov_dict_get(loop->participants, (void *)(intptr_t)1);
  testrun(NULL != p->client);

  testrun(ov_vocs_loop_add_participant(loop, 2, "client2", "user2", "role2"));
  testrun(2 == ov_vocs_loop_get_participants_count(loop));
  testrun(ov_vocs_loop_add_participant(loop, 3, "client3", "user3", "role3"));
  testrun(3 == ov_vocs_loop_get_participants_count(loop));
  testrun(ov_vocs_loop_add_participant(loop, 4, "client4", "user4", "role4"));
  testrun(4 == ov_vocs_loop_get_participants_count(loop));

  testrun(NULL == ov_vocs_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_loop_drop_participant() {

  ov_vocs_loop *loop = ov_vocs_loop_create("name");
  testrun(loop);

  testrun(!ov_vocs_loop_drop_participant(NULL, 1));
  testrun(ov_vocs_loop_drop_participant(loop, 1));

  testrun(ov_vocs_loop_add_participant(loop, 1, "client0", "user0", "role0"));
  testrun(ov_vocs_loop_add_participant(loop, 2, "client2", "user2", "role2"));
  testrun(ov_vocs_loop_add_participant(loop, 3, "client3", "user3", "role3"));
  testrun(ov_vocs_loop_add_participant(loop, 4, "client4", "user4", "role4"));
  testrun(4 == ov_vocs_loop_get_participants_count(loop));

  testrun(ov_vocs_loop_drop_participant(loop, 2));
  testrun(3 == ov_vocs_loop_get_participants_count(loop));
  testrun(ov_vocs_loop_drop_participant(loop, 1));
  testrun(2 == ov_vocs_loop_get_participants_count(loop));
  testrun(ov_vocs_loop_drop_participant(loop, 4));
  testrun(1 == ov_vocs_loop_get_participants_count(loop));
  testrun(ov_vocs_loop_drop_participant(loop, 3));
  testrun(0 == ov_vocs_loop_get_participants_count(loop));

  testrun(NULL == ov_vocs_loop_free(loop));

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
  testrun_test(test_ov_vocs_loop_create);
  testrun_test(test_ov_vocs_loop_cast);
  testrun_test(test_ov_vocs_loop_free);

  testrun_test(test_ov_vocs_loop_get_participants_count);

  testrun_test(test_ov_vocs_loop_add_participant);
  testrun_test(test_ov_vocs_loop_drop_participant);

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
