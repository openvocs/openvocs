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

#include "../include/ov_format_ethernet.h"
#include "../include/ov_format_ipv4.h"
#include "../include/ov_format_ipv6.h"

#include <arpa/inet.h>
#include <ov_arch/ov_byteorder.h>
#include <ov_base/ov_crc32.h>
#include <ov_base/ov_utils.h>

/*----------------------------------------------------------------------------*/

char const *OV_FORMAT_ETHERNET_TYPE_STRING = "ethernet";

static const uint32_t ETHERNET_MAGIC_BYTES = 0xA1BCDE;

/*----------------------------------------------------------------------------*/

static const size_t INT_HEADER_MIN_LENGTH = 14;

static const size_t CRC32_OCTETS = 4;

/*----------------------------------------------------------------------------*/

typedef struct {

    uint32_t magic_bytes;
    ov_format_ethernet_header header;
    uint32_t crc32_checksum;

    /* Required for crc32 check */
    ov_buffer raw_frame;

    struct {
        bool crc_present : 1;
    };

} ethernet_data;

/*----------------------------------------------------------------------------*/

static ethernet_data *as_ethernet_data(void *data) {

    if (0 == data) return 0;

    ethernet_data *ethernet_data = data;

    if (ETHERNET_MAGIC_BYTES != ethernet_data->magic_bytes) return 0;

    return ethernet_data;
}

/*----------------------------------------------------------------------------*/
static bool get_ethernet_header_unsafe(ov_format_ethernet_header *out,
                                       uint8_t **rd_ptr,
                                       size_t *length) {

    OV_ASSERT(0 != out);
    OV_ASSERT(0 != rd_ptr);
    OV_ASSERT(0 != *rd_ptr);
    OV_ASSERT(0 != length);
    OV_ASSERT(0 != length);

    ov_format_ethernet_header hdr = {0};

    if (*length < INT_HEADER_MIN_LENGTH) {

        ov_log_error("ETHERNET header too small");
        goto error;
    }

    uint8_t *ptr = *rd_ptr;
    OV_ASSERT(0 != ptr);

    size_t read_octets = 0;

    memcpy(hdr.dst_mac, ptr, OV_FORMAT_ETHERNET_MAC_LEN_OCTETS);

    ptr += OV_FORMAT_ETHERNET_MAC_LEN_OCTETS;
    read_octets += OV_FORMAT_ETHERNET_MAC_LEN_OCTETS;

    memcpy(hdr.src_mac, ptr, OV_FORMAT_ETHERNET_MAC_LEN_OCTETS);

    ptr += OV_FORMAT_ETHERNET_MAC_LEN_OCTETS;
    read_octets += OV_FORMAT_ETHERNET_MAC_LEN_OCTETS;

    hdr.type_set = true;
    hdr.type = ntohs(*((uint16_t *)ptr));

    ptr += 2;
    read_octets += 2;

    if (1536 > hdr.type) {

        hdr.type_set = false;
        hdr.length = hdr.type;
    }

    /*************************************************************************
                             Update out parameters
     ************************************************************************/

    *rd_ptr = ptr;
    *length -= read_octets;

    memcpy(out, &hdr, sizeof(hdr));

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static bool get_crc32_checksum_unsafe(uint32_t *crc32_out,
                                      uint8_t **rd_ptr,
                                      size_t *length) {

    OV_ASSERT(0 != crc32_out);
    OV_ASSERT(0 != rd_ptr);
    OV_ASSERT(0 != length);

    uint8_t *ptr = *rd_ptr;

    OV_ASSERT(0 != ptr);

    if (*length < CRC32_OCTETS) {

        ov_log_error("Not enought data for CRC32, require at least 4 octets");
        goto error;
    }

    uint32_t crc32 = 0;

    /* We need to copy the CRC, because within the frame memory block,
     * it might not be aligned at a 32bit boundary ... */
    memcpy(&crc32, ptr + *length - CRC32_OCTETS, CRC32_OCTETS);

    *crc32_out = ntohl(crc32);

    *length -= CRC32_OCTETS;

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

    ethernet_data *rdata = as_ethernet_data(data);

    if (0 == rdata) {

        ov_log_error("Expected format ethernet, but got something different");
        goto error;
    }

    rdata->raw_frame.start = 0;
    rdata->raw_frame.length = 0;

    ov_buffer buf = ov_format_payload_read_chunk_nocopy(f, 0);

    if (0 == buf.start) {

        goto error;
    }

    rdata->raw_frame = buf;

    if (!get_ethernet_header_unsafe(&rdata->header, &buf.start, &buf.length)) {

        goto error;
    }

    rdata->crc32_checksum = 0;

    if (rdata->crc_present &&
        !get_crc32_checksum_unsafe(
            &rdata->crc32_checksum, &buf.start, &buf.length)) {

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

    ethernet_data *rdata = calloc(1, sizeof(ethernet_data));
    OV_ASSERT(0 != rdata);

    rdata->magic_bytes = ETHERNET_MAGIC_BYTES;
    rdata->crc_present = true;

    if (0 != options) {

        ov_format_option_ethernet *foe = options;
        rdata->crc_present = foe->crc_present;
    }

    return rdata;
}

/*----------------------------------------------------------------------------*/

static void *impl_free_data(void *data) {

    if (0 == as_ethernet_data(data)) {

        ov_log_error(
            "Internal error: Expected to be called with format "
            "ethernet");
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

bool ov_format_ethernet_install(ov_format_registry *registry) {

    ov_format_handler handler = {
        .next_chunk = impl_next_chunk,
        .write_chunk = impl_write_chunk,
        .create_data = impl_create_data,
        .free_data = impl_free_data,
    };

    return ov_format_registry_register_type(
        OV_FORMAT_ETHERNET_TYPE_STRING, handler, registry);
}

/*----------------------------------------------------------------------------*/

bool ov_format_ethernet_get_header(ov_format const *f,
                                   ov_format_ethernet_header *hdr) {

    if (0 == hdr) {

        ov_log_error("No receiving header given");
        goto error;
    }

    ethernet_data *rdata = as_ethernet_data(ov_format_get_custom_data(f));

    if (0 == rdata) {

        ov_log_error("Expected ETHERNET format");
        goto error;
    }

    memcpy(hdr, &rdata->header, sizeof(ov_format_ethernet_header));

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

uint32_t ov_format_ethernet_get_crc32(ov_format const *f) {

    ethernet_data *edata = as_ethernet_data(ov_format_get_custom_data(f));

    if (0 == edata) {

        ov_log_error("Expected to be called with ETHERNET format");
        goto error;
    }

    return edata->crc32_checksum;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

uint32_t ov_format_ethernet_calculate_crc32(ov_format const *fmt) {

    ethernet_data *edata = as_ethernet_data(ov_format_get_custom_data(fmt));

    if (0 == edata) {

        ov_log_error(
            "Internal error: Expected to be called with ethernet "
            "format");
        goto error;
    }

    if (0 == edata->raw_frame.start) {

        ov_log_error("No valid frame data read yet");
        goto error;
    }

    if (sizeof(uint32_t) > edata->raw_frame.length) {

        ov_log_error("Raw frame too short to calculate CRC32?");
        goto error;
    }

    uint32_t crc32 = ov_crc32_zlib(
        0, edata->raw_frame.start, edata->raw_frame.length - sizeof(uint32_t));

    return crc32;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

char *ov_format_ethernet_mac_to_string(uint8_t *mac,
                                       char *out_string,
                                       size_t out_len) {

    static char static_str[3 * OV_FORMAT_ETHERNET_MAC_LEN_OCTETS] = {0};

    if (0 == mac) {

        goto error;
    }

    if (0 == out_string) {

        out_string = static_str;
        out_len = sizeof(static_str);
    }

    size_t octets_written = snprintf(out_string,
                                     out_len,
                                     "%x:%x:%x:%x:%x:%x",
                                     mac[0] & 0xff,
                                     mac[1] & 0xff,
                                     mac[2] & 0xff,
                                     mac[3] & 0xff,
                                     mac[4] & 0xff,
                                     mac[5] & 0xff);

    out_string[out_len - 1] = 0;

    OV_ASSERT(2 * OV_FORMAT_ETHERNET_MAC_LEN_OCTETS <= octets_written + 1);
    OV_ASSERT(3 * OV_FORMAT_ETHERNET_MAC_LEN_OCTETS > octets_written);

    return out_string;

error:

    return 0;
}

/*****************************************************************************
                              Ethernet dispatcher
 ****************************************************************************/

char const *OV_FORMAT_ETHERNET_DISPATCHER_TYPE_STRING = "ethernet_ip";

static const uint32_t DISPATCHER_MAGIC_BYTES = 0xA112DE;

typedef enum {

    INVALID = -0x0001,
    IPV4 = 0x800,
    IPV6 = 0x86dd

} ethertype;

typedef struct {

    uint32_t magic_bytes;

    ov_format *ipv4;
    ov_format *ipv6;
    ov_format *buffered;
    ov_format *ethernet;

    /* Format that read the last packet */
    ethertype current_ethertype;

} dispatcher_data;

/*----------------------------------------------------------------------------*/

static dispatcher_data *as_dispatcher_data(void *data) {

    if (0 == data) return 0;

    dispatcher_data *dispatcher_data = data;

    if (DISPATCHER_MAGIC_BYTES != dispatcher_data->magic_bytes) return 0;

    return dispatcher_data;
}

/*----------------------------------------------------------------------------*/

static void *impl_dispatcher_free_data(void *data) {

    dispatcher_data *ddata = as_dispatcher_data(data);

    if (0 == ddata) {

        goto error;
    }

    data = 0;

    OV_ASSERT(0 != ddata);

    if (0 != ddata->ethernet) {

        ddata->ethernet = ov_format_close_non_recursive(ddata->ethernet);
    }

    if (0 != ddata->buffered) {

        ddata->buffered = ov_format_close_non_recursive(ddata->buffered);
    }

    if (0 != ddata->ipv4) {

        ddata->ipv4 = ov_format_close_non_recursive(ddata->ipv4);
    }

    if (0 != ddata->ipv6) {

        ddata->ipv6 = ov_format_close_non_recursive(ddata->ipv6);
    }

    OV_ASSERT(0 == ddata->ethernet);
    OV_ASSERT(0 == ddata->buffered);
    OV_ASSERT(0 == ddata->ipv4);
    OV_ASSERT(0 == ddata->ipv6);

    free(ddata);

    ddata = 0;

error:

    return data;
}

/*----------------------------------------------------------------------------*/

const uint8_t DUMMY_DATA = 0;
const size_t DUMMY_DATA_LENGTH = 1;

static void *impl_dispatcher_create_data(ov_format *f, void *etheroptions) {

    dispatcher_data *data = 0;

    if (0 == f) {

        goto error;
    }

    data = calloc(1, sizeof(dispatcher_data));

    data->magic_bytes = DISPATCHER_MAGIC_BYTES;

    /* Initialize with dummy data - will never be read */
    data->buffered =
        ov_format_buffered((uint8_t *)&DUMMY_DATA, DUMMY_DATA_LENGTH);

    if (0 == data->buffered) {

        ov_log_error("Could not create buffered format");
        goto error;
    }

    data->ethernet = ov_format_as(f, "ethernet", etheroptions, 0);
    data->ipv4 = ov_format_as(data->buffered, "ipv4", 0, 0);
    data->ipv6 = ov_format_as(data->buffered, "ipv6", 0, 0);

    if ((0 == data->ethernet) || (0 == data->ipv4) || (0 == data->ipv6)) {

        goto error;
    }

    data->current_ethertype = INVALID;

    return data;

error:

    data = impl_dispatcher_free_data(data);

    OV_ASSERT(0 == data);

    return 0;
}

/*----------------------------------------------------------------------------*/

static ov_buffer impl_dispatcher_next_chunk(ov_format *f,
                                            size_t requested_bytes,
                                            void *data) {

    ov_buffer buffer = {0};

    if (0 == f) {

        ov_log_error("Called with 0 pointer");
        goto error;
    }

    dispatcher_data *ddata = as_dispatcher_data(data);

    if (0 == ddata) {

        ov_log_error(
            "Expected to be called with ethernet ip dispatcher "
            "format");
        goto error;
    }

    OV_ASSERT(0 != ddata->buffered);
    OV_ASSERT(0 != ddata->ethernet);
    OV_ASSERT(0 != ddata->ipv4);
    OV_ASSERT(0 != ddata->ipv6);

    ov_buffer new_data =
        ov_format_payload_read_chunk_nocopy(ddata->ethernet, 0);

    if (0 == new_data.start) {

        ov_log_error("No data received from ethernet");
        goto error;
    }

    if (!ov_format_buffered_update(
            ddata->buffered, new_data.start, new_data.length)) {

        goto error;
    }

    ov_format_ethernet_header hdr;

    if (!ov_format_ethernet_get_header(ddata->ethernet, &hdr)) {
        goto error;
    }

    switch (hdr.type) {

        case IPV4:

            ddata->current_ethertype = IPV4;

            buffer = ov_format_payload_read_chunk_nocopy(
                ddata->ipv4, requested_bytes);

            break;

        case IPV6:

            ddata->current_ethertype = IPV6;

            buffer = ov_format_payload_read_chunk_nocopy(
                ddata->ipv6, requested_bytes);

            break;

        default:

            ddata->current_ethertype = INVALID;

            ov_log_error("Unsupported ethertype %" PRIu16, hdr.type);
            goto error;
    };

error:

    return buffer;
}

/*---------------------------------------------------------------------------*/

static ov_format *impl_dispatcher_responsible_for(ov_format const *f,
                                                  char const *type) {

    if (0 == type) goto error;

    dispatcher_data *data = as_dispatcher_data(ov_format_get_custom_data(f));

    if (0 == data) goto error;

    /* We are responsible for both ipv4 and ipv6 */

    switch (data->current_ethertype) {

        case IPV4:

            if (0 == strcmp(type, "ipv4")) return data->ipv4;
            break;

        case IPV6:

            if (0 == strcmp(type, "ipv6")) return data->ipv6;
            break;

        default:

            return false;
    }

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_format_ethernet_dispatcher_install(ov_format_registry *registry) {

    ov_format_handler handler = {

        .next_chunk = impl_dispatcher_next_chunk,
        .write_chunk = impl_write_chunk,
        .create_data = impl_dispatcher_create_data,
        .free_data = impl_dispatcher_free_data,
        .responsible_for = impl_dispatcher_responsible_for,

    };

    return ov_format_registry_register_type(
        OV_FORMAT_ETHERNET_DISPATCHER_TYPE_STRING, handler, registry);
}
