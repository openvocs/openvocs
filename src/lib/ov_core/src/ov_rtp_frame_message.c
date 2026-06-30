/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

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

        ------------------------------------------------------------------------
*/

#include <ov_base/ov_utils.h>

#include "../include/ov_rtp_frame_message.h"
#include <ov_base/ov_registered_cache.h>
#include <ov_base/ov_rtp_frame.h>

/*----------------------------------------------------------------------------*/

static ov_registered_cache *g_cache = 0;

/*----------------------------------------------------------------------------*/

static ov_rtp_frame_message *as_rtp_frame_message(void *vptr) {

    ov_thread_message *msg = ov_thread_message_cast(vptr);

    if (0 == msg)
        return 0;

    if (OV_RTP_FRAME_MESSAGE_TYPE != msg->type)
        return 0;

    return (ov_rtp_frame_message *)msg;
}

/******************************************************************************
 *                             Interface methods
 ******************************************************************************/

static ov_thread_message *impl_rtp_frame_message_free(ov_thread_message *msg) {

    ov_rtp_frame_message *rtp_message = as_rtp_frame_message(msg);

    if (0 == rtp_message) {

        ov_log_error("Wrong argument - expected "
                     "ov_rtp_frame_message");
        goto error;
    }

    msg = 0;

    if (0 != rtp_message->frame) {

        OV_ASSERT(0 != rtp_message->frame->free);
        rtp_message->frame = ov_rtp_frame_free(rtp_message->frame);
    }

    OV_ASSERT(0 == rtp_message->frame);

    rtp_message = ov_registered_cache_put(g_cache, rtp_message);

    if (0 != rtp_message) {

        free(rtp_message);
        rtp_message = 0;
    }

    return 0;

error:

    return msg;
}

/******************************************************************************
 *                               Public methods
 ******************************************************************************/

static void *message_vptr_free(void *vmsg) {

    return impl_rtp_frame_message_free((ov_thread_message *)vmsg);
}

/*----------------------------------------------------------------------------*/

void ov_rtp_frame_message_enable_caching(size_t cache_size) {

    ov_registered_cache_config cfg = {
        .item_free = message_vptr_free,
        .capacity = cache_size,
    };

    g_cache = ov_registered_cache_extend("ov_rtp_frame_message", cfg);

    ov_rtp_frame_enable_caching(cache_size);
}

/*----------------------------------------------------------------------------*/

ov_thread_message *ov_rtp_frame_message_create(ov_rtp_frame *frame) {

    ov_rtp_frame_message *msg = ov_registered_cache_get(g_cache);

    if (0 == msg) {

        msg = calloc(1, sizeof(ov_rtp_frame_message));
    }

    OV_ASSERT(0 != msg);

    msg->public = (ov_thread_message){

        .magic_bytes = OV_THREAD_MESSAGE_MAGIC_BYTES,
        .type = OV_RTP_FRAME_MESSAGE_TYPE,
        .free = impl_rtp_frame_message_free,
    };

    msg->frame = frame;

    return &msg->public;
}

/*----------------------------------------------------------------------------*/
