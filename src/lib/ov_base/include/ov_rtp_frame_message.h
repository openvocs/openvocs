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
#ifndef OV_RTP_FRAME_MESSAGE_H
#define OV_RTP_FRAME_MESSAGE_H

/*----------------------------------------------------------------------------*/

#include "ov_rtp_frame.h"
#include "ov_thread_message.h"

/*----------------------------------------------------------------------------*/

#define OV_RTP_FRAME_MESSAGE_TYPE                                              \
    ((int)(OV_THREAD_MESSAGE_START_USER_TYPES + 'r' + '0'))

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_thread_message public;

    ov_rtp_frame *frame;

} ov_rtp_frame_message;

/*----------------------------------------------------------------------------*/

void ov_rtp_frame_message_enable_caching(size_t cache_size);

/*----------------------------------------------------------------------------*/

ov_thread_message *ov_rtp_frame_message_create(ov_rtp_frame *frame);

/*----------------------------------------------------------------------------*/

#endif
