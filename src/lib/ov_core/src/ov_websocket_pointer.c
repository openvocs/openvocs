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
        @file           ov_websocket_pointer.c
        @author         Markus Töpfer

        @date           2020-12-26


        ------------------------------------------------------------------------
*/
#include "../include/ov_websocket_pointer.h"

#include <ov_base/ov_utils.h>
#include <strings.h>

#include <ov_base/ov_base64.h>
#include <ov_base/ov_random.h>
#include <ov_base/ov_registered_cache.h>
#include <ov_base/ov_string.h>

#include <ov_base/ov_id.h>

#include <ov_encryption/ov_hash.h>

#define OV_WEBSOCKET_FRAME_DEFAULT_BUFFER_SIZE 1024

static const size_t ws_key_len = strlen(OV_WEBSOCKET_KEY);
static const size_t ws_key_upgrade_len = strlen(OV_WEBSOCKET_KEY_UPGRADE);

static const char *GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
static const size_t GUID_LEN = 36;

static ov_registered_cache *g_cache = 0;

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      #SECURE KEY
 *
 *      ------------------------------------------------------------------------
 */

bool ov_websocket_generate_secure_websocket_key(uint8_t *buffer, size_t size) {

    if (!buffer || !size)
        goto error;

    if (size < OV_WEBSOCKET_SECURE_KEY_SIZE)
        goto error;

    ov_id uuid = {0};
    ov_id_fill_with_uuid(uuid);

    if (!ov_base64_encode((uint8_t *)uuid, 16, &buffer, &size))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_websocket_generate_secure_accept_key(const uint8_t *key, size_t len,
                                             uint8_t **result, size_t *size) {

    if (!key || (len != OV_WEBSOCKET_SECURE_KEY_SIZE) || !result || !size)
        return false;

    size_t length = len + 2 + GUID_LEN;

    uint8_t acceptkey[length];
    memset(acceptkey, 0, length);

    memcpy(acceptkey, key, len);

    // STEP 2 - concatenate the GUID
    strcat((char *)acceptkey, GUID);

    // STEP 3 - SHA1 hash the result
    size_t buf_size = OV_SHA1_SIZE;
    uint8_t buf[buf_size];
    uint8_t *hash = buf;

    if (!ov_hash_string(OV_HASH_SHA1, acceptkey, strlen((char *)acceptkey),
                        &hash, &buf_size))
        return false;

    // STEP 4 - BASE64 encode the result
    if (!ov_base64_encode(hash, SHA_DIGEST_LENGTH, result, size))
        return false;

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #REQUEST
 *
 *      ------------------------------------------------------------------------
 */

static bool is_upgrade_request(const ov_http_message *msg) {

    if (!msg)
        goto error;

    const ov_http_header *upgrade = ov_http_header_get_unique(
        msg->header, msg->config.header.capacity, OV_HTTP_KEY_UPGRADE);

    const ov_http_header *connection = ov_http_header_get_unique(
        msg->header, msg->config.header.capacity, OV_HTTP_KEY_CONNECTION);

    if (!upgrade || !connection)
        goto error;

    /* Upgrade request MUST be websocket */

    if (upgrade->value.length != ws_key_len)
        goto error;

    if (0 !=
        strncasecmp((char *)upgrade->value.start, OV_WEBSOCKET_KEY, ws_key_len))
        goto error;

    /*
     *      This is an example firefix request:
     *
     *      Connection:keep-alive, Upgrade
     *      Upgrade:websocket
     */

    if (!ov_http_pointer_find_item_in_comma_list(
            connection->value.start, connection->value.length,
            (uint8_t *)OV_WEBSOCKET_KEY_UPGRADE, ws_key_upgrade_len, true))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_websocket_process_handshake_request(const ov_http_message *msg,
                                            ov_http_message **out,
                                            bool *is_handshake) {

    size_t accept_key_size = 31; // required + terminating /0
    uint8_t buffer[accept_key_size];
    memset(buffer, 0, accept_key_size);
    uint8_t *accept_key = buffer;

    if (!msg || !out || !ov_http_is_request(msg, OV_HTTP_METHOD_GET))
        goto error;

    if (!is_upgrade_request(msg))
        goto error;

    const ov_http_header *host = ov_http_header_get_unique(
        msg->header, msg->config.header.capacity, OV_HTTP_KEY_HOST);

    const ov_http_header *sec_key = ov_http_header_get_unique(
        msg->header, msg->config.header.capacity, OV_WEBSOCKET_KEY_SECURE);

    const ov_http_header *sec_ver =
        ov_http_header_get_unique(msg->header, msg->config.header.capacity,
                                  OV_WEBSOCKET_KEY_SECURE_VERSION);

    if (!host || !sec_key || !sec_ver)
        goto error;

    if (is_handshake)
        *is_handshake = true;

    /*  NOTE
     *  Origin is not checked, as we do not limit based on origin.
     *  PATH is not checked, as we do not differentiate between paths
     */

    if (0 != strncmp(OV_WEBSOCKET_VERSION, (char *)sec_ver->value.start,
                     sec_ver->value.length)) {

        *out = ov_http_create_status_string(msg->config, msg->version, 426,
                                            OV_HTTP_UPGRADE_REQUIRED);

        ov_http_message_add_header_string(*out, OV_WEBSOCKET_KEY_SECURE_VERSION,
                                          OV_WEBSOCKET_VERSION);

        ov_http_message_close_header(*out);

        goto error;
    }

    if (ov_http_header_get(msg->header, msg->config.header.capacity,
                           OV_WEBSOCKET_KEY_SECURE_EXTENSION))
        ov_log_error("Websocket request with extension set - "
                     " unsupported (ignoring extensions!)");

    if (ov_http_header_get(msg->header, msg->config.header.capacity,
                           OV_WEBSOCKET_KEY_SECURE_PROTOCOL))
        ov_log_error("Websocket request with subprotocols set - "
                     " unsupported (ignoring subprotocols!)");

    /*  sec_key
     *
     *  MUST be a base64 encoded 16 byte value
     *  (24 bytes) */

    if (sec_key->value.length != 24)
        goto error;

    if (!ov_websocket_generate_secure_accept_key(sec_key->value.start,
                                                 sec_key->value.length,
                                                 &accept_key, &accept_key_size))
        goto error;

    /* Create response */

    *out = ov_http_create_status_string(msg->config, msg->version, 101,
                                        OV_HTTP_SWITCH_PROTOCOLS);

    ov_http_message_add_header_string(*out, OV_HTTP_KEY_UPGRADE,
                                      OV_WEBSOCKET_KEY);

    ov_http_message_add_header_string(*out, OV_HTTP_KEY_CONNECTION,
                                      OV_HTTP_KEY_UPGRADE);

    ov_http_message_add_header_string(*out, OV_WEBSOCKET_KEY_SECURE_ACCEPT,
                                      (char *)accept_key);

    ov_http_message_close_header(*out);

    return true;
error:
    if (is_handshake)
        *is_handshake = false;
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #CACHING
 *
 *      ------------------------------------------------------------------------
 */

void ov_websocket_enable_caching(size_t capacity) {

    ov_registered_cache_config cfg = {

        .capacity = capacity,
        .item_free = ov_websocket_frame_free_uncached,

    };

    g_cache = ov_registered_cache_extend("websocket_frame", cfg);
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #CREATE
 *
 *      ------------------------------------------------------------------------
 */

static ov_websocket_frame_config config_init(ov_websocket_frame_config config) {

    if (0 == config.buffer.default_size)
        config.buffer.default_size = OV_WEBSOCKET_FRAME_DEFAULT_BUFFER_SIZE;
    return config;
}

/*----------------------------------------------------------------------------*/

static ov_websocket_frame *frame_create(ov_registered_cache *cache,
                                        ov_websocket_frame_config config) {

    ov_websocket_frame *frame = ov_registered_cache_get(cache);

    if (!frame) {

        frame = calloc(1, sizeof(ov_websocket_frame));
        if (!frame)
            goto error;

        frame->magic_byte = OV_WEBSOCKET_MAGIC_BYTE;
    }

    OV_ASSERT(ov_websocket_frame_cast(frame));
    frame->config = config;

    if (frame->buffer) {

        ov_buffer_clear(frame->buffer);

        if (frame->buffer->capacity < frame->config.buffer.default_size)
            if (!ov_buffer_extend(frame->buffer,
                                  frame->config.buffer.default_size -
                                      frame->buffer->capacity))
                goto error;

    } else {

        frame->buffer = ov_buffer_create(config.buffer.default_size);
    }

    return frame;
error:
    ov_websocket_frame_free_uncached(frame);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_websocket_frame *
ov_websocket_frame_create(ov_websocket_frame_config config) {

    config = config_init(config);

    ov_websocket_frame *frame = frame_create(g_cache, config);

    if (!frame)
        return NULL;

    OV_ASSERT(ov_websocket_frame_cast(frame));
    OV_ASSERT(frame->buffer);
    return frame;
}

/*----------------------------------------------------------------------------*/

bool ov_websocket_frame_clear(ov_websocket_frame *data) {

    ov_websocket_frame *frame = ov_websocket_frame_cast(data);
    if (!frame)
        goto error;

    frame->opcode = 0;
    frame->state = OV_WEBSOCKET_FRAGMENTATION_NONE;
    frame->mask = NULL;
    frame->content = (ov_memory_pointer){0};

    if (frame->buffer) {

        if ((0 != frame->config.buffer.max_bytes_recache) &&
            (frame->buffer->capacity >
             frame->config.buffer.max_bytes_recache)) {
            frame->buffer = ov_buffer_free_uncached(frame->buffer);
        } else {
            ov_buffer_clear(frame->buffer);
        }
    }

    if (!frame->buffer)
        frame->buffer = ov_buffer_create(frame->config.buffer.default_size);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

void *ov_websocket_frame_free(void *data) {

    ov_websocket_frame *frame = ov_websocket_frame_cast(data);
    if (!ov_websocket_frame_clear(frame))
        return data;

    if ((0 != frame->config.buffer.max_bytes_recache) &&
        (frame->buffer->capacity > frame->config.buffer.max_bytes_recache)) {
        frame->buffer = ov_buffer_free_uncached(frame->buffer);
    }

    frame->buffer = ov_buffer_free(frame->buffer);
    frame = ov_registered_cache_put(g_cache, frame);
    return ov_websocket_frame_free_uncached(frame);
}

/*----------------------------------------------------------------------------*/

void *ov_websocket_frame_free_uncached(void *data) {

    ov_websocket_frame *frame = ov_websocket_frame_cast(data);
    if (!ov_websocket_frame_clear(frame))
        return data;

    frame->buffer = ov_buffer_free_uncached(frame->buffer);
    frame = ov_data_pointer_free(frame);
    return frame;
}

/*----------------------------------------------------------------------------*/

ov_websocket_frame *ov_websocket_frame_cast(void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data == OV_WEBSOCKET_MAGIC_BYTE)
        return (ov_websocket_frame *)data;

    return NULL;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      DE / ENCODING
 *
 *      ------------------------------------------------------------------------
 */

static ov_websocket_parser_state parse_payload_length(ov_websocket_frame *frame,
                                                      uint8_t **next) {

    /*
     *  This function will return SUCCESS if the length is set correct,
     *  and all bytes of the content are contained within the buffer.
     *
     *  If some bytes are missing, but the buffer is valid in terms of
     *  length parameter, the function will return PROGRESS.
     *
     *  If the length is wrong in terms of base framing of RFC 6455
     *  the function will return ERROR.
     *
     *  If some masking is used, this will shift the content start,
     *  so masking is part of length evaluation and included within this
     *  function.
     *
     *  ON SUCCESS:
     *
     *  frame->mask will point to mask within the frame buffer if used
     *  frame->content.start will point to content start within the frame buffer
     *  frame->content.length will contain the length of the content
     *
     *  If next is present it will point to the first byte
     *  after the frame content, which may be the next frame within the buffer
     *  e.g. due to io read of additional bytes.
     *
     */

    ov_websocket_parser_state state = OV_WEBSOCKET_PARSER_ERROR;

    if (!frame || !frame->buffer)
        goto error;

    if (frame->buffer->length < 2)
        return OV_WEBSOCKET_PARSER_PROGRESS;

    int64_t length = frame->buffer->start[1] & 0x7F;

    if (length < 126) {

        frame->content.start = frame->buffer->start + 2;
        frame->content.length = length;

        if (length == 0)
            frame->content.start = NULL;

        state = OV_WEBSOCKET_PARSER_SUCCESS;
        goto finish;
    }

    if (length == 126) {

        if (frame->buffer->length < 4)
            return OV_WEBSOCKET_PARSER_PROGRESS;

        frame->content.start = frame->buffer->start + 4;

        length = frame->buffer->start[2] << 8 | frame->buffer->start[3];
        if (length < 125)
            goto error;

        frame->content.length = length;

        state = OV_WEBSOCKET_PARSER_SUCCESS;
        goto finish;
    }

    if (length != 127) {
        goto error;
    }

    length = 0;

    // most significant bit must be 0
    if (frame->buffer->start[2] & 0x80)
        goto error;

    if (frame->buffer->length < 10)
        return OV_WEBSOCKET_PARSER_PROGRESS;

    /*
     *  Here we use integer assignments,
     *  to BYTE SHIFT at COPY level of the original buffer.
     *
     *  Some other network to host conversion may be used,
     *  but this byte shifting approach will work
     *  independent of the host encoding (litte or big endian)
     */

    int64_t byte1 = frame->buffer->start[2];
    int64_t byte2 = frame->buffer->start[3];
    int64_t byte3 = frame->buffer->start[4];
    int64_t byte4 = frame->buffer->start[5];
    int64_t byte5 = frame->buffer->start[6];
    int64_t byte6 = frame->buffer->start[7];
    int64_t byte7 = frame->buffer->start[8];
    int64_t byte8 = frame->buffer->start[9];

    length = (byte1 << 56) + (byte2 << 48) + (byte3 << 40) + (byte4 << 32) +
             (byte5 << 24) + (byte6 << 16) + (byte7 << 8) + byte8;

    if (length < 0x10000)
        goto error;

    frame->content.start = frame->buffer->start + +10;
    frame->content.length = length;

    state = OV_WEBSOCKET_PARSER_SUCCESS;

finish:

    OV_ASSERT(state == OV_WEBSOCKET_PARSER_SUCCESS);

    /* Masking used ? */

    if (frame->buffer->start[1] & 0x80) {

        if (NULL == frame->content.start)
            goto error;

        frame->mask = frame->content.start;
        frame->content.start += 4;
    }

    /*  All bytes of content received ? */

    if (frame->content.start) {

        if (frame->buffer->length <
            ((frame->content.start - frame->buffer->start) +
             frame->content.length))
            return OV_WEBSOCKET_PARSER_PROGRESS;

        if (next)
            *next = (uint8_t *)frame->content.start + frame->content.length;

    } else if (next) {

        // no content (0 length)

        *next = frame->buffer->start + 2;
    }

    return state;
error:
    return OV_WEBSOCKET_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

/*
        Websocket frame

                 0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-------+-+-------------+-------------------------------+
        |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
        |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
        |N|V|V|V|       |S|             |   (if payload len==126/127)   |
        | |1|2|3|       |K|             |                               |
        +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
        |     Extended payload length continued, if payload len == 127  |
        + - - - - - - - - - - - - - - - +-------------------------------+
        |                               |Masking-key, if MASK set to 1  |
        +-------------------------------+-------------------------------+
        | Masking-key (continued)       |          Payload Data         |
        +-------------------------------- - - - - - - - - - - - - - - - +
        :                     Payload Data continued ...                :
        + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
        |                     Payload Data continued ...                |
        +---------------------------------------------------------------+
*/

/*----------------------------------------------------------------------------*/

static ov_websocket_fragmentation_state
fragmentation_state(const uint8_t buffer) {

    bool fin = false;
    bool opcode = false;

    if (buffer & 0x80)
        fin = true;

    if (0 != (buffer & 0x0F))
        opcode = true;

    if (fin) {

        if (opcode)
            return OV_WEBSOCKET_FRAGMENTATION_NONE;

        return OV_WEBSOCKET_FRAGMENTATION_LAST;
    }

    if (opcode)
        return OV_WEBSOCKET_FRAGMENTATION_START;

    return OV_WEBSOCKET_FRAGMENTATION_CONTINUE;
}

/*----------------------------------------------------------------------------*/

ov_websocket_parser_state ov_websocket_parse_frame(ov_websocket_frame *frame,
                                                   uint8_t **next) {

    ov_websocket_parser_state state = OV_WEBSOCKET_PARSER_ERROR;

    if (!frame)
        goto error;

    if (!frame->buffer)
        goto error;

    if (!frame->buffer->start)
        goto error;

    if (frame->buffer->length == 0)
        goto error;

    /* Unset valued to be set */

    frame->mask = NULL;
    frame->content.start = NULL;
    frame->content.length = 0;

    state = parse_payload_length(frame, next);

    if (state != OV_WEBSOCKET_PARSER_SUCCESS)
        return state;

    switch (0x0F & frame->buffer->start[0]) {

    case OV_WEBSOCKET_OPCODE_CONTINUATION:
    case OV_WEBSOCKET_OPCODE_TEXT:
    case OV_WEBSOCKET_OPCODE_BINARY:
    case OV_WEBSOCKET_OPCODE_CLOSE:
    case OV_WEBSOCKET_OPCODE_PING:
    case OV_WEBSOCKET_OPCODE_PONG:
        frame->opcode = 0x0F & frame->buffer->start[0];
        break;
    default:
        goto error;
    }

    frame->state = fragmentation_state(frame->buffer->start[0]);

    return OV_WEBSOCKET_PARSER_SUCCESS;

error:
    if (next && frame && frame->buffer)
        *next = frame->buffer->start;

    return OV_WEBSOCKET_PARSER_ERROR;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      DATA MASKING
 *
 *      ------------------------------------------------------------------------
 */

static bool generate_masking_key(uint8_t *buffer, size_t size) {

    if (!buffer || size < 4)
        return false;

    return ov_random_bytes(buffer, 4);
}

/*----------------------------------------------------------------------------*/

static bool mask_data(uint8_t *buffer, size_t size, const uint8_t *mask) {

    if (!buffer || size < 1 || !mask)
        goto error;

    uint8_t j = 0;

    for (size_t i = 0; i < size; i++) {
        j = i % 4;
        buffer[i] = buffer[i] ^ mask[j];
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_websocket_frame_unmask(ov_websocket_frame *frame) {

    if (!frame)
        goto error;

    if (!frame->content.start) {

        if (frame->mask)
            goto error;

        goto done;
    }

    if (frame->mask)
        return mask_data((uint8_t *)frame->content.start, frame->content.length,
                         frame->mask);
done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_websocket_set_data(ov_websocket_frame *frame, const uint8_t *data,
                           size_t length, bool mask) {

    if (!frame)
        goto error;

    size_t required = 0;

    if (!data)
        length = 0;

    if (0 == length)
        data = NULL;

    if (length < 126) {

        required = 2 + length;

    } else if (length < 0xffff) {

        required = 4 + length;

    } else {

        required = 10 + length;
    }

    if (mask)
        required += 4;

    if (!frame->buffer) {

        frame->buffer = ov_buffer_create(required);

    } else if (frame->buffer->capacity < required) {

        if (!ov_buffer_extend(frame->buffer,
                              required - frame->buffer->capacity))
            goto error;
    }

    // set buffer to empty do not change byte 0
    memset(frame->buffer->start + 1, 0, frame->buffer->capacity - 1);

    if (!data || (0 == length)) {

        if (mask)
            goto error;
    }

    uint8_t *next = NULL;
    uint8_t *mask_start = NULL;

    uint8_t flag_mask = 0;
    if (mask)
        flag_mask = 0x80;

    // set length
    if (length < 126) {

        frame->buffer->start[1] = flag_mask | length;
        next = frame->buffer->start + 2;

    } else if (length < 0xffff) {

        frame->buffer->start[1] = flag_mask | 126;
        frame->buffer->start[2] = length >> 8;
        frame->buffer->start[3] = length;
        next = frame->buffer->start + 4;

    } else {

        frame->buffer->start[1] = flag_mask | 127;
        frame->buffer->start[2] = length >> 56;
        frame->buffer->start[3] = length >> 48;
        frame->buffer->start[4] = length >> 40;
        frame->buffer->start[5] = length >> 32;
        frame->buffer->start[6] = length >> 24;
        frame->buffer->start[7] = length >> 16;
        frame->buffer->start[8] = length >> 8;
        frame->buffer->start[9] = length;
        next = frame->buffer->start + 10;
    }

    if (mask) {

        if (!generate_masking_key(next, 4))
            goto error;

        mask_start = next;
        next += 4;
    }

    if (data && (0 != length)) {

        if (!memcpy(next, data, length))
            goto error;
    }

    if (mask) {

        if (!mask_data(next, length, mask_start))
            goto error;
    }

    /*  Reparse to frame and set correct buffer length */

    next = NULL;
    frame->buffer->length = frame->buffer->capacity;

    if (OV_WEBSOCKET_PARSER_SUCCESS != ov_websocket_parse_frame(frame, &next))
        goto error;

    frame->buffer->length = next - frame->buffer->start;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_websocket_frame_shift_trailing_bytes(ov_websocket_frame *source,
                                             uint8_t *next,
                                             ov_websocket_frame **dest) {

    ov_websocket_frame *new = NULL;
    if (!source || !next || !dest)
        goto error;

    new = ov_websocket_frame_create(source->config);
    if (!new)
        goto error;

    if (next == source->buffer->start + source->buffer->length)
        goto done;

    size_t len = source->buffer->length - (next - source->buffer->start);

    if (!ov_buffer_set(new->buffer, next, len))
        goto error;

    if (!memset(next, 0, len))
        goto error;

    source->buffer->length = (next - source->buffer->start);

done:
    *dest = new;
    return true;
error:
    new = ov_websocket_frame_free(new);
    return false;
}
/*----------------------------------------------------------------------------*/

ov_websocket_frame_config
ov_websocket_frame_config_from_json(const ov_json_value *value) {

    ov_websocket_frame_config out = {0};
    if (!value)
        goto error;

    ov_json_value *val = NULL;
    const ov_json_value *config = ov_json_object_get(value, OV_KEY_WEBSOCKET);
    if (!config)
        config = value;

    val = ov_json_object_get(config, OV_KEY_BUFFER);
    out.buffer.default_size =
        ov_json_number_get(ov_json_object_get(val, OV_KEY_SIZE));
    out.buffer.max_bytes_recache =
        ov_json_number_get(ov_json_object_get(val, OV_KEY_MAX_CACHE));

    return out;
error:
    return (ov_websocket_frame_config){0};
}

/*----------------------------------------------------------------------------*/

ov_json_value *
ov_websocket_frame_config_to_json(ov_websocket_frame_config config) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *item = NULL;

    out = ov_json_object();
    if (!out)
        goto error;

    val = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_BUFFER, val))
        goto error;

    item = val;
    val = NULL;

    val = ov_json_number(config.buffer.default_size);
    if (!ov_json_object_set(item, OV_KEY_SIZE, val))
        goto error;

    val = ov_json_number(config.buffer.max_bytes_recache);
    if (!ov_json_object_set(item, OV_KEY_MAX_CACHE, val))
        goto error;

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_websocket_frame *
ov_websocket_frame_pop(ov_buffer **input,
                       const ov_websocket_frame_config *config,
                       ov_websocket_parser_state *state) {

    ov_websocket_frame *frame = NULL;
    ov_buffer *new_buffer = NULL;

    ov_websocket_parser_state s = OV_WEBSOCKET_PARSER_ERROR;

    if (!input || !config || !state)
        goto error;

    ov_buffer *buffer = *input;

    frame = calloc(1, sizeof(ov_websocket_frame));
    if (!frame)
        goto error;
    frame->magic_byte = OV_WEBSOCKET_MAGIC_BYTE;
    frame->config = *config;
    frame->buffer = buffer;

    uint8_t *next = NULL;

    s = ov_websocket_parse_frame(frame, &next);

    switch (s) {

    case OV_WEBSOCKET_PARSER_PROGRESS:
        frame->buffer = NULL;
        new_buffer = buffer;
        goto done;

    case OV_WEBSOCKET_PARSER_SUCCESS:
        break;

    default:
        frame->buffer = NULL;
        goto error;
    }

    if (next == frame->buffer->start + frame->buffer->length)
        goto done;

    size_t len = frame->buffer->length - (next - frame->buffer->start);
    new_buffer = ov_buffer_create(len);

    if (!ov_buffer_set(new_buffer, next, len))
        goto error;

    if (!memset(next, 0, len))
        goto error;

    frame->buffer->length = (next - frame->buffer->start);

done:
    *state = s;
    *input = new_buffer;
    return frame;

error:
    if (state)
        *state = OV_WEBSOCKET_PARSER_ERROR;
    frame = ov_websocket_frame_free(frame);
    return NULL;
}