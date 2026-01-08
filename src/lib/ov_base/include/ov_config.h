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
        @file           ov_config.h
        @author         Markus Töpfer
        @author         Michael Beer

        @date           2020-03-11

        Basic functions for config loading and access.

        ------------------------------------------------------------------------
*/
#ifndef ov_config_h
#define ov_config_h

#include "ov_json.h"
#include "ov_socket.h"
#include "ov_version.h"

extern const char *VERSION_REQUEST_ONLY;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

char const *ov_config_default_config_file_for(char const *app_name);

/*----------------------------------------------------------------------------*/

/**
        Load some JSON config from a path.

        @param path     (mandatory) path to JSON config file
*/
ov_json_value *ov_config_load(const char *path);

/*---------------------------------------------------------------------------*/

/**
        Parse -c out of command line args.

        If -v was set, the version will be printed to stderr

        BEWARE:  `getopt` used internally - might need reset of `optind` etc.

        @param argc     number of args
        @param argv     array of args

        @return path set over -c OR NULL
*/
const char *ov_config_path_from_command_line(size_t argc, char **argv);

/*---------------------------------------------------------------------------*/

/**
        Parse for -c for config path and load the JSON file provided.
        Prints the version if -v is identified.

        BEWARE:  `getopt` used internally - might need reset of `optind` etc.

        Combination of
        @see ov_config_path_from_command_line and
        @see ov_config_load

        @NOTE other options MAY not be added, but you MAY use some
        custom options within the JSON config loaded

        @param argc     number of args
        @param argv     array of args
*/
ov_json_value *ov_config_from_command_line(size_t argc, char *argv[]);

/*----------------------------------------------------------------------------*/

double ov_config_double_or_default(ov_json_value const *jval, char const *key,
                                   double default_val);

uint32_t ov_config_u32_or_default(ov_json_value const *jval, char const *key,
                                  uint32_t default_val);

uint64_t ov_config_u64_or_default(ov_json_value const *jval, char const *key,
                                  uint64_t default_val);

bool ov_config_bool_or_default(ov_json_value const *jval, char const *key,
                               bool default_val);

/*----------------------------------------------------------------------------*/

ov_socket_configuration ov_config_socket_configuration_or_default(
    ov_json_value const *jcfg, char const *key,
    ov_socket_configuration default_socket);

/*----------------------------------------------------------------------------*/

#endif /* ov_config_h */
