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
        @file           ov_websocket_pointer.h
        @author         Markus Töpfer

        @date           2020-12-26

        This implementation will created semantial pointers to some plugged in
        buffer, without changing the source buffer. The source buffer is 100%
        independent of the semantical websocket pointer overhead, but the
        websocket pointer is dependent on the source buffer.

        When using this function it MUST be ensured the source buffer is valid,
        as long as the ov_websocket_frame is used.

        Therefore ov_websocket_frame_create will create a buffer on init
        and the content buffer is managed within ov_websocket_frame.

        The buffer MAY be unplugged at ANY time using frame->buffer = NULL.
        Whenever the buffer is unplugged, it MUST be freed using ov_buffer_free
        at some point, to avoid memleaks.

        Intended usage of ov_websocket_frame :

            char *next = NULL;
            ov_websocket_parser_state state = OV_WEBSOCKET_PARSER_ERROR;

            ov_websocket_frame *frame = ov_websocket_frame_create(config);

                ...

            ssize_t in = recv(socket,
                    frame->buffer->start,frame->buffer->length,0);

                ...

            state = ov_websocket_parse_frame(msg, &next);

            if (state == OV_HTTP_PARSER_SUCCESS){

                if (next == frame->buffer->start + frame->buffer->length)){

                    // exactely ONE frame received

                    process_frame(frame);

                } else {

                    // some trailing bytes contained in frame->buffer

                    ov_websocket_frame *next_frame = NULL;
                    ov_websocket_shift_trailing_bytes(frame, next, &next_frame);

                    // frame will now contain exactely ONE websocket frame
                    // next_frame will contain any "overread" data e.g. the
                    // next frame received from network IO buffer

                    process_frame(frame);

                }
            }

        NOTE    process_frame will receive some allocated pointer based
                memory area and MAY transfer the pointer to threads for
                further processing.

        NOTE    allocations MAY be minimized with ov_websocket_enable_caching
                to cache ov_websocket_frame structures AND contained buffers.

                ov_websocket_frame_free is a threadsafe implementation to
                release some structures and return them into the cache


        ------------------------------------------------------------------------
*/
#ifndef ov_websocket_pointer_h
#define ov_websocket_pointer_h

#include "ov_http_pointer.h"
#include <stdbool.h>

#define OV_WEBSOCKET_MAGIC_BYTE 0xF0DF
#define OV_WEBSOCKET_SECURE_KEY_SIZE 24

/*
 *      ------------------------------------------------------------------------
 *
 *      SEMANTICS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_WEBSOCKET_VERSION "13"

#define OV_WEBSOCKET_KEY "websocket"
#define OV_WEBSOCKET_KEY_UPGRADE "upgrade"
#define OV_WEBSOCKET_KEY_SECURE "Sec-WebSocket-Key"
#define OV_WEBSOCKET_KEY_SECURE_VERSION "Sec-WebSocket-Version"
#define OV_WEBSOCKET_KEY_SECURE_ACCEPT "Sec-WebSocket-Accept"

#define OV_WEBSOCKET_KEY_SECURE_PROTOCOL "Sec-WebSocket-Protocol"
#define OV_WEBSOCKET_KEY_SECURE_EXTENSION "Sec-WebSocket-Extension"

/*
 *      ------------------------------------------------------------------------
 *
 *      STRUCTS
 *
 *      ------------------------------------------------------------------------
 */

typedef enum {

  OV_WEBSOCKET_PARSER_ABSENT = -3,  // not present e.g. when searching a key
  OV_WEBSOCKET_PARSER_OOB = -2,     // Out of Bound e.g. for array
  OV_WEBSOCKET_PARSER_ERROR = -1,   // processing error or mismatch
  OV_WEBSOCKET_PARSER_PROGRESS = 0, // still matching, need more input data
  OV_WEBSOCKET_PARSER_SUCCESS = 1   // content match

} ov_websocket_parser_state;

/*----------------------------------------------------------------------------*/

typedef enum {

  OV_WEBSOCKET_FRAGMENTATION_ERROR = -1,   // input error
  OV_WEBSOCKET_FRAGMENTATION_NONE = 0,     // not fragmented
  OV_WEBSOCKET_FRAGMENTATION_START = 1,    // start frame
  OV_WEBSOCKET_FRAGMENTATION_CONTINUE = 2, // continue frame
  OV_WEBSOCKET_FRAGMENTATION_LAST = 3      // last frame

} ov_websocket_fragmentation_state;

/*----------------------------------------------------------------------------*/

typedef enum {

  OV_WEBSOCKET_OPCODE_CONTINUATION = 0x00,
  OV_WEBSOCKET_OPCODE_TEXT = 0x01,
  OV_WEBSOCKET_OPCODE_BINARY = 0x02,
  OV_WEBSOCKET_OPCODE_CLOSE = 0x08,
  OV_WEBSOCKET_OPCODE_PING = 0x09,
  OV_WEBSOCKET_OPCODE_PONG = 0x0A

} ov_websocket_opcode;

/*----------------------------------------------------------------------------*/

typedef struct ov_websocket_frame_config {

  struct {

    size_t default_size;      // default buffer size
    size_t max_bytes_recache; // max buffer size to recache

  } buffer;

} ov_websocket_frame_config;

/*----------------------------------------------------------------------------*/

typedef struct {

  uint16_t magic_byte;
  ov_websocket_frame_config config;

  ov_websocket_opcode opcode;
  ov_websocket_fragmentation_state state;

  const uint8_t *mask;

  ov_memory_pointer content;

  ov_buffer *buffer;

} ov_websocket_frame;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_websocket_frame *ov_websocket_frame_create(ov_websocket_frame_config config);
bool ov_websocket_frame_clear(ov_websocket_frame *message);
void *ov_websocket_frame_free(void *message);
void *ov_websocket_frame_free_uncached(void *message);
ov_websocket_frame *ov_websocket_frame_cast(void *message);

/*----------------------------------------------------------------------------*/

void ov_websocket_enable_caching(size_t capacity);

/*----------------------------------------------------------------------------*/

/**
    Shift all additional bytes after the frame to a new frame
    to buffer additional incoming data.

    This function SHOULD be used in combination with
    @see ov_websocket_parse_frame after success of the parsing, to move
    any potential additional data to some new buffer and to clean the source
    frame from any additional bytes behind message end.

    @param source   source frame to be cleaned from additional bytes
    @param next     pointer to end of the frame within the source frame buffer
    @param dest     output pointer for new frame, containing the
                    additional bytes (if any)

    @NOTE after *dest is set it SHOULD be checked using
    @see ov_websocket_parse_frame for compliance

    @NOTE source->buffer will be set to 0 starting at next
    @NOTE on success *dest will be set and MAY be an empty ov_websocket_frame
    if no traling bytes are contained in source
*/
bool ov_websocket_frame_shift_trailing_bytes(ov_websocket_frame *source,
                                             uint8_t *next,
                                             ov_websocket_frame **dest);

/*
 *      ------------------------------------------------------------------------
 *
 *      HANDSHAKE
 *
 *      ------------------------------------------------------------------------
 */

/**
    Process some handshake request.

    NOTE the host header is checked for presence, but ignored otherwise
    NOTE method uri (target) of the request is ignored in processing

    So this function is handling all websocket related data, without host or
    request target specific content, which SHOULD be checked out of websocket
    processing context.

    @param msg          original input message
    @param out          generated respone (MUST present and will be set on true)
    @param is_handshake will be set to true, when the request is a handshake,
                        so any error repsonse with is_handshake true is a
                        processing error of the handshake

    @returns false if the request is NOT a handshake request,
    or processing failed.
*/
bool ov_websocket_process_handshake_request(const ov_http_message *msg,
                                            ov_http_message **out,
                                            bool *is_handshake);

/*
 *      ------------------------------------------------------------------------
 *
 *      DATA
 *
 *      ------------------------------------------------------------------------
 */

/**
    Set content data in a websocket frame

    @param frame    frame to use
    @param data     data to be set
    @param size     sizeof data
    @param mask     if true apply masking

    @returns true if the data was set to the frame (frame will be reparsed)
*/
bool ov_websocket_set_data(ov_websocket_frame *frame, const uint8_t *data,
                           size_t length, bool mask);

/*----------------------------------------------------------------------------*/

/**
    Unmask a frame if masking is set.

    @param frame    parsed frame input

    @returns true if mask is NOT set
    @returns true if unmasking was successful
    @returns false if mask is set without content
    @returns false if unmasking failed

*/
bool ov_websocket_frame_unmask(ov_websocket_frame *parsed_frame);

/*
 *      ------------------------------------------------------------------------
 *
 *      DE / ENCODING
 *
 *      ------------------------------------------------------------------------
 */

ov_websocket_parser_state ov_websocket_parse_frame(ov_websocket_frame *self,
                                                   uint8_t **next);

/*
 *      ------------------------------------------------------------------------
 *
 *      SECURE KEY
 *
 *      ------------------------------------------------------------------------
 */

/**
    Generate a random base64 encoded secure key.

    @param buffer   buffer to fill
    @param size     size of buffer (min OV_WEBSOCKET_SECURE_KEY_SIZE)
*/
bool ov_websocket_generate_secure_websocket_key(uint8_t *buffer, size_t size);

/*----------------------------------------------------------------------------*/

/**
    Generate a secure accept key,
    based on some secure key input.

    @param key      secure key to use
    @param length   length of the key to use (MUST be 24)
    @param result   pointer for result buffer
    @param size     pointer for result buffer size

    NOTE *result will be some allocated string and MUST be freed by the caller
*/
bool ov_websocket_generate_secure_accept_key(const uint8_t *key, size_t length,
                                             uint8_t **result, size_t *size);

/*----------------------------------------------------------------------------*/

ov_websocket_frame_config
ov_websocket_frame_config_from_json(const ov_json_value *value);
ov_json_value *
ov_websocket_frame_config_to_json(ov_websocket_frame_config config);

/*----------------------------------------------------------------------------*/

ov_websocket_frame *
ov_websocket_frame_pop(ov_buffer **buffer,
                       const ov_websocket_frame_config *config,
                       ov_websocket_parser_state *state);

#endif /* ov_websocket_pointer_h */
