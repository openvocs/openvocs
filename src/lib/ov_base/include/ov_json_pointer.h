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
        @file           ov_json_pointer.h
        @author         Markus Toepfer

        @date           2018-12-05

        @ingroup        ov_json_pointer

        @brief          Definition of implementation of RFC 6901
                        "JavaScript Object Notation (JSON) Pointer" 

        ------------------------------------------------------------------------
*/
#ifndef ov_json_pointer_h
#define ov_json_pointer_h

#include "ov_json_value.h"
#include <stdbool.h>

/*----------------------------------------------------------------------------*/

/**
        Get a value using pointer based access, e.g.

        for

        {
         "a" : {
           "b" : {
              "cdef" : 14
           }
         }
        }

        ov_json_get(value, "/a/b/cdef") -> "14"
        ov_json_get(value, "/b/cdef") -> NULL


        using a \0 terminated pointer string

*/
ov_json_value const *ov_json_get(const ov_json_value *value,
                                 const char *pointer);

/*----------------------------------------------------------------------------*/

#endif /* ov_json_pointer_h */
