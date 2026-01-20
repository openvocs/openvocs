/***

  Copyright   2016    German Aerospace Center DLR e.V.,
  German Space Operations Center (GSOC)

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

This file is part of the openvocs project. http://openvocs.org

 ***/ /**

      @author              Michael Beer
      @date                2017-08-08

     **/

#include "../../include/ov_rtp_frame.h"
#include "../../include/ov_utils.h"
#include <arpa/inet.h>
#include <stdlib.h>

#include "../../include/ov_constants.h"
#include "../../include/ov_dump.h"
#include "../../include/ov_registered_cache.h"
#include "../../include/ov_utils.h"

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 201112L)

#error("Require C11 support")

#endif

#ifdef DEBUG

uint64_t num_frames_created = 0;
uint64_t num_frames_freed = 0;

#define STATISTICS_INCREASE_NUM_FRAMES_CREATED ++num_frames_created
#define STATISTICS_INCREASE_NUM_FRAMES_TERMINATED ++num_frames_freed

#else

#define STATISTICS_INCREASE_NUM_FRAMES_CREATED
#define STATISTICS_INCREASE_NUM_FRAMES_TERMINATED

#endif

/*----------------------------------------------------------------------------*/

ov_registered_cache *g_rtp_frame_cache = 0;

/*---------------------------------------------------------------------------*/

const size_t RTP_HEADER_MIN_LENGTH = 12;

/******************************************************************************
 *
 *  PRIVATE
 *
 ******************************************************************************/

/*
 * This is the actual structure of an ov_rtp_frame.
 * keeps additional internal data that is NOT intended for outsiders to be
 * fiddled with (directly)
 */
typedef struct {

    ov_rtp_frame frame;

    /* Used to keep the CSRC ids converted from big endian if necessary -
     * The pointer in frame.expanded.csrc_ids will possibly point to
     * internal_csrc_ids.data .
     * This is NOT of interest for the user of an rtp frame.
     *
     * If data has not been allocated, set data = 0,
     * if data != 0, it will be assumed to have been allocated already,
     * even if num_allocated_entries = 0.
     */
    struct {
        size_t num_allocated_entries;
        uint32_t *data;
    } internal_csrc_ids;

} internal_frame;

/*---------------------------------------------------------------------------*/

static void *frame_free(void *vframe) {

    ov_rtp_frame *frame = vframe;

    if (0 == frame) {
        goto error;
    }

    if (0 != frame->expanded.extension.allocated_bytes) {
        free(frame->expanded.extension.data);
    }

    if (0 != frame->bytes.data) {
        free(frame->bytes.data);
    }

    internal_frame *internal;
    internal = (internal_frame *)frame;

    if (0 != internal->internal_csrc_ids.data) {

        free(internal->internal_csrc_ids.data);
        internal->internal_csrc_ids.data = 0;
    }

    free(frame);
    frame = 0;

    return 0;

error:

    return frame;
}

/*----------------------------------------------------------------------------*/

static bool set_csrc_array_to_little_endian(ov_rtp_frame *frame,
                                            const uint32_t *csrcs,
                                            uint8_t num) {

    if (0 == num)
        return true;

    if (0 == frame)
        goto error;
    if (0 == csrcs)
        goto error;

    internal_frame *internal = (internal_frame *)frame;

    /* Ensure we got enough target memory */
    uint32_t *csrcs_array = internal->internal_csrc_ids.data;

    OV_ASSERT(((0 != internal->internal_csrc_ids.num_allocated_entries) &&
               (0 != csrcs_array)) ||
              ((0 == internal->internal_csrc_ids.num_allocated_entries) &&
               (0 == csrcs_array)));

    if (0 == csrcs_array) {

        csrcs_array = calloc(1, num * sizeof(uint32_t));
        internal->internal_csrc_ids.num_allocated_entries = num;
    }

    OV_ASSERT(0 != csrcs_array);

    if (num > internal->internal_csrc_ids.num_allocated_entries) {

        csrcs_array = realloc(csrcs_array, num * sizeof(uint32_t));
        internal->internal_csrc_ids.num_allocated_entries = num;
    }

    internal->internal_csrc_ids.data = csrcs_array;

    OV_ASSERT(internal->internal_csrc_ids.num_allocated_entries >= num);

    /* Now the easy part - copy & convert all CSRCs to little endian */

    for (uint8_t i = 0; i < num; ++i) {
        csrcs_array[i] = ntohl(csrcs[i]);
    }

    frame->expanded.csrc_ids = csrcs_array;

    return true;

error:

    return false;
}

/*---------------------------------------------------------------------------*/

static ov_rtp_frame *impl_rtp_frame_copy(ov_rtp_frame const *self) {

    /* Slow, naive implementation - but will always be accurate, simple to
     * understand and I dont think this method will be used very often
     * and/or
     * is time critical.
     * If it turns out it is, we can optimize still.
     */

    if ((0 == self) || (0 == self->bytes.length)) {
        goto error;
    }

    return ov_rtp_frame_decode(self->bytes.data, self->bytes.length);

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

static ov_rtp_frame *impl_rtp_frame_free(ov_rtp_frame *self) {

    if (0 == self) {

        return 0;
    }

    if (0 == ov_registered_cache_put(g_rtp_frame_cache, self)) {

        return 0;
    }

    return frame_free(self);
}

/*----------------------------------------------------------------------------*/

static ov_rtp_frame *rtp_frame_create() {

    /* Lets allocate enough memory
     * for the internal data structures as well ...*/
    ov_rtp_frame *frame = calloc(1, sizeof(internal_frame));

    STATISTICS_INCREASE_NUM_FRAMES_CREATED;

    ov_rtp_frame_expansion *frame_data = (ov_rtp_frame_expansion *)frame;

    frame_data->version = RTP_VERSION_2;

    frame_data->padding_bit = false;
    frame_data->extension_bit = false;
    frame_data->marker_bit = false;

    frame->free = impl_rtp_frame_free;
    frame->copy = impl_rtp_frame_copy;

    /* Remainder shall be 0 */

    return frame;
}

/*----------------------------------------------------------------------------*/

static ov_rtp_frame *rtp_frame_get_new(size_t min_bytes_length) {

    ov_rtp_frame *frame = ov_registered_cache_get(g_rtp_frame_cache);

    if (0 == frame) {
        frame = rtp_frame_create();
    }

    OV_ASSERT(0 != frame);

    /* If the frame came from the cache, there might be some leftovers to clear
     */

    if (0 < frame->expanded.extension.allocated_bytes) {
        free(frame->expanded.extension.data);
        frame->expanded.extension.data = 0;
        frame->expanded.extension.allocated_bytes = 0;
    }

    if ((0 != frame->bytes.data) &&
        (min_bytes_length > frame->bytes.allocated_bytes)) {

        free(frame->bytes.data);
        frame->bytes.data = 0;
    }

    if (0 == frame->bytes.data) {

        frame->bytes.data = malloc(min_bytes_length * sizeof(uint8_t));
        frame->bytes.allocated_bytes = min_bytes_length;
    }

    return frame;
}

/*---------------------------------------------------------------------------*/

#if OV_BYTE_ORDER == OV_LITTLE_ENDIAN

#define SET_CSRC_ARRAY(frame, csrcs, num)                                      \
    set_csrc_array_to_little_endian(frame, csrcs, num)

#else

#define SET_CSRC_ARRAY(frame, csrcs, num)                                      \
    {                                                                          \
        frame->expanded.csrc_ids = csrcs;                                      \
    }

#endif

/******************************************************************************
 *                                PUBLIC STUFF
 ******************************************************************************/

void ov_rtp_frame_enable_caching(size_t capacity) {

    ov_registered_cache_config cfg = {

        .capacity = capacity,
        .item_free = frame_free,

    };

    g_rtp_frame_cache = ov_registered_cache_extend("rtp_frame", cfg);

    OV_ASSERT(0 != g_rtp_frame_cache);
}

/*----------------------------------------------------------------------------*/

ov_rtp_frame *ov_rtp_frame_encode(const ov_rtp_frame_expansion *rtp_data) {

    ov_rtp_frame *frame = 0;

    if (0 == rtp_data) {
        goto error;
    }

    /* Check which data is actually present */
    const size_t payload_length = rtp_data->payload.length;
    const size_t padding_length = rtp_data->padding.length;
    const size_t extension_len = rtp_data->extension.length;
    const uint8_t csrc_count = rtp_data->csrc_count;

    /* Calculate bytes required to encode frame */

    if (csrc_count > 15)
        goto error;

    volatile size_t length = RTP_HEADER_MIN_LENGTH;
    length += csrc_count * sizeof(uint32_t);

    if (rtp_data->extension_bit) {
        length += sizeof(uint32_t) + extension_len;
    }

    /* Length of extension is stored in multiples of 4 octets.
     * Check whether header lengths is divisible by 4 */
    uint16_t extension_len_4octets = extension_len / 4;

    if (4 * extension_len_4octets != extension_len) {
        goto error;
    }

    if (0 < payload_length) {
        length += payload_length;
    }

    if (0xff < padding_length) {
        goto error;
    }

    length += padding_length;

    if (rtp_data->padding_bit) {
        length += 1;
    }

    frame = rtp_frame_get_new(length);

    OV_ASSERT(0 != frame);
    OV_ASSERT(0 != frame->bytes.data);
    OV_ASSERT(length <= frame->bytes.allocated_bytes);
    OV_ASSERT(0 == frame->expanded.extension.allocated_bytes);

    ov_rtp_frame_expansion *exp = &frame->expanded;

    OV_ASSERT(0 == exp->extension.allocated_bytes);

    /* Copy expanded data */
    frame->expanded = *rtp_data;
    /* Clear header extension stuff, if used we will set it down below again
     */
    exp->extension.allocated_bytes = 0;
    exp->extension.length = 0;
    exp->extension.data = 0;

    frame->bytes.length = length;

    uint8_t *bytes = frame->bytes.data;
    memset(bytes, 0, length);

    /* ENCODE */

    uint8_t u8 = 0;
    uint16_t u16 = 0;
    uint32_t u32 = 0;

    /* SET BYTE 0 */
    u8 = rtp_data->version << 6;
    u8 |= (rtp_data->csrc_count & 0x0F);

    if (rtp_data->padding_bit)
        u8 |= 0x20;

    if (rtp_data->extension_bit)
        u8 |= 0x10;

    bytes[0] = u8;

    u8 = 0;

    /* SET BYTE 1 */
    if (rtp_data->marker_bit)
        u8 = 0x80;

    u8 |= rtp_data->payload_type & 0x7F;

    bytes[1] = u8;

    u8 = 0;
    uint8_t *next = bytes + 2;

    /* SET SEQUENCE NUMBER */
    u16 = htons(rtp_data->sequence_number);
    memcpy(next, &u16, 2);

    next += 2;

    /* SET TIMESTAMP */
    u32 = htonl(rtp_data->timestamp);
    memcpy(next, &u32, 4);
    next += 4;

    /* SET SSRC */
    u32 = htonl(rtp_data->ssrc);
    memcpy(next, &u32, 4);
    next += 4;

    /* SET CSRCs */
    uint32_t *csrc = (uint32_t *)next;

    for (size_t i = 0; i < csrc_count; ++i, ++csrc) {
        *csrc = htonl(rtp_data->csrc_ids[i]);
    }

    next += csrc_count * sizeof(uint32_t);

    /* ADD EXTENSIONS */
    if (rtp_data->extension_bit) {

        *(uint16_t *)next = htons(rtp_data->extension.type);
        next += 2;
        *(uint16_t *)next = htons(extension_len_4octets);
        next += 2;

        uint8_t *ext = rtp_data->extension.data;

        if (0 != ext) {

            if (0 == extension_len)
                goto error;

            memcpy(next, ext, extension_len);

            exp->extension.data = next;
            exp->extension.length = extension_len;
            next += extension_len;
        }
    }

    /* ADD PAYLOAD */
    if (0 < payload_length) {

        if (0 == exp->payload.data)
            goto error;

        memcpy(next, exp->payload.data, payload_length);
        exp->payload.data = next;

        next += payload_length;
    }

    /* ADD PADDING */
    if (0 < padding_length) {

        if (!rtp_data->padding_bit)
            goto error;

        uint8_t *pad = rtp_data->padding.data;

        if (0 == pad)
            goto error;

        memcpy(next, pad, padding_length);
        exp->padding.data = next;

        next += padding_length;
    }

    if (rtp_data->padding_bit) {

        *next = padding_length + 1;
    }

    return frame;

error:

    if (0 != frame)
        frame->free(frame);

    return 0;
}

/*----------------------------------------------------------------------------*/

ov_rtp_frame *ov_rtp_frame_decode(const uint8_t *input, const size_t length) {

    ov_rtp_frame *frame = 0;

    if ((0 == input) || (0 == length)) {

        ov_log_error("no data to decode");
        goto error;
    }

    frame = rtp_frame_get_new(length);
    OV_ASSERT(0 != frame);
    OV_ASSERT(0 != frame->bytes.data);
    OV_ASSERT(length <= frame->bytes.allocated_bytes);

    memcpy(frame->bytes.data, input, length);
    frame->bytes.length = length;

    uint8_t *bytes = frame->bytes.data;

    ov_rtp_frame_expansion *exp = &frame->expanded;
    uint8_t *extension_data = exp->extension.data;
    size_t extension_allocated_bytes = exp->extension.allocated_bytes;
    memset(exp, 0, sizeof(*exp));

    /* Retain extension data buffer if allocated.
     * Content must be cleared.
     * This is done by setting the extension data length to 0 */
    exp->extension.data = extension_data;
    extension_data = 0;
    exp->extension.allocated_bytes = extension_allocated_bytes;
    extension_allocated_bytes = 0;

    /* Decode 1st byte */
    volatile uint8_t byte;
    volatile size_t expected_length = RTP_HEADER_MIN_LENGTH;

    byte = bytes[0];

    exp->version = (byte >> 6) & 0x03;
    exp->padding_bit = byte & 0x20;
    exp->extension_bit = byte & 0x10;

    if (exp->extension_bit) {
        expected_length += 4;
    }

    byte &= 0x0f;
    if (0x0e < byte)
        goto error;

    expected_length += byte * sizeof(uint32_t);
    if (length < expected_length)
        goto error;

    exp->csrc_count = byte;

    /* Decode 1st byte */
    byte = bytes[1];
    exp->marker_bit = byte & 0x80;
    exp->payload_type = byte & 0x7f;

    uint8_t *current = bytes + 2;

    /* Decode sequence number */
    exp->sequence_number = ntohs(*(uint16_t *)current);
    current += 2;

    /* Decode timestamp */
    exp->timestamp = ntohl(*(uint32_t *)current);
    current += 4;

    /* decode src id */
    exp->ssrc = ntohl(*(uint32_t *)current);
    current += 4;

    OV_ASSERT(current == bytes + RTP_HEADER_MIN_LENGTH);

    /* And extract / index csrc array */
    byte = exp->csrc_count;
    SET_CSRC_ARRAY(frame, (uint32_t *)current, byte);
    current += byte * sizeof(uint32_t);

    if (exp->extension_bit) {

        exp->extension.type = ntohs(*(uint16_t *)current);
        current += 2;
        exp->extension.length = ntohs(*(uint16_t *)current) * sizeof(uint32_t);
        current += 2;
    }

    expected_length += exp->extension.length;
    if (length < expected_length)
        goto error;

    if (0 < exp->extension.length) {

        exp->extension.data = current;
    }

    current += exp->extension.length;

    /* Add payload */

    if (current < bytes + length) {

        /* We got payload */
        exp->payload.length = bytes + length - current;
        exp->payload.data = current;
    }

    if (exp->padding_bit) {

        byte = bytes[length - 1];
        if (0 == byte)
            goto error;
        if (byte > exp->payload.length)
            goto error;

        exp->padding.length = byte - 1;
        exp->payload.length -= byte;
        exp->padding.data = &bytes[length - byte];
    }

    return frame;

error:

    if (0 != frame) {
        frame = frame->free(frame);
    }

    OV_ASSERT(0 == frame);

    return 0;
}

/*----------------------------------------------------------------------------*/

ov_rtp_frame *ov_rtp_frame_free(ov_rtp_frame *frame) {

    if (0 == frame) {
        goto error;
    }

    OV_ASSERT(0 != frame);
    OV_ASSERT(0 != frame->free);

    return frame->free(frame);

error:

    return frame;
}

/*---------------------------------------------------------------------------*/

bool ov_rtp_frame_dump(const ov_rtp_frame_expansion *frame, FILE *stream) {

#define BOOL_TO_STRING(x) ((x) ? "true" : "false")

    if (!frame) {
        ov_log_error("No frame given");
        goto error;
    }

    if (!stream) {
        ov_log_error("No stream given");
        goto error;
    }

    fprintf(stream, "\nRTP Frame dump\n\n");
    fprintf(stream, "RTP Version         %5u\n", frame->version),
        fprintf(stream, "RTP Padding bit     %5s\n",
                BOOL_TO_STRING(frame->padding_bit));
    fprintf(stream, "RTP Extension bit   %5s\n",
            BOOL_TO_STRING(frame->extension_bit));
    fprintf(stream, "RTP Marker bit      %5s\n",
            BOOL_TO_STRING(frame->marker_bit));
    fprintf(stream, "RTP Payload type    %5" PRIu8 "\n", frame->payload_type);
    fprintf(stream, "RTP Sequence number %5" PRIu16 "\n",
            frame->sequence_number);
    fprintf(stream, "RTP Timestamp (SR Units) %22" PRIu32 "\n",
            frame->timestamp);
    fprintf(stream, "RTP SSRC ID              %22" PRIu32 "\n", frame->ssrc);
    fprintf(stream, "RTP CSRC IDs             %22" PRIu8 "\n",
            frame->csrc_count);

    for (size_t i = 0; i < frame->csrc_count; i++) {
        fprintf(stream, "     CSRC ID             %22" PRIu32 "\n",
                frame->csrc_ids[i]);
    }

    fprintf(stream, "\n");

    if (0 == frame->payload.length) {

        fprintf(stream, "RTP Payload   NONE\n");

    } else {

        fprintf(stream, "RTP Payload Length       %6zd\n",
                frame->payload.length);

        ov_dump_binary_as_hex(stream, frame->payload.data,
                              frame->payload.length);

        fprintf(stream, "\n\n");
    }

    if (!frame->padding_bit) {

        fprintf(stream, "RTP Padding   NONE\n");

    } else {

        fprintf(stream, "RTP Padding Length       %6zd\n",
                frame->padding.length + 1);

        ov_dump_binary_as_hex(stream, frame->padding.data,
                              frame->padding.length);

        fprintf(stream, "\n\n");
    }

    if (!frame->extension_bit) {

        fprintf(stream, "RTP Extension NONE\n");

    } else {

        fprintf(stream, "RTP Extension type       %6" PRIu16 "\n",
                frame->extension.type);

        fprintf(stream, "RTP Extension Length     %6zd\n",
                frame->extension.length);

        ov_dump_binary_as_hex(stream, frame->extension.data,
                              frame->extension.length);
    }

    fprintf(stream, "\n");

    return true;

error:
    return false;

#undef BOOL_TO_STRING
}
