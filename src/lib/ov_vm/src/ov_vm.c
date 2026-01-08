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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "../include/ov_vm.h"
#include <../include/ov_vm_prog_db.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_time.h>

/*----------------------------------------------------------------------------*/

ov_vm_instr OV_VM_INST_END = {OP_END, 0, 0, 0};

/*----------------------------------------------------------------------------*/

typedef struct {

  char const *symbol;
  ov_vm_opcode_handler handler;
  ov_vm_opcode_handler inv_handler;

} OpcodeDef;

struct ov_vm_struct {

  ov_event_loop *loop;
  uint32_t timer_id;
  uint64_t program_timeout_usecs;

  OpcodeDef opcode[UINT8_MAX];

  ov_vm_prog_db *db;

  struct {
    void (*program_failed_to_abort)(ov_vm *, char const *id);
    void (*program_aborted)(ov_vm *, char const *id);
    void (*program_done)(ov_vm *, char const *id);
  } notifier;

  void (*release_data)(ov_vm *, void *);
};

/*****************************************************************************
                                    HELPERS
 ****************************************************************************/

static bool is_vm_valid(ov_vm const *vm) {

  if (0 == vm) {
    ov_log_error("No VM");
    return false;
  } else {
    return true;
  }
}

/*----------------------------------------------------------------------------*/

static bool are_instr_valid(ov_vm_instr const *instr) {

  if (0 == instr) {
    ov_log_error("No instructions");
    return false;
  } else {
    return true;
  }
}

/*----------------------------------------------------------------------------*/

static bool is_id_valid(char const *id) {

  if (0 == id) {
    ov_log_error("No ID given");
    return false;
  } else {
    return true;
  }
}

/*----------------------------------------------------------------------------*/

static bool is_prog_valid(ov_vm_prog *prog) {
  if (OV_VM_PROG_INVALID == ov_vm_prog_state(prog)) {
    ov_log_error("Programs %s is invalid", ov_vm_prog_id(prog));
    return false;
  } else {
    return true;
  }
}

/*----------------------------------------------------------------------------*/

static ov_vm_prog_db const *get_prog_db(ov_vm const *self) {

  if (is_vm_valid(self)) {
    return self->db;
  } else {
    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static ov_vm_prog_db *get_prog_db_mut(ov_vm *self) {

  if (is_vm_valid(self)) {
    return self->db;
  } else {
    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static ov_vm_prog *prog_for_id(ov_vm *self, char const *id) {

  return ov_vm_prog_db_get(get_prog_db_mut(self), id);
}

/*****************************************************************************
                                STANDARD OPCODES
 ****************************************************************************/

ov_vm_retval end_handler(ov_vm *vm, ov_vm_prog *prog) {

  UNUSED(vm);
  UNUSED(prog);

  ov_log_info("Program reached END");
  return OC_FINISHED;
}

/*----------------------------------------------------------------------------*/

ov_vm_retval nop_handler(ov_vm *vm, ov_vm_prog *prog) {

  UNUSED(vm);
  UNUSED(prog);

  ov_log_info("NOP performed");
  return OC_NEXT;
}

/*----------------------------------------------------------------------------*/

ov_vm_retval invalid_handler(ov_vm *vm, ov_vm_prog *prog) {

  UNUSED(vm);
  UNUSED(prog);

  ov_log_error("Tried to execute INVALID opcode");
  return OC_ERROR;
}

/*****************************************************************************
                                  PRUNE TIMER
 ****************************************************************************/

static void abort_program(ov_vm *self, ov_vm_prog *prog);

static void prune_overdue_requests(ov_vm *self, ov_vm_prog_db *db,
                                   uint64_t timeout_usecs,
                                   size_t max_no_to_prune) {

  uint64_t now_usecs = ov_time_get_current_time_usecs() - timeout_usecs;

  char const *id = ov_vm_prog_db_next_due(db, now_usecs);
  size_t overdue = 0;

  for (; (0 != id) && (overdue < max_no_to_prune); ++overdue) {

    ov_log_info("Program %s overdue", ov_string_sanitize(id));
    ov_vm_prog *prog = ov_vm_prog_db_get(db, id);
    abort_program(self, prog);

    id = ov_vm_prog_db_next_due(db, now_usecs);
  }

  ov_log_info("Aborted %zu overdue programs", overdue);
}

/*----------------------------------------------------------------------------*/

static bool timer_callback(uint32_t id, void *the_vm) {

  UNUSED(id);

  ov_vm *vm = the_vm;

  if (!is_vm_valid(vm)) {
    return false;
  } else {
    prune_overdue_requests(vm, vm->db, vm->program_timeout_usecs, 5);
    vm->timer_id = ov_event_loop_timer_set(vm->loop, vm->program_timeout_usecs,
                                           vm, timer_callback);

    return true;
  }
}

/*****************************************************************************
                                     CREATE
 ****************************************************************************/

static void release_data(void *data, void *the_vm) {

  ov_vm *vm = the_vm;

  if ((!is_vm_valid(the_vm)) || (0 == data) || (0 == vm->release_data)) {
    return;
  } else {
    vm->release_data(vm, data);
  }
}

/*----------------------------------------------------------------------------*/

ov_vm *ov_vm_create(ov_vm_config cfg, ov_event_loop *loop) {

  UNUSED(cfg);

  if (0 == loop) {
    ov_log_error("No loop given");
    return 0;
  }

  ov_vm *vm = calloc(1, sizeof(ov_vm));

  vm->opcode[OP_END] = (OpcodeDef){"END", end_handler, 0};
  vm->opcode[OP_NOP] = (OpcodeDef){"NOP", nop_handler, 0};
  vm->opcode[OP_INVALID] = (OpcodeDef){"INVALID", invalid_handler, 0};

  vm->db = ov_vm_prog_db_create(cfg.max_requests, vm, release_data);

  vm->release_data = cfg.release_data;

  vm->notifier.program_aborted = cfg.notify_program_aborted;
  vm->notifier.program_failed_to_abort = cfg.notify_program_failed_to_abort;
  vm->notifier.program_done = cfg.notify_program_done;

  uint64_t timeout_msecs =
      OV_OR_DEFAULT(cfg.default_program_timeout_msecs,
                    OV_DEFAULT_VM_TIMER_INTERVAL_USECS / 1000);

  vm->loop = loop;
  vm->program_timeout_usecs = 1000 * timeout_msecs;
  vm->timer_id = ov_event_loop_timer_set(vm->loop, vm->program_timeout_usecs,
                                         vm, timer_callback);

  return vm;
}

/*----------------------------------------------------------------------------*/

ov_vm *ov_vm_free(ov_vm *self) {

  if (0 == self) {
    return 0;
  } else {
    self->timer_id = ov_event_loop_timer_unset(self->loop, self->timer_id, 0);
    self->db = ov_vm_prog_db_free(self->db);
    free(self);
    return 0;
  }
}

/*----------------------------------------------------------------------------*/

uint64_t ov_vm_program_timeout_msecs(ov_vm const *self) {

  if (ov_ptr_valid(self, "Cannot get program timeout of VM - no VM")) {
    return self->program_timeout_usecs / 1000.0;
  } else {
    return 0;
  }
}

/*----------------------------------------------------------------------------*/

bool ov_vm_program_set_timeout_msecs(ov_vm *self, uint64_t timeout_msecs) {

  if (ov_ptr_valid(self, "Cannot get program timeout of VM - no VM")) {
    self->program_timeout_usecs = timeout_msecs * 1000.0;
    return true;
  } else {
    return false;
  }
}

/*----------------------------------------------------------------------------*/

static OpcodeDef *opcodedef(ov_vm *vm, uint8_t oc) {

  if (!is_vm_valid(vm)) {
    return 0;
  } else if (oc * sizeof(OpcodeDef) >= sizeof(vm->opcode)) {
    ov_log_error("Opcode %" PRIu8 " out of range", oc);
    return 0;
  } else {
    return vm->opcode + oc;
  }
}

/*----------------------------------------------------------------------------*/

static char const *op_to_symbol(ov_vm *self, uint8_t oc) {

  OpcodeDef *def = opcodedef(self, oc);

  if ((0 == def) || (0 == def->symbol)) {
    return "UNKNOWN";
  } else {
    return def->symbol;
  }
}

/*----------------------------------------------------------------------------*/

#define DESC_OPCODE(self, buf, oc) describe_opcode(self, sizeof(buf), buf, oc)

static char *describe_opcode(ov_vm *self, size_t blen_bytes, char *buffer,
                             uint8_t oc) {

  OV_ASSERT(0 != buffer);
  OV_ASSERT(0 < blen_bytes);

  snprintf(buffer, blen_bytes, "%s(%" PRIu8 ")", op_to_symbol(self, oc), oc);

  return buffer;
}

/*----------------------------------------------------------------------------*/

ov_vm_retval inv_handler_dummy(ov_vm *self, ov_vm_prog *prog) {

  char buf[30] = {0};
  ov_vm_instr curr = ov_vm_prog_current_instr(prog);

  ov_log_info("Inverting %s - nothing to be done",
              DESC_OPCODE(self, buf, curr.opcode));

  return OC_NEXT;
}

/*----------------------------------------------------------------------------*/

static ov_vm_opcode_handler sanitize_inv_handler(ov_vm_opcode_handler handler,
                                                 uint8_t opcode,
                                                 char const *symbol) {

  if (0 == handler) {

    ov_log_warning("No inverse handler given for %s (%" PRIu8 ")",
                   ov_string_sanitize(symbol), opcode);
    return inv_handler_dummy;
  } else {
    return handler;
  }
}

/*----------------------------------------------------------------------------*/

bool ov_vm_register(ov_vm *self, uint8_t opcode, char const *symbol,
                    ov_vm_opcode_handler handler,
                    ov_vm_opcode_handler inv_handler) {

  OpcodeDef *def = opcodedef(self, opcode);

  inv_handler = sanitize_inv_handler(inv_handler, opcode, symbol);

  if (0 == def) {

    return false;

  } else if ((OP_END == opcode) || (OP_NOP == opcode)) {

    char buf[30] = {0};
    ov_log_error("Tried to overwrite %s", DESC_OPCODE(self, buf, opcode));
    return false;

  } else if ((0 == symbol) || (0 == handler)) {
    ov_log_error("symbol or handler missing");
    return false;

  } else {
    def->symbol = symbol;
    def->handler = handler;
    def->inv_handler = inv_handler;
    return true;
  }
}

/*----------------------------------------------------------------------------*/

static void notify_program_failed_to_abort(ov_vm *self, ov_vm_prog *prog) {

  if ((0 != self) && (0 != prog) &&
      (0 != self->notifier.program_failed_to_abort)) {
    self->notifier.program_failed_to_abort(self, ov_vm_prog_id(prog));
  }
}

/*----------------------------------------------------------------------------*/

static void notify_program_aborted(ov_vm *self, ov_vm_prog *prog) {

  if ((0 != self) && (0 != prog) && (0 != self->notifier.program_aborted)) {
    self->notifier.program_aborted(self, ov_vm_prog_id(prog));
  }
}

/*----------------------------------------------------------------------------*/

static void notify_program_done(ov_vm *self, ov_vm_prog *prog) {

  if ((0 != self) && (0 != prog) && (0 != self->notifier.program_done)) {
    self->notifier.program_done(self, ov_vm_prog_id(prog));
  }
}

/*----------------------------------------------------------------------------*/

static void release_program(ov_vm *self, ov_vm_prog *prog) {

  ov_vm_prog_log_result(prog);

  switch (ov_vm_prog_state(prog)) {

  case OV_VM_PROG_FAILED_TO_ABORT:
    notify_program_failed_to_abort(self, prog);
    ov_vm_prog_db_remove(self->db, ov_vm_prog_id(prog));
    break;

  case OV_VM_PROG_ABORTING:
    notify_program_aborted(self, prog);
    ov_vm_prog_db_remove(self->db, ov_vm_prog_id(prog));
    break;

  case OV_VM_PROG_OK:
    notify_program_done(self, prog);
    ov_vm_prog_db_remove(self->db, ov_vm_prog_id(prog));
    break;

  case OV_VM_PROG_INVALID:
    ov_log_error("Serious internal error: No program (0 pointer)");
    // OV_ASSERT(! "MUST NEVER HAPPEN");
    break;
  }
}

/*----------------------------------------------------------------------------*/

static ov_vm_exec_result execute(ov_vm *self, ov_vm_prog *prog);

static void abort_program_with(ov_vm *self, ov_vm_prog *prog,
                               ov_vm_abort_opts opts) {

  char *backtrace = 0;

  switch (ov_vm_prog_state(prog)) {

  case OV_VM_PROG_ABORTING:
    ov_vm_prog_state_set(prog, OV_VM_PROG_FAILED_TO_ABORT);
    // Call failed handler
    release_program(self, prog);
    break;

  case OV_VM_PROG_OK:
    ov_vm_prog_state_set(prog, OV_VM_PROG_ABORTING);

    if ((!opts.finish_current_step) &&
        (OC_WAIT_AND_NEXT == ov_vm_prog_get_last_instr_retval(prog))) {
      // After trigger/continue returns, the program counter
      // of our program points to the next instruction to execute.
      // This is fine if the last instruction was completed.
      // However, for WAIT_AND_REPEAT and WAIT_AND_NEXT, the
      // last instruction was not completed, but interrupted.
      // Hence we need to decrease the counter once more
      // to prevent inverting the unfinished instruction
      ov_vm_prog_propagate_program_counter(prog);
    }

    ov_vm_prog_propagate_program_counter(prog);
    execute(self, prog);
    break;

  case OV_VM_PROG_FAILED_TO_ABORT:
    ov_log_warning("Program is in state 'FAILED_TO_ABORT'");
    OV_ASSERT(!" NEVER TO HAPPEN");
    abort();
    break;

  case OV_VM_PROG_INVALID:
    ov_log_warning("Program is invalid: %p", prog);
    backtrace = ov_arch_compile_backtrace(20);
    ov_log_warning("Backtrace: %s", ov_string_sanitize(backtrace));
    backtrace = ov_free(backtrace);
    break;
  };
}

/*----------------------------------------------------------------------------*/

static void abort_program(ov_vm *self, ov_vm_prog *prog) {

  return abort_program_with(self, prog, (ov_vm_abort_opts){0});
}

/*----------------------------------------------------------------------------*/

static ov_vm_opcode_handler handler_from_opcode_def(OpcodeDef const *def) {

  if (0 == def) {
    return 0;
  } else if (0 == def->handler) {
    ov_log_error("No handler for instruction %s", def->symbol);
    return 0;
  } else {
    return def->handler;
  }
}

/*----------------------------------------------------------------------------*/

static ov_vm_opcode_handler
inverse_handler_from_opcode_def(OpcodeDef const *def) {

  if (0 == def) {
    ov_log_error("No opcode definition");
    return 0;
  } else {
    return def->inv_handler;
  }
}

/*----------------------------------------------------------------------------*/

static ov_vm_opcode_handler
get_handler_from_opcode_def(OpcodeDef const *def,
                            ov_vm_prog_status prog_state) {

  switch (prog_state) {

  case OV_VM_PROG_OK:

    return handler_from_opcode_def(def);

  case OV_VM_PROG_ABORTING:

    return inverse_handler_from_opcode_def(def);

  case OV_VM_PROG_FAILED_TO_ABORT:
    ov_log_error("Invalid program - failed to abort");
    return 0;

  case OV_VM_PROG_INVALID:
    ov_log_error("Invalid program");
    return 0;

  default:
    OV_ASSERT(!"MUST NEVER HAPPEN");
    abort();
  }
}

/*----------------------------------------------------------------------------*/

static OpcodeDef const *get_opcode_def(ov_vm const *self, uint8_t opcode) {

  if (!is_vm_valid(self)) {
    return 0;
  } else if (OP_INVALID == opcode) {
    ov_log_error("Invalid opcode");
    return 0;
  } else {
    return self->opcode + opcode;
  }
}

/*----------------------------------------------------------------------------*/

static ov_vm_opcode_handler get_handler_for_current_instr(ov_vm const *self,
                                                          ov_vm_prog *prog) {

  ov_vm_instr instr = ov_vm_prog_current_instr(prog);
  OpcodeDef const *def = get_opcode_def(self, instr.opcode);

  if (0 == def) {
    return 0;
  } else {
    return get_handler_from_opcode_def(def, ov_vm_prog_state(prog));
  }
}

/*----------------------------------------------------------------------------*/

static int execute_current_instruction(ov_vm *self, ov_vm_prog *prog) {

  ov_vm_opcode_handler handler = get_handler_for_current_instr(self, prog);

  if (0 == handler) {
    return OC_ERROR;
  } else {

    ov_vm_instr instr = ov_vm_prog_current_instr(prog);
    char buf[30] = {0};
    ov_log_info("Executing program %s - %s", ov_vm_prog_id(prog),
                DESC_OPCODE(self, buf, instr.opcode));

    return handler(self, prog);
  }
}

/*----------------------------------------------------------------------------*/

static ov_vm_exec_result execute(ov_vm *self, ov_vm_prog *prog) {

  if (!is_prog_valid(prog)) {
    return OV_EXEC_ERROR;
  }

  do {

    ov_vm_retval retval = execute_current_instruction(self, prog);
    ov_vm_prog_set_last_instr_retval(prog, retval);

    switch (retval) {

    case OC_ERROR:

      abort_program(self, prog);
      return OV_EXEC_ERROR;

    case OC_NEXT:
      ov_vm_prog_propagate_program_counter(prog);
      break;

    case OC_WAIT_AND_NEXT:

      ov_vm_prog_propagate_program_counter(prog);

      if (OV_VM_PROG_ABORTING == ov_vm_prog_state(prog)) {
        ov_log_error("WAIT ignored while aborting prorgram");
        break;
      } else {
        return OV_EXEC_WAIT;
      }

    case OC_WAIT_AND_REPEAT:

      if (OV_VM_PROG_ABORTING == ov_vm_prog_state(prog)) {
        ov_log_error("WAIT ignored while aborting prorgram");
        break;
      } else {
        return OV_EXEC_WAIT;
      }

    case OC_FINISHED:

      release_program(self, prog);
      return OV_EXEC_OK;
    };

  } while (-1 < ov_vm_prog_program_counter(prog));

  // Program was aborted, abortion finished
  release_program(self, prog);
  return OV_EXEC_ERROR;
}

/*----------------------------------------------------------------------------*/

static ov_vm_exec_result execute_prog(ov_vm *self, ov_vm_prog *prog) {

  if (!is_vm_valid(self)) {
    // We cannot do much - since we don't have a VM, we cannto release the
    // program in the database
    return OV_EXEC_ERROR;
  } else if (0 == prog) {
    ov_log_error("No program to execute (0 pointer)");
    return OV_EXEC_TRIGGER_FAIL;
  } else {
    return execute(self, prog);
  }
}

/*----------------------------------------------------------------------------*/

ov_vm_exec_result ov_vm_trigger(ov_vm *self, ov_vm_instr const *instructions,
                                char const *id, void *data) {

  if ((!is_vm_valid(self)) || (!are_instr_valid(instructions)) ||
      (!is_id_valid(id))) {

    return OV_EXEC_TRIGGER_FAIL;

  } else {

    return execute_prog(self,
                        ov_vm_prog_db_insert(self->db, id, instructions, data));
  }
}

/*----------------------------------------------------------------------------*/

ov_vm_exec_result ov_vm_continue(ov_vm *self, char const *id) {

  ov_vm_prog *prog = prog_for_id(self, id);

  if (0 == prog) {
    ov_log_error("No such program: %s", ov_string_sanitize(id));
    return OV_EXEC_ERROR;
  } else {
    return execute(self, prog);
  }
}

/*----------------------------------------------------------------------------*/

bool ov_vm_abort_with(ov_vm *self, char const *id, ov_vm_abort_opts opts) {

  ov_vm_prog *prog = prog_for_id(self, id);

  if (0 == prog) {
    ov_log_error("No such program: %s", ov_string_sanitize(id));
    return false;
  } else {
    abort_program_with(self, prog, opts);
    return true;
  }
}

/*****************************************************************************
                             Add an alias for a program
 ****************************************************************************/

bool ov_vm_alias(ov_vm *self, ov_vm_prog *prog, char const *new_id) {

  char const *old_id = ov_vm_prog_id(prog);
  return ov_vm_prog_db_alias(get_prog_db_mut(self), old_id, new_id);
}

/*****************************************************************************
                                    DATA FOR
 ****************************************************************************/

void *ov_vm_data_for(ov_vm *self, char const *id) {

  if ((!is_vm_valid(self)) || (!is_id_valid(id))) {
    return 0;
  } else {
    return ov_vm_prog_data(ov_vm_prog_db_get(self->db, id));
  }
}

/*----------------------------------------------------------------------------*/

bool ov_vm_for_each_program(ov_vm const *self,
                            bool (*process)(char const *id,
                                            ov_vm_prog const *prog,
                                            uint64_t start_time_epoch_usecs,
                                            void *data),
                            void *data) {

  return ov_vm_prog_db_for_each(get_prog_db(self), process, data);
}

/*----------------------------------------------------------------------------*/
