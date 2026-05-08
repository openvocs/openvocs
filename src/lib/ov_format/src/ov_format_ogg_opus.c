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

        ------------------------------------------------------------------------
*/

#include "../include/ov_format_ogg_opus.h"
#include "../include/ov_format_ogg.h"
#include <math.h>
#include <ov_arch/ov_byteorder.h>
#include <ov_base/ov_chunker.h>
#include <ov_base/ov_string.h>

/*----------------------------------------------------------------------------*/

char const *OV_FORMAT_OGG_OPUS_TYPE_STRING = "ogg/opus";
const uint16_t OV_OGG_OPUS_PRESKIP_SAMPLES = 3840;

/*----------------------------------------------------------------------------*/

static size_t encode32(uint32_t u32, uint8_t *out) {

    OV_ASSERT((0 != out) && (0 != out));

    u32 = OV_H32TOLE(u32);
    memcpy(out, &u32, sizeof(u32));

    return sizeof(u32);
}

/*----------------------------------------------------------------------------*/

static uint32_t decode32(uint8_t const **in) {

    if (ov_ptr_valid(in, "Cannot decode integer - no input") &&
        ov_ptr_valid(*in, "Cannot decode integer - no input")) {

        uint32_t val = 0;
        memcpy(&val, *in, 4);
        *in += 4;
        return OV_LE32TOH(val);

    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static size_t encode16(uint16_t u16, uint8_t *out) {

    OV_ASSERT((0 != out) && (0 != out));

    u16 = OV_H16TOLE(u16);
    memcpy(out, &u16, sizeof(u16));

    return sizeof(u16);
}

/*----------------------------------------------------------------------------*/

static uint16_t decode16(uint8_t const **in) {

    if (ov_ptr_valid(in, "Cannot decode integer - no input") &&
        ov_ptr_valid(*in, "Cannot decode integer - no input")) {

        uint32_t val = 0;
        memcpy(&val, *in, 2);
        *in += 2;
        return OV_LE16TOH(val);

    } else {
        return 0;
    }
}

/*****************************************************************************
                                    Strings
 ****************************************************************************/

static size_t encode_string(char const *str, uint8_t *write_ptr,
                            size_t write_capacity_octets) {

    size_t str_len = ov_string_len(str);

    if (ov_cond_valid(4 + str_len <= write_capacity_octets,
                      "Cannot write Comment Header - string too long") &&
        ov_ptr_valid(write_ptr,
                     "cannot write Comment Header - no memory (0 poiner)") &&
        ov_cond_valid(4 == encode32(str_len, write_ptr),
                      "Cannot write Comment Header - could not write string "
                      "length")) {

        memcpy(write_ptr + 4, str, str_len);

        return 4 + str_len;

    } else {

        return 0;
    }
}

/*****************************************************************************
                            Q7.8 Fixed point format
 ****************************************************************************/

static uint16_t to_q7_8(float f) {

    TODO("IMPLEMENT");

    const uint16_t UNITY_GAIN = 0x0000;

    // Formally, conversion has to be done like:

    // Separate f into N . FRACTION

    // Turn fraction into binary fraction of 8 bits length
    // Ensure N < 128

    // Move both into single 16 bit value:

    // 0 [7 Bits of N][8 Bits of FRACTION]

    // Invert -> 1 [Inverted 7 bits of N][inverted 8 bits of FRACTION]

    // Tread as 16bit unsigned int and add 1:

    // uint16: 1 [Inverted 7 bits N][inverted 8 bits of FRACTION] + 1
    //
    // Return

    double whole_number_double = 0;
    modf(f, &whole_number_double);

    if (ov_cond_valid(whole_number_double != whole_number_double,
                      "OGG OPUS gain is NaN") &&
        ov_cond_valid(whole_number_double < 128.0,
                      "OGG OPUS gain too high, must not exceed 128")) {
        return UNITY_GAIN;
    } else {
        return UNITY_GAIN;
    }
}

/*----------------------------------------------------------------------------*/

static float from_q_7_8(uint16_t q7_8_val) {

    TODO("IMPLEMENT");
    UNUSED(q7_8_val);
    return 0.0;
}

/*****************************************************************************
                                   ID Header
 ****************************************************************************/

typedef struct {

    ov_format_ogg_opus_options options;
    uint8_t num_channels;

} IdHeader;

const size_t ID_HEADER_SIZE = 19;
const char *ID_OGG_OPUS = "OpusHead";

uint8_t *write_id_header_to(IdHeader header, uint8_t ogg_opus_version,
                            uint8_t *write_ptr, size_t write_capacity_octets) {

    if (ov_cond_valid(write_capacity_octets >= ID_HEADER_SIZE,
                      "Cannot write ID header - insufficient memory "
                      "available")) {

        memcpy(write_ptr, ID_OGG_OPUS, strlen(ID_OGG_OPUS));
        write_ptr += 8;

        *write_ptr = ogg_opus_version;
        ++write_ptr;
        *write_ptr = header.num_channels;
        ++write_ptr;

        write_ptr += encode16(header.options.preskip_samples, write_ptr);
        write_ptr += encode32(header.options.samplerate_hz, write_ptr);
        write_ptr +=
            encode16(to_q7_8(header.options.output_gain_db), write_ptr);

        // We only use channel mapping family 0:
        // - 1 stream mono, None streams stereo
        // Channel count & coupled channel counts not encoded explicitly
        *write_ptr = 0;
        ++write_ptr;

        return write_ptr;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

size_t read_id_header_from(uint8_t const *read_ptr, size_t available_octets,
                           IdHeader *header) {

    if (ov_cond_valid(ID_HEADER_SIZE <= available_octets,
                      "Cannot read ID header - header incomplete") &&
        ov_ptr_valid(read_ptr,
                     "Cannot read ID Header - no input (0 pointer)") &&
        ov_ptr_valid(header,
                     "Cannot read ID Header - no id header object to store "
                     "data") &&
        ov_cond_valid(0 == memcmp(read_ptr, ID_OGG_OPUS, strlen(ID_OGG_OPUS)),
                      "Cannot read ID Header - Invalid magic bytes") &&
        ov_cond_valid(1 == read_ptr[8],
                      "Cannot read ID Header - unsupported format version") &&
        ov_cond_valid(1 == read_ptr[9],
                      "Cannot read ID Header - unsupported number of channels "
                      "- only single channel supported") &&
        ov_cond_valid(0 == read_ptr[18],
                      "Cannot read ID Header - unsupported channel mapping")) {

        read_ptr += 10;

        header->options.preskip_samples = decode16(&read_ptr);
        header->options.samplerate_hz = decode32(&read_ptr);
        header->options.output_gain_db = from_q_7_8(decode16(&read_ptr));
        header->num_channels = 1;

        return ID_HEADER_SIZE;

    } else {

        return 0;
    }
}

/*****************************************************************************
                                    COMMENTS
 ****************************************************************************/

typedef struct {

    char *vendor;

    size_t num_comments;
    char **comments;

    size_t comments_capacity;

    // The comment block is built like
    // |--- String Len (octets ---|KEY_____=VALUE____
    // |--- String Len ---|KEY____=VALUE___
    // ...
    //
    // The strings will be stored into 'comments' array as a whole, ie.
    // without separating key from value.
    //
    // When searching for a particular key, we need to compare the keys,
    // and the matiching key will be compared until its end anyways,
    // thence after comparison, we will have the position of '=',
    // and need only skip 2 chars to get to the start of the value.
    // Since it terminates in a 0, this pointer is all we have to return.
    //
    // The overhead is 5 octets per comment.
    // No use copying the key-values over into other structures.
    //
    // Of course, access would be faster if we would store the keys into a
    // hashtable.
    // If this turns out to be necessary, we can do that afterwards as well.

} CommentHeader;

void comment_header_clear(CommentHeader *header) {

    if (0 != header) {

        header->vendor = ov_free(header->vendor);

        for (size_t i = 0; i < header->num_comments; ++i) {
            header->comments[i] = ov_free(header->comments[i]);
        }

        header->comments = ov_free(header->comments);
        memset(header, 0, sizeof(CommentHeader));
    }
}

/*----------------------------------------------------------------------------*/

const size_t MIN_COMMENT_HEADER_SIZE = 8;

char const *COMMENT_HEADER_ID = "OpusTags";
const size_t COMMENT_HEADER_ID_LEN = 8;

/**
 * Writes comment header start, i.e. magic bytes and vendor info.
 * vendor must be ASCII-encoded!
 */
size_t write_start_comment_header_to(char const *vendor, uint8_t *write_ptr,
                                     size_t write_capacity_octets) {

    size_t vendor_len = ov_string_len(vendor);

    if (ov_ptr_valid(vendor, "Cannot write Comment Header - no vendor name") &&
        ov_ptr_valid(write_ptr, "Cannot write Comment Header - no memory") &&
        ov_cond_valid(COMMENT_HEADER_ID_LEN + 4 + vendor_len <=
                          write_capacity_octets,
                      "")) {

        memcpy(write_ptr, COMMENT_HEADER_ID, COMMENT_HEADER_ID_LEN);
        return 8 +
               encode_string(vendor, write_ptr + 8, write_capacity_octets - 8);

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

uint8_t *write_comments_to(uint8_t *write_ptr, size_t write_capacity_octets,
                           char const *const *const comments,
                           size_t num_comments) {

    if (ov_ptr_valid(write_ptr, "Cannot write Comment Header - no memory") &&
        ov_cond_valid(4 <= write_capacity_octets, "")) {

        size_t written_octets = encode32((uint32_t)num_comments, write_ptr);

        write_ptr += written_octets;
        write_capacity_octets -= written_octets;

        for (size_t n = 0; n < num_comments; ++n) {

            size_t written_octets =
                encode_string(comments[n], write_ptr, write_capacity_octets);

            if (4 > written_octets) {

                return 0;

            } else {

                write_ptr += written_octets;
                write_capacity_octets -= written_octets;
            }
        }

        return write_ptr;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static size_t comments_length(const CommentHeader header) {

    size_t len = 0;

    for (size_t i = 0; i < header.num_comments; ++i) {
        len += 4 + ov_string_len(header.comments[i]);
    }

    return len;
}

/*----------------------------------------------------------------------------*/

static size_t comment_header_calc_required_octets(const CommentHeader header) {

    return 8 + 4 + ov_string_len(header.vendor) + 4 + comments_length(header);
}

/*----------------------------------------------------------------------------*/

static uint8_t *write_comment_header_to(const CommentHeader header,
                                        uint8_t *write_ptr,
                                        size_t write_capacity_octets) {

    size_t written = write_start_comment_header_to(header.vendor, write_ptr,
                                                   write_capacity_octets);

    if (0 < written) {

        return write_comments_to(
            write_ptr + written, write_capacity_octets - written,
            (char const *const *)header.comments, header.num_comments);
    }

    return 0;
}

/*----------------------------------------------------------------------------*/

static char *make_comment_string(char const *key, char const *value) {

    if (ov_ptr_valid(key, "Cannot add comment - no key") &&
        ov_ptr_valid(value, "Cannot create comment - no value")) {

        size_t keylen = strlen(key);
        size_t vallen = strlen(value);

        //                       KEY      =  VALUE   \0
        size_t required_octets = keylen + 1 + vallen + 1;
        char *comment = malloc(required_octets);

        char *orig = comment;

        memcpy(comment, key, keylen);
        comment += keylen;
        *comment = '=';
        ++comment;

        memcpy(comment, value, vallen);

        comment += vallen;
        *comment = 0;

        return orig;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static void expand_comment_array_if_required(CommentHeader *header) {

    char **old = 0;

    if (ov_ptr_valid(header, "Cannot add comment - no comment header") &&
        header->comments_capacity <= header->num_comments) {

        old = header->comments;

        header->comments_capacity += 10;

        header->comments = calloc(header->comments_capacity, sizeof(char *));
    }

    if (0 != old) {
        memcpy(header->comments, old, header->num_comments * sizeof(char *));
        old = ov_free(old);
    }
}

/*----------------------------------------------------------------------------*/

bool comment_add(CommentHeader *header, char const *key, char const *value) {

    char *comment = make_comment_string(key, value);

    if (ov_ptr_valid(header, "Cannot add comment - no header") &&
        (0 != comment)) {

        expand_comment_array_if_required(header);
        header->comments[header->num_comments++] = comment;
        return true;

    } else {

        comment = ov_free(comment);
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static char const *match_comment(char const *comment, char const *key) {

    if (ov_ptr_valid(comment, "Cannot get comment: No comment") &&
        ov_ptr_valid(key, "Cannot get comment - no key")) {

        for (; 0 != *key; ++key, ++comment) {
            if ((0 == *comment) || (*key != *comment)) {
                return 0;
            }
        }

        // We matched the entire string and comment now points to
        //
        // KEY___=VALUE0
        //       ^

        if (ov_cond_valid('=' == *comment, "Cannot get comment: Not found")) {

            return comment + 1;

        } else {

            return 0;
        }

    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

char const *comment_get(CommentHeader const *header, char const *key) {

    char const *value = 0;

    if (ov_ptr_valid(header, "Cannot get comment: No comment header")) {

        for (size_t i = 0; (i < header->num_comments) && (0 == value); ++i) {
            value = match_comment(header->comments[i], key);
        }
    }

    return value;
}

/*****************************************************************************
                                 Actual Format
 ****************************************************************************/

static ov_format_ogg_opus_options default_options = {

    .output_gain_db = 0.0,
    .preskip_samples = 3840, /* Recommendation, see rfc7845, section 5.1 */
    .samplerate_hz = 48000,
};

typedef struct {

    uint32_t magic_bytes;

    struct {

        bool id_serialized : 1;
        bool comments_serialized : 1;

        bool id_deserialized : 1;
    };

    IdHeader id;
    CommentHeader comments;

    ov_format *ogg_format;

} OggOpus;

#define MAGIC_BYTES 0xacacacac

/*----------------------------------------------------------------------------*/

static OggOpus const *as_ogg_opus(void const *vptr) {

    OggOpus const *opus = vptr;

    if (ov_ptr_valid(opus, "No Ogg Opus object (0 pointer)") &&
        ov_cond_valid(opus->magic_bytes,
                      "No Ogg Opus object (Magic bytes mismatch)")) {

        return opus;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static OggOpus *as_ogg_opus_mut(void *vptr) {

    return (OggOpus *)as_ogg_opus(vptr);
}

/*----------------------------------------------------------------------------*/

static IdHeader get_id_header(OggOpus *self) {

    if (ov_ptr_valid(self, "Cannot get ID header: Invalid Ogg Opus object")) {
        return self->id;
    } else {
        return (IdHeader){0};
    }
}

/*----------------------------------------------------------------------------*/

static bool serialize_id_header(ov_format *f, OggOpus *self) {

    ov_buffer *buffer = ov_buffer_create(24);

    uint8_t *ptr = write_id_header_to(get_id_header(self), 1, buffer->start,
                                      buffer->capacity);

    bool retval = false;

    if (ov_ptr_valid(self, "Cannot write ID header: No OggOpus object") &&
        ov_ptr_valid(ptr, "Could not write Ogg Opus ID header")) {

        if (!self->id_serialized) {

            buffer->length = ptr - buffer->start;

            ov_format_payload_write_chunk(f, buffer);
            ov_format_ogg_new_page(f, 0); // Force new Page

            self->id_serialized = true;
        }

        retval = true;
    }

    buffer = ov_buffer_free(buffer);
    return retval;
}

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_chunker *chunker;
    ov_format *format;

} OurChunker;

static ov_buffer *next_chunk(OurChunker chunker, size_t requested_octets) {

    ov_buffer *chunk = ov_chunker_next_chunk(chunker.chunker, requested_octets);

    bool data_available = true;

    while (data_available && (0 == chunk)) {

        ov_buffer *buf =
            ov_format_payload_read_chunk(chunker.format, requested_octets);

        data_available = ov_chunker_add(chunker.chunker, buf);
        buf = ov_buffer_free(buf);

        chunk = ov_chunker_next_chunk(chunker.chunker, requested_octets);
    }

    return chunk;
}

/*----------------------------------------------------------------------------*/

static bool deserialize_id_header(OurChunker chunker, OggOpus *self) {

    if (ov_ptr_valid(
            self, "Cannot deserialize id header - invalid Ogg Opus object")) {

        ov_buffer *chunk = next_chunk(chunker, ID_HEADER_SIZE);

        bool ok = (0 != chunk) &&
                  (chunk->length ==
                   read_id_header_from(chunk->start, chunk->length, &self->id));

        chunk = ov_buffer_free(chunk);

        self->id_deserialized = true;
        return ok;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool serialize_comment_header(ov_format *f, OggOpus *self) {

    if (ov_ptr_valid(self, "Cannot serialize comment header - Invalid Ogg Opus "
                           "object")) {

        if (!self->comments_serialized) {

            size_t required_octets =
                comment_header_calc_required_octets(self->comments);

            ov_buffer *buf = ov_buffer_create(required_octets);

            uint8_t *ptr = write_comment_header_to(self->comments, buf->start,
                                                   buf->capacity);

            buf->length = ptr - buf->start;

            ov_format_payload_write_chunk(f, buf);

            buf = ov_buffer_free(buf);

            ov_format_ogg_new_page(f, 0); // Force new Page
            self->comments_serialized = true;
            // and we are good to start writing opus audio ...
        }

        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static void set_bool_ptr(bool *bptr, bool val) {

    if (0 != bptr) {
        *bptr = val;
    }
}

/*----------------------------------------------------------------------------*/

static uint32_t u32_from_format(OurChunker chunker, bool *ok_ptr) {

    ov_buffer *chunk = next_chunk(chunker, 4);
    uint32_t val = 0;

    if (ov_cond_valid((0 != chunk) && (4 == chunk->length),
                      "Cannot decode unsigned 32bit int - not enough data")) {

        memcpy(&val, chunk->start, 4);
        set_bool_ptr(ok_ptr, true);

    } else {

        set_bool_ptr(ok_ptr, false);
    }

    chunk = ov_buffer_free(chunk);
    return OV_LE32TOH(val);
}

/*----------------------------------------------------------------------------*/

static char *buffer_start_as_cptr(ov_buffer *buf) {

    if (0 != buf) {

        char *str = calloc(1, buf->length + 1);
        memcpy(str, buf->start, buf->length);
        return str;

    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static char *string_from_format(OurChunker chunker) {

    bool ok = false;
    uint32_t octets = u32_from_format(chunker, &ok);

    if (ok) {
        ov_buffer *chunk = next_chunk(chunker, octets);

        char *str = buffer_start_as_cptr(chunk);
        chunk = ov_buffer_free(chunk);

        return str;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static bool comment_header_from_format(OurChunker chunker,
                                       CommentHeader *header) {

    ov_buffer *chunk = next_chunk(chunker, COMMENT_HEADER_ID_LEN);

    bool ok = ov_buffer_equals(chunk, COMMENT_HEADER_ID);

    chunk = ov_buffer_free(chunk);

    if (ov_cond_valid(ok, "Cannot deserialize comment header ID not found") &&
        ov_ptr_valid(header, "Cannot deserialize comment header - no target "
                             "CommentHeader object")) {

        char *vendor = string_from_format(chunker);
        if (0 != vendor) {
            header->vendor = ov_free(header->vendor);
            header->vendor = vendor;
            vendor = 0;
        }

        header->num_comments = u32_from_format(chunker, &ok);

        header->comments = calloc(1, header->num_comments * sizeof(char *));

        bool ok = true;

        for (size_t i = 0; ok && (i < header->num_comments); ++i) {
            header->comments[i] = string_from_format(chunker);
            ok = (0 != header->comments[i]);
        }

        return ok;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool deserialize_comment_header(OurChunker chunker, OggOpus *self) {

    if (!ov_ptr_valid(self,
                      "Cannot deserialize comment header - invalid Ogg Opus "
                      "object")) {

        return false;

    } else if (!ov_cond_valid(self->id_deserialized,
                              "Cannot deserialize comment header - ID header "
                              "not deserialized yet")) {

        return false;

    } else {

        return comment_header_from_format(chunker, &self->comments);
    }
}

/*----------------------------------------------------------------------------*/

static bool deserialize_headers(ov_format *f, OggOpus *self) {

    if (ov_ptr_valid(self,
                     "Cannot deserialize headers - Invalid Ogg Opus object")) {

        OurChunker chunker = {
            .chunker = ov_chunker_create(),
            .format = f,
        };

        bool ok = deserialize_id_header(chunker, self) &&
                  deserialize_comment_header(chunker, self);

        chunker.chunker = ov_chunker_free(chunker.chunker);

        return ok;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool serialize_headers(ov_format *f, OggOpus *self) {

    if (ov_ptr_valid(self,
                     "Cannot serialze headers - Invalid Ogg Opus object")) {

        return self->comments_serialized || (serialize_id_header(f, self) &&
                                             serialize_comment_header(f, self));

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static void *impl_free_data(void *data);

static void *impl_create_data(ov_format *f, void *options) {

    UNUSED(f);

    ov_format_ogg_opus_options *opus_opts =
        OV_OR_DEFAULT(options, &default_options);

    OggOpus *opus = calloc(1, sizeof(OggOpus));

    opus->magic_bytes = MAGIC_BYTES;
    opus->id.num_channels = 1;
    opus->id.options = *opus_opts;
    opus->id.num_channels = 1;

    opus->comments.vendor = strdup("Ivaldi");

    if (OV_WRITE == ov_format_get_mode(f)) {

        serialize_id_header(f, opus);

    } else if ((OV_READ == ov_format_get_mode(f)) &&
               (!deserialize_headers(f, opus))) {

        opus = impl_free_data(opus);
    }

    return opus;
}

/*----------------------------------------------------------------------------*/

static void *impl_free_data(void *data) {

    OggOpus *opus = as_ogg_opus_mut(data);

    if (0 != opus) {

        comment_header_clear(&opus->comments);
        return ov_free(opus);

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ov_buffer impl_next_chunk(ov_format *f, size_t requested_bytes,
                                 void *data) {

    UNUSED(data);
    return ov_format_payload_read_chunk_nocopy(f, requested_bytes);
}

/*----------------------------------------------------------------------------*/

static ssize_t impl_write_chunk(ov_format *f, ov_buffer const *chunk,
                                void *data) {

    if (serialize_headers(f, as_ogg_opus_mut(data))) {
        return ov_format_payload_write_chunk(f, chunk);
    } else {
        return -1;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_format_ogg_opus_install(ov_format_registry *registry) {

    ov_format_handler handler = {
        .next_chunk = impl_next_chunk,
        .write_chunk = impl_write_chunk,
        .overwrite = 0,
        .ready_format = 0,
        .create_data = impl_create_data,
        .free_data = impl_free_data,
    };

    return ov_format_registry_register_type(OV_FORMAT_OGG_OPUS_TYPE_STRING,
                                            handler, registry);
}

/*----------------------------------------------------------------------------*/

ov_format *ov_format_ogg_opus_create(ov_format *lower_layer,
                                     ov_format_ogg_opus_options *opts) {

    ov_format_ogg_install(0);
    ov_format_ogg_opus_install(0);

    ov_format_ogg_options oggopts = {

        .stream_serial = 0x205c8959,
        .chunk_length_ms = 20,
        .samplerate_hz = default_options.samplerate_hz,

    };

    if (0 != opts) {
        oggopts.samplerate_hz = opts->samplerate_hz;
    };

    ov_format *ogg =
        ov_format_as(lower_layer, OV_FORMAT_OGG_TYPE_STRING, &oggopts, 0);

    return ov_format_as(ogg, OV_FORMAT_OGG_OPUS_TYPE_STRING, opts, 0);
}

/*----------------------------------------------------------------------------*/

char const *ov_format_ogg_opus_comment(ov_format *self, char const *key) {

    OggOpus *opus = as_ogg_opus_mut(ov_format_get_custom_data(self));

    if (ov_ptr_valid(opus, "Cannot add comment: Not an Ogg Opus format")) {

        return comment_get(&opus->comments, key);

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_format_ogg_opus_comment_set(ov_format *self, char const *key,
                                    char const *value) {

    OggOpus *opus = as_ogg_opus_mut(ov_format_get_custom_data(self));

    if (ov_ptr_valid(opus, "Cannot add comment: Not an Ogg Opus format") &&
        ov_cond_valid(!opus->comments_serialized,
                      "Cannot add comment: Writing audio payload already "
                      "started")) {

        comment_add(&opus->comments, key, value);
        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/
