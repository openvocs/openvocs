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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

        General to access SQL databases.

        Currently, however, some features that might not be part of the
        SQL standards are assumed:

           - The SQLDB supports the DATE type in the format YYYY-MM-DD
           - The SQLDB supports the TIME type in the format HH:MM:SS

        Thus, while writing an adaptor for a database that does not support
        those in this particular format will cause errors with the applications
        as they will generate SQL statements incompatible with the DB.

        ------------------------------------------------------------------------
*/
#ifndef OV_DATABASE_H
#define OV_DATABASE_H
/*----------------------------------------------------------------------------*/

#include "ov_base/ov_result.h"
#include <inttypes.h>
#include <ov_base/ov_json_value.h>

/*----------------------------------------------------------------------------*/

#define OV_DB_POSTGRES "postgres"
#define OV_DB_SQLITE "sqlite"

extern char const *OV_DB_SQLITE_MEMORY;

typedef struct ov_database_struct {

    // Set by us - modules shall NOT use these!
    uint32_t magic_bytes;

    bool (*close)(struct ov_database_struct *self);

    ov_result (*query)(struct ov_database_struct *self,
                       char const *sql,
                       ov_json_value **jtarget);

    bool (*add_limit_clause)(char *target,
                             size_t target_capacity_octets,
                             char const *select_statement,
                             uint32_t limit,
                             uint32_t offset);

} ov_database;

/**
 * For most databases, we are dealing with a network socket to connect to.
 * For SQLite, it is a file. Hence, host/port is unused and the file name
 * is expected within the dbname . If dbname is set to OV_DB_SQLITE_MEMORY,
 * the database will be created in memory.
 */
typedef struct {

    char const *type;
    char const *host;
    uint16_t port;
    char const *dbname;
    char const *user;
    char const *password;

    uint64_t reconnect_secs;

} ov_database_info;

ov_database_info ov_database_info_from_json(ov_json_value const *jval);
ov_json_value *ov_database_info_to_json(ov_database_info dbi);

/*----------------------------------------------------------------------------*/

typedef ov_database *(*ov_database_connector)(ov_database_info info);

bool ov_database_export_symbols_for_plugins();

bool ov_database_connector_register(char const *dbtype,
                                    ov_database_connector connector);

/*----------------------------------------------------------------------------*/

ov_database *ov_database_connect(ov_database_info info);

/**
 * Like connect, but will create one single connection and return that one
 * as long as this one is open.
 * Treat the returned object just as any other database connection, e.g.
 * close it as soon as you are done with it.
 * Internally, the singleton is closed when as many closes()
 * have been performed as the singleton was handed out.
 */
ov_database *ov_database_connect_singleton(ov_database_info info);

ov_database *ov_database_close(ov_database *self);

/*----------------------------------------------------------------------------*/

/**
 * Send SQL query to database.
 * If data is expected to be returned, hand over a JSON value `jtarget` .
 * The data will be added to this JSON value.
 */
ov_result ov_database_query(ov_database *self,
                            char const *sql,
                            ov_json_value **jtarget);

/*----------------------------------------------------------------------------*/

bool ov_database_add_limit_clause(ov_database *self,
                                  char *target,
                                  size_t target_capacity_octets,
                                  char const *select_statement,
                                  uint32_t limit,
                                  uint32_t offset);

/*----------------------------------------------------------------------------*/
#endif
