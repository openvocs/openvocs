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
        @author         Tobias Kolb

        This header only discovers the architecture we run on
        and sets the OV_ARCH macro accordingly

        ------------------------------------------------------------------------
*/

#ifndef OV_ARCH_UUID
#define OV_ARCH_UUID

#include "ov_arch.h"

#if OV_ARCH == OV_MACOS
#include <uuid/uuid.h>
#else
#include <uuid.h>
#endif

#endif
