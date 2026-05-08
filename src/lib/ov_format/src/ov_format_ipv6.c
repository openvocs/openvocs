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

#include "../include/ov_format_ipv6.h"

#include <arpa/inet.h>
#include <ov_arch/ov_byteorder.h>
#include <ov_base/ov_utils.h>

/*----------------------------------------------------------------------------*/

char const *OV_FORMAT_IPV6_TYPE_STRING = "ipv6";

static const uint32_t IPV6_MAGIC_BYTES = 0xB2BCDE;

/*----------------------------------------------------------------------------*/

static const size_t INT_HEADER_MIN_LENGTH = 40;

/*----------------------------------------------------------------------------*/

typedef struct {

    uint32_t magic_bytes;
    ov_format_ipv6_header header;

} ipv6_data;

/*----------------------------------------------------------------------------*/

static ipv6_data *as_ipv6_data(void *data) {

    if (0 == data)
        return 0;

    ipv6_data *ipv6_data = data;

    if (IPV6_MAGIC_BYTES != ipv6_data->magic_bytes)
        return 0;

    return ipv6_data;
}

/*----------------------------------------------------------------------------*/

static size_t get_next_header_length(uint8_t type) {

    switch (type) {

    case 6:
        return 0; // TCP

    case 17:
        return 0; // UDP

    case 58:
        return 0; // ICMPv6
    };

    /* We do not support any other types right now ... */
    return 1;
}

/*----------------------------------------------------------------------------*/

static bool skip_extension_headers(uint8_t **rd_ptr, size_t *length,
                                   uint8_t next_header_type,
                                   uint8_t *payload_type_out) {

    UNUSED(rd_ptr);
    UNUSED(length);

    size_t next_header_length = get_next_header_length(next_header_type);

    /* We just discard any packet carrying one or more  extension header -
     * too complex to handle right now */
    if (0 != next_header_length)
        goto error;

    *payload_type_out = next_header_type;

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static bool get_ipv6_header_unsafe(ov_format_ipv6_header *out, uint8_t **rd_ptr,
                                   size_t *length) {

    OV_ASSERT(0 != out);
    OV_ASSERT(0 != rd_ptr);
    OV_ASSERT(0 != *rd_ptr);
    OV_ASSERT(0 != length);
    OV_ASSERT(0 != length);

    ov_format_ipv6_header hdr = {0};

    if (*length < INT_HEADER_MIN_LENGTH) {

        ov_log_error("IPV6 header too small");
        goto error;
    }

    uint8_t *ptr = *rd_ptr;
    OV_ASSERT(0 != ptr);

    size_t read_octets = 0;

    uint8_t byte = *ptr;
    ptr += 1;

    /* Right shift portable because we deal with unsigned type here */
    if ((byte >> 4) != 6) {

        ov_log_error("Wrong version in IPv6 header. Expected 6, got %" PRIu8,
                     byte >> 4);
        goto error;
    }

    hdr.traffic_class = byte & 0x0f;
    hdr.traffic_class <<= 4;

    byte = *ptr;
    ptr += 1;

    hdr.traffic_class += (byte & 0xf0) >> 4;

    hdr.flow_label = (byte & 0x0f);
    hdr.flow_label <<= 16;

    hdr.flow_label += ntohs(*(uint16_t *)ptr);
    ptr += 2;

    hdr.payload_length = ntohs(*(uint16_t *)ptr);
    ptr += 2;

    uint8_t next_header = *ptr;

    ptr += 1;

    hdr.hop_limit = *ptr;
    ptr += 1;

    memcpy(hdr.src_ip, ptr, 16);

    ptr += 16;

    memcpy(hdr.dst_ip, ptr, 16);

    ptr += 16;

    OV_ASSERT(*rd_ptr + 40 == ptr);

    /* Just skip any options etc. */

    read_octets = 40;

    /*************************************************************************
                             Update out parameters
     ************************************************************************/

    *length -= read_octets;
    *rd_ptr = ptr;

    if (!skip_extension_headers(rd_ptr, length, next_header,
                                &hdr.next_header)) {

        goto error;
    }

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

    ipv6_data *rdata = as_ipv6_data(data);

    if (0 == rdata) {

        ov_log_error("Expected format ipv6, but got something different");
        goto error;
    }

    ov_buffer buf = ov_format_payload_read_chunk_nocopy(f, 0);

    if (0 == buf.start) {

        goto error;
    }

    if (!get_ipv6_header_unsafe(&rdata->header, &buf.start, &buf.length)) {

        goto error;
    }

    /* With the absence of extension headers, the buf must be the payload length
     * of the ipv6 header */
    if (buf.length != rdata->header.payload_length) {

        ov_log_error("IPv6 paket corrupt - header payload length "
                     "(%zu) do not match actual paylaod length (%" PRIu16,
                     rdata->header.payload_length, buf.length);

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

    ipv6_data *rdata = calloc(1, sizeof(ipv6_data));
    OV_ASSERT(0 != rdata);

    rdata->magic_bytes = IPV6_MAGIC_BYTES;

    return rdata;
}

/*----------------------------------------------------------------------------*/

static void *impl_free_data(void *data) {

    if (0 == as_ipv6_data(data)) {

        ov_log_error("Internal error: Expected to be called with format "
                     "ipv6");
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

bool ov_format_ipv6_install(ov_format_registry *registry) {

    ov_format_handler handler = {
        .next_chunk = impl_next_chunk,
        .write_chunk = impl_write_chunk,
        .create_data = impl_create_data,
        .free_data = impl_free_data,
    };

    return ov_format_registry_register_type(OV_FORMAT_IPV6_TYPE_STRING, handler,
                                            registry);
}

/*----------------------------------------------------------------------------*/

bool ov_format_ipv6_get_header(ov_format const *f, ov_format_ipv6_header *hdr) {

    if (0 == hdr) {

        ov_log_error("No receiving header given");
        goto error;
    }

    ipv6_data *rdata = as_ipv6_data(ov_format_get_custom_data(f));

    if (0 == rdata) {

        ov_log_error("Expected IPV6 format");
        goto error;
    }

    memcpy(hdr, &rdata->header, sizeof(ov_format_ipv6_header));

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

char *ov_format_ipv6_ip_to_string(uint8_t *ip, char *out_buf,
                                  size_t out_buf_len) {

    static char ip_string[2 * 16 + 7 + 1] = {0};

    if (0 == ip)
        goto error;

    if (0 == out_buf) {

        out_buf = ip_string;
        out_buf_len = sizeof(ip_string);
    }

    if (sizeof(ip_string) > out_buf_len) {

        ov_log_warning("out buffer too small to keep entire ipv6 address - "
                       "could be cut off");
    }

    snprintf(out_buf, out_buf_len,
             "%" PRIu8 "%" PRIu8 ":%" PRIu8 "%" PRIu8 ":%" PRIu8 "%" PRIu8
             ":%" PRIu8 "%" PRIu8 ":%" PRIu8 "%" PRIu8 ":%" PRIu8 "%" PRIu8
             ":%" PRIu8 "%" PRIu8 ":%" PRIu8 "%" PRIu8,
             ip[0], ip[1], ip[2], ip[3], ip[4], ip[5], ip[6], ip[7], ip[8],
             ip[9], ip[10], ip[11], ip[12], ip[13], ip[14], ip[15]);

    return out_buf;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/
