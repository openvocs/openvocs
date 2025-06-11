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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

/*----------------------------------------------------------------------------*/

int16_t from_be(int16_t s) {

    uint16_t us = (uint16_t)s;

    uint16_t ur = (0x00ff & us) << 8;
    ur += (0xff00 & us) >> 8;

    return (int16_t)ur;
}

/*----------------------------------------------------------------------------*/

static int16_t swap_bytes(int16_t s) {

    uint16_t us = (uint16_t)s;

    uint16_t ur = (0x00ff & us) << 8;
    ur += (0xff00 & us) >> 8;

    return (int16_t)ur;
}

/*----------------------------------------------------------------------------*/

static FILE *open_for(char const *path, char const *mode) {

    FILE *f = fopen(path, mode);

    if (0 == f) {
        fprintf(stderr, "Could not open %s\n", path);
        exit(EXIT_FAILURE);
    }

    return f;
}

/*----------------------------------------------------------------------------*/

_Noreturn static int convert_csv_to_bin(int argc, char **argv) {

    if (3 < argc) {
        fprintf(stderr, "Require input file name as argument\n");
        exit(EXIT_FAILURE);
    }

    char const *fname = argv[2];
    char outname[255] = {0};

    snprintf(outname, sizeof(outname), "%s.out", fname);
    outname[sizeof(outname) - 1] = 0;

    printf("Readin from %s, writing to %s\n", fname, outname);

    FILE *in = open_for(fname, "r");
    FILE *out = open_for(outname, "w");

    double dvalue = 0.0;

    size_t converted = 0;

    while (1 == fscanf(in, "%lf", &dvalue)) {

        ++converted;

        int16_t ivalue = (int16_t)(dvalue * INT16_MAX);
        ivalue = swap_bytes(ivalue);
        fwrite(&ivalue, sizeof(ivalue), 1, out);
    }

    printf("Done - converted %zu values\n", converted);

    fclose(in);
    in = 0;

    fclose(out);
    out = 0;

    exit(EXIT_SUCCESS);
}

/*----------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    if (2 > argc) {
        fprintf(stderr, "Require input file name as argument\n");
        return EXIT_FAILURE;
    }

    if (0 == strcmp(argv[1], "c")) {
        convert_csv_to_bin(argc, argv);
    }

    char const *fname = argv[1];

    struct stat file_stat = {0};

    if (0 != stat(fname, &file_stat)) {
        fprintf(stderr, "Could not access file %s\n", fname);
        exit(EXIT_FAILURE);
    }

    size_t fsize = file_stat.st_size;

    size_t remainder = fsize % sizeof(int16_t);

    if (0 != remainder) {
        fprintf(
            stderr, "File size %zu no multiple of %zu", fsize, sizeof(int16_t));
    }

    fsize -= remainder;
    fprintf(stderr, "Adjusting file size to %zu\n", fsize);

    int fd = open(argv[1], O_RDONLY);

    if (0 > fd) {
        fprintf(stderr, "Could not open %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    int16_t *bin_data = mmap(0, fsize, PROT_READ, MAP_PRIVATE, fd, 0);

    if (MAP_FAILED == bin_data) {
        fprintf(stderr, "Could not map file\n");
        return EXIT_FAILURE;
    }

    for (size_t sample = 0; sample < fsize / 2; ++sample) {
        int16_t s = bin_data[sample];
        s = from_be(s);
        fprintf(stdout, "%zu %" PRIi16 "\n", sample, s);
    }

    if (0 != munmap(bin_data, fsize)) {
        fprintf(stderr, "Could not unmap file\n");
    }

    return EXIT_SUCCESS;
}
