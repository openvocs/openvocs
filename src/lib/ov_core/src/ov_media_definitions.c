/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_media_definitions.c
        @author         author
        @date           2021-06-10


        ------------------------------------------------------------------------
*/
#include "../include/ov_media_definitions.h"

#include <string.h>
#include <strings.h>

ov_media_type ov_media_type_from_string(const char *string) {

  if (!string)
    goto error;

  if (0 == strcmp(string, OV_MEDIA_KEY_REQUEST))
    return OV_MEDIA_REQUEST;

  if (0 == strcmp(string, OV_MEDIA_KEY_OFFER))
    return OV_MEDIA_OFFER;

  if (0 == strcmp(string, OV_MEDIA_KEY_ANSWER))
    return OV_MEDIA_ANSWER;

error:
  return OV_MEDIA_ERROR;
}

/*----------------------------------------------------------------------------*/

const char *ov_media_type_to_string(ov_media_type type) {

  switch (type) {

  case OV_MEDIA_REQUEST:
    return OV_MEDIA_KEY_REQUEST;

  case OV_MEDIA_OFFER:
    return OV_MEDIA_KEY_OFFER;

  case OV_MEDIA_ANSWER:
    return OV_MEDIA_KEY_ANSWER;

  default:
    break;
  }

  return NULL;
}