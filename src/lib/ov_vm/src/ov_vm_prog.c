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

#include "ov_vm_prog.h"
#include <ov_base/ov_string.h>
#include <ov_base/ov_trace.h>

/*----------------------------------------------------------------------------*/

char const *ov_vm_prog_status_to_string(ov_vm_prog_status status) {

    switch (status) {

    case OV_VM_PROG_OK:
        return "OK";

    case OV_VM_PROG_ABORTING:
        return "ABORTING";

    case OV_VM_PROG_FAILED_TO_ABORT:
        return "FAILED_TO_ABORT";

    case OV_VM_PROG_INVALID:
        return "INVALID";

    default:
        OV_ASSERT(!"MUST NEVER HAPPEN");
        return "INVALID";
    };
}

/*****************************************************************************
                                 prog structure
 ****************************************************************************/

struct ov_vm_prog {

    /**
     * Identifier for that program.
     * Might be used to associate e.g. signaling messages with this program.
     */
    char id[OV_VM_PROG_ID_MAX_LEN];

    ov_vm_instr const *instructions;
    void *data;

    ssize_t program_counter;

    ov_vm_prog_status state;

    ov_result error;

    int32_t last_instr_retval;

    int prog_started_epoch_secs;
};

/*----------------------------------------------------------------------------*/

_Static_assert(sizeof(struct ov_vm_prog) <= sizeof(ov_vm_prog_mem),
               "ov_vm_prog_mem smaller than ov_vm_prog");

/*----------------------------------------------------------------------------*/

ov_vm_prog_status ov_vm_prog_state(ov_vm_prog const *prog) {

    if (0 == prog) {
        return OV_VM_PROG_INVALID;
    } else {
        return prog->state;
    }
}

/*----------------------------------------------------------------------------*/

void ov_vm_prog_state_set(ov_vm_prog *prog, int new_state) {
    if (0 != prog) {
        OV_ASSERT(-1 < new_state);
        OV_ASSERT(new_state < 1 + OV_VM_PROG_INVALID);
        prog->state = new_state;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_vm_prog_instructions_set(ov_vm_prog *prog, ov_vm_instr const *instr) {

    if (0 != prog) {
        prog->instructions = instr;
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

ov_vm_instr const *ov_vm_prog_instructions(ov_vm_prog const *prog) {

    if (0 != prog) {
        return prog->instructions;
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

ov_vm_instr ov_vm_prog_current_instr(ov_vm_prog const *prog) {

    if (0 != prog) {
        return prog->instructions[prog->program_counter];
    } else {
        return (ov_vm_instr){
            .opcode = OP_INVALID,
        };
    }
}

/*----------------------------------------------------------------------------*/

char const *ov_vm_prog_id(ov_vm_prog const *prog) {

    if (0 != prog) {
        return prog->id;
    } else {
        return "INVALID_PROGRAM";
    }
}

/*----------------------------------------------------------------------------*/

bool ov_vm_prog_id_set(ov_vm_prog *prog, char const *id) {

    if (0 != prog) {
        strncpy(prog->id, id, OV_VM_PROG_ID_MAX_LEN);
        prog->id[OV_VM_PROG_ID_MAX_LEN - 1] = 0;
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

void *ov_vm_prog_data(ov_vm_prog *prog) {

    if (0 != prog) {
        return prog->data;
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_vm_prog_data_set(ov_vm_prog *prog, void *data) {

    if (0 != prog) {
        prog->data = data;
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

ov_result ov_vm_prog_result(ov_vm_prog const *prog) {

    ov_result res = {0};

    if (0 != prog) {
        res = prog->error;
    } else {
        res = (ov_result){
            .error_code = OV_ERROR_INVALID_RESULT,
            .message = "Could not get result of program - program is invalid",
        };
    }

    if (0 == res.message) {
        res.message = "No Message";
    }

    return res;
}

/*----------------------------------------------------------------------------*/

void ov_vm_prog_result_set(ov_vm_prog *prog, int err_code, char const *msg) {

    if (0 != prog) {
        ov_result_set(&prog->error, err_code, msg);
    }
}

/*----------------------------------------------------------------------------*/

void ov_vm_prog_log_result(ov_vm_prog *prog) {

    ov_result res = ov_vm_prog_result(prog);

    switch (res.error_code) {

    case OV_ERROR_NOERROR:
        ov_log_info("Program %s stopped - result is %s (%" PRIu64 ")",
                    ov_vm_prog_id(prog), ov_string_sanitize(res.message),
                    res.error_code);
        break;

    case OV_ERROR_INVALID_RESULT:
        ov_log_error("Program %s invalid", ov_vm_prog_id(prog));
        break;

    default:

        ov_log_info("Program %s failed - result is %s (%" PRIu64 ")",
                    ov_vm_prog_id(prog), ov_string_sanitize(res.message),
                    res.error_code);
        break;
    };
}

/*----------------------------------------------------------------------------*/

ssize_t ov_vm_prog_program_counter(ov_vm_prog const *prog) {

    if (0 != prog) {
        return prog->program_counter;
    } else {
        return -1;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_vm_prog_program_counter_reset(ov_vm_prog *prog) {

    if (0 != prog) {
        prog->program_counter = 0;
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

void ov_vm_prog_propagate_program_counter(ov_vm_prog *prog) {

    if (OV_TRACE_ACTIVE && (0 != prog)) {

        int now = time(0);

        prog->prog_started_epoch_secs =
            OV_OR_DEFAULT(prog->prog_started_epoch_secs, now);
        OV_TRACE_TIMEOUT(prog->prog_started_epoch_secs, now, prog->id);
        prog->prog_started_epoch_secs = now;
    }

    switch (ov_vm_prog_state(prog)) {

    case OV_VM_PROG_OK:

        ++prog->program_counter;
        break;

    case OV_VM_PROG_ABORTING:

        --prog->program_counter;
        break;

    case OV_VM_PROG_FAILED_TO_ABORT:
    case OV_VM_PROG_INVALID:
        ov_log_warning("Cannot propagate program counter: Program invalid");
        // OV_ASSERT(!"NEVER TO HAPPEN");
    }
}

/*----------------------------------------------------------------------------*/

bool ov_vm_prog_set_last_instr_retval(ov_vm_prog *prog, ov_vm_retval retval) {

    if (!ov_ptr_valid(prog, "No progam given")) {

        return false;

    } else {

        prog->last_instr_retval = retval;
        return true;
    }
}

/*----------------------------------------------------------------------------*/

ov_vm_retval ov_vm_prog_get_last_instr_retval(ov_vm_prog const *prog) {

    if (!ov_ptr_valid(prog, "No progam given")) {

        return OC_ERROR;

    } else {

        return prog->last_instr_retval;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_vm_prog_reset_prog_started(ov_vm_prog *prog) {

    if (!ov_ptr_valid(prog, "No progam given")) {

        return false;

    } else {

        prog->prog_started_epoch_secs = 0;
        return true;
    }
}

/*----------------------------------------------------------------------------*/
