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
        @file           ov_password.c
        @author         Markus Töpfer

        @date           2022-01-03


        ------------------------------------------------------------------------
*/
#include <ov_base/ov_convert.h>
#include <ov_core/ov_password.h>

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>

static void print_usage() {

    fprintf(stdout, "\n");
    fprintf(stdout,
            "Generate a salted password hash with either of PKDS2 or scrypt\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "USAGE              [OPTIONS]... [PASSWORD]\n");
    fprintf(stdout, "\n");
    fprintf(stdout,
            "               -w,     --workfactor    scrypt workfactor to "
            "use\n");
    fprintf(stdout,
            "               -b,     --blocksize     scrypt blocksize to use\n");
    fprintf(stdout,
            "               -p,     --parallel      scrypt parallel to use\n");
    fprintf(stdout, "               -h,     --help          print this help\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "\n");
    fprintf(stdout,
            "This tool may generate salted password with scrypt, \n"
            "whenever some option is set, or defaults to PKDS2 if no option is "
            "used.\n");

    return;
}

/*----------------------------------------------------------------------------*/

bool read_user_input(int argc, char *argv[], ov_password_hash_parameter *params,
                     char **password, size_t *length) {

    int c = 0;
    int option_index = 0;

    uint16_t len = 0;

    while (1) {

        static struct option long_options[] = {

            /* These options don’t set a flag.
               We distinguish them by their indices. */
            {"parallel", required_argument, 0, 'p'},
            {"workfactor", required_argument, 0, 'w'},
            {"blocksize", required_argument, 0, 'b'},
            {"length", required_argument, 0, 'l'},
            {"help", optional_argument, 0, 'h'},
            {0, 0, 0, 0}};

        /* getopt_long stores the option index here. */

        c = getopt_long(argc, argv, "p:w:b:l:?h", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {

        case 0:

            printf("option %s", long_options[option_index].name);
            if (optarg)
                printf(" with arg %s", optarg);
            printf("\n");
            *password = optarg;
            break;

        case 'h':
            print_usage();
            goto error;
            break;

        case '?':
            print_usage();
            goto error;
            break;

        case 'p':
            ov_convert_string_to_uint16(optarg, strlen(optarg),
                                        &params->parallel);
            break;

        case 'l':
            ov_convert_string_to_uint16(optarg, strlen(optarg), &len);
            break;

        case 'w':
            ov_convert_string_to_uint16(optarg, strlen(optarg),
                                        &params->workfactor);
            break;

        case 'b':
            ov_convert_string_to_uint16(optarg, strlen(optarg),
                                        &params->blocksize);
            break;

        default:
            print_usage();
            goto error;
        }
    }

    if (optind < argc) {
        *password = argv[optind++];
    }

    *length = len;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {

    ov_password_hash_parameter params = {0};
    char *password = NULL;
    char *encryption = "PKDS2";
    size_t length = 0;

    if (!read_user_input(argc, argv, &params, &password, &length))
        goto error;

    if ((0 != params.workfactor) || (0 != params.blocksize) ||
        (0 != params.parallel))
        encryption = "scrypt";

    fprintf(stdout,
            "... using %s encryption\n"
            "    parallel   : %" PRIu16 "\n"
            "    workfactor : %" PRIu16 "\n"
            "    blocksize  : %" PRIu16 "\n",
            encryption, params.parallel, params.workfactor, params.blocksize);

    ov_json_value *hashed = ov_password_hash(password, params, length);
    char *str = ov_json_value_to_string(hashed);
    fprintf(stdout, "\nsalted password:\n%s\n", str);
    str = ov_data_pointer_free(str);
    hashed = ov_json_value_free(hashed);

    return EXIT_SUCCESS;
error:
    return EXIT_FAILURE;
}