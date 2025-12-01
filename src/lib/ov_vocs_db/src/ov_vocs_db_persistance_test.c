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
        @file           ov_vocs_db_persistance_test.c
        @author         Markus

        @date           2022-07-26


        ------------------------------------------------------------------------
*/
#include "ov_vocs_db_persistance.c"
#include <ov_test/testrun.h>

#include "../include/ov_vocs_test_db.h"

#ifndef OV_TEST_RESOURCE_DIR
#error "Must provide -D OV_TEST_RESOURCE_DIR=value while compiling this file."
#endif

#include <ov_base/ov_time.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_vocs_db_persistance_create() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});

  testrun(loop);
  testrun(db);

  ov_vocs_db_persistance *self = ov_vocs_db_persistance_create(
      (ov_vocs_db_persistance_config){.loop = loop, .db = db});

  testrun(self);
  testrun(self->config.timeout.thread_lock_usec == IMPL_DEFAULT_LOCK_USEC);
  testrun(OV_TIMER_INVALID == self->timer.auth_snapshot);
  testrun(OV_TIMER_INVALID == self->timer.state_snapshot);

  testrun(ov_thread_lock_try_lock(&self->lock));
  testrun(ov_thread_lock_unlock(&self->lock));

  testrun(NULL == ov_vocs_db_persistance_free(self));

  self = ov_vocs_db_persistance_create(
      (ov_vocs_db_persistance_config){.loop = loop,
                                      .db = db,
                                      .timeout.thread_lock_usec = 10,
                                      .timeout.state_snapshot_seconds = 1,
                                      .timeout.auth_snapshot_seconds = 1});

  testrun(self);
  testrun(self->config.timeout.thread_lock_usec == 10);
  testrun(OV_TIMER_INVALID != self->timer.auth_snapshot);
  testrun(OV_TIMER_INVALID != self->timer.state_snapshot);

  testrun(NULL == ov_vocs_db_persistance_free(self));
  testrun(NULL == ov_vocs_db_free(db));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_persistance_free() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});

  testrun(loop);
  testrun(db);

  ov_vocs_db_persistance *self = ov_vocs_db_persistance_create(
      (ov_vocs_db_persistance_config){.loop = loop, .db = db});

  testrun(NULL == ov_vocs_db_persistance_free(self));
  testrun((void *)loop ==
          (void *)ov_vocs_db_persistance_free((ov_vocs_db_persistance *)loop));
  testrun((void *)db ==
          (void *)ov_vocs_db_persistance_free((ov_vocs_db_persistance *)db));

  testrun(NULL == ov_vocs_db_free(db));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_persistance_cast() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});

  testrun(loop);
  testrun(db);

  ov_vocs_db_persistance *self = ov_vocs_db_persistance_create(
      (ov_vocs_db_persistance_config){.loop = loop, .db = db});

  for (size_t i = 0; i < 0xffff; i++) {

    self->magic_byte = i;
    if (i == OV_VOCS_DB_PERSISTANCE_MAGIC_BYTE) {
      testrun(ov_vocs_db_persistance_cast(self));
    } else {
      testrun(!ov_vocs_db_persistance_cast(self));
    }
  }

  self->magic_byte = OV_VOCS_DB_PERSISTANCE_MAGIC_BYTE;

  testrun(NULL == ov_vocs_db_persistance_free(self));
  testrun(NULL == ov_vocs_db_free(db));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_persistance_load() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});

  testrun(loop);
  testrun(db);

  ov_vocs_db_persistance_config c = (ov_vocs_db_persistance_config){
      .loop = loop, .db = db, .path = OV_TEST_RESOURCE_DIR};

  ov_vocs_db_persistance *self = ov_vocs_db_persistance_create(c);

  uint64_t start = ov_time_get_current_time_usecs();
  testrun(ov_vocs_db_persistance_load(self));
  uint64_t stop = ov_time_get_current_time_usecs();
  fprintf(stdout, "... loaded in %" PRIu64 " usec\n", stop - start);

  ov_json_value *out =
      ov_vocs_db_get_entity(db, OV_VOCS_DB_PROJECT, "project@localhost");
  testrun(out);
  out = ov_json_value_free(out);

  testrun(NULL == ov_vocs_db_persistance_free(self));
  testrun(NULL == ov_vocs_db_free(db));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_persistance_save() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});

  testrun(loop);
  testrun(db);

  ov_vocs_db_persistance_config c = (ov_vocs_db_persistance_config){
      .loop = loop, .db = db, .path = OV_TEST_RESOURCE_DIR};

  ov_vocs_db_persistance *self = ov_vocs_db_persistance_create(c);
  testrun(ov_vocs_db_persistance_load(self));

  testrun(ov_vocs_db_persistance_save(self));
  testrun(ov_vocs_db_persistance_load(self));

  ov_json_value *cpy = ov_vocs_db_eject(db, OV_VOCS_DB_TYPE_AUTH);
  testrun(cpy);

  ov_json_value *out =
      ov_vocs_db_get_entity(db, OV_VOCS_DB_PROJECT, "project@localhost");
  testrun(out);
  out = ov_json_value_free(out);

  testrun(
      ov_vocs_db_delete_entity(db, OV_VOCS_DB_PROJECT, "project@localhost"));

  testrun(!ov_vocs_db_get_entity(db, OV_VOCS_DB_PROJECT, "project@localhost"));

  uint64_t start = ov_time_get_current_time_usecs();
  testrun(ov_vocs_db_persistance_save(self));
  uint64_t stop1 = ov_time_get_current_time_usecs();
  testrun(ov_vocs_db_persistance_load(self));
  uint64_t stop2 = ov_time_get_current_time_usecs();

  fprintf(stdout, "save %" PRIu64 " usec load %" PRIu64 " usec\n",
          stop1 - start, stop2 - stop1);

  out = ov_vocs_db_get_entity(db, OV_VOCS_DB_USER, "admin");
  testrun(out);
  out = ov_json_value_free(out);

  // add some state to the system

  testrun(ov_vocs_db_set_state(db, "user1", "role1", "loop1", OV_VOCS_SEND));
  testrun(ov_vocs_db_set_state(db, "user1", "role2", "loop1", OV_VOCS_SEND));
  testrun(ov_vocs_db_set_state(db, "user1", "role1", "loop2", OV_VOCS_SEND));
  testrun(ov_vocs_db_set_volume(db, "user1", "role1", "loop1", 40));
  testrun(ov_vocs_db_set_volume(db, "user1", "role2", "loop1", 50));
  testrun(ov_vocs_db_set_volume(db, "user1", "role1", "loop2", 60));

  start = ov_time_get_current_time_usecs();
  testrun(ov_vocs_db_persistance_save(self));
  stop1 = ov_time_get_current_time_usecs();
  testrun(ov_vocs_db_persistance_load(self));
  stop2 = ov_time_get_current_time_usecs();

  fprintf(stdout, "save %" PRIu64 " usec load %" PRIu64 " usec\n",
          stop1 - start, stop2 - stop1);

  testrun(OV_VOCS_SEND == ov_vocs_db_get_state(db, "user1", "role1", "loop1"));
  testrun(OV_VOCS_SEND == ov_vocs_db_get_state(db, "user1", "role2", "loop1"));
  testrun(OV_VOCS_SEND == ov_vocs_db_get_state(db, "user1", "role1", "loop2"));
  testrun(40 == ov_vocs_db_get_volume(db, "user1", "role1", "loop1"));
  testrun(50 == ov_vocs_db_get_volume(db, "user1", "role2", "loop1"));
  testrun(60 == ov_vocs_db_get_volume(db, "user1", "role1", "loop2"));

  // reset filesystem data
  testrun(ov_vocs_db_inject(db, OV_VOCS_DB_TYPE_AUTH, cpy));
  testrun(ov_vocs_db_persistance_save(self));

  testrun(NULL == ov_vocs_db_persistance_free(self));
  testrun(NULL == ov_vocs_db_free(db));
  testrun(NULL == ov_event_loop_free(loop));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool create_db(ov_vocs_db *db, int domains, int projects, int items) {

  ov_json_value *empty = ov_json_object();
  if (!ov_vocs_db_inject(db, OV_VOCS_DB_TYPE_AUTH, empty))
    goto error;
  empty = ov_json_object();
  if (!ov_vocs_db_inject(db, OV_VOCS_DB_TYPE_STATE, empty))
    goto error;

  // ov_vocs_db_dump(stdout, db);

  char id[100] = {0};

  for (int i = 0; i < domains; i++) {

    // fprintf(stdout, "i %i\n",i);

    snprintf(id, 100, "id%i", i);

    if (!ov_vocs_db_create_entity(db, OV_VOCS_DB_DOMAIN, id,
                                  OV_VOCS_DB_SCOPE_DOMAIN, NULL))
      goto error;

    for (int x = 0; x < projects; x++) {

      // fprintf(stdout, "i %i %i\n",i, x);

      char project[100] = {0};
      snprintf(project, 100, "project%i%i", i, x);

      if (!ov_vocs_db_create_entity(db, OV_VOCS_DB_PROJECT, project,
                                    OV_VOCS_DB_SCOPE_DOMAIN, id))
        goto error;

      for (int y = 0; y < items; y++) {

        // fprintf(stdout, "i %i %i %i\n",i, x, y);

        char name[100] = {0};
        snprintf(name, 100, "name%i%i%i", i, x, y);

        if (!ov_vocs_db_create_entity(db, OV_VOCS_DB_LOOP, name,
                                      OV_VOCS_DB_SCOPE_PROJECT, project))
          goto error;

        if (!ov_vocs_db_create_entity(db, OV_VOCS_DB_ROLE, name,
                                      OV_VOCS_DB_SCOPE_PROJECT, project))
          goto error;

        if (!ov_vocs_db_create_entity(db, OV_VOCS_DB_USER, name,
                                      OV_VOCS_DB_SCOPE_PROJECT, project))
          goto error;
      }
    }
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

int check_performance() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});

  testrun(loop);
  testrun(db);

  ov_vocs_db_persistance_config c = (ov_vocs_db_persistance_config){
      .loop = loop, .db = db, .path = OV_TEST_RESOURCE_DIR};

  ov_vocs_db_persistance *self = ov_vocs_db_persistance_create(c);
  testrun(ov_vocs_db_persistance_load(self));

  ov_json_value *cpy = ov_vocs_db_eject(db, OV_VOCS_DB_TYPE_AUTH);
  testrun(cpy);

  testrun(create_db(db, 2, 3, 4));

  uint64_t start = 0;
  uint64_t stop1 = 0;
  uint64_t stop2 = 0;

  start = ov_time_get_current_time_usecs();
  testrun(ov_vocs_db_persistance_save(self));
  stop1 = ov_time_get_current_time_usecs();
  testrun(ov_vocs_db_persistance_load(self));
  stop2 = ov_time_get_current_time_usecs();

  fprintf(stdout, "234 save %" PRIu64 " usec load %" PRIu64 " usec\n",
          stop1 - start, stop2 - stop1);

  testrun(create_db(db, 10, 10, 10));

  start = ov_time_get_current_time_usecs();
  testrun(ov_vocs_db_persistance_save(self));
  stop1 = ov_time_get_current_time_usecs();
  testrun(ov_vocs_db_persistance_load(self));
  stop2 = ov_time_get_current_time_usecs();

  fprintf(stdout, "101010 save %" PRIu64 " usec load %" PRIu64 " usec\n",
          stop1 - start, stop2 - stop1);

  // reset filesystem data
  testrun(ov_vocs_db_inject(db, OV_VOCS_DB_TYPE_AUTH, cpy));
  testrun(ov_vocs_db_persistance_save(self));

  testrun(NULL == ov_vocs_db_persistance_free(self));
  testrun(NULL == ov_vocs_db_free(db));
  testrun(NULL == ov_event_loop_free(loop));

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

  testrun_test(test_ov_vocs_db_persistance_create);
  testrun_test(test_ov_vocs_db_persistance_free);
  testrun_test(test_ov_vocs_db_persistance_cast);

  testrun_test(test_ov_vocs_db_persistance_save);
  testrun_test(test_ov_vocs_db_persistance_load);

  testrun_test(check_performance);

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
