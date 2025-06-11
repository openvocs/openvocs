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

#include "ov_vm.c"
#include "ov_vm.h"
#include <ov_base/ov_random.h>

const uint64_t TIMEOUT_VM_MSECS = 1 * 1000;

/*****************************************************************************
                                    OPCODES
 ****************************************************************************/

const uint8_t OP_NEXT = 0x01;
const uint8_t OP_NEXT_INV = 0x11;
const uint8_t OP_WAIT = 0x02;
const uint8_t OP_WAIT_INV = 0x12;
const uint8_t OP_NEXT_2 = 0x03;
const uint8_t OP_NEXT_2_INV = 0x13;

const uint8_t OP_NEXT_NO_INV = 0x04;
const uint8_t OP_ABORT = 0x05;
const uint8_t OP_NEXT_INV_ABORTS = 0x06;

typedef struct {

    size_t times_called;

    ov_vm_instr instr;
    char const *id;
    void *data;

} handler_state;

/*----------------------------------------------------------------------------*/

static bool opcode_equals(handler_state self, int16_t opcode) {

    if (0 > opcode) {
        return true;
    } else {
        return opcode == (int16_t)self.instr.opcode;
    }
}

/*----------------------------------------------------------------------------*/

static bool arg_equals(handler_state self, size_t arg_index, int16_t ref) {

    TEST_ASSERT(3 > arg_index);

    if (0 > ref) {
        return true;
    } else {
        return ref == (int16_t)self.instr.args[arg_index];
    }
}

/*----------------------------------------------------------------------------*/

static bool id_equals(handler_state self, char const *ref) {

    if (0 == ref) {
        return true;
    } else if (0 == self.id) {
        return false;
    } else {
        return 0 == strcmp(self.id, ref);
    }
}

/*----------------------------------------------------------------------------*/

static bool data_equals(handler_state self, void *ref) {

    if (0 == ref) {
        return true;
    } else {
        return self.data == ref;
    }
}

/*----------------------------------------------------------------------------*/

static void reset_handler_state(handler_state *self) {
    self->times_called = 0;
    self->data = 0;
    self->id = 0;
    memset(&self->instr, 0, sizeof(self->instr));
}

/*----------------------------------------------------------------------------*/

#define IGNORE -1

static bool handler_state_equals(handler_state self,
                                 size_t times_called,
                                 int16_t opcode,
                                 int16_t arg0,
                                 int16_t arg1,
                                 int16_t arg2,
                                 char const *id,
                                 void *data) {

    return (self.times_called == times_called) && opcode_equals(self, opcode) &&
           arg_equals(self, 0, arg0) && arg_equals(self, 1, arg1) &&
           arg_equals(self, 2, arg2) && id_equals(self, id) &&
           data_equals(self, data);
}

/*----------------------------------------------------------------------------*/

static handler_state next_state = {0};

static ov_vm_retval next_handler(ov_vm *vm, ov_vm_prog *prog) {
    UNUSED(vm);

    ++next_state.times_called;

    next_state.instr = ov_vm_prog_current_instr(prog);
    next_state.id = ov_vm_prog_id(prog);
    next_state.data = ov_vm_prog_data(prog);

    return OC_NEXT;
}

/*----------------------------------------------------------------------------*/

static handler_state next_inv_state = {0};

static ov_vm_retval next_inv_handler(ov_vm *vm, ov_vm_prog *prog) {
    UNUSED(vm);

    ++next_inv_state.times_called;

    next_inv_state.instr = ov_vm_prog_current_instr(prog);
    next_inv_state.id = ov_vm_prog_id(prog);
    next_inv_state.data = ov_vm_prog_data(prog);

    return OC_NEXT;
}

/*----------------------------------------------------------------------------*/

static handler_state next_2_state = {0};

static ov_vm_retval next_2_handler(ov_vm *vm, ov_vm_prog *prog) {
    UNUSED(vm);

    ++next_2_state.times_called;

    next_2_state.instr = ov_vm_prog_current_instr(prog);
    next_2_state.id = ov_vm_prog_id(prog);
    next_2_state.data = ov_vm_prog_data(prog);

    return OC_NEXT;
}

/*----------------------------------------------------------------------------*/

static handler_state next_2_inv_state = {0};

static ov_vm_retval next_2_inv_handler(ov_vm *vm, ov_vm_prog *prog) {
    UNUSED(vm);

    ++next_2_inv_state.times_called;

    next_2_inv_state.instr = ov_vm_prog_current_instr(prog);
    next_2_inv_state.id = ov_vm_prog_id(prog);
    next_2_inv_state.data = ov_vm_prog_data(prog);

    return OC_NEXT;
}

/*----------------------------------------------------------------------------*/

static handler_state wait_state = {0};

static ov_vm_retval wait_handler(ov_vm *vm, ov_vm_prog *prog) {
    UNUSED(vm);

    ++wait_state.times_called;

    wait_state.instr = ov_vm_prog_current_instr(prog);
    wait_state.id = ov_vm_prog_id(prog);
    wait_state.data = ov_vm_prog_data(prog);

    return OC_WAIT_AND_NEXT;
}

/*----------------------------------------------------------------------------*/

static handler_state wait_inv_state = {0};

static ov_vm_retval wait_inv_handler(ov_vm *vm, ov_vm_prog *prog) {
    UNUSED(vm);

    ++wait_inv_state.times_called;

    wait_inv_state.instr = ov_vm_prog_current_instr(prog);
    wait_inv_state.id = ov_vm_prog_id(prog);
    wait_inv_state.data = ov_vm_prog_data(prog);

    return OC_WAIT_AND_NEXT;
}

/*----------------------------------------------------------------------------*/

static handler_state abort_state = {0};

static ov_vm_retval abort_handler(ov_vm *vm, ov_vm_prog *prog) {
    UNUSED(vm);

    ++abort_state.times_called;

    abort_state.instr = ov_vm_prog_current_instr(prog);
    abort_state.id = ov_vm_prog_id(prog);
    abort_state.data = ov_vm_prog_data(prog);

    return OC_ERROR;
}

/*----------------------------------------------------------------------------*/

static void reset_handler_states() {

    reset_handler_state(&next_state);
    reset_handler_state(&next_2_state);
    reset_handler_state(&wait_state);
    reset_handler_state(&next_inv_state);
    reset_handler_state(&next_2_inv_state);
    reset_handler_state(&abort_state);
    reset_handler_state(&wait_inv_state);
}

/*****************************************************************************
                             NOTIFICATION HANDLERS
 ****************************************************************************/

typedef struct {
    size_t times_called;
    char const *prog;
} notify_state;

static bool notify_state_equals(notify_state state,
                                size_t times_called,
                                char const *prog) {

    return (state.times_called == times_called) &&
           (0 == ov_string_compare(prog, state.prog));
}

/*----------------------------------------------------------------------------*/

static notify_state failed_state = {0};

void notify_failed(ov_vm *self, char const *id) {
    UNUSED(self);
    ++failed_state.times_called;
    failed_state.prog = id;
}

static notify_state aborted_state = {0};

void notify_aborted(ov_vm *self, char const *id) {
    UNUSED(self);
    ++aborted_state.times_called;
    aborted_state.prog = id;
}

static notify_state done_state = {0};

void notify_done(ov_vm *self, char const *id) {
    UNUSED(self);
    ++done_state.times_called;
    done_state.prog = id;
}

/*----------------------------------------------------------------------------*/

static void reset_notify_states() {
    failed_state = (notify_state){0};
    aborted_state = (notify_state){0};
    done_state = (notify_state){0};
}

/*----------------------------------------------------------------------------*/

struct {
    void *last_data_to_be_freed;
    size_t times_called;
} release_state = {0};

static void release_dummy(ov_vm *the_vm, void *data) {

    UNUSED(the_vm);

    ++release_state.times_called;
    release_state.last_data_to_be_freed = data;
}

/*----------------------------------------------------------------------------*/

static bool release_state_equals(size_t times_called, void *data) {

    return (release_state.times_called == times_called) &&
           (release_state.last_data_to_be_freed == data);
}

/*----------------------------------------------------------------------------*/

static void reset_release_state() {
    memset(&release_state, 0, sizeof(release_state));
}

/*****************************************************************************
                                    HELPERS
 ****************************************************************************/

typedef struct {
    ov_vm *vm;
    ov_event_loop *loop;

} resources;

static resources create_resources(ov_vm_config *cfg_ptr,
                                  ov_event_loop_config *loop_cfg_ptr) {
    ov_vm_config cfg = {
        .default_program_timeout_msecs = TIMEOUT_VM_MSECS,
    };

    ov_event_loop_config loop_cfg = ov_event_loop_config_default();

    if (0 != cfg_ptr) {
        cfg = *cfg_ptr;
    }

    if (0 != loop_cfg_ptr) {
        loop_cfg = *loop_cfg_ptr;
    }

    resources res = {
        .loop = ov_event_loop_default(loop_cfg),
    };

    res.vm = ov_vm_create(cfg, res.loop);

    return res;
}

/*----------------------------------------------------------------------------*/

static bool register_opcodes(ov_vm *vm) {

    return ov_vm_register(
               vm, OP_NEXT, "NEXT", next_handler, next_inv_handler) &&
           ov_vm_register(
               vm, OP_NEXT_2, "NEXT_2", next_2_handler, next_2_inv_handler) &&
           ov_vm_register(
               vm, OP_WAIT, "WAIT", wait_handler, wait_inv_handler) &&
           ov_vm_register(vm, OP_ABORT, "ABORT", abort_handler, 0) &&
           ov_vm_register(vm,
                          OP_NEXT_INV_ABORTS,
                          "NEXT_INV_ABORTS",
                          next_handler,
                          abort_handler) &&
           ov_vm_register(vm, OP_NEXT_NO_INV, "NEXT_NO_INV", next_handler, 0);
}

/*----------------------------------------------------------------------------*/

static bool free_resources(resources res) {
    if (0 != res.vm) {
        res.vm = ov_vm_free(res.vm);
    }
    if (0 != res.loop) {
        res.loop = ov_event_loop_free(res.loop);
    }

    return (0 == res.vm) && (0 == res.loop);
}

/*----------------------------------------------------------------------------*/

static resources create_resources_with_custom_opcodes(
    ov_vm_config *vm_cfg, ov_event_loop_config *loop_cfg) {

    resources res = create_resources(vm_cfg, loop_cfg);

    if (!register_opcodes(res.vm)) {
        free_resources(res);
    }

    return res;
}

/*----------------------------------------------------------------------------*/

static resources create_resources_w_opcodes_notify_handlers(
    ov_vm_config *vm_cfg, ov_event_loop_config *loop_cfg) {

    ov_vm_config cfg = {
        .default_program_timeout_msecs = TIMEOUT_VM_MSECS,
    };

    if (0 == vm_cfg) {
        vm_cfg = &cfg;
    }

    vm_cfg->notify_program_failed_to_abort =
        OV_OR_DEFAULT(vm_cfg->notify_program_failed_to_abort, notify_failed);

    vm_cfg->notify_program_aborted =
        OV_OR_DEFAULT(vm_cfg->notify_program_aborted, notify_aborted);

    vm_cfg->notify_program_done =
        OV_OR_DEFAULT(vm_cfg->notify_program_done, notify_done);

    vm_cfg->release_data = OV_OR_DEFAULT(vm_cfg->release_data, release_dummy);

    return create_resources_with_custom_opcodes(vm_cfg, loop_cfg);
}

/*****************************************************************************
                                     CREATE
 ****************************************************************************/

static int test_ov_vm_create() {
    ov_vm_config cfg = {0};

    testrun(0 == ov_vm_create(cfg, 0));

    ov_event_loop *loop = ov_event_loop_default(ov_event_loop_config_default());
    testrun(0 != loop);

    ov_vm *vm = ov_vm_create(cfg, loop);
    testrun(0 != vm);

    vm = ov_vm_free(vm);
    loop = ov_event_loop_free(loop);

    testrun(0 == vm);
    testrun(0 == loop);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_vm_free() {
    testrun(0 == ov_vm_free(0));

    resources res = create_resources(0, 0);

    testrun(0 != res.vm);
    testrun(0 == ov_vm_free(res.vm));
    testrun(0 == ov_event_loop_free(res.loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_vm_register() {
    testrun(!ov_vm_register(0, 0, 0, 0, 0));

    resources res = create_resources(0, 0);
    testrun(0 != res.vm);

    testrun(!ov_vm_register(res.vm, 0, 0, 0, 0));
    testrun(!ov_vm_register(0, 0x01, 0, 0, 0));
    testrun(!ov_vm_register(res.vm, 0x01, 0, 0, 0));
    testrun(!ov_vm_register(0, 0, "OPCODE01", 0, 0));
    testrun(!ov_vm_register(res.vm, 0, "OPCODE01", 0, 0));
    testrun(!ov_vm_register(0, 0x01, "OPCODE01", 0, 0));
    testrun(!ov_vm_register(res.vm, 0x01, "OPCODE01", 0, 0));
    testrun(!ov_vm_register(0, 0, 0, next_handler, 0));
    testrun(!ov_vm_register(res.vm, 0, 0, next_handler, 0));
    testrun(!ov_vm_register(0, 0x01, 0, next_handler, 0));
    testrun(!ov_vm_register(res.vm, 0x01, 0, next_handler, 0));
    testrun(!ov_vm_register(0, 0, "OPCODE01", next_handler, 0));
    testrun(ov_vm_register(res.vm, 0x01, "OPCODE01", next_handler, 0));
    testrun(!ov_vm_register(0, 0, 0, 0, next_inv_handler));
    testrun(!ov_vm_register(res.vm, 0, 0, 0, next_inv_handler));
    testrun(!ov_vm_register(0, 0x02, 0, 0, next_inv_handler));
    testrun(!ov_vm_register(res.vm, 0x02, 0, 0, next_inv_handler));
    testrun(!ov_vm_register(0, 0, "OPCODE02", 0, next_inv_handler));
    testrun(!ov_vm_register(0, 0, 0, next_handler, next_inv_handler));
    testrun(!ov_vm_register(res.vm, 0, 0, next_handler, next_inv_handler));
    testrun(!ov_vm_register(0, 0x02, 0, next_handler, next_inv_handler));
    testrun(!ov_vm_register(res.vm, 0x02, 0, next_handler, next_inv_handler));
    testrun(!ov_vm_register(0, 0, "OPCODE02", next_handler, next_inv_handler));

    testrun(
        !ov_vm_register(res.vm, 0, "OPCODE02", next_handler, next_inv_handler));

    testrun(ov_vm_register(
        res.vm, 0x02, "OPCODE02", next_handler, next_inv_handler));

    testrun(free_resources(res));

    return testrun_log_success();
}

/*****************************************************************************
                                    TRIGGER
 ****************************************************************************/

static int test_ov_vm_trigger() {

    testrun(OV_EXEC_TRIGGER_FAIL == ov_vm_trigger(0, 0, 0, 0));

    resources res = create_resources(0, 0);
    testrun(0 != res.vm);

    testrun(OV_EXEC_TRIGGER_FAIL == ov_vm_trigger(res.vm, 0, 0, 0));

    ov_vm_instr instr[] = {
        {OP_NEXT, 11, 12, 13}, {OP_WAIT, 21, 22, 23}, {OP_END, 0, 0, 0}};

    testrun(OV_EXEC_TRIGGER_FAIL == ov_vm_trigger(0, instr, 0, 0));

    // Should fail because the opcodes are not registered yet
    testrun(OV_EXEC_TRIGGER_FAIL == ov_vm_trigger(res.vm, instr, 0, 0));

    testrun(free_resources(res));

    res = create_resources_w_opcodes_notify_handlers(0, 0);

    testrun(0 != res.vm);

    // char const *ov_vm_trigger(ov_vm *self,
    //                          ov_vm_instr instructions[],
    //                           void *data,
    //                           uint64_t timeout_msecs);

    ov_vm_instr empty[] = {OV_VM_INST_END};
    char const *prog_id = "empty";

    testrun(OV_EXEC_OK == ov_vm_trigger(res.vm, empty, prog_id, 0));

    testrun(handler_state_equals(
        next_state, 0, IGNORE, IGNORE, IGNORE, IGNORE, 0, 0));

    testrun(handler_state_equals(
        wait_state, 0, IGNORE, IGNORE, IGNORE, IGNORE, 0, 0));

    reset_handler_states();
    reset_notify_states();
    reset_release_state();

    // Single step "next"

    ov_vm_instr single_next[] = {{OP_NEXT, 11, 12, 13}, OV_VM_INST_END};
    prog_id = "single_next";

    testrun(OV_EXEC_OK == ov_vm_trigger(res.vm, single_next, prog_id, 0));

    testrun(
        handler_state_equals(next_state, 1, OP_NEXT, 11, 12, 13, prog_id, 0));

    testrun(handler_state_equals(
        wait_state, 0, IGNORE, IGNORE, IGNORE, IGNORE, 0, 0));

    testrun(notify_state_equals(failed_state, 0, 0));
    testrun(notify_state_equals(aborted_state, 0, 0));
    testrun(notify_state_equals(done_state, 1, prog_id));

    // Test invalid opcmode

    // Test abort

    reset_handler_states();
    reset_notify_states();
    reset_release_state();

    int test_data = 42;

    // Abort

    prog_id = "abort";

    ov_vm_instr prog_abort[] = {{OP_NEXT, 11, 12, 13},
                                {OP_NEXT, 14, 15, 16},
                                {OP_ABORT, 31, 32, 33},
                                {OP_END, 0, 0, 0}};

    testrun(OV_EXEC_ERROR ==
            ov_vm_trigger(res.vm, prog_abort, prog_id, &test_data));

    testrun(handler_state_equals(
        next_state, 2, OP_NEXT, 14, 15, 16, prog_id, &test_data));

    testrun(handler_state_equals(
        next_inv_state, 2, OP_NEXT, 11, 12, 13, prog_id, &test_data));

    testrun(handler_state_equals(
        abort_state, 1, OP_ABORT, 31, 32, 33, prog_id, &test_data));

    testrun(notify_state_equals(failed_state, 0, 0));
    testrun(notify_state_equals(aborted_state, 1, prog_id));
    testrun(notify_state_equals(done_state, 0, 0));

    testrun(release_state_equals(1, &test_data));

    reset_handler_states();
    reset_notify_states();
    reset_release_state();

    // Fail during abortion

    prog_id = "fail_during_abort";

    ov_vm_instr fail_during_abort[] = {{OP_NEXT, 11, 12, 13},
                                       {OP_NEXT_INV_ABORTS, 21, 22, 23},
                                       {OP_NEXT, 14, 15, 16},
                                       {OP_ABORT, 31, 32, 33},
                                       {OP_END, 0, 0, 0}};

    reset_handler_states();
    reset_notify_states();
    reset_release_state();

    testrun(OV_EXEC_ERROR ==
            ov_vm_trigger(res.vm, fail_during_abort, prog_id, &test_data));

    testrun(handler_state_equals(
        next_state, 3, OP_NEXT, 14, 15, 16, prog_id, &test_data));

    testrun(handler_state_equals(
        abort_state, 2, OP_NEXT_INV_ABORTS, 21, 22, 23, prog_id, &test_data));

    testrun(handler_state_equals(
        next_inv_state, 1, OP_NEXT, 14, 15, 16, prog_id, &test_data));

    testrun(notify_state_equals(failed_state, 1, prog_id));
    testrun(notify_state_equals(aborted_state, 0, 0));
    testrun(notify_state_equals(done_state, 0, 0));

    testrun(release_state_equals(1, &test_data));

    reset_handler_states();
    reset_notify_states();
    reset_release_state();

    // Test timeout
    ov_vm_instr timeout_prog[] = {{OP_NEXT, 11, 12, 13},
                                  {OP_NEXT, 21, 22, 23},
                                  {OP_WAIT, 14, 15, 16},
                                  {OP_END, 0, 0, 0}};

    // This one should time out
    testrun(OV_EXEC_WAIT ==
            ov_vm_trigger(res.vm, timeout_prog, "timeout", &test_data));

    res.loop->run(res.loop, TIMEOUT_VM_MSECS * 1000 * 3);

    testrun(handler_state_equals(
        next_state, 2, OP_NEXT, 21, 22, 23, "timeout", &test_data));

    testrun(handler_state_equals(
        next_inv_state, 2, OP_NEXT, 11, 12, 13, "timeout", &test_data));

    testrun(notify_state_equals(failed_state, 0, 0));
    testrun(notify_state_equals(aborted_state, 1, "timeout"));
    testrun(notify_state_equals(done_state, 0, 0));

    testrun(release_state_equals(1, &test_data));

    free_resources(res);

    reset_handler_states();
    reset_notify_states();
    reset_release_state();

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_vm_continue() {

    testrun(OV_EXEC_ERROR == ov_vm_continue(0, 0));

    resources res = create_resources_with_custom_opcodes(0, 0);

    testrun(OV_EXEC_ERROR == ov_vm_continue(res.vm, 0));

    char const *our_id = "Skaftafell";

    testrun(OV_EXEC_ERROR == ov_vm_continue(res.vm, our_id));

    // Now trigger real program

    char const *prog_id = "husavik";

    ov_vm_instr instr[] = {{OP_NEXT, 11, 12, 13},
                           {OP_WAIT, 21, 22, 23},
                           {OP_NEXT, 14, 15, 16},
                           {OP_END, 0, 0, 0}};

    reset_handler_state(&next_state);
    reset_handler_state(&wait_state);

    testrun(OV_EXEC_WAIT == ov_vm_trigger(res.vm, instr, prog_id, 0));

    testrun(
        handler_state_equals(next_state, 1, OP_NEXT, 11, 12, 13, prog_id, 0));

    testrun(
        handler_state_equals(wait_state, 1, OP_WAIT, 21, 22, 23, prog_id, 0));

    testrun(OV_EXEC_ERROR == ov_vm_continue(res.vm, "akureiri"));
    testrun(OV_EXEC_OK == ov_vm_continue(res.vm, prog_id));

    testrun(
        handler_state_equals(next_state, 2, OP_NEXT, 14, 15, 16, prog_id, 0));

    testrun(handler_state_equals(wait_state, 1, OP_WAIT, 21, 22, 23, 0, 0));

    testrun(OV_EXEC_ERROR == ov_vm_continue(res.vm, "akureiri"));
    testrun(OV_EXEC_ERROR == ov_vm_continue(res.vm, prog_id));

    free_resources(res);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_vm_abort() {

    testrun(!ov_vm_abort(0, 0));

    resources res = create_resources_with_custom_opcodes(0, 0);

    testrun(!ov_vm_abort(res.vm, 0));

    char const *our_id = "Skaftafell";

    testrun(!ov_vm_abort(res.vm, our_id));

    // Now trigger real program

    char const *prog_id = "husavik";

    ov_vm_instr instr[] = {{OP_NEXT, 11, 12, 13},
                           {OP_NEXT_2, 21, 22, 23},
                           {OP_NEXT, 14, 15, 16},
                           {OP_NEXT_NO_INV, 41, 42, 43},
                           {OP_WAIT, 31, 32, 33},
                           {OP_NEXT, 17, 15, 16},
                           {OP_END, 0, 0, 0}};

    reset_handler_states();

    testrun(OV_EXEC_WAIT == ov_vm_trigger(res.vm, instr, prog_id, 0));

    testrun(handler_state_equals(
        next_state, 3, OP_NEXT_NO_INV, 41, 42, 43, prog_id, 0));

    testrun(handler_state_equals(
        next_2_state, 1, OP_NEXT_2, 21, 22, 23, prog_id, 0));

    testrun(
        handler_state_equals(wait_state, 1, OP_WAIT, 31, 32, 33, prog_id, 0));

    testrun(handler_state_equals(
        next_inv_state, 0, IGNORE, IGNORE, IGNORE, IGNORE, 0, 0));

    testrun(handler_state_equals(
        next_2_inv_state, 0, IGNORE, IGNORE, IGNORE, IGNORE, 0, 0));

    testrun(handler_state_equals(
        wait_inv_state, 0, IGNORE, IGNORE, IGNORE, IGNORE, 0, 0));

    testrun(notify_state_equals(failed_state, 0, 0));
    testrun(notify_state_equals(aborted_state, 0, 0));
    testrun(notify_state_equals(done_state, 0, 0));

    reset_handler_states();

    testrun(!ov_vm_abort(res.vm, "akureiri"));
    testrun(ov_vm_abort(res.vm, prog_id));

    testrun(handler_state_equals(
        next_state, 0, IGNORE, IGNORE, IGNORE, IGNORE, 0, 0));

    testrun(handler_state_equals(
        next_2_state, 0, IGNORE, IGNORE, IGNORE, IGNORE, 0, 0));

    testrun(handler_state_equals(
        wait_state, 0, IGNORE, IGNORE, IGNORE, IGNORE, 0, 0));

    testrun(handler_state_equals(
        next_inv_state, 2, OP_NEXT, 11, 12, 13, prog_id, 0));

    testrun(handler_state_equals(
        next_2_inv_state, 1, OP_NEXT_2, 21, 22, 23, prog_id, 0));

    // The current instruction must not be reverted!
    testrun(handler_state_equals(
        wait_inv_state, 0, IGNORE, IGNORE, IGNORE, IGNORE, 0, 0));

    testrun(notify_state_equals(failed_state, 0, 0));
    testrun(notify_state_equals(aborted_state, 0, 0));
    testrun(notify_state_equals(done_state, 0, 0));

    testrun(!ov_vm_abort(res.vm, "akureiri"));
    testrun(!ov_vm_abort(res.vm, prog_id));

    testrun(OV_EXEC_ERROR == ov_vm_continue(res.vm, "akureiri"));
    testrun(OV_EXEC_ERROR == ov_vm_continue(res.vm, prog_id));

    free_resources(res);

    // Same again, but with abort-handler

    reset_handler_states();

    res = create_resources_w_opcodes_notify_handlers(0, 0);

    int test_data = 42;
    testrun(OV_EXEC_WAIT == ov_vm_trigger(res.vm, instr, prog_id, &test_data));

    testrun(handler_state_equals(
        next_state, 3, OP_NEXT_NO_INV, 41, 42, 43, prog_id, &test_data));

    testrun(handler_state_equals(
        next_2_state, 1, OP_NEXT_2, 21, 22, 23, prog_id, &test_data));

    testrun(handler_state_equals(
        wait_state, 1, OP_WAIT, 31, 32, 33, prog_id, &test_data));

    testrun(handler_state_equals(
        next_inv_state, 0, IGNORE, IGNORE, IGNORE, IGNORE, 0, 0));

    testrun(handler_state_equals(
        next_2_inv_state, 0, IGNORE, IGNORE, IGNORE, IGNORE, 0, 0));

    testrun(handler_state_equals(
        wait_inv_state, 0, IGNORE, IGNORE, IGNORE, IGNORE, 0, 0));

    testrun(notify_state_equals(failed_state, 0, 0));
    testrun(notify_state_equals(aborted_state, 0, 0));
    testrun(notify_state_equals(done_state, 0, 0));

    testrun(release_state_equals(0, 0));

    reset_handler_states();

    testrun(!ov_vm_abort(res.vm, "akureiri"));
    testrun(ov_vm_abort(res.vm, prog_id));

    testrun(handler_state_equals(
        next_state, 0, IGNORE, IGNORE, IGNORE, IGNORE, 0, 0));

    testrun(handler_state_equals(
        next_2_state, 0, IGNORE, IGNORE, IGNORE, IGNORE, 0, 0));

    testrun(handler_state_equals(
        wait_state, 0, IGNORE, IGNORE, IGNORE, IGNORE, 0, 0));

    testrun(handler_state_equals(
        next_inv_state, 2, OP_NEXT, 11, 12, 13, prog_id, &test_data));

    testrun(handler_state_equals(
        next_2_inv_state, 1, OP_NEXT_2, 21, 22, 23, prog_id, &test_data));

    testrun(handler_state_equals(
        wait_inv_state, 0, IGNORE, IGNORE, IGNORE, IGNORE, 0, 0));

    testrun(notify_state_equals(failed_state, 0, 0));
    testrun(notify_state_equals(aborted_state, 1, prog_id));
    testrun(notify_state_equals(done_state, 0, 0));

    testrun(release_state_equals(1, &test_data));

    testrun(!ov_vm_abort(res.vm, "akureiri"));
    testrun(!ov_vm_abort(res.vm, prog_id));

    testrun(OV_EXEC_ERROR == ov_vm_continue(res.vm, "akureiri"));
    testrun(OV_EXEC_ERROR == ov_vm_continue(res.vm, prog_id));

    free_resources(res);

    reset_handler_states();
    reset_notify_states();
    reset_release_state();

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_vm_data_for() {

    testrun(0 == ov_vm_data_for(0, 0));

    resources res = create_resources_with_custom_opcodes(0, 0);
    testrun(0 != res.vm);
    testrun(0 != res.loop);

    testrun(0 == ov_vm_data_for(res.vm, 0));
    testrun(0 == ov_vm_data_for(res.vm, "aBc"));

    char const *our_id = "asbyrgi";
    char const *other_id = "thorsmoerk";

    testrun(0 == ov_vm_data_for(res.vm, our_id));

    ov_vm_instr instr[] = {{OP_WAIT, 21, 22, 23}, {OP_END, 0, 0, 0}};

    int our_data = 1705;
    int other_data = 1715;

    testrun(ov_vm_trigger(res.vm, instr, our_id, &our_data));

    testrun(&our_data == ov_vm_data_for(res.vm, our_id));

    testrun(&our_data == ov_vm_data_for(res.vm, our_id));
    testrun(0 == ov_vm_data_for(res.vm, other_id));

    testrun(ov_vm_trigger(res.vm, instr, other_id, &other_data));

    testrun(&our_data == ov_vm_data_for(res.vm, our_id));
    testrun(&other_data == ov_vm_data_for(res.vm, other_id));

    free_resources(res);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_vm",
            test_ov_vm_create,
            test_ov_vm_free,
            test_ov_vm_register,
            test_ov_vm_trigger,
            test_ov_vm_continue,
            test_ov_vm_abort,
            test_ov_vm_data_for);

/*----------------------------------------------------------------------------*/
