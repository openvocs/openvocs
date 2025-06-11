/***

Copyright   2018        German Aerospace Center DLR e.V.,
                        German Space Operations Center (GSOC)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    This file is part of the openvocs project. http://openvocs.org

***/ /**

     \file               ov_rtp_frame_buffer.c
     @author         Michael Beer
     \date               2018-08-22

     \ingroup            empty

     \brief              empty

     Internally, the frame buffer consists of a list of stages, each
     stage containing a set of RTP frames which have distinct SSIDs,
     sorted from the lowest SSID to the highest SSID.

     It looks like that:

     stages: list [

     stage 0 (oldest frames / smallest sequence numbers) [frame with lowest
 SSID, ..., frame with highest SSID]
     stage 1 (younger frames / higher sequence numbers)  [frame with lowest
 SSID, ..., frame with highest SSID]
     ...
     stage COUNT (youngest frames / highest sequence numbers) [frame with lowest
 SSID, ..., frame with highest SSID]

     ]

 **/
/*-------------------------------------------------------------------------*/

#include "../../include/ov_utils.h"

#include <ov_log/ov_log.h>

#include "../../include/ov_linked_list.h"
#include "../../include/ov_utils.h"

#include "../../include/ov_rtp_frame_buffer.h"

/******************************************************************************
 *                                 CONSTANTS
 ******************************************************************************/

static size_t const NUM_FRAMES_TO_BUFFER_DEFAULT = 2;

static uint32_t const MAGIC_NUMBER = 0x52664200;

/* Marker address - the address of ALREADY_PRESENT cannot be occupied by any
 * object, since ALREADY_PRESENT is there ... */
static void const *ALREADY_PRESENT = &ALREADY_PRESENT;

/******************************************************************************
 *                               INTERNAL TYPES
 ******************************************************************************/

typedef struct {

    ov_rtp_frame_buffer public;

    /* a list of hashes */
    ov_list *stages;
    ov_rtp_frame_buffer_config config;

} internal_frame_buffer;

/******************************************************************************
 *                             INTERNAL FUNCTIONS
 ******************************************************************************/

static inline internal_frame_buffer *as_internal_frame_buffer(void *arg) {

    if (0 == arg) return 0;
    uint32_t *magic_number = arg;
    if (MAGIC_NUMBER != *magic_number) return 0;

    return arg;
}

/*----------------------------------------------------------------------------*/

static inline ov_list *add_stage(ov_list *restrict stages,
                                 size_t const MAX_STAGES) {

    OV_ASSERT(stages);
    ov_list *new_stage = 0;

    if (MAX_STAGES <= stages->count(stages)) {
        return 0;
    }

    new_stage = ov_linked_list_create((ov_list_config){0});

    stages->push(stages, new_stage);

    return new_stage;
}

/******************************************************************************
 *                                INSET STUFF
 ******************************************************************************/

static inline ov_rtp_frame *insert_into_stage(ov_list *stage,
                                              ov_rtp_frame *frame) {

    OV_ASSERT(0 != stage);
    OV_ASSERT(0 != frame);

    uint32_t const SSID = frame->expanded.ssrc;
    uint32_t const SEQUENCE_NUMBER = frame->expanded.sequence_number;

    void *iter = stage->iter(stage);
    ov_rtp_frame *current = 0;

    for (size_t i = 1; 0 != iter; ++i) {

        iter = stage->next(stage, iter, (void **)&current);

        if (0 == current) {
            continue;
        }

        uint32_t const CURRENT_SSID = current->expanded.ssrc;

        if (SSID > CURRENT_SSID) {
            continue;
        }

        OV_ASSERT(SSID <= CURRENT_SSID);

        if (SSID < CURRENT_SSID) {

            stage->insert(stage, i, frame);
            return 0;
        }

        OV_ASSERT(SSID == CURRENT_SSID);

        uint32_t const CURRENT_SEQUENCE_NUMBER =
            current->expanded.sequence_number;

        if (SEQUENCE_NUMBER > CURRENT_SEQUENCE_NUMBER) {

            /* We cannot insert our frame here */
            return frame;
        }

        OV_ASSERT(SEQUENCE_NUMBER <= CURRENT_SEQUENCE_NUMBER);

        if (SEQUENCE_NUMBER == CURRENT_SEQUENCE_NUMBER) {

            return (ov_rtp_frame *)ALREADY_PRESENT;
        }

        OV_ASSERT(SEQUENCE_NUMBER < CURRENT_SEQUENCE_NUMBER);

        if (SEQUENCE_NUMBER < CURRENT_SEQUENCE_NUMBER) {

            /* Replace current frame with out frame */
            if (stage->set(stage, i, frame, (void **)&current)) return current;
        }
    }

    stage->push(stage, frame);
    return 0;
}

/*----------------------------------------------------------------------------*/

static inline ov_rtp_frame *insert_into_stages(ov_list *stages,
                                               size_t const MAX_STAGES,
                                               ov_rtp_frame *frame) {

    OV_ASSERT(0 != stages);
    OV_ASSERT(0 != frame);

    ov_rtp_frame *remainder = frame;

    size_t const COUNT = stages->count(stages);

    if ((0 == COUNT) && (0 == add_stage(stages, MAX_STAGES))) {
        return 0;
    }

    OV_ASSERT(0 != remainder);

    void *iter = stages->iter(stages);
    ov_list *stage = 0;

    while (0 != iter) {

        iter = stages->next(stages, iter, (void **)&stage);
        if (0 == stage) break;

        OV_ASSERT(0 != stage);
        OV_ASSERT(0 != remainder);

        ov_rtp_frame *retval = insert_into_stage(stage, remainder);

        if (0 == retval) {

            /* Successful inserted */
            return 0;
        }

        OV_ASSERT(0 != retval);

        if (ALREADY_PRESENT == retval) {
            /* Same frame already present */
            return (ov_rtp_frame *)ALREADY_PRESENT;
        }

        remainder = retval;
        OV_ASSERT(0 != remainder);
        /* Proceed since we might be able to insert the remainder into
         * another stage */
    };

    return remainder;
}

/*----------------------------------------------------------------------------*/

static inline ov_rtp_frame *drop_frame_from_stage(ov_list *stage,
                                                  uint32_t const SSID) {
    OV_ASSERT(stage);

    void *iter = stage->iter(stage);
    ov_rtp_frame *frame = 0;

    for (size_t i = 1; 0 != iter; ++i) {

        iter = stage->next(stage, iter, (void **)&frame);

        uint32_t const CURRENT_SSID = frame->expanded.ssrc;
        if (SSID == CURRENT_SSID) {
            return stage->remove(stage, i);
        }

        if (SSID < CURRENT_SSID) {
            /* Since the list is ordered, no SSID found */
            return 0;
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------*/

static inline ov_rtp_frame *drop_oldest_frame(ov_list *stages,
                                              ov_rtp_frame const *frame) {

    /* This function is only called when all stages contain
     * a frame with frame->expanded.ssrc */
    OV_ASSERT(0 != stages);
    OV_ASSERT(0 != frame);

    uint32_t const SSID = frame->expanded.ssrc;
    size_t const COUNT = stages->count(stages);

    if (0 == COUNT) goto error;

    ov_list *prev_stage = 0;
    void *stage_iter = stages->iter(stages);

    if (0 == stage_iter) return 0;

    stage_iter = stages->next(stages, stage_iter, (void **)&prev_stage);

    if (0 == prev_stage) return 0;

    OV_ASSERT(0 != prev_stage);

    ov_rtp_frame *remainder = drop_frame_from_stage(prev_stage, SSID);
    OV_ASSERT(0 != remainder);

    while (0 != stage_iter) {

        ov_list *current_stage = 0;
        stage_iter = stages->next(stages, stage_iter, (void **)&current_stage);

        /* There MUST not be holes in our stages list ! */
        OV_ASSERT(0 != current_stage);

        ov_rtp_frame *frame_to_shift =
            drop_frame_from_stage(current_stage, SSID);

        OV_ASSERT(0 != frame_to_shift);

        void *old = insert_into_stage(prev_stage, frame_to_shift);
        OV_ASSERT(0 == old);

        prev_stage = current_stage;
        current_stage = 0;

        OV_ASSERT(0 != prev_stage);
    }

    return remainder;

error:

    return 0;
}

/******************************************************************************
 *                              TERMINATE STUFF
 ******************************************************************************/

/**
 * Wrapper to rtp_frame->free to be used with for_each()
 */
static bool free_rtp_frame(void *value, void *arg) {

    UNUSED(arg);

    ov_rtp_frame *frame = value;

    if (0 == frame) return true;

    return 0 == frame->free(frame);
}

/*----------------------------------------------------------------------------*/

static inline ov_list *free_stages(ov_list *restrict stages) {

    OV_ASSERT(0 != stages);

    ov_list *stage = stages->pop(stages);

    while (0 != stage) {

        /* Terminate all frames in hash */
        stage->for_each(stage, 0, free_rtp_frame);

        stage = stage->free(stage);
        if (0 != stage) {

            ov_log_error(
                "Mem leak: Could not free "
                "hash");
        }

        stage = stages->pop(stages);
    };

    stages = stages->free(stages);

    return stages;
}

/******************************************************************************
 *                          INTERFACE IMPLEMENTATION
 ******************************************************************************/

void *impl_free(void *arg) {

    internal_frame_buffer *internal = as_internal_frame_buffer(arg);

    if (0 == internal) {

        goto error;
    }

    arg = 0;

    ov_list *stages = internal->stages;

    if (0 != stages) {

        stages = free_stages(stages);
    }

    internal->stages = 0;

    free(internal);
    internal = 0;

error:

    return arg;
}

/******************************************************************************
 *                              PUBLIC FUNCTIONS
 ******************************************************************************/

ov_rtp_frame_buffer *ov_rtp_frame_buffer_create(
    ov_rtp_frame_buffer_config config) {

    internal_frame_buffer *buffer = calloc(1, sizeof(internal_frame_buffer));
    buffer->public = (ov_rtp_frame_buffer){
        .magic_number = MAGIC_NUMBER,
        .free = impl_free,
    };

    if (0 == config.num_frames_to_buffer_per_stream) {
        config.num_frames_to_buffer_per_stream = NUM_FRAMES_TO_BUFFER_DEFAULT;
    }

    buffer->config = config;
    buffer->stages = ov_linked_list_create((ov_list_config){0});

    return (ov_rtp_frame_buffer *)buffer;
}

/*----------------------------------------------------------------------------*/

ov_rtp_frame_buffer *ov_rtp_frame_buffer_free(ov_rtp_frame_buffer *self) {

    if (0 != self) {

        return self->free(self);

    } else {

        return self;
    }
}

/*----------------------------------------------------------------------------*/

ov_rtp_frame *ov_rtp_frame_buffer_add(ov_rtp_frame_buffer *restrict self,
                                      ov_rtp_frame *frame) {

    ov_rtp_frame *remainder = frame;

    if (0 == remainder) {

        ov_log_error("No frame to insert given");
        goto finish;
    }

    internal_frame_buffer *internal = as_internal_frame_buffer(self);

    if (0 == internal) {

        ov_log_error("No frame buffer given");
        goto finish;
    }

    if (0 == internal->stages) {

        ov_log_error("Frame buffer does not contain stages");
        goto finish;
    }

    size_t const MAX_STAGES = internal->config.num_frames_to_buffer_per_stream;

    remainder = insert_into_stages(internal->stages, MAX_STAGES, frame);

    if (0 == remainder) {

        goto finish;
    }

    if (ALREADY_PRESENT == remainder) {

        remainder = frame;
        goto finish;
    }

    /* Ok, we got one frame too much, and its the one with the
     * topmost sequence number of SSID
     * We can try to add another stage at the end of the stages...*/

    ov_list *new_stage = add_stage(internal->stages, MAX_STAGES);

    if ((0 != new_stage) && (0 == insert_into_stage(new_stage, remainder))) {

        remainder = 0;
        goto finish;
    }

    new_stage = 0;

    /* Ok, thus we have to drop the frame with the lowest sequence
     * number
     * to clear space for the frame ...
     */

    ov_rtp_frame *dropped_frame =
        drop_oldest_frame(internal->stages, remainder);

    OV_ASSERT(0 != dropped_frame);

    insert_into_stages(internal->stages, MAX_STAGES, remainder);
    remainder = dropped_frame;

finish:

    return remainder;
}

/*----------------------------------------------------------------------------*/

ov_list *ov_rtp_frame_buffer_get_current_frames(
    ov_rtp_frame_buffer *restrict self) {

    ov_list *current = 0;

    internal_frame_buffer *internal = as_internal_frame_buffer(self);
    if (0 != self) {

        ov_list *stages = internal->stages;
        current = stages->remove(stages, 1);
    }

    return current;
}

/*----------------------------------------------------------------------------*/

void ov_rtp_frame_buffer_print(FILE *stream,
                               ov_rtp_frame_buffer const *buffer) {

    if (0 == stream) {
        stream = stdout;
    }

    /* Cast away the const, but we WONT modify ! */
    internal_frame_buffer *internal = as_internal_frame_buffer((void *)buffer);

    if (0 == internal) {
        fprintf(stream, "NULL\n");
        goto finish;
    }

    fprintf(stream, "FRAME BUFFER %p\n", buffer);

    ov_list *stages = internal->stages;

    if (0 == stages) {
        fprintf(stream, "STAGES == NULL\n");
        goto finish;
    }

    ov_list *stage = 0;
    void *iter = stages->iter(stages);

    for (size_t i = 0; 0 != iter; ++i) {

        iter = stages->next(stages, iter, (void **)&stage);

        fprintf(stream, "        STAGE [%zu]:   ", i);

        if (0 == stage) {
            fprintf(stream, "NULL\n");
            continue;
        }

        fprintf(stream, "\n");

        ov_rtp_frame *frame = 0;
        void *frame_iter = stage->iter(stage);

        for (size_t j = 0; 0 != frame_iter; ++j) {

            frame_iter = stage->next(stage, frame_iter, (void **)&frame);

            fprintf(stream, "                [%zu]:   ", j);

            if (0 == frame) {
                fprintf(stream, " NULL\n");
                continue;
            }

            fprintf(stream,
                    "SSID == %" PRIu32 " SEQ == %" PRIu32 "\n",
                    frame->expanded.ssrc,
                    frame->expanded.sequence_number);
        }
    }

finish:

    NOP;
}

/*----------------------------------------------------------------------------*/
