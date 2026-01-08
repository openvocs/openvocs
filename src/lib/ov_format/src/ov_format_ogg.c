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

        @author         Michael J. Beer, DLR/GSOC


        Ogg organises in pages / segments.

        Each write_chunk writes into one segment. If it does not fit into one
        segment, it is cut into seqments of maxlen, followed by one segment
        with the remainder (See ogg rfc 3533).

        read_next_chunk then returns exactly that chunk by reassembling all
        those segments back into the original chunk.

        Writing

        is rather simple: Just append segments to current page until page is
        full, then serialize it, and start new one.


        Reading

        is a bit more tricky since one has to assemble the segments back
        into the original chunk...

        ------------------------------------------------------------------------
*/

#include "../include/ov_format_ogg.h"
#include <inttypes.h>
#include <ov_arch/ov_byteorder.h>
#include <ov_base/ov_buffer.h>
#include <ov_base/ov_chunker.h>
#include <ov_base/ov_crc32.h>
#include <ov_base/ov_utils.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

char const *OV_FORMAT_OGG_TYPE_STRING = "ogg";

size_t const OFFSET_CRC32 = 22;
size_t const OFFSET_NUM_SEGMENTS = 26;
size_t const OFFSET_SEGMENT_TABLE = 27;

typedef struct {

    struct header_type_flags {

        bool continuation : 1;
        bool begin_of_stream : 1;
        bool end_of_stream : 1;

    } flags;

    // Only used when decoding
    uint32_t crc32;

    int64_t sample_position; // Yes, this can assume negative values (-1 denotes
                             // no single frame finishes on this page)

    uint32_t stream_serial_number;
    uint32_t sequence_number;

    uint8_t num_segments;

    uint8_t segment_table[0xff];

} PageHeader;

/*----------------------------------------------------------------------------*/

#define MAX_LEN_PAGE_OCTETS (0xffff)

typedef struct {

    PageHeader header;

    // In Read mode: Contains entire read page
    // In Write mode: Contains Payload. Header data is kept decoded and written
    // directly to lower layer when encoded
    struct {
        uint8_t data[MAX_LEN_PAGE_OCTETS];
        size_t len_octets;
    } bitstream;

} Page;

/*----------------------------------------------------------------------------*/

typedef struct {

    uint64_t samples_per_chunk;

    struct {
        bool stream_start : 1;
    };

    uint32_t stream_serial_number;
    uint32_t sequence_number;

} OggParameters;

/*----------------------------------------------------------------------------*/

typedef struct {

    uint32_t magic_bytes;

    ov_chunker *chunker;
    ov_format *lower_format;

    OggParameters parameters;

    Page page;

} OggOut;

/*----------------------------------------------------------------------------*/

uint32_t OGG_OUT_MAGIC_BYTES = 0xaf3f12ff;

static OggOut *as_ogg_out(void *data) {

    OggOut *oggout = data;
    if (ov_ptr_valid(oggout, "Invalid ogg object - 0 pointer") &&
        ov_cond_valid(OGG_OUT_MAGIC_BYTES == oggout->magic_bytes,
                      "Invalid oggout object - invalid magic bytes")) {

        return oggout;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static bool page_increase_sample_count(Page *page, uint32_t inc) {

    if (!ov_ptr_valid(page, "Cannot increase sample count - no page given")) {

        return false;

    } else {

        page->header.sample_position += inc;
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool page_clear(Page *page, bool continuation,
                       OggParameters const *params) {

    if (!ov_ptr_valid(params,
                      "Cannot create new Ogg page: No OGG parameters")) {

        return 0;

    } else {

        memset(page, 0, sizeof(Page));

        page->header.flags.begin_of_stream = params->stream_start;
        page->header.flags.continuation = continuation;
        page->header.stream_serial_number = params->stream_serial_number;
        page->header.sequence_number = params->sequence_number;
        page->header.sample_position = 0;

        return page;
    }
}

/*----------------------------------------------------------------------------*/

OggOut *ogg_out_create(ov_format *lower_format,
                       ov_format_ogg_options *options) {

    if (ov_ptr_valid(options, "Invalid OGG options: 0 pointer")) {
        OggOut *ogg = calloc(1, sizeof(OggOut));

        ogg->magic_bytes = OGG_OUT_MAGIC_BYTES;

        ogg->lower_format = lower_format;

        ogg->chunker = ov_chunker_create();

        ogg->parameters.samples_per_chunk =
            options->chunk_length_ms * options->samplerate_hz;

        ogg->parameters.samples_per_chunk /= 1000;

        ogg->parameters.stream_serial_number = options->stream_serial;
        ogg->parameters.stream_start = true;

        page_clear(&ogg->page, false, &ogg->parameters);

        return ogg;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ssize_t serialize_page(ov_format *fout, Page const *page);

static bool finish_stream(OggOut *self) {

    if (ov_ptr_valid(self, "Cannot finish stream - invalid oggout object")) {
        self->page.header.flags.end_of_stream = true;

        bool ok = serialize_page(self->lower_format, &self->page);

        // each stream got its own page numbering
        self->parameters.sequence_number = 0;
        self->parameters.stream_start = true;
        return ok && page_clear(&self->page, false, &self->parameters);

    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

OggOut *ogg_out_free(OggOut *self) {

    if (finish_stream(self)) {
        self->chunker = ov_chunker_free(self->chunker);
    }

    return ov_free(self);
}

/*----------------------------------------------------------------------------*/

static uint8_t encode_flags(struct header_type_flags flags) {

    return flags.continuation * 0x01 + flags.begin_of_stream * 0x02 +
           flags.end_of_stream * 0x04;
}

/*----------------------------------------------------------------------------*/

static size_t encode_64(uint64_t u64, uint8_t **out) {

    OV_ASSERT((0 != out) && (0 != *out));

    u64 = OV_H64TOLE(u64);
    memcpy(*out, &u64, sizeof(u64));

    *out += sizeof(u64);

    return sizeof(u64);
}

/*----------------------------------------------------------------------------*/

static size_t encode_32(uint32_t u32, uint8_t **out) {

    OV_ASSERT((0 != out) && (0 != *out));

    u32 = OV_H32TOLE(u32);
    memcpy(*out, &u32, sizeof(u32));

    *out += sizeof(u32);

    return sizeof(u32);
}

/*----------------------------------------------------------------------------*/

/**
 * Leaves the CRC at 0.
 * The CRC is calculated over the page header and the actual data
 * with CRC in header set to 0
 * Thus we first require the page header without CRC, calculate the CRC
 * and then insert the CRC to the page header.
 */
static ssize_t serialize_page_header(PageHeader const header,
                                     uint8_t *target_buffer,
                                     size_t target_buffer_len_octets) {

    size_t written_octets = 0;

    if ((!ov_ptr_valid(target_buffer,
                       "Cannot serialize OGG Page: No target buffer")) ||
        (!ov_cond_valid(target_buffer_len_octets >=
                            (size_t)27 + header.num_segments,
                        "Cannot serialize OGG Page: Target buffer too "
                        "small"))) {

        return -1;

    } else {

        char *tb = (char *)target_buffer;

        memcpy(tb, "OggS", 4);
        tb[4] = 0;
        tb[5] = encode_flags(header.flags);

        tb += 6;
        written_octets = 6;

        written_octets += encode_64(OV_TWOS_COMPLEMENT(header.sample_position),
                                    (uint8_t **)&tb);

        written_octets +=
            encode_32(header.stream_serial_number, (uint8_t **)&tb);

        written_octets += encode_32(header.sequence_number, (uint8_t **)&tb);

        // Skip CRC
        memset(tb, 0, sizeof(uint32_t));
        tb[4] = header.num_segments;
        tb += 5;
        written_octets += 5;

        OV_ASSERT(27 == written_octets);

        memcpy(tb, &header.segment_table,
               header.num_segments * sizeof(uint8_t));

        written_octets += header.num_segments;

        return written_octets;
    }
}

/*----------------------------------------------------------------------------*/

static bool write_to_format(ov_format *fout, uint8_t const *data,
                            size_t len_octets) {

    // Complicated, but how else to do it safely?

    if (ov_ptr_valid(data, "Cannot write to format - no data")) {

        ov_buffer *buf = ov_buffer_create(1);
        uint8_t *original_start = buf->start;
        uint8_t original_capacity = buf->capacity;

        buf->start = (uint8_t *)data;
        buf->length = len_octets;
        buf->capacity = len_octets;

        ssize_t written_octets = ov_format_payload_write_chunk(fout, buf);

        buf->start = original_start;
        buf->capacity = original_capacity;
        buf->length = 0;

        buf = ov_buffer_free(buf);

        return (written_octets >= 0) && (len_octets == (size_t)written_octets);

    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static ssize_t serialize_page(ov_format *fout, Page const *page) {

    uint8_t out[27 + 255] = {0};
    size_t out_len_octets = sizeof(out);

    if ((!ov_ptr_valid(page, "Cannot serialize OGG page: No page given"))) {

        return -1;

    } else {

        ssize_t written_octets =
            serialize_page_header(page->header, out, out_len_octets);

        if (0 > written_octets) {

            return -1;

        } else {

            uint32_t crc32 = OV_H32TOLE(
                ov_crc32_ogg(ov_crc32_ogg(0, out, written_octets),
                             page->bitstream.data, page->bitstream.len_octets));

            memcpy(out + OFFSET_CRC32, &crc32, sizeof(crc32));

            if (write_to_format(fout, out, written_octets) &&
                write_to_format(fout, page->bitstream.data,
                                page->bitstream.len_octets)) {

                return written_octets + page->bitstream.len_octets;

            } else {

                ov_log_error("I/O error when writing ogg");
                return -1;
            }
        }
    }
}

/*----------------------------------------------------------------------------*/

static bool page_set_sample_position(Page *page, int64_t sample_pos) {

    if ((-1 < sample_pos) &&
        ov_ptr_valid(page, "Cannot set sample position: Page invalid")) {

        page->header.sample_position = sample_pos;
        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool rotate_page(ov_format *fout, Page *page, bool continuation,
                        OggParameters *params) {

    if ((!ov_ptr_valid(page, "Cannot rotate page: Invalid page")) ||
        (!ov_ptr_valid(params, "Cannot rotate page: Invalid parameters"))) {

        return false;

    } else if (UINT8_MAX == page->header.num_segments) {

        params->stream_start = false;
        params->sequence_number++;

        int64_t current_sample_pos = page->header.sample_position;

        return ov_cond_valid(serialize_page(fout, page),
                             "Cannot serialize page") &&
               ov_cond_valid(page_clear(page, continuation, params),
                             "Cannot reset page") &&
               page_set_sample_position(page, current_sample_pos);

    } else {

        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool add_segment(ov_format *fout, Page *page, ov_buffer *segment,
                        bool continuation, OggParameters *params) {

    size_t seglen = ov_buffer_len(segment);

    if ((ov_ptr_valid(params, "Cannot serialize to OGG: No OGG parameters")) &&
        (ov_cond_valid(UINT8_MAX >= seglen,
                       "Cannot add Ogg segment that exceeds maximum "
                       "length"))) {

        rotate_page(fout, page, continuation, params);

        OV_ASSERT(page->header.num_segments <= UINT8_MAX);

        page->header.segment_table[page->header.num_segments++] = seglen;

        memcpy(page->bitstream.data + page->bitstream.len_octets,
               segment->start, seglen);
        page->bitstream.len_octets += seglen;

        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static ssize_t serialize_to_pages(ov_format *fout, ov_chunker *chunker,
                                  OggParameters *params, Page *page) {

    ov_buffer *segment = ov_chunker_next_chunk(chunker, 255);

    ssize_t serialized_octets = 0;

    bool continuation = false;

    while (0 != segment) {

        add_segment(fout, page, segment, continuation, params);

        serialized_octets += 255;

        segment = ov_buffer_free(segment);
        segment = ov_chunker_next_chunk(chunker, 255);

        continuation = true;
    };

    // until here, we only wrote segments of exactly 255 octets, thus
    // we need either to write a last segment with less than 255 octets
    // or an empty segment
    segment = ov_chunker_remainder(chunker);
    add_segment(fout, page, segment, continuation, params);

    serialized_octets += ov_buffer_len(segment);

    segment = ov_buffer_free(segment);

    page_increase_sample_count(page, params->samples_per_chunk);

    return serialized_octets;
}

/*----------------------------------------------------------------------------*/

static ssize_t impl_write_chunk(ov_format *format, ov_buffer const *chunk,
                                void *data) {

    OggOut *out = as_ogg_out(data);

    if ((!ov_ptr_valid(out, "Cannot write chunk: Invalid Ogg format")) ||
        (!ov_ptr_valid(chunk, "Cannot write chunk: No chunk"))) {

        return -1;

    } else {

        ov_chunker_add(out->chunker, chunk);

        return serialize_to_pages(format, out->chunker, &out->parameters,
                                  &out->page);
    }
}

/*****************************************************************************
                                    READING
 ****************************************************************************/

typedef struct {

    uint32_t magic_bytes;

    struct {
        uint32_t serial_number;

        bool has_been_set;
        bool start_found;
    } primary_stream;

    size_t current_segment;
    size_t current_segment_offset;

    Page page;

} OggIn;

uint32_t OGG_IN_MAGIC_BYTES = 0xaf3f12f0;

/*----------------------------------------------------------------------------*/

static OggIn *as_ogg_in(void *data) {

    OggIn *oggin = data;
    if (ov_ptr_valid(oggin, "Invalid OggIn object - 0 pointer") &&
        (ov_cond_valid(OGG_IN_MAGIC_BYTES == oggin->magic_bytes,
                       "Invalid OggIn object - magic bytes invalid"))) {
        return oggin;
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

OggIn *ogg_in_create(ov_format_ogg_options *options) {

    OggIn *ogg_in = calloc(1, sizeof(OggIn));

    ogg_in->magic_bytes = OGG_IN_MAGIC_BYTES;

    if ((0 != options) && (0 != options->stream_serial)) {

        ogg_in->primary_stream.has_been_set = true;
        ogg_in->primary_stream.serial_number = options->stream_serial;
    }

    return ogg_in;
}

/*----------------------------------------------------------------------------*/

OggIn *ogg_in_free(OggIn *self) { return ov_free(self); }

static bool get_octets(ov_format *format, Page *page, size_t num_octets) {

    if ((!ov_ptr_valid(page, "Cannot read next page: Invalid page object")) ||
        (!ov_cond_valid(num_octets + page->bitstream.len_octets <=
                            MAX_LEN_PAGE_OCTETS,
                        "Tried to read page larger than maximum"))) {

        return false;

    } else {

        ov_buffer buf = ov_format_payload_read_chunk_nocopy(format, num_octets);

        if (0 == buf.length) {

            ov_log_info("End of stream reached");
            return false;

        } else if (num_octets != buf.length) {

            ov_log_error("Could not get octets: Expected %zu, but got %zu",
                         num_octets, buf.length);

            return false;

        } else {

            memcpy(page->bitstream.data + page->bitstream.len_octets, buf.start,
                   buf.length);
            page->bitstream.len_octets += buf.length;

            return true;
        }
    }
}

/*----------------------------------------------------------------------------*/

static uint64_t decode_64(uint8_t const *in) {

    OV_ASSERT(0 != in);

    uint64_t result = 0;
    memcpy(&result, in, sizeof(result));
    return OV_LE64TOH(result);
}

/*----------------------------------------------------------------------------*/

static uint64_t decode_32(uint8_t const *in) {

    OV_ASSERT(0 != in);

    uint32_t result = 0;
    memcpy(&result, in, sizeof(result));
    return OV_LE32TOH(result);
}

/*----------------------------------------------------------------------------*/

static void print_flags(bool c, bool b, bool e) {

    if (c) {

        fprintf(stdout, "Continuation   ");
    }

    if (b) {

        fprintf(stdout, "Begin of Stream   ");
    }

    if (e) {

        fprintf(stdout, "End of Stream   ");
    }
}

/*----------------------------------------------------------------------------*/

static void print_header(PageHeader const *header) {

    if (0 != header) {

        fprintf(stdout,
                "Segment %" PRIu32 ": CRC: %" PRIu32 "  Segments: %" PRIu8
                "   Stream id %" PRIu32 "   Sample position: %" PRIi64 "   ",
                header->sequence_number, header->crc32, header->num_segments,
                header->stream_serial_number, header->sample_position);

        print_flags(header->flags.continuation, header->flags.begin_of_stream,
                    header->flags.end_of_stream);

        fprintf(stdout, "\n");

    } else {

        fprintf(stdout, "Page header is null\n");
    }
}

/*----------------------------------------------------------------------------*/

static bool decode_base_header(Page *page) {

    if ((!ov_ptr_valid(page, "Cannot decode header - invalid page")) ||
        (!ov_cond_valid(page->bitstream.len_octets > 26,
                        "Cannot decode header - not enough data")) ||
        (!ov_cond_valid(0 == memcmp("OggS", page->bitstream.data, 4),
                        "Cannot decode ogg header, magic bytes 'OggS' not "
                        "found")) ||
        (!ov_cond_valid(0 == page->bitstream.data[4],
                        "Cannot decode Ogg header - invalid ogg format, "
                        "expect "
                        "version 0"))) {

        return false;

    } else {

        uint8_t flags = page->bitstream.data[5];
        page->header.flags.continuation = (0 != (flags & 0x01));
        page->header.flags.begin_of_stream = (0 != (flags & 0x02));
        page->header.flags.end_of_stream = (0 != (flags & 0x04));

        page->header.sample_position = decode_64(page->bitstream.data + 6);
        page->header.stream_serial_number =
            decode_32(page->bitstream.data + 14);
        page->header.sequence_number = decode_32(page->bitstream.data + 18);
        page->header.crc32 = decode_32(page->bitstream.data + 22);

        page->header.num_segments = page->bitstream.data[26];

        print_header(&page->header);

        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool decode_segment_table(Page *page) {

    if ((!ov_ptr_valid(page, "Cannot decode segment table - no page")) ||
        (!ov_cond_valid(OFFSET_NUM_SEGMENTS < page->bitstream.len_octets,
                        "Cannot decode segment table - not enough data")) ||
        (!ov_cond_valid(OFFSET_SEGMENT_TABLE +
                                page->bitstream.data[OFFSET_NUM_SEGMENTS] <=
                            page->bitstream.len_octets,
                        "Cannot decode segment table - no data"))) {

        return false;

    } else {

        memcpy(&page->header.segment_table,
               page->bitstream.data + OFFSET_SEGMENT_TABLE,
               page->bitstream.data[OFFSET_NUM_SEGMENTS]);

        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool update_header(ov_format *format, Page *page) {

    return get_octets(format, page, 27) && decode_base_header(page) &&
           get_octets(format, page,
                      page->header.num_segments * sizeof(uint8_t)) &&
           decode_segment_table(page);

    // read_result result =
    //     get_octets(format, page, 27) && decode_base_header(page);

    // if (READ_OK != result) {
    //     return result;
    // } else {
    //     result = get_octets(
    //         format, page, page->header.num_segments * sizeof(uint8_t));

    //    if (READ_OK != result) {
    //        return result;
    //    } else if (decode_segment_table(page)) {
    //        return READ_OK;
    //    } else {
    //        return READ_ERROR;
    //    }
    //}
}

/*----------------------------------------------------------------------------*/

static ssize_t calculate_total_segment_octets_in_page(Page const *page) {

    if (!ov_ptr_valid(page, "Cannot read next page: Invalid page object")) {

        return -1;

    } else {

        size_t octets = 0;

        for (size_t i = 0; i < page->header.num_segments; ++i) {
            octets += page->header.segment_table[i];
        }

        return octets;
    }
}

/*----------------------------------------------------------------------------*/

static bool update_segments(ov_format *format, Page *page) {

    ssize_t num_segment_octets = calculate_total_segment_octets_in_page(page);

    return (0 <= num_segment_octets) &&
           get_octets(format, page, (size_t)num_segment_octets);
}

/*----------------------------------------------------------------------------*/

static bool check_check_sum(Page *page) {

    if ((!ov_ptr_valid(page, "Cannot verify checksum - invalid page")) ||
        !ov_cond_valid(OFFSET_CRC32 + 3 <= page->bitstream.len_octets,
                       "Cannot verify checksum - not enough data")) {

        return false;

    } else {

        // Erase checksum field
        memset(page->bitstream.data + OFFSET_CRC32, 0, 4);

        if (page->header.crc32 !=
            ov_crc32_ogg(0, page->bitstream.data, page->bitstream.len_octets)) {
            fprintf(stderr,
                    "CRC32 check failed: In Header: %" PRIu32
                    ", calculated: %" PRIu32 "\n",
                    page->header.crc32,
                    ov_crc32_ogg(0, page->bitstream.data,
                                 page->bitstream.len_octets));
        }

        return page->header.crc32 == ov_crc32_ogg(0, page->bitstream.data,
                                                  page->bitstream.len_octets);
    }
}

/*----------------------------------------------------------------------------*/

static bool reset_bitstream(Page *page) {

    if (ov_ptr_valid(page, "Cannot reset bitstream: Invalid page")) {
        page->bitstream.len_octets = 0;
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool next_page(ov_format *format, OggIn *self) {

    if (!ov_ptr_valid(self, "Cannot read page: Invalid OggIn object")) {

        return false;

    } else if ((!reset_bitstream(&self->page)) ||
               (!update_header(format, &self->page)) ||
               (!update_segments(format, &self->page)) ||
               (!check_check_sum(&self->page))) {

        return false;

    } else {

        self->current_segment = 0;
        self->current_segment_offset =
            OFFSET_SEGMENT_TABLE + self->page.header.num_segments;

        return true;
    }
}

/*----------------------------------------------------------------------------*/

bool set_primary_stream_serial(OggIn *self, uint32_t new_stream_serial_number) {

    if (!ov_ptr_valid(self, "Cannot read page: Invalid OggIn object")) {

        return false;

    } else {

        self->primary_stream.serial_number = new_stream_serial_number;
        self->primary_stream.has_been_set = true;

        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool next_page_for_primary_stream(ov_format *format, OggIn *self) {

    if ((!ov_ptr_valid(self, "Cannot read page: Invalid OggIn object"))) {

        return false;

    } else {

        bool ok = next_page(format, self);

        while (ok && ((!self->page.header.flags.begin_of_stream) ||
                      (self->primary_stream.serial_number !=
                       self->page.header.stream_serial_number))) {

            ok = next_page(format, self);
        }

        return ok;
    }
}

/*----------------------------------------------------------------------------*/

static bool update_page(ov_format *format, OggIn *self) {

    if ((!ov_ptr_valid(self, "Cannot read page: Invalid OggIn object"))) {

        return false;

    } else if (!self->primary_stream.has_been_set) {

        self->primary_stream.start_found =
            next_page(format, self) &&
            set_primary_stream_serial(self,
                                      self->page.header.stream_serial_number);

        return self->primary_stream.start_found;

    } else if (!self->primary_stream.start_found) {

        self->primary_stream.start_found =
            next_page_for_primary_stream(format, self);

        return self->primary_stream.start_found;

    } else if (self->page.header.num_segments > self->current_segment) {

        return true;

    } else if (self->page.header.flags.end_of_stream) {

        self->primary_stream.start_found = false;

        ov_log_error("End of stream reached");
        return false;

    } else {

        return next_page(format, self);
    }
}

/*----------------------------------------------------------------------------*/

typedef struct {
    bool chunk_ended;
    ov_buffer *chunk;
} gather_chunk_result;

static gather_chunk_result gather_chunk(OggIn *self) {

    gather_chunk_result result = {0};

    size_t end_segment = self->current_segment;
    size_t total_size_octets = 0;

    for (; (end_segment < self->page.header.num_segments) &&
           (!result.chunk_ended);
         ++end_segment) {

        uint8_t segsize = self->page.header.segment_table[end_segment];
        total_size_octets += segsize;

        result.chunk_ended = (255 > segsize);
    }

    size_t offset_segment = self->current_segment_offset;

    if (offset_segment + total_size_octets > self->page.bitstream.len_octets) {

        ov_log_error("OGG corrupted: segment table lists more bytes than "
                     " available in byte stream: %zu vs %zu",
                     offset_segment + total_size_octets,
                     self->page.bitstream.len_octets);

        return result;

    } else {

        result.chunk = ov_buffer_create(total_size_octets);
        memcpy(result.chunk->start, self->page.bitstream.data + offset_segment,
               total_size_octets);

        result.chunk->length = total_size_octets;

        self->current_segment = end_segment;
        self->current_segment_offset += total_size_octets;

        return result;
    }
}

/*----------------------------------------------------------------------------*/

ov_buffer *read_chunk(ov_format *format, OggIn *self) {

    if (ov_ptr_valid(self, "Cannot read chunk: Invalid Ogg format") &&
        update_page(format, self)) {

        gather_chunk_result result = gather_chunk(self);

        if (!result.chunk_ended) {

            ov_buffer *tail = read_chunk(format, self);
            result.chunk = ov_buffer_concat(result.chunk, tail);
            tail = ov_buffer_free(tail);
        }

        return result.chunk;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ov_buffer impl_next_chunk(ov_format *f, size_t requested_bytes,
                                 void *data) {

    UNUSED(requested_bytes);

    OggIn *oggin = as_ogg_in(data);

    ov_buffer *chunk = read_chunk(f, oggin);

    ov_buffer result = {0};

    if (0 != chunk) {

        result.start = chunk->start;
        result.length = chunk->length;
        // Need to set this, otherwise the data pointer is not freed, see
        // documentation of ov_format.next_chunk
        result.capacity = result.length;

        chunk->start = 0;
        chunk->capacity = 0;

        chunk = ov_buffer_free(chunk);
    }

    return result;
}

/*****************************************************************************
                                 ov_format_ogg
 ****************************************************************************/

static void *impl_create_data(ov_format *f, void *options) {

    ov_format_ogg_options default_options = {

        .chunk_length_ms = 20,
        .samplerate_hz = 48000,
        .stream_serial = 0x12000021,

    };

    options = OV_OR_DEFAULT(options, &default_options);

    switch (ov_format_get_mode(f)) {

    case OV_READ:

        return ogg_in_create(options);

    case OV_WRITE:

        return ogg_out_create(f, options);

    case OV_INVALID:

        return 0;

    default:
        OV_ASSERT(!"MUST NEVER HAPPEN");
        return 0;
    };
}

/*----------------------------------------------------------------------------*/

static void *impl_free_data(void *data) {

    OggIn *in = as_ogg_in(data);
    OggOut *out = as_ogg_out(data);

    if (0 != in) {

        return ogg_in_free(in);

    } else if (0 != out) {

        return ogg_out_free(out);

    } else {

        return data;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_format_ogg_install(ov_format_registry *registry) {

    ov_format_handler handler = {
        .next_chunk = impl_next_chunk,
        .write_chunk = impl_write_chunk,
        .overwrite = 0,
        .ready_format = 0,
        .create_data = impl_create_data,
        .free_data = impl_free_data,
    };

    return ov_format_registry_register_type(OV_FORMAT_OGG_TYPE_STRING, handler,
                                            registry);
}

/*----------------------------------------------------------------------------*/

bool ov_format_ogg_new_page(ov_format *self, int64_t sample_position_old_page) {

    OggOut *out = as_ogg_out(ov_format_get_custom_data(self));

    if (ov_ptr_valid(out, "Cannot write OGG: Invalid OggOut object") &&
        page_set_sample_position(&out->page, sample_position_old_page) &&
        (-1 < serialize_page(out->lower_format, &out->page))) {

        out->parameters.sequence_number++;
        out->parameters.stream_start = false;

        return page_clear(&out->page, false, &out->parameters);

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_format_ogg_new_stream(ov_format *self, uint32_t stream_serial) {

    OggOut *out = as_ogg_out(ov_format_get_custom_data(self));

    if (ov_cond_valid(finish_stream(out),
                      "Cannot start new stream - invalid ogg format")) {

        out->parameters.stream_serial_number = stream_serial;

        return page_clear(&out->page, false, &out->parameters);

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_format_ogg_select_stream(ov_format *self, uint32_t stream_serial) {

    OggIn *in = as_ogg_in(ov_format_get_custom_data(self));

    if (ov_ptr_valid(in, "Cannot select stream: Invalid oggin object")) {

        in->primary_stream.serial_number = stream_serial;
        in->primary_stream.has_been_set = true;
        in->primary_stream.start_found = false;

        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

ov_buffer ov_format_ogg_read_page(ov_format *self) {

    OggIn *in = as_ogg_in(ov_format_get_custom_data(self));

    if (ov_ptr_valid(
            in, "Cannot return current page content - Invalid oggin object")) {

        return (ov_buffer){0};

    } else {

        TODO("PROPAGATE PAGE");

        size_t header_len = OFFSET_SEGMENT_TABLE + in->page.header.num_segments;

        ov_buffer page = {

            .start = in->page.bitstream.data + header_len,
            .length = in->page.bitstream.len_octets - header_len,
        };

        TODO("PROPAGATE PAGE");

        return page;
    }
}

/*----------------------------------------------------------------------------*/
