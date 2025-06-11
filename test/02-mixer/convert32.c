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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include <inttypes.h>
#include <stddef.h>

#include <stdio.h>

#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char **argv) {

    int fh = open(argv[1], O_RDONLY);

    int32_t in_buffer[1024] = {0};

    ssize_t bytes_read = 0;

    while (1) {

        bytes_read = read(fh, in_buffer, sizeof(in_buffer));

        if (1 > bytes_read) break;

        for (size_t i = 0; i < bytes_read / 4; ++i) {

            printf("%" PRIi32 "\n", in_buffer[i]);
        }
    };
}
