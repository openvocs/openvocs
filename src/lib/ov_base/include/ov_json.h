/***
        ------------------------------------------------------------------------

        Copyright (c) 2018 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_json.h
        @author         Markus Toepfer

        @date           2018-12-02

        @ingroup        ov_json

        @brief          Definition of some basic JSON parsing und unparsing 
                        functions. 

        ------------------------------------------------------------------------
*/
#ifndef ov_json_h
#define ov_json_h

#include "ov_utils.h"

#include "ov_json_parser.h"
#include "ov_json_pointer.h"
#include "ov_json_value.h"

/*----------------------------------------------------------------------------*/

/**
 *      Decode a JSON string to a ov_json_value
 */
ov_json_value *ov_json_decode(const char *string);

/*----------------------------------------------------------------------------*/

/**
 *      Encode a JSON value to string.
 */
char *ov_json_encode(const ov_json_value *value);

/*----------------------------------------------------------------------------*/

/**
        Parse a value from a buffer.

        @param json     json implementation
        @param buffer   start of the buffer to
   parse
        @param size     size of the buffer to
   parse
*/
ov_json_value *ov_json_read(const char *buffer, size_t size);

/*----------------------------------------------------------------------------*/

/**
        Write a JSON value to an buffer.

        @param value    json value to write
        @param buffer   start of the buffer to parse
        @param size     size of the buffer to parse
        @param next     next pointer after write
*/
bool ov_json_write(ov_json_value *value,
                   char *buffer,
                   size_t size,
                   char **next);

/*----------------------------------------------------------------------------*/

/**
        Read a json value from file.

        @param path     path to read
*/
ov_json_value *ov_json_read_file(const char *path);

/*----------------------------------------------------------------------------*/

/**
        Read all json files from directory, will ignore non JSON files.
        (no error if folder contains files with other content)

        @param path     path to read
        @param ext      (optional) extention to include

        @returns

        {
            "filename1" : < file1 content >,
            "filename2" : < file2 content >,
            ...
        }
*/
ov_json_value *ov_json_read_dir(const char *path, const char *ext);

/*----------------------------------------------------------------------------*/

/**
        Write a json value to file.

        @param file     path to file
        @param value    value to write to file
*/
bool ov_json_write_file(const char *file, const ov_json_value *value);

#endif /* ov_json_h */
