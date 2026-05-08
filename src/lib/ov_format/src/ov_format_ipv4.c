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

#include "../include/ov_format_ipv4.h"

#include <arpa/inet.h>
#include <ov_arch/ov_byteorder.h>
#include <ov_base/ov_utils.h>

/*----------------------------------------------------------------------------*/

char const *OV_FORMAT_IPV4_TYPE_STRING = "ipv4";

static const uint32_t IPV4_MAGIC_BYTES = 0xA1BCDE;

/*----------------------------------------------------------------------------*/

static const size_t INT_HEADER_MIN_LENGTH = 20;

/*----------------------------------------------------------------------------*/

typedef struct {

    uint32_t magic_bytes;
    ov_format_ipv4_header header;

} ipv4_data;

/*----------------------------------------------------------------------------*/

static ipv4_data *as_ipv4_data(void *data) {

    if (0 == data)
        return 0;

    ipv4_data *ipv4_data = data;

    if (IPV4_MAGIC_BYTES != ipv4_data->magic_bytes)
        return 0;

    return ipv4_data;
}

/*----------------------------------------------------------------------------*/
static bool get_ipv4_header_unsafe(ov_format_ipv4_header *out, uint8_t **rd_ptr,
                                   size_t *length) {

    OV_ASSERT(0 != out);
    OV_ASSERT(0 != rd_ptr);
    OV_ASSERT(0 != *rd_ptr);
    OV_ASSERT(0 != length);
    OV_ASSERT(0 != length);

    ov_format_ipv4_header hdr = {0};

    if (*length < INT_HEADER_MIN_LENGTH) {

        ov_log_error("IPV4 header too small");
        goto error;
    }

    uint8_t *ptr = *rd_ptr;
    OV_ASSERT(0 != ptr);

    size_t read_octets = 0;

    uint8_t byte = *ptr;
    ptr += 1;

    /* Right shift portable because we deal with unsigned type here */
    if ((byte >> 4) != 4) {

        ov_log_error("Wrong version in IPv4 header. Expected 4, got %" PRIu8,
                     byte >> 4);
        goto error;
    }

    uint8_t ihl = byte & 0x0f;

    if (5 > ihl) {

        ov_log_error("IHL of IPv4 header too small, must be at least 5, but "
                     "found only to be %" PRIu8,
                     ihl);
        goto error;
    }

    /* next byte ignored */
    ptr += 1;

    hdr.header_length_octets = sizeof(uint32_t) * ihl;

    hdr.total_length_octets = ntohs(*(uint16_t *)ptr);
    ptr += 2;

    if (hdr.header_length_octets > hdr.total_length_octets) {

        ov_log_error("Total length of IP packet too small, must be at least "
                     "%zu "
                     "bytes, but found only to be %" PRIu8,
                     hdr.header_length_octets, hdr.total_length_octets);

        goto error;
    }

    /* Skip the next 4 bytes as we are not interested in them ... */

    ptr += 4;

    hdr.time_to_live = *ptr;

    ++ptr;

    hdr.protocol = *ptr;

    ++ptr;

    hdr.header_checksum = ntohs(*(uint16_t *)ptr);

    ptr += 2;

    memcpy(hdr.src_ip, ptr, 4);

    ptr += 4;

    memcpy(hdr.dst_ip, ptr, 4);

    ptr += 4;

    /* Just skip any options etc. */

    read_octets = hdr.header_length_octets;

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

static ov_buffer impl_next_chunk(ov_format *f, size_t requested_bytes,
                                 void *data) {

    UNUSED(requested_bytes);

    ov_buffer payload = {0};

    ipv4_data *rdata = as_ipv4_data(data);

    if (0 == rdata) {

        ov_log_error("Expected format ipv4, but got something different");
        goto error;
    }

    ov_buffer buf = ov_format_payload_read_chunk_nocopy(f, 0);

    if (0 == buf.start) {

        goto error;
    }

    if (!get_ipv4_header_unsafe(&rdata->header, &buf.start, &buf.length)) {

        goto error;
    }

    if (buf.length != rdata->header.total_length_octets -
                          rdata->header.header_length_octets) {

        ov_log_error("IPv4 paket corrupt - header length(%zu) + payload length "
                     "(%zu) do not match packet length (%" PRIu16,
                     rdata->header.header_length_octets, buf.length,
                     rdata->header.total_length_octets);

        goto error;
    }

    payload = buf;

error:

    return payload;
}

/*----------------------------------------------------------------------------*/

static ssize_t impl_write_chunk(ov_format *f, ov_buffer const *chunk,
                                void *data) {

    UNUSED(f);
    UNUSED(chunk);
    UNUSED(data);

    TODO("Implement");

    return false;
}

/*----------------------------------------------------------------------------*/

static void *impl_create_data(ov_format *f, void *options) {

    UNUSED(f);
    UNUSED(options);

    ipv4_data *rdata = calloc(1, sizeof(ipv4_data));
    OV_ASSERT(0 != rdata);

    rdata->magic_bytes = IPV4_MAGIC_BYTES;

    return rdata;
}

/*----------------------------------------------------------------------------*/

static void *impl_free_data(void *data) {

    if (0 == as_ipv4_data(data)) {

        ov_log_error("Internal error: Expected to be called with format "
                     "ipv4");
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

bool ov_format_ipv4_install(ov_format_registry *registry) {

    ov_format_handler handler = {
        .next_chunk = impl_next_chunk,
        .write_chunk = impl_write_chunk,
        .create_data = impl_create_data,
        .free_data = impl_free_data,
    };

    return ov_format_registry_register_type(OV_FORMAT_IPV4_TYPE_STRING, handler,
                                            registry);
}

/*----------------------------------------------------------------------------*/

bool ov_format_ipv4_get_header(ov_format const *f, ov_format_ipv4_header *hdr) {

    if (0 == hdr) {

        ov_log_error("No receiving header given");
        goto error;
    }

    ipv4_data *rdata = as_ipv4_data(ov_format_get_custom_data(f));

    if (0 == rdata) {

        ov_log_error("Expected IPV4 format");
        goto error;
    }

    memcpy(hdr, &rdata->header, sizeof(ov_format_ipv4_header));

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

char *ov_format_ipv4_ip_to_string(uint8_t *ip, char *out_buf,
                                  size_t out_buf_len) {

    static char ip_string[4 * 3 + 4] = {0};

    if (0 == ip)
        goto error;

    if (0 == out_buf) {

        out_buf = ip_string;
        out_buf_len = sizeof(ip_string);
    }

    snprintf(out_buf, out_buf_len, "%" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8,
             ip[0], ip[1], ip[2], ip[3]);

    return out_buf;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/
