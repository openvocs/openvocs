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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "../include/ov_serde.h"
#include "ov_error_codes.h"
#include <errno.h>

/*----------------------------------------------------------------------------*/

static bool is_valid(ov_serde *self) {

    if (0 == self) {
        ov_log_error("No serde object (0 pointer)");
        return false;
    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

ov_serde_state ov_serde_add_raw(ov_serde *self, ov_buffer const *raw,
                                ov_result *res) {

    if (!is_valid(self)) {
        ov_result_set(res, OV_ERROR_INTERNAL_SERVER, "No serde object");
        return OV_SERDE_ERROR;
    } else {
        OV_ASSERT(0 != self->add_raw);
        return self->add_raw(self, raw, res);
    }
}

/*----------------------------------------------------------------------------*/

ov_serde_data OV_SERDE_DATA_NO_MORE = {0};

ov_serde_data ov_serde_pop_datum(ov_serde *self, ov_result *res) {

    if (!is_valid(self)) {
        ov_result_set(res, OV_ERROR_INTERNAL_SERVER, "No Serde instance");
        return (ov_serde_data){0};
    } else {
        OV_ASSERT(0 != self->pop_datum);
        return self->pop_datum(self, res);
    }
}

/*----------------------------------------------------------------------------*/

bool ov_serde_clear_buffer(ov_serde *self) {

    if (!is_valid(self)) {
        return false;
    } else if (0 == self->clear_buffer) {
        ov_log_warning("serde object does not provide 'clear_buffer'");
        return true;
    } else {
        return self->clear_buffer(self);
    }
}

/*----------------------------------------------------------------------------*/

static bool is_fh_valid(int fh) {

    if (0 > fh) {
        ov_log_error("File handle invalid");
        return false;
    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_serde_serialize(ov_serde *self, int fh, ov_serde_data data,
                        ov_result *res) {

    if ((!is_valid(self)) || (!is_fh_valid(fh))) {
        ov_result_set(res, OV_ERROR_INTERNAL_SERVER, "No serde object");
        return false;
    } else if (0 == self->serialize) {
        ov_log_error("serde object does not provide 'serialize'");
        return false;
    } else {
        return self->serialize(self, fh, data, res);
    }
}

/*----------------------------------------------------------------------------*/

ov_serde *ov_serde_free(ov_serde *self) {

    if (!is_valid(self)) {
        return false;
    } else {
        OV_ASSERT(0 != self->free);
        return self->free(self);
    }
}

/*****************************************************************************
                                   TO STRING
 ****************************************************************************/

static size_t fsize(FILE *s) {

    if (0 == s) {

        ov_log_error("Invalid stream: 0 pointer");
        return 0;

    } else if (0 == fseek(s, 0, SEEK_END)) {

        int len = ftell(s);
        rewind(s);

        if (0 > len) {

            ov_log_error("ftell failed: %s", strerror(errno));
            return 0;

        } else {

            ov_log_info("File is %il bytes long", len);
            return (size_t)len;
        }

    } else {

        ov_log_error("Seek failed");
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static char *alloc_for_string_of_length(size_t bytes) {

    if (0 == bytes) {
        return 0;
    } else {
        return malloc(bytes + 1);
    }
}

/*----------------------------------------------------------------------------*/

static bool file_to_buffer(FILE *s, char *buf, size_t buflen_bytes) {

    if ((0 == s) || (0 == buf) || (0 == buflen_bytes)) {
        ov_log_error("Invalid argument");
        return false;

    } else if (0 == fread(buf, buflen_bytes, 1, s)) {

        ov_log_error("Could not read from file");
        return false;

    } else {

        return true;
    }
}

/*----------------------------------------------------------------------------*/

static char *string_from_file(FILE *in) {

    size_t nbytes = fsize(in);
    char *buffer = alloc_for_string_of_length(nbytes);

    if (!file_to_buffer(in, buffer, nbytes)) {

        ov_free(buffer);
        return 0;

    } else {

        buffer[nbytes] = 0;
        return buffer;
    }
}

/*----------------------------------------------------------------------------*/

static int fh_from_stream(FILE *out) {

    if (0 == out) {

        return -1;

    } else {

        return fileno(out);
    }
}

/*----------------------------------------------------------------------------*/

static char *to_string_via_tmpfile(ov_serde *self, FILE *out,
                                   ov_serde_data data, ov_result *res) {
    int fh = fh_from_stream(out);

    if (0 > fh) {

        ov_result_set(res, OV_ERROR_INTERNAL_SERVER, "Invalid stream");
        return 0;

    } else if (!ov_serde_serialize(self, fh, data, res)) {

        return 0;

    } else {

        return string_from_file(out);
    }
}

/*----------------------------------------------------------------------------*/

char *ov_serde_to_string(ov_serde *self, ov_serde_data data, ov_result *res) {
    // A really crude hack - but everything else -
    // using memfd_create(2) (does not work),
    // serializing via FILE* instead of a file handle (renders the problem that
    // when serializing to a socket - which is the most important usecase of
    // this function - required first dumping the serialized string to a memory
    // area, then writing the memory area to the socket)
    // has serious flaws.
    FILE *tfile = tmpfile();

    if (0 != tfile) {

        char *result = to_string_via_tmpfile(self, tfile, data, res);
        fclose(tfile);
        return result;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/
