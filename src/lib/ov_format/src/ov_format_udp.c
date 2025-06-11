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

#include "../include/ov_format_udp.h"

#include <arpa/inet.h>
#include <ov_arch/ov_byteorder.h>
#include <ov_base/ov_utils.h>

/*----------------------------------------------------------------------------*/

char const *OV_FORMAT_UDP_TYPE_STRING = "udp";

static const uint32_t UDP_MAGIC_BYTES = 0xB2ABCF;

/*----------------------------------------------------------------------------*/

static const size_t INT_HEADER_LENGTH = sizeof(uint16_t) * 4;

/*----------------------------------------------------------------------------*/

typedef struct {

    uint32_t magic_bytes;
    ov_format_udp_header header;

} udp_data;

/*----------------------------------------------------------------------------*/

static udp_data *as_udp_data(void *data) {

    if (0 == data) return 0;

    udp_data *udp_data = data;

    if (UDP_MAGIC_BYTES != udp_data->magic_bytes) return 0;

    return udp_data;
}

/*----------------------------------------------------------------------------*/
static bool get_udp_header_unsafe(ov_format_udp_header *out,
                                  uint8_t **rd_ptr,
                                  size_t *length) {

    OV_ASSERT(0 != out);
    OV_ASSERT(0 != rd_ptr);
    OV_ASSERT(0 != *rd_ptr);
    OV_ASSERT(0 != length);
    OV_ASSERT(0 != length);

    ov_format_udp_header hdr = {0};

    if (*length < INT_HEADER_LENGTH) {

        ov_log_error("UDP header too small");
        goto error;
    }

    uint8_t *ptr = *rd_ptr;
    OV_ASSERT(0 != ptr);

    size_t read_octets = 0;

    hdr.source_port = ntohs(*(uint16_t *)ptr);
    ptr += 2;

    hdr.destination_port = ntohs(*(uint16_t *)ptr);
    ptr += 2;

    hdr.length_octets = ntohs(*(uint16_t *)ptr);
    ptr += 2;

    hdr.checksum = ntohs(*(uint16_t *)ptr);
    ptr += 2;

    if (hdr.length_octets != *length) {

        ov_log_error("Length given in UDP header (%" PRIu16
                     " octets) "
                     "and received from lower layer (%zu octets) do not match",
                     hdr.length_octets,
                     *length);

        goto error;
    }

    read_octets = INT_HEADER_LENGTH;

    /*************************************************************************
                             Update out parameters
     ************************************************************************/

    *rd_ptr += read_octets;
    *length -= read_octets;

    memcpy(out, &hdr, sizeof(hdr));

    return true;

error:

    return false;
}

/*****************************************************************************
                                   Interface
 ****************************************************************************/

static ov_buffer impl_next_chunk(ov_format *f,
                                 size_t requested_bytes,
                                 void *data) {

    UNUSED(requested_bytes);

    ov_buffer payload = {0};

    udp_data *rdata = as_udp_data(data);

    if (0 == rdata) {

        ov_log_error("Expected format udp, but got something different");
        goto error;
    }

    ov_buffer buf = ov_format_payload_read_chunk_nocopy(f, 0);

    if (0 == buf.start) {

        goto error;
    }

    if (!get_udp_header_unsafe(&rdata->header, &buf.start, &buf.length)) {

        goto error;
    }

    if (rdata->header.length_octets != INT_HEADER_LENGTH + buf.length) {

        ov_log_error(
            "UDP paket corrupt - header length(%zu) + payload length "
            "(%zu) do not match packet length (%" PRIu16,
            INT_HEADER_LENGTH,
            buf.length,
            rdata->header.length_octets);

        goto error;
    }

    payload = buf;

error:

    return payload;
}

/*----------------------------------------------------------------------------*/

static ssize_t impl_write_chunk(ov_format *f,
                                ov_buffer const *chunk,
                                void *data) {

    UNUSED(f);
    UNUSED(chunk);
    UNUSED(data);

    TODO("Implement");

    return -1;
}

/*----------------------------------------------------------------------------*/

static void *impl_create_data(ov_format *f, void *options) {

    UNUSED(f);
    UNUSED(options);

    udp_data *rdata = calloc(1, sizeof(udp_data));
    OV_ASSERT(0 != rdata);

    rdata->magic_bytes = UDP_MAGIC_BYTES;

    return rdata;
}

/*----------------------------------------------------------------------------*/

static void *impl_free_data(void *data) {

    if (0 == as_udp_data(data)) {

        ov_log_error(
            "Internal error: Expected to be called with format "
            "udp");
        goto error;
    }

    free(data);
    data = 0;

error:

    return data;
}

/*****************************************************************************
                                     PUBLIC
 ****************************************************************************/

bool ov_format_udp_install(ov_format_registry *registry) {

    ov_format_handler handler = {
        .next_chunk = impl_next_chunk,
        .write_chunk = impl_write_chunk,
        .create_data = impl_create_data,
        .free_data = impl_free_data,
    };

    return ov_format_registry_register_type(
        OV_FORMAT_UDP_TYPE_STRING, handler, registry);
}

/*----------------------------------------------------------------------------*/

bool ov_format_udp_get_header(ov_format const *f, ov_format_udp_header *hdr) {

    if (0 == hdr) {

        ov_log_error("No receiving header given");
        goto error;
    }

    udp_data *rdata = as_udp_data(ov_format_get_custom_data(f));

    if (0 == rdata) {

        ov_log_error("Expected UDP format");
        goto error;
    }

    memcpy(hdr, &rdata->header, sizeof(ov_format_udp_header));

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/
