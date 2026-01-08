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

     \file               ov_rtp_frame.h
     @author         Michael Beer
     \date               2017-08-08

     \ingroup            ov_rtp

     \brief              Definition of RTP of RFC 3550

     You can safely cast an ov_rtp_frame to ov_rtp_frame_expansion pointers.

 **/

#ifndef ov_rtp_frame_h
#define ov_rtp_frame_h

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "ov_utils.h"

/*----------------------------------------------------------------------------*/

#ifdef DEBUG

extern uint64_t num_frames_created;
extern uint64_t num_frames_freed;

#endif

/*---------------------------------------------------------------------------*/

/**
 * Denotes 'MSID does not matter
 */
#define OV_RTP_MSID_ANY 0

extern const size_t RTP_HEADER_MIN_LENGTH;

#define RTP_MAX_NUMBER_OF_CSRC_IDS ((uint64_t)((1 << 4) - 1))
#define RTP_PADDING_MIN_LENGTH 1 // one octet for amount of octets
#define OV_RTP_PAYLOAD_TYPE_DEFAULT 0

enum ov_rtp_version_enum {
    RTP_VERSION_2 = 2,
    RTP_VERSION_1 = 1,
    RTP_VERSION_INVALID = 0
};

typedef enum ov_rtp_version_enum ov_rtp_version;
typedef struct ov_rtp_frame_expansion_struct ov_rtp_frame_expansion;
typedef struct ov_rtp_frame_struct ov_rtp_frame;
typedef uint8_t ov_rtp_payload_type;

/******************************************************************************
 *    RTP FRAME STRUCT
 ******************************************************************************/

/**
 * Represents an RTP frame in memory.
 *
 * This is a mere shell for its data - the pointers contained herein are not
 * managed in any way: No method will attempt to perform a deep copy of this
 * frame!
 *
 * This struct is intended as a simple shell wrapping references to the actual
 * data to easily transfer frame data to functions, e.g.:
 *
 * ov_rtp_frame_encode( (ov_rtp_frame_expansion) {
 *                       .payload_data    = &payload,
 *                       .payload_length  = 143,
 *                       .src_id          = src_id,
 *                       .timestamp       = timestamp,
 *                       .sequence_number = seq,
 * });
 *
 *
 */
struct ov_rtp_frame_expansion_struct {

    ov_rtp_version version;

    /**
     * It might seem as if padding/extension bits were redundand
     * since there is the padding length etc as well.
     * But: There might be ONE SINGLE padding byte which is then
     * determined to contain exactly 0x01.
     * This case is tricky and might be reflected by including
     * the length encoding last byte into the padding data array
     * (Which is nasty - if you want to pad with 2 zeroes, you dont
     * want to hand in an array containing {0, 0, 3}, and,
     * we would have to handle the case of {0, 0, 2} etc.).
     * When padding, you either want to
     * (1) increase the frame size to a certain size -> you dont care about
     *     the padding data.
     * (2) You want to store additional data in the padding area ->
     *     You dont want to fiddle with the last, length encoding byte
     *
     * Padding is tricky in any ways, we deal with it this way:
     * 1. If you want certain data being appended to the payload as padding,
     *    hand it over in padding.data / padding.length and set the
     *    padding bit. The actual padding will be 1 more byte (the
     *    length encoding byte).
     * 2. If you want pad EXACTLY one single byte, this byte will be the
     *    length encoding byte. No need to hand over additional padding
     * data.
     *    Just set the padding bit and set the padding length to zero.
     *
     * Again, padding is tricky. Sorry!
     */
    bool padding_bit;
    /**
     * For reasons similar to the ones stated for the padding bit,
     * this bit is necessary as well...
     * (hint: The extension might consist of the header only,
     *  and the 0 is a valid value for 'type'...)
     */
    bool extension_bit;

    bool marker_bit;

    uint8_t payload_type;

    uint16_t sequence_number;

    uint32_t timestamp;
    uint32_t ssrc;

    uint8_t csrc_count;
    uint32_t *csrc_ids;

    struct rtp_payload_t {
        size_t length;
        uint8_t *data;
    } payload;

    struct rtp_padding_t {
        size_t length;
        uint8_t *data;
    } padding;

    struct rtp_extension_t {
        uint16_t type;
        size_t length;
        size_t allocated_bytes; /* Max. capacity of data. If 0, data is
                                   not suitable to be written to. If
                                   required, data would have to be
                                   allocated.
                                   On the other hand, if != 0,
                                   data is assumed to must be freed
                                   eventually */
        uint8_t *data;
    } extension;
};

/*---------------------------------------------------------------------------*/

/**
 * Represents a parsed RTP frame.
 * This is basically an RTP frame, but with its byte representation
 * cached.
 * The actual frame struct is the field parsed. It keeps onpy pointers to
 * the bytes array.
 * DONT FREE ANYTHING IN PARSED!
 *
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    Byte 1         | Byte 2        | Byte 3        | Byte 4
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                           timestamp                           |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |           synchronization source (SSRC) identifier            |
 *   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *   |            contributing source (CSRC) identifiers             |
 *   |                             ....                              |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *   You can safely cast a pointer to ov_rtp_frame to a pointer to
 *   ov_rtp_frame_extension, i.e. you can safely pass a pointer to
 *   an ov_rtp_frame to a function expecting a ov_rtp_frame_extension.
 */
struct ov_rtp_frame_struct {

    ov_rtp_frame_expansion expanded;

    /**
     * Contains the byte encoding of the RTP data ready to be sent over the
     * net e.g.
     */
    struct {

        size_t allocated_bytes;
        size_t length;
        uint8_t *data;

    } bytes;

    ov_rtp_frame *(*copy)(ov_rtp_frame const *self);
    ov_rtp_frame *(*free)(ov_rtp_frame *self);
};

/******************************************************************************
 *                                  CACHING
 ******************************************************************************/

void ov_rtp_frame_enable_caching(size_t capacity);

/******************************************************************************
 *
 *
 *  Constructors
 *
 ******************************************************************************/

/**
 * This function has no intrinsic logic,
 * i.e. padding bit will be encoded independently of presence of padding data.
 * This has been an explicit design decision - dont fiddle with this function to
 * include logic that does not belong in here!
 */
ov_rtp_frame *ov_rtp_frame_encode(const ov_rtp_frame_expansion *input_frame);

/**
 * Creates a frame by parsing the bytes in raw.
 * This new frame wraps the bytes array, therefore if parsing has been
 * successful,
 */
ov_rtp_frame *ov_rtp_frame_decode(const uint8_t *bytes, const size_t length);

/*----------------------------------------------------------------------------*/

ov_rtp_frame *ov_rtp_frame_free(ov_rtp_frame *frame);

/******************************************************************************
 *
 *  Additional functions
 *
 ******************************************************************************/

bool ov_rtp_frame_dump(const ov_rtp_frame_expansion *frame, FILE *stream);

#endif /* ov_rtp_frame_h */
