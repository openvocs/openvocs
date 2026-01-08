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

#include "../include/ov_format_pcap.h"
#include "../include/ov_format_ethernet.h"

#include <ov_base/ov_utils.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include <errno.h>
#include <ov_base/ov_file.h>
#include <ov_log/ov_log.h>

/*****************************************************************************
                                   PCAP DEFs
 ****************************************************************************/

char const *OV_FORMAT_PCAP_TYPE_STRING = "pcap";

/*----------------------------------------------------------------------------*/

const uint32_t pcap_magic_bytes = 0xa1b2c3d4;

const size_t pcap_global_header_length = 4 + 2 + 2 + 4 + 4 + 4 + 4;

/*----------------------------------------------------------------------------*/

typedef struct {

    uint32_t magic_bytes;
    ov_format_pcap_global_header global_header;
    ov_format_pcap_packet_header current_packet_header;

} pcap_data;

const size_t pcap_packet_header_length = 4 + 4 + 4 + 4;

/*****************************************************************************
                                   FUNCTIONS
 ****************************************************************************/

static pcap_data *as_pcap_data(void *data) {

    if (0 == data)
        return 0;

    pcap_data *pcap_data = data;

    if (pcap_magic_bytes != pcap_data->magic_bytes)
        return 0;

    return pcap_data;
}

/*----------------------------------------------------------------------------*/

enum bytes_swapped {

    PCAP_INVALID,
    PCAP_SWAPPED,
    PCAP_PROPER,

};

static enum bytes_swapped detect_bytes_swapped(uint8_t **rd_ptr,
                                               size_t *length) {

    OV_ASSERT(0 != rd_ptr);
    OV_ASSERT(0 != *rd_ptr);
    OV_ASSERT(0 != length);

    if (*length < 4) {

        goto error;
    }

    uint32_t *magic_bytes = (uint32_t *)*rd_ptr;

    *length -= 4;
    *rd_ptr += 4;

    switch (*magic_bytes) {

    case 0xa1b2c3d4:
    case 0xa1b23c4d:

        return PCAP_PROPER;

    case 0xd4c3b2a1:
    case 0x4d3cb2a1:

        return PCAP_SWAPPED;

    default:

        /* Entirely wrong magic_bytes -> Not a pcap file */
        return PCAP_INVALID;
    };

error:

    return PCAP_INVALID;
}

/*----------------------------------------------------------------------------*/

static bool get_global_header_unsafe(ov_format_pcap_global_header *out,
                                     uint8_t **rd_ptr, size_t *length) {

    OV_ASSERT(0 != out);
    OV_ASSERT(0 != rd_ptr);
    OV_ASSERT(0 != *rd_ptr);
    OV_ASSERT(0 != length);

    if (pcap_global_header_length > *length) {

        ov_log_error("File too small for a proper PCAP file");
        goto error;
    }

    ov_format_pcap_global_header hdr = {
        .bytes_swapped = false,
    };

    enum bytes_swapped bytes_swapped = detect_bytes_swapped(rd_ptr, length);

    if (PCAP_INVALID == bytes_swapped) {

        ov_log_error("No proper magic bytes");
        goto error;
    }

    ov_file_byteorder byteorder = OV_FILE_RAW;

    if (PCAP_SWAPPED == bytes_swapped) {

        hdr.bytes_swapped = true;
        byteorder = OV_FILE_SWAP_BYTES;
    }

    bool ok = ov_file_get_16(&hdr.version_major, rd_ptr, length, byteorder);

    ok = ok && ov_file_get_16(&hdr.version_minor, rd_ptr, length, byteorder);

    ok = ok &&
         ov_file_get_32((uint32_t *)&hdr.thiszone, rd_ptr, length, byteorder);

    ok = ok && ov_file_get_32(&hdr.sigfigs, rd_ptr, length, byteorder);

    ok = ok && ov_file_get_32(&hdr.snaplen, rd_ptr, length, byteorder);

    ok = ok && ov_file_get_32(&hdr.data_link_type, rd_ptr, length, byteorder);

    if (!ok) {

        ov_log_error("Error while reading PCAP global header");
        goto error;
    }

    memcpy(out, &hdr, sizeof(hdr));

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static bool get_packet_header_unsafe(ov_format_pcap_packet_header *out,
                                     uint8_t **rd_ptr, size_t *length,
                                     const bool bytes_swapped) {

    OV_ASSERT(0 != out);
    OV_ASSERT(0 != rd_ptr);
    OV_ASSERT(0 != *rd_ptr);
    OV_ASSERT(0 != length);

    ov_file_byteorder byteorder = OV_FILE_RAW;

    if (bytes_swapped) {

        byteorder = OV_FILE_SWAP_BYTES;
    }

    if (pcap_packet_header_length > *length) {

        ov_log_error("Too few bytes for proper PCAP packet header");
        goto error;
    }

    ov_format_pcap_packet_header hdr = {0};

    bool ok = ov_file_get_32(&hdr.timestamp_secs, rd_ptr, length, byteorder);

    ok = ok && ov_file_get_32(&hdr.timestamp_usecs, rd_ptr, length, byteorder);
    ok = ok &&
         ov_file_get_32(&hdr.length_stored_bytes, rd_ptr, length, byteorder);
    ok = ok &&
         ov_file_get_32(&hdr.length_origin_bytes, rd_ptr, length, byteorder);

    if (!ok) {

        ov_log_error("Could not decode PCAP packet header");
        goto error;
    }

    memcpy(out, &hdr, sizeof(ov_format_pcap_packet_header));

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static ov_buffer impl_next_chunk(ov_format *f, size_t req_bytes, void *data) {

    UNUSED(req_bytes);

    ov_buffer payload = {0};

    pcap_data *pcap_data = as_pcap_data(data);

    if (0 == pcap_data) {

        ov_log_error("Internal error: function was called expecting PCAP "
                     "format");
        goto error;
    }

    ov_buffer buf = {0};

    buf = ov_format_payload_read_chunk_nocopy(f, pcap_packet_header_length);

    if (0 == buf.start) {

        ov_log_error("Could not read next packet header");
        goto error;
    }

    ov_format_pcap_packet_header hdr = {0};

    if (!get_packet_header_unsafe(&hdr, &buf.start, &buf.length,
                                  pcap_data->global_header.bytes_swapped)) {

        goto error;
    }

    buf = (ov_buffer){0};

    buf = ov_format_payload_read_chunk_nocopy(f, hdr.length_stored_bytes);

    if (0 == buf.start) {

        ov_log_error("Could not read packet payload, expected %" PRIu32 " bytes"
                     " of "
                     "payloa"
                     "d");
        goto error;
    }

    pcap_data->current_packet_header = hdr;

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

    TODO("NOT IMPLEMENTED");

    return -1;
}

/*----------------------------------------------------------------------------*/

static void *impl_data_create(ov_format *f, void *options) {

    UNUSED(options);

    pcap_data *data = 0;

    ov_buffer header =
        ov_format_payload_read_chunk_nocopy(f, pcap_global_header_length);

    if (0 == header.start) {

        ov_log_error("Could not read PCAP header");
        goto error;
    }

    ov_format_pcap_global_header global_header = {0};

    if (!get_global_header_unsafe(&global_header, &header.start,
                                  &header.length)) {

        ov_log_error("Could not decode global PCAP header");
        goto error;
    }

    data = calloc(1, sizeof(pcap_data));
    OV_ASSERT(0 != data);

    data->magic_bytes = pcap_magic_bytes;
    data->global_header = global_header;

    return data;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

static void *impl_data_free(void *data) {

    pcap_data *pcap = as_pcap_data(data);

    if (0 == pcap)
        return data;

    free(pcap);
    pcap = 0;

    return pcap;
}

/*****************************************************************************
                                     PUBLIC
 ****************************************************************************/

bool ov_format_pcap_install(ov_format_registry *registry) {

    ov_format_handler handler = {

        .next_chunk = impl_next_chunk,
        .write_chunk = impl_write_chunk,
        .create_data = impl_data_create,
        .free_data = impl_data_free,

    };

    return ov_format_registry_register_type(OV_FORMAT_PCAP_TYPE_STRING, handler,
                                            registry);
}

/*----------------------------------------------------------------------------*/

bool ov_format_pcap_get_global_header(ov_format const *f,
                                      ov_format_pcap_global_header *hdr) {

    if (0 == hdr)
        goto error;

    pcap_data *pcap = as_pcap_data(ov_format_get_custom_data(f));

    if (0 == pcap) {

        ov_log_error("Could not get pcap custom data - not a pcap format?");
        goto error;
    }

    memcpy(hdr, &pcap->global_header, sizeof(*hdr));

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_format_pcap_get_current_packet_header(
    ov_format const *f, ov_format_pcap_packet_header *hdr) {

    pcap_data *data = as_pcap_data(ov_format_get_custom_data(f));

    if (0 == data) {

        ov_log_error("Internal error: Expected to be called on a  PCAP file");
        goto error;
    }

    if (0 == hdr) {

        ov_log_error("Null pointer");
        goto error;
    }

    memcpy(hdr, &data->current_packet_header,
           sizeof(ov_format_pcap_packet_header));

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

int ov_format_pcap_print_global_header(FILE *out,
                                       ov_format_pcap_global_header *hdr) {

    if (0 == out)
        return -EINVAL;

    if (0 == hdr)
        return -EINVAL;

    return fprintf(out,
                   "bytes swapped: %s\n"
                   "version: %" PRIu16 ".%" PRIu16 "\n"
                   "thiszone %" PRIi32 "\n"
                   "sigfigs %" PRIu32 "\n"
                   "snaplen %" PRIu32 "\n"
                   "network %" PRIu32 "\n",
                   hdr->bytes_swapped ? "yes" : "no", hdr->version_major,
                   hdr->version_minor, hdr->thiszone, hdr->sigfigs,
                   hdr->snaplen, hdr->data_link_type);
}

/*----------------------------------------------------------------------------*/

ov_format *ov_format_pcap_create_network_layer_format(ov_format *pcap_fmt) {

    ov_format *network_layer = 0;
    ov_format *ipv4 = 0;

    ov_format_pcap_global_header hdr = {0};

    if (!ov_format_pcap_get_global_header(pcap_fmt, &hdr)) {

        ov_log_error("Could not get global PCAP header");
        goto error;
    }

    /* PCAPs don't save the CRC32 footer */
    ov_format_option_ethernet fo_ethernet = {
        .crc_present = false,
    };

    switch (hdr.data_link_type) {

    case OV_FORMAT_PCAP_LINKTYPE_ETHERNET:

        network_layer = ov_format_as(pcap_fmt, "ethernet_ip", &fo_ethernet, 0);

        if (0 == network_layer)
            goto error;

        break;

    case OV_FORMAT_PCAP_LINKTYPE_LINUX_SLL:
        network_layer = ov_format_as(pcap_fmt, "linux_sll", &fo_ethernet, 0);

        if (0 == network_layer)
            goto error;

        ipv4 = ov_format_as(network_layer, "ipv4", 0, 0);

        if (0 == ipv4) {

            network_layer = ov_format_close_non_recursive(network_layer);
            goto error;
        }

        network_layer = ipv4;
        ipv4 = 0;

        break;

    default:

        ov_log_error("Unsupported link layer format: %" PRIu32,
                     hdr.data_link_type);
    }

error:

    return network_layer;
}
