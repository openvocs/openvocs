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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_VM_PROG_H
#define OV_VM_PROG_H
/*----------------------------------------------------------------------------*/

#include <ov_base/ov_result.h>
#include <stdint.h>

/*----------------------------------------------------------------------------*/

#define OV_VM_PROG_ID_MAX_LEN 255

/*****************************************************************************
                                     TYPES
 ****************************************************************************/

typedef struct {

    uint8_t opcode;
    uint8_t args[3];
    // char desc[255];

} ov_vm_instr;

/*----------------------------------------------------------------------------*/

typedef enum {

    OP_END = 0,
    OP_NOP = 0xfd,
    OP_INVALID = 0xfe,

} ov_vm_reserved_opcodes;

/*----------------------------------------------------------------------------*/

typedef enum {
    OV_VM_PROG_OK = 0,
    OV_VM_PROG_ABORTING,
    OV_VM_PROG_FAILED_TO_ABORT,
    OV_VM_PROG_INVALID,
} ov_vm_prog_status;

/*----------------------------------------------------------------------------*/

typedef struct ov_vm_prog ov_vm_prog;

/*****************************************************************************
                                   FUNCTIONS
 ****************************************************************************/

char const *ov_vm_prog_status_to_string(ov_vm_prog_status status);

ov_vm_prog_status ov_vm_prog_state(ov_vm_prog const *prog);

void ov_vm_prog_state_set(ov_vm_prog *prog, int new_state);

/*----------------------------------------------------------------------------*/

bool ov_vm_prog_instructions_set(ov_vm_prog *prog, ov_vm_instr const *instr);

ov_vm_instr const *ov_vm_prog_instructions(ov_vm_prog const *prog);

ov_vm_instr ov_vm_prog_current_instr(ov_vm_prog const *prog);

/*----------------------------------------------------------------------------*/

char const *ov_vm_prog_id(ov_vm_prog const *prog);

bool ov_vm_prog_id_set(ov_vm_prog *prog, char const *id);

/*----------------------------------------------------------------------------*/

void *ov_vm_prog_data(ov_vm_prog *prog);

bool ov_vm_prog_data_set(ov_vm_prog *prog, void *data);

/*----------------------------------------------------------------------------*/

ov_result ov_vm_prog_result(ov_vm_prog const *prog);

void ov_vm_prog_result_set(ov_vm_prog *prog, int err_code, char const *msg);

void ov_vm_prog_log_result(ov_vm_prog *prog);

/*----------------------------------------------------------------------------*/

ssize_t ov_vm_prog_program_counter(ov_vm_prog const *prog);

bool ov_vm_prog_program_counter_reset(ov_vm_prog *prog);

void ov_vm_prog_propagate_program_counter(ov_vm_prog *prog);

/*----------------------------------------------------------------------------*/

typedef enum {

    OC_ERROR = -1,
    OC_NEXT,
    OC_FINISHED,
    OC_WAIT_AND_NEXT,
    OC_WAIT_AND_REPEAT, // Wait, then repeat step (NOT allowed in reversed
                        // order)
} ov_vm_retval;

bool ov_vm_prog_set_last_instr_retval(ov_vm_prog *prog, ov_vm_retval retval);

ov_vm_retval ov_vm_prog_get_last_instr_retval(ov_vm_prog const *prog);

/*----------------------------------------------------------------------------*/

/**
 * Type that is big enough to keep an ov_vm_prog structure
 */
typedef uint8_t ov_vm_prog_mem[OV_VM_PROG_ID_MAX_LEN + 56 + 1];

/*----------------------------------------------------------------------------*/

bool ov_vm_prog_reset_prog_started(ov_vm_prog *prog);

/*----------------------------------------------------------------------------*/
#endif
