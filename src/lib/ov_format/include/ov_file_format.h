/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_file_format.h
        @author         Markus Töpfer

        @date           2021-01-02

        Manage and parse file formats.

        This implementation SHALL be used to create a registry for files and
        manage some kind of META information of files.

        The implementation is done to gather and hold enough file information
        required for MIME based applications e.g. HTTP or SMTP.

        Supported file formats SHALL be registered with some handler in line
        with ov_format, as well as MIME information for the format. So this
        implementation is some extension of ov_format with additional infos.

        USE CASE 1

            HTTP GET needs some information about file size, MIME type and
            encoding. This may be gathered using ov_file_format_get_desc
            for some PATH in combination with file type specific information
            of registered files included in ov_file_format_registry.

            e.g. some .html extension SHALL use MIME text/html or some .opus
            extension SHALL use MIME audio/opus

        USE CASE 2

            Some encoding scheme SHALL be implemented for available format
            implementations of ov_format*

            ov_file_desc_from_path will gather enough information for file
            extensions, which MAY be processed in reverse order to get to the
            files content.

            e.g. ov_file_desc_from_path(path) will return with extension array

                desc.ext[0] = "gzip"
                desc.ext[1] = "json"
                desc.ext[2] = 0

            and may trigger following processing chain

            ov_file_desc desc = ov_file_desc_from_path(path);
            ov_format *x = ov_format_open(path, OV_READ);

            for (size_t i = 0; i < OV_FILE_EXT_MAX; i++){

                if (desc.ext[i][0] == 0)
                    break;

                x = ov_file_format_as(x, desc.ext[i]);
            }

            format x will be json -> gzip -> RAW

            NOTE this code above requires ov_format_gzip as well as
            ov_format_json to be added to the registry before!

            e.g.

            (A) register some json handler with extension json for .json files

            ov_file_format_register(
                    <private_registry_pointer>,
                    (ov_file_format_parameter){
                        .name = "json",
                        .handler = ov_format_json_handler
                    },
                    1,
                    { "json" });

            (B) register some gzip handler with extension gzip for .gzip files

            ov_file_format_register(
                    <private_registry_pointer>,
                    (ov_file_format_parameter){
                        .name = "gzip",
                        .handler = ov_format_gzip_handler
                    },
                    1,
                    { "gzip" });

        ------------------------------------------------------------------------
*/
#ifndef ov_file_format_h
#define ov_file_format_h

#include <limits.h>
#include <stdbool.h>

#include <ov_base/ov_json.h>
#include <ov_base/ov_memory_pointer.h>

#define OV_FILE_FORMAT_PARAMETER_NAME_MAX 255
#define OV_FILE_FORMAT_TYPE_NAME_MAX 255
#define OV_FILE_FORMAT_MIME_MAX 255

#define OV_FILE_EXT_STRING_MAX 255
#define OV_FILE_EXT_MAX 20

/*
 *      ------------------------------------------------------------------------
 *
 *      TYPE DEFINITIONS
 *
 *      ... before including the ov_format headers to allow the use
 *      of ov_file_format within ov_format functionality.
 *
 *      ------------------------------------------------------------------------
 */

typedef struct ov_file_format_registry_struct ov_file_format_registry;
typedef struct ov_file_format_parameter ov_file_format_parameter;
typedef struct ov_file_desc ov_file_desc;
typedef struct ov_file_format_desc ov_file_format_desc;

#include "ov_format.h"

/*----------------------------------------------------------------------------*/

struct ov_file_format_parameter {

  /* Mandatory unique name */

  char name[OV_FILE_FORMAT_PARAMETER_NAME_MAX];

  /* Optional (recommended) parameter input for MIME type */
  char mime[OV_FILE_FORMAT_MIME_MAX];

  /* Optional (recommended) format handler */
  ov_format_handler handler;
};

/*----------------------------------------------------------------------------*/

struct ov_file_desc {

  /*  content length of the file in bytes */

  ssize_t bytes;

  /*  array of file extensions (byte) strings with last to first order
   *
   *  e.g.    path/file.data.gzip.1
   *
   *      ext[0] = "1"
   *      ext[1] = "gzip"
   *      ext[2] = "data"
   *      ext[3] = 0
   */
  char ext[OV_FILE_EXT_MAX][OV_FILE_EXT_STRING_MAX];
};

/*----------------------------------------------------------------------------*/

struct ov_file_format_desc {

  ov_file_desc desc;

  /* MIME type/subtype e.g. audio/opus video/ogg text/html */
  char mime[OV_FILE_FORMAT_MIME_MAX];
};

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_file_format_registry *ov_file_format_registry_cast(void *data);

/*----------------------------------------------------------------------------*/

/**
    This is @see ov_format_as using ov_file_format_registry instead of
    ov_file_registry.
 */
ov_format *ov_file_format_as(ov_format *f, char const *file_type, void *options,
                             ov_file_format_registry *registry);

/*----------------------------------------------------------------------------*/

/**
    This function will create a file desc from a given input path containing the
    size of the files, as well as file extensions parsed as stings in desc.ext.

    Array of file extensions (byte) strings with last to first order so the last
    applied encoding e.g. gzip will be the first extension present, which may be
    handy for ov_format to build a backward decoding chain.

    e.g.    path/file.data.gzip.1

            ext[0] = "1"
            ext[1] = "gzip"
            ext[2] = "data"
            ext[3 ->] = 0

    @param path path to check  (UTF8)

    @returns desc with parsed extensions of the file
    @returns empty empty desc with (bytes == -1) on error
*/
ov_file_desc ov_file_desc_from_path(const char *restrict path);

/*----------------------------------------------------------------------------*/

/**
    Check if the encoding of some path is a valid UTF-8 sequence.

    NOTE This will actually validate the whole file encoding.
    NOTE This will return true for ASCII encoded file, whoever the default
    for openvocs SHOULD be UTF-8 to support non US/latin character sets.

    @param path path to file to check
*/
bool ov_file_encoding_is_utf8(const char *restrict path);

/*----------------------------------------------------------------------------*/

/**
    Get some file format description for an input at path containing MIME and
    ov_file_desc information.

    @param registry     registry instance
    @param path         file path

    @returns desc for format
    @returns -1 == format.desc.bytes on error
*/
ov_file_format_desc
ov_file_format_get_desc(const ov_file_format_registry *registry,
                        const char *restrict path);

/*
 *      ------------------------------------------------------------------------
 *
 *      #REGISTER
 *
 *      ------------------------------------------------------------------------
 */

/**
    Get the format parameter of some registered name

    @param registry     register to use
    @param name         name used as parameter.name in ov_file_format_register

    @returns            ov_file_format_parameter for the name
    @returns            NULL on error
*/
const ov_file_format_parameter *
ov_file_format_get(const ov_file_format_registry *registry, const char *name);

/*----------------------------------------------------------------------------*/

/**
    Get the format parameter of some registered file extension

    @param registry     register to use
    @param extension    name used in ext_array in ov_file_format_register

    @returns            ov_file_format_parameter for the name
    @returns            NULL on error
*/
const ov_file_format_parameter *
ov_file_format_get_ext(const ov_file_format_registry *registry,
                       const char *extension);

/*----------------------------------------------------------------------------*/

/**
    Create or extend a registry with some new file format.

    @param registry     pointer some registry (if *registry == NULL)
                        a new register will be created.
    @param parameter    parameter for the format
    @param ext_size     size of (optional) ext_array (may be 0)
    @param ext_array    (optional) ext_array to register the
                        format for some file extensions e.g. .json .gzip .png

    NOTE ext_array is used to register some handler for several extensions
    e.g. { "htm", "html "} or { "jpg", "jpeg" }
*/
bool ov_file_format_register(ov_file_format_registry **registry,
                             ov_file_format_parameter parameter,
                             size_t ext_size, char const *ext_array[]);

/*----------------------------------------------------------------------------*/

/**
    Free some register of file formats

    @param registry     pointer to registry to be freed
*/
bool ov_file_format_free_registry(ov_file_format_registry **registry);

/*----------------------------------------------------------------------------*/

/**
    Load some mime based formats from a path.
    This function is intended to load and connect mime type and file extensions
    in a configurable way.

    Example input JSON structure expected at PATH:

        {
            "jpeg" : {
                "mime" : "image/jpeg",
                "extension" : [ "jpg" , "jpeg" ]
            },

            "png" : {
                "mime" : "image/png",
                "extension" : [ "png" ]
            },

            "html" : {
                "mime" : "test/html",
                "extension" : [ "htm", "html" ]
            }
        }

    @param registry     instance to create / fill
    @param path         path! to load
                        (will actually try to load all files from path with ext)
    @param ext          (optional) ext to load e.g. "json" or "mime" to
                        organize files in a file and identify them as config
                        files for the mime registry.
*/
bool ov_file_format_register_from_json_from_path(
    ov_file_format_registry **registry, const char *restrict path,
    const char *ext);

/*----------------------------------------------------------------------------*/

/**
    This function will parse some JSON object to the required data for
    ov_file_format.

    Valid input without ov_json_object_get(input, name):

            {
                "mime" : "image/jpeg",
                "extension" : [ "jpg" , "jpeg" ]
            }

    Valid input with ov_json_object_get(input, name):

        {
            "name" : {
                "mime" : "image/jpeg",
                "extension" : [ "jpg" , "jpeg" ]
            }
        }

    So some valid input is either the value content of some key "name",
    or some bigger JSON object containing several parameter, where content to
    parse is identified by name.

    @param input        json object to check
    @param name         format name to set / search
    @param parameter    pointer to parameter struct
    @param size         pointer to be set to size of extensions
                        NOTE will be checked before setting the array and
                        changed to the actual size of the array elements.
                        It is recommended to use some array of OV_FILE_EXT_MAX
                        as input to the function
    @param array        array to be filled with pointers to extension strings

    NOTE the output will contain pointers within the input JSON structure,
    so array[1] will actually point to the string "jpeg" within the array
    "extension" of the input JSON value.

    NOTE output lifetime is limited to the lifetime of the input value,
    due to pointer based usage.

    NOTE this function is external due to the fact to ADD some
    ov_format_handler before registering the format!

    NOTE this function requires "mime" to be set within the JSON object!
*/
bool ov_file_format_register_values_from_json(
    const ov_json_value *input, const char *name,
    ov_file_format_parameter *paramter, size_t *size, char *array[]);

/*----------------------------------------------------------------------------*/

/**
    Create some JSON value of form:

        {
            "mime" : "image/jpeg",
            "extension" : [ "jpg" , "jpeg" ]
        }

    @param json     optional JSON instance
    @param param    parameter to use (NOTE will create mime if present)
    @param size     size of extensions array (NOTE will fail if size == 0)
    @param array    extension array (NOTE will stop if array[i] == 0) without
                    failure

    @returns JSON representation of parameter and extensions

    NOTE this function allows "mime" to NOT be set within the JSON object!
*/
ov_json_value *
ov_file_format_register_values_to_json(ov_file_format_parameter param,
                                       size_t size, char const *array[]);

#endif /* ov_file_format_h */
