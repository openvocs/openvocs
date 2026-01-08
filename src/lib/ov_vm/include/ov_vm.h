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

        A VM to process command sequences ('programs') asynchronously.
        This engine is, although asynchron, NOT PARALLEL!

        If you want to use it in a multithreaded context, ensure that
        accesses to this engine are serialized by some means (mutexes e.g.).

        Basically, this is a small virtual machine:

        You define a program, i.e. a sequence of opcodes/args to process:

           opcodes
            |    arguments
            |     |  |  |
        {   V     V  V  V
           {0x15, 0, 0, 0}, <- instruction 1
           {0x12, 3, 1, 1}, <- instruction 2
           ...
        }

        Each line is an 'instruction'.
        The first column in each instruction is the 'opcode'.

        You can trigger the execution of such a program by

        ```
        ov_cntl_instr instructions[] = {
          {0x14, 0, 1, 0},
          {0x1, 2, 2, 2},
          {OP_END, 0, 0, 0}}; <- THIS LINE IS MANADORY AT THE END OF EACH PROG

        char const our_data[] = "Additional data";

        ov_vm_trigger(vm, instructions, &our_data);
        ```

        The program `instructions` will then be processed.
        Note: Each program sequence needs to terminate with an
        `{OP_END, 0, 0, 0}` line.

        Each opcode triggers the execution of an handler.
        A handler is just a method associated with an opcode.
        See `ov_cntl_instr_handler` .

        Apart from `OP_END` and OP_NOP there are no predefined opcodes.

        That is, the user needs to register his own opcodes / handlers.
        See `ov_vm_register` .

        The return value of a handler determines how the vm proceeds:

        `OC_NEXT` :
            Proceed with next instruction.
        `OV_FINISHED`  :
            Sequence done.
        `OC_WAIT_AND_NEXT` :
            Interrupt program and return. Continues with next instruction when
            'ov_vm_continue` is called for the particular program.
        `OC_WAIT_AND_REPEAT` :
            Like `OC_WAIT_AND_NEXT`, but repeat same instruction again.
        `OC_ERROR`:
            Error occured. Instruction sequence is aborted.

        Each program got a timeout. If the sequence has not finished within
        this timeout, it is aborted. (The timeout can be specified when 
        triggering the program).

        Beware: It is guaranteed that a program is not aborted *before* the
        timeout expires. It is not guaranteed how long after the expiration
        the program remains active.

        # Instructions

        Most of the relevant stuff has been said already above.

        The arguments are there to 'individualize' each instruction.
        E.g. if you got an opcode `ADD_DEST`, and your data resembles
        something like {mixer, multiplexer} , you might want to specify
        whom to send an `add_destination` request:

        {ADD_DEST, 0, 0, 0}  -> Send add_destination to mixer
        {ADD_DEST, 1, 0, 0}  -> Send add_destination to multiplexer

        Since your handler receives the args, it can distinguish:

        ```
        ov_cntl_retval add_dest(ov_vm *self,
                              ov_cntl_instr instr,
                              void *data) {

            ...
                our_data *od = as_our_data(data);

                switch(instr.args[0]) {
                    case 0:
                        target = od->mixer;
                        break;
                    case 1:
                        target = od->multiplexer;
                        break;
                    default:
                        nuke_everything();
                };

            ...
        ```

        Why 3?

        During resmgr development, it turned out that 3 is a sweet spot -
        not too many to become messy, not too few to properly use them.

        # Triggering a program

        Sequences are triggered by

        `ov_vm_trigger(ov_vm *, ov_cntl_instr[], void *data,
        uint64_t timeout_msecs)`

        The `data` is just some arbitrary pointer which is handed over to
        each handler in order to allow for passing arbitrary data to the
        handlers.

        When the program is finished, ov_vm calls its `release_data`
        callback on `data` (see ov_vm_config / ov_vm_create ).
        It is up to you what the callback does, but usually it will perform
        some free / release of some sort.

        But how you deal with the data pointer, is up to you.

        However, if your callback releases the data, DO NOT use the data
        pointer ONCE you triggered ov_vm_trigger!

        When triggered, each program is given a unique UUID.
        This UUID is passed to each handler and should be used to associate
        I/O to the request.
        E.g. if a signaling message is created in a handler, the message
        should receive this UUID as its message UUID.

        # Aborting a program

        If an instruction fails or a program has reached its timeout,
        it is aborted.

        Abortion is done by reversing all instructions done this far in reverse
        order.
        A single instruction is reversed by replacing its opcode by the reverse
        opcode, then executing the instruction again.
        The reverse of an opcode has to be given when the opcode is registered.
        (OP_NOP can be employed if the instruction shall not be reverted).

        When done, `notify_program_aborted` is called.

        # Waiting for certain events (e.g. I/O)

        If the program should continue only after some other event,
        the handler that wants the program to wait must return either
            `OC_WAIT_AND_NEXT` or `OC_WAIT_AND_REPEAT` .

        The VM now returns to the caller.

        2 things now might happen:

        - The program runs into timeout -> aborted
        - Somebody calls ov_vm_continue -> request continues

        Example:

        Assume the handler for `S2` will return OV_WAIT_AND_NEXT`.

        A program is triggered:

        ```id = ov_vm_trigger(..., { {S1,0 ,0 ,0},
                                     {S2, 0, 0, 0},
                                     {S3, 0, 0, 0}, ...)```
            -> Instruction `S1` is executed, then `S2`. The handler of `S2`
                returns OC_WAIT_AND_NEXT`
            -> `ov_vm_trigger` returns
        Some other part of the program calls `ov_vm_continue(vm, id, true)`
            -> instruction S3 and all the following are executed.
            -> When `OP_END` is reached, or another handler returns `OC_WAIT*`
               `ov_vm_continue` returns again.

        ------------------------------------------------------------------------
*/
#ifndef OV_CONTROLLER_H
#define OV_CONTROLLER_H
/*----------------------------------------------------------------------------*/

#include <ov_base/ov_event_loop.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ov_vm_prog.h"

/*----------------------------------------------------------------------------*/

extern ov_vm_instr OV_VM_INST_END;

/*----------------------------------------------------------------------------*/

typedef struct ov_vm_struct ov_vm;

typedef struct {

    size_t max_requests;

    uint64_t default_program_timeout_msecs;

    /**
     * Callback to call on `data` after a program has finished
     */
    void (*release_data)(ov_vm *, void *);

    void (*notify_program_failed_to_abort)(ov_vm *, char const *id);
    void (*notify_program_aborted)(ov_vm *, char const *id);
    void (*notify_program_done)(ov_vm *, char const *id);

} ov_vm_config;

ov_vm *ov_vm_create(ov_vm_config cfg, ov_event_loop *loop);

ov_vm *ov_vm_free(ov_vm *self);

/*----------------------------------------------------------------------------*/

uint64_t ov_vm_program_timeout_msecs(ov_vm const *self);

bool ov_vm_program_set_timeout_msecs(ov_vm *self, uint64_t timeout_msecs);

/*****************************************************************************
                              OPCODE REGISTRATION
 ****************************************************************************/

/**
 * Handler to execute for a particular opcode.
 * The handler can query/alter the prog state its the getters/setters (see
 * ov_vm_prog.h) The hander should set the result of the program at least in
 * case of error
 */
typedef ov_vm_retval (*ov_vm_opcode_handler)(ov_vm *, ov_vm_prog *prog);

/*----------------------------------------------------------------------------*/

bool ov_vm_register(ov_vm *self, uint8_t opcode, char const *symbol,
                    ov_vm_opcode_handler handler,
                    ov_vm_opcode_handler inv_handler);

/*----------------------------------------------------------------------------*/

typedef enum {

    OV_EXEC_OK /* Program succeeded */,
    OV_EXEC_TRIGGER_FAIL /* Triggering a program did not succeed */,
    OV_EXEC_ERROR /* Program failed (but was successfully triggered) */,
    OV_EXEC_WAIT /* Program requires external action and continuing via
                    ov_vm_continue */

} ov_vm_exec_result;

/**
 * Trigger a program.
 * @param data Arbitrary data handed over to your program.
 * Beware if this function returns anything but OV_EXEC_TRIGGER_FAIL:
 * If you set a `release_data` callback on your VM,
 * and this `release_data` frees `data` (as it is intended), then once you
 * passed `data` to `ov_vm_trigger`, do not touch it afterwards!
 * If this function returns OV_EXEC_TRIGGER_FAIL, you still must take care
 * of `data`.
 */
ov_vm_exec_result ov_vm_trigger(ov_vm *self, ov_vm_instr const *instructions,
                                char const *id, void *data);

/*----------------------------------------------------------------------------*/

/**
 * Continue a program that has been stopped by a handler returning OC_WAIT*
 */
ov_vm_exec_result ov_vm_continue(ov_vm *self, char const *id);

/**
 * Aborts a program.
 * A program being aborted will never wait for anything.
 * Wrapper for ov_vm_abort_with() .
 * For optional parameters, see ov_vm_abort_opts.
 */
#define ov_vm_abort(self, id, ...)                                             \
    ov_vm_abort_with(self, id, (ov_vm_abort_opts){__VA_ARGS__})

/*----------------------------------------------------------------------------*/

typedef struct {

    bool finish_current_step : 1;

} ov_vm_abort_opts;

bool ov_vm_abort_with(ov_vm *self, char const *id, ov_vm_abort_opts opts);

/*----------------------------------------------------------------------------*/

/**
 * Add an alias for a program.
 * The alias will behave exactly as the ID of the program, i.e.
 * every call that expects an ID can receive an alias for the same program
 * instead.
 *
 * Example:
 *
 *      ov_vm_trigger(our_vm, instructions, "OurProgram", 0);
 *      ...
 *      // Somewhere in a callback:
 *      ...
 *      ov_vm_alias(our_vm, prog, "Alias");
 *      ...
 *      // Now the program "OurProgram" can also be addressed by "Alias":
 *
 *      ov_vm_abort(our_vm, "OurProgram");
 *      ov_vm_abort(our_vm, "Alias");
 *
 *      do the same.
 */
bool ov_vm_alias(ov_vm *self, ov_vm_prog *prog, char const *new_id);

/*----------------------------------------------------------------------------*/

/**
 * Get the data for a specific program
 */
void *ov_vm_data_for(ov_vm *self, char const *id);

/*----------------------------------------------------------------------------*/

/**
 * Iterate over all currently active programs and call `process` on each
 * of them.
 * The iteration is stopped and the function returns when either there are
 * no more programs or `process` returns false (in both cases this function
 * returns with true).
 * @return false if this function was called the wrong way (e.g. with 0 for
 * self)
 */
bool ov_vm_for_each_program(ov_vm const *self,
                            bool (*process)(char const *id,
                                            ov_vm_prog const *prog,
                                            uint64_t start_time_epoch_usecs,
                                            void *data),
                            void *data);

/*----------------------------------------------------------------------------*/
#endif
