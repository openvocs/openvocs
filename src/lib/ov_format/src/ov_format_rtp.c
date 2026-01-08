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

#include "../include/ov_format_rtp.h"

#include <arpa/inet.h>
#include <ov_arch/ov_byteorder.h>
#include <ov_base/ov_utils.h>

/*----------------------------------------------------------------------------*/

char const *OV_FORMAT_RTP_TYPE_STRING = "rtp";

const ov_buffer INT_NO_PADDING = {0};

ov_buffer const *OV_FORMAT_RTP_NO_PADDING = &INT_NO_PADDING;

static const uint32_t RTP_MAGIC_BYTES = 0xA1BCDE;

/*----------------------------------------------------------------------------*/

static const size_t INT_HEADER_MIN_LENGTH = 12;

/*----------------------------------------------------------------------------*/

typedef struct {

    uint32_t magic_bytes;
    ov_format_rtp_header header;
    ov_format_rtp_contributing_sources csrcs;
    ov_format_rtp_extension_header ext_header;

    ov_buffer padding;

    struct {

        bool ext_header_set : 1;
        bool padding_set : 1;
    };

} rtp_data;

/*----------------------------------------------------------------------------*/

static rtp_data *as_rtp_data(void *data) {

    if (0 == data)
        return 0;

    rtp_data *rtp_data = data;

    if (RTP_MAGIC_BYTES != rtp_data->magic_bytes)
        return 0;

    return rtp_data;
}

/*----------------------------------------------------------------------------*/

typedef struct {

    unsigned num_csrcs : 4;
    bool ext_header_set : 1;
    bool padding_set : 1;

} implicit_header;

static bool get_rtp_header_unsafe(ov_format_rtp_header *out,
                                  implicit_header *iheader, uint8_t **rd_ptr,
                                  size_t *length) {

    OV_ASSERT(0 != out);
    OV_ASSERT(0 != iheader);
    OV_ASSERT(0 != rd_ptr);
    OV_ASSERT(0 != *rd_ptr);
    OV_ASSERT(0 != length);
    OV_ASSERT(0 != length);

    ov_format_rtp_header hdr = {0};

    if (*length < INT_HEADER_MIN_LENGTH) {

        ov_log_error("RTP header too small");
        goto error;
    }

    uint8_t *ptr = *rd_ptr;
    OV_ASSERT(0 != ptr);

    uint8_t byte = *ptr;
    size_t read_octets = 0;

    ++ptr;
    ++read_octets;

    hdr.version = (byte >> 6) & 0x03;

    iheader->padding_set = (0 != (byte & 0x20));
    iheader->ext_header_set = (0 != (byte & 0x10));

    iheader->num_csrcs = byte & 0x0f;

    byte = *ptr;
    ++ptr;
    ++read_octets;

    hdr.marker = (0 != (byte & 0x80));
    hdr.payload_type = byte & 0x7f;

    hdr.sequence_number = ntohs(*(uint16_t *)ptr);

    ptr += 2;
    read_octets += 2;

    hdr.timestamp = ntohl(*(uint32_t *)ptr);
    ptr += 4;
    read_octets += 4;

    hdr.ssrc = ntohl(*(uint32_t *)ptr);
    ptr += 4;
    read_octets += 4;

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

static bool
get_contributing_sources_unsafe(ov_format_rtp_contributing_sources *csrcs_out,
                                size_t num_csrcs, uint8_t **rd_ptr,
                                size_t *length) {

    OV_ASSERT(0 != csrcs_out);
    OV_ASSERT(0 != rd_ptr);
    OV_ASSERT(0 != length);

    csrcs_out->num = num_csrcs;

    if (num_csrcs * sizeof(uint32_t) > *length) {

        ov_log_error("RTP header corrupt: Frame too small to contain %zu CSRCs",
                     num_csrcs);

        goto error;
    }

    uint8_t *ptr = *rd_ptr;
    size_t read_octets = 0;

    for (size_t i = 0; num_csrcs > i; ++i) {

        csrcs_out->ids[i] = ntohl(*(uint32_t *)ptr);
        ptr += 4;
        read_octets += 4;
    }

    *rd_ptr = ptr;
    *length -= read_octets;

    return true;

error:

    return false;
}
/*----------------------------------------------------------------------------*/

static bool get_extension_header_unsafe(ov_format_rtp_extension_header *hdr_out,
                                        uint8_t **rd_ptr, size_t *length) {

    OV_ASSERT(0 != hdr_out);
    OV_ASSERT(0 != rd_ptr);
    OV_ASSERT(0 != length);

    uint8_t *ptr = (uint8_t *)*rd_ptr;
    size_t consumed_octets = 0;

    OV_ASSERT(0 != ptr);

    uint16_t id = ntohs(*(uint16_t *)ptr);
    ptr += 2;
    consumed_octets += 2;

    size_t len = ntohs(*(uint16_t *)ptr);
    ptr += 2;
    consumed_octets += 2;

    const size_t data_len_octets = sizeof(uint32_t) * len;

    if (*length < consumed_octets) {

        ov_log_error("Corrupted frame: extension header longer than frame!");
        goto error;
    }

    /* Copy all the stuff ... */

    hdr_out->id = id;
    hdr_out->length_4_octets = len;
    hdr_out->payload = (uint32_t *)ptr;

    consumed_octets += data_len_octets;
    ptr += data_len_octets;

    *length -= consumed_octets;
    *rd_ptr = ptr;

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static bool get_padding_unsafe(ov_buffer *padded_payload,
                               ov_buffer *padding_out) {

    OV_ASSERT(0 != padded_payload);
    OV_ASSERT(0 != padding_out);

    if (1 > padded_payload->length) {

        goto payload_too_short;
    }

    size_t padding_len =
        (size_t)padded_payload->start[padded_payload->length - 1];

    if (padding_len > padded_payload->length) {

        goto payload_too_short;
    }

    padded_payload->length -= padding_len;

    padding_out->length = padding_len;
    padding_out->start = padded_payload->start + padded_payload->length;

    padded_payload->length -= padding_len;

    return true;

payload_too_short:
    ov_log_error("Payload too short for padding");

    return false;
}
/*****************************************************************************
                                   Interface
 ****************************************************************************/

static ov_buffer impl_next_chunk(ov_format *f, size_t requested_bytes,
                                 void *data) {

    UNUSED(requested_bytes);

    ov_buffer payload = {0};

    rtp_data *rdata = as_rtp_data(data);

    if (0 == rdata) {

        ov_log_error("Expected format rtp, but got something different");
        goto error;
    }

    ov_buffer buf = ov_format_payload_read_chunk_nocopy(f, 0);

    if (0 == buf.start) {

        goto error;
    }

    implicit_header iheader = {0};

    if (!get_rtp_header_unsafe(&rdata->header, &iheader, &buf.start,
                               &buf.length)) {

        goto error;
    }

    if (2 != rdata->header.version) {

        ov_log_error("Only supporting RTP v2, but %u found",
                     rdata->header.version);
        goto error;
    }

    if (!get_contributing_sources_unsafe(&rdata->csrcs, iheader.num_csrcs,
                                         &buf.start, &buf.length)) {

        ov_log_error("Could not read contributing sources");
        goto error;
    }

    if ((iheader.ext_header_set) &&
        (!get_extension_header_unsafe(&rdata->ext_header, &buf.start,
                                      &buf.length))) {

        ov_log_error("Could not get extension header from RTP frame");
        goto error;
    }

    if (iheader.padding_set && (!get_padding_unsafe(&buf, &rdata->padding))) {
        ov_log_error("Could not extract padding");
        goto error;
    }

    rdata->ext_header_set = iheader.ext_header_set;
    rdata->padding_set = iheader.padding_set;

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

    return -1;
}

/*----------------------------------------------------------------------------*/

static void *impl_create_data(ov_format *f, void *options) {

    UNUSED(f);
    UNUSED(options);

    rtp_data *rdata = calloc(1, sizeof(rtp_data));
    OV_ASSERT(0 != rdata);

    rdata->magic_bytes = RTP_MAGIC_BYTES;

    return rdata;
}

/*----------------------------------------------------------------------------*/

static void *impl_free_data(void *data) {

    if (0 == as_rtp_data(data)) {

        ov_log_error("Internal error: Expected to be called with format rtp");
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

bool ov_format_rtp_install(ov_format_registry *registry) {

    ov_format_handler handler = {
        .next_chunk = impl_next_chunk,
        .write_chunk = impl_write_chunk,
        .create_data = impl_create_data,
        .free_data = impl_free_data,
    };

    return ov_format_registry_register_type(OV_FORMAT_RTP_TYPE_STRING, handler,
                                            registry);
}

/*----------------------------------------------------------------------------*/

bool ov_format_rtp_get_header(ov_format const *f, ov_format_rtp_header *hdr) {

    if (0 == hdr) {

        ov_log_error("No receiving header given");
        goto error;
    }

    rtp_data *rdata = as_rtp_data(ov_format_get_custom_data(f));

    if (0 == rdata) {

        ov_log_error("Expected RTP format");
        goto error;
    }

    memcpy(hdr, &rdata->header, sizeof(ov_format_rtp_header));

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_format_rtp_get_contributing_sources(
    ov_format const *f, ov_format_rtp_contributing_sources *csrcs) {

    if (0 == csrcs) {

        ov_log_error("Require receiving csrc struct, got 0 pointer");
        goto error;
    }

    rtp_data *rdata = as_rtp_data(ov_format_get_custom_data(f));

    if (0 == rdata) {

        ov_log_error("Expected RTP format");
        goto error;
    }

    memcpy(csrcs, &rdata->csrcs, sizeof(*csrcs));

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_format_rtp_get_extension_header(ov_format const *f,
                                        ov_format_rtp_extension_header *ext) {

    if (0 == ext) {

        ov_log_error("Require receiving extension header struct, got 0 "
                     "pointer");
        goto error;
    }

    rtp_data const *rdata = as_rtp_data(ov_format_get_custom_data(f));

    if (0 == rdata) {

        ov_log_error("Expected RTP format");
        goto error;
    }

    if (!rdata->ext_header_set) {

        goto no_extension;
    }

    memcpy(ext, &rdata->ext_header, sizeof(*ext));

    return true;

no_extension:
error:

    return false;
}

/*----------------------------------------------------------------------------*/

ov_buffer *ov_format_rtp_get_padding(ov_format const *f) {

    rtp_data const *rdata = as_rtp_data(ov_format_get_custom_data(f));

    if (0 == rdata) {

        ov_log_error("Expected to be called with an RTP format");
        goto error;
    }

    if (!rdata->padding_set) {

        goto no_padding;
    }

    OV_ASSERT(0 != rdata->padding.start);

    ov_buffer *out = ov_buffer_create(rdata->padding.length);

    OV_ASSERT(0 != out);
    OV_ASSERT(out->capacity >= rdata->padding.length);

    memcpy(out->start, rdata->padding.start, rdata->padding.length);
    out->length = rdata->padding.length;

    return out;

no_padding:

    return (ov_buffer *)OV_FORMAT_RTP_NO_PADDING;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/
