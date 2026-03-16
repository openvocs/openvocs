/***
        ------------------------------------------------------------------------

        Copyright (c) 2026 German Aerospace Center DLR e.V. (GSOC)

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

#include "ov_database.h"

void set_database_info_getter(ov_database_info (*info_getter)(void));
void set_database_wait_for_reconnect_secs(size_t secs);

int test_ov_db_prepare();
int test_ov_db_events_add_participation_state_unoptimized();
int test_ov_db_events_add_participation_state();
int test_ov_db_events_get_partitipation_state();
int test_ov_db_recordings_add();
int test_ov_db_recordings_remove();
int test_ov_db_recordings_get();
