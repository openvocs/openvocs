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
        @file           ov_vm_prog_db.c
        @author         Michael J. Beer

        @date           2019-10-11

        ------------------------------------------------------------------------
*/
#include "../include/ov_vm_prog_db.h"
#include "ov_vm_prog.h"
#include <ov_base/ov_hashtable.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_result.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_time.h>

/*----------------------------------------------------------------------------*/

// TODO: Shift to ov_constants
#define DEFAULT_NUMBER_SLOTS 64

/*----------------------------------------------------------------------------*/

static bool is_id_valid(char const *id) {

  size_t len = ov_string_len(id);

  if (0 == len) {

    ov_log_warning("No ID (either 0 pointer or 0 length");
    return false;

  } else if (OV_VM_PROG_ID_MAX_LEN <= len) {

    ov_log_warning("ID too long: %*s ... (continues with '%*s') ",
                   OV_VM_PROG_ID_MAX_LEN, id, 10, id + OV_VM_PROG_ID_MAX_LEN);
    return false;

  } else {

    ov_log_debug("Valid ID: %s", id);
    return true;
  }
}

/*----------------------------------------------------------------------------*/

static bool is_db_valid(ov_vm_prog_db const *db) {

  if (0 == db) {
    ov_log_error("No Program DB");
    return false;
  } else {
    return true;
  }
}

/*----------------------------------------------------------------------------*/

static bool are_inst_valid(ov_vm_instr const *instr) {

  if (0 == instr) {
    ov_log_error("No instructions given (0 pointer)");
    return false;
  } else {
    return true;
  }
}

/*----------------------------------------------------------------------------*/

typedef struct {

  ov_vm_prog_mem public;
  uint64_t start_time_epoch_usecs;

} ProgState;

/*----------------------------------------------------------------------------*/

struct ov_vm_prog_db_struct {

  ov_hashtable *store;
  ov_hashtable *aliases;

  /* Array contains all request_states, the available AND used ones
   * In fact, the memory they use is the particular entry of this array.
   */
  struct {
    ProgState *states;
    size_t capacity;
  } pool;
  /*
   * Array of pointers to AVAILABLE request_states.
   * Capacity equals the capacity of state_pool.
   * usable index runs from 1 through capacity.
   * slot 0 is a dummy slot, and index 0 denotes 'is empty'.
   */
  struct {
    ProgState **states;
    size_t topmost_usable_slot_index;
    size_t capacity;
  } available;

  struct {
    void (*release)(void *data, void *add);
    void *additional;
  } release;
};

/*----------------------------------------------------------------------------*/

static ov_hashtable const *get_store(ov_vm_prog_db const *self) {

  if (!is_db_valid(self)) {
    return 0;
  } else {
    return self->store;
  }
}

/*----------------------------------------------------------------------------*/

static ov_hashtable *get_store_mut(ov_vm_prog_db *self) {

  if (!is_db_valid(self)) {
    return 0;
  } else {
    return self->store;
  }
}

/*----------------------------------------------------------------------------*/

static ov_hashtable *get_aliases(ov_vm_prog_db *self) {

  if (!is_db_valid(self)) {
    return 0;
  } else {
    return self->aliases;
  }
}

/*----------------------------------------------------------------------------*/

static void release_data(void *data, void *additional,
                         void (*releaser)(void *, void *)) {

  if ((0 != releaser) && (0 != data)) {
    releaser(data, additional);
  }
}

/*----------------------------------------------------------------------------*/

static void clear_state(ProgState *state, void *additional,
                        void (*releaser)(void *, void *)) {

  if (0 != state) {

    state->start_time_epoch_usecs = UINT64_MAX;

    ov_vm_prog *prog = (ov_vm_prog *)state;

    release_data(ov_vm_prog_data(prog), additional, releaser);
    ov_vm_prog_data_set(prog, 0);

    ov_vm_prog_result_set(prog, OV_ERROR_NOERROR, 0);

    ov_vm_prog_program_counter_reset(prog);
    ov_vm_prog_state_set(prog, OV_VM_PROG_INVALID);
  }
}

/*----------------------------------------------------------------------------*/

static ProgState *prog_for_id_internal(ov_hashtable *store, char const *id) {

  ProgState *state = ov_hashtable_get(store, id);

  if (0 == state) {
    ov_log_warning("Program '%s' not found", ov_string_sanitize(id));
  } else {
    ov_log_debug("Program '%s' found", ov_string_sanitize(id));
  }

  return state;
}

/*----------------------------------------------------------------------------*/

static char const *get_unaliased_id(ov_hashtable const *aliases,
                                    char const *alias) {

  char const *id = ov_hashtable_get(aliases, alias);

  if (!ov_ptr_valid(alias, "No alias given")) {
    return 0;
  } else if (0 == id) {
    return alias;
  } else {
    return id;
  }
}

/*----------------------------------------------------------------------------*/

static ProgState *prog_for_id(ov_vm_prog_db *self, char const *id) {

  ov_hashtable *store = get_store_mut(self);
  return prog_for_id_internal(store, get_unaliased_id(get_aliases(self), id));
}

/*----------------------------------------------------------------------------*/

ov_vm_prog_db *ov_vm_prog_db_create(size_t slots, void *additional,
                                    void (*release_data)(void *, void *)) {

  ov_vm_prog_db *db = calloc(1, sizeof(ov_vm_prog_db));

  db->pool.capacity = slots;

  slots = OV_OR_DEFAULT(slots, DEFAULT_NUMBER_SLOTS);

  db->store = ov_hashtable_create_c_string(slots);
  db->aliases = ov_hashtable_create_c_string(slots);

  OV_ASSERT(0 != db->store);
  OV_ASSERT(0 != db->aliases);

  db->pool.states = calloc(slots, sizeof(ProgState));
  db->pool.capacity = slots;
  db->available.states = calloc(slots + 1, sizeof(ProgState *));

  for (size_t i = 0; slots > i; ++i) {
    db->available.states[i + 1] = db->pool.states + i;
    clear_state(db->pool.states + i, 0, 0);
  }

  db->available.topmost_usable_slot_index = slots;
  db->available.capacity = slots;
  db->release.release = release_data;
  db->release.additional = additional;

  return db;
}

/*----------------------------------------------------------------------------*/

static void free_db_states_unsafe(ov_vm_prog_db *self) {

  OV_ASSERT(0 != self);

  if (0 != self->pool.states) {

    for (size_t i = 0; i < self->pool.capacity; ++i) {
      clear_state(self->pool.states + i, self->release.additional,
                  self->release.release);
    }

    free(self->pool.states);

  } else {
    ov_log_warning("States pool empty");
  }
}

/*----------------------------------------------------------------------------*/

static bool free_alias_entry(void const *key, void const *value, void *arg) {

  UNUSED(key);
  UNUSED(arg);

  ov_free((void *)value);

  return true;
}

/*----------------------------------------------------------------------------*/

ov_vm_prog_db *ov_vm_prog_db_free(ov_vm_prog_db *self) {

  if (0 != self) {

    self->store = ov_hashtable_free(self->store);
    free_db_states_unsafe(self);

    ov_hashtable_for_each(self->aliases, free_alias_entry, 0);
    self->aliases = ov_hashtable_free(self->aliases);

    self->available.states = ov_free(self->available.states);
    free(self);
  }

  return 0;
}

/*****************************************************************************
                                     INSERT
 ****************************************************************************/

static ProgState *register_prog_unsafe(ov_hashtable *store, ProgState *prog) {

  ov_vm_prog *p = (ov_vm_prog *)prog;

  ov_vm_prog_reset_prog_started(p);
  char const *id = ov_vm_prog_id(p);

  if (0 != ov_hashtable_set(store, id, prog)) {
    ov_log_error("Could not register prog for ID %s", id);
    return 0;
  } else {
    ov_log_info("Registered program %s", id);
    return prog;
  }
}

/*----------------------------------------------------------------------------*/

static size_t get_topmost_usable_slot_index(ov_vm_prog_db const *self) {

  if (!is_db_valid(self)) {
    return 0;
  } else {
    return self->available.topmost_usable_slot_index;
  }
}

/*----------------------------------------------------------------------------*/

static ProgState *fresh_prog_state_unsafe(ov_vm_prog_db *self) {

  size_t index = get_topmost_usable_slot_index(self);

  if (0 == index) {
    ov_log_warning("No more request state objects available");
    return 0;
  } else {

    ProgState *prog = self->available.states[index];

    self->available.states[index] = 0;
    --self->available.topmost_usable_slot_index;

    return prog;
  }
}

/*----------------------------------------------------------------------------*/

static ProgState *init_prog_state_unsafe(ProgState *prog_state, char const *id,
                                         ov_vm_instr const *instructions,
                                         void *data) {

  ov_vm_prog *prog = (ov_vm_prog *)prog_state;

  ov_vm_prog_id_set(prog, id);
  ov_vm_prog_state_set(prog, OV_VM_PROG_OK);
  ov_vm_prog_instructions_set(prog, instructions);

  ov_vm_prog_program_counter_reset(prog);
  ov_vm_prog_data_set(prog, data);

  ov_vm_prog_result_set(prog, OV_ERROR_NOERROR, 0);
  ov_vm_prog_set_last_instr_retval(prog, OC_NEXT);

  return prog_state;
}

/*----------------------------------------------------------------------------*/

static bool already_registered(ov_vm_prog_db *self, char const *id) {

  if (0 != ov_vm_prog_db_get(self, id)) {

    ov_log_error("ID %s already known to store", id);
    return true;

  } else {

    return false;
  }
}

/*----------------------------------------------------------------------------*/

static ov_vm_prog *insert_prog(ProgState *prog, ov_hashtable *store,
                               char const *id, ov_vm_instr const *instructions,
                               void *data) {

  OV_ASSERT((0 != store) && (0 != id) && (0 != instructions));

  if (0 == prog) {
    return 0;
  } else {
    prog->start_time_epoch_usecs = ov_time_get_current_time_usecs();
    return (ov_vm_prog *)register_prog_unsafe(
        store, init_prog_state_unsafe(prog, id, instructions, data));
  }
}

/*----------------------------------------------------------------------------*/

ov_vm_prog *ov_vm_prog_db_insert(ov_vm_prog_db *self, char const *id,
                                 ov_vm_instr const *instructions, void *data) {

  ov_hashtable *store = get_store_mut(self);

  if ((0 == store) ||
      (!ov_cond_valid(is_id_valid(id),
                      "Cannot register program - ID invalid")) ||
      (!are_inst_valid(instructions)) || already_registered(self, id)) {

    return 0;

  } else {
    return insert_prog(fresh_prog_state_unsafe(self), store, id, instructions,
                       data);
  }
}

/*----------------------------------------------------------------------------*/

bool ov_vm_prog_db_alias(ov_vm_prog_db *self, char const *id,
                         char const *alias) {

  ov_hashtable *aliases = get_aliases(self);
  char const *unaliased_id = get_unaliased_id(aliases, id);

  ov_vm_prog const *prog_of_id = ov_vm_prog_db_get(self, unaliased_id);
  ov_vm_prog const *prog_of_alias = ov_vm_prog_db_get(self, alias);

  if ((0 == aliases) || (!ov_ptr_valid(alias, "No alias given"))) {

    return false;

  } else if (0 == prog_of_id) {

    ov_log_error("Cannot alias program %s: No such program",
                 ov_string_sanitize(id));
    return false;

  } else if (prog_of_id == prog_of_alias) {

    ov_log_warning("Program %s already aliased with %s", ov_string_sanitize(id),
                   ov_string_sanitize(alias));
    return true;

  } else if (0 != prog_of_alias) {

    ov_log_error("Cannot alias program %s: Alias %s already used by program %s",
                 ov_string_sanitize(id), ov_string_sanitize(alias),
                 ov_string_sanitize(unaliased_id));
    return false;

  } else {

    ov_hashtable_set(aliases, alias, ov_string_dup(unaliased_id));
    return true;
  }
}

/*****************************************************************************
                                      GET
 ****************************************************************************/

ov_vm_prog *ov_vm_prog_db_get(ov_vm_prog_db *self, char const *id) {

  if ((!is_db_valid(self)) || (!is_id_valid(id))) {
    return 0;
  } else {
    return (ov_vm_prog *)prog_for_id(self, id);
  }
}

/*****************************************************************************
                                     REMOVE
 ****************************************************************************/

static bool release_prog(ov_vm_prog_db *self, ProgState *prog) {

  if (!ov_ptr_valid(self, "Cannot remove program from Prog DB: No DB given")) {

    return false;

  } else {

    if (ov_ptr_valid(prog, "Cannot remove program from Prog DB: No program")) {

      size_t capacity = self->available.capacity;
      size_t ti = self->available.topmost_usable_slot_index;

      ti += 1;

      OV_ASSERT(capacity >= ti);

      clear_state(prog, self->release.additional, self->release.release);

      self->available.states[ti] = prog;
      self->available.topmost_usable_slot_index = ti;
    }

    return true;
  }
}

/*----------------------------------------------------------------------------*/

struct gather_aliases_for_struct {

  char const *aliases[5];
  char const *id;
};

/*----------------------------------------------------------------------------*/

static bool insert_alias_if_space_left(struct gather_aliases_for_struct *ga_arg,
                                       char const *alias) {

  size_t i = 0;

  for (i = 0; i < 6; ++i) {

    if (0 == ga_arg->aliases[i]) {
      ga_arg->aliases[i] = alias;
      return true;
    }
  }

  return false;
}

/*----------------------------------------------------------------------------*/

static bool gather_aliases_for(void const *key, void const *value, void *arg) {

  OV_ASSERT(0 != arg);

  struct gather_aliases_for_struct *ga_arg = arg;

  if ((0 != value) && (0 != ga_arg->id) && (0 == strcmp(value, ga_arg->id))) {

    return insert_alias_if_space_left(ga_arg, key);
  }

  return true;
}

/*----------------------------------------------------------------------------*/

static char *string_dup(char const *str) {

  if (0 != str) {

    return strdup(str);

  } else {

    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static void remove_aliases_for(ov_hashtable *aliases, char const *id) {

  char *id_dup = string_dup(id);

  struct gather_aliases_for_struct ga_arg = {
      .id = id_dup,
  };

  size_t aliases_removed = 0;

  ov_hashtable_for_each(aliases, gather_aliases_for, &ga_arg);

  while (0 != ga_arg.aliases[0]) {

    for (size_t i = 0; i < 5; ++i) {

      if (0 != ga_arg.aliases[i]) {

        ov_log_debug("Removing %zu alias %s for %s", i, ga_arg.aliases[i],
                     ov_string_sanitize(id));

        ov_free(ov_hashtable_remove(aliases, ga_arg.aliases[i]));
        ++aliases_removed;
      }
    }

    memset(ga_arg.aliases, 0, sizeof(ga_arg.aliases));
    ov_hashtable_for_each(aliases, gather_aliases_for, &ga_arg);
  };

  id_dup = ov_free(id_dup);

  ov_log_debug("Freed %zu aliases", aliases_removed);
}

/*----------------------------------------------------------------------------*/

static bool remove_internal(ov_vm_prog_db *self, char const *id) {

  ov_log_info("Removing program %s", id);
  ProgState *prog = ov_hashtable_remove(get_store_mut(self), id);

  if (release_prog(self, prog)) {

    remove_aliases_for(get_aliases(self), id);
    return true;

  } else {

    return false;
  }
}

/*----------------------------------------------------------------------------*/

bool ov_vm_prog_db_remove(ov_vm_prog_db *self, char const *id) {

  return remove_internal(self, get_unaliased_id(get_aliases(self), id));
}

/*****************************************************************************
                                  UPDATE_TIME
 ****************************************************************************/

bool ov_vm_prog_db_update_time(ov_vm_prog_db *self, char const *id) {

  ProgState *state = prog_for_id(self, id);

  if ((!is_db_valid(self)) || (!is_id_valid(id)) || (0 == state)) {

    ov_log_warning("Cannot update time for VM prog '%s'",
                   ov_string_sanitize(id));
    return false;

  } else {

    ov_log_info("Updating time for request %s", id);
    state->start_time_epoch_usecs = ov_time_get_current_time_usecs();

    return true;
  }
}

/*****************************************************************************
                                    NEXT_DUE
 ****************************************************************************/

static char const *next_due_unsafe(ProgState *progs, size_t capacity,
                                   uint64_t time_limit_epoch_usecs) {

  OV_ASSERT(0 != progs);

  for (size_t i = 0; i < capacity; ++i) {

    if (time_limit_epoch_usecs > progs[i].start_time_epoch_usecs) {

      char const *id = ov_vm_prog_id((ov_vm_prog *)(progs + i));
      ov_log_info("Next due request: %s", id);

      return id;
    }
  }

  return 0;
}

/*----------------------------------------------------------------------------*/

char const *ov_vm_prog_db_next_due(ov_vm_prog_db *self,
                                   uint64_t time_limit_epoch_usecs) {

  if (!is_db_valid(self)) {

    return 0;

  } else {

    return next_due_unsafe(self->pool.states, self->pool.capacity,
                           time_limit_epoch_usecs);
  }
}

/*****************************************************************************
                                      DUMP
 ****************************************************************************/

static size_t dump_state_unsafe(FILE *out, size_t i, ProgState const *prog) {

  OV_ASSERT(0 != out);
  OV_ASSERT(0 != prog);

  uint64_t ts_usecs = prog->start_time_epoch_usecs;

  fprintf(out, "%5zu is ", i);

  if (ts_usecs == UINT64_MAX) {

    fprintf(out, "unused\n");
    return 0;

  } else {

    uint64_t ts_secs = ts_usecs / 1000 / 1000;
    uint64_t ts_remainder_usecs = ts_usecs - ts_secs * 1000 * 1000;

    fprintf(out, "IN USE. %s    Timestamp: %10" PRIu64 "s, %7" PRIu64 "us\n",
            ov_vm_prog_id((ov_vm_prog *)prog), ts_secs, ts_remainder_usecs);

    return 1;
  }
}

/*----------------------------------------------------------------------------*/

static size_t dump_states_unsafe(FILE *out, ProgState const *progs,
                                 size_t capacity) {

  OV_ASSERT(0 != out);
  OV_ASSERT(0 != progs);

  size_t num_in_use = 0;

  for (size_t i = 0; i < capacity; ++i) {

    num_in_use += dump_state_unsafe(out, i, progs + i);
  }

  return num_in_use;
}

/*----------------------------------------------------------------------------*/

void ov_vm_prog_db_dump(FILE *out, ov_vm_prog_db *self) {

  if ((0 == out) || (0 == self)) {

    ov_log_error("0 pointer as parameter");

  } else {

    size_t in_use =
        dump_states_unsafe(out, self->pool.states, self->pool.capacity);

    fprintf(out,
            "\nCapacity: %5zu   Entries used: %5zu   Entries unused: "
            "%5zu  "
            " in available pool: %5zu\n\n",
            self->pool.capacity, in_use, self->pool.capacity - in_use,
            self->available.topmost_usable_slot_index);
  }
}

/*****************************************************************************
                                for each program
 ****************************************************************************/

typedef struct process_wrapper_arg {
  uint32_t magic_bytes;
  void *original_arg;
  bool (*process)(char const *id, ov_vm_prog const *prog,
                  uint64_t start_time_epoch_usecs, void *data);
} ProcessArg;

#define PROCESS_ARG_MAGIC_BYTES 0x12a2b321

static ProcessArg *as_process_arg(void *varg) {

  ProcessArg *parg = varg;

  if (0 == parg) {

    return 0;

  } else {

    OV_ASSERT(PROCESS_ARG_MAGIC_BYTES == parg->magic_bytes);
    return varg;
  }
}

/*----------------------------------------------------------------------------*/

static bool process_wrapper(void const *key, void const *value, void *arg) {

  ProcessArg *parg = as_process_arg(arg);

  ProgState const *state = value;

  if (ov_ptr_valid(state,
                   "Cannot iterate over active VM programs: internal error - "
                   "no program") &&
      (0 != parg)) {

    return parg->process((char const *)key, (ov_vm_prog const *)state->public,
                         state->start_time_epoch_usecs, parg->original_arg);
  } else {
    return false;
  }
}

/*----------------------------------------------------------------------------*/

bool ov_vm_prog_db_for_each(ov_vm_prog_db const *self,
                            bool (*process)(char const *id,
                                            ov_vm_prog const *prog,
                                            uint64_t start_time_epoch_usecs,
                                            void *data),
                            void *data) {

  ProcessArg parg = {
      .magic_bytes = PROCESS_ARG_MAGIC_BYTES,
      .process = process,
      .original_arg = data,
  };

  ov_hashtable const *store = get_store(self);

  if (ov_ptr_valid(store, "Cannot iterate over programs - no Program DB") &&
      ov_ptr_valid(process,
                   "Cannot iterate over programs - no processing function")) {

    ov_hashtable_for_each(store, process_wrapper, &parg);
    return true;

  } else {

    return false;
  }
}

/*----------------------------------------------------------------------------*/
