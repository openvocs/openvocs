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

#include "../include/ov_database_events.h"
#include "ov_database.h"
#include <ov_base/ov_json_array.h>
#include <ov_base/ov_string.h>

/*----------------------------------------------------------------------------*/

static bool stradd(char **target,
                   size_t *target_capacity,
                   char const *to_append) {

    size_t append_len = ov_string_len(to_append);

    if (0 == append_len) {

        return true;

    } else if (ov_ptr_valid(target, "Cannot append string: No target string") ||
               ov_ptr_valid(
                   *target, "Cannot append string: No target string") ||
               ov_ptr_valid(
                   target_capacity, "Cannot append string: No target string") ||
               ov_cond_valid(0 < *target_capacity,
                             "Cannot append string: target capacity too small "
                             "(0)") ||
               ov_ptr_valid(
                   to_append, "Cannot append string: No string to append") ||
               ov_cond_valid(append_len + 1 <= *target_capacity,
                             "Cannot append string: insufficient memory")) {

        memcpy(*target, to_append, append_len);
        (*target)[append_len] = 0;

        *target += append_len;
        *target_capacity -= append_len;

        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool set_bool(bool *var, bool value) {

    if (0 != var) {
        *var = value;
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool add_and_clause(char **target, size_t *capacity, bool add_and) {

    if (add_and) {

        return stradd(target, capacity, " AND ");

    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool query(ov_database *self,
                  char const *sql,
                  ov_json_value **jtarget,
                  char const *error_msg) {

    fprintf(stderr, "SQL: %s\n", sql);

    ov_result res = ov_database_query(self, sql, jtarget);

    if (OV_ERROR_NOERROR != res.error_code) {

        ov_log_error("%s: %s",
                     ov_string_sanitize(error_msg),
                     ov_result_get_message(res));
        ov_result_clear(&res);

        return false;

    } else {

        ov_result_clear(&res);
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool result_has_less_entries_than(ov_json_value const **res,
                                         uint32_t max_num_entries) {

    return (0 == max_num_entries) || (0 == res) || (0 == *res) ||
           (ov_json_is_array(*res) &&
            (max_num_entries > ov_json_array_count(*res)));
}

/*----------------------------------------------------------------------------*/

static bool add_limit_clause_if_required(ov_database *self,
                                         char *target,
                                         size_t target_capacity_octets,
                                         char const *statement,
                                         uint32_t max_num_results) {

    if (0 == max_num_results) {

        return (0 != target) && (0 != statement) &&
               ((int64_t)target_capacity_octets >
                snprintf(target, target_capacity_octets, "%s;", statement));

    } else {
        return ov_database_add_limit_clause(self,
                                            target,
                                            target_capacity_octets,
                                            statement,
                                            max_num_results + 1,
                                            0);
    }
}

/*----------------------------------------------------------------------------*/

typedef enum { OK, TOO_MANY_RESULTS, ERROR } QueryResult;

static QueryResult query_select(ov_database *self,
                                uint32_t max_num_results,
                                char const *sql,
                                ov_json_value **jtarget,
                                char const *error_msg) {

    char select_sql[1000] = {0};

    if (ov_ptr_valid(sql, "No SQL statement") &&
        add_limit_clause_if_required(
            self, select_sql, sizeof(select_sql), sql, max_num_results)) {

        bool ok = query(self, select_sql, jtarget, error_msg);

        if (ok) {

            if ((0 == max_num_results) ||
                result_has_less_entries_than(
                    (ov_json_value const **)jtarget, max_num_results + 1)) {

                return OK;

            } else {

                if (0 != jtarget) {
                    *jtarget = ov_json_value_free(*jtarget);
                }

                return TOO_MANY_RESULTS;
            }

        } else {

            return ERROR;
        }

    } else {

        return ERROR;
    }
}

/*----------------------------------------------------------------------------*/

#define RECORDINGS_TABLE "recordings"
#define ID_LEN 36
#define URI_LEN 300
#define LOOP_LEN 200

#define PARTICIPATION_EVENTS_TABLE "events"
#define USER_LEN 300
#define ROLE_LEN 300
#define PARTICIPATION_STATE_LEN 15

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

bool ov_db_prepare(ov_database *self) {

    return query(self,
                  "CREATE TABLE IF NOT EXISTS "
                  RECORDINGS_TABLE
                  " (id CHAR(" STR(ID_LEN)
                  "), uri VARCHAR(" STR(URI_LEN)
                  "), loop VARCHAR(" STR(LOOP_LEN)
                  "), starttime TIMESTAMP, endtime TIMESTAMP);",
                  0,
                  "Could not prepare recordings database") &&
            query(
                self,
                "CREATE TABLE IF NOT EXISTS "
                PARTICIPATION_EVENTS_TABLE 
                " (usr VARCHAR(" STR(USER_LEN)
                "), role VARCHAR(" STR(ROLE_LEN)
                "), loop VARCHAR(" STR(LOOP_LEN)
                "), evstate VARCHAR(" STR(PARTICIPATION_STATE_LEN)
                "), evtime TIMESTAMP);",
                0,
                "Could not prepare recordings database");
}

/*----------------------------------------------------------------------------*/

static char *epoch_secs_to_sql_datetime(char *target,
                                        size_t target_capacity,
                                        time_t time_epoch_secs) {

    struct tm time_tm = {0};

    if (0 != target) {

        gmtime_r(&time_epoch_secs, &time_tm);
        snprintf(target,
                 target_capacity,
                 "%04d-%02d-%02d %02d:%02d:%02d",
                 1900 + time_tm.tm_year,
                 1 + time_tm.tm_mon,
                 time_tm.tm_mday,
                 time_tm.tm_hour,
                 time_tm.tm_min,
                 time_tm.tm_sec);
    }

    return target;
}

/*----------------------------------------------------------------------------*/

bool ov_db_events_add_participation_state(ov_database *self,
                                          const char *user,
                                          const char *role,
                                          const char *loop,
                                          ov_participation_state state,
                                          time_t time_epoch) {

    size_t user_len = ov_string_len(user);
    size_t role_len = ov_string_len(role);
    size_t loop_len = ov_string_len(loop);

    char sql[500] = {0};
    char str_time[30] = {0};
    char const *str_state = ov_participation_state_to_string(state);

    if (ov_ptr_valid(user,
                     "Cannot insert participation event into database: No user "
                     "given") &&
        ov_ptr_valid(role,
                     "Cannot insert participation event into database: No role "
                     "given") &&
        ov_ptr_valid(loop,
                     "Cannot insert participation event into database: No loop "
                     "given") &&
        ov_cond_valid(user_len <= USER_LEN,
                      "Cannot insert recording into database: User string is "
                      "too long") &&
        ov_cond_valid(role_len <= ROLE_LEN,
                      "Cannot insert recording into database: Role string is "
                      "too long") &&
        ov_cond_valid(loop_len <= LOOP_LEN,
                      "Cannot insert recording into database: Role string is "
                      "too long") &&
        ov_ptr_valid(str_state,
                     "Cannot insert participation event into database: "
                     "Invalid participation state") &&
        ov_cond_valid(OV_PARTICIPATION_STATE_NONE != state,
                      "Cannot insert participation event into database: "
                      "Invalid participation state")) {

        snprintf(
            sql,
            sizeof(sql),
            "INSERT INTO " PARTICIPATION_EVENTS_TABLE
            " (usr, role, loop, evstate, evtime) "
            " VALUES ('%s', '%s', '%s', '%s', '%s');",
            user,
            role,
            loop,
            str_state,
            epoch_secs_to_sql_datetime(str_time, sizeof(str_time), time_epoch));

        return query(
            self, sql, 0, "Cannot insert participation event into database");

    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool pstate_params_to_sql_query(
    char *target,
    size_t target_capacity,
    ov_db_events_get_participation_state_params params) {

    char *write_ptr = target;

    stradd(&write_ptr,
           &target_capacity,
           "SELECT usr, loop, evstate, evtime  "
           "FROM " PARTICIPATION_EVENTS_TABLE);

    if ((0 != params.user) || (0 != params.from_epoch_secs) ||
        (0 != params.until_epoch_secs) || (0 != params.loop) ||
        (0 != params.role)) {

        stradd(&write_ptr, &target_capacity, " WHERE ");
    }

    bool condition_already_there = false;

    if ((0 != params.user) &&
        ((!add_and_clause(
             &write_ptr, &target_capacity, condition_already_there)) ||
         (!stradd(&write_ptr, &target_capacity, "usr='")) ||
         (!stradd(&write_ptr, &target_capacity, params.user)) ||
         (!stradd(&write_ptr, &target_capacity, "'")) ||
         (!set_bool(&condition_already_there, true)))) {
        return false;
    }

    if ((0 != params.role) &&
        ((!add_and_clause(
             &write_ptr, &target_capacity, condition_already_there)) ||
         (!stradd(&write_ptr, &target_capacity, "role='")) ||
         (!stradd(&write_ptr, &target_capacity, params.role)) ||
         (!stradd(&write_ptr, &target_capacity, "'")) ||
         (!set_bool(&condition_already_there, true)))) {
        return false;
    }

    if ((0 != params.loop) &&
        ((!add_and_clause(
             &write_ptr, &target_capacity, condition_already_there)) ||
         (!stradd(&write_ptr, &target_capacity, "loop='")) ||
         (!stradd(&write_ptr, &target_capacity, params.loop)) ||
         (!stradd(&write_ptr, &target_capacity, "'")) ||
         (!set_bool(&condition_already_there, true)))) {
        return false;
    }

    if ((OV_PARTICIPATION_STATE_NONE != params.state) &&
        ((!add_and_clause(
             &write_ptr, &target_capacity, condition_already_there)) ||
         (!stradd(&write_ptr, &target_capacity, "evstate='")) ||
         (!stradd(&write_ptr,
                  &target_capacity,
                  ov_participation_state_to_string(params.state))) ||
         (!stradd(&write_ptr, &target_capacity, "'")) ||
         (!set_bool(&condition_already_there, true)))) {
        return false;
    }

    char timestamp[30] = {0};

    if ((0 != params.from_epoch_secs) &&

        ((!add_and_clause(
             &write_ptr, &target_capacity, condition_already_there)) ||
         (!stradd(&write_ptr, &target_capacity, "evtime>'")) ||
         (!stradd(&write_ptr,
                  &target_capacity,
                  epoch_secs_to_sql_datetime(
                      timestamp, sizeof(timestamp), params.from_epoch_secs))) ||
         (!stradd(&write_ptr, &target_capacity, "'")) ||
         (!set_bool(&condition_already_there, true)))) {

        return false;
    }

    if ((0 != params.until_epoch_secs) &&

        ((!add_and_clause(
             &write_ptr, &target_capacity, condition_already_there)) ||
         (!stradd(&write_ptr, &target_capacity, "evtime<'")) ||
         (!stradd(
             &write_ptr,
             &target_capacity,
             epoch_secs_to_sql_datetime(
                 timestamp, sizeof(timestamp), params.until_epoch_secs))) ||
         (!stradd(&write_ptr, &target_capacity, "'")) ||
         (!set_bool(&condition_already_there, true)))) {

        return false;
    }

    return (0 != target);
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_db_events_get_participation_state_struct(
    ov_database *self,
    uint32_t max_num_results,
    ov_db_events_get_participation_state_params params) {

    char sql[1000] = {0};

    QueryResult result = ERROR;

    ov_json_value *jtarget = 0;

    if (pstate_params_to_sql_query(sql, sizeof(sql), params)) {

        result = query_select(self,
                              max_num_results,
                              sql,
                              &jtarget,
                              "Could not query database for pariticipation "
                              "states");
    }

    switch (result) {

        case TOO_MANY_RESULTS:
            jtarget = ov_json_value_free(jtarget);
            return (ov_json_value *)OV_DB_RECORDINGS_RESULT_TOO_BIG;

        case ERROR:
            jtarget = ov_json_value_free(jtarget);
            return jtarget;

        case OK:
            return jtarget;

        default:
            return jtarget;
    };

    return 0;
}

/*****************************************************************************
                                   Recordings
 ****************************************************************************/

bool ov_db_recordings_add(ov_database *self,
                          char const *id,
                          char const *loop,
                          char const *uri,
                          time_t start_epoch_secs,
                          time_t end_epoch_secs) {

    char sql[500] = {0};

    size_t id_len = ov_string_len(id);
    size_t uri_len = ov_string_len(uri);
    size_t loop_len = ov_string_len(loop);

    char tsstart[30] = {0};
    char tsend[30] = {0};

    fprintf(stderr,
            "Inserting %s %s Loop: %s from %zu until %zu\n",
            ov_string_sanitize(id),
            ov_string_sanitize(uri),
            ov_string_sanitize(loop),
            start_epoch_secs,
            end_epoch_secs);

    if (ov_ptr_valid(
            id, "Cannot insert recording into database: No ID given") &&
        ov_ptr_valid(
            uri, "Cannot insert recording into database: No URI given") &&
        ov_ptr_valid(
            loop, "Cannot insert recording into database: No Loop given") &&
        ov_cond_valid(id_len == ID_LEN,
                      "Cannot insert recording into database: ID is not a "
                      "UUID") &&
        ov_cond_valid(uri_len <= URI_LEN,
                      "Cannot insert recording into database: URI too long") &&
        ov_cond_valid(loop_len <= LOOP_LEN,
                      "Cannot insert recording into database: Loop too long")) {

        snprintf(
            sql,
            sizeof(sql),
            "INSERT INTO " RECORDINGS_TABLE
            " (id, uri, loop, starttime, endtime) "
            " VALUES ('%s', '%s', '%s', '%s', '%s');",
            id,
            uri,
            loop,
            epoch_secs_to_sql_datetime(
                tsstart, sizeof(tsstart), start_epoch_secs),
            epoch_secs_to_sql_datetime(tsend, sizeof(tsend), end_epoch_secs));

        return query(self, sql, 0, "Could not add recording to database");

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool params_to_sql_query_without_user(
    char *target,
    size_t target_capacity,
    const ov_db_recordings_get_params params) {

    char *write_ptr = target;

    stradd(&write_ptr,
           &target_capacity,
           "SELECT r.id, r.uri, r.loop, r.starttime, r.endtime "
           "FROM " RECORDINGS_TABLE " r");

    if ((0 != params.id) || (0 != params.from_epoch_secs) ||
        (0 != params.until_epoch_secs) || (0 != params.loop)) {

        stradd(&write_ptr, &target_capacity, " WHERE ");
    }

    bool condition_already_there = false;

    if ((0 != params.id) &&

        ((!add_and_clause(
             &write_ptr, &target_capacity, condition_already_there)) ||
         (!stradd(&write_ptr, &target_capacity, "id='")) ||
         (!stradd(&write_ptr, &target_capacity, params.id)) ||
         (!stradd(&write_ptr, &target_capacity, "'")) ||
         (!set_bool(&condition_already_there, true)))) {
        return false;
    }

    if ((0 != params.loop) &&

        ((!add_and_clause(
             &write_ptr, &target_capacity, condition_already_there)) ||
         (!stradd(&write_ptr, &target_capacity, "loop='")) ||
         (!stradd(&write_ptr, &target_capacity, params.loop)) ||
         (!stradd(&write_ptr, &target_capacity, "'")) ||
         (!set_bool(&condition_already_there, true)))) {
        return false;
    }

    char timestamp[30] = {0};

    if ((0 != params.from_epoch_secs) &&

        ((!add_and_clause(
             &write_ptr, &target_capacity, condition_already_there)) ||
         (!stradd(&write_ptr, &target_capacity, "starttime>'")) ||
         (!stradd(&write_ptr,
                  &target_capacity,
                  epoch_secs_to_sql_datetime(
                      timestamp, sizeof(timestamp), params.from_epoch_secs))) ||
         (!stradd(&write_ptr, &target_capacity, "'")) ||
         (!set_bool(&condition_already_there, true)))) {

        return false;
    }

    if ((0 != params.until_epoch_secs) &&

        ((!add_and_clause(
             &write_ptr, &target_capacity, condition_already_there)) ||
         (!stradd(&write_ptr, &target_capacity, "endtime<'")) ||
         (!stradd(
             &write_ptr,
             &target_capacity,
             epoch_secs_to_sql_datetime(
                 timestamp, sizeof(timestamp), params.until_epoch_secs))) ||
         (!stradd(&write_ptr, &target_capacity, "'")) ||
         (!set_bool(&condition_already_there, true)))) {

        return false;
    }

    return (0 != target);
}

/*----------------------------------------------------------------------------*/

static bool params_to_sql_query_with_user(
    char *target,
    size_t target_capacity,
    const ov_db_recordings_get_params params) {

    char timestamp[30] = {0};
    char *write_ptr = target;

    stradd(&write_ptr,
           &target_capacity,
           "SELECT r.id, r.uri, r.loop, r.starttime, r.endtime "
           "FROM " RECORDINGS_TABLE " r, " PARTICIPATION_EVENTS_TABLE
           " e WHERE e.usr='");

    stradd(&write_ptr, &target_capacity, params.user);
    stradd(&write_ptr, &target_capacity, "' AND r.loop=e.loop");

    if (((OV_PARTICIPATION_STATE_RECV == params.user_state) ||
         (OV_PARTICIPATION_STATE_SEND == params.user_state)) &&
        ((!add_and_clause(&write_ptr, &target_capacity, true)) ||
         (!stradd(&write_ptr, &target_capacity, "e.evstate='")) ||
         (!stradd(&write_ptr,
                  &target_capacity,
                  ov_participation_state_to_string(params.user_state))) ||
         (!stradd(&write_ptr, &target_capacity, "'")))) {

        return false;

    } else if ((!add_and_clause(&write_ptr, &target_capacity, true)) ||
               (!stradd(&write_ptr, &target_capacity, "(")) ||
               (!stradd(&write_ptr, &target_capacity, "e.evstate='")) ||
               (!stradd(&write_ptr,
                        &target_capacity,
                        ov_participation_state_to_string(
                            OV_PARTICIPATION_STATE_SEND))) ||

               (!stradd(&write_ptr, &target_capacity, "' OR ")) ||
               (!stradd(&write_ptr, &target_capacity, "e.evstate='")) ||
               (!stradd(&write_ptr,
                        &target_capacity,
                        ov_participation_state_to_string(
                            OV_PARTICIPATION_STATE_RECV))) ||
               (!stradd(&write_ptr, &target_capacity, "')"))) {

        return false;
    }

    if ((0 != params.id) &&
        ((!add_and_clause(&write_ptr, &target_capacity, true)) ||
         (!stradd(&write_ptr, &target_capacity, "r.id='")) ||
         (!stradd(&write_ptr, &target_capacity, params.id)) ||
         (!stradd(&write_ptr, &target_capacity, "'")))) {

        return false;
    }

    if ((0 != params.loop) &&
        ((!add_and_clause(&write_ptr, &target_capacity, true)) ||
         (!stradd(&write_ptr, &target_capacity, "e.loop='")) ||
         (!stradd(&write_ptr, &target_capacity, params.loop)) ||
         (!stradd(&write_ptr, &target_capacity, "'")))) {

        return false;
    }

    if ((0 != params.until_epoch_secs) &&

        ((!add_and_clause(&write_ptr, &target_capacity, true)) ||
         (!stradd(&write_ptr, &target_capacity, "e.evtime<'")) ||
         (!stradd(
             &write_ptr,
             &target_capacity,
             epoch_secs_to_sql_datetime(
                 timestamp, sizeof(timestamp), params.until_epoch_secs))) ||
         (!stradd(&write_ptr, &target_capacity, "' AND '")) ||
         (!stradd(
             &write_ptr,
             &target_capacity,
             epoch_secs_to_sql_datetime(
                 timestamp, sizeof(timestamp), params.until_epoch_secs))) ||
         (!stradd(&write_ptr, &target_capacity, "'>r.starttime")))) {

        return false;
    }

    if ((0 != params.from_epoch_secs) &&

        ((!add_and_clause(&write_ptr, &target_capacity, true)) ||
         (!stradd(&write_ptr, &target_capacity, "e.evtime>'")) ||
         (!stradd(&write_ptr,
                  &target_capacity,
                  epoch_secs_to_sql_datetime(
                      timestamp, sizeof(timestamp), params.from_epoch_secs))) ||
         (!stradd(&write_ptr, &target_capacity, "' AND r.endtime>'")) ||
         (!stradd(&write_ptr,
                  &target_capacity,
                  epoch_secs_to_sql_datetime(
                      timestamp, sizeof(timestamp), params.from_epoch_secs))) ||
         (!stradd(&write_ptr, &target_capacity, "'")))) {

        return false;
    }

    return (0 != target);
}

/*----------------------------------------------------------------------------*/

static bool params_to_sql_query(char *target,
                                size_t target_capacity,
                                const ov_db_recordings_get_params params) {

    if (0 == params.user) {

        return params_to_sql_query_without_user(
            target, target_capacity, params);

    } else {

        return params_to_sql_query_with_user(target, target_capacity, params);
    }
}

/*----------------------------------------------------------------------------*/

ov_json_value const *OV_DB_RECORDINGS_RESULT_TOO_BIG =
    (ov_json_value const *)&OV_DB_RECORDINGS_RESULT_TOO_BIG;

ov_json_value *ov_db_recordings_get_struct(ov_database *self,
                                           uint32_t max_num_results,
                                           ov_db_recordings_get_params params) {

    char sql[1000] = {0};

    QueryResult result = ERROR;

    ov_json_value *jtarget = 0;

    if (params_to_sql_query(sql, sizeof(sql), params)) {

        result = query_select(self,
                              max_num_results,
                              sql,
                              &jtarget,
                              "Could not query database for recordings");
    }

    switch (result) {

        case TOO_MANY_RESULTS:
            jtarget = ov_json_value_free(jtarget);
            return (ov_json_value *)OV_DB_RECORDINGS_RESULT_TOO_BIG;

        case ERROR:
            jtarget = ov_json_value_free(jtarget);
            return jtarget;

        case OK:
            return jtarget;

        default:
            return jtarget;
    };
}

/*----------------------------------------------------------------------------*/
