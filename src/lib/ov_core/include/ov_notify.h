/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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
        @copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_NOTIFY_H
#define OV_NOTIFY_H

/*----------------------------------------------------------------------------*/

#include "ov_recording.h"
#include <ov_base/ov_json_value.h>

/*----------------------------------------------------------------------------*/

typedef enum {

  NOTIFY_INVALID = -1,     /* Requires ' entity' in parameters to be set */
  NOTIFY_ENTITY_LOST = 0,  /* Requires ' entity' in parameters to be set */
  NOTIFY_INCOMING_CALL,    /* Requires 'call' in parameters to be set */
  NOTIFY_NEW_CALL,         /* Requires 'call' in parameters to be set */
  NOTIFY_CALL_TERMINATED,  /* Requires 'call' in parameters to be set */
  NOTIFY_NEW_RECORDING,    /* Requires 'recording' in parameters to be set */
  NOTIFY_PLAYBACK_STOPPED, /* Unsupported as of now ... */

} ov_notify_type;

typedef union {

  struct {
    char const *name;
    char const *type;
  } entity;

  struct {

    char const *id;
    char const *loop;
    char const *peer;

  } call;

  ov_recording recording;

} ov_notify_parameters;

/*----------------------------------------------------------------------------*/

char const *ov_notify_type_to_string(ov_notify_type type);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_notify_message(char const *uuid, ov_notify_type notify_type,
                                 ov_notify_parameters parameters);

/*----------------------------------------------------------------------------*/

ov_notify_type ov_notify_parse(ov_json_value const *parameters,
                               ov_notify_parameters *notify_params);

/*----------------------------------------------------------------------------*/
#endif
