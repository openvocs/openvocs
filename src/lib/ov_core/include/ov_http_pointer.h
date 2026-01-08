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
        @file           ov_http_pointer.h
        @author         Markus Töpfer

        @date           2020-12-10

        This implementation will created semantial pointers to some plugged in
        buffer, without changing the source buffer. The source buffer is 100%
        independent of the semantical HTTP pointer overhead, but the HTTP
        pointer is dependent on the source buffer.

        When using this function it MUST be ensured the source buffer is valid,
        as long as the ov_http_pointer is used.

        Therefore ov_http_message_create will create a buffer on init
        and the content buffer is managed within ov_http_message.

        The buffer MAY be unplugged at ANY time using msg->buffer = NULL.
        Whenever the buffer is unplugged, it MUST be freed using ov_buffer_free
        at some point, to avoid memleaks.

        Intended usage of ov_http_message :

            char *next = NULL;
            ov_http_parser_state state = OV_HTTP_PARSER_ERROR;

            ov_http_message *msg = ov_http_message_create(config);

                ...

            ssize_t in = recv(socket,msg->buffer->start,msg->buffer->length,0);

                ...

            state = ov_http_pointer_parse_message(msg, &next);

            if (state == OV_HTTP_PARSER_SUCCESS){

                if (next == msg->buffer->start + msg->buffer->length)){

                    // exactely ONE http message received

                    process_http_in(msg);

                } else {

                    // some trailing bytes contained in msg->buffer

                    ov_http_message *next_msg = NULL;
                    ov_http_message_shift_trailing_bytes(msg, next, &next_msg);

                    // msg will now contain exactely ONE http message
                    // next_msg will contain any "overread" data e.g. the
                    // next http message received from network IO buffer

                    process_http_in(msg);

                }
            }

        NOTE    process_http_in will receive some allocated pointer based
                memory area and MAY transfer the pointer to threads for
                further processing.

        NOTE    allocations MAY be minimized with ov_http_message_enable_caching
                to cache ov_http_message structures AND contained buffers.

                ov_http_message_free is a threadsafe implementation to release
                some structures and return them into the cache

        ------------------------------------------------------------------------
*/
#ifndef ov_http_pointer_h
#define ov_http_pointer_h

#include <inttypes.h>
#include <stdlib.h>

#include <ov_base/ov_buffer.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_memory_pointer.h>

#include <ov_format/ov_file_format.h>

#define OV_HTTP_MESSAGE_MAGIC_BYTE 0xF0D0

/*
 *      METHODS
 */

#define OV_HTTP_METHOD_GET "GET"
#define OV_HTTP_METHOD_HEAD "HEAD"
#define OV_HTTP_METHOD_POST "POST"
#define OV_HTTP_METHOD_PUT "PUT"
#define OV_HTTP_METHOD_DELETE "DELETE"
#define OV_HTTP_METHOD_CONNECT "CONNECT"
#define OV_HTTP_METHOD_OPTIONS "OPTIONS"
#define OV_HTTP_METHOD_TRACE "TRACE"

/*
 *      STATUS
 */

#define OV_HTTP_CONTINUE "Continue"
#define OV_HTTP_SWITCH_PROTOCOLS "Switching Protocols"

#define OV_HTTP_OK "OK"
#define OV_HTTP_CREATED "Created"
#define OV_HTTP_ACCEPTED "Accepted"
#define OV_HTTP_NON_AUTH_INFO "Non-Authoritative Information"
#define OV_HTTP_NO_CONTENT "No Content"
#define OV_HTTP_RESET_CONTENT "Reset Content"
#define OV_HTTP_PARTIAL_CONTENT "Partial Content"

#define OV_HTTP_MULTIPLE_CHOICES "Multiple Choices"
#define OV_HTTP_MOVED_PERMANENTLY "Moved Permanently"
#define OV_HTTP_FOUND "Found"
#define OV_HTTP_SEE_OTHER "See Other"
#define OV_HTTP_NOT_MODIFIED "Not Modified"
#define OV_HTTP_USE_PROXY "Use Proxy"
#define OV_HTTP_TEMPORARY_REDIRECT "Temporary Redirect"

#define OV_HTTP_BAD_REQUEST "Bad Request"
#define OV_HTTP_UNAUTHORIZED "Unauthorized"
#define OV_HTTP_PAYMENT_REDIRECT "Payment Required"
#define OV_HTTP_FORBIDDEN "Forbidden"
#define OV_HTTP_NOT_FOUND "Not Found"
#define OV_HTTP_NOT_ALLOWED "Method Not Allowed"
#define OV_HTTP_NOT_ACCEPTABLE "Not Acceptable"
#define OV_HTTP_PROXY_AUTH_REQUIRED "Proxy Authentication Required"
#define OV_HTTP_REQUEST_TIMEOUT "Request Timeout"
#define OV_HTTP_CONFLICT "Conflict"
#define OV_HTTP_GONE "Gone"
#define OV_HTTP_LENGTH_REQUIRED "Length Required"
#define OV_HTTP_PRECONDITION_FAILED "Precondition Failed"
#define OV_HTTP_PAYLOAD_TO_LARGE "Payload Too Large"
#define OV_HTTP_URI_TO_LONG "URI Too Long"
#define OV_HTTP_UNSUPPORTED_MEDIA "Unsupported Media Type"
#define OV_HTTP_RANGE_NOT_SATISFIABLE "Range Not Satisfiable"
#define OV_HTTP_EXPECTATION_FAILED "Expectation Failed"
#define OV_HTTP_UPGRADE_REQUIRED "Upgrade Required"

#define OV_HTTP_INTERNAL_SERVER_ERROR "Internal Server Error"
#define OV_HTTP_NOT_IMPLEMENTED "Not Implemented"
#define OV_HTTP_BAD_GATEWAY "Bad Gateway"
#define OV_HTTP_SERVICE_UNAVAILABLE "Service Unavailable"
#define OV_HTTP_GATEWAY_TIMEOUT "Gateway Timeout"
#define OV_HTTP_VERSION_NOT_SUPPORTED "HTTP Version Not Supported"

/*
 *      HEADER FIELDS
 */

#define OV_HTTP_KEY_TRANSFER_ENCODING "Transfer-Encoding"
#define OV_HTTP_KEY_CONTENT_LENGTH "Content-Length"
#define OV_HTTP_KEY_CONTENT_RANGE "Content-Range"
#define OV_HTTP_KEY_CONTENT_TYPE "Content-Type"
#define OV_HTTP_KEY_CONNECT "Connect"
#define OV_HTTP_KEY_CONNECTION "Connection"
#define OV_HTTP_KEY_UPGRADE "Upgrade"
#define OV_HTTP_KEY_DATE "Date"
#define OV_HTTP_KEY_SERVER "Server"
#define OV_HTTP_KEY_HOST "Host"

#define OV_HTTP_KEY_LOCATION "Location"

/*
 *      TRANSFER ENCODING
 */

#define OV_HTTP_CHUNKED "chunked"
#define OV_HTTP_COMPRESS "compress"
#define OV_HTTP_DEFLATE "deflate"
#define OV_HTTP_GZIP "gzip"

/*----------------------------------------------------------------------------*/

typedef enum {

  OV_HTTP_PARSER_ABSENT = -3,  // not present e.g. when searching a key
  OV_HTTP_PARSER_OOB = -2,     // Out of Bound e.g. for array
  OV_HTTP_PARSER_ERROR = -1,   // processing error or mismatch
  OV_HTTP_PARSER_PROGRESS = 0, // still matching, need more input data
  OV_HTTP_PARSER_SUCCESS = 1   // content match

} ov_http_parser_state;

/*----------------------------------------------------------------------------*/

typedef struct {

  uint8_t major;
  uint8_t minor;

} ov_http_version;

/*----------------------------------------------------------------------------*/

typedef struct {

  uint32_t code;

  ov_memory_pointer phrase;

} ov_http_status;

/*----------------------------------------------------------------------------*/

typedef struct {

  ov_memory_pointer method;
  ov_memory_pointer uri;

} ov_http_request;

/*----------------------------------------------------------------------------*/

typedef struct {

  ov_memory_pointer name;
  ov_memory_pointer value;

} ov_http_header;

/*----------------------------------------------------------------------------*/

typedef struct ov_http_message_config {

  struct {

    size_t capacity;              // amount of headers supported
    size_t max_bytes_method_name; // max bytes of a method name
    size_t max_bytes_line;        // max bytes of a header line

  } header;

  struct {

    size_t default_size;      // default buffer size
    size_t max_bytes_recache; // max buffer size to recache

  } buffer;

  struct {

    size_t max; // max transfer encodings allowed

  } transfer;

  struct {

    size_t max_bytes; // max chunk size allowed

  } chunk;

} ov_http_message_config;

/*----------------------------------------------------------------------------*/

typedef struct {

  uint16_t magic_byte;
  ov_http_message_config config;

  ov_buffer *buffer;

  ov_memory_pointer body;

  /* Chunk will be set in case chunked transfer is used. */
  ov_memory_pointer chunk;

  /*  Startline entires.
   *
   *      quick request/status check will be
   *      if (0 == msg.status.code) -> request
   */

  ov_http_version version;
  ov_http_request request;
  ov_http_status status;

  /*  Array of header field pointers in order of reception of configred size
   */
  ov_http_header header[];

} ov_http_message;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_http_message *ov_http_message_create(ov_http_message_config config);
bool ov_http_message_clear(ov_http_message *message);
void *ov_http_message_free(void *message);
void *ov_http_message_free_uncached(void *message);
ov_http_message *ov_http_message_cast(void *message);

/*----------------------------------------------------------------------------*/

void ov_http_message_enable_caching(size_t capacity);

/*----------------------------------------------------------------------------*/

/**
    Set default parameter, if config parameter is 0
*/
ov_http_message_config
ov_http_message_config_init(ov_http_message_config config);

/*
 *      ------------------------------------------------------------------------
 *
 *      DE / ENCODING
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

/**
    This function will parse the state out of its own buffer.

    @param message  HTTP message with msg->buffer to parse
    @param next     pointer to next byte within own buffer

    @NOTE This function expects HTTP1.1 message content with either some
    Content-Length present, or Transfer-Encoding with chunked as last applied
    encoding, to estimate the size of the HTTP message.

    @NOTE This function will only parse the length specific characteristics,
    independent of message type (request/state) or any type related specifics.
    The purpose of this parsing is the check for length compliance and length
    estimation, as long as the buffer content is in line with the HTTP grammar.
*/
ov_http_parser_state ov_http_pointer_parse_message(ov_http_message *message,
                                                   uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
    This function will ensure the buffer of the msg has at least capacity open
    bytes.

    @param message  message to check
    @param capacity required open capacity to set
*/
bool ov_http_message_ensure_open_capacity(ov_http_message *message,
                                          size_t capacity);

/*----------------------------------------------------------------------------*/

/**
    Create some ov_http_message and set status line in msg->buffer

    @param config   (optional) config
    @param version  version (will be set as is)
    @param status   status to set
*/
ov_http_message *ov_http_create_status(ov_http_message_config config,
                                       ov_http_version version,
                                       ov_http_status status);

/*----------------------------------------------------------------------------*/

/**
    Create some ov_http_message and set status line in msg->buffer

    @param config   (optional) config
    @param version  version (will be set as is)
    @param code     code to set
    @param phrase   phrase to set
*/
ov_http_message *ov_http_create_status_string(ov_http_message_config config,
                                              ov_http_version version,
                                              uint16_t code,
                                              const char *phrase);

/*----------------------------------------------------------------------------*/

ov_http_message *ov_http_status_not_found(const ov_http_message *source);
ov_http_message *ov_http_status_forbidden(const ov_http_message *source);

/*----------------------------------------------------------------------------*/

/**
    Create some ov_http_message and set request line in msg->buffer

    @param config   (optional) config
    @param version  version (will be set as is)
    @param request  request to set

    @NOTE returns NULL if request.method.length >
    msg->config.header.max_bytes_method_name
*/
ov_http_message *ov_http_create_request(ov_http_message_config config,
                                        ov_http_version version,
                                        ov_http_request request);

/*----------------------------------------------------------------------------*/

/**
    Create some ov_http_message and set request line in msg->buffer

    @param config   (optional) config
    @param version  version (will be set as is)
    @param method   method string
    @param uri      uri string

    @NOTE returns NULL if strlen(method) >
    msg->config.header.max_bytes_method_name

    @NOTE returns NULL if strlen(uri) > PATH_MAX
*/
ov_http_message *ov_http_create_request_string(ov_http_message_config config,
                                               ov_http_version version,
                                               const char *method,
                                               const char *uri);

/*----------------------------------------------------------------------------*/

/**
    Check if the msg is a request and for some method name.
*/
bool ov_http_is_request(const ov_http_message *msg, const char *method);

/*----------------------------------------------------------------------------*/

/**
    Add some header to the msg buffer

    @param msg      message instance
    @param header   header to add

    @NOTE will write behind msg->buffer->length and reallocate in case of
    non sufficiant space

    @NOTE MUST be used before ov_http_message_close_header

    @NOTE msg MUST already contain a valid startline
*/
bool ov_http_message_add_header(ov_http_message *msg, ov_http_header header);

/*----------------------------------------------------------------------------*/

/**
    Add some header to the msg buffer

    @param msg      message instance
    @param key      header key
    @param val      header value

    @NOTE will write behind msg->buffer->length and reallocate in case of
    non sufficiant space

    @NOTE MUST be used before ov_http_message_close_header
*/
bool ov_http_message_add_header_string(ov_http_message *msg, const char *key,
                                       const char *val);

/*----------------------------------------------------------------------------*/

/**
    Add content types based on the file format description.

    @param msg      message instance
    @param mime     mime to be set
    @param charset  (optional) charset to be used
*/
bool ov_http_message_add_content_type(ov_http_message *msg, const char *mine,
                                      const char *charset);

/*----------------------------------------------------------------------------*/

/**
    Add transfer encodings based on optional file format description.

    Will add at least:
        "transfer-coding="chunked\r\n"

    May add (depending on file extensions of format)
        "transfer-coding="chunked;gzip;compress;deflate\r\n"

    @param msg      message instance
    @param format   (optional) file format description
*/
bool ov_http_message_add_transfer_encodings(ov_http_message *msg,
                                            const ov_file_format_desc *format);

/*----------------------------------------------------------------------------*/

bool ov_http_message_set_date(ov_http_message *msg);
bool ov_http_message_set_content_length(ov_http_message *msg, size_t length);
bool ov_http_message_set_content_range(ov_http_message *msg, size_t length,
                                       size_t start, size_t end);

/*----------------------------------------------------------------------------*/

/**
    Close the header of some message

    @param msg      message instance

    @NOTE will write empty line at end of msg->buffer
    @NOTE will not check if startline or headers are valid!
*/
bool ov_http_message_close_header(ov_http_message *msg);

/*----------------------------------------------------------------------------*/

/**
    Add some body to the msg buffer

    @param msg      message instance
    @param body     body to set

    @NOTE will check for header end within buffer and return error if the
    last 4 byte are NOT "\r\n\r\n"
*/
bool ov_http_message_add_body(ov_http_message *msg, ov_memory_pointer body);

/*----------------------------------------------------------------------------*/

/**
    Add some body as string

    @param msg      message instance
    @param body     body to set

    @NOTE will check for header end within buffer and return error if the
    last 4 byte are NOT "\r\n\r\n"
*/
bool ov_http_message_add_body_string(ov_http_message *msg, const char *body);

/*----------------------------------------------------------------------------*/

/**
    Shift all additional bytes after the message body to a new HTTP message
    to buffer additional incoming data.

    This function SHOULD be used in combination with
    @see ov_http_pointer_parse_message after success of the parsing, to move
    any potential additional data to some new buffer and to clean the source
    message from any additional bytes behind the HTTP message end.

    @param source   source message to be cleaned from additional bytes
    @param next     pointer to end of the http message
    @param dest     output pointer for new ov_http_message, containing the
                    additional bytes (if any)

    @NOTE after *dest is set it SHOULD be checked using
    @see ov_http_pointer_parse_message for grammar compliance

    @NOTE source->buffer will be set to 0 starting at next
    @NOTE on success *dest will be set and MAY be an empty ov_http_message
    if no traling bytes are contained in source
*/
bool ov_http_message_shift_trailing_bytes(ov_http_message *source,
                                          uint8_t *next,
                                          ov_http_message **dest);

/*----------------------------------------------------------------------------*/

/**
    Parse the version out of some buffer.

    @param buffer   pointer to start of the buffer to match
    @param length   length of the buffer to match
    @param version  (mandatory) ov_http_version to be set

    @returns parsing state of the buffer

    on OV_HTTP_PARSER_SUCCESS version->major and version->minor will be set
*/
ov_http_parser_state ov_http_pointer_parse_version(const uint8_t *buffer,
                                                   size_t length,
                                                   ov_http_version *version);

/*----------------------------------------------------------------------------*/

/**
    Parse the status line out of some buffer.

    @param buffer   pointer to start of the buffer to match
    @param length   length of the buffer to match
    @param status   (mandatory) ov_http_status to be set
    @param version  (mandatory) ov_http_version to be set

    @returns parsing state of the buffer

    @NOTE OV_HTTP_PARSER_SUCCESS will be set and phrase will point to the input
   buffer

    @NOTE will parse with AND without lineend
*/
ov_http_parser_state ov_http_pointer_parse_status_line(const uint8_t *buffer,
                                                       size_t length,
                                                       ov_http_status *status,
                                                       ov_http_version *version,
                                                       uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
    Parse the request line out of some buffer.

    @param buffer   pointer to start of the buffer to match
    @param length   length of the buffer to match
    @param request  (mandatory) ov_http_request to be set
    @param version  (mandatory) ov_http_version to be set
    @param max_req  (optional) max request length (0 == unlimited)

    @returns parsing state of the buffer
‚
    @NOTE OV_HTTP_PARSER_SUCCESS will be set and uri and method
    will point to the input buffer

    @NOTE will parse with AND without lineend
*/
ov_http_parser_state ov_http_pointer_parse_request_line(
    const uint8_t *buffer, size_t length, ov_http_request *request,
    ov_http_version *version, uint32_t max_req, uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
    Parse the header line out of some buffer.

    @param buffer   pointer to start of the buffer to match
    @param length   length of the buffer to match
    @param line     (mandatory) ov_http_header to be set
    @param max      (optional) max line length (0 == unlimited)

    @returns parsing state of the buffer

    @NOTE OV_HTTP_PARSER_SUCCESS name and value will point to the input buffer

    @NOTE will parse success ONLY with lineend
*/
ov_http_parser_state ov_http_pointer_parse_header_line(const uint8_t *buffer,
                                                       size_t length,
                                                       ov_http_header *line,
                                                       uint32_t max,
                                                       uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
    Parse the header out of some buffer.

    @param buffer   pointer to start of the buffer to match
    @param length   length of the buffer to match
    @param array    array of ov_http_header lines
    @param size     size of array
    @param line     (mandatory) ov_http_header to be set
    @param max      (optional) max_header_line length (0 == unlimited)
    @param next     (optional) pointer to next byte after header end

    @returns parsing state of the buffer

    @NOTE will parse success ONLY on \r\n\r\n
*/
ov_http_parser_state
ov_http_pointer_parse_header(const uint8_t *buffer, size_t length,
                             ov_http_header *array, size_t size,
                             uint32_t max_header_line_length, uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
    Parse some item out of a comma list. The comma list MAY include whitespace
    between comma and content.

    This function is used in e.g. @see ov_http_pointer_find_item_in_comma_list
    to walk some comma separated content list and made external for other use
    cases.

    If some ,, is empty or whitepace only, the function will return false,
    otherwise *item and *item_len will be set whitespace clean front and back.

    @param start    start of a buffer
    @param length   length of the buffer
    @param item     pointer to item to be set (content first non whitespace)
    @param item_len length of parsed item of the comma list, without trailing
                    whitespace
    @param next     pointer to next content part of a comma list, will be set
                    to NULL if no more content is contained.

    @returns true if some item with some (non whitespace) content was parsed.
    if no comma is contained, next will become NULL
*/
bool ov_http_pointer_parse_comma_list_item(const uint8_t *start, size_t length,
                                           uint8_t **item, size_t *item_len,
                                           uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
    Parse some comma separated list for "string" of "string_length"

    e.g. like the value of some http header "keep-alive, Upgrade" to search
    for the name "Upgrade"

    @param start        start of a buffer
    @param length       length of the buffer
    @param buffer       string to search (chars maybe > 127)
    @param len          length of the search buffer
    @param case_ignore  ignore the case of the input, the buffer will
                        be evaluated as char* instead of uint8_t*

    @returns pointer to first non http whitespace character of the input buffer,
    which contains the search buffer (will always return first match only)
    @return NULL if the buffer is not contained within a comma content list

    For the example "keep-alive, Upgrade" with the search term "Upgrade"
    the return pointer will point to "Upgrade" and not to the whitespace after
    the comma.
*/
const uint8_t *ov_http_pointer_find_item_in_comma_list(const uint8_t *start,
                                                       size_t length,
                                                       const uint8_t *buffer,
                                                       size_t len,
                                                       bool case_ignore);

/*----------------------------------------------------------------------------*/

/**
    Get some header line, out of some header array.

    @param array    some array of ov_http_header pointers
    @param size     size of the array
    @param name     (mandatory) field name to search

    @returns first occurance of the name within the array
*/
const ov_http_header *ov_http_header_get(const ov_http_header *array,
                                         size_t size, const char *name);

/*----------------------------------------------------------------------------*/

/**
    Get some unique header line, out of some header array.
    This function checks if the header is contained, and if so,
    it MUST be contained once.

    @param array    some array of ov_http_header pointers
    @param size     size of the array
    @param name     (mandatory) field name to search

    @returns first occurance of the name within the array
*/
const ov_http_header *ov_http_header_get_unique(const ov_http_header *array,
                                                size_t size, const char *name);

/*----------------------------------------------------------------------------*/

/**
    Get some header line, out of some header array.

    @param array    some array of ov_http_header pointers
    @param size     size of the array
    @param index    start of search
    @param name     (mandatory) field name to search

    @returns first occurance of the name after index within the array
    will forward index to the the returned header if found
*/
const ov_http_header *ov_http_header_get_next(const ov_http_header *array,
                                              size_t size, size_t *index,
                                              const char *name);

/*----------------------------------------------------------------------------*/

/**
    Parse the actual transfer encodings set within the header out of some
    message.

    @param message  message to check
    @param array    array of memory pointers
    @param size     size of array

    @NOTE This function will parse all occuranced of transfer definition in
    order out of the header, but will NOT check the content or double entries.
    it will only set data pointers to the array with boundaries of identified
    entry strings in comma list and multiheader Transfer-Encoding fields.

    @returns
    OV_HTTP_PARSER_ABSENT if no transfer encoding is used within the header
    OV_HTTP_PARSER_OOB if more tranfer encodings as size are found
    OV_HTTP_PARSER_ERROR on any other error
    OV_HTTP_PARSER_SUCCESS on success
*/
ov_http_parser_state
ov_http_pointer_parse_transfer_encodings(const ov_http_message *message,
                                         ov_memory_pointer *array, size_t size);

/*----------------------------------------------------------------------------*/

/**
    Create a config out of some JSON object.

    Will check for KEY OV_KEY_HTTP_MESSAGE or try to parse the input direct.

    Expected input:

    {
        "buffer":
        {
            "max cache":0,
            "size":0
        },
        "chunk":
        {
            "max":0
        },
        "header":
        {
            "capacity":0,
            "lines":0,
            "method":0
        },
        "transfer":
        {
            "max":0
        }
    }
    @param value    JSON value to parse
*/
ov_http_message_config
ov_http_message_config_from_json(const ov_json_value *value);

/*----------------------------------------------------------------------------*/

/**
    Create the JSON config of form @see ov_http_message_config_from_json
*/
ov_json_value *ov_http_message_config_to_json(ov_http_message_config config);

/*----------------------------------------------------------------------------*/

ov_http_message *ov_http_message_pop(ov_buffer **buffer,
                                     const ov_http_message_config *config,
                                     ov_http_parser_state *state);

#endif /* ov_http_pointer_h */
