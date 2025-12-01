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
        @file           ov_version.h
        @author         Markus Toepfer

        @date           2020-02-10

        @brief          Some access header to makefile based version
                        and build number management.

        @NOTE           header only implementation.

        ------------------------------------------------------------------------
*/

#ifndef ov_version_h
#define ov_version_h

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef OV_VERSION
#error "Must provide -D OV_VERSION=value while compiling."
#endif

#ifndef OV_VERSION_BUILD_ID
#error "Must provide -D OV_VERSION_BUILD_ID=value while compiling."
#endif

#ifndef OV_VERSION_COMMIT_ID
#error "Must provide -D OV_VERSION_COMMIT_ID=value while compiling."
#endif

#ifndef OV_VERSION_BUILD_DATE
#error "Must provide -D OV_VERSION_BUILD_DATE=value while compiling."
#endif

#ifndef OV_VERSION_COMPILER
#error "Must provide -D OV_VERSION_COMPILER=value while compiling."
#endif

#define OV_COPYRIGHT                                                           \
    "Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)\n"             \
    "\n"                                                                       \
    "Licensed under the Apache License, Version 2.0 (the \"License\");\n"      \
    "you may not use this file except in compliance with the License.\n"       \
    "You may obtain a copy of the License at\n"                                \
    "\n"                                                                       \
    "        http://www.apache.org/licenses/LICENSE-2.0\n"                     \
    "\n"                                                                       \
    "Unless required by applicable law or agreed to in writing, "              \
    "software\n"                                                               \
    "distributed under the License is distributed on an \"AS IS\" "            \
    "BASIS,\n"                                                                 \
    "WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or "         \
    "implied.\n"                                                               \
    "See the License for the specific language governing permissions "         \
    "and\n"                                                                    \
    "limitations under the License.\n"                                         \
    "\n"                                                                       \
    "This file is part of the openvocs project. https://openvocs.org\n"

/*----------------------------------------------------------------------------*/

/**
        Print the version information to a stream.
        @param stream   filestream e.g. stdout
*/
#define OV_VERSION_PRINT(stream)                                               \
    fprintf(stream,                                                            \
            "\n"                                                               \
            "%s"                                                               \
            "------------------------------------------------------------"     \
            "----------------\n"                                               \
            "    build date : %s\n"                                            \
            " build version : %s\n"                                            \
            "      build id : %s\n"                                            \
            "     commit id : %s\n"                                            \
            " Compiler      : %s\n"                                            \
            "------------------------------------------------------------"     \
            "----------------\n"                                               \
            "\n",                                                              \
            OV_COPYRIGHT,                                                      \
            OV_VERSION_BUILD_DATE,                                             \
            OV_VERSION,                                                        \
            OV_VERSION_BUILD_ID,                                               \
            OV_VERSION_COMMIT_ID,                                              \
            OV_VERSION_COMPILER);

/*----------------------------------------------------------------------------*/

inline const char *ov_version() { return OV_VERSION; }

/*----------------------------------------------------------------------------*/

inline const char *ov_version_build_id() { return OV_VERSION_BUILD_ID; }

/*----------------------------------------------------------------------------*/

inline const char *ov_version_build_date() { return OV_VERSION_BUILD_DATE; }

/*----------------------------------------------------------------------------*/

/*
char const *ov_version_additional_info();
bool ov_version_set_additional_info(char const *additional_info);
*/

#endif /* ov_version_h */
