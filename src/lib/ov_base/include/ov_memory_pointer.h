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
        @file           ov_memory_pointer.h
        @author         Markus Töpfer

        @date           2020-12-29


        ------------------------------------------------------------------------
*/
#ifndef ov_memory_pointer_h
#define ov_memory_pointer_h

#include <inttypes.h>
#include <stdlib.h>

typedef struct {

    const uint8_t *start;
    size_t length;

} ov_memory_pointer;

#endif /* ov_memory_pointer_h */
