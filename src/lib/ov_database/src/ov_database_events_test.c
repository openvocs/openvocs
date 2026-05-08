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
#include "ov_database_events_tester.h"

#include <ov_base/ov_id.h>
#include <ov_base/ov_json_pointer.h>
#include <ov_base/ov_plugin_system.h>
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

static ov_database_info database_info_sqlite(char const *fname) {
    return (ov_database_info){

        .type = OV_DB_SQLITE,
        .dbname = OV_OR_DEFAULT(fname, OV_DB_SQLITE_MEMORY),

    };
}

static ov_database_info get_database_info_sqlite() {
    return database_info_sqlite(0);
}

/*----------------------------------------------------------------------------*/

static int test_init(void) {
    set_database_info_getter(get_database_info_sqlite);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_database_events", test_init, test_ov_db_prepare,
            test_ov_db_events_add_participation_state_unoptimized,
            test_ov_db_events_add_participation_state,
            test_ov_db_events_get_partitipation_state,
            test_ov_db_recordings_add, test_ov_db_recordings_remove,
            test_ov_db_recordings_get);

/*----------------------------------------------------------------------------*/
