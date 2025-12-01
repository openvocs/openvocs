/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_vm_prog_db_test.c
        @author         Michael J. Beer

        @date           2019-10-11

        ------------------------------------------------------------------------
*/

#include <ov_test/ov_test.h>

#include "ov_vm_prog_db.c"
#include <ov_base/ov_id.h>
#include <ov_base/ov_random.h>

ov_vm_instr test_instr[] = {{1, 2, 0}, {2, 3, 0}, {3, 1, 0}, {0, 0, 0}};
ov_vm_instr test_instr_2[] = {{9, 3, 0}, {8, 2, 0}, {7, 1, 0}, {0, 0, 0}};

/*----------------------------------------------------------------------------*/

static void random_id(char *id) {

  ov_random_string(&id, OV_VM_PROG_ID_MAX_LEN, 0);
  size_t end = ov_random_range(1, OV_VM_PROG_ID_MAX_LEN - 1);
  id[end] = 0;
}

/*----------------------------------------------------------------------------*/

#define NO_IDS ((size_t)5)

static bool id_contained_in(char const *id,
                            char const ids[][OV_VM_PROG_ID_MAX_LEN]) {

  for (size_t i = 0; i < NO_IDS; ++i) {

    if (0 == strncmp(id, ids[i], OV_VM_PROG_ID_MAX_LEN)) {
      return true;
    }
  }

  return false;
}

/*----------------------------------------------------------------------------*/

static void random_unique_id(char *id,
                             const char ids[][OV_VM_PROG_ID_MAX_LEN]) {

  do {
    random_id(id);
  } while (id_contained_in(id, ids));
}

/*----------------------------------------------------------------------------*/

static void random_unique_id_at(char ids[][OV_VM_PROG_ID_MAX_LEN], size_t i) {

  char id[OV_VM_PROG_ID_MAX_LEN] = {0};
  random_unique_id(id, ids);
  strncpy(ids[i], id, OV_VM_PROG_ID_MAX_LEN);
}

/*----------------------------------------------------------------------------*/

static size_t num_release_called = 0;
static void *release_additional = 0;

static void release(void *data, void *additional) {

  ov_free(data);
  num_release_called += 1;
  release_additional = additional;
}

/*---------------------------------------------------------------------------*/

static int test_ov_vm_prog_db_create() {

  ov_vm_prog_db *store = 0;

  store = ov_vm_prog_db_create(0, 0, 0);
  testrun(0 != store);
  testrun(0 == ov_vm_prog_db_free(store));

  int additional = 1;

  store = ov_vm_prog_db_create(0, &additional, 0);
  testrun(0 != store);
  testrun(0 == ov_vm_prog_db_free(store));

  store = ov_vm_prog_db_create(0, 0, release);
  testrun(0 != store);
  testrun(0 == ov_vm_prog_db_free(store));

  store = ov_vm_prog_db_create(1, 0, 0);
  testrun(0 != store);
  testrun(0 == ov_vm_prog_db_free(store));

  store = ov_vm_prog_db_create(1, &additional, 0);
  testrun(0 != store);
  testrun(0 == ov_vm_prog_db_free(store));

  store = ov_vm_prog_db_create(0, &additional, release);
  testrun(0 != store);
  testrun(0 == ov_vm_prog_db_free(store));

  store = ov_vm_prog_db_create(1, &additional, release);
  testrun(0 != store);
  testrun(0 == ov_vm_prog_db_free(store));

  testrun(0 == num_release_called);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static char *strdup_if_demanded(char *str, bool allocate) {

  if (allocate) {
    return strdup(str);
  } else {
    return str;
  }
}

/*----------------------------------------------------------------------------*/

static void insert_random_entries(ov_vm_prog_db *db, size_t num,
                                  bool allocate_data) {

  char ids[num][OV_VM_PROG_ID_MAX_LEN];
  memset(ids, 0, num * OV_VM_PROG_ID_MAX_LEN);

  for (size_t i = 0; i < num; ++i) {
    random_unique_id_at(ids, i);
    ov_vm_prog_db_insert(db, ids[i], test_instr,
                         strdup_if_demanded("aha", allocate_data));
  }
}

/*----------------------------------------------------------------------------*/

static int test_ov_vm_prog_db_free() {

  // Most cases are already dealt with in test_create

  ov_vm_prog_db *store = 0;

  store = ov_vm_prog_db_create(12, 0, release);
  testrun(0 != store);

  insert_random_entries(store, 5, true);

  testrun(0 == ov_vm_prog_db_free(store));

  testrun(5 == num_release_called);

  num_release_called = 0;

  // Same without data release function

  store = ov_vm_prog_db_create(12, 0, 0);
  testrun(0 != store);

  insert_random_entries(store, 5, false);

  testrun(0 == ov_vm_prog_db_free(store));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_vm_prog_db_insert() {

  char const *id = "muhaha";

  ov_vm_prog_db *store = 0;

  store = ov_vm_prog_db_create(NO_IDS, 0, release);
  testrun(0 != store);

  testrun(0 == ov_vm_prog_db_insert(0, id, 0, 0));
  testrun(0 == ov_vm_prog_db_insert(store, id, 0, 0));
  testrun(0 == ov_vm_prog_db_insert(0, 0, test_instr, 0));
  testrun(0 == ov_vm_prog_db_insert(store, 0, test_instr, 0));

  /*************************************************************************
                                Legal calls
   ************************************************************************/

  ov_vm_prog *progs[NO_IDS] = {0};
  char ids[NO_IDS][OV_VM_PROG_ID_MAX_LEN] = {0};

  // Ensure that it works with uuid strings as well
  ov_id_fill_with_uuid(ids[0]);

  for (size_t i = 1; NO_IDS > i; ++i) {
    random_unique_id_at(ids, i);
  }

  progs[0] = ov_vm_prog_db_insert(store, ids[0], test_instr, 0);
  testrun(0 != progs[0]);
  testrun(OV_VM_PROG_OK == ov_vm_prog_state(progs[0]));

  progs[1] = ov_vm_prog_db_insert(store, ids[1], test_instr, 0);
  testrun(0 != progs[1]);
  testrun(OV_VM_PROG_OK == ov_vm_prog_state(progs[1]));

  /* inserting same ID twice must fail */
  progs[2] = ov_vm_prog_db_insert(store, ids[0], test_instr, 0);
  testrun(0 == progs[2]);

  char i_str[10];

  for (size_t i = 2; i < NO_IDS; ++i) {
    sprintf(i_str, "%zu", i);
    progs[i] = ov_vm_prog_db_insert(store, ids[i], test_instr, strdup(i_str));
  }

  /* We reached the maximum configured slots - further insertions should
   * fail */

  char id_too_much[OV_VM_PROG_ID_MAX_LEN];
  random_unique_id(id_too_much, ids);

  testrun(0 == ov_vm_prog_db_insert(store, id_too_much, test_instr, 0));

  testrun(0 == ov_vm_prog_db_free(store));
  testrun(3 == num_release_called);

  num_release_called = 0;

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_vm_prog_db_get() {

  testrun(!ov_vm_prog_db_get(0, 0));

  char id_string[NO_IDS][OV_VM_PROG_ID_MAX_LEN] = {0};

  // Again: at least one id should resemble a UUID
  ov_id_fill_with_uuid(id_string[0]);

  for (size_t i = 1; i < NO_IDS; ++i) {
    random_unique_id_at(id_string, i);
  }

  testrun(0 == ov_vm_prog_db_get(0, id_string[0]));

  int additional = 1;

  ov_vm_prog_db *store = ov_vm_prog_db_create(10, &additional, release);
  testrun(0 != store);

  testrun(0 == ov_vm_prog_db_get(store, id_string[0]));

  ov_vm_prog *prog[NO_IDS] = {0};

  prog[0] = ov_vm_prog_db_insert(store, id_string[0], test_instr, 0);

  testrun(0 != prog[0]);

  testrun(prog[0] == ov_vm_prog_db_get(store, id_string[0]));

  char id_unknown[OV_VM_PROG_ID_MAX_LEN];
  random_unique_id(id_unknown, id_string);

  testrun(0 == ov_vm_prog_db_get(store, id_unknown));

  testrun(prog[0] == ov_vm_prog_db_get(store, id_string[0]));

  for (size_t i = 1; i < NO_IDS; ++i) {

    /* This ID should not never be found */
    random_unique_id(id_unknown, id_string);

    testrun(0 == ov_vm_prog_db_get(store, id_unknown));

    testrun(0 == ov_vm_prog_db_get(store, id_string[i]));

    prog[i] = ov_vm_prog_db_insert(store, id_string[i], test_instr,
                                   strdup("useless"));

    testrun(0 != prog[i]);

    testrun(0 == ov_vm_prog_db_get(store, id_unknown));
    testrun(prog[i] == ov_vm_prog_db_get(store, id_string[i]));
  }

  for (size_t i = 0; i < NO_IDS; ++i) {

    /* Should not be found */
    random_unique_id(id_unknown, id_string);

    testrun(0 == ov_vm_prog_db_get(store, id_unknown));
    testrun(prog[i] == ov_vm_prog_db_get(store, id_string[i]));
  }

  testrun(0 == ov_vm_prog_db_free(store));
  testrun(NO_IDS - 1 == num_release_called);
  testrun(&additional == release_additional);

  num_release_called = 0;
  release_additional = 0;

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_vm_prog_db_remove() {

  testrun(!ov_vm_prog_db_remove(0, 0));

  ov_vm_prog *prog[NO_IDS] = {0};

  char id_string[NO_IDS][OV_VM_PROG_ID_MAX_LEN] = {0};

  // At least one id should resemble a UUID
  ov_id_fill_with_uuid(id_string[0]);

  for (size_t i = 1; i < NO_IDS; ++i) {
    random_unique_id_at(id_string, i);
  }

  testrun(!ov_vm_prog_db_remove(0, id_string[0]));

  int additional = 1;

  ov_vm_prog_db *store = ov_vm_prog_db_create(10, &additional, release);
  testrun(0 != store);

  testrun(ov_vm_prog_db_remove(store, id_string[0]));

  prog[0] = ov_vm_prog_db_insert(store, id_string[0], test_instr, 0);

  testrun(0 != prog[0]);

  char id_unknown[OV_VM_PROG_ID_MAX_LEN];
  random_unique_id(id_unknown, id_string);

  testrun(ov_vm_prog_db_remove(store, id_unknown));
  testrun(0 != ov_vm_prog_db_get(store, id_string[0]));

  testrun(ov_vm_prog_db_remove(store, id_string[0]));
  testrun(0 == ov_vm_prog_db_get(store, id_string[0]));

  testrun(0 == num_release_called);
  testrun(0 == release_additional);

  // give actual data to ensure it is freed

  for (size_t i = 0; i < NO_IDS; ++i) {

    prog[i] =
        ov_vm_prog_db_insert(store, id_string[i], test_instr, strdup("abc"));
    testrun(0 != prog[i]);
  }

  /* Subsequently remove all entries, and keep checking that the
   * entries not yet removed are still accessible */

  size_t indices_to_remove[] = {3, 2, 4, 0, 1};

  for (size_t i = 0; i < sizeof(indices_to_remove) / sizeof(size_t); ++i) {

    size_t index = indices_to_remove[i];

    testrun(ov_vm_prog_db_remove(store, id_string[index]));
    testrun(0 == ov_vm_prog_db_get(store, id_string[index]));

    testrun(i + 1 == num_release_called);
    testrun(&additional == release_additional);
    additional = 0;

    ov_vm_prog_db_dump(stderr, store);

    /* Now check whether not removed entries are still accessible */

    for (size_t remi = 1 + i; remi < sizeof(indices_to_remove) / sizeof(size_t);
         ++remi) {

      size_t mapped_index = indices_to_remove[remi];

      testrun(0 != ov_vm_prog_db_get(store, id_string[mapped_index]));
    }
  }
  testrun(sizeof(indices_to_remove) / sizeof(indices_to_remove[0]) ==
          num_release_called);

  testrun(0 == ov_vm_prog_db_free(store));
  testrun(NO_IDS == num_release_called);

  num_release_called = 0;

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_vm_prog_db_update_time() {

  char id[] = "Plautze";

  testrun(!ov_vm_prog_db_update_time(0, 0));
  testrun(!ov_vm_prog_db_update_time(0, id));
  testrun(!ov_vm_prog_db_update_time(0, 0));
  testrun(!ov_vm_prog_db_update_time(0, id));

  ov_vm_prog_db *store = ov_vm_prog_db_create(10, 0, 0);

  testrun(!ov_vm_prog_db_update_time(store, 0));
  testrun(!ov_vm_prog_db_update_time(store, id));
  testrun(!ov_vm_prog_db_update_time(store, 0));

  /* insert entry */

  ov_vm_prog *orig = ov_vm_prog_db_insert(store, id, test_instr, 0);
  testrun(0 != orig);

  /* Try to update unknown state */

  testrun(!ov_vm_prog_db_update_time(store, "Killerplautze"));

  /* update valid entry */

  testrun(ov_vm_prog_db_update_time(store, id));

  testrun(0 == ov_vm_prog_db_free(store));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_vm_prog_db_next_due() {

  testrun(0 == ov_vm_prog_db_next_due(0, 1));

  ov_vm_prog_db *store = ov_vm_prog_db_create(10, 0, 0);
  testrun(0 != store);

  testrun(0 == ov_vm_prog_db_next_due(store, 1));

  uint64_t in_the_past_usecs = ov_time_get_current_time_usecs();

  /* insert entry */

  char id[] = "ahoj";

  ov_vm_prog *orig = ov_vm_prog_db_insert(store, id, test_instr, 0);

  testrun(0 != orig);

  /* And check whether it is returned if due */

  char const *out_string =
      ov_vm_prog_db_next_due(store, in_the_past_usecs + 300 * 1000 * 1000);

  testrun(0 == strncmp(id, out_string, 36 + 1));

  out_string = 0;

  /* in the past, request should not be ready ... */
  testrun(0 == ov_vm_prog_db_next_due(store, in_the_past_usecs));

  usleep(30000);
  out_string = ov_vm_prog_db_next_due(store, in_the_past_usecs + 25000);

  testrun(0 != out_string);
  testrun(0 == strncmp(id, out_string, sizeof(id)));

  /* With a new timestamp ,the entry should not be ready yet */

  testrun(ov_vm_prog_db_update_time(store, id));

  testrun(0 == ov_vm_prog_db_next_due(store, in_the_past_usecs + 25000));

  testrun(0 == ov_vm_prog_db_free(store));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_vm_prog_db_alias() {

  testrun(!ov_vm_prog_db_alias(0, 0, 0));

  ov_vm_prog_db *store = ov_vm_prog_db_create(10, 0, 0);
  testrun(0 != store);

  // No program "anton"

  testrun(!ov_vm_prog_db_alias(store, 0, 0));
  testrun(!ov_vm_prog_db_alias(0, "alpha", 0));
  testrun(!ov_vm_prog_db_alias(store, "alpha", 0));
  testrun(!ov_vm_prog_db_alias(0, 0, "bravo"));
  testrun(!ov_vm_prog_db_alias(store, 0, "bravo"));
  testrun(!ov_vm_prog_db_alias(store, "alpha", "bravo"));

  // Create program "alpha"

  ov_vm_prog *prog = ov_vm_prog_db_insert(store, "alpha", test_instr, 0);
  testrun(0 != prog);

  testrun(prog == ov_vm_prog_db_get(store, "alpha"));
  testrun(0 == ov_vm_prog_db_get(store, "bravo"));
  testrun(0 == ov_vm_prog_db_get(store, "charlie"));
  testrun(0 == ov_vm_prog_db_get(store, "delta"));
  testrun(0 == ov_vm_prog_db_get(store, "echo"));
  testrun(0 == ov_vm_prog_db_get(store, "foxtrott"));
  testrun(0 == ov_vm_prog_db_get(store, "golf"));
  testrun(0 == ov_vm_prog_db_get(store, "hotel"));
  testrun(0 == ov_vm_prog_db_get(store, "india"));
  testrun(0 == ov_vm_prog_db_get(store, "julia"));
  testrun(0 == ov_vm_prog_db_get(store, "kilo"));
  testrun(0 == ov_vm_prog_db_get(store, "mike"));
  testrun(0 == ov_vm_prog_db_get(store, "november"));
  testrun(0 == ov_vm_prog_db_get(store, "oscar"));

  testrun(0 == ov_vm_prog_db_get(store, "whiskey"));
  testrun(0 == ov_vm_prog_db_get(store, "x-ray"));
  testrun(0 == ov_vm_prog_db_get(store, "yankee"));
  testrun(0 == ov_vm_prog_db_get(store, "zulu"));

  // Check alias again

  testrun(!ov_vm_prog_db_alias(store, 0, 0));
  testrun(!ov_vm_prog_db_alias(0, "alpha", 0));
  testrun(!ov_vm_prog_db_alias(store, "alpha", 0));
  testrun(!ov_vm_prog_db_alias(0, 0, "bravo"));
  testrun(!ov_vm_prog_db_alias(store, 0, "bravo"));
  testrun(ov_vm_prog_db_alias(store, "alpha", "bravo"));

  testrun(prog == ov_vm_prog_db_get(store, "alpha"));
  testrun(prog == ov_vm_prog_db_get(store, "bravo"));
  testrun(0 == ov_vm_prog_db_get(store, "charlie"));
  testrun(0 == ov_vm_prog_db_get(store, "delta"));
  testrun(0 == ov_vm_prog_db_get(store, "echo"));
  testrun(0 == ov_vm_prog_db_get(store, "foxtrott"));
  testrun(0 == ov_vm_prog_db_get(store, "golf"));
  testrun(0 == ov_vm_prog_db_get(store, "hotel"));
  testrun(0 == ov_vm_prog_db_get(store, "india"));
  testrun(0 == ov_vm_prog_db_get(store, "julia"));
  testrun(0 == ov_vm_prog_db_get(store, "kilo"));
  testrun(0 == ov_vm_prog_db_get(store, "mike"));
  testrun(0 == ov_vm_prog_db_get(store, "november"));
  testrun(0 == ov_vm_prog_db_get(store, "oscar"));

  testrun(0 == ov_vm_prog_db_get(store, "whiskey"));
  testrun(0 == ov_vm_prog_db_get(store, "x-ray"));
  testrun(0 == ov_vm_prog_db_get(store, "yankee"));
  testrun(0 == ov_vm_prog_db_get(store, "zulu"));

  // Performing the same alias again should succeed, but do nothing

  testrun(ov_vm_prog_db_alias(store, "alpha", "alpha"));
  testrun(ov_vm_prog_db_alias(store, "alpha", "bravo"));
  testrun(ov_vm_prog_db_alias(store, "bravo", "alpha"));
  testrun(ov_vm_prog_db_alias(store, "bravo", "bravo"));

  testrun(prog == ov_vm_prog_db_get(store, "alpha"));
  testrun(prog == ov_vm_prog_db_get(store, "bravo"));
  testrun(0 == ov_vm_prog_db_get(store, "charlie"));
  testrun(0 == ov_vm_prog_db_get(store, "delta"));
  testrun(0 == ov_vm_prog_db_get(store, "echo"));
  testrun(0 == ov_vm_prog_db_get(store, "foxtrott"));
  testrun(0 == ov_vm_prog_db_get(store, "golf"));
  testrun(0 == ov_vm_prog_db_get(store, "hotel"));
  testrun(0 == ov_vm_prog_db_get(store, "india"));
  testrun(0 == ov_vm_prog_db_get(store, "julia"));
  testrun(0 == ov_vm_prog_db_get(store, "kilo"));
  testrun(0 == ov_vm_prog_db_get(store, "mike"));
  testrun(0 == ov_vm_prog_db_get(store, "november"));
  testrun(0 == ov_vm_prog_db_get(store, "oscar"));

  testrun(0 == ov_vm_prog_db_get(store, "whiskey"));
  testrun(0 == ov_vm_prog_db_get(store, "x-ray"));
  testrun(0 == ov_vm_prog_db_get(store, "yankee"));
  testrun(0 == ov_vm_prog_db_get(store, "zulu"));

  // Add a few more aliases
  testrun(ov_vm_prog_db_alias(store, "bravo", "charlie"));
  testrun(ov_vm_prog_db_alias(store, "bravo", "delta"));
  testrun(ov_vm_prog_db_alias(store, "bravo", "echo"));
  testrun(ov_vm_prog_db_alias(store, "bravo", "foxtrott"));
  testrun(ov_vm_prog_db_alias(store, "bravo", "golf"));
  testrun(ov_vm_prog_db_alias(store, "bravo", "hotel"));
  testrun(ov_vm_prog_db_alias(store, "bravo", "india"));
  testrun(ov_vm_prog_db_alias(store, "bravo", "julia"));
  testrun(ov_vm_prog_db_alias(store, "bravo", "kilo"));
  testrun(ov_vm_prog_db_alias(store, "india", "mike"));
  testrun(ov_vm_prog_db_alias(store, "bravo", "november"));

  testrun(prog == ov_vm_prog_db_get(store, "alpha"));
  testrun(prog == ov_vm_prog_db_get(store, "bravo"));
  testrun(prog == ov_vm_prog_db_get(store, "charlie"));
  testrun(prog == ov_vm_prog_db_get(store, "delta"));
  testrun(prog == ov_vm_prog_db_get(store, "echo"));
  testrun(prog == ov_vm_prog_db_get(store, "foxtrott"));
  testrun(prog == ov_vm_prog_db_get(store, "golf"));
  testrun(prog == ov_vm_prog_db_get(store, "hotel"));
  testrun(prog == ov_vm_prog_db_get(store, "india"));
  testrun(prog == ov_vm_prog_db_get(store, "julia"));
  testrun(prog == ov_vm_prog_db_get(store, "kilo"));
  testrun(prog == ov_vm_prog_db_get(store, "mike"));
  testrun(prog == ov_vm_prog_db_get(store, "november"));
  testrun(0 == ov_vm_prog_db_get(store, "oscar"));

  testrun(0 == ov_vm_prog_db_get(store, "whiskey"));
  testrun(0 == ov_vm_prog_db_get(store, "x-ray"));
  testrun(0 == ov_vm_prog_db_get(store, "yankee"));
  testrun(0 == ov_vm_prog_db_get(store, "zulu"));

  // Add second program with id already in use should fail
  testrun(prog == ov_vm_prog_db_get(store, "alpha"));
  testrun(0 == ov_vm_prog_db_insert(store, "alpha", test_instr, 0));
  testrun(prog == ov_vm_prog_db_get(store, "alpha"));
  testrun(0 == ov_vm_prog_db_insert(store, "bravo", test_instr, 0));
  testrun(prog == ov_vm_prog_db_get(store, "alpha"));

  ov_vm_prog *prog_zulu = ov_vm_prog_db_insert(store, "zulu", test_instr, 0);
  testrun(0 != prog_zulu);

  testrun(prog == ov_vm_prog_db_get(store, "alpha"));

  // Aliasing with an alias already in used should fail
  testrun(!ov_vm_prog_db_alias(store, "zulu", "bravo"));
  testrun(prog == ov_vm_prog_db_get(store, "alpha"));
  testrun(!ov_vm_prog_db_alias(store, "zulu", "alpha"));
  testrun(prog == ov_vm_prog_db_get(store, "alpha"));

  testrun(prog == ov_vm_prog_db_get(store, "alpha"));
  testrun(prog == ov_vm_prog_db_get(store, "bravo"));
  testrun(prog == ov_vm_prog_db_get(store, "charlie"));
  testrun(prog == ov_vm_prog_db_get(store, "delta"));
  testrun(prog == ov_vm_prog_db_get(store, "echo"));
  testrun(prog == ov_vm_prog_db_get(store, "foxtrott"));
  testrun(prog == ov_vm_prog_db_get(store, "golf"));
  testrun(prog == ov_vm_prog_db_get(store, "hotel"));
  testrun(prog == ov_vm_prog_db_get(store, "india"));
  testrun(prog == ov_vm_prog_db_get(store, "julia"));
  testrun(prog == ov_vm_prog_db_get(store, "kilo"));
  testrun(prog == ov_vm_prog_db_get(store, "mike"));
  testrun(prog == ov_vm_prog_db_get(store, "november"));
  testrun(0 == ov_vm_prog_db_get(store, "oscar"));

  testrun(0 == ov_vm_prog_db_get(store, "whiskey"));
  testrun(0 == ov_vm_prog_db_get(store, "x-ray"));
  testrun(0 == ov_vm_prog_db_get(store, "yankee"));
  testrun(prog_zulu == ov_vm_prog_db_get(store, "zulu"));

  testrun(ov_vm_prog_db_alias(store, "zulu", "yankee"));

  testrun(prog == ov_vm_prog_db_get(store, "alpha"));
  testrun(prog == ov_vm_prog_db_get(store, "bravo"));
  testrun(prog == ov_vm_prog_db_get(store, "charlie"));
  testrun(prog == ov_vm_prog_db_get(store, "delta"));
  testrun(prog == ov_vm_prog_db_get(store, "echo"));
  testrun(prog == ov_vm_prog_db_get(store, "foxtrott"));
  testrun(prog == ov_vm_prog_db_get(store, "golf"));
  testrun(prog == ov_vm_prog_db_get(store, "hotel"));
  testrun(prog == ov_vm_prog_db_get(store, "india"));
  testrun(prog == ov_vm_prog_db_get(store, "julia"));
  testrun(prog == ov_vm_prog_db_get(store, "kilo"));
  testrun(prog == ov_vm_prog_db_get(store, "mike"));
  testrun(prog == ov_vm_prog_db_get(store, "november"));
  testrun(0 == ov_vm_prog_db_get(store, "oscar"));

  testrun(0 == ov_vm_prog_db_get(store, "whiskey"));
  testrun(0 == ov_vm_prog_db_get(store, "x-ray"));
  testrun(prog_zulu == ov_vm_prog_db_get(store, "yankee"));
  testrun(prog_zulu == ov_vm_prog_db_get(store, "zulu"));

  // Add one more progam that will be kept in DB until DB is freed
  // to ensure freeing DB also clears remaining aliases
  ov_vm_prog *prog_haegar =
      ov_vm_prog_db_insert(store, "haegar", test_instr, 0);
  testrun(0 != prog_haegar);

  testrun(ov_vm_prog_db_alias(store, "haegar", "gunnar"));
  testrun(ov_vm_prog_db_alias(store, "gunnar", "einar"));
  testrun(prog_haegar == ov_vm_prog_db_get(store, "haegar"));
  testrun(prog_haegar == ov_vm_prog_db_get(store, "gunnar"));
  testrun(prog_haegar == ov_vm_prog_db_get(store, "einar"));

  // Remove a program with aliases

  testrun(ov_vm_prog_db_remove(store, "alpha"));

  testrun(0 == ov_vm_prog_db_get(store, "alpha"));
  testrun(0 == ov_vm_prog_db_get(store, "bravo"));
  testrun(0 == ov_vm_prog_db_get(store, "charlie"));
  testrun(0 == ov_vm_prog_db_get(store, "delta"));
  testrun(0 == ov_vm_prog_db_get(store, "echo"));
  testrun(0 == ov_vm_prog_db_get(store, "foxtrott"));
  testrun(0 == ov_vm_prog_db_get(store, "golf"));
  testrun(0 == ov_vm_prog_db_get(store, "hotel"));
  testrun(0 == ov_vm_prog_db_get(store, "india"));
  testrun(0 == ov_vm_prog_db_get(store, "julia"));
  testrun(0 == ov_vm_prog_db_get(store, "kilo"));
  testrun(0 == ov_vm_prog_db_get(store, "mike"));
  testrun(0 == ov_vm_prog_db_get(store, "november"));
  testrun(0 == ov_vm_prog_db_get(store, "oscar"));

  testrun(0 == ov_vm_prog_db_get(store, "whiskey"));
  testrun(0 == ov_vm_prog_db_get(store, "x-ray"));
  testrun(prog_zulu == ov_vm_prog_db_get(store, "yankee"));
  testrun(prog_zulu == ov_vm_prog_db_get(store, "zulu"));

  // Remove program by alias

  testrun(ov_vm_prog_db_remove(store, "yankee"));

  testrun(0 == ov_vm_prog_db_get(store, "alpha"));
  testrun(0 == ov_vm_prog_db_get(store, "bravo"));
  testrun(0 == ov_vm_prog_db_get(store, "charlie"));
  testrun(0 == ov_vm_prog_db_get(store, "delta"));
  testrun(0 == ov_vm_prog_db_get(store, "echo"));
  testrun(0 == ov_vm_prog_db_get(store, "foxtrott"));
  testrun(0 == ov_vm_prog_db_get(store, "golf"));
  testrun(0 == ov_vm_prog_db_get(store, "hotel"));
  testrun(0 == ov_vm_prog_db_get(store, "india"));
  testrun(0 == ov_vm_prog_db_get(store, "julia"));
  testrun(0 == ov_vm_prog_db_get(store, "kilo"));
  testrun(0 == ov_vm_prog_db_get(store, "mike"));
  testrun(0 == ov_vm_prog_db_get(store, "november"));
  testrun(0 == ov_vm_prog_db_get(store, "oscar"));

  testrun(0 == ov_vm_prog_db_get(store, "whiskey"));
  testrun(0 == ov_vm_prog_db_get(store, "x-ray"));
  testrun(0 == ov_vm_prog_db_get(store, "yankee"));
  testrun(0 == ov_vm_prog_db_get(store, "zulu"));

  testrun(0 == ov_vm_prog_db_free(store));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_vm_prog_db", test_ov_vm_prog_db_create, test_ov_vm_prog_db_free,
            test_ov_vm_prog_db_insert, test_ov_vm_prog_db_get,
            test_ov_vm_prog_db_remove, test_ov_vm_prog_db_update_time,
            test_ov_vm_prog_db_next_due, test_ov_vm_prog_db_alias);

/*----------------------------------------------------------------------------*/
