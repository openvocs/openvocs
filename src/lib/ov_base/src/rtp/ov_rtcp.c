/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#include "../../include/ov_rtcp.h"
#include "../../include/ov_registered_cache.h"
#include "../../include/ov_string.h"
#include <arpa/inet.h>

/*----------------------------------------------------------------------------*/

char const *ov_rtcp_type_to_string(ov_rtcp_type type) {

    switch (type) {

        case OV_RTCP_INVALID:

            return "INVALID";

        case OV_RTCP_RECEIVER_REPORT:

            return "RECEIVER REPORT";

        case OV_RTCP_SENDER_REPORT:

            return "SENDER REPORT";

        case OV_RTCP_SOURCE_DESC:

            return "SOURCE DESC";

        case OV_RTCP_BYE:

            return "BYE";

        default:
            OV_ASSERT(!"MUST NEVER HAPPEN");
            return "INVALID";
    };
}

/*----------------------------------------------------------------------------*/

#define HEADER_LEN_OCTETS 4
#define CNAME_TYPE 0x01

#define MAGIC_BYTES 0xa387127a

struct ov_rtcp_message_struct {

    uint32_t magic_bytes;

    size_t len_octets;
    ov_rtcp_type type;
};

/*----------------------------------------------------------------------------*/

#define MAX_NUM_CHUNKS 31

typedef struct {

    ov_rtcp_message general;

    struct {

        uint32_t ssrc;
        // Text is max 255 in RTCP, but it is not null-terminated, hence we need
        // one additional byte for the 0
        char cname[255 + 1];

    } chunks[MAX_NUM_CHUNKS];

} sdes_message;

ov_registered_cache *g_sdes_cache = 0;

/*----------------------------------------------------------------------------*/

static bool init_message(ov_rtcp_message *self,
                         ov_rtcp_type type,
                         size_t payload_len_octets) {

    if (!ov_ptr_valid(self, "Cannot initialize message: 0 pointer")) {
        return false;
    } else {
        self->magic_bytes = MAGIC_BYTES;
        self->type = type;
        self->len_octets = HEADER_LEN_OCTETS + payload_len_octets;
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static sdes_message *new_sdes_message(size_t len_octets) {

    sdes_message *msg = ov_registered_cache_get(g_sdes_cache);

    if (0 == msg) {

        msg = calloc(1, sizeof(sdes_message));
        init_message(&msg->general, OV_RTCP_SOURCE_DESC, len_octets);

    } else {

        memset(&msg->chunks, 0, sizeof(msg->chunks));
    }

    return msg;
}
/*----------------------------------------------------------------------------*/

static ov_rtcp_message *as_rtcp_message(void *ptr) {

    ov_rtcp_message *msg = ptr;

    if ((!ov_ptr_valid(ptr, "Not an RTCP object: 0 pointer")) ||
        (!ov_cond_valid(MAGIC_BYTES == msg->magic_bytes,
                        "Not an RTCP object: Invalid Magic bytes"))) {

        return 0;

    } else {

        return msg;
    }
}

/*----------------------------------------------------------------------------*/

ov_rtcp_message *ov_rtcp_message_free(ov_rtcp_message *self) {

    switch (ov_rtcp_message_type(self)) {

        case OV_RTCP_INVALID:

            ov_log_error("Won't free RTCP object: Invalid object");
            return self;

        case OV_RTCP_SOURCE_DESC:

            return ov_free(self);

        case OV_RTCP_BYE:
        case OV_RTCP_SENDER_REPORT:

            return ov_free(self);

        default:

            ov_log_error("Won't free message: Unsupported type");
            return self;
    }
}

/*----------------------------------------------------------------------------*/

size_t ov_rtcp_message_len_octets(ov_rtcp_message const *self) {

    if (ov_ptr_valid(
            as_rtcp_message((void *)self), "Not a valid RTCP message")) {

        return self->len_octets;

    } else {

        return 0;
    }
}

/*****************************************************************************
                                     Decode
 ****************************************************************************/

struct data {

    uint8_t const *ptr;
    size_t length;
};

static struct data to_data(uint8_t const **in, size_t *lenoctets) {

    if ((!ov_ptr_valid(in, "Cannot decode RTCP: No data (0 pointer)")) ||
        (!ov_ptr_valid(*in, "Cannot decode RTCP: No data (0 pointer)")) ||
        (!ov_ptr_valid(
            lenoctets, "Cannot decode RTCP: No data size (0 pointer)"))) {
        return (struct data){0};
    } else {

        return (struct data){
            .ptr = *in,
            .length = *lenoctets,
        };
    }
}

/*----------------------------------------------------------------------------*/

static ov_rtcp_type to_type(uint8_t n) {

    switch (n) {

        case 200:
            return OV_RTCP_SENDER_REPORT;

        case 201:
            return OV_RTCP_RECEIVER_REPORT;

        case 202:
            return OV_RTCP_SOURCE_DESC;

        case 203:
            return OV_RTCP_BYE;

        default:
            return OV_RTCP_INVALID;
    };
}

/*----------------------------------------------------------------------------*/

struct rtcp_header {

    ov_rtcp_type type;
    // Length of the REMAINDER OF THE RTCP frame (without header!)
    size_t length;

    struct {
        bool pad : 1;
        unsigned count : 5; // Interpretation depends on type
    };
};

/*----------------------------------------------------------------------------*/

static struct rtcp_header decode_header(struct data *data) {

    struct rtcp_header header = {0};
    header.type = OV_RTCP_INVALID;

    if ((0 == data) || (4 > data->length)) {

        ov_log_error("Not enough data to decode to RTCP header");
        return header;

    } else if (0x80 != (data->ptr[0] & 0xc0)) {

        ov_log_error(
            "Cannot decode RTCP header: Expected version number '2', but got "
            "%u",
            data->ptr[0] & 3);

        return header;

    } else {

        header.pad = (data->ptr[0] & 0x20);

        header.count = data->ptr[0] & 0x8f;
        header.type = to_type(data->ptr[1]);
        header.length = ntohs(*(uint16_t *)(data->ptr + 2)) * 4;

        fprintf(stderr,
                "RTCP type: %s, length %zu, Count: %i\n",
                ov_rtcp_type_to_string(header.type),
                header.length,
                header.count);

        data->ptr += 4;
        data->length -= 4;

        return header;
    }
}

/*----------------------------------------------------------------------------*/

static bool copy_text(char *target,
                      size_t target_capacity_octets,
                      uint8_t const *src,
                      size_t octets_to_copy) {

    if ((0 == target) ||
        (!ov_cond_valid(target_capacity_octets >= octets_to_copy + 1,
                        "Cannot decode RTCP SDES message: Target buffer for "
                        "Text element too small"))) {
        return false;
    } else {

        memcpy(target, src, octets_to_copy);
        target[octets_to_copy] = 0;
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static size_t pad_octets_to_align(size_t num_unaligned_octets) {

    return (4 - num_unaligned_octets % 4) % 4;
}

/*----------------------------------------------------------------------------*/

static bool skip_alignment(uint8_t const **data,
                           size_t *num_octets,
                           size_t consumed_octets) {

    if ((0 != data) && (0 != *data) && (0 != num_octets)) {

        size_t octets_to_skip = consumed_octets % 4;
        *num_octets -= octets_to_skip;
        *data += octets_to_skip;
        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool get_item(uint8_t const **in,
                     size_t *in_octets,
                     uint8_t item_type,
                     char *target_buffer,
                     size_t target_buffer_capacity_octets) {

    // TODO: Check in, in_octes, target_buffer for validity

    bool found = false;
    size_t consumed_octets = 0;

    while (true) {

        uint8_t const *data = *in;

        if (!ov_cond_valid(*in_octets >= 1,
                           "Cannot parse RTCP SDES message item: Too less "
                           "input data (require at least 1 byte")) {

            return false;

        } else if (0 == data[0]) {

            skip_alignment(in, in_octets, consumed_octets);
            return found;

        } else if (!ov_cond_valid(*in_octets >= 2,
                                  "Cannot parse RTCP SDES message item: Too "
                                  "less "
                                  "input data (require at least 2 byte")) {

            return false;

        } else {

            uint8_t len_item_octets = data[1];

            if (!ov_cond_valid(len_item_octets + consumed_octets <= *in_octets,
                               "Cannot parse RTCP SDES item: Insufficient "
                               "input "
                               "data")) {

                return false;

            } else if (data[0] == item_type) {

                ov_log_info("Decoding SDES item: Found proper item: %" PRIu8,
                            item_type);

                copy_text(target_buffer,
                          target_buffer_capacity_octets,
                          data + 2,
                          len_item_octets);
                found = true;
            }

            *in += 2 + len_item_octets;
            consumed_octets += 2 + len_item_octets;
        }
    }
}

/*----------------------------------------------------------------------------*/

static bool parse_chunk(uint8_t const **in,
                        size_t *in_num_octets,
                        size_t chunk_index,
                        sdes_message *msg) {

    if ((!ov_ptr_valid(
            in, "Cannot parse chunk of RTCP SDES message: No input")) ||
        (!ov_ptr_valid(in_num_octets,
                       "Cannot parse chunk of RTCP SDES message: No input "
                       "length (0 pointer)")) ||
        (!ov_ptr_valid(msg,
                       "Cannot parse chunk of RTCP SDES message: No target "
                       "message (0 pointer)")) ||
        (!ov_cond_valid(chunk_index < MAX_NUM_CHUNKS,
                        "Cannot parse chunk of RTCP SDES message: chunk "
                        "index "
                        "out of range")) ||
        (!ov_cond_valid(*in_num_octets >= 4,
                        "Cannot decode RTCP SDES message: Insufficient "
                        "data"))) {

        return false;

    } else {

        size_t num_octets = *in_num_octets;
        uint8_t const *data = *in;

        uint32_t ssrc = ntohl(*(uint32_t *)data);
        data += 4;
        num_octets -= 4;

        if (!get_item(&data,
                      &num_octets,
                      1,
                      msg->chunks[chunk_index].cname,
                      sizeof(msg->chunks[chunk_index].cname))) {

            return false;

        } else {

            *in = data;
            *in_num_octets = num_octets;
            msg->chunks[chunk_index].ssrc = ssrc;
            return true;
        }
    }
}

/*----------------------------------------------------------------------------*/

static bool parse_chunks(uint8_t const *in,
                         size_t in_num_octets,
                         size_t num_chunks,
                         sdes_message *msg) {

    if ((!ov_ptr_valid(in, "Cannot decode RTCP SDES message: No input data")) ||
        (!ov_ptr_valid(
            msg, "Cannot decode RTCP SDES message: No target message"))) {

        return false;

    } else {

        for (size_t i = 0; i < num_chunks; ++i) {

            if (!parse_chunk(&in, &in_num_octets, i, msg)) {
                return false;
            }
        }

        return true;
    }
}

/*----------------------------------------------------------------------------*/

static ov_rtcp_message *decode_sdes(struct rtcp_header header,
                                    struct data data) {

    if (0 == header.count) {

        return ov_rtcp_message_sdes(0, 0);

    } else {

        if (1 < header.count) {

            ov_log_warning(
                "Got %zu SDES descriptions, but only taking 1 into account",
                header.count);
        }

        sdes_message *msg = new_sdes_message(header.length);

        if (parse_chunks(data.ptr, data.length, header.count, msg)) {

            return &msg->general;

        } else {

            ov_rtcp_message_free(&msg->general);
            msg = 0;
            return 0;
        }
    }
}

/*----------------------------------------------------------------------------*/

static ov_rtcp_message *decode_generic_message(struct rtcp_header header,
                                               struct data data) {

    UNUSED(data);

    if (4 <= header.length) {

        ov_rtcp_message *msg = calloc(1, sizeof(ov_rtcp_message));
        init_message(msg, header.type, header.length);

        return msg;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

ov_rtcp_message *ov_rtcp_message_decode(uint8_t const **buf,
                                        size_t *lenoctets) {

    struct data data = to_data(buf, lenoctets);

    struct rtcp_header header = decode_header(&data);

    if (0 == header.length) {

        return 0;

    } else if (data.length < header.length) {

        ov_log_warning(
            "Cannot decode RTCP message: Message incomplete - "
            "expect %zu bytes, but only got %zu bytes",
            header.length,
            data.length);

        return 0;

    } else {

        ov_rtcp_message *msg = 0;

        switch (header.type) {

            case OV_RTCP_SOURCE_DESC:

                msg = decode_sdes(header, data);
                break;

            case OV_RTCP_BYE:
            case OV_RTCP_INVALID:
            case OV_RTCP_RECEIVER_REPORT:
            case OV_RTCP_SENDER_REPORT:

                msg = decode_generic_message(header, data);
                break;

            default:
                OV_ASSERT(!"MUST NEVER HAPPEN");
                return 0;
        };

        // Skip over read message
        size_t consumed_octets = header.length + data.ptr - *buf;
        *buf += consumed_octets;
        *lenoctets -= consumed_octets;

        OV_ASSERT(*buf == data.ptr + header.length);

        return msg;
    }
}

/*****************************************************************************
                                     encode
 ****************************************************************************/

static bool encode_header(ov_rtcp_message const *self,
                          uint8_t **out,
                          size_t *out_len) {

    if ((!ov_ptr_valid(self, "Cannot encode RTCP header: No message")) ||
        (!ov_ptr_valid(out, "Cannot encode RTCP header: No target buffer")) ||
        (!ov_ptr_valid(*out, "Cannot encode RTCP header: No target buffer")) ||
        (!ov_ptr_valid(
            out_len, "Cannot encode RTCP header: No target buffer length")) ||
        (!ov_cond_valid(4 <= *out_len,
                        "Cannot encode RTCP header: Target buffer too "
                        "small"))) {

        return false;

    } else {

        uint8_t *write_ptr = *out;

        write_ptr[0] =
            0x81; // beware: We only support SDES with 1 chunk for now
        write_ptr[1] = OV_RTCP_SOURCE_DESC;

        // Remember: Size is size without header, in 32 bit words
        uint16_t payload_len =
            htons((ov_rtcp_message_len_octets(self) - 4) / 4);

        uint8_t *payload_len_bp = (uint8_t *)&payload_len;

        write_ptr[2] = payload_len_bp[0];
        write_ptr[3] = payload_len_bp[1];

        *out = write_ptr + 4;
        *out_len -= 4;

        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool encode_ssrc(uint32_t ssrc, uint8_t **out, size_t *out_len) {

    if ((!ov_ptr_valid(out, "Cannot encode RTCP header: No target buffer")) ||
        (!ov_ptr_valid(*out, "Cannot encode RTCP header: No target buffer")) ||
        (!ov_ptr_valid(
            out_len, "Cannot encode RTCP header: No target buffer length")) ||
        (!ov_cond_valid(4 <= *out_len,
                        "Cannot encode RTCP header: Target buffer too "
                        "small"))) {

        return false;

    } else {

        uint8_t *write_ptr = *out;

        ssrc = htonl(ssrc);
        uint8_t *ssrc_bp = (uint8_t *)&ssrc;

        write_ptr[0] = ssrc_bp[0];
        write_ptr[1] = ssrc_bp[1];
        write_ptr[2] = ssrc_bp[2];
        write_ptr[3] = ssrc_bp[3];

        *out = write_ptr + 4;
        *out_len -= 4;

        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool encode_cname_entry(char const *cname,
                               uint8_t **out,
                               size_t *out_len) {

    size_t cname_len = ov_string_len(cname);

    if ((!ov_ptr_valid(cname, "Cannot encode RTCP cname: No CNAME")) ||
        (!ov_ptr_valid(out, "Cannot encode RTCP cname: No target buffer")) ||
        (!ov_ptr_valid(*out, "Cannot encode RTCP cname: No target buffer")) ||
        (!ov_ptr_valid(
            out_len, "Cannot encode RTCP cname: No target buffer length")) ||
        (!ov_cond_valid(
            cname_len < 256, "Cannot encode RTCP cname: CNAME too long")) ||
        (!ov_cond_valid(cname_len + 2 <= *out_len,
                        "Cannot encode RTCP cname: Target buffer too small"))) {

        return false;

    } else {

        uint8_t *write_ptr = *out;

        write_ptr[0] = CNAME_TYPE;
        write_ptr[1] = (uint8_t)cname_len;

        *out += 2;

        memcpy(*out, (uint8_t const *)cname, cname_len);

        *out += cname_len;
        *out_len -= cname_len + 2;

        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool align_to_32_bit_boundary(uint8_t **data,
                                     size_t *num_octets,
                                     size_t consumed_octets) {

    if ((0 != data) && (0 != *data) && (0 != num_octets)) {

        size_t num_octets_to_pad = pad_octets_to_align(consumed_octets);
        memset(*data, 0, num_octets_to_pad);
        *data += num_octets_to_pad;
        *num_octets -= num_octets_to_pad;

        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool encode_sdes(ov_rtcp_message const *self,
                        uint8_t **out,
                        size_t *out_len) {

    if ((!ov_ptr_valid(self, "Cannot encode RTCP header: No message")) ||
        (!ov_ptr_valid(out, "Cannot encode RTCP header: No target buffer")) ||
        (!ov_ptr_valid(*out, "Cannot encode RTCP header: No target buffer")) ||
        (!ov_ptr_valid(
            out_len, "Cannot encode RTCP header: No target buffer length"))) {

        return false;

    } else {
        uint8_t *start = *out;

        return encode_ssrc(ov_rtcp_message_sdes_ssrc(self, 0), out, out_len) &&
               encode_cname_entry(
                   ov_rtcp_message_sdes_cname(self, 0), out, out_len) &&
               align_to_32_bit_boundary(out, out_len, *out - start);
    }
}

/*----------------------------------------------------------------------------*/

static size_t calc_required_octets(ov_rtcp_message const *msgs[]) {

    size_t octets = 0;

    for (size_t i = 0; 0 != msgs[i]; ++i) {
        size_t msg_octets = ov_rtcp_message_len_octets(msgs[i]);

        if (4 > msg_octets) {
            ov_log_error(
                "Cannot encode RTCP messages: Message too short, expect at "
                "least 4 "
                "octets, got %zu",
                msg_octets);
            octets = 0;
            break;

        } else {

            octets += ov_rtcp_message_len_octets(msgs[i]);
        }
    }

    return octets;
}

/*----------------------------------------------------------------------------*/

static bool encode_message(ov_rtcp_message const *self,
                           uint8_t **data,
                           size_t *len) {

    if (encode_header(self, data, len)) {

        switch (ov_rtcp_message_type(self)) {

            case OV_RTCP_SOURCE_DESC:

                return encode_sdes(self, data, len);

            default:

                ov_log_error(
                    "Cannot encode RTCP Message - type %s currently "
                    "unsupported",
                    ov_string_sanitize(
                        ov_rtcp_type_to_string(ov_rtcp_message_type(self))));
                return false;
        };

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

ov_buffer *ov_rtcp_messages_encode(ov_rtcp_message const *msgs[]) {

    size_t required_octets = calc_required_octets(msgs);

    if (4 > required_octets) {

        return 0;

    } else {

        OV_ASSERT(4 <= required_octets);

        ov_buffer *buffer = ov_buffer_create(required_octets);
        OV_ASSERT(0 != buffer);

        uint8_t *data = buffer->start;
        size_t len = required_octets;

        bool ok = true;

        for (size_t i = 0; ok && (0 != msgs[i]); ++i) {
            ok = encode_message(msgs[i], &data, &len);
        }

        if (ok) {

            buffer->length = data - buffer->start;

        } else {

            buffer = ov_buffer_free(buffer);
        }

        return buffer;
    }
}

/*----------------------------------------------------------------------------*/

ov_rtcp_type ov_rtcp_message_type(ov_rtcp_message const *self) {

    if (ov_ptr_valid(
            as_rtcp_message((void *)self), "Not a valid RTCP message")) {

        return self->type;

    } else {

        return OV_RTCP_INVALID;
    }
}

/*----------------------------------------------------------------------------*/

static size_t calculate_sdes_msg_len(char const *cname) {

    size_t payload_len = 4 + 2 + ov_string_len(cname);

    payload_len += pad_octets_to_align(payload_len);

    OV_ASSERT(0 == payload_len % 4);

    return payload_len;
}

/*----------------------------------------------------------------------------*/

ov_rtcp_message *ov_rtcp_message_sdes(char const *cname, uint32_t ssrc) {

    size_t cname_len = ov_string_len(cname);

    if (!ov_cond_valid(cname_len < 256,
                       "Cannot create RTCP SDES message: CNAME too long - "
                       "max is 255 octets")) {

        return 0;

    } else {

        sdes_message *msg = new_sdes_message(calculate_sdes_msg_len(cname));

        ov_string_copy(
            msg->chunks[0].cname, cname, sizeof(msg->chunks[0].cname));
        msg->chunks[0].ssrc = ssrc;

        return &msg->general;
    }
}

/*----------------------------------------------------------------------------*/

uint32_t ov_rtcp_message_sdes_ssrc(ov_rtcp_message const *self,
                                   size_t chunk_index) {

    if ((!ov_cond_valid(OV_RTCP_SOURCE_DESC == ov_rtcp_message_type(self),
                        "Cannot free message: Not an SDES message")) ||
        (!ov_cond_valid(chunk_index < MAX_NUM_CHUNKS,
                        "Chunk index out of range - at most 31 chunks ares "
                        "supported"))) {
        return 0;

    } else {

        return ((sdes_message *)self)->chunks[chunk_index].ssrc;
    }
}

/*----------------------------------------------------------------------------*/

static char const *canonize_cname(char const *cname) {

    if ((0 == cname) || (0 == cname[0])) {
        return 0;
    } else {
        return cname;
    }
}

/*----------------------------------------------------------------------------*/

char const *ov_rtcp_message_sdes_cname(ov_rtcp_message const *self,
                                       size_t chunk_index) {

    sdes_message const *sdes_msg = (sdes_message const *)self;

    if ((!ov_cond_valid(OV_RTCP_SOURCE_DESC == ov_rtcp_message_type(self),
                        "Cannot free message: Not an SDES message")) ||
        (!ov_cond_valid(chunk_index < MAX_NUM_CHUNKS,
                        "Chunk index out of range - at most 31 chunks ares "
                        "supported"))) {
        return 0;

    } else {

        return canonize_cname(sdes_msg->chunks[chunk_index].cname);
    }
}

/*----------------------------------------------------------------------------*/

static void *rtcp_free(void *vptr) {

    return ov_rtcp_message_free(as_rtcp_message(vptr));
}

/*----------------------------------------------------------------------------*/

void ov_rtpc_enable_caching(size_t capacity) {

    ov_registered_cache_config cfg = {

        .capacity = capacity,
        .item_free = rtcp_free,

    };

    g_sdes_cache = ov_registered_cache_extend("rtcp_sdes", cfg);

    OV_ASSERT(0 != g_sdes_cache);
}

/*----------------------------------------------------------------------------*/
