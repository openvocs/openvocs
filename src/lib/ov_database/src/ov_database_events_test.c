/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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

#include "ov_database_events.c"
#include <ov_base/ov_id.h>
#include <ov_base/ov_json_pointer.h>
#include <ov_base/ov_plugin_system.h>
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

#ifdef OV_TEST_USE_POSTGRES

static ov_database_info database_info(char const *dbname) {

  return (ov_database_info){

      .type = OV_DB_POSTGRES,
      .host = "127.0.0.1",
      .port = 5432,
      .dbname = OV_OR_DEFAULT(dbname, "test123"),
      .user = "openvocs",
      .password = "secret",

  };
}

#else

static ov_database_info database_info(char const *fname) {

  return (ov_database_info){

      .type = OV_DB_SQLITE,
      .dbname = OV_OR_DEFAULT(fname, OV_DB_SQLITE_MEMORY),

  };
}

#endif

/*----------------------------------------------------------------------------*/

static ov_database *connect_to_db() {

  return ov_database_connect(database_info(0));
}

/*----------------------------------------------------------------------------*/

static int test_ov_db_prepare() {

  testrun(!ov_db_prepare(0));

  ov_database *db = connect_to_db();
  testrun(0 != db);

  testrun(ov_db_prepare(db));

  db = ov_database_close(db);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_db_events_add_participation_state() {

  testrun(!ov_db_events_add_participation_state(
      0, 0, 0, 0, OV_PARTICIPATION_STATE_NONE, 0));

  ov_database *db = connect_to_db();
  testrun(0 != db);
  testrun(ov_db_prepare(db));

  testrun(!ov_db_events_add_participation_state(
      db, 0, 0, 0, OV_PARTICIPATION_STATE_NONE, 0));

  testrun(!ov_db_events_add_participation_state(
      0, "user1", 0, 0, OV_PARTICIPATION_STATE_NONE, 0));

  testrun(!ov_db_events_add_participation_state(
      db, "user1", 0, 0, OV_PARTICIPATION_STATE_NONE, 0));

  testrun(!ov_db_events_add_participation_state(
      0, 0, "role2", 0, OV_PARTICIPATION_STATE_NONE, 0));

  testrun(!ov_db_events_add_participation_state(
      db, 0, "role2", 0, OV_PARTICIPATION_STATE_NONE, 0));

  testrun(!ov_db_events_add_participation_state(
      0, "user1", "role2", 0, OV_PARTICIPATION_STATE_NONE, 0));

  testrun(!ov_db_events_add_participation_state(
      db, "user1", "role2", 0, OV_PARTICIPATION_STATE_NONE, 0));

  testrun(!ov_db_events_add_participation_state(
      db, 0, 0, "loop3", OV_PARTICIPATION_STATE_NONE, 0));

  testrun(!ov_db_events_add_participation_state(
      0, "user1", 0, "loop3", OV_PARTICIPATION_STATE_NONE, 0));

  testrun(!ov_db_events_add_participation_state(
      db, "user1", 0, "loop3", OV_PARTICIPATION_STATE_NONE, 0));

  testrun(!ov_db_events_add_participation_state(
      0, 0, "role2", "loop3", OV_PARTICIPATION_STATE_NONE, 0));

  testrun(!ov_db_events_add_participation_state(
      db, 0, "role2", "loop3", OV_PARTICIPATION_STATE_NONE, 0));

  testrun(!ov_db_events_add_participation_state(
      0, "user1", "role2", "loop3", OV_PARTICIPATION_STATE_NONE, 0));

  testrun(!ov_db_events_add_participation_state(
      db, "user1", "role2", "loop3", OV_PARTICIPATION_STATE_NONE, 0));

  testrun(ov_db_events_add_participation_state(db, "user1", "role2", "loop3",
                                               OV_PARTICIPATION_STATE_RECV, 0));

  testrun(ov_db_events_add_participation_state(
      db, "user1", "role2", "loop3", OV_PARTICIPATION_STATE_RECV, 13));

  db = ov_database_close(db);

  return testrun_log_success();
}

/*****************************************************************************
                      ov_db_events_get_participation_state
 ****************************************************************************/

typedef struct {
  bool found;
  char const *user;
} find_pstate_params;

static bool find_pstate(void *value, void *data) {

  find_pstate_params *params = data;
  ov_json_value *jval = value;

  if ((0 != params) && (0 != jval)) {

    char const *user = ov_json_string_get(ov_json_get(jval, "/usr"));

    params->found = (params->found || ov_string_equal(user, params->user));
    return true;

  } else {

    return false;
  }
}

/*----------------------------------------------------------------------------*/

static bool pstates_user_is_there(ov_json_value const *jval, char const *user) {

  fprintf(stderr, "Looking for pstate event for user: %s\n",
          ov_string_sanitize(user));

  find_pstate_params params = {
      .found = false,
      .user = user,
  };

  ov_json_array_for_each((ov_json_value *)jval, &params, find_pstate);
  return params.found;
}

/*----------------------------------------------------------------------------*/

static int test_ov_db_events_get_partitipation_state() {

  ov_database *db = connect_to_db();
  testrun(0 != db);
  testrun(ov_db_prepare(db));

  testrun(0 == ov_db_events_get_participation_state(0, 0, 0));
  testrun(0 ==
          ov_db_events_get_participation_state(0, 0, .user = "bauer hans"));

  char const *users[] = {"adolf", "bauer",   "conrad", "duerer", "ernst",
                         "frank", "goering", "heinz",  "ingo",   "johann"};

  size_t num_events = sizeof(users) / sizeof(users[0]);

  char const *roles[] = {"r1", "r5", "r2", "r4", "r3",
                         "r3", "r4", "r2", "r5", "r1"};

  char const *loops[] = {"l1", "l2", "l3", "l4", "l5",
                         "l1", "l2", "l3", "l4", "l5"};

  for (size_t i = 0; i < num_events; ++i) {

    testrun(ov_db_events_add_participation_state(
        db, users[i], roles[i], loops[i], OV_PARTICIPATION_STATE_RECV, i));
  }

  ov_json_value *jresult =
      ov_db_events_get_participation_state(db, 0, .loop = "l5");

  fprintf(stderr, "result is %p\n", jresult);

  ov_json_value_dump(stderr, jresult);

  testrun(!pstates_user_is_there(jresult, users[0]));
  testrun(!pstates_user_is_there(jresult, users[1]));
  testrun(!pstates_user_is_there(jresult, users[2]));
  testrun(!pstates_user_is_there(jresult, users[3]));
  testrun(pstates_user_is_there(jresult, users[4]));
  testrun(!pstates_user_is_there(jresult, users[5]));
  testrun(!pstates_user_is_there(jresult, users[6]));
  testrun(!pstates_user_is_there(jresult, users[7]));
  testrun(!pstates_user_is_there(jresult, users[8]));
  testrun(pstates_user_is_there(jresult, users[9]));

  jresult = ov_json_value_free(jresult);

  jresult = ov_db_events_get_participation_state(db, 0, 0);

  ov_json_value_dump(stderr, jresult);

  testrun(pstates_user_is_there(jresult, users[0]));
  testrun(pstates_user_is_there(jresult, users[1]));
  testrun(pstates_user_is_there(jresult, users[2]));
  testrun(pstates_user_is_there(jresult, users[3]));
  testrun(pstates_user_is_there(jresult, users[4]));
  testrun(pstates_user_is_there(jresult, users[5]));
  testrun(pstates_user_is_there(jresult, users[6]));
  testrun(pstates_user_is_there(jresult, users[7]));
  testrun(pstates_user_is_there(jresult, users[8]));
  testrun(pstates_user_is_there(jresult, users[9]));

  jresult = ov_json_value_free(jresult);

  jresult = ov_db_events_get_participation_state(db, 1, 0);
  testrun(OV_DB_RECORDINGS_RESULT_TOO_BIG == jresult);
  jresult = 0;

  jresult = ov_db_events_get_participation_state(
      db, 0, .from_epoch_secs = 3, .until_epoch_secs = 8, .role = "r3");

  ov_json_value_dump(stderr, jresult);

  testrun(!pstates_user_is_there(jresult, users[0]));
  testrun(!pstates_user_is_there(jresult, users[1]));
  testrun(!pstates_user_is_there(jresult, users[2]));
  testrun(!pstates_user_is_there(jresult, users[3]));
  testrun(pstates_user_is_there(jresult, users[4]));
  testrun(pstates_user_is_there(jresult, users[5]));
  testrun(!pstates_user_is_there(jresult, users[6]));
  testrun(!pstates_user_is_there(jresult, users[7]));
  testrun(!pstates_user_is_there(jresult, users[8]));
  testrun(!pstates_user_is_there(jresult, users[9]));

  jresult = ov_json_value_free(jresult);

  db = ov_database_close(db);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_db_recordings_add() {

  testrun(!ov_db_recordings_add(0, 0, 0, 0, 0, 0));

  ov_database *db = connect_to_db();
  testrun(0 != db);
  testrun(ov_db_prepare(db));

  testrun(!ov_db_recordings_add(db, 0, 0, 0, 0, 0));

  ov_id id = {0};
  ov_id_fill_with_uuid(id);

  testrun(!ov_db_recordings_add(0, id, 0, 0, 0, 0));

  db = ov_database_close(db);
  db = connect_to_db();
  testrun(0 != db);
  testrun(ov_db_prepare(db));

  testrun(!ov_db_recordings_add(db, id, 0, 0, 0, 0));

  db = ov_database_close(db);
  db = connect_to_db();
  testrun(0 != db);
  testrun(ov_db_prepare(db));

  testrun(!ov_db_recordings_add(0, 0, "loop1", 0, 0, 0));

  db = ov_database_close(db);
  db = connect_to_db();
  testrun(0 != db);
  testrun(ov_db_prepare(db));

  testrun(!ov_db_recordings_add(db, 0, "loop1", 0, 0, 0));

  db = ov_database_close(db);
  db = connect_to_db();
  testrun(0 != db);
  testrun(ov_db_prepare(db));

  testrun(!ov_db_recordings_add(0, id, "loop1", 0, 0, 0));

  db = ov_database_close(db);
  db = connect_to_db();
  testrun(0 != db);
  testrun(ov_db_prepare(db));

  testrun(!ov_db_recordings_add(db, id, "loop1", 0, 0, 0));

  db = ov_database_close(db);
  db = connect_to_db();
  testrun(0 != db);
  testrun(ov_db_prepare(db));

  testrun(!ov_db_recordings_add(0, 0, 0, "alba", 0, 0));

  db = ov_database_close(db);
  db = connect_to_db();
  testrun(0 != db);
  testrun(ov_db_prepare(db));

  testrun(!ov_db_recordings_add(db, 0, 0, "alba", 0, 0));

  db = ov_database_close(db);
  db = connect_to_db();
  testrun(0 != db);
  testrun(ov_db_prepare(db));

  testrun(!ov_db_recordings_add(0, id, 0, "alba", 0, 0));

  db = ov_database_close(db);
  db = connect_to_db();
  testrun(0 != db);
  testrun(ov_db_prepare(db));

  testrun(!ov_db_recordings_add(db, id, 0, "alba", 0, 0));

  db = ov_database_close(db);
  db = connect_to_db();
  testrun(0 != db);
  testrun(ov_db_prepare(db));

  testrun(!ov_db_recordings_add(0, 0, "loop1", "alba", 0, 0));

  db = ov_database_close(db);
  db = connect_to_db();
  testrun(0 != db);
  testrun(ov_db_prepare(db));

  testrun(!ov_db_recordings_add(db, 0, "loop1", "alba", 0, 0));

  db = ov_database_close(db);
  db = connect_to_db();
  testrun(0 != db);
  testrun(ov_db_prepare(db));

  testrun(!ov_db_recordings_add(0, id, "loop1", "alba", 0, 0));

  db = ov_database_close(db);
  db = connect_to_db();
  testrun(0 != db);
  testrun(ov_db_prepare(db));

  testrun(ov_db_recordings_add(db, id, "loop1", "alba", 0, 0));

  db = ov_database_close(db);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_db_recordings_remove() { return testrun_log_success(); }

/*****************************************************************************
                              ov_db_recordings_get
 ****************************************************************************/

typedef struct {
  bool found;
  char const *id;
} find_recording_params;

static bool find_recording(void *value, void *data) {

  find_recording_params *params = data;
  ov_json_value *jval = value;

  if ((0 != params) && (0 != jval)) {

    char const *id = ov_json_string_get(ov_json_get(jval, "/id"));

    params->found = (params->found || ov_string_equal(id, params->id));
    return true;

  } else {

    return false;
  }
}

/*----------------------------------------------------------------------------*/

static bool rec_is_there(ov_json_value const *jval, char const *id) {

  fprintf(stderr, "Looking for recording: %s\n", ov_string_sanitize(id));

  find_recording_params params = {
      .found = false,
      .id = id,
  };

  ov_json_array_for_each((ov_json_value *)jval, &params, find_recording);
  return params.found;
}

/*----------------------------------------------------------------------------*/

static bool rec_found_in_json(ov_json_value const *jval,
                              ssize_t const *recs_expected_indices,
                              ov_id const all_recs[]) {

  size_t all_recs_len = 0;
  for (all_recs_len = 0; ov_id_valid(all_recs[all_recs_len]); ++all_recs_len)
    ;

  size_t rei_len = 0;
  for (rei_len = 0; -1 < recs_expected_indices[rei_len]; ++rei_len)
    ;

  for (size_t i = 0; i < all_recs_len; ++i) {

    char const *rec = all_recs[i];
    intptr_t iptr = i;

    bool expected =
        ov_utils_is_in_array(recs_expected_indices, rei_len, (void *)iptr);

    if (expected && (!rec_is_there(jval, rec))) {

      ov_log_error("Expected recording %s, but is not there", rec);
      return false;

    } else if ((!expected) && rec_is_there(jval, rec)) {

      ov_log_error("Unexpected recording %s, but is there", rec);
      return false;
    }
  }

  return true;
}

/*----------------------------------------------------------------------------*/

static bool recs_found(ov_database *db, ssize_t const *recs_expected,
                       ov_id const all_recs[],
                       ov_db_recordings_get_params params) {

  ov_json_value *recs = ov_db_recordings_get_struct(db, 0, params);

  if (0 == recs) {

    return false;

  } else {

    ov_json_value_dump(stderr, recs);
    bool ok = rec_found_in_json(recs, recs_expected, all_recs);
    recs = ov_json_value_free(recs);

    return ok;
  }
}

#define expected_loops(...)                                                    \
  (ssize_t[]) { __VA_ARGS__, -1 }

#define search_params(x, ...)                                                  \
  (ov_db_recordings_get_params) { x, __VA_ARGS__ }

/*----------------------------------------------------------------------------*/

static int test_ov_db_recordings_get() {

  testrun(0 == ov_db_recordings_get(0, 0, 0));

  ov_database_export_symbols_for_plugins();

  // Use Postgres DB
  // ov_plugin_system_load_dir(OPENVOCS_ROOT "/build/plugins");
  // ov_plugin_system_load(OPENVOCS_ROOT
  //                      "/build/plugins/libov_plugin_postgres2.so",
  //                      (ov_plugin_load_flags){0});
  // ov_database *db = ov_database_connect((ov_database_info){
  //    .type = OV_DB_POSTGRES,
  //    .host = "192.168.122.171",
  //    .port = 5432,
  //    .dbname = "openvocs",
  //    .user = "openvocs",
  //    .password = "openvocs",
  //});

  // ov_database *db = ov_database_connect(database_info("/tmp/sql.sqlite"));

  ov_database *db = connect_to_db();
  testrun(0 != db);

  testrun(ov_db_prepare(db));

  // Fill in some stuff

  // The ids array should terminate with a 0
  // Hence we allocate one entry in excess, but keep it 0
  ov_id ids[12] = {0};
  size_t ids_len = sizeof(ids) / sizeof(ids[0]) - 1;

  char const *uris[] = {"anton", "berton", "certon", "david", "emil", "fabian",
                        "gerd",  "hein",   "igor",   "kurt",  "lima", 0};

  size_t A = 0;
  size_t B = 1;
  size_t C = 2;
  size_t D = 3;
  size_t E = 4;
  size_t F = 5;
  size_t G = 6;
  size_t H = 7;
  size_t I = 8;
  size_t K = 9;
  size_t L = 10;

  char const *loops[sizeof(ids) / sizeof(ids[0])] = {
      "A", "B", "C", "D", "E", "F", "G", "H", "I", "K", "L", 0};

  for (size_t i = 0; i < ids_len - 1; ++i) {
    ov_id_fill_with_uuid(ids[i]);
    ov_db_recordings_add(db, ids[i], loops[i], uris[i], i, i + 100);
  }

  // Add one recording with an greater timestamp
  // Check proper timestamp conversion
  // Here, day is bigger than 12 -> 2024-10-30 ok, 2024-30-10 false
  ov_id_fill_with_uuid(ids[L]);
  ov_db_recordings_add(db, ids[L], loops[L], uris[L], 1730157139, 1730157150);

  ov_json_value *recs = ov_db_recordings_get(db, 0, 0);
  testrun(0 != recs);

  ov_json_value_dump(stderr, recs);

  for (size_t i = 0; i < ids_len; ++i) {
    testrun(rec_is_there(recs, ids[i]));
  }

  recs = ov_json_value_free(recs);

  testrun(recs_found(db, expected_loops(B), ids, search_params(.id = ids[B])));

  testrun(recs_found(db, expected_loops(E, F, G, H, I, K, L), ids,
                     search_params(.from_epoch_secs = 3)));

  testrun(recs_found(
      db, expected_loops(G, H), ids,
      search_params(.from_epoch_secs = 5, .until_epoch_secs = 100 + 8)));

  testrun(recs_found(db, expected_loops(B), ids,
                     search_params(.loop = "B", .until_epoch_secs = 100 + 8)));

  testrun(ov_db_events_add_participation_state(
      db, "adolf", "role2", "A", OV_PARTICIPATION_STATE_RECV, 60));
  testrun(ov_db_events_add_participation_state(
      db, "bauer", "role2", "A", OV_PARTICIPATION_STATE_RECV, 90));
  testrun(ov_db_events_add_participation_state(
      db, "ernst", "role2", "A", OV_PARTICIPATION_STATE_RECV, 110));

  testrun(ov_db_events_add_participation_state(db, "ernst", "role2", "B",
                                               OV_PARTICIPATION_STATE_RECV, 0));
  testrun(ov_db_events_add_participation_state(
      db, "ernst", "role2", "B", OV_PARTICIPATION_STATE_RECV, 102));

  testrun(ov_db_events_add_participation_state(db, "bauer", "role2", "C",
                                               OV_PARTICIPATION_STATE_RECV, 1));

  testrun(ov_db_events_add_participation_state(db, "bauer", "role2", "D",
                                               OV_PARTICIPATION_STATE_RECV, 4));
  testrun(ov_db_events_add_participation_state(
      db, "conrad", "role2", "D", OV_PARTICIPATION_STATE_RECV, 104));

  testrun(ov_db_events_add_participation_state(db, "bauer", "role2", "I",
                                               OV_PARTICIPATION_STATE_RECV, 9));

  testrun(ov_db_events_add_participation_state(
      db, "ernst", "role2", "K", OV_PARTICIPATION_STATE_RECV, 103));

  testrun(ov_db_events_add_participation_state(
      db, "bauer", "role2", "L", OV_PARTICIPATION_STATE_RECV, 1730157135));

  // The database now looks like
  //
  // participation_events;
  //    usr   | role  | loop  | evstate |       evtime
  // ---------+-------+-------+---------+---------------------
  //  adolf   | role2 | A     | recv    | 1970-01-01 00:01:00
  //  bauer   | role2 | A     | recv    | 1970-01-01 00:01:30
  //  ernst   | role2 | A     | recv    | 1970-01-01 00:01:50
  //  ernst   | role2 | B     | recv    | 1970-01-01 00:00:00
  //  ernst   | role2 | B     | recv    | 1970-01-01 00:01:42
  //  bauer   | role2 | C     | recv    | 1970-01-01 00:00:01
  //  bauer   | role2 | D     | recv    | 1970-01-01 00:00:04
  //  conrad  | role2 | D     | recv    | 1970-01-01 00:01:44
  //  bauer   | role2 | I     | recv    | 1970-01-01 00:00:09
  //  ernst   | role2 | K     | recv    | 1970-01-01 00:01:43
  //  bauer   | role2 | L     | recv    | 2024-10-30 09:24:58
  //
  // recordings;
  //   uri   | loop  |      starttime      |       endtime
  // --------+-------+---------------------+---------------------
  //  anton  | A     | 1970-01-01 00:00:00 | 1970-01-01 00:01:40
  //  berton | B     | 1970-01-01 00:00:01 | 1970-01-01 00:01:41
  //  certon | C     | 1970-01-01 00:00:02 | 1970-01-01 00:01:42
  //  david  | D     | 1970-01-01 00:00:03 | 1970-01-01 00:01:43
  //  emil   | E     | 1970-01-01 00:00:04 | 1970-01-01 00:01:44
  //  fabian | F     | 1970-01-01 00:00:05 | 1970-01-01 00:01:45
  //  gerd   | G     | 1970-01-01 00:00:06 | 1970-01-01 00:01:46
  //  hein   | H     | 1970-01-01 00:00:07 | 1970-01-01 00:01:47
  //  igor   | I     | 1970-01-01 00:00:08 | 1970-01-01 00:01:48
  //  kurt   | K     | 1970-01-01 00:00:09 | 1970-01-01 00:01:49
  //  lima   | L     | 1970-01-01 00:00:09 | 2024-10-30 09:25:00
  //
  //  (Left out irrelevant entries from above)

  // testrun(recs_found(db, (ssize_t[]){A, B, C, D, E, F, G, H, I, K, -1},
  // ids, to_params(
  //         )));

  testrun(
      recs_found(db, expected_loops(K), ids,
                 search_params(.user = "ernst", .from_epoch_secs = 100 + 2)));

  testrun(recs_found(db, expected_loops(A, D, I, L), ids,
                     search_params(.user = "bauer", .from_epoch_secs = 2)));

  testrun(recs_found(db, expected_loops(A, I, L), ids,
                     search_params(.user = "bauer", .from_epoch_secs = 5)));

  testrun(recs_found(db, expected_loops(A, C, D, I, L), ids,
                     search_params(.user = "bauer")));

  testrun(recs_found(db, expected_loops(C, D), ids,
                     search_params(.user = "bauer", .until_epoch_secs = 7)));

  testrun(recs_found(db, expected_loops(D), ids,
                     search_params(.user = "bauer", .until_epoch_secs = 7,
                                   .from_epoch_secs = 2)));

  // Test limitation of number of results
  // Three matching entries in databass: C, D, I
  recs = ov_db_recordings_get_struct(
      db, 1, search_params(.user = "bauer", .until_epoch_secs = 80));

  testrun(OV_DB_RECORDINGS_RESULT_TOO_BIG == recs);

  recs = ov_db_recordings_get_struct(
      db, 2, search_params(.user = "bauer", .until_epoch_secs = 80));

  testrun(OV_DB_RECORDINGS_RESULT_TOO_BIG == recs);

  recs = ov_db_recordings_get_struct(
      db, 3, search_params(.user = "bauer", .until_epoch_secs = 80));

  ov_json_value_dump(stderr, recs);

  testrun(rec_found_in_json(recs, expected_loops(C, D, I), ids));

  recs = ov_json_value_free(recs);

  recs = ov_db_recordings_get_struct(
      db, 3, search_params(.user = "bauer", .from_epoch_secs = 1000));

  ov_json_value_dump(stderr, recs);

  testrun(rec_found_in_json(recs, expected_loops(L), ids));

  recs = ov_json_value_free(recs);

  testrun(recs_found(db, expected_loops(C, D, I), ids,
                     search_params(.user = "bauer", .until_epoch_secs = 80)));

  db = ov_database_close(db);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_database_events", test_ov_db_prepare,
            test_ov_db_events_add_participation_state,
            test_ov_db_events_get_partitipation_state,
            test_ov_db_recordings_add, test_ov_db_recordings_remove,
            test_ov_db_recordings_get);

/*----------------------------------------------------------------------------*/
