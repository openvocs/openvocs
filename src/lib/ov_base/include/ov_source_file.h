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
        @file           ov_source_file.h
        @author         Markus Toepfer

        @date           2019-02-10

        @brief          Function set to create ov_* module files and
                        sub project folders.

        ------------------------------------------------------------------------
*/

#ifndef ov_source_file_h
#define ov_source_file_h

typedef struct ov_source_file_config ov_source_file_config;

#include <stdio.h>
#include <stdlib.h>

#include "ov_copyright.h"
#include "ov_version.h"

#define OV_SOURCE_FILE_FOLDER_INCLUDE "include"
#define OV_SOURCE_FILE_FOLDER_SOURCE "src"
#define OV_SOURCE_FILE_TAG_AUTHOR "author"
#define OV_SOURCE_FILE_NAME "ov_source_file"

/*----------------------------------------------------------------------------*/

struct ov_source_file_config {

  struct {

    const char *name;
    const char *path;
    const char *url; // optional URL to use

    bool create;

  } project;

  ov_copyright_config copyright;
};

/*----------------------------------------------------------------------------*/

/**
        Read input from commandline for an ov_source_file_config.

        @param config   config to fill
        @param app_name name of the implementation binary, using the functions
*/
bool ov_source_file_read_user_input(int argc, char *argv[],
                                    ov_source_file_config *config,
                                    const char *app_name);

/*----------------------------------------------------------------------------*/

/**
        Read the git author from the environment,
        and write to buffer of size.
*/
bool ov_source_file_get_git_author(char *buffer, size_t size);

/*----------------------------------------------------------------------------*/

/**
        Create the source files include/NAME.h src/NAME.c src/NAME_test.c
        using the config provided.
*/
bool ov_source_file_create_source_files(ov_source_file_config *config);

/*----------------------------------------------------------------------------*/

/**
        Create a new subproject with form:

        NAME
        |
        |- include
        |   | - NAME.h
        |
        |- src
        |   | NAME.c
        |   | NAME_test.c
        |
        |- makefile
*/
bool ov_source_file_create_project(ov_source_file_config *config);

/*----------------------------------------------------------------------------*/

/**
        Write some intro and outro around some text lines.
*/
char *ov_source_file_insert_at_each_line(const char *text, const char *intro,
                                         const char *outro);

#endif /* ov_source_file_h */