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
        @file           ov_websocket_pointer_test.c
        @author         Markus Töpfer

        @date           2020-12-26


        ------------------------------------------------------------------------
*/
#include "ov_websocket_pointer.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_websocket_generate_secure_websocket_key() {

  size_t amount = 100;
  uint8_t *buffer[amount];

  for (size_t i = 0; i < amount; i++) {

    buffer[i] = calloc(OV_WEBSOCKET_SECURE_KEY_SIZE + 1, sizeof(uint8_t));
    testrun(buffer[i]);
    testrun(ov_websocket_generate_secure_websocket_key(
        buffer[i], OV_WEBSOCKET_SECURE_KEY_SIZE));

    for (size_t k = 0; k < i; k++) {

      testrun(0 != memcmp(buffer[k], buffer[i], OV_WEBSOCKET_SECURE_KEY_SIZE));
    }
  }

  testrun(!ov_websocket_generate_secure_websocket_key(NULL, 0));
  testrun(!ov_websocket_generate_secure_websocket_key(buffer[0], 0));
  testrun(!ov_websocket_generate_secure_websocket_key(
      NULL, OV_WEBSOCKET_SECURE_KEY_SIZE));

  testrun(!ov_websocket_generate_secure_websocket_key(
      buffer[0], OV_WEBSOCKET_SECURE_KEY_SIZE - 1));

  testrun(ov_websocket_generate_secure_websocket_key(
      buffer[0], OV_WEBSOCKET_SECURE_KEY_SIZE));

  for (size_t i = 0; i < amount; i++) {
    free(buffer[i]);
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_websocket_generate_secure_accept_key() {

  size_t size = OV_WEBSOCKET_SECURE_KEY_SIZE;
  uint8_t buffer[size];
  memset(buffer, 0, size);

  size_t hash_size = OV_SHA1_SIZE;
  uint8_t hashed[hash_size];
  memset(hashed, 0, hash_size);

  size_t base_size = OV_SHA1_SIZE;
  uint8_t base[base_size];
  memset(base, 0, base_size);

  size_t length = OV_WEBSOCKET_SECURE_KEY_SIZE + GUID_LEN;
  uint8_t expect[length];
  memset(expect, 0, length);

  testrun(ov_websocket_generate_secure_websocket_key(
      buffer, OV_WEBSOCKET_SECURE_KEY_SIZE));

  testrun(memcpy(expect, buffer, OV_WEBSOCKET_SECURE_KEY_SIZE));
  testrun(memcpy(expect + OV_WEBSOCKET_SECURE_KEY_SIZE, GUID, GUID_LEN));

  uint8_t *ptr = hashed;
  testrun(ov_hash_string(OV_HASH_SHA1, expect, length, &ptr, &hash_size));

  uint8_t *key = NULL;
  size_t len = 0;

  testrun(!ov_websocket_generate_secure_accept_key(NULL, 0, NULL, NULL));

  testrun(ov_websocket_generate_secure_accept_key(
      buffer, OV_WEBSOCKET_SECURE_KEY_SIZE, &key, &len));

  testrun(key);
  testrun(len == 28);

  ptr = base;
  testrun(ov_base64_decode((uint8_t *)key, len, &ptr, &base_size));
  testrun(base_size == OV_SHA1_SIZE);

  testrun(0 == memcmp(hashed, base, OV_SHA1_SIZE));

  free(key);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_websocket_process_handshake_request() {

  uint8_t key[OV_WEBSOCKET_SECURE_KEY_SIZE + 1];
  memset(key, 0, OV_WEBSOCKET_SECURE_KEY_SIZE + 1);

  uint8_t *accept_key = NULL;
  size_t accept_key_length = 0;

  testrun(ov_websocket_generate_secure_websocket_key(
      key, OV_WEBSOCKET_SECURE_KEY_SIZE));

  testrun(ov_websocket_generate_secure_accept_key(
      key, OV_WEBSOCKET_SECURE_KEY_SIZE, &accept_key, &accept_key_length));

  ov_http_message_config config = (ov_http_message_config){0};
  ov_http_version version = (ov_http_version){.major = 1, .minor = 1};

  bool is_handshake = false;
  const ov_http_header *header = NULL;

  ov_http_message *out = NULL;
  ov_http_message *in =
      ov_http_create_request_string(config, version, "GET", "/");
  testrun(in);
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_HOST, "host"));
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_UPGRADE,
                                            OV_WEBSOCKET_KEY));
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_CONNECTION,
                                            OV_HTTP_KEY_UPGRADE));
  testrun(ov_http_message_add_header_string(in, OV_WEBSOCKET_KEY_SECURE,
                                            (char *)key));
  testrun(ov_http_message_add_header_string(in, OV_WEBSOCKET_KEY_SECURE_VERSION,
                                            "13"));
  testrun(ov_http_message_close_header(in));
  testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_message(in, NULL));

  testrun(!ov_websocket_process_handshake_request(NULL, NULL, NULL));
  testrun(!ov_websocket_process_handshake_request(in, NULL, &is_handshake));
  testrun(!ov_websocket_process_handshake_request(NULL, &out, &is_handshake));

  testrun(ov_websocket_process_handshake_request(in, &out, &is_handshake));
  testrun(out);
  testrun(is_handshake == true);
  testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_message(out, NULL));
  // check out
  testrun(out->status.code == 101);
  testrun(0 == strncmp(OV_HTTP_SWITCH_PROTOCOLS,
                       (char *)out->status.phrase.start,
                       out->status.phrase.length));
  header = ov_http_header_get_unique(out->header, out->config.header.capacity,
                                     OV_HTTP_KEY_UPGRADE);
  testrun(0 == strncmp(OV_WEBSOCKET_KEY, (char *)header->value.start,
                       header->value.length));
  header = ov_http_header_get_unique(out->header, out->config.header.capacity,
                                     OV_HTTP_KEY_CONNECTION);
  testrun(0 == strncmp(OV_HTTP_KEY_UPGRADE, (char *)header->value.start,
                       header->value.length));
  header = ov_http_header_get_unique(out->header, out->config.header.capacity,
                                     OV_WEBSOCKET_KEY_SECURE_ACCEPT);

  testrun(0 == memcmp(accept_key, header->value.start, header->value.length));

  out = ov_http_message_free(out);
  is_handshake = false;

  testrun(ov_websocket_process_handshake_request(in, &out, NULL));
  testrun(out);
  testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_message(out, NULL));
  // check out
  testrun(out->status.code == 101);
  testrun(0 == strncmp(OV_HTTP_SWITCH_PROTOCOLS,
                       (char *)out->status.phrase.start,
                       out->status.phrase.length));
  header = ov_http_header_get_unique(out->header, out->config.header.capacity,
                                     OV_HTTP_KEY_UPGRADE);
  testrun(0 == strncmp(OV_WEBSOCKET_KEY, (char *)header->value.start,
                       header->value.length));
  header = ov_http_header_get_unique(out->header, out->config.header.capacity,
                                     OV_HTTP_KEY_CONNECTION);
  testrun(0 == strncmp(OV_HTTP_KEY_UPGRADE, (char *)header->value.start,
                       header->value.length));
  header = ov_http_header_get_unique(out->header, out->config.header.capacity,
                                     OV_WEBSOCKET_KEY_SECURE_ACCEPT);
  testrun(0 == memcmp(accept_key, header->value.start, header->value.length));

  out = ov_http_message_free(out);
  is_handshake = false;

  // METHOD not GET
  is_handshake = false;
  in = ov_http_message_free(in);
  in = ov_http_create_request_string(config, version, "PUT", "/");
  testrun(in);
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_HOST, "host"));
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_UPGRADE,
                                            OV_WEBSOCKET_KEY));
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_CONNECTION,
                                            OV_HTTP_KEY_UPGRADE));
  testrun(ov_http_message_add_header_string(in, OV_WEBSOCKET_KEY_SECURE,
                                            (char *)key));
  testrun(ov_http_message_add_header_string(in, OV_WEBSOCKET_KEY_SECURE_VERSION,
                                            "13"));
  testrun(ov_http_message_close_header(in));
  testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_message(in, NULL));
  testrun(!ov_websocket_process_handshake_request(in, &out, &is_handshake));
  testrun(is_handshake == false);
  testrun(NULL == out);

  // NOT upgrade request
  is_handshake = false;
  in = ov_http_message_free(in);
  in = ov_http_create_request_string(config, version, "GET", "/");
  testrun(in);
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_HOST, "host"));
  testrun(
      ov_http_message_add_header_string(in, OV_HTTP_KEY_UPGRADE, "somthing"));
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_CONNECTION,
                                            OV_HTTP_KEY_UPGRADE));
  testrun(ov_http_message_add_header_string(in, OV_WEBSOCKET_KEY_SECURE,
                                            (char *)key));
  testrun(ov_http_message_add_header_string(in, OV_WEBSOCKET_KEY_SECURE_VERSION,
                                            "13"));
  testrun(ov_http_message_close_header(in));
  testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_message(in, NULL));
  testrun(!ov_websocket_process_handshake_request(in, &out, &is_handshake));
  testrun(is_handshake == false);
  testrun(NULL == out);

  // No connection upgrade
  is_handshake = false;
  in = ov_http_message_free(in);
  in = ov_http_create_request_string(config, version, "GET", "/");
  testrun(in);
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_HOST, "host"));
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_UPGRADE,
                                            OV_WEBSOCKET_KEY));
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_CONNECTION,
                                            "something"));
  testrun(ov_http_message_add_header_string(in, OV_WEBSOCKET_KEY_SECURE,
                                            (char *)key));
  testrun(ov_http_message_add_header_string(in, OV_WEBSOCKET_KEY_SECURE_VERSION,
                                            "13"));
  testrun(ov_http_message_close_header(in));
  testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_message(in, NULL));
  testrun(!ov_websocket_process_handshake_request(in, &out, &is_handshake));
  testrun(is_handshake == false);
  testrun(NULL == out);

  // No host
  is_handshake = false;
  in = ov_http_message_free(in);
  in = ov_http_create_request_string(config, version, "GET", "/");
  testrun(in);
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_UPGRADE,
                                            OV_WEBSOCKET_KEY));
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_CONNECTION,
                                            OV_HTTP_KEY_UPGRADE));
  testrun(ov_http_message_add_header_string(in, OV_WEBSOCKET_KEY_SECURE,
                                            (char *)key));
  testrun(ov_http_message_add_header_string(in, OV_WEBSOCKET_KEY_SECURE_VERSION,
                                            "13"));
  testrun(ov_http_message_close_header(in));
  testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_message(in, NULL));
  testrun(!ov_websocket_process_handshake_request(in, &out, &is_handshake));
  testrun(is_handshake == false);
  testrun(NULL == out);

  // No sec_key
  is_handshake = false;
  in = ov_http_message_free(in);
  in = ov_http_create_request_string(config, version, "GET", "/");
  testrun(in);
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_HOST, "host"));
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_UPGRADE,
                                            OV_WEBSOCKET_KEY));
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_CONNECTION,
                                            OV_HTTP_KEY_UPGRADE));
  testrun(ov_http_message_add_header_string(in, OV_WEBSOCKET_KEY_SECURE_VERSION,
                                            "13"));
  testrun(ov_http_message_close_header(in));
  testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_message(in, NULL));
  testrun(!ov_websocket_process_handshake_request(in, &out, &is_handshake));
  testrun(is_handshake == false);
  testrun(NULL == out);

  // No sec_ver
  is_handshake = false;
  in = ov_http_message_free(in);
  in = ov_http_create_request_string(config, version, "GET", "/");
  testrun(in);
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_HOST, "host"));
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_UPGRADE,
                                            OV_WEBSOCKET_KEY));
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_CONNECTION,
                                            OV_HTTP_KEY_UPGRADE));
  testrun(ov_http_message_add_header_string(in, OV_WEBSOCKET_KEY_SECURE,
                                            (char *)key));
  testrun(ov_http_message_close_header(in));
  testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_message(in, NULL));
  testrun(!ov_websocket_process_handshake_request(in, &out, &is_handshake));
  testrun(is_handshake == false);
  testrun(NULL == out);

  // Wrong secure version
  is_handshake = false;
  in = ov_http_message_free(in);
  in = ov_http_create_request_string(config, version, "GET", "/");
  testrun(in);
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_HOST, "host"));
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_UPGRADE,
                                            OV_WEBSOCKET_KEY));
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_CONNECTION,
                                            OV_HTTP_KEY_UPGRADE));
  testrun(ov_http_message_add_header_string(in, OV_WEBSOCKET_KEY_SECURE,
                                            (char *)key));
  testrun(ov_http_message_add_header_string(in, OV_WEBSOCKET_KEY_SECURE_VERSION,
                                            "12"));
  testrun(ov_http_message_close_header(in));
  testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_message(in, NULL));
  testrun(!ov_websocket_process_handshake_request(in, &out, &is_handshake));
  testrun(is_handshake == false);
  testrun(NULL != out);
  testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_message(out, NULL));
  // check out
  testrun(out->status.code == 426);
  testrun(0 == strncmp(OV_HTTP_UPGRADE_REQUIRED,
                       (char *)out->status.phrase.start,
                       out->status.phrase.length));
  header = ov_http_header_get_unique(out->header, out->config.header.capacity,
                                     OV_WEBSOCKET_KEY_SECURE_VERSION);
  testrun(0 ==
          strncmp("13", (char *)header->value.start, header->value.length));
  out = ov_http_message_free(out);

  // secure key not 24 bytes
  char *wrong_key = "key";
  is_handshake = false;
  in = ov_http_message_free(in);
  in = ov_http_create_request_string(config, version, "GET", "/");
  testrun(in);
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_HOST, "host"));
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_UPGRADE,
                                            OV_WEBSOCKET_KEY));
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_CONNECTION,
                                            OV_HTTP_KEY_UPGRADE));
  testrun(ov_http_message_add_header_string(in, OV_WEBSOCKET_KEY_SECURE,
                                            wrong_key));
  testrun(ov_http_message_add_header_string(in, OV_WEBSOCKET_KEY_SECURE_VERSION,
                                            "13"));
  testrun(ov_http_message_close_header(in));
  testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_message(in, NULL));
  testrun(!ov_websocket_process_handshake_request(in, &out, &is_handshake));
  testrun(is_handshake == false);
  testrun(NULL == out);

  // upgrade in comma list
  is_handshake = false;
  in = ov_http_message_free(in);
  in = ov_http_create_request_string(config, version, "GET", "/");
  testrun(in);
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_HOST, "host"));
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_UPGRADE,
                                            OV_WEBSOCKET_KEY));
  testrun(ov_http_message_add_header_string(in, OV_HTTP_KEY_CONNECTION,
                                            "x, y, z,  upgrade , whatever "));
  testrun(ov_http_message_add_header_string(in, OV_WEBSOCKET_KEY_SECURE,
                                            (char *)key));
  testrun(ov_http_message_add_header_string(in, OV_WEBSOCKET_KEY_SECURE_VERSION,
                                            "13"));
  testrun(ov_http_message_close_header(in));
  testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_message(in, NULL));
  testrun(ov_websocket_process_handshake_request(in, &out, &is_handshake));
  testrun(is_handshake == true);
  testrun(NULL != out);
  testrun(OV_HTTP_PARSER_SUCCESS == ov_http_pointer_parse_message(out, NULL));
  // check out
  testrun(out->status.code == 101);
  testrun(0 == strncmp(OV_HTTP_SWITCH_PROTOCOLS,
                       (char *)out->status.phrase.start,
                       out->status.phrase.length));
  header = ov_http_header_get_unique(out->header, out->config.header.capacity,
                                     OV_HTTP_KEY_UPGRADE);
  testrun(0 == strncmp(OV_WEBSOCKET_KEY, (char *)header->value.start,
                       header->value.length));
  header = ov_http_header_get_unique(out->header, out->config.header.capacity,
                                     OV_HTTP_KEY_CONNECTION);
  testrun(0 == strncmp(OV_HTTP_KEY_UPGRADE, (char *)header->value.start,
                       header->value.length));
  header = ov_http_header_get_unique(out->header, out->config.header.capacity,
                                     OV_WEBSOCKET_KEY_SECURE_ACCEPT);
  testrun(0 == memcmp(accept_key, header->value.start, header->value.length));

  out = ov_http_message_free(out);

  testrun(NULL == ov_http_message_free(in));
  testrun(NULL == ov_http_message_free(out));

  accept_key = ov_data_pointer_free(accept_key);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_fragmentation_state() {

  uint8_t buffer = 0;

  bool fin = false;
  bool opcode = false;

  ov_websocket_fragmentation_state state;

  for (size_t i = 0; i < 0xff; i++) {

    buffer = i;

    fin = false;
    opcode = false;

    if (buffer & 0x80)
      fin = true;

    if (buffer & 0x0F)
      opcode = true;

    state = fragmentation_state(buffer);

    if (fin && opcode) {
      testrun(state == OV_WEBSOCKET_FRAGMENTATION_NONE);
    } else if (fin) {
      testrun(state == OV_WEBSOCKET_FRAGMENTATION_LAST);
    } else if (opcode) {
      testrun(state == OV_WEBSOCKET_FRAGMENTATION_START);
    } else {
      testrun(state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
    }
  }

  testrun(OV_WEBSOCKET_FRAGMENTATION_CONTINUE == fragmentation_state(0));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_parse_payload_length() {

  uint8_t *next = NULL;

  ov_websocket_frame *frame = ov_websocket_frame_create(
      (ov_websocket_frame_config){.buffer.default_size = 0xFffff});

  testrun(frame);
  testrun(frame->buffer);
  testrun(frame->buffer->capacity == 0xFffff);

  // set min valid for OK
  frame->buffer->start[1] = 0;
  frame->buffer->length = 2;

  testrun(OV_WEBSOCKET_PARSER_ERROR == parse_payload_length(NULL, NULL));
  testrun(OV_WEBSOCKET_PARSER_ERROR == parse_payload_length(NULL, &next));

  // parse without next
  testrun(OV_WEBSOCKET_PARSER_SUCCESS == parse_payload_length(frame, NULL));
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);
  // parse with next
  testrun(OV_WEBSOCKET_PARSER_SUCCESS == parse_payload_length(frame, &next));
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);
  testrun(next == frame->buffer->start + frame->buffer->length);

  // reset
  memset(frame->buffer->start, 0, frame->buffer->capacity);
  frame->buffer->length = 0;

  memset(frame->buffer->start, 'A', 150);

  // check length < 126 without masking
  for (size_t i = 0; i < 126; i++) {

    frame->buffer->start[1] = i;
    for (size_t x = 0; x < i + 1; x++) {

      frame->buffer->length = x;
      testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
              parse_payload_length(frame, NULL));
    }

    frame->buffer->length = i + 2;
    testrun(OV_WEBSOCKET_PARSER_SUCCESS == parse_payload_length(frame, NULL));
  }

  // check length < 126 with masking
  for (size_t i = 0; i < 126; i++) {

    frame->buffer->start[1] = i | 0x80;

    for (size_t x = 0; x < i + 6; x++) {

      frame->buffer->length = x;

      if (i == 0) {

        if (x < 2) {
          testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
                  parse_payload_length(frame, NULL));
        } else {
          // Zero content but masking set
          testrun(OV_WEBSOCKET_PARSER_ERROR ==
                  parse_payload_length(frame, NULL));
        }

      } else {
        testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
                parse_payload_length(frame, NULL));
      }
    }

    frame->buffer->length = i + 6;
    if (i == 0) {
      // Zero content but masking set
      testrun(OV_WEBSOCKET_PARSER_ERROR == parse_payload_length(frame, NULL));
    } else {
      testrun(OV_WEBSOCKET_PARSER_SUCCESS == parse_payload_length(frame, NULL));
    }
  }

  // check next we start at 1 to avoid testing for 0 with mask (tested above)
  for (size_t i = 1; i < 126; i++) {

    frame->buffer->start[1] = i;
    next = NULL;

    for (size_t x = 0; x < i + 1; x++) {

      frame->buffer->length = x;
      testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
              parse_payload_length(frame, &next));
      testrun(next == NULL);
    }

    frame->buffer->length = i + 2;
    testrun(OV_WEBSOCKET_PARSER_SUCCESS == parse_payload_length(frame, &next));
    testrun(next == frame->buffer->start + i + 2);

    // check with masking
    frame->buffer->start[1] = i | 0x80;

    next = NULL;

    for (size_t x = 0; x < i + 6; x++) {

      frame->buffer->length = x;

      testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
              parse_payload_length(frame, &next));
      testrun(next == NULL);
    }

    frame->buffer->length = i + 6;
    testrun(OV_WEBSOCKET_PARSER_SUCCESS == parse_payload_length(frame, &next));
    testrun(next == frame->buffer->start + i + 6);
  }

  // check 1byte length max
  next = NULL;
  frame->buffer->start[127] = 'B';
  frame->buffer->length = 135;
  frame->buffer->start[1] = 125;
  frame->buffer->length = 127;
  testrun(OV_WEBSOCKET_PARSER_SUCCESS == parse_payload_length(frame, &next));
  testrun(next = frame->buffer->start + 127);
  testrun(*next == 'B');
  testrun(*(next - 1) == 'A');
  testrun(*(next + 1) == 'A');

  // check max 1byte length with mask
  next = NULL;
  frame->buffer->start[127] = 'A';
  frame->buffer->start[131] = 'B';
  frame->buffer->start[132] = 'C';
  frame->buffer->start[1] = 125 | 0x80;
  frame->buffer->length = 131;
  testrun(OV_WEBSOCKET_PARSER_SUCCESS == parse_payload_length(frame, &next));
  testrun(next = frame->buffer->start + 131);
  testrun(*next == 'B');
  testrun(*(next - 1) == 'A');
  testrun(*(next + 1) == 'C');

  // reset
  memset(frame->buffer->start, 'A', frame->buffer->capacity);
  frame->buffer->length = 0;

  // check with length byte 126 (masking and non masking)
  for (size_t i = 126; i < 0xffff; i++) {

    frame->buffer->start[1] = 126;
    frame->buffer->start[2] = i >> 8;
    frame->buffer->start[3] = i;

    next = NULL;

    // we test from i to limit the runtime of the test
    for (size_t x = i; x < i + 3; x++) {

      frame->buffer->length = x;
      testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
              parse_payload_length(frame, &next));
      testrun(next == NULL);
    }

    frame->buffer->length = i + 4;
    testrun(OV_WEBSOCKET_PARSER_SUCCESS == parse_payload_length(frame, &next));
    testrun(next == frame->buffer->start + i + 4);

    // check with masking
    next = NULL;

    frame->buffer->start[1] = 126 | 0x80;

    // we test from i to limit the runtime of the test
    for (size_t x = i; x < i + 7; x++) {

      frame->buffer->length = x;
      testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
              parse_payload_length(frame, &next));
      testrun(next == NULL);
    }

    frame->buffer->length = i + 8;
    testrun(OV_WEBSOCKET_PARSER_SUCCESS == parse_payload_length(frame, &next));
    testrun(next == frame->buffer->start + i + 8);
  }

  // check with length byte 127 (masking and non masking)

  // NOTE limited size due to linux create error for buffer of uin64_t
  for (size_t i = 0xF000; i < 0xFffff; i += 0x10001) {

    frame->buffer->start[1] = 127;
    frame->buffer->start[2] = i >> 56;
    frame->buffer->start[3] = i >> 48;
    frame->buffer->start[4] = i >> 40;
    frame->buffer->start[5] = i >> 32;
    frame->buffer->start[6] = i >> 24;
    frame->buffer->start[7] = i >> 16;
    frame->buffer->start[8] = i >> 8;
    frame->buffer->start[9] = i;

    if (i < 0x10000) {
      testrun(OV_WEBSOCKET_PARSER_ERROR == parse_payload_length(frame, &next));
      continue;
    }

    if (frame->buffer->start[2] & 0x80) {
      testrun(OV_WEBSOCKET_PARSER_ERROR == parse_payload_length(frame, &next));
      continue;
    }

    next = NULL;

    // we test from i to limit the runtime of the test
    for (size_t x = i; x < i + 10; x++) {

      frame->buffer->length = x;
      testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
              parse_payload_length(frame, &next));
      testrun(next == NULL);
    }

    frame->buffer->length = i + 10;
    testrun(OV_WEBSOCKET_PARSER_SUCCESS == parse_payload_length(frame, &next));
    testrun(next == frame->buffer->start + i + 10);

    // check with masking
    next = NULL;

    frame->buffer->start[1] = 127 | 0x80;

    // we test from i to limit the runtime of the test
    for (size_t x = i; x < i + 14; x++) {

      frame->buffer->length = x;
      testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
              parse_payload_length(frame, &next));
      testrun(next == NULL);
    }

    frame->buffer->length = i + 14;
    testrun(OV_WEBSOCKET_PARSER_SUCCESS == parse_payload_length(frame, &next));
    testrun(next == frame->buffer->start + i + 14);
  }

  // check edge case with length byte 127 (masking and non masking)

  // NOTE this test increases the test runtime quite significiant,
  for (size_t i = 0xF0000; i < 0xFffff; i += 0x10001) {

    frame->buffer->start[1] = 127;
    frame->buffer->start[2] = i >> 56;
    frame->buffer->start[3] = i >> 48;
    frame->buffer->start[4] = i >> 40;
    frame->buffer->start[5] = i >> 32;
    frame->buffer->start[6] = i >> 24;
    frame->buffer->start[7] = i >> 16;
    frame->buffer->start[8] = i >> 8;
    frame->buffer->start[9] = i;

    // 64-bit unsigned integer (the most significant bit MUST be 0)
    if (frame->buffer->start[2] & 0x80) {
      testrun(OV_WEBSOCKET_PARSER_ERROR == parse_payload_length(frame, &next));
      continue;
    }

    next = NULL;

    // we test from i to limit the runtime of the test
    for (size_t x = i; x < i + 10; x++) {

      frame->buffer->length = x;
      testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
              parse_payload_length(frame, &next));
      testrun(next == NULL);
    }

    frame->buffer->length = i + 10;
    testrun(OV_WEBSOCKET_PARSER_SUCCESS == parse_payload_length(frame, &next));
    testrun(next == frame->buffer->start + i + 10);

    // check with masking
    next = NULL;

    frame->buffer->start[1] = 127 | 0x80;

    // we test from i to limit the runtime of the test
    for (size_t x = i; x < i + 14; x++) {

      frame->buffer->length = x;
      testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
              parse_payload_length(frame, &next));
      testrun(next == NULL);
    }

    frame->buffer->length = i + 14;
    testrun(OV_WEBSOCKET_PARSER_SUCCESS == parse_payload_length(frame, &next));
    testrun(next == frame->buffer->start + i + 14);
  }

  testrun(NULL == ov_websocket_frame_free(frame));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_websocket_frame_cast() {

  for (uint16_t i = 0; i < 0xFFFF; i++) {

    if (i == OV_WEBSOCKET_MAGIC_BYTE) {
      testrun(ov_websocket_frame_cast(&i));
      testrun(!ov_http_message_cast(&i));
    } else {
      testrun(!ov_websocket_frame_cast(&i));
    }
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_websocket_frame_create() {

  ov_websocket_frame_config config = {0};
  ov_websocket_frame *frame = ov_websocket_frame_create(config);
  testrun(frame);
  testrun(ov_websocket_frame_cast(frame));
  testrun(frame->buffer);
  testrun(frame->config.buffer.default_size ==
          OV_WEBSOCKET_FRAME_DEFAULT_BUFFER_SIZE);
  testrun(frame->buffer->capacity == OV_WEBSOCKET_FRAME_DEFAULT_BUFFER_SIZE);
  frame = ov_websocket_frame_free(frame);

  // check with caching
  ov_websocket_enable_caching(1);
  testrun(g_cache != NULL);

  ov_websocket_frame *frame1 = NULL;
  ov_websocket_frame *frame2 = NULL;
  ov_websocket_frame *frame3 = NULL;
  ov_websocket_frame *frame4 = NULL;

  frame = ov_websocket_frame_create(config);
  testrun(frame);
  testrun(ov_websocket_frame_cast(frame));

  frame2 = ov_websocket_frame_create(config);
  testrun(frame2);
  testrun(ov_websocket_frame_cast(frame2));

  ov_websocket_frame_free(frame);
  frame3 = ov_websocket_frame_create(config);
  testrun(frame3);
  testrun(ov_websocket_frame_cast(frame3));
  testrun(frame3 == frame);

  ov_websocket_frame_free(frame2);
  frame4 = ov_websocket_frame_create(config);
  testrun(frame4);
  testrun(ov_websocket_frame_cast(frame4));
  testrun(frame4 == frame2);

  ov_websocket_frame_free(frame3);
  ov_websocket_frame_free(frame4);

  // we check what we get from cache
  frame3 = ov_registered_cache_get(g_cache);
  testrun(frame3);
  testrun(frame3 == frame);
  frame4 = ov_registered_cache_get(g_cache);
  testrun(!frame4);

  frame3 = ov_websocket_frame_free_uncached(frame3);
  testrun(!ov_registered_cache_get(g_cache));

  frame = NULL;
  frame1 = NULL;
  frame2 = NULL;
  frame3 = NULL;

  config = (ov_websocket_frame_config){.buffer.default_size = 100};

  frame = ov_websocket_frame_create(config);
  testrun(frame);
  testrun(frame->buffer->capacity == 100);

  ov_websocket_frame_free(frame);

  // we request a bigger size
  config = (ov_websocket_frame_config){.buffer.default_size = 200};

  frame1 = ov_websocket_frame_create(config);
  testrun(frame1);
  testrun(frame1 == frame);
  testrun(frame->buffer == frame1->buffer);
  testrun(frame1->buffer->capacity == 200);

  ov_websocket_frame_free(frame1);

  // we request a smaller buffer size

  config = (ov_websocket_frame_config){.buffer.default_size = 50};

  frame1 = ov_websocket_frame_create(config);
  testrun(frame1);
  testrun(frame1 == frame);
  testrun(frame->buffer == frame1->buffer);
  testrun(frame1->buffer->capacity == 50);

  frame1->buffer = ov_buffer_free_uncached(frame1->buffer);
  ov_websocket_frame_free(frame1);
  frame2 = ov_websocket_frame_create(config);
  testrun(frame2);
  testrun(frame2 == frame1);
  testrun(frame2->buffer != NULL);
  testrun(frame2->buffer->capacity == 50);

  frame = ov_websocket_frame_free_uncached(frame);
  ov_registered_cache_free_all();
  g_cache = NULL;

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_websocket_frame_clear() {

  ov_websocket_frame_config config = {0};
  ov_websocket_frame *frame = ov_websocket_frame_create(config);
  testrun(frame);
  testrun(ov_websocket_frame_cast(frame));
  testrun(frame->buffer);
  testrun(frame->config.buffer.default_size ==
          OV_WEBSOCKET_FRAME_DEFAULT_BUFFER_SIZE);
  testrun(frame->buffer->capacity == OV_WEBSOCKET_FRAME_DEFAULT_BUFFER_SIZE);

  testrun(!ov_websocket_frame_clear(NULL));
  testrun(ov_websocket_frame_clear(frame));

  testrun(frame->buffer->capacity == OV_WEBSOCKET_FRAME_DEFAULT_BUFFER_SIZE);
  testrun(frame->buffer->length == 0);
  testrun(frame->opcode == 0);
  testrun(frame->state == 0);
  testrun(frame->mask == NULL);
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);

  char *content = "some content";
  testrun(ov_buffer_set(frame->buffer, content, strlen(content)));
  testrun(frame->buffer->start[0] != 0);
  testrun(frame->buffer->length != 0);
  frame->opcode = OV_WEBSOCKET_OPCODE_CLOSE;
  frame->mask = frame->buffer->start;
  frame->content.start = frame->buffer->start;
  frame->content.length = frame->buffer->length;

  testrun(ov_websocket_frame_clear(frame));

  testrun(frame->buffer->capacity == OV_WEBSOCKET_FRAME_DEFAULT_BUFFER_SIZE);
  testrun(frame->buffer->length == 0);
  testrun(frame->opcode == 0);
  testrun(frame->state == 0);
  testrun(frame->mask == NULL);
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);

  // check with max_recache < buffer->capacity
  frame->config.buffer.max_bytes_recache =
      OV_WEBSOCKET_FRAME_DEFAULT_BUFFER_SIZE;

  testrun(ov_websocket_frame_clear(frame));

  testrun(frame->buffer->capacity == OV_WEBSOCKET_FRAME_DEFAULT_BUFFER_SIZE);
  testrun(frame->buffer->length == 0);
  testrun(frame->opcode == 0);
  testrun(frame->state == 0);
  testrun(frame->mask == NULL);
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);

  // check with max_recache > buffer->capacity
  frame->config.buffer.max_bytes_recache =
      OV_WEBSOCKET_FRAME_DEFAULT_BUFFER_SIZE - 1;

  testrun(ov_websocket_frame_clear(frame));

  testrun(frame->buffer != NULL);
  testrun(frame->opcode == 0);
  testrun(frame->state == 0);
  testrun(frame->mask == NULL);
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);

  testrun(NULL == ov_websocket_frame_free(frame));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_websocket_frame_free() {

  ov_websocket_frame_config config = {0};
  ov_websocket_frame *frame = ov_websocket_frame_create(config);
  testrun(frame);

  testrun(NULL == ov_websocket_frame_free(NULL));
  testrun(NULL == ov_websocket_frame_free(frame));

  // check with caching
  ov_websocket_enable_caching(1);
  testrun(g_cache != NULL);

  frame = ov_websocket_frame_create(config);
  testrun(frame);
  testrun(NULL == ov_websocket_frame_free(frame));

  void *data = ov_registered_cache_get(g_cache);
  testrun(data);
  testrun(data == frame);

  testrun(NULL == ov_websocket_frame_free_uncached(frame));
  testrun(!ov_registered_cache_get(g_cache));

  ov_registered_cache_free_all();
  g_cache = NULL;

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_websocket_frame_free_uncached() {

  ov_websocket_frame_config config = {0};
  ov_websocket_frame *frame = ov_websocket_frame_create(config);
  testrun(frame);

  testrun(NULL == ov_websocket_frame_free_uncached(NULL));
  testrun(NULL == ov_websocket_frame_free_uncached(frame));

  // check with caching
  ov_websocket_enable_caching(1);
  testrun(g_cache != NULL);

  frame = ov_websocket_frame_create(config);
  testrun(frame);
  testrun(NULL == ov_websocket_frame_free_uncached(frame));

  testrun(!ov_registered_cache_get(g_cache));
  ov_registered_cache_free_all();
  g_cache = NULL;

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_websocket_parse_frame() {

  uint8_t *next = NULL;
  ov_websocket_frame_config config = {.buffer.default_size = 0xffFFFF};
  ov_websocket_frame *frame = ov_websocket_frame_create(config);
  testrun(frame);

  testrun(OV_WEBSOCKET_PARSER_ERROR == ov_websocket_parse_frame(NULL, NULL));
  testrun(OV_WEBSOCKET_PARSER_ERROR == ov_websocket_parse_frame(NULL, &next));
  testrun(OV_WEBSOCKET_PARSER_ERROR == ov_websocket_parse_frame(frame, NULL));

  frame->buffer->length = 1;
  testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
          ov_websocket_parse_frame(frame, NULL));

  // check length of 0 with complete structure
  frame->buffer->length = 2;
  testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
          ov_websocket_parse_frame(frame, &next));
  testrun(next == frame->buffer->start + 2);
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);
  testrun(frame->mask == NULL);
  testrun(frame->opcode == OV_WEBSOCKET_OPCODE_CONTINUATION);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  for (size_t i = 1; i < 2; i++) {
    next = NULL;
    frame->buffer->length = i;
    testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
            ov_websocket_parse_frame(frame, &next));
    testrun(next == NULL);
  }

  for (size_t i = 2; i < 10; i++) {
    frame->buffer->length = i;
    next = NULL;
    testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
            ov_websocket_parse_frame(frame, &next));
    testrun(next == frame->buffer->start + 2);
  }

  frame->buffer->length = 2;

  // fin no set opcode not set
  next = NULL;
  frame->buffer->start[0] = 0;
  testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
          ov_websocket_parse_frame(frame, &next));
  testrun(next == frame->buffer->start + 2);
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);
  testrun(frame->mask == NULL);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  testrun(frame->opcode == OV_WEBSOCKET_OPCODE_CONTINUATION);

  // fin set
  next = NULL;
  frame->buffer->start[0] = 0x80;
  testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
          ov_websocket_parse_frame(frame, &next));
  testrun(next == frame->buffer->start + 2);
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);
  testrun(frame->mask == NULL);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_LAST);
  testrun(frame->opcode == OV_WEBSOCKET_OPCODE_CONTINUATION);

  // fin set opcode set
  next = NULL;
  frame->buffer->start[0] = 0x80 | OV_WEBSOCKET_OPCODE_TEXT;
  testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
          ov_websocket_parse_frame(frame, &next));
  testrun(next == frame->buffer->start + 2);
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);
  testrun(frame->mask == NULL);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_NONE);
  testrun(frame->opcode == OV_WEBSOCKET_OPCODE_TEXT);

  // fin set opcode set
  next = NULL;
  frame->buffer->start[0] = 0x80 | OV_WEBSOCKET_OPCODE_BINARY;
  testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
          ov_websocket_parse_frame(frame, &next));
  testrun(next == frame->buffer->start + 2);
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);
  testrun(frame->mask == NULL);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_NONE);
  testrun(frame->opcode == OV_WEBSOCKET_OPCODE_BINARY);

  // fin set opcode set
  next = NULL;
  frame->buffer->start[0] = 0x80 | OV_WEBSOCKET_OPCODE_PING;
  testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
          ov_websocket_parse_frame(frame, &next));
  testrun(next == frame->buffer->start + 2);
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);
  testrun(frame->mask == NULL);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_NONE);
  testrun(frame->opcode == OV_WEBSOCKET_OPCODE_PING);

  // fin no set opcode set
  next = NULL;
  frame->buffer->start[0] = OV_WEBSOCKET_OPCODE_PONG;
  testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
          ov_websocket_parse_frame(frame, &next));
  testrun(next == frame->buffer->start + 2);
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);
  testrun(frame->mask == NULL);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_START);
  testrun(frame->opcode == OV_WEBSOCKET_OPCODE_PONG);

  // fin no set opcode set
  next = NULL;
  frame->buffer->start[0] = OV_WEBSOCKET_OPCODE_CLOSE;
  testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
          ov_websocket_parse_frame(frame, &next));
  testrun(next == frame->buffer->start + 2);
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);
  testrun(frame->mask == NULL);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_START);
  testrun(frame->opcode == OV_WEBSOCKET_OPCODE_CLOSE);

  // mask set no content
  next = NULL;
  frame->buffer->start[0] = 0;
  frame->buffer->start[1] = 0x80;
  testrun(OV_WEBSOCKET_PARSER_ERROR == ov_websocket_parse_frame(frame, &next));

  // mask set no content
  next = NULL;
  frame->buffer->start[0] = OV_WEBSOCKET_OPCODE_CLOSE;
  frame->buffer->start[1] = 0x80;
  testrun(OV_WEBSOCKET_PARSER_ERROR == ov_websocket_parse_frame(frame, &next));

  // check RSV ignore (FIN set OPCODE wrong)
  next = NULL;
  frame->buffer->start[0] = 0xFF;
  frame->buffer->start[1] = 0x00;
  testrun(OV_WEBSOCKET_PARSER_ERROR == ov_websocket_parse_frame(frame, &next));

  // check first byte all
  for (int i = 0; i <= 0xff; i++) {

    next = NULL;
    frame->buffer->start[0] = i;

    switch (0x0F & i) {

    case OV_WEBSOCKET_OPCODE_CONTINUATION:
    case OV_WEBSOCKET_OPCODE_TEXT:
    case OV_WEBSOCKET_OPCODE_BINARY:
    case OV_WEBSOCKET_OPCODE_CLOSE:
    case OV_WEBSOCKET_OPCODE_PING:
    case OV_WEBSOCKET_OPCODE_PONG:
      testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
              ov_websocket_parse_frame(frame, &next));
      break;
    default:
      testrun(OV_WEBSOCKET_PARSER_ERROR ==
              ov_websocket_parse_frame(frame, &next));
    }
  }

  // check size

  // valid 1st byte length < 125
  frame->buffer->start[0] = 0x80 | OV_WEBSOCKET_OPCODE_TEXT;

  for (size_t i = 1; i < 126; i++) {

    frame->buffer->start[1] = i;

    for (size_t x = i; x < 10; x++) {

      frame->buffer->length = x;
      next = NULL;

      if (x < (i + 2)) {
        testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
                ov_websocket_parse_frame(frame, &next));
      } else {
        testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
                ov_websocket_parse_frame(frame, &next));
        testrun(next == frame->buffer->start + i + 2);
        testrun(frame->content.start == frame->buffer->start + 2);
        testrun(frame->content.length == i);
        testrun(frame->mask == NULL);
        testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_NONE);
        testrun(frame->opcode == OV_WEBSOCKET_OPCODE_TEXT);
      }
    }
  }

  // valid 1st byte length > 125
  frame->buffer->start[0] = 0x80 | OV_WEBSOCKET_OPCODE_TEXT;

  for (size_t i = 126; i < 0xffff; i += 100) {

    frame->buffer->start[1] = 126;
    frame->buffer->start[2] = i >> 8;
    frame->buffer->start[3] = i;

    for (size_t x = i; x < 10; x++) {

      frame->buffer->length = x;
      next = NULL;

      if (x < (i + 4)) {
        testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
                ov_websocket_parse_frame(frame, &next));
      } else {
        testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
                ov_websocket_parse_frame(frame, &next));
        testrun(next == frame->buffer->start + i + 4);
        testrun(frame->content.start == frame->buffer->start + 4);
        testrun(frame->content.length == i);
        testrun(frame->mask == NULL);
        testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_NONE);
        testrun(frame->opcode == OV_WEBSOCKET_OPCODE_TEXT);
      }
    }

    // set masking
    frame->buffer->start[2] |= 0x80;

    for (size_t x = i; x < 10; x++) {

      frame->buffer->length = x;
      next = NULL;

      if (x < (i + 8)) {
        testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
                ov_websocket_parse_frame(frame, &next));
      } else {
        testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
                ov_websocket_parse_frame(frame, &next));
        testrun(next == frame->buffer->start + i + 8);
        testrun(frame->content.start == frame->buffer->start + 8);
        testrun(frame->content.length == i);
        testrun(frame->mask == frame->buffer->start + i + 4);
        testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_NONE);
        testrun(frame->opcode == OV_WEBSOCKET_OPCODE_TEXT);
      }
    }
  }

  // valid 1st byte length > 125 (edge case)
  frame->buffer->start[0] = 0x80 | OV_WEBSOCKET_OPCODE_TEXT;
  frame->buffer->start[1] = 126;
  frame->buffer->start[2] = 0xFF;
  frame->buffer->start[3] = 0XFF;

  for (size_t x = 0; x < 10; x++) {

    frame->buffer->length = 0xFFFF + x;
    next = NULL;

    if (x < 4) {
      testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
              ov_websocket_parse_frame(frame, &next));
    } else {
      testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
              ov_websocket_parse_frame(frame, &next));
      testrun(next == frame->buffer->start + 0xFFFF + 4);
      testrun(frame->content.start == frame->buffer->start + 4);
      testrun(frame->content.length == 0xFFFF);
      testrun(frame->mask == NULL);
      testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_NONE);
      testrun(frame->opcode == OV_WEBSOCKET_OPCODE_TEXT);
    }
  }

  // set masking
  frame->buffer->start[1] |= 0x80;

  for (size_t x = 0; x < 10; x++) {

    frame->buffer->length = 0xFFFF + x;
    next = NULL;

    if (x < 8) {
      testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
              ov_websocket_parse_frame(frame, &next));
    } else {
      testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
              ov_websocket_parse_frame(frame, &next));
      testrun(next == frame->buffer->start + 0xFFFF + 8);
      testrun(frame->content.start == frame->buffer->start + 8);
      testrun(frame->content.length == 0xFFFF);
      testrun(frame->mask == frame->buffer->start + 4);
      testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_NONE);
      testrun(frame->opcode == OV_WEBSOCKET_OPCODE_TEXT);
    }
  }

  // valid 1st byte length > 0xFFFF
  frame->buffer->start[0] = 0x80 | OV_WEBSOCKET_OPCODE_TEXT;

  for (uint64_t i = 0xffff; i < 0xfFFFF; i += 0x10001) {

    frame->buffer->start[1] = 127;
    frame->buffer->start[2] = i >> 56;
    frame->buffer->start[3] = i >> 48;
    frame->buffer->start[4] = i >> 40;
    frame->buffer->start[5] = i >> 32;
    frame->buffer->start[6] = i >> 24;
    frame->buffer->start[7] = i >> 16;
    frame->buffer->start[8] = i >> 8;
    frame->buffer->start[9] = i;

    for (size_t x = i + 6; x < 12; x++) {

      frame->buffer->length = x;
      next = NULL;

      if (x < (i + 10)) {
        testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
                ov_websocket_parse_frame(frame, &next));
      } else {
        testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
                ov_websocket_parse_frame(frame, &next));
        testrun(next == frame->buffer->start + i + 10);
        testrun(frame->content.start == frame->buffer->start + 10);
        testrun(frame->content.length == i);
        testrun(frame->mask == NULL);
        testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_NONE);
        testrun(frame->opcode == OV_WEBSOCKET_OPCODE_TEXT);
      }
    }

    // set masking
    frame->buffer->start[2] |= 0x80;

    for (size_t x = i + 6; x < 16; x++) {

      frame->buffer->length = x;
      next = NULL;

      if (x < (i + 14)) {
        testrun(OV_WEBSOCKET_PARSER_PROGRESS ==
                ov_websocket_parse_frame(frame, &next));
      } else {
        testrun(OV_WEBSOCKET_PARSER_SUCCESS ==
                ov_websocket_parse_frame(frame, &next));
        testrun(next == frame->buffer->start + i + 14);
        testrun(frame->content.start == frame->buffer->start + 14);
        testrun(frame->content.length == i);
        testrun(frame->mask == frame->buffer->start + i + 10);
        testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_NONE);
        testrun(frame->opcode == OV_WEBSOCKET_OPCODE_TEXT);
      }
    }
  }

  testrun(NULL == ov_websocket_frame_free(frame));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_generate_masking_key() {

  uint8_t empty[5] = {0};

  uint8_t *array[20] = {0};
  for (size_t i = 0; i < 20; i++) {
    array[i] = calloc(1, 5);
  }

  testrun(!generate_masking_key(NULL, 0));
  testrun(!generate_masking_key(array[0], 0));
  testrun(!generate_masking_key(array[0], 1));
  testrun(!generate_masking_key(array[0], 2));
  testrun(!generate_masking_key(array[0], 3));
  testrun(!generate_masking_key(NULL, 4));

  for (size_t i = 0; i < 20; i++) {
    testrun(generate_masking_key(array[i], 5));
    testrun(0 != memcmp(array[i], &empty, 5));
  }

  for (size_t i = 0; i < 20; i++) {

    for (size_t x = 0; x < 20; x++) {

      if (x == i)
        continue;

      testrun(0 != memcmp(array[i], array[x], 5));
    }
  }

  for (size_t i = 0; i < 20; i++) {
    array[i] = ov_data_pointer_free(array[i]);
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_mask_data() {

  uint8_t mask[5] = {0};
  uint8_t data[100] = {0};
  uint8_t source[100] = {0};

  testrun(generate_masking_key(mask, 5));
  testrun(ov_random_bytes(data, 100));
  memcpy(&source, &data, 100);

  testrun(0 == memcmp(&source, &data, 100));

  testrun(!mask_data(NULL, 0, NULL));
  testrun(!mask_data(NULL, 100, mask));
  testrun(!mask_data(data, 0, mask));
  testrun(!mask_data(data, 100, NULL));

  testrun(mask_data(data, 100, mask));

  uint8_t j = 0;

  for (size_t i = 0; i < 100; i++) {
    j = i % 4;
    data[i] = source[i] ^ mask[j];
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_websocket_set_data() {

  ov_websocket_frame_config config = {.buffer.default_size = 10};

  ov_websocket_frame *frame = ov_websocket_frame_create(config);
  testrun(frame);
  testrun(frame->buffer->capacity == 10);

  char *data = "test";
  testrun(!ov_websocket_set_data(NULL, NULL, 0, false));

  // basically some reset of the content
  testrun(ov_websocket_set_data(frame, NULL, 0, false));
  testrun(frame->buffer);
  testrun(frame->buffer->capacity == 10);
  testrun(frame->buffer->length == 2);
  testrun(frame->opcode == 0);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  testrun(frame->mask == NULL);
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);

  // masking without content requested
  testrun(!ov_websocket_set_data(frame, NULL, 0, true));

  testrun(ov_websocket_set_data(frame, (uint8_t *)data, strlen(data), false));
  testrun(frame->buffer);
  testrun(frame->buffer->capacity == 10);
  testrun(frame->buffer->length == 2 + strlen(data));
  testrun(frame->opcode == 0);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  testrun(frame->mask == NULL);
  testrun(frame->content.start == frame->buffer->start + 2);
  testrun(frame->content.length == strlen(data));
  testrun(0 == memcmp(data, frame->content.start, frame->content.length));

  testrun(ov_websocket_set_data(frame, (uint8_t *)data, strlen(data), true));
  testrun(frame->buffer);
  testrun(frame->buffer->capacity == 10);
  testrun(frame->buffer->length == 6 + strlen(data));
  testrun(frame->opcode == 0);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  testrun(frame->mask == frame->buffer->start + 2);
  testrun(frame->content.start == frame->buffer->start + 6);
  testrun(frame->content.length == strlen(data));
  testrun(0 != memcmp(data, frame->content.start, frame->content.length));

  // NOTE only content will be unmaked!
  testrun(ov_websocket_frame_unmask(frame));
  testrun(frame->buffer);
  testrun(frame->buffer->capacity == 10);
  testrun(frame->buffer->length == 6 + strlen(data));
  testrun(frame->opcode == 0);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  testrun(frame->mask == frame->buffer->start + 2);
  testrun(frame->content.start == frame->buffer->start + 6);
  testrun(frame->content.length == strlen(data));
  testrun(0 == memcmp(data, frame->content.start, frame->content.length));

  // check reset without data
  testrun(ov_websocket_set_data(frame, NULL, strlen(data), false));
  testrun(frame->buffer);
  testrun(frame->buffer->capacity == 10);
  testrun(frame->buffer->length == 2);
  testrun(frame->opcode == 0);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  testrun(frame->mask == NULL);
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);

  data = "something longer as buffer capacity";
  testrun(ov_websocket_set_data(frame, (uint8_t *)data, strlen(data), true));
  testrun(frame->buffer);
  testrun(frame->buffer->capacity > 10);
  testrun(frame->buffer->capacity == strlen(data) + 6);
  testrun(frame->buffer->length == 6 + strlen(data));
  testrun(frame->opcode == 0);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  testrun(frame->mask == frame->buffer->start + 2);
  testrun(frame->content.start == frame->buffer->start + 6);
  testrun(frame->content.length == strlen(data));
  testrun(0 != memcmp(data, frame->content.start, frame->content.length));

  // check reset without data length
  testrun(ov_websocket_set_data(frame, (uint8_t *)data, 0, false));
  testrun(frame->buffer);
  testrun(frame->buffer->capacity == strlen(data) + 6);
  testrun(frame->buffer->length == 2);
  testrun(frame->opcode == 0);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  testrun(frame->mask == NULL);
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);

  // check with length > 125
  uint8_t buffer[200] = {0};
  memset(buffer, 'A', 200);

  testrun(ov_websocket_set_data(frame, buffer, 200, false));
  testrun(frame->buffer);
  testrun(frame->buffer->capacity == 204);
  testrun(frame->buffer->length == 204);
  testrun(frame->opcode == 0);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  testrun(frame->mask == NULL);
  testrun(frame->content.start == frame->buffer->start + 4);
  testrun(frame->content.length == 200);
  testrun(0 == memcmp(buffer, frame->content.start, frame->content.length));

  // check with mask
  testrun(ov_websocket_set_data(frame, buffer, 200, true));
  testrun(frame->buffer);
  testrun(frame->buffer->capacity == 208);
  testrun(frame->buffer->length == 208);
  testrun(frame->opcode == 0);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  testrun(frame->mask == frame->buffer->start + 4);
  testrun(frame->content.start == frame->buffer->start + 8);
  testrun(frame->content.length == 200);
  testrun(0 != memcmp(buffer, frame->content.start, frame->content.length));
  // NOTE only content will be unmaked!
  testrun(ov_websocket_frame_unmask(frame));
  testrun(frame->buffer->capacity == 208);
  testrun(frame->buffer->length == 208);
  testrun(frame->opcode == 0);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  testrun(frame->mask == frame->buffer->start + 4);
  testrun(frame->content.start == frame->buffer->start + 8);
  testrun(frame->content.length == 200);
  testrun(0 == memcmp(buffer, frame->content.start, frame->content.length));

  testrun(NULL == ov_websocket_frame_free(frame));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_websocket_frame_unmask() {

  ov_websocket_frame_config config = {.buffer.default_size = 10};

  ov_websocket_frame *frame = ov_websocket_frame_create(config);
  testrun(frame);
  testrun(frame->buffer->capacity == 10);

  char *data = "test";

  // create unmasked frame
  testrun(ov_websocket_set_data(frame, (uint8_t *)data, strlen(data), false));
  testrun(frame->buffer);
  testrun(frame->buffer->capacity == 10);
  testrun(frame->buffer->length == 2 + strlen(data));
  testrun(frame->opcode == 0);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  testrun(frame->mask == NULL);
  testrun(frame->content.start == frame->buffer->start + 2);
  testrun(frame->content.length == strlen(data));
  testrun(0 == memcmp(data, frame->content.start, frame->content.length));

  testrun(!ov_websocket_frame_unmask(NULL));
  testrun(ov_websocket_frame_unmask(frame));
  testrun(frame->buffer);
  testrun(frame->buffer->capacity == 10);
  testrun(frame->buffer->length == 2 + strlen(data));
  testrun(frame->opcode == 0);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  testrun(frame->mask == NULL);
  testrun(frame->content.start == frame->buffer->start + 2);
  testrun(frame->content.length == strlen(data));
  testrun(0 == memcmp(data, frame->content.start, frame->content.length));

  // created masked frame
  testrun(ov_websocket_set_data(frame, (uint8_t *)data, strlen(data), true));
  testrun(frame->buffer);
  testrun(frame->buffer->capacity == 10);
  testrun(frame->buffer->length == 6 + strlen(data));
  testrun(frame->opcode == 0);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  testrun(frame->mask == frame->buffer->start + 2);
  testrun(frame->content.start == frame->buffer->start + 6);
  testrun(frame->content.length == strlen(data));
  testrun(0 != memcmp(data, frame->content.start, frame->content.length));
  // unmask content
  testrun(ov_websocket_frame_unmask(frame));
  testrun(frame->buffer);
  testrun(frame->buffer->capacity == 10);
  testrun(frame->buffer->length == 6 + strlen(data));
  testrun(frame->opcode == 0);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  testrun(frame->mask == frame->buffer->start + 2);
  testrun(frame->content.start == frame->buffer->start + 6);
  testrun(frame->content.length == strlen(data));
  testrun(0 == memcmp(data, frame->content.start, frame->content.length));

  // reset frame
  testrun(ov_websocket_set_data(frame, NULL, 0, false));
  testrun(frame->buffer);
  testrun(frame->buffer->capacity == 10);
  testrun(frame->buffer->length == 2);
  testrun(frame->opcode == 0);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  testrun(frame->mask == NULL);
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);

  testrun(ov_websocket_frame_unmask(frame));
  testrun(frame->buffer);
  testrun(frame->buffer->capacity == 10);
  testrun(frame->buffer->length == 2);
  testrun(frame->opcode == 0);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  testrun(frame->mask == NULL);
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);

  // set mask, but no content
  frame->mask = (uint8_t *)data;
  testrun(!ov_websocket_frame_unmask(frame));
  testrun(frame->buffer);
  testrun(frame->buffer->capacity == 10);
  testrun(frame->buffer->length == 2);
  testrun(frame->opcode == 0);
  testrun(frame->state == OV_WEBSOCKET_FRAGMENTATION_CONTINUE);
  testrun(frame->mask == (uint8_t *)data);
  testrun(frame->content.start == NULL);
  testrun(frame->content.length == 0);

  testrun(NULL == ov_websocket_frame_free(frame));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_websocket_frame_shift_trailing_bytes() {

  ov_websocket_frame *frame =
      ov_websocket_frame_create((ov_websocket_frame_config){0});

  testrun(frame);

  for (size_t i = 0; i < 200; i++) {

    if (i < 100) {
      frame->buffer->start[i] = 'A';
    } else {
      frame->buffer->start[i] = 'B';
    }
  }

  frame->buffer->length = 200;

  ov_websocket_frame *dest = NULL;
  testrun(!ov_websocket_frame_shift_trailing_bytes(NULL, NULL, NULL));
  testrun(!ov_websocket_frame_shift_trailing_bytes(frame, NULL, &dest));
  testrun(!ov_websocket_frame_shift_trailing_bytes(
      frame, frame->buffer->start + 100, NULL));
  testrun(!ov_websocket_frame_shift_trailing_bytes(
      NULL, frame->buffer->start + 100, &dest));

  testrun(ov_websocket_frame_shift_trailing_bytes(
      frame, frame->buffer->start + 100, &dest));
  testrun(dest);

  for (size_t i = 0; i < 200; i++) {

    if (i < 100) {
      testrun(frame->buffer->start[i] == 'A');
      testrun(dest->buffer->start[i] == 'B');
    } else {
      testrun(frame->buffer->start[i] == 0);
      testrun(dest->buffer->start[i] == 0);
    }
  }

  dest = ov_websocket_frame_free(dest);

  testrun(ov_websocket_frame_shift_trailing_bytes(
      frame, frame->buffer->start + 100, &dest));
  testrun(dest);

  for (size_t i = 0; i < 200; i++) {

    if (i < 100) {
      testrun(frame->buffer->start[i] == 'A');
      testrun(dest->buffer->start[i] == 0);
    } else {
      testrun(frame->buffer->start[i] == 0);
      testrun(dest->buffer->start[i] == 0);
    }
  }

  dest = ov_websocket_frame_free(dest);
  frame = ov_websocket_frame_free(frame);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_websocket_frame_config_from_json() {

  ov_websocket_frame_config in = {0};
  ov_websocket_frame_config out = {0};
  ov_json_value *value = ov_websocket_frame_config_to_json(in);
  testrun(value);
  out = ov_websocket_frame_config_from_json(NULL);
  testrun(0 == out.buffer.default_size);
  testrun(0 == out.buffer.max_bytes_recache);
  out = ov_websocket_frame_config_from_json(value);
  testrun(0 == out.buffer.default_size);
  testrun(0 == out.buffer.max_bytes_recache);
  value = ov_json_value_free(value);

  in.buffer.default_size = 4;
  in.buffer.max_bytes_recache = 5;

  value = ov_websocket_frame_config_to_json(in);
  out = ov_websocket_frame_config_from_json(value);
  testrun(4 == out.buffer.default_size);
  testrun(5 == out.buffer.max_bytes_recache);

  ov_json_value *obj = ov_json_object();
  testrun(ov_json_object_set(obj, OV_KEY_WEBSOCKET, value));
  out = ov_websocket_frame_config_from_json(obj);
  testrun(4 == out.buffer.default_size);
  testrun(5 == out.buffer.max_bytes_recache);
  obj = ov_json_value_free(obj);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_websocket_frame_config_to_json() {

  ov_websocket_frame_config config = {0};
  ov_json_value *out = ov_websocket_frame_config_to_json(config);
  testrun(out);
  testrun(ov_json_get(out, "/" OV_KEY_BUFFER));
  testrun(ov_json_get(out, "/" OV_KEY_BUFFER "/" OV_KEY_SIZE));
  testrun(ov_json_get(out, "/" OV_KEY_BUFFER "/" OV_KEY_MAX_CACHE));

  testrun(0 == ov_json_number_get(
                   ov_json_get(out, "/" OV_KEY_BUFFER "/" OV_KEY_SIZE)));
  testrun(0 == ov_json_number_get(
                   ov_json_get(out, "/" OV_KEY_BUFFER "/" OV_KEY_MAX_CACHE)));
  out = ov_json_value_free(out);

  config.buffer.default_size = 4;
  config.buffer.max_bytes_recache = 5;

  out = ov_websocket_frame_config_to_json(config);
  testrun(out);
  testrun(4 == ov_json_number_get(
                   ov_json_get(out, "/" OV_KEY_BUFFER "/" OV_KEY_SIZE)));
  testrun(5 == ov_json_number_get(
                   ov_json_get(out, "/" OV_KEY_BUFFER "/" OV_KEY_MAX_CACHE)));
  out = ov_json_value_free(out);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

  testrun_init();

  testrun_test(test_ov_websocket_generate_secure_websocket_key);
  testrun_test(test_ov_websocket_generate_secure_accept_key);

  testrun_test(test_ov_websocket_frame_cast);
  testrun_test(test_ov_websocket_frame_create);
  testrun_test(test_ov_websocket_frame_clear);
  testrun_test(test_ov_websocket_frame_free);
  testrun_test(test_ov_websocket_frame_free_uncached);

  testrun_test(test_ov_websocket_process_handshake_request);
  testrun_test(check_fragmentation_state);
  testrun_test(check_parse_payload_length);
  testrun_test(test_ov_websocket_parse_frame);

  testrun_test(check_generate_masking_key);
  testrun_test(check_mask_data);

  testrun_test(test_ov_websocket_set_data);
  testrun_test(test_ov_websocket_frame_unmask);

  testrun_test(test_ov_websocket_frame_shift_trailing_bytes);

  testrun_test(test_ov_websocket_frame_config_from_json);
  testrun_test(test_ov_websocket_frame_config_to_json);

  return testrun_counter;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST EXECUTION                                                  #EXEC
 *
 *      ------------------------------------------------------------------------
 */

testrun_run(all_tests);
