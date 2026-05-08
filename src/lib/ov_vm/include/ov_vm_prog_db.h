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
        @author         Michael J. Beer
        @date           2022-06-01

        Database of programs that are currently executed.

        Each program is identified by an ID - a string of up to
        OV_VM_PROG_ID_MAX_LEN chars.

        This ID might be used to associate a program to e.g. a signaling
        request by setting the ID of the program to equal the UUID of the
        event message.

        When a response arrives, its UUID equals the UUID of the event message.
        Hence, by looking up its UUID here, one can find the program assoc.
        with the signalling request.

        The ID might change during program execution;

        Fancy this:

        A program involves generating

        1. An ov signaling event (identified by a UUID) and wait for it.
        2. Upon success response, generate a SIP message (identified by the
           Call-ID)  and wait for it.

        For finding the program assoc. with the OV event, the program must be
        identified by the event's UUID.

        For finding the program assoc. with the SIP message, the program must be
        identified by the Call-ID.

        We cannot force the UUID and Call-ID to be identical, because the first
        is 16 byte hex string, the other is a URI-like string.

        Hence, we must allow for relabelling a program:

        Every identifier can be turned into a string of arbitrary length.

        We allow for changing the ID string of a program.

        Problem solved.

        ------------------------------------------------------------------------
*/
#ifndef OV_PROG_DB_H
#define OV_PROG_DB_H

/*----------------------------------------------------------------------------*/

#include "ov_vm_prog.h"
#include <ov_base/ov_json_value.h>

/*----------------------------------------------------------------------------*/

struct ov_vm_prog_db_struct;
typedef struct ov_vm_prog_db_struct ov_vm_prog_db;

/******************************************************************************
 *                                 FUNCTIONS
 ******************************************************************************/

ov_vm_prog_db *ov_vm_prog_db_create(size_t buckets, void *additional,
                                    void (*release_data)(void *, void *));

/*----------------------------------------------------------------------------*/

ov_vm_prog_db *ov_vm_prog_db_free(ov_vm_prog_db *self);

/*----------------------------------------------------------------------------*/

/**
 * Creates a new request_state object.
 * Rejects invalid request_states like one missing steps.
 *
 * @param original_message might be omitted. If there, will be copied!
 */
ov_vm_prog *ov_vm_prog_db_insert(ov_vm_prog_db *self, char const *id,
                                 ov_vm_instr const *instructions, void *data);

/*----------------------------------------------------------------------------*/

/**
 * Set an alias for a program.
 * This alias is kind of a second id.
 * The program can then be identified by either it's original id
 * or one of it's aliases.
 * This holds FOR ALL ov_vm_prog_* functions that require a prog id.
 * You get the same program if you call `ov_vm_prog_db_get` with the id
 * of the program or one of it's aliases e.g.
 */
bool ov_vm_prog_db_alias(ov_vm_prog_db *self, char const *id,
                         char const *alias);

/*----------------------------------------------------------------------------*/

ov_vm_prog *ov_vm_prog_db_get(ov_vm_prog_db *self, char const *id);

/*----------------------------------------------------------------------------*/

/**
 * Removes a prog from the db.
 * @return true if there is no prog assoc. with id after call any more
 *
 * BEWARE: This function returns true if the db does not contain the program
 * identified by id on return, REGARDLESS whether it was removed or not
 * contained in the store in the first place!
 */
bool ov_vm_prog_db_remove(ov_vm_prog_db *self, char const *id);

/*----------------------------------------------------------------------------*/

/**
 * Set timestamp to current time.
 * @return true if entry could be found and time updated. False otherwise.
 */
bool ov_vm_prog_db_update_time(ov_vm_prog_db *self, char const *id);

/*----------------------------------------------------------------------------*/

/**
 * Searches for a program that was started BEFORE time_limit_epoch_usecs
 * @return UUID string of found request or 0 if something went wrong/none found
 */
char const *ov_vm_prog_db_next_due(ov_vm_prog_db *self,
                                   uint64_t time_limit_epoch_usecs);

/*----------------------------------------------------------------------------*/

/**
 * Debug function:
 * Dumps state / content of a db to out
 */
void ov_vm_prog_db_dump(FILE *out, ov_vm_prog_db *self);

/*----------------------------------------------------------------------------*/

/**
 * Iterate over all programs in the DB and call `process` on each
 * of them.
 * The iteration is stopped and the function returns when either there are
 * no more programs or `process` returns false (in both cases this function
 * returns with true).
 * @return false if this function was called the wrong way (e.g. with 0 for
 * self)
 */
bool ov_vm_prog_db_for_each(ov_vm_prog_db const *self,
                            bool (*process)(char const *id,
                                            ov_vm_prog const *prog,
                                            uint64_t start_time_epoch_usecs,
                                            void *data),
                            void *data);

/*----------------------------------------------------------------------------*/
#endif
