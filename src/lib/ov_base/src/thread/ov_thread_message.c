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
*
*    \file               ov_thread_message.c
*    \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
*    \date               2017-10-29
*
*    \ingroup            empty
*
*    \brief              empty
*
*/
/*---------------------------------------------------------------------------*/

#include "../../include/ov_thread_message.h"
#include "../../include/ov_utils.h"

/******************************************************************************
 *                             INTERNAL FUNCTIONS
 ******************************************************************************/

static ov_thread_message *free_message(ov_thread_message *message) {

    if (0 == message)
        goto error;

    if (!OV_THREAD_MESSAGE_VALID(message))
        goto error;

    ov_json_value *json = message->json_message;

    json = ov_json_value_free(json);
    OV_ASSERT(0 == json);

    free(message);
    message = 0;

error:

    return message;
}

/******************************************************************************
 *
 *  PUBLIC FUNCTIONS
 *
 ******************************************************************************/

ov_thread_message *
ov_thread_message_standard_create(ov_thread_message_type type,
                                  ov_json_value *json) {

    ov_thread_message *message = 0;

    switch (type) {

    case OV_THREAD_MESSAGE_TYPE_ENSURE_SIGNED_INT_TYPE:

        /* THIS SHOULD NEVER HAPPEN !!! */
        return 0;

    case OV_GENERIC_MESSAGE:
    default:

        message = calloc(1, sizeof(ov_thread_message));
        message->free = free_message;

        break;
    };

    if (0 != json) {
        message->json_message = json;
    }

    message->type = type;
    message->magic_bytes = OV_THREAD_MESSAGE_MAGIC_BYTES;

    return message;
}

/*----------------------------------------------------------------------------*/

ov_thread_message *ov_thread_message_cast(void *vptr) {

    if (0 == vptr)
        return 0;

    ov_thread_message *msg = vptr;

    if (!OV_THREAD_MESSAGE_VALID(msg))
        return 0;

    return msg;
}

/*---------------------------------------------------------------------------*/

ov_thread_message *ov_thread_message_free(ov_thread_message *msg) {

    if ((0 == msg) || (0 == msg->free)) {
        goto error;
    }

    return msg->free(msg);

error:

    return msg;
}

/*----------------------------------------------------------------------------*/
