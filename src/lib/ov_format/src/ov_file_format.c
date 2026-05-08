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
        @file           ov_file_format.c
        @author         Markus Töpfer

        @date           2021-01-02


        ------------------------------------------------------------------------
*/
#include "../include/ov_file_format.h"

#include <ov_base/ov_utils.h>

#include <ctype.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "../include/ov_format_registry.h"
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_constants.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_file.h>
#include <ov_base/ov_utf8.h>

/*----------------------------------------------------------------------------*/

#define OV_FILE_FORMAT_MAGIC_BYTE 0x1f0A

#define OV_FILE_FORMAT_REGISTRY_MAGIC_BYTE 0x7513

/*----------------------------------------------------------------------------*/

struct ov_file_format_registry_struct {

    const uint16_t magic_byte;

    ov_dict *format;
    ov_dict *extensions;

    ov_format_registry *format_handler;
};

/*----------------------------------------------------------------------------*/

static const ov_file_format_registry init_reg = (ov_file_format_registry){

    .magic_byte = OV_FILE_FORMAT_REGISTRY_MAGIC_BYTE};

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_format *ov_file_format_as(ov_format *f, char const *file_type, void *options,
                             ov_file_format_registry *registry) {

    if (!registry || !registry->format_handler)
        return NULL;

    return ov_format_as(f, file_type, options, registry->format_handler);
}

/*----------------------------------------------------------------------------*/

static bool parse_extensions(ov_file_desc *desc, const char *restrict path,
                             const char stop) {

    /*
     *  This function is intendet to be used to parse dot separated strings.
     *  Strings MAY be unicode, UTF8 encoded, ASCII chars or whatever ...
     *  this function will dot separate strings. NOT more NOT less.
     *
     *  This function is OS independent, as the path is ignored but may be input
     *  as stop character set to either / or \ for LINUX, MacOS, WINDOWS or
     *  whatever other OS is using.
     */

    if (!desc || !path)
        goto error;

    /* (1) clean extensions */

    memset(desc->ext, 0, sizeof(desc->ext));

    /* (2) walk extensions from last to first */

    char *start = (char *)path;
    size_t size = strlen(start);
    size_t item = 0;
    size_t len = 0;

    char *last = start + size;
    char *ptr = NULL;

    for (size_t i = size; i > 0; i--) {

        if (0 != stop)
            if (start[i] == stop)
                break;

        if (start[i] != '.')
            continue;

        if (i > 0)
            if (start[i - 1] == '.')
                break;

        ptr = start + i + 1;
        len = last - ptr;

        if ((0 == len) && item > 0)
            goto error;

        OV_ASSERT(len < OV_FILE_EXT_STRING_MAX);

        if (len > OV_FILE_EXT_STRING_MAX)
            goto error;
        /*
         *  Alternative char based copy?
         *
         *  Could be used, if it is not ensured the 2nd dimension is
         *  some consistent memory area at [i]
         *
         *  for (size_t j = 0; j < len; j++)
         *      desc->ext[item][j] = ptr[j];
         */

        if (!memcpy(desc->ext[item], ptr, len))
            goto error;

        for (size_t x = 0; x < len; x++) {

            if (isalpha(desc->ext[item][x]))
                desc->ext[item][x] = tolower(desc->ext[item][x]);
        }

        last = start + i;
        item++;
    }

    return true;
error:
    if (desc)
        memset(desc->ext, 0, sizeof(desc->ext));
    return false;
}

/*----------------------------------------------------------------------------*/

ov_file_desc ov_file_desc_from_path(const char *restrict path) {

    ov_file_desc desc = (ov_file_desc){0};

    if (!path)
        goto error;

    desc.bytes = ov_file_read_check_get_bytes(path);
    if (desc.bytes < 0)
        goto error;

    /* valid and readable file */

    if (!parse_extensions(&desc, path, OV_PATH_DELIMITER))
        goto error;

    return desc;
error:
    return (ov_file_desc){.bytes = -1};
}

/*----------------------------------------------------------------------------*/

bool ov_file_encoding_is_utf8(const char *restrict path) {

    int fd = -1;
    uint8_t *start = NULL;
    size_t size = 0;

    /*  NOTE
     *
     *  The file is checked at disk without reading it to memory.
     *  This may be slower as copying to memory for small files,
     *  but avoids to load potential huge files into memory,
     *  just to check if the encoding is some valid UTF-8 sequence.
     */

    if (!path)
        goto error;

    fd = open(path, O_RDONLY | O_CLOEXEC);
    if (0 > fd) {

        ov_log_error("%s open failed: %s", path, strerror(errno));
        goto error;
    }

    struct stat statbuf = {0};

    if (0 != fstat(fd, &statbuf)) {

        ov_log_error("%s fstat failed: %s", path, strerror(errno));
        goto error;
    }

    int prot_mode = PROT_READ;
    int mflags = MAP_SHARED;

    size = statbuf.st_size;
    start = mmap(NULL, size, prot_mode, mflags, fd, 0);

    if (start == MAP_FAILED) {
        /* avoid unmap of MAP_FAILED */
        start = NULL;
        ov_log_error("%s mmap failed: %s", path, strerror(errno));
        goto error;
    }

    /* NOTE size may be bigger as the actual content,
     * dependent on the actual page size,
     * but zero-filled as stated in man mmap.
     * Anyway, zero is a valid UTF-8 sequenz,
     * so we can check the whole size */

    if (!ov_utf8_validate_sequence(start, size)) {

        ov_log_error("%s not UTF8", path);
        goto error;
    }

    munmap(start, size);
    start = NULL;

    close(fd);
    fd = -1;

    return true;
error:

    if (start)
        munmap(start, size);

    if (0 > fd)
        close(fd);

    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #REGISTER
 *
 *      ------------------------------------------------------------------------
 */

ov_file_format_registry *ov_file_format_registry_cast(void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data == OV_FILE_FORMAT_REGISTRY_MAGIC_BYTE)
        return (ov_file_format_registry *)data;

    return NULL;
}

/*----------------------------------------------------------------------------*/

static void *free_registry(void *data) {

    ov_file_format_registry *reg = ov_file_format_registry_cast(data);
    if (!reg)
        return data;

    reg->format = ov_dict_free(reg->format);
    reg->format_handler = ov_format_registry_clear(reg->format_handler);
    reg->extensions = ov_dict_free(reg->extensions);
    reg = ov_data_pointer_free(reg);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_file_format_registry *create_registry() {

    ov_file_format_registry *reg = calloc(1, sizeof(ov_file_format_registry));
    if (!reg)
        goto error;

    if (!memcpy(reg, &init_reg, sizeof(ov_file_format_registry))) {
        reg = ov_data_pointer_free(reg);
        goto error;
    }

    ov_dict_config config = ov_dict_string_key_config(255);
    config.value.data_function.free = ov_data_pointer_free;

    reg->format = ov_dict_create(config);
    reg->format_handler = ov_format_registry_create();

    config.value.data_function.free = NULL;
    reg->extensions = ov_dict_create(config);

    if (!reg->format || !reg->extensions)
        goto error;

    OV_ASSERT(ov_file_format_registry_cast(reg));
    OV_ASSERT(ov_dict_cast(reg->format));
    OV_ASSERT(0 != reg->format_handler);
    OV_ASSERT(ov_dict_cast(reg->extensions));

    return reg;
error:
    free_registry(reg);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_file_format_register(ov_file_format_registry **registry,
                             ov_file_format_parameter parameter,
                             size_t file_extension_size,
                             char const *file_extension_array[]) {

    ov_file_format_parameter *val = NULL;
    char *key = 0;

    if (!registry)
        goto error;

    ov_file_format_registry *reg = *registry;

    if (0 == parameter.name[0])
        goto error;

    if (!reg) {

        reg = create_registry();

        if (!reg)
            goto error;

        *registry = reg;
    }

    if (!ov_format_registry_unregister_type(parameter.name, 0,
                                            reg->format_handler)) {

        ov_log_error("Could not unregister format %s", parameter.name);
        goto error;
    }

    if (!ov_format_registry_register_type(parameter.name, parameter.handler,
                                          reg->format_handler)) {

        ov_log_error("%s failed to register as format", parameter.name);

        goto error;
    }

    /* Here we will work in override mode and support the latest added
     * format desc parser. */

    if (ov_dict_is_set(reg->format, parameter.name)) {

        ov_log_debug("%s already registered as format - overriding",
                     parameter.name);
    }

    key = strndup(parameter.name, OV_FILE_FORMAT_PARAMETER_NAME_MAX + 1);

    if (0 == key)
        goto error;

    val = calloc(1, sizeof(ov_file_format_parameter));
    OV_ASSERT(0 != val);

    *val = parameter;

    if (!ov_dict_set(reg->format, key, val, 0)) {
        val = ov_data_pointer_free(val);
        goto error;
    }

    /*
     *  Here we do set the pointer of val in two independent dicts!
     *
     *  This will enable fast access to ov_file_format_parameter over either
     *  the name OR the some file extension, but MUST ensure reg->format
     *  is valid as long as reg->extension is valid, that is the reason
     *  why ov_file_format_registry is private only and we dont support
     *  clearing the dicts externaly.
     */

    for (size_t i = 0; i < file_extension_size; i++) {

        if (0 == file_extension_array[i])
            break;

        char const *name = file_extension_array[i];

        if (strlen(name) > OV_FILE_EXT_STRING_MAX)
            goto error;

        if (name[0] == '.') {

            ov_log_debug("%s starts with . (dot) - ignoring as extension",
                         name);

            continue;
        }

        /* Here we will work in override mode and support the latest added
         * format desc parser for some extension. */

        key = strndup(name, OV_FILE_EXT_STRING_MAX + 1);
        if (!key)
            goto error;

        for (size_t i = 0; i < strlen(key); i++) {

            if (isalpha(key[i]))
                key[i] = tolower(key[i]);
        }

        if (ov_dict_is_set(reg->extensions, key)) {

            ov_log_debug("%s already registered as extension - overriding",
                         key);
        }

        if (!ov_dict_set(reg->extensions, key, val, 0))
            goto error;
    }

    return true;

error:

    key = ov_data_pointer_free(key);

    OV_ASSERT(0 == val);
    OV_ASSERT(0 == key);

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_file_format_free_registry(ov_file_format_registry **registry) {

    if (!registry)
        return true;

    if (!*registry)
        return true;

    if (!ov_file_format_registry_cast(*registry))
        return false;

    ov_file_format_registry *reg = *registry;
    *registry = NULL;

    OV_ASSERT(NULL == free_registry(reg));
    return true;
}

/*----------------------------------------------------------------------------*/

const ov_file_format_parameter *
ov_file_format_get(const ov_file_format_registry *registry, const char *name) {

    if (!registry || !name)
        return NULL;

    return (const ov_file_format_parameter *)ov_dict_get(registry->format,
                                                         name);
}

/*----------------------------------------------------------------------------*/

const ov_file_format_parameter *
ov_file_format_get_ext(const ov_file_format_registry *registry,
                       const char *name) {

    if (!registry || !name)
        return NULL;

    return (const ov_file_format_parameter *)ov_dict_get(registry->extensions,
                                                         name);
}

/*----------------------------------------------------------------------------*/

ov_file_format_desc
ov_file_format_get_desc(const ov_file_format_registry *registry,
                        const char *restrict path) {

    ov_file_format_desc format = (ov_file_format_desc){0};

    format.desc = ov_file_desc_from_path(path);
    if (-1 == format.desc.bytes)
        goto error;

    if (0 == format.desc.ext[0][0]) {

        /* Some file request at path without any file extension */

    } else {

        const ov_file_format_parameter *para =
            ov_file_format_get_ext(registry, format.desc.ext[0]);

        if (!para) {

            /* Some unregistered extension used */

        } else if (0 == para->mime[0]) {

            /* No MIME type used during registration */

        } else {

            /* Copy MIME type */
            if (!strncpy(format.mime, para->mime, OV_FILE_FORMAT_MIME_MAX))
                goto error;
        }
    }

    return format;

error:
    return (ov_file_format_desc){.desc.bytes = -1};
}

/*----------------------------------------------------------------------------*/

static bool check_is_json_string(void *val, void *data) {

    if (ov_json_is_string(val))
        return true;

    ov_log_error("NOT a json string");
    if (data) { /* unused */
    };
    return false;
}

/*----------------------------------------------------------------------------*/

static bool check_mime_object(void *val) {

    if (!ov_json_is_object(val)) {
        ov_log_error("... pair content not object");
        goto error;
    }

    size_t count = ov_json_object_count(val);

    if (2 != count) {
        ov_log_error("... pair expected 2 keys | %zu keys present", count);
        goto error;
    }

    ov_json_value *value = ov_json_object_get(val, OV_KEY_MIME);

    if (!value || !ov_json_is_string(value)) {
        ov_log_error("... pair content %s not string or not set", OV_KEY_MIME);
        goto error;
    }

    value = ov_json_object_get(val, OV_KEY_EXTENSION);

    if (!value || !ov_json_is_array(value)) {
        ov_log_error("... pair content %s not array or not set",
                     OV_KEY_EXTENSION);
        goto error;
    }

    if (!ov_json_array_for_each(value, NULL, check_is_json_string)) {
        ov_log_error("... pair content %s content failure", OV_KEY_EXTENSION);
        goto error;
    }

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool check_pairs(const void *key, void *val, void *data) {

    if (!key)
        return true;

    if (data) { /* unused */
    }

    if (!check_mime_object(val)) {
        ov_log_error("Failure in pair %s", (char *)key);
        return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_file_format_register_values_from_json(
    const ov_json_value *input, const char *name,
    ov_file_format_parameter *parameter, size_t *size, char *array[]) {

    if (!input || !size || !array || !parameter)
        goto error;

    /* Use desired input data */

    const ov_json_value *data = ov_json_object_get(input, name);
    if (!data)
        data = input;

    if (!check_mime_object((void *)data))
        goto error;

    const char *mime =
        ov_json_string_get(ov_json_object_get(data, OV_KEY_MIME));
    if (!mime)
        goto error;

    memset(parameter->mime, 0, OV_FILE_FORMAT_MIME_MAX);
    memset(parameter->name, 0, OV_FILE_FORMAT_TYPE_NAME_MAX);

    if (strlen(mime) > OV_FILE_FORMAT_MIME_MAX)
        goto error;

    if (!snprintf(parameter->mime, OV_FILE_FORMAT_MIME_MAX, "%s", mime))
        goto error;

    if (name) {

        if (strlen(name) > OV_FILE_FORMAT_TYPE_NAME_MAX)
            goto error;

        if (!snprintf(parameter->name, OV_FILE_FORMAT_TYPE_NAME_MAX, "%s",
                      name))
            goto error;
    }

    ov_json_value *arr = ov_json_object_get(data, OV_KEY_EXTENSION);

    size_t count = ov_json_array_count(arr);

    if (*size < count)
        goto error;

    *size = count;

    for (size_t i = 0; i < count; i++) {
        array[i] = (char *)ov_json_string_get(ov_json_array_get(arr, i + 1));
        if (NULL == array[i])
            goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *
ov_file_format_register_values_to_json(ov_file_format_parameter parameter,
                                       size_t size, char const *array[]) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *arr = NULL;

    if ((0 == size) || (!array))
        goto error;

    out = ov_json_object();
    if (!out)
        goto error;

    val = ov_json_array();
    if (!val)
        goto error;

    if (!ov_json_object_set(out, OV_KEY_EXTENSION, val))
        goto error;

    arr = val;
    val = NULL;

    if (0 != parameter.mime[0]) {

        val = ov_json_string(parameter.mime);

        if (!ov_json_object_set(out, OV_KEY_MIME, val))
            goto error;
    }

    for (size_t i = 0; i < size; i++) {

        if (array[i] == 0)
            break;

        val = ov_json_string(array[i]);
        if (!ov_json_array_push(arr, val))
            goto error;
    }

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool validate_file_content_is_mime_format(const void *key, void *val,
                                                 void *data) {

    if (!key)
        return true;

    /* INPUT will be:
     *
     *  key = filename
     *  val = {
     *          "format1" : {
     *              "mime" : "<mine>",
     *              "extension" : [ ... ]
     *          },
     *          "format2" : { ... },
     *          "format3" : { ... }
     *  }
     */

    if (!ov_json_object_for_each(val, NULL, check_pairs))
        goto error;

    return true;
error:
    if (key)
        ov_log_error("Failure in file %s", (char *)key);

    if (data) { /* ignore */
    };
    return false;
}

/*----------------------------------------------------------------------------*/

static bool register_pairs(const void *key, void *val, void *data) {

    /* INPUT will be:
     *
     *  key = formatname
     *  val = {
     *      "mime" : "<mine>",
     *      "extension" : [ ... ]
     *  }
     */

    if (!key)
        return true;

    if (!data) {
        OV_ASSERT(data);
        ov_log_error("Register pairs without registry");
        return false;
    }

    const char *name = (char *)key;
    const char *mime = ov_json_string_get(ov_json_object_get(val, OV_KEY_MIME));
    if (!mime) {
        ov_log_error("Failure %s key %s not string", name, OV_KEY_MIME);
        return false;
    }

    ov_json_value *arr = ov_json_object_get(val, OV_KEY_EXTENSION);

    size_t size = ov_json_array_count(arr);

    char const *items[size];

    if (size < 1)
        goto error;

    for (size_t i = 0; i < size; i++) {
        items[i] = (char *)ov_json_string_get(ov_json_array_get(arr, i + 1));
        if (NULL == items[i])
            goto error;
    }

    ov_file_format_parameter param = {0};

    if (strlen(name) >= OV_FILE_FORMAT_TYPE_NAME_MAX) {
        ov_log_error("%s to long", name);
        goto error;
    }

    if (strlen(mime) >= OV_FILE_FORMAT_MIME_MAX) {
        ov_log_error("%s to long", mime);
        goto error;
    }

    if (!strncat(param.name, name, OV_FILE_FORMAT_TYPE_NAME_MAX - 1)) {
        ov_log_error("Failed to set name %s", name);
        goto error;
    }

    if (!strncat(param.mime, mime, OV_FILE_FORMAT_MIME_MAX - 1)) {
        ov_log_error("Failed to set mime %s", mime);
        goto error;
    }

    if (!ov_file_format_register((ov_file_format_registry **)data, param, size,
                                 items))
        goto error;

    // ov_log_debug("registered format |%s|\n", name);
    return true;
error:
    if (key)
        ov_log_error("Failed to register %s", (char *)key);

    return false;
}

/*----------------------------------------------------------------------------*/

static bool register_file_content(const void *key, void *val, void *data) {

    if (!key)
        return true;

    if (!val || !data)
        return false;

    /* INPUT will be:
     *
     *  key = filename
     *  val = {
     *          "format1" : {
     *              "mime" : "<mine>",
     *              "extension" : [ ... ]
     *          },
     *          "format2" : { ... },
     *          "format3" : { ... }
     *  }
     */

    return ov_json_object_for_each(val, data, register_pairs);
}

/*----------------------------------------------------------------------------*/

bool ov_file_format_register_from_json_from_path(
    ov_file_format_registry **registry, const char *restrict path,
    const char *ext) {

    ov_json_value *data = NULL;

    if (!registry || !path)
        goto error;

    data = ov_json_read_dir(path, ext);
    if (!data) {
        ov_log_error("Could not read JSON at %s ext %s", path, ext);
        goto error;
    }

    if (!ov_json_object_for_each(data, NULL,
                                 validate_file_content_is_mime_format)) {
        ov_log_error("Validate file content failed for %s | %s", path, ext);
        goto error;
    }

    if (!ov_json_object_for_each(data, registry, register_file_content)) {
        ov_log_error("Register file content failed for %s | %s", path, ext);
        goto error;
    }

    ov_json_value_free(data);

    ov_log_debug("registered formats with ext |%s| from path |%s|\n", ext,
                 path);
    return true;
error:
    ov_json_value_free(data);
    return false;
}
