/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_file.c
        @author         Markus Toepfer

        @date           2019-07-23

        @ingroup        ov_websocket

        @brief          Implementation of


        ------------------------------------------------------------------------
*/
#include "../../include/ov_file.h"

#include <stdbool.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../include/ov_utils.h"
#include <errno.h>
#include <ov_arch/ov_byteorder.h>
#include <ov_log/ov_log.h>
#include <string.h>

/*----------------------------------------------------------------------------*/

bool ov_file_exists(char const *path) {

    if (0 == path) return false;

    return 0 == access(path, F_OK);
}

/*----------------------------------------------------------------------------*/

const char *ov_file_read_check(const char *restrict path) {

    struct stat statbuf = {0};
    errno = 0;

    /* Never pass 0 pointers to system calls  except expicitly
     * allowed */
    if (0 == path) return OV_FILE_ACCESS_FAILED;

    if (access(path, F_OK) == -1) return OV_FILE_ACCESS_FAILED;

    if (0 != stat(path, &statbuf)) {
        return strerror(errno);
    }

    mode_t mode = statbuf.st_mode & S_IFMT;

    if (mode == S_IFDIR) {
        /* Is special, because open(2) will work perfectly on dirs */
        return OV_FILE_IS_DIR;
    }

    if ((mode == S_IFIFO) || (mode == S_IFREG) || (mode == S_IFSOCK)) {

        int fd = -1;
        fd = open(path, O_RDONLY);

        if (0 > fd) {
            return OV_FILE_COULD_NOT_READ;
        }

        close(fd);

        return 0;
    }

    return OV_FILE_UNKNOWN_ERROR;
}

/*----------------------------------------------------------------------------*/

ssize_t ov_file_read_check_get_bytes(const char *restrict path) {

    struct stat statbuf = {0};
    errno = 0;

    /* Never pass 0 pointers to system calls  except expicitly
     * allowed */
    if (0 == path) goto error;

    if (access(path, F_OK) == -1) goto error;

    if (0 != stat(path, &statbuf)) {
        goto error;
    }

    mode_t mode = statbuf.st_mode & S_IFMT;

    if (mode == S_IFDIR) {
        /* Is special, because open(2) will work perfectly on dirs */
        goto error;
    }

    if ((mode == S_IFIFO) || (mode == S_IFREG) || (mode == S_IFSOCK)) {

        int fd = -1;
        fd = open(path, O_RDONLY);

        if (0 > fd) {
            goto error;
        }

        close(fd);

        return statbuf.st_size;
    }

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

ov_file_handle_state ov_file_read(const char *path,
                                  uint8_t **buffer,
                                  size_t *size) {

    ov_file_handle_state state = OV_FILE_ERROR;

    bool created = false;

    FILE *fp = NULL;

    if (!path || !buffer || !size) goto error;

    const char *failure = ov_file_read_check(path);
    if (failure) {

        ov_log_error(
            "READ, file (%s) "
            " %s.",
            path,
            failure);

        goto error;
    }

    fp = fopen(path, "r");
    if (!fp) {
        state = OV_FILE_FAILURE;
        goto error;
    }

    size_t filesize = 0;
    size_t read = 0;
    size_t sz = 0;

    uint8_t *buf = NULL;

    if (fp != NULL) {

        if (fseek(fp, 0L, SEEK_END) == 0) {

            filesize = ftell(fp);

            if ((int)filesize == -1) {

                ov_log_error(
                    "READ, file (%s) "
                    "could not read file size.",
                    path);

                state = OV_FILE_FAILURE;
                goto error;
            }

            sz = filesize + 2;

            if (*buffer) {

                if (*size < sz) {
                    state = OV_FILE_MEMORY_ERROR;
                    goto error;
                }

            } else {

                *buffer = calloc(sz + 1, sizeof(char));
                if (!*buffer) {
                    state = OV_FILE_MEMORY_ERROR;
                    goto error;
                }

                created = true;
            }

            buf = *buffer;
            *size = filesize;

            memset(buf, 0, sz);

            if (fseek(fp, 0L, SEEK_SET) == 0) {

                read = fread(buf, sizeof(char), filesize, fp);

                if (read != filesize) {

                    ov_log_error(
                        "READ, file (%s) "
                        "could not read file.",
                        path);

                    state = OV_FILE_FAILURE;
                    goto error;
                }

            } else {

                ov_log_error(
                    "READ, file (%s) "
                    "could not get back to start.",
                    path);

                state = OV_FILE_FAILURE;
                goto error;
            }

        } else {

            ov_log_error(
                "READ, file (%s) "
                "could not find EOF",
                path);

            state = OV_FILE_FAILURE;
            goto error;
        }

    } else {

        ov_log_error(
            "READ, file (%s) "
            "could not open file.",
            path);

        state = OV_FILE_FAILURE;
        goto error;
    }

    if (fp) fclose(fp);
    return OV_FILE_SUCCESS;
error:
    if (created && buffer) {
        free(*buffer);
        *buffer = NULL;
    }

    if (fp) fclose(fp);
    return state;
}

/*----------------------------------------------------------------------------*/

ov_file_handle_state ov_file_read_partial(const char *path,
                                          uint8_t **buffer,
                                          size_t *size,
                                          size_t from,
                                          size_t to,
                                          size_t *all) {

    ov_file_handle_state state = OV_FILE_ERROR;

    bool created = false;

    FILE *fp = NULL;

    if (!path || !buffer || !size) goto error;

    const char *failure = ov_file_read_check(path);
    if (failure) {

        ov_log_error(
            "READ, file (%s) "
            " %s.",
            path,
            failure);

        goto error;
    }

    fp = fopen(path, "r");
    if (!fp) {
        state = OV_FILE_FAILURE;
        goto error;
    }

    size_t filesize = 0;
    size_t read = 0;
    size_t sz = to - from;

    uint8_t *buf = NULL;

    if (fp != NULL) {

        if (fseek(fp, 0L, SEEK_END) == 0) {

            filesize = ftell(fp);

            if ((int)filesize == -1) {

                ov_log_error(
                    "READ, file (%s) "
                    "could not read file size.",
                    path);

                state = OV_FILE_FAILURE;
                goto error;
            }

            if (0 == to) {
                to = filesize;
                sz = to - from;
            }

            if (*buffer) {

                if (*size < sz) {
                    state = OV_FILE_MEMORY_ERROR;
                    goto error;
                }

            } else {

                *buffer = calloc(sz + 1, sizeof(char));
                if (!*buffer) {
                    state = OV_FILE_MEMORY_ERROR;
                    goto error;
                }

                created = true;
            }

            buf = *buffer;
            *size = sz;

            memset(buf, 0, sz);

            if (fseek(fp, from, SEEK_SET) == 0) {

                read = fread(buf, sizeof(char), sz, fp);

                if (read != sz) {

                    ov_log_error(
                        "READ, file (%s) "
                        "could not read file.",
                        path);

                    state = OV_FILE_FAILURE;
                    goto error;
                }

            } else {

                ov_log_error(
                    "READ, file (%s) "
                    "could not get back to start.",
                    path);

                state = OV_FILE_FAILURE;
                goto error;
            }

        } else {

            ov_log_error(
                "READ, file (%s) "
                "could not find EOF",
                path);

            state = OV_FILE_FAILURE;
            goto error;
        }

    } else {

        ov_log_error(
            "READ, file (%s) "
            "could not open file.",
            path);

        state = OV_FILE_FAILURE;
        goto error;
    }

    if (fp) fclose(fp);

    *all = filesize;
    return OV_FILE_SUCCESS;
error:
    if (created && buffer) {
        free(*buffer);
        *buffer = NULL;
    }

    if (fp) fclose(fp);
    return state;
}

/*----------------------------------------------------------------------------*/

ov_file_handle_state ov_file_write(const char *path,
                                   const uint8_t *buffer,
                                   size_t size,
                                   const char *mode) {

    ov_file_handle_state state = OV_FILE_ERROR;
    FILE *fp = NULL;

    if (!path || !buffer || size < 1) goto error;

    if (!mode) mode = "w";

    fp = fopen(path, mode);

    size_t count = 0;

    if (fp != NULL) {

        count = fwrite(buffer, sizeof(char), size, fp);

        if (count == size) {

            ov_log_debug(
                "WRITE, file (%s), "
                "wrote %jd bytes.",
                path,
                count);

            state = OV_FILE_SUCCESS;

        } else {

            ov_log_error(
                "WRITE, file (%s), "
                "could not write all bytes, wrote %i/%i",
                path,
                count,
                size);

            state = OV_FILE_FAILURE;
        }

    } else {

        ov_log_error(
            "WRITE, file (%s), "
            "could not open path for write",
            path);

        state = OV_FILE_FAILURE;
    }

    //  current state
error:
    if (fp) fclose(fp);
    return state;
}

/*----------------------------------------------------------------------------*/

bool ov_file_get_16(uint16_t *out,
                    uint8_t **rd_ptr,
                    size_t *length,
                    ov_file_byteorder byte_order) {

    if (0 == out) return false;
    if (0 == rd_ptr) return false;
    if (0 == *rd_ptr) return false;
    if (0 == length) return false;
    if (2 > *length) return false;

    uint16_t read_16 = *((uint16_t *)(*rd_ptr));

    *rd_ptr += 2;
    *length -= 2;

    switch (byte_order) {

        case OV_FILE_RAW:

            break;

        case OV_FILE_SWAP_BYTES:
            read_16 = OV_SWAP_16(read_16);
            break;

        case OV_FILE_LITTLE_ENDIAN:

            read_16 = OV_LE16TOH(read_16);
            break;

        case OV_FILE_BIG_ENDIAN:

            read_16 = OV_BE16TOH(read_16);
            break;

        default:

            OV_ASSERT(!"MUST NEVER HAPPEN");
    };

    *out = read_16;

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_file_get_32(uint32_t *out,
                    uint8_t **rd_ptr,
                    size_t *length,
                    ov_file_byteorder byte_order) {

    if (0 == out) return false;
    if (0 == rd_ptr) return false;
    if (0 == *rd_ptr) return false;
    if (0 == length) return false;
    if (4 > *length) return false;

    uint32_t read_32 = *((uint32_t *)(*rd_ptr));

    *rd_ptr += 4;
    *length -= 4;

    switch (byte_order) {

        case OV_FILE_RAW:
            break;

        case OV_FILE_SWAP_BYTES:
            read_32 = OV_SWAP_32(read_32);
            break;

        case OV_FILE_LITTLE_ENDIAN:

            read_32 = OV_LE32TOH(read_32);
            break;

        case OV_FILE_BIG_ENDIAN:

            read_32 = OV_BE32TOH(read_32);
            break;

        default:

            OV_ASSERT(!"MUST NEVER HAPPEN");
    };

    *out = read_32;

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_file_write_16(uint8_t **write_ptr, size_t *length, uint16_t value) {

    if (0 == write_ptr) return false;
    if (0 == length) return false;
    if (2 > *length) return false;

    *(uint16_t *)(*write_ptr) = value;

    *write_ptr += sizeof(uint16_t);
    *length -= sizeof(uint16_t);

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_file_write_32(uint8_t **write_ptr, size_t *length, uint32_t value) {

    if (0 == write_ptr) return false;
    if (0 == length) return false;
    if (2 > *length) return false;

    *(uint32_t *)(*write_ptr) = value;

    *write_ptr += sizeof(uint32_t);
    *length -= sizeof(uint32_t);

    return true;
}

/*----------------------------------------------------------------------------*/
