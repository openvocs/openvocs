/***
        ------------------------------------------------------------------------

        Copyright (c) 2026 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_timestamp.c
        @author         Töpfer, Markus

        @date           2026-05-23


        ------------------------------------------------------------------------
*/

#include <ov_base/ov_time.h>

/*----------------------------------------------------------------------------*/

int main(int argc, char **argv) {

        UNUSED(argc);
        UNUSED(argv);

        char *t = ov_timestamp(false);
        fprintf(stdout, "%s\n", t);
        t = ov_data_pointer_free(t);

        return EXIT_SUCCESS;
}



