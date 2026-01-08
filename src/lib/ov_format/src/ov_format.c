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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "../include/ov_format.h"

#include <errno.h>
#include <ov_base/ov_utils.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include <ov_log/ov_log.h>

/*----------------------------------------------------------------------------*/

ov_format_mode ov_format_get_mode(ov_format *self) {

    if (!ov_ptr_valid(self, "Invalid format: 0 pointer")) {
        return OV_INVALID;
    } else {
        return self->mode;
    }
}

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_format public;

    ov_buffer (*read_next_payload_chunk)(ov_format *, size_t, void *);
    ssize_t (*write_payload_chunk)(ov_format *, ov_buffer const *, void *);

    ssize_t (*overwrite)(ov_format *f, size_t offset, ov_buffer const *chunk,
                         void *data);

    void *(*free_data)(void *);

    bool (*ready_format)(ov_format *f, void *data);

    bool (*has_more_data)(ov_format const *);

    ov_format *(*responsible_for)(ov_format const *f, char const *type_name);
    void *data;

    /*
     * Maps to lower layer format.
     * For bare formats, points back to itself
     */
    ov_format *lower_layer;

} internal;

/*****************************************************************************
                                 MEMORY Format
 ****************************************************************************/

typedef struct {
    uint8_t *start;
    uint8_t **start_tracker;
    size_t size;
    uint8_t *access_pointer;
    uint8_t **end_tracker;
} mem_data;

/*----------------------------------------------------------------------------*/

char const *MEM_TYPE = "mem";

/*----------------------------------------------------------------------------*/

static internal *is_mem(ov_format const *f) {

    if (0 == f)
        return 0;

    if (MEM_TYPE == f->type)
        return (internal *)f;

    return 0;
}

/*----------------------------------------------------------------------------*/

static mem_data *as_mem_data(void *data) {

    if (0 == data)
        return data;

    return data;
}

/*----------------------------------------------------------------------------*/

static mem_data *get_mem_data(ov_format *f) {

    internal *iformat = is_mem(f);

    if (0 == iformat) {

        ov_log_error("Expected mem format");
        goto error;
    }

    return as_mem_data(iformat->data);

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

static bool set_access_ptr(mem_data *mem, uint8_t *new_access_ptr) {

    if (0 != mem) {

        mem->access_pointer = new_access_ptr;

        if (mem->end_tracker) {
            *mem->end_tracker = new_access_ptr;
        }

        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static ov_buffer read_next_payload_chunk_from_mem(mem_data *mem,
                                                  size_t requested_bytes) {

    if (0 == mem) {

        goto error;
    }

    uint8_t *rd_ptr = mem->access_pointer;
    uint8_t *base_ptr = mem->start;
    size_t size_bytes = mem->size;

    if (0 == mem->start) {

        OV_ASSERT(!"Only mem/mem-mapped supported currently!");
        goto error;
    }

    OV_ASSERT(rd_ptr >= base_ptr);
    OV_ASSERT(base_ptr + size_bytes >= rd_ptr);

    size_t bytes_available = base_ptr + size_bytes - rd_ptr;

    size_t bytes_to_read = bytes_available;

    if ((0 != requested_bytes) && (requested_bytes < bytes_to_read)) {

        bytes_to_read = requested_bytes;
    }

    ov_buffer result = {
        .start = rd_ptr,
        .length = bytes_to_read,
        .capacity = 0,
    };

    set_access_ptr(mem, mem->access_pointer + bytes_to_read);

    return result;

error:

    return (ov_buffer){0};
}

/*----------------------------------------------------------------------------*/

/* MEM read */

static ov_buffer
mem_read_next_payload_chunk(ov_format *f, size_t requested_bytes, void *data) {

    UNUSED(data);

    mem_data *mem = get_mem_data(f);

    return read_next_payload_chunk_from_mem(mem, requested_bytes);
}

/*----------------------------------------------------------------------------*/

static void *mem_free_data(void *data) {

    mem_data *mem = as_mem_data(data);

    if (0 == mem)
        return data;

    memset(mem, 0, sizeof(*mem));
    free(mem);

    mem = 0;

    return mem;
}

/*----------------------------------------------------------------------------*/

static bool mem_has_more_data(ov_format const *f) {

    mem_data *mem = as_mem_data(ov_format_get_custom_data(f));

    if (0 == mem)
        goto error;

    OV_ASSERT(mem->access_pointer >= mem->start);

    return mem->access_pointer < mem->start + mem->size;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static ov_format *mem_format_read_create(uint8_t *memory, size_t length_bytes) {

    if (0 == memory) {

        ov_log_error("Wont create format for 0 pointer");
        goto error;
    }

    if (0 == length_bytes) {

        ov_log_error("Wont create format for zero - length memory");
        goto error;
    }

    internal *f = calloc(1, sizeof(internal));

    OV_ASSERT(0 != f);

    mem_data *mem = calloc(1, sizeof(mem_data));

    f->data = mem;

    mem->start = memory;
    mem->size = length_bytes;
    mem->access_pointer = memory;

    f->public = (ov_format){

        .type = MEM_TYPE,
        .mode = OV_READ,

    };

    f->free_data = mem_free_data;
    f->read_next_payload_chunk = mem_read_next_payload_chunk;
    f->write_payload_chunk = 0;
    f->has_more_data = mem_has_more_data;

    f->lower_layer = &f->public;

    return &f->public;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

/* MEM Write */

static ssize_t mem_write_payload_chunk_nocheck(mem_data *mem,
                                               ov_buffer const *chunk) {

    OV_ASSERT(0 != mem);
    OV_ASSERT(0 != chunk);

    if (0 == mem->start) {

        ov_log_error("Invalid format: No memory area to write to");
        goto error;
    }

    size_t num_bytes_writable = (mem->start + mem->size) - mem->access_pointer;

    size_t bytes_to_write = chunk->length;

    if (num_bytes_writable < bytes_to_write) {

        bytes_to_write = num_bytes_writable;
        ov_log_info("Requested to write %zu, but only %zu bytes writable",
                    chunk->length, bytes_to_write);
    }

    memcpy(mem->access_pointer, chunk->start, bytes_to_write);
    set_access_ptr(mem, mem->access_pointer + bytes_to_write);

    return bytes_to_write;

error:

    return -1;
}

/*----------------------------------------------------------------------------*/

static ssize_t mem_write_payload_chunk(ov_format *f, ov_buffer const *chunk,
                                       void *data) {

    UNUSED(data);

    if ((0 == chunk) || (0 == chunk->start)) {

        ov_log_error("chunk is a 0 pointer");
        goto error;
    }

    mem_data *mem = get_mem_data(f);

    if (0 == mem) {

        goto error;
    }

    if (f->mode != OV_WRITE) {

        ov_log_error("Format is not writable");
        goto error;
    }

    return mem_write_payload_chunk_nocheck(mem, chunk);

error:

    return -1;
}

/*----------------------------------------------------------------------------*/

static bool mem_extend_for(mem_data *mem, size_t bytes_to_write) {

    OV_ASSERT(0 != mem);

    size_t num_bytes_writable = (mem->start + mem->size) - mem->access_pointer;

    while (num_bytes_writable < bytes_to_write) {

        ptrdiff_t access_offset = mem->access_pointer - mem->start;

        const size_t new_size = mem->size * 2;

        OV_ASSERT(new_size > mem->size);

        if (new_size < mem->size) {

            ov_log_error("Fatal: Overflow in memory extension detected");
            goto error;
        }

        mem->start = realloc(mem->start, new_size);
        mem->size = new_size;
        set_access_ptr(mem, mem->start + access_offset);

        ov_log_info("Extended memory to write to to %zu", mem->size);

        num_bytes_writable = (mem->start + mem->size) - mem->access_pointer;
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static ssize_t mem_write_payload_chunk_auto_extend(ov_format *f,
                                                   ov_buffer const *chunk,
                                                   void *data) {

    if ((0 == chunk) || (0 == chunk->start)) {

        ov_log_error("chunk is a 0 pointer");
        goto error;
    }

    mem_data *mem = data;

    if (0 == mem) {

        goto error;
    }

    if (f->mode != OV_WRITE) {

        ov_log_error("Format is not writable");
        goto error;
    }

    if (0 == mem->start) {

        ov_log_error("Invalid format: no memory area to write to");
        goto error;
    }

    size_t bytes_to_write = chunk->length;

    if (!mem_extend_for(mem, bytes_to_write)) {
        goto error;
    }

    memcpy(mem->access_pointer, chunk->start, bytes_to_write);
    set_access_ptr(mem, mem->access_pointer + bytes_to_write);

    return bytes_to_write;

error:

    return -1;
}

/*----------------------------------------------------------------------------*/

static ssize_t mem_overwrite_payload_chunk(ov_format *f, size_t offset,
                                           ov_buffer const *chunk, void *data) {

    if ((0 == chunk) || (0 == chunk->start)) {

        ov_log_error("chunk is a 0 pointer");
        goto error;
    }

    mem_data *mem = data;

    if (0 == mem) {

        goto error;
    }

    if (f->mode != OV_WRITE) {

        ov_log_error("Format is not writable");
        goto error;
    }

    if (0 == mem->start) {

        ov_log_error("Invalid format: no memory area to write to");
        goto error;
    }

    uint8_t *write_ptr = mem->start + offset;

    if (write_ptr + chunk->length > mem->access_pointer) {

        ov_log_error("Writing out - of - bounds");
        goto error;
    }

    memcpy(write_ptr, chunk->start, chunk->length);

    return chunk->length;

error:

    return -1;
}
/*----------------------------------------------------------------------------*/

static void *mem_write_free_data_auto_extend(void *data) {

    mem_data *mem = as_mem_data(data);

    if (0 == mem)
        return data;

    mem->start = ov_free(mem->start);

    return mem_free_data(data);
}

/*----------------------------------------------------------------------------*/

static ov_format *mem_format_write_create(uint8_t *memory,
                                          size_t length_bytes) {

    ssize_t (*write_chunk)(ov_format *, ov_buffer const *, void *) =
        mem_write_payload_chunk;

    void *(*free_data)(void *data) = mem_free_data;

    if (0 == length_bytes) {

        ov_log_error("Wont create format for zero - length memory");
        goto error;
    }

    if (0 == memory) {

        /* Auto-extend mode */

        memory = calloc(1, length_bytes);
        write_chunk = mem_write_payload_chunk_auto_extend;
        free_data = mem_write_free_data_auto_extend;
    }

    internal *f = calloc(1, sizeof(internal));

    OV_ASSERT(0 != f);

    mem_data *mem = calloc(1, sizeof(mem_data));

    f->data = mem;

    mem->start = memory;
    mem->size = length_bytes;
    mem->access_pointer = memory;

    f->public = (ov_format){

        .type = MEM_TYPE,
        .mode = OV_WRITE,

    };

    f->free_data = free_data;
    f->read_next_payload_chunk = 0;
    f->write_payload_chunk = write_chunk;
    f->overwrite = mem_overwrite_payload_chunk;
    f->has_more_data = 0;

    f->lower_layer = &f->public;

    return &f->public;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

/* General ov_format_from_memory */

ov_format *ov_format_from_memory(uint8_t *memory, size_t length_bytes,
                                 ov_format_mode mode) {

    if (OV_READ == mode) {

        return mem_format_read_create(memory, length_bytes);
    }

    if (OV_WRITE == mode) {

        return mem_format_write_create(memory, length_bytes);
    }

    return 0;
}

/*----------------------------------------------------------------------------*/

/* ov_format_get_memory */

static ov_buffer get_memory_from_lower_layer_nocheck(ov_format const *f) {

    internal *intformat = (internal *)f;
    ov_format *lower_fmt = intformat->lower_layer;

    OV_ASSERT(0 != lower_fmt);

    return ov_format_get_memory(intformat->lower_layer);
}

/*----------------------------------------------------------------------------*/

ov_buffer ov_format_get_memory(ov_format const *f) {

    ov_buffer mem_area = {0};

    if (0 == f) {

        ov_log_error("Got 0 pointer");
        goto error;
    }

    if (OV_WRITE != f->mode) {

        ov_log_error("get_memory only supported by writeable formats");
        goto error;
    }

    internal *i = (internal *)f;

    void *data = ov_format_get_custom_data(f);

    if ((0 != i->ready_format) && (!i->ready_format(i->lower_layer, data))) {

        ov_log_error("Could not ready format for data usage");
        goto error;
    }

    if (0 == is_mem(f)) {

        return get_memory_from_lower_layer_nocheck(f);
    }

    /* We are a memory format */

    mem_data const *mem = get_mem_data((ov_format *)f);

    if (0 == mem) {
        goto error;
    }

    mem_area.start = mem->start;
    mem_area.length = mem->access_pointer - mem_area.start;

error:

    return mem_area;
}

/*----------------------------------------------------------------------------*/

bool ov_format_attach_end_ptr_tracker(ov_format *fmt, uint8_t **tracker) {

    if (ov_ptr_valid(tracker, "Won't attach end ptr tracker: No tracker") &&
        ov_ptr_valid(fmt, "Cannot attach end tracker - no format") &&
        ov_cond_valid(OV_WRITE == fmt->mode,
                      "get_memory only supported by writeable formats")) {

        if (0 == is_mem(fmt)) {

            internal *intformat = (internal *)fmt;
            ov_format *lower_fmt = intformat->lower_layer;

            OV_ASSERT(0 != lower_fmt);

            return ov_format_attach_end_ptr_tracker(intformat->lower_layer,
                                                    tracker);

        } else {

            /* We are a memory format */

            mem_data *mem = get_mem_data((ov_format *)fmt);

            if (ov_ptr_valid(mem,
                             "Cannot attach end tracker: Internal error")) {

                mem->end_tracker = tracker;
                *tracker = mem->access_pointer;
                return true;

            } else {

                return false;
            }
        }
    } else {

        return false;
    }
}

/*****************************************************************************
                                  FILE Format
 ****************************************************************************/

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

/*----------------------------------------------------------------------------*/

static void *map_file_unsafe(char const *path, ov_format_mode mode,
                             int *filedesc, size_t *file_size) {

    int fd = -1;

    OV_ASSERT(0 != path);
    OV_ASSERT(0 != file_size);

    OV_ASSERT((0 != *file_size) || (OV_READ == mode));

    int fmode = O_RDONLY;

    switch (mode) {

    case OV_READ:

        fmode = O_RDONLY;
        break;

    case OV_WRITE:

        fmode = O_RDWR | O_CREAT | O_TRUNC;
        break;

    default:

        OV_ASSERT(!"MUST NEVER HAPPEN!");
    }

    fd = open(path, fmode | O_CLOEXEC, S_IRWXU);

    if (0 > fd) {
        ov_log_error("Could not open %s: %s", path, strerror(errno));
        goto error;
    }

    int prot_mode = PROT_READ;
    int mflags = MAP_SHARED;

    if (O_RDONLY == fmode) {

        struct stat statbuf = {0};

        if (0 != fstat(fd, &statbuf)) {

            ov_log_error("fstat failed: %s", strerror(errno));
            goto error;
        }

        *file_size = statbuf.st_size;

    } else {

        /* File has to be created / overwritten */

        if (0 > lseek(fd, *file_size, SEEK_SET)) {

            ov_log_error("Could not set file size on output file %s: %s", path,
                         strerror(errno));

            goto error;
        }

        /* If file is empty, mapping fails - don't know how to remove again
         * file size MUST be even, otherwise pcm16s decoding fails */
        char padding[2] = {0};

        if (2 != write(fd, padding, 2)) {

            ov_log_error("Could not create output file %s", path);
            goto error;
        }

        prot_mode |= PROT_WRITE;
    }

    char *addr = mmap(0, *file_size, prot_mode, mflags, fd, 0);

    if ((caddr_t)-1 == addr) {

        ov_log_error("Could not map file %s: %s", path, strerror(errno));
        goto error;
    }

    if (0 != filedesc) {

        *filedesc = fd;
    }

    return addr;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

static void unmap_file_unsafe(void *mapped_region, size_t length) {

    munmap(mapped_region, length);
}

/*----------------------------------------------------------------------------*/

typedef struct {

    mem_data mem;

    bool mapped;
    int filedescriptor;

} file_data;

/*----------------------------------------------------------------------------*/

char const *FILE_TYPE = "file";

/*----------------------------------------------------------------------------*/

static internal *is_file(ov_format const *f) {

    if (0 == f)
        return 0;

    if (FILE_TYPE == f->type)
        return (internal *)f;

    return 0;
}

/*----------------------------------------------------------------------------*/

static file_data *as_file_data(void *data) {

    if (0 == data)
        return data;

    return data;
}

/*----------------------------------------------------------------------------*/

static file_data *get_file_data(ov_format *f) {

    internal *iformat = is_file(f);

    if (0 == iformat) {

        ov_log_error("Expected bare format");
        goto error;
    }

    return as_file_data(iformat->data);

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

static ov_buffer
file_read_next_payload_chunk(ov_format *f, size_t requested_bytes, void *data) {

    UNUSED(data);

    file_data *file = get_file_data(f);

    if (0 == file) {

        ov_log_error("Require file format");
        goto error;
    }

    /* Currently, we support only memory mapped files. */
    return read_next_payload_chunk_from_mem(&file->mem, requested_bytes);

error:

    return (ov_buffer){0};
}

/*----------------------------------------------------------------------------*/

static ssize_t file_write_payload_chunk(ov_format *f, ov_buffer const *chunk,
                                        void *data) {

    if ((0 == chunk) || (0 == chunk->start)) {

        ov_log_error("chunk is a 0 pointer");
        goto error;
    }

    file_data *file = data;

    if (0 == file) {

        goto error;
    }

    if (f->mode != OV_WRITE) {

        ov_log_error("Format is not writable");
        goto error;
    }

    /* Currently mapped formats for writing is not supported */
    OV_ASSERT(!file->mapped);

    if (0 < file->filedescriptor) {

        return write(file->filedescriptor, chunk->start, chunk->length);
    }

error:

    return -1;
}

/*----------------------------------------------------------------------------*/

static ssize_t overwrite_on_fd_nocheck(int fd, size_t offset,
                                       ov_buffer const *chunk) {

    ssize_t bytes_written = -1;

    OV_ASSERT(0 < fd);
    OV_ASSERT(0 != chunk);
    OV_ASSERT(0 != chunk->start);

    struct stat statbuf = {0};

    if (0 != fstat(fd, &statbuf)) {

        ov_log_error("Could not get file size");
        goto error;
    }

    OV_ASSERT(0 <= statbuf.st_size);

    if (offset + chunk->length > (size_t)statbuf.st_size) {

        ov_log_error("File not big enough - need %zu bytes, only got %zu bytes",
                     offset + chunk->length, (size_t)statbuf.st_size);
        goto error;
    }

    if ((off_t)offset != lseek(fd, offset, SEEK_SET)) {

        ov_log_error("Could not seek to %zu", offset);
        bytes_written = -bytes_written;
        goto error_reset;
    }

    bytes_written = write(fd, chunk->start, chunk->length);

    if (bytes_written != (ssize_t)chunk->length) {

        ov_log_error("Could not write to file: %s", strerror(errno));
        bytes_written = -bytes_written;
        goto error_reset;
    }

error_reset:

    if (0 > lseek(fd, 0, SEEK_END)) {

        ov_log_error("Could not go back to end of file");
        goto error;
    }

error:

    return bytes_written;
}

/*----------------------------------------------------------------------------*/

static ssize_t file_overwrite(ov_format *f, size_t offset,
                              ov_buffer const *chunk, void *data) {

    file_data *fdata = data;

    if (0 == fdata) {

        goto error;
    }

    if (0 == f) {

        ov_log_error("No format to write to given");
    }

    if ((0 == chunk) || (0 == chunk->start)) {

        ov_log_error("No chunk to write given");
        goto error;
    }

    if (f->mode != OV_WRITE) {

        ov_log_error("Format is not writable");
        goto error;
    }

    /* Currently mapped formats for writing is not supported */
    OV_ASSERT(!fdata->mapped);

    if (0 < fdata->filedescriptor) {

        return overwrite_on_fd_nocheck(fdata->filedescriptor, offset, chunk);
    }

error:

    return -1;
}

/*----------------------------------------------------------------------------*/

static void *file_free_data(void *data) {

    file_data *file = as_file_data(data);

    if (0 == file)
        return data;

    if ((file->mapped) && (0 != file->mem.start)) {

        unmap_file_unsafe(file->mem.start, file->mem.size);
        file->mem.start = 0;
        file->mem.size = 0;
    }

    if (0 < file->filedescriptor) {

        close(file->filedescriptor);
        file->filedescriptor = -1;
    }

    memset(file, 0, sizeof(*file));

    file = ov_free(file);

    return file;
}

/*----------------------------------------------------------------------------*/

static bool file_has_more_data(ov_format const *f) {

    file_data *file = as_file_data(ov_format_get_custom_data(f));

    if (0 == file)
        goto error;

    return file->mem.access_pointer < file->mem.start + file->mem.size;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static ov_format *open_file_read(char const *path) {

    int fd = -1;
    size_t length_bytes = 0;

    void *content = map_file_unsafe(path, OV_READ, &fd, &length_bytes);

    if (0 == content) {

        ov_log_error("Could not open file %s", path);
        goto error;
    }

    internal *f = calloc(1, sizeof(internal));

    OV_ASSERT(0 != f);

    file_data *file = calloc(1, sizeof(file_data));

    f->data = file;

    file->mapped = true;
    file->mem.start = content;
    file->mem.size = length_bytes;
    file->mem.access_pointer = content;
    file->filedescriptor = fd,

    f->public = (ov_format){

        .type = FILE_TYPE,
        .mode = OV_READ,

    };

    f->free_data = file_free_data;
    f->read_next_payload_chunk = file_read_next_payload_chunk;
    f->write_payload_chunk = 0;
    f->has_more_data = file_has_more_data;

    f->lower_layer = &f->public;

    return &f->public;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

static ov_format *open_file_write(char const *path) {

    int fd = open(path, O_CREAT | O_WRONLY | O_CLOEXEC | O_TRUNC, S_IRWXU);

    if (0 >= fd) {

        ov_log_error("Could not open %s for writing: %s (%i)", path,
                     strerror(errno), errno);
        goto error;
    }

    internal *f = calloc(1, sizeof(internal));

    OV_ASSERT(0 != f);

    file_data *file = calloc(1, sizeof(file_data));

    f->data = file;

    file->mapped = false;
    file->mem.start = 0;
    file->mem.size = 0;
    file->mem.access_pointer = 0;
    file->filedescriptor = fd,

    f->public = (ov_format){

        .type = FILE_TYPE,
        .mode = OV_WRITE,

    };

    f->free_data = file_free_data;
    f->read_next_payload_chunk = 0;
    f->write_payload_chunk = file_write_payload_chunk;
    f->overwrite = file_overwrite;
    f->has_more_data = 0;

    f->lower_layer = &f->public;

    return &f->public;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

ov_format *ov_format_open(char const *path, ov_format_mode mode) {

    if (0 == path) {

        ov_log_error("No file path given");
        goto error;
    }

    if (OV_READ == mode) {

        return open_file_read(path);
    }

    if (OV_WRITE == mode) {

        return open_file_write(path);
    }

error:

    return 0;
}

/*****************************************************************************
                            General format functions
 ****************************************************************************/

ov_format *ov_format_close(ov_format *f) {

    internal *intformat = (internal *)f;
    f = 0;

    if (0 == intformat)
        goto finish;

    ov_format *lower_fmt = intformat->lower_layer;
    OV_ASSERT(0 != lower_fmt);

    if ((0 != intformat->ready_format) &&
        (!intformat->ready_format(lower_fmt, intformat->data))) {

        ov_log_error("While closing format: Could not ready its content for "
                     "usage");
    }

    if (0 != intformat->free_data) {

        intformat->data = intformat->free_data(intformat->data);
    }

    OV_ASSERT(0 == intformat->data);

    if (&intformat->public == intformat->lower_layer) {

        /* A bare format pints its lower_layer to itself ... */
        intformat->lower_layer = 0;
    }

    intformat->lower_layer = ov_format_close(intformat->lower_layer);

    OV_ASSERT(0 == intformat->lower_layer);

    intformat = ov_free(intformat);

finish:

    return (ov_format *)intformat;
}

/*----------------------------------------------------------------------------*/

ov_format *ov_format_close_non_recursive(ov_format *f) {

    internal *intformat = (internal *)f;

    if (0 == intformat)
        goto finish;

    if (0 != intformat->free_data) {

        intformat->data = intformat->free_data(intformat->data);
    }

    OV_ASSERT(0 == intformat->data);

    intformat = ov_free(intformat);

finish:

    return (ov_format *)intformat;
}

/*****************************************************************************
                                Format stacking
 ****************************************************************************/

static bool recursive_has_more_data(ov_format const *format) {

    return ov_format_has_more_data(format);
}

/*----------------------------------------------------------------------------*/

ov_format *ov_format_wrap(ov_format *f, char const *type,
                          ov_format_handler *handler, void *options) {

    internal *intformat = 0;

    if (0 == f) {

        ov_log_error("No lower format given");
        goto error;
    }

    if (0 == type) {

        ov_log_error("No type given (0 pointer)");
        goto error;
    }

    if (0 == handler) {

        ov_log_error("No format handler");
        goto error;
    }

    intformat = calloc(1, sizeof(internal));

    intformat->read_next_payload_chunk = handler->next_chunk;
    intformat->write_payload_chunk = handler->write_chunk;
    intformat->overwrite = handler->overwrite;
    intformat->ready_format = handler->ready_format;
    intformat->free_data = handler->free_data;
    intformat->responsible_for = handler->responsible_for;

    intformat->has_more_data = recursive_has_more_data;

    intformat->data = 0;

    if (0 != handler->create_data) {

        intformat->data = handler->create_data(f, options);
    }

    if ((0 != handler->create_data) && (0 == intformat->data)) {

        ov_log_error("Could not initialize format");
        goto error;
    }

    intformat->lower_layer = f;

    intformat->public = (ov_format){

        .type = type,
        .mode = f->mode,

    };

    return &intformat->public;

error:

    intformat = ov_free(intformat);

    return 0;
}

/*----------------------------------------------------------------------------*/

ov_format const *ov_format_get(ov_format const *f, char const *format_desc) {

    ov_format const *found = 0;

    if (0 == f)
        goto error;
    if (0 == format_desc)
        goto error;

    OV_ASSERT(0 != f);
    OV_ASSERT(0 != format_desc);

    if (0 == strncmp(f->type, format_desc, OV_FORMAT_TYPE_MAX_STR_LEN)) {

        goto we_are_requested;
    }

    internal *intformat = (internal *)f;
    OV_ASSERT(0 != intformat->lower_layer);

    if (0 != intformat->responsible_for) {

        found = intformat->responsible_for(f, format_desc);
    }

    if (0 != found)
        goto found_format;

    /* Are we the lowest layer already -> nothing suitable found */
    if (f == intformat->lower_layer) {

        ov_log_error(" No format %s found in format stack", format_desc);
        goto error;
    }

    /* Otherwise, look deeper */
    return ov_format_get(intformat->lower_layer, format_desc);

we_are_requested:

    found = f;

found_format:

    return found;

error:

    return 0;
}

/*****************************************************************************
                     Public - general functions on formats
 ****************************************************************************/

bool ov_format_has_more_data(ov_format const *f) {

    if (0 == f)
        goto error;

    internal *intformat = (internal *)f;

    OV_ASSERT(0 != intformat->lower_layer);

    if (0 == intformat->has_more_data)
        goto error;

    return intformat->has_more_data(intformat->lower_layer);

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static ov_buffer *buffer_wrap(ov_buffer *buf) {

    if (0 == buf)
        return 0;

    if (0 == buf->length)
        return 0;

    ov_buffer *ret_buffer = ov_buffer_create(buf->length);
    OV_ASSERT(buf->length <= ret_buffer->capacity);

    ret_buffer->length = buf->length;
    memcpy(ret_buffer->start, buf->start, buf->length);

    ov_buffer_clear(buf);

    return ret_buffer;
}

/*----------------------------------------------------------------------------*/

ov_buffer *ov_format_payload_read_chunk(ov_format *f, size_t requested_bytes) {

    if (0 == f)
        goto error;

    internal *intformat = (internal *)f;

    OV_ASSERT(0 != intformat->lower_layer);

    if (0 == intformat->read_next_payload_chunk)
        goto error;

    ov_buffer data = intformat->read_next_payload_chunk(
        intformat->lower_layer, requested_bytes, intformat->data);

    return buffer_wrap(&data);

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

ov_buffer ov_format_payload_read_chunk_nocopy(ov_format *f,
                                              size_t requested_bytes) {

    ov_buffer buf = {0};

    if (0 == f)
        goto error;

    internal *intformat = (internal *)f;

    OV_ASSERT(0 != intformat->lower_layer);

    if (0 == intformat->read_next_payload_chunk)
        goto error;

    buf = intformat->read_next_payload_chunk(intformat->lower_layer,
                                             requested_bytes, intformat->data);

    if (0 == buf.length) {

        buf.start = 0;
    }

error:

    return buf;
}

/*----------------------------------------------------------------------------*/

ssize_t ov_format_payload_write_chunk(ov_format *f, ov_buffer const *chunk) {

    if (0 == f) {
        ov_log_error("Format is a 0 pointer");
        goto error;
    }

    if (0 == chunk) {

        ov_log_error("Chunk is a 0 pointer");
        goto error;
    }

    if (0 == chunk->start) {

        ov_log_error("Chunk->start is a 0 pointer");
        goto error;
    }

    internal *intformat = (internal *)f;

    OV_ASSERT(0 != intformat->lower_layer);

    if (0 == intformat->write_payload_chunk)
        goto error;

    return intformat->write_payload_chunk(intformat->lower_layer, chunk,
                                          intformat->data);

error:

    return -1;
}

/*----------------------------------------------------------------------------*/

ssize_t ov_format_payload_overwrite(ov_format *f, size_t offset,
                                    ov_buffer const *chunk) {

    if (0 == f) {

        ov_log_error("Require format, got 0 pointer");
        goto error;
    }

    if ((0 == chunk) || (0 == chunk->start)) {

        ov_log_error("No chunk to write given");
        goto error;
    }

    internal *intformat = (internal *)f;

    OV_ASSERT(intformat->lower_layer);

    if (0 == intformat->overwrite) {

        ov_log_error("Overwrite not supported by format");
        goto error;
    }

    return intformat->overwrite(intformat->lower_layer, offset, chunk,
                                intformat->data);

error:

    return -1;
}

/*----------------------------------------------------------------------------*/

void *ov_format_get_custom_data(ov_format const *f) {

    if (0 == f)
        goto error;

    internal *intformat = (internal *)f;

    OV_ASSERT(0 != intformat->lower_layer);

    return intformat->data;

error:

    return 0;
}

/*****************************************************************************
                                Buffered Format
 ****************************************************************************/

static ov_buffer impl_buffered_read_next_payload_chunk(ov_format *f,
                                                       size_t requested_bytes,
                                                       void *data) {

    UNUSED(data);

    mem_data *mem = get_mem_data(f);

    if (0 == mem) {

        goto error;
    }

    uint8_t *base_ptr = mem->start;
    size_t bytes_available = mem->size;

    if (0 == mem->start) {

        OV_ASSERT(!"Only mem supported currently!");
        goto error;
    }

    size_t bytes_to_read = bytes_available;

    if ((0 != requested_bytes) && (requested_bytes < bytes_to_read)) {

        bytes_to_read = requested_bytes;
    }

    ov_buffer result = {
        .start = base_ptr,
        .length = bytes_to_read,
        .capacity = 0,
    };

    return result;

error:

    return (ov_buffer){0};
}

/*----------------------------------------------------------------------------*/

ov_format *ov_format_buffered(uint8_t *data, size_t length) {

    ov_format *fmt = ov_format_from_memory(data, length, OV_READ);

    if (0 == fmt) {

        ov_log_error("Could not create buffered format");
        goto error;
    }

    internal *intformat = is_mem(fmt);

    OV_ASSERT(0 != intformat);

    intformat->read_next_payload_chunk = impl_buffered_read_next_payload_chunk;

    return fmt;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_format_buffered_update(ov_format *f, uint8_t *new_data, size_t length) {

    if (0 == length) {
        ov_log_error("refuse to update buffered format with zero length "
                     "data");
        goto error;
    }

    if (0 == new_data) {

        ov_log_error("refuse to update buffered format with 0 pointer");
        goto error;
    }

    mem_data *mem = get_mem_data(f);

    if (0 == mem) {

        ov_log_error("Buffered format has no memory attached");
        goto error;
    }

    mem->start = new_data;
    mem->size = length;

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/
