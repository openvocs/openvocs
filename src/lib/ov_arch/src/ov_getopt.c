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
        @file           ov_getopt.c
        @author         Markus Töpfer

        @date           2020-05-22


        ------------------------------------------------------------------------
*/
#include "../include/ov_getopt.h"

#include <unistd.h>

void ov_reset_getopt(void) {

#if OV_ARCH == OV_LINUX
  optind = 0;
#elif OV_ARCH == OV_MACOS
  optind = 1;
#endif

#ifdef HAVE_OPTRESET
  optreset = 1;
#endif
}