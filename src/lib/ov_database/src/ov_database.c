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

#include "../include/ov_database.h"
#include "../sqlite3/sqlite3.h"
#include "ov_base/ov_error_codes.h"
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_hashtable.h>
#include <ov_base/ov_json_pointer.h>
#include <ov_base/ov_plugin_system.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_teardown.h>
#include <stdbool.h>
#include <wchar.h>
#include <wctype.h>

/*----------------------------------------------------------------------------*/

char const *OV_DB_SQLITE_MEMORY = ":memory:";

/*----------------------------------------------------------------------------*/

ov_database_info ov_database_info_from_json(ov_json_value const *jval) {

  ov_database_info dbi = {0};

  dbi.type = ov_json_string_get(ov_json_get(jval, "/" OV_KEY_TYPE));
  dbi.host = ov_json_string_get(ov_json_get(jval, "/" OV_KEY_HOST));
  dbi.port = ov_json_number_get(ov_json_get(jval, "/" OV_KEY_PORT));
  dbi.dbname = ov_json_string_get(ov_json_get(jval, "/" OV_KEY_DB));
  dbi.user = ov_json_string_get(ov_json_get(jval, "/" OV_KEY_USER));
  dbi.password = ov_json_string_get(ov_json_get(jval, "/" OV_KEY_PASSWORD));

  // jval = json_from_string("{\"" OV_KEY_HOST "\":\"krambambuli\","
  //         "\"" OV_KEY_PORT "\":2144, \"" OV_KEY_USER "\":\"arbol\","
  //         "\"" OV_KEY_PASSWORD "\"braga\", \"" OV_KEY_DB "\":\"db2\","
  //         "\"" OV_KEY_TYPE "\":\"baalburga\"}");

  return dbi;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_database_info_to_json(ov_database_info dbi) {

  ov_json_value *jval = ov_json_object();

  ov_json_object_set(jval, OV_KEY_TYPE, ov_json_string(dbi.type));
  ov_json_object_set(jval, OV_KEY_HOST, ov_json_string(dbi.host));
  ov_json_object_set(jval, OV_KEY_PORT, ov_json_number(dbi.port));
  ov_json_object_set(jval, OV_KEY_DB, ov_json_string(dbi.dbname));
  ov_json_object_set(jval, OV_KEY_USER, ov_json_string(dbi.user));
  ov_json_object_set(jval, OV_KEY_PASSWORD, ov_json_string(dbi.password));

  return jval;
}

/*----------------------------------------------------------------------------*/

#define MAGIC_BYTES 0x30001aba

/*----------------------------------------------------------------------------*/

static ov_database const *as_database(void const *vptr) {

  ov_database const *db = vptr;

  if (ov_ptr_valid(vptr, "Invalid Database connection - 0 pointer") &&
      ov_cond_valid(MAGIC_BYTES == db->magic_bytes,
                    "Invalid Database connection - invalid magic bytes")) {

    return db;

  } else {

    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static ov_database *as_database_mut(void *vptr) {

  return (ov_database *)as_database(vptr);
}

/*****************************************************************************
                                     SQLite
 ****************************************************************************/

typedef struct {

  ov_database public;
  sqlite3 *sqlite;

} db_sqlite;

static db_sqlite const *as_sqlite(ov_database const *self) {
  return (db_sqlite const *)self;
}

static db_sqlite *as_sqlite_mut(ov_database *self) {
  return (db_sqlite *)as_sqlite(self);
}

/*----------------------------------------------------------------------------*/

static bool sqlite_close(ov_database *self) {

  db_sqlite *sql = as_sqlite_mut(as_database_mut(self));

  if (ov_ptr_valid(sql, "Cannot close database: No database")) {

    sqlite3_close(sql->sqlite);
    return true;
  }

  return false;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *sqlite_row_to_json(size_t num_cols, char **cols,
                                         char **col_names) {

  if ((0 == num_cols) || (0 == cols) || (0 == col_names)) {

    fprintf(stderr, "Cannot add row: %zu cols: %p col_names  %p\n", num_cols,
            cols, col_names);

    return 0;

  } else {

    ov_json_value *jrow = ov_json_object();

    for (size_t i = 0; i < num_cols; ++i) {

      if (0 != col_names[i]) {

        ov_json_object_set(jrow, col_names[i],
                           ov_json_string(OV_OR_DEFAULT(cols[i], "")));
      }
    }

    return jrow;
  }
}

/*----------------------------------------------------------------------------*/

static int sqlite_exec_callback(void *jtarget, int num_cols, char **cols,
                                char **col_names) {

  ov_json_value *jval = jtarget;

  if ((0 != jval) &&
      ov_cond_valid(num_cols > 0, "Querying database failed: number of "
                                  "result columns is negative")) {

    fprintf(stderr, "Pushing another row...\n");

    ov_json_array_push(jtarget, sqlite_row_to_json(num_cols, cols, col_names));
  }

  return 0;
}

/*----------------------------------------------------------------------------*/

static ov_result sqlite_query(ov_database *self, char const *sql,
                              ov_json_value **jtarget) {

  ov_result res = {
      .error_code = OV_ERROR_NOERROR,
  };

  ov_json_value *jval = 0;

  if (0 != jtarget) {
    jval = ov_json_array();
  }

  db_sqlite *sdb = as_sqlite_mut(self);

  if (ov_ptr_valid(sdb, "Cannot query database - no database") &&
      ov_ptr_valid(sdb->sqlite, "Cannot query database - database not fully "
                                "initialized")) {

    char *errormsg = 0;

    if (SQLITE_OK !=
        sqlite3_exec(sdb->sqlite, sql, sqlite_exec_callback, jval, &errormsg)) {

      ov_result_set(&res, OV_ERROR_INTERNAL_SERVER, errormsg);
      sqlite3_free(errormsg);

    } else if (0 != jtarget) {

      *jtarget = jval;
      jval = 0;
    }

    ov_json_value_free(jval);

  } else {

    ov_result_set(&res, OV_ERROR_CODE_NOT_IMPLEMENTED,
                  "Cannot query database - no database");
  }

  return res;
}

/*---------------------------------------------------------------------------*/

bool sqlite_add_limit_clause(char *target, size_t target_capacity_octets,
                             char const *select_statement, uint32_t limit,
                             uint32_t offset) {

  UNUSED(offset);

  OV_ASSERT(0 != target);
  OV_ASSERT(0 != select_statement);

  int res = snprintf(target, target_capacity_octets, "%s LIMIT %" PRIu32 ";",
                     select_statement, limit);

  return (res > 0) && (res < (int64_t)target_capacity_octets);
}

/*----------------------------------------------------------------------------*/

static ov_database *new_database_sqlite(ov_database_info info) {

  db_sqlite *db = 0;

  sqlite3 *sdb = 0;

  info.dbname = OV_OR_DEFAULT(info.dbname, OV_DB_SQLITE_MEMORY);

  int res = sqlite3_open(info.dbname, &sdb);

  if (res == SQLITE_OK) {

    db = calloc(1, sizeof(db_sqlite));

    db->public.magic_bytes = MAGIC_BYTES;
    db->public.query = sqlite_query;
    db->public.close = sqlite_close;
    db->public.add_limit_clause = sqlite_add_limit_clause;
    db->sqlite = sdb;
    return &db->public;

  } else {

    ov_log_error("Could not connect to SQLite3 database: %s",
                 sqlite3_errmsg(sdb));
    sqlite3_close(sdb);
    sdb = 0;
    return 0;
  }
}

/*****************************************************************************
                                    GLOBALS
 ****************************************************************************/

static ov_hashtable *database_providers = 0;
static ov_database *g_database = 0;
size_t g_database_users = 0;

/*----------------------------------------------------------------------------*/

static ov_database *database_close(ov_database *self);

static void teardown_database(void) {

  database_providers = ov_hashtable_free(database_providers);
  g_database = database_close(g_database);
  g_database_users = 0;
}

/*****************************************************************************
                                     PUBLIC
 ****************************************************************************/

static ov_database_connector connector_for(char const *dbtype) {

  if (ov_string_equal(dbtype, OV_DB_SQLITE)) {

    return new_database_sqlite;

  } else {

    return ov_hashtable_get(database_providers, dbtype);
  }
}

/*----------------------------------------------------------------------------*/

bool ov_database_export_symbols_for_plugins() {

  return ov_plugin_system_export_symbol_for_plugin(
      "ov_database_connector_register_1", ov_database_connector_register);
}

/*----------------------------------------------------------------------------*/

bool ov_database_connector_register(char const *dbtype,
                                    ov_database_connector connector) {

  if (ov_ptr_valid(dbtype,
                   "Cannot register database provider: No type given") &&
      ov_ptr_valid(connector,
                   "Cannot register database provider: No db connector")) {

    if (0 == database_providers) {

      ov_teardown_register(teardown_database, "ov_database");
      database_providers = ov_hashtable_create_c_string(10);
    }

    if (0 != ov_hashtable_set(database_providers, dbtype, connector)) {

      ov_log_warning("Overwriting database provider for %s", dbtype);
    }

    ov_log_info("Registered Database connector for %s", dbtype);
    return true;

  } else {

    return false;
  }
}

/*----------------------------------------------------------------------------*/

static ov_database *polish_db(ov_database *db) {

  if (0 != db) {

    db->magic_bytes = MAGIC_BYTES;
  }

  return db;
}

/*----------------------------------------------------------------------------*/

ov_database *ov_database_connect(ov_database_info info) {

  ov_database_connector connector = connector_for(info.type);

  if (ov_ptr_valid(connector, "Unsupported database type")) {

    return polish_db(connector(info));

  } else {

    return 0;
  }
}

/*----------------------------------------------------------------------------*/

ov_database *ov_database_connect_singleton(ov_database_info info) {

  if (0 == g_database) {
    ov_teardown_register(teardown_database, "ov_database");
    g_database = ov_database_connect(info);
    g_database_users = 0;
  }

  ++g_database_users;

  return g_database;
}

/*----------------------------------------------------------------------------*/

static ov_database *database_close(ov_database *self) {

  if (ov_ptr_valid(self, "Cannot close database - no database") &&
      ov_ptr_valid(self->close, "Cannot close database - no close method")) {

    self->close(self);
    return ov_free(self);

  } else {

    return self;
  }
}

/*----------------------------------------------------------------------------*/

ov_database *ov_database_close(ov_database *self) {

  if (self != g_database) {

    self = database_close(self);

  } else {

    --g_database_users;

    if (0 == g_database_users) {

      g_database = database_close(g_database);
      self = g_database;

    } else {

      self = 0;
    }
  }

  return self;
}

/*----------------------------------------------------------------------------*/

ov_result ov_database_query(ov_database *self, char const *sql,
                            ov_json_value **jtarget) {

  if (ov_ptr_valid(self, "Cannot query database - no database") &&
      ov_ptr_valid(self->query, "Cannot query database - no query method")) {

    return self->query(self, sql, jtarget);

  } else {

    ov_result res = {0};
    ov_result_set(&res, OV_ERROR_CODE_NOT_IMPLEMENTED,
                  "Cannot query database - No DB / query method");
    return res;
  }
}

/*----------------------------------------------------------------------------*/

bool ov_database_add_limit_clause(ov_database *self, char *target,
                                  size_t target_capacity_octets,
                                  char const *select_statement, uint32_t limit,
                                  uint32_t offset) {

  return ov_cond_valid(0 == offset, "Offset not supported currently") &&
         ov_ptr_valid(self, "Cannot add limit clause to select statement - no "
                            "database") &&
         ov_ptr_valid(self->add_limit_clause,
                      "Cannot add limit clause to select statement - no such "
                      "method") &&
         ov_ptr_valid(target,
                      "Cannot add limit clause to select statement - no "
                      "target "
                      "buffer") &&
         ov_ptr_valid(select_statement,
                      "Cannot add limit clause to select statement - no "
                      "statement") &&

         self->add_limit_clause(target, target_capacity_octets,
                                select_statement, limit, offset);
}
