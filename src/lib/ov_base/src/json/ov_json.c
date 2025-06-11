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
        @file           ov_json.c
        @author         Markus Toepfer

        @date           2018-12-02

        @ingroup        ov_json

        ------------------------------------------------------------------------
*/

#include "../../include/ov_json.h"
#include <dirent.h>
#include <limits.h>

ov_json_value *ov_json_decode(const char *string) {

    if (!string) return NULL;

    int64_t len = strlen(string);
    ov_json_value *out = NULL;

    int64_t r = ov_json_parser_decode(&out, string, len);

    if (r != len) {
        out = ov_json_value_free(out);
        return NULL;
    }

    return out;
}

/*----------------------------------------------------------------------------*/

char *ov_json_encode(const ov_json_value *value) {

    return ov_json_value_to_string_with_config(
        value, ov_json_config_stringify_default());
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_json_read_file(const char *path) {

    char *buffer = NULL;
    ov_json_value *value = NULL;

    if (!path) goto error;

    size_t size = 0;
    size_t filesize = 0;
    size_t read = 0;

    if (access(path, F_OK) == -1) {

        ov_log_warning(
            "JSON READ, file (%s) "
            "does not exist or no access.",
            path);

        goto error;
    }

    FILE *fp = fopen(path, "r");

    if (fp != NULL) {

        if (fseek(fp, 0L, SEEK_END) == 0) {

            filesize = ftell(fp);

            if ((int)filesize == -1) {
                ov_log_error(
                    "JSON READ, file (%s) "
                    "could not read file size.",
                    path);
                fclose(fp);
                goto error;
            }

            size = filesize + 2;
            buffer = calloc(size + 1, sizeof(char));

            if (fseek(fp, 0L, SEEK_SET) == 0) {

                read = fread(buffer, sizeof(char), filesize, fp);

                if (read == 0) {

                    ov_log_error(
                        "JSON READ, file (%s) "
                        "could not read file.",
                        path);
                    fclose(fp);
                    goto error;
                }

            } else {

                ov_log_error(
                    "JSON READ, file (%s) "
                    "could not get back to start.",
                    path);

                fclose(fp);
                goto error;
            }
        }

        fclose(fp);

    } else {

        ov_log_error(
            "JSON READ, file (%s) "
            "could not open file.");

        fclose(fp);
        goto error;
    }

    int64_t r = ov_json_parser_decode(&value, buffer, size);

    if (r < 1) {

        ov_log_error(
            "JSON READ, file (%s) "
            "could not parse JSON.",
            path);
        goto error;
    }

    if (buffer) free(buffer);

    return value;

error:
    if (buffer) free(buffer);
    ov_json_value_free(value);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_json_read_dir(const char *path, const char *extension) {

    ov_json_value *value = NULL;
    ov_json_value *content = NULL;

    if (!path) goto error;

    errno = 0;

    DIR *dp;
    struct dirent *ep;

    size_t extlen = 0;
    size_t dirlen = strlen(path);

    char filename[PATH_MAX + 1];
    memset(filename, 0, PATH_MAX + 1);

    int len, i;

    if (extension) {
        extlen = strlen(extension);
        if (extlen == 0) goto error;
    }

    value = ov_json_object();
    if (!value) {

        ov_log_error("Could not create object");
        goto error;
    }

    dp = opendir(path);

    if (dp == NULL) {

        ov_log_debug(
            "JSON LOAD,"
            "could not open dir %s ERRNO %i | %s",
            path,
            errno,
            strerror(errno));
        goto error;
    }

    while ((ep = readdir(dp))) {

        content = NULL;
        memset(filename, 0, PATH_MAX);

        /*
         *  Do not try to read /. or /..
         */

        if (0 == strcmp(ep->d_name, ".") || (0 == strcmp(ep->d_name, "..")))
            continue;

        strcpy(filename, path);
        if (path[dirlen] != '/') strncat(filename, "/", PATH_MAX);

        strcat(filename, ep->d_name);

        len = strlen(ep->d_name);
        i = len;
        while (i > 0) {
            if (ep->d_name[i] == '.') break;
            i--;
        }

        if (extension) {

            if (1 > 0) {

                // file with extension
                if (0 == strncmp(ep->d_name + i + 1, extension, extlen)) {

                    // same extension as input extension
                    content = ov_json_read_file(filename);
                }
                // ignore files with wrong extension
            }
            // ignore files without extension

        } else {

            // read all
            content = ov_json_read_file(filename);
        }

        if (content) {

            if (!ov_json_object_set(value, ep->d_name, content)) {

                ov_log_debug(
                    "JSON LOAD, file (%s) "
                    "failure adding content.",
                    filename);

                content = ov_json_value_free(content);
            }
        }
    }

    (void)closedir(dp);

    return value;
error:
    ov_json_value_free(value);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_json_write_file(const char *path, const ov_json_value *value) {

    if (!path || !value) return false;

    ov_json_stringify_config config = ov_json_config_stringify_default();

    size_t count = 0;
    size_t size = ov_json_parser_calculate(value, &config);
    if (size < 1) return false;

    char buffer[size + 1];
    memset(buffer, 0, size + 1);

    int64_t r = ov_json_parser_encode(
        value, &config, ov_json_parser_collocate_ascending, buffer, size);
    if ((size_t)r != size) goto error;

    FILE *fp = fopen(path, "w");

    if (fp != NULL) {

        count = fprintf(fp, "%s\n", buffer);
        fclose(fp);

        if (count > 0) {
            /*
                        ov_log_debug("JSON WRITE, file (%s), "
                                     "wrote %jd bytes.",
                                     path,
                                     count);
            */
        } else {

            ov_log_error(
                "JSON WRITE, file (%s), "
                "could not write",
                path);

            goto error;
        }

    } else {

        ov_log_error(
            "JSON WRITE, file (%s), "
            "could not open path for write",
            path);

        goto error;
    }

    return true;

error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC default functionality
 *
 *      ------------------------------------------------------------------------
 */

ov_json_value *ov_json_read(const char *buffer, size_t size) {

    ov_json_value *value = NULL;

    if (!buffer || size < 1) goto error;

    int64_t r = ov_json_parser_decode(&value, buffer, size);
    if (r < 1) goto error;

    return value;
error:
    ov_json_value_free(value);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_json_write(ov_json_value *value,
                   char *buffer,
                   size_t size,
                   char **next) {

    if (!value || !buffer || size < 1) goto error;

    ov_json_stringify_config config = ov_json_config_stringify_default();

    int64_t r = ov_json_parser_encode(
        value, &config, ov_json_parser_collocate_ascending, buffer, size);
    if (r < 1) goto error;

    if (next) *next = buffer + r;

    return true;

error:
    if (next) *next = buffer;

    return false;
}
