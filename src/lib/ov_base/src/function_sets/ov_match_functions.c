/***
        ------------------------------------------------------------------------

        Copyright (c) 2018 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_match_functions.c
        @author         Michael Beer
        @author         Markus Toepfer

        @date           2018-08-15

        @ingroup        ov_basics

        @brief          Implementation of some default match functions.


        ------------------------------------------------------------------------
*/
#include "../../include/ov_match_functions.h"

#include <string.h>
#include <strings.h>

bool ov_match_strict(const void *key, const void *string) {

  if (!key || !string)
    return false;

  size_t len = strlen(string);

  if (strlen(key) != len)
    return false;

  if (0 == memcmp(key, string, len))
    return true;

  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_match_c_string_strict(const void *key, const void *string) {

  if (key == string)
    return true;

  if (!key || !string)
    return false;

  if (0 == strcmp(key, string))
    return true;

  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_match_c_string_case_ignore_strict(const void *key, const void *string) {

  if (key == string)
    return true;

  if (!key || !string)
    return false;

  if (0 == strcasecmp(key, string))
    return true;

  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_match_intptr(const void *ptr1, const void *ptr2) {

  return (ptr1 == ptr2);
}

/*---------------------------------------------------------------------------*/

bool ov_match_uint64(const void *ptr1, const void *ptr2) {

  if (!ptr1 || !ptr2)
    return false;

  return ((*(uint64_t *)ptr1) == (*(uint64_t *)ptr2));
}
