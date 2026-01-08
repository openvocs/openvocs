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
        @file           ov_vocs_permission.c
        @author         Markus

        @date           2022-07-26


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_permission.h"
#include <ov_base/ov_config_keys.h>
#include <string.h>

const char *ov_vocs_permission_to_string(ov_vocs_permission self) {

  switch (self) {

  case OV_VOCS_RECV:
    return OV_KEY_RECV;

  case OV_VOCS_SEND:
    return OV_KEY_SEND;

  default:
    break;
  }

  return OV_KEY_NONE;
}

/*----------------------------------------------------------------------------*/

ov_vocs_permission ov_vocs_permission_from_string(const char *str) {

  if (!str)
    goto error;

  size_t len = strlen(str);
  if (len != 4)
    goto error;

  if (0 == strcmp(str, OV_KEY_RECV))
    return OV_VOCS_RECV;

  if (0 == strcmp(str, OV_KEY_SEND))
    return OV_VOCS_SEND;

error:
  return OV_VOCS_NONE;
}

/*----------------------------------------------------------------------------*/

ov_vocs_permission ov_vocs_permission_from_json(const ov_json_value *val) {

  return ov_vocs_permission_from_string(ov_json_string_get(val));
}
/*----------------------------------------------------------------------------*/

bool ov_vocs_permission_granted(ov_vocs_permission reference,
                                ov_vocs_permission check) {

  switch (check) {

  case OV_VOCS_NONE:
    break;

  case OV_VOCS_RECV:

    if (OV_VOCS_NONE == reference)
      goto error;

    break;

  case OV_VOCS_SEND:

    if (OV_VOCS_SEND != reference)
      goto error;

    break;
  }

  return true;
error:
  return false;
}