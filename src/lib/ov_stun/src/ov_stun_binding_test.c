/***
        ------------------------------------------------------------------------

        Copyright (c) 2018 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_stun_binding_test.c
        @author         Markus Toepfer

        @date           2018-10-23

        @ingroup        ov_stun

        @brief          Unit tests of RFC 5389 method "binding"


        ------------------------------------------------------------------------
*/

#include "ov_stun_binding.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_stun_binding_class_support() {

  size_t size = 100;
  uint8_t buf[size];
  memset(buf, 0, size);

  testrun(!ov_stun_binding_class_support(NULL, 0));
  testrun(!ov_stun_binding_class_support(buf, 0));
  testrun(!ov_stun_binding_class_support(NULL, 20));
  testrun(!ov_stun_binding_class_support(buf, 19));

  testrun(ov_stun_frame_set_request(buf, 20));
  testrun(ov_stun_binding_class_support(buf, 20));

  testrun(ov_stun_frame_set_indication(buf, 20));
  testrun(ov_stun_binding_class_support(buf, 20));

  testrun(ov_stun_frame_set_success_response(buf, 20));
  testrun(ov_stun_binding_class_support(buf, 20));

  testrun(ov_stun_frame_set_error_response(buf, 20));
  testrun(ov_stun_binding_class_support(buf, 20));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_method_is_binding() {

  size_t size = 100;
  uint8_t buf[size];
  memset(buf, 0, size);

  testrun(!ov_stun_method_is_binding(buf, size));
  testrun(ov_stun_frame_set_method(buf, size, STUN_BINDING));
  testrun(ov_stun_method_is_binding(buf, size));

  for (size_t i = 0; i <= 0xfff; i++) {

    testrun(ov_stun_frame_set_method(buf, size, i));

    if (i == STUN_BINDING) {
      testrun(ov_stun_method_is_binding(buf, size));
    } else {
      testrun(!ov_stun_method_is_binding(buf, size));
    }
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_generate_binding_request_plain() {

  size_t len = 0;
  size_t size = 100;
  uint8_t buf[size];
  memset(buf, 0, size);

  uint8_t *next = NULL;

  size_t az = STUN_FRAME_ATTRIBUTE_ARRAY_SIZE;
  uint8_t *arr[az];

  for (size_t i = 0; i < az; i++) {
    arr[i] = NULL;
  }

  uint8_t transaction_id[12] = {0};

  char *software = "software";

  uint8_t non_valid[6];
  memcpy(non_valid, "valid", 5);
  non_valid[3] = 0xff;

  uint8_t *content = NULL;
  size_t clen = 0;

  testrun(ov_stun_frame_generate_transaction_id(transaction_id));

  // check all set
  testrun(ov_stun_generate_binding_request_plain(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      true));

  len = next - buf;
  testrun(ov_stun_frame_is_valid(buf, len));
  testrun(ov_stun_frame_class_is_request(buf, len));
  testrun(ov_stun_method_is_binding(buf, len));
  testrun(ov_stun_frame_has_magic_cookie(buf, len));
  testrun(0 == memcmp(transaction_id, buf + 8, 12));
  testrun(ov_stun_frame_slice(buf, len, arr, az));
  testrun(ov_stun_attribute_frame_is_software(arr[0], len - (arr[0] - buf)));
  testrun(
      ov_stun_software_decode(arr[0], len - (arr[0] - buf), &content, &clen));
  testrun(0 == memcmp(software, content, clen));
  testrun(clen == strlen(software));
  testrun(ov_stun_attribute_frame_is_fingerprint(arr[1], len - (arr[1] - buf)));
  testrun(ov_stun_check_fingerprint(buf, len, arr, az, true));
  testrun(NULL == arr[2]);

  // reset
  memset(buf, 0, size);

  // check without fingerprint
  testrun(ov_stun_generate_binding_request_plain(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      false));

  len = next - buf;
  testrun(ov_stun_frame_is_valid(buf, len));
  testrun(ov_stun_frame_class_is_request(buf, len));
  testrun(ov_stun_method_is_binding(buf, len));
  testrun(ov_stun_frame_has_magic_cookie(buf, len));
  testrun(0 == memcmp(transaction_id, buf + 8, 12));
  testrun(ov_stun_frame_slice(buf, len, arr, az));
  testrun(ov_stun_attribute_frame_is_software(arr[0], len - (arr[0] - buf)));
  testrun(
      ov_stun_software_decode(arr[0], len - (arr[0] - buf), &content, &clen));
  testrun(0 == memcmp(software, content, clen));
  testrun(clen == strlen(software));
  testrun(NULL == arr[1]);

  // reset
  memset(buf, 0, size);

  // check without fingerprint and software
  testrun(ov_stun_generate_binding_request_plain(
      buf, size, &next, transaction_id, NULL, 0, false));

  len = next - buf;
  testrun(ov_stun_frame_is_valid(buf, len));
  testrun(ov_stun_frame_class_is_request(buf, len));
  testrun(ov_stun_method_is_binding(buf, len));
  testrun(ov_stun_frame_has_magic_cookie(buf, len));
  testrun(0 == memcmp(transaction_id, buf + 8, 12));
  testrun(ov_stun_frame_slice(buf, len, arr, az));
  testrun(NULL == arr[0]);
  // reset

  memset(buf, 0, size);

  // invalid input
  testrun(!ov_stun_generate_binding_request_plain(
      buf, size, &next, transaction_id, (uint8_t *)software, 0, false));

  testrun(!ov_stun_generate_binding_request_plain(
      buf, size, &next, transaction_id, non_valid, 5, false));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_generate_binding_request_short_term() {

  size_t az = STUN_FRAME_ATTRIBUTE_ARRAY_SIZE;
  uint8_t *arr[az];

  for (size_t i = 0; i < az; i++) {
    arr[i] = NULL;
  }

  size_t len = 0;
  size_t size = 100;
  uint8_t buf[size];
  memset(buf, 0, size);

  uint8_t *next = NULL;

  uint8_t transaction_id[12] = {0};

  char *software = "software";
  char *username = "username";
  char *key = "key";

  uint8_t non_valid[6];
  memcpy(non_valid, "valid", 5);
  non_valid[3] = 0xff;

  uint8_t *content = NULL;
  size_t clen = 0;

  testrun(ov_stun_frame_generate_transaction_id(transaction_id));

  // check all set
  testrun(ov_stun_generate_binding_request_short_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, strlen(username), (uint8_t *)key, strlen(key),
      true));

  len = next - buf;
  testrun(ov_stun_frame_is_valid(buf, len));
  testrun(ov_stun_frame_class_is_request(buf, len));
  testrun(ov_stun_method_is_binding(buf, len));
  testrun(ov_stun_frame_has_magic_cookie(buf, len));
  testrun(0 == memcmp(transaction_id, buf + 8, 12));
  testrun(ov_stun_frame_slice(buf, len, arr, az));
  testrun(ov_stun_attribute_frame_is_username(arr[0], len - (arr[0] - buf)));
  testrun(
      ov_stun_username_decode(arr[0], len - (arr[0] - buf), &content, &clen));
  testrun(0 == memcmp(username, content, clen));
  testrun(clen == strlen(username));
  testrun(ov_stun_attribute_frame_is_software(arr[1], len - (arr[1] - buf)));
  testrun(
      ov_stun_software_decode(arr[1], len - (arr[1] - buf), &content, &clen));
  testrun(0 == memcmp(software, content, clen));
  testrun(clen == strlen(software));
  testrun(ov_stun_attribute_frame_is_message_integrity(arr[2],
                                                       len - (arr[2] - buf)));
  testrun(ov_stun_check_message_integrity(buf, len, arr, az, (uint8_t *)key,
                                          strlen(key), true));
  testrun(ov_stun_attribute_frame_is_fingerprint(arr[3], len - (arr[3] - buf)));
  testrun(ov_stun_check_fingerprint(buf, len, arr, az, true));
  for (size_t i = 5; i < az; i++) {
    testrun(arr[i] == NULL);
  }

  // reset
  memset(buf, 0, size);

  // check without fingerprint
  testrun(ov_stun_generate_binding_request_short_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, strlen(username), (uint8_t *)key, strlen(key),
      false));

  len = next - buf;
  testrun(ov_stun_frame_is_valid(buf, len));
  testrun(ov_stun_frame_class_is_request(buf, len));
  testrun(ov_stun_method_is_binding(buf, len));
  testrun(ov_stun_frame_has_magic_cookie(buf, len));
  testrun(0 == memcmp(transaction_id, buf + 8, 12));
  testrun(ov_stun_frame_slice(buf, len, arr, az));
  testrun(ov_stun_attribute_frame_is_username(arr[0], len - (arr[0] - buf)));
  testrun(
      ov_stun_username_decode(arr[0], len - (arr[0] - buf), &content, &clen));
  testrun(0 == memcmp(username, content, clen));
  testrun(clen == strlen(username));
  testrun(ov_stun_attribute_frame_is_software(arr[1], len - (arr[1] - buf)));
  testrun(
      ov_stun_software_decode(arr[1], len - (arr[1] - buf), &content, &clen));
  testrun(0 == memcmp(software, content, clen));
  testrun(clen == strlen(software));
  testrun(ov_stun_attribute_frame_is_message_integrity(arr[2],
                                                       len - (arr[2] - buf)));
  testrun(ov_stun_check_message_integrity(buf, len, arr, az, (uint8_t *)key,
                                          strlen(key), true));
  for (size_t i = 4; i < az; i++) {
    testrun(arr[i] == NULL);
  }

  memset(buf, 0, size);

  // optional input (software)
  testrun(ov_stun_generate_binding_request_short_term(
      buf, size, &next, transaction_id, NULL, strlen(software),
      (uint8_t *)username, strlen(username), (uint8_t *)key, strlen(key),
      false));
  memset(buf, 0, size);

  // invalid input
  testrun(!ov_stun_generate_binding_request_short_term(
      buf, size, &next, transaction_id, (uint8_t *)software, 0,
      (uint8_t *)username, strlen(username), (uint8_t *)key, strlen(key),
      false));

  testrun(!ov_stun_generate_binding_request_short_term(
      buf, size, &next, transaction_id, non_valid, 5, (uint8_t *)username,
      strlen(username), (uint8_t *)key, strlen(key), false));

  testrun(!ov_stun_generate_binding_request_short_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      NULL, strlen(username), (uint8_t *)key, strlen(key), false));

  testrun(!ov_stun_generate_binding_request_short_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, 0, (uint8_t *)key, strlen(key), false));

  testrun(!ov_stun_generate_binding_request_short_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      non_valid, 5, (uint8_t *)key, strlen(key), false));

  testrun(!ov_stun_generate_binding_request_short_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, strlen(username), NULL, strlen(key), false));

  testrun(!ov_stun_generate_binding_request_short_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, strlen(username), (uint8_t *)key, 0, false));

  // check with whatever key
  testrun(ov_stun_generate_binding_request_short_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, strlen(username), non_valid, 5, false));

  len = next - buf;
  testrun(ov_stun_frame_is_valid(buf, len));
  testrun(ov_stun_frame_class_is_request(buf, len));
  testrun(ov_stun_method_is_binding(buf, len));
  testrun(ov_stun_frame_has_magic_cookie(buf, len));
  testrun(0 == memcmp(transaction_id, buf + 8, 12));
  testrun(ov_stun_frame_slice(buf, len, arr, az));
  testrun(ov_stun_attribute_frame_is_username(arr[0], len - (arr[0] - buf)));
  testrun(
      ov_stun_username_decode(arr[0], len - (arr[0] - buf), &content, &clen));
  testrun(0 == memcmp(username, content, clen));
  testrun(clen == strlen(username));
  testrun(ov_stun_attribute_frame_is_software(arr[1], len - (arr[1] - buf)));
  testrun(
      ov_stun_software_decode(arr[1], len - (arr[1] - buf), &content, &clen));
  testrun(0 == memcmp(software, content, clen));
  testrun(clen == strlen(software));
  testrun(ov_stun_attribute_frame_is_message_integrity(arr[2],
                                                       len - (arr[2] - buf)));
  testrun(!ov_stun_check_message_integrity(buf, len, arr, az, (uint8_t *)key,
                                           strlen(key), true));
  testrun(
      ov_stun_check_message_integrity(buf, len, arr, az, non_valid, 5, true));
  for (size_t i = 4; i < az; i++) {
    testrun(arr[i] == NULL);
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_generate_binding_request_long_term() {

  size_t az = STUN_FRAME_ATTRIBUTE_ARRAY_SIZE;
  uint8_t *arr[az];

  for (size_t i = 0; i < az; i++) {
    arr[i] = NULL;
  }

  size_t len = 0;
  size_t size = 100;
  uint8_t buf[size];
  memset(buf, 0, size);

  uint8_t *next = NULL;

  uint8_t transaction_id[12] = {0};

  char *software = "software";
  char *username = "username";
  char *realm = "realm";
  char *nonce = "nonce";
  char *key = "key";

  uint8_t non_valid[6];
  memcpy(non_valid, "valid", 5);
  non_valid[3] = 0xff;

  uint8_t *content = NULL;
  size_t clen = 0;

  testrun(ov_stun_frame_generate_transaction_id(transaction_id));

  // check all set
  testrun(ov_stun_generate_binding_request_long_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, strlen(username), (uint8_t *)realm, strlen(realm),
      (uint8_t *)nonce, strlen(nonce), (uint8_t *)key, strlen(key), true));

  len = next - buf;
  testrun(ov_stun_frame_is_valid(buf, len));
  testrun(ov_stun_frame_class_is_request(buf, len));
  testrun(ov_stun_method_is_binding(buf, len));
  testrun(ov_stun_frame_has_magic_cookie(buf, len));
  testrun(0 == memcmp(transaction_id, buf + 8, 12));
  testrun(ov_stun_frame_slice(buf, len, arr, az));
  testrun(ov_stun_attribute_frame_is_username(arr[0], len - (arr[0] - buf)));
  testrun(
      ov_stun_username_decode(arr[0], len - (arr[0] - buf), &content, &clen));
  testrun(0 == memcmp(username, content, clen));
  testrun(clen == strlen(username));
  testrun(ov_stun_attribute_frame_is_realm(arr[1], len - (arr[1] - buf)));
  testrun(ov_stun_realm_decode(arr[1], len - (arr[1] - buf), &content, &clen));
  testrun(0 == memcmp(realm, content, clen));
  testrun(clen == strlen(realm));
  testrun(ov_stun_attribute_frame_is_nonce(arr[2], len - (arr[2] - buf)));
  testrun(ov_stun_nonce_decode(arr[2], len - (arr[2] - buf), &content, &clen));
  testrun(0 == memcmp(nonce, content, clen));
  testrun(clen == strlen(nonce));
  testrun(ov_stun_attribute_frame_is_software(arr[3], len - (arr[3] - buf)));
  testrun(
      ov_stun_software_decode(arr[3], len - (arr[3] - buf), &content, &clen));
  testrun(0 == memcmp(software, content, clen));
  testrun(clen == strlen(software));
  testrun(ov_stun_attribute_frame_is_message_integrity(arr[4],
                                                       len - (arr[4] - buf)));
  testrun(ov_stun_check_message_integrity(buf, len, arr, az, (uint8_t *)key,
                                          strlen(key), true));
  testrun(ov_stun_attribute_frame_is_fingerprint(arr[5], len - (arr[5] - buf)));
  testrun(ov_stun_check_fingerprint(buf, len, arr, az, true));
  for (size_t i = 6; i < az; i++) {
    testrun(arr[i] == NULL);
  }

  // reset
  memset(buf, 0, size);

  // check without fingerprint
  testrun(ov_stun_generate_binding_request_long_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, strlen(username), (uint8_t *)realm, strlen(realm),
      (uint8_t *)nonce, strlen(nonce), (uint8_t *)key, strlen(key), false));

  len = next - buf;
  testrun(ov_stun_frame_is_valid(buf, len));
  testrun(ov_stun_frame_class_is_request(buf, len));
  testrun(ov_stun_method_is_binding(buf, len));
  testrun(ov_stun_frame_has_magic_cookie(buf, len));
  testrun(0 == memcmp(transaction_id, buf + 8, 12));
  testrun(ov_stun_frame_slice(buf, len, arr, az));
  testrun(ov_stun_attribute_frame_is_username(arr[0], len - (arr[0] - buf)));
  testrun(
      ov_stun_username_decode(arr[0], len - (arr[0] - buf), &content, &clen));
  testrun(0 == memcmp(username, content, clen));
  testrun(clen == strlen(username));
  testrun(ov_stun_attribute_frame_is_realm(arr[1], len - (arr[1] - buf)));
  testrun(ov_stun_realm_decode(arr[1], len - (arr[1] - buf), &content, &clen));
  testrun(0 == memcmp(realm, content, clen));
  testrun(clen == strlen(realm));
  testrun(ov_stun_attribute_frame_is_nonce(arr[2], len - (arr[2] - buf)));
  testrun(ov_stun_nonce_decode(arr[2], len - (arr[2] - buf), &content, &clen));
  testrun(0 == memcmp(nonce, content, clen));
  testrun(clen == strlen(nonce));
  testrun(ov_stun_attribute_frame_is_software(arr[3], len - (arr[3] - buf)));
  testrun(
      ov_stun_software_decode(arr[3], len - (arr[3] - buf), &content, &clen));
  testrun(0 == memcmp(software, content, clen));
  testrun(clen == strlen(software));
  testrun(ov_stun_attribute_frame_is_message_integrity(arr[4],
                                                       len - (arr[4] - buf)));
  testrun(ov_stun_check_message_integrity(buf, len, arr, az, (uint8_t *)key,
                                          strlen(key), true));
  for (size_t i = 5; i < az; i++) {
    testrun(arr[i] == NULL);
  }

  // optional input (software)
  testrun(ov_stun_generate_binding_request_long_term(
      buf, size, &next, transaction_id, NULL, strlen(software),
      (uint8_t *)username, strlen(username), (uint8_t *)realm, strlen(realm),
      (uint8_t *)nonce, strlen(nonce), (uint8_t *)key, strlen(key), false));
  memset(buf, 0, size);

  // invalid input
  testrun(!ov_stun_generate_binding_request_long_term(
      buf, size, &next, transaction_id, (uint8_t *)software, 0,
      (uint8_t *)username, strlen(username), (uint8_t *)realm, strlen(realm),
      (uint8_t *)nonce, strlen(nonce), (uint8_t *)key, strlen(key), false));

  testrun(!ov_stun_generate_binding_request_long_term(
      buf, size, &next, transaction_id, non_valid, 5, (uint8_t *)username,
      strlen(username), (uint8_t *)realm, strlen(realm), (uint8_t *)nonce,
      strlen(nonce), (uint8_t *)key, strlen(key), false));

  testrun(!ov_stun_generate_binding_request_long_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      NULL, strlen(username), (uint8_t *)realm, strlen(realm), (uint8_t *)nonce,
      strlen(nonce), (uint8_t *)key, strlen(key), false));

  testrun(!ov_stun_generate_binding_request_long_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, 0, (uint8_t *)realm, strlen(realm), (uint8_t *)nonce,
      strlen(nonce), (uint8_t *)key, strlen(key), false));

  testrun(!ov_stun_generate_binding_request_long_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      non_valid, 5, (uint8_t *)realm, strlen(realm), (uint8_t *)nonce,
      strlen(nonce), (uint8_t *)key, strlen(key), false));

  testrun(!ov_stun_generate_binding_request_long_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, strlen(username), NULL, strlen(realm),
      (uint8_t *)nonce, strlen(nonce), (uint8_t *)key, strlen(key), false));

  testrun(!ov_stun_generate_binding_request_long_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, strlen(username), (uint8_t *)realm, 0,
      (uint8_t *)nonce, strlen(nonce), (uint8_t *)key, strlen(key), false));

  testrun(!ov_stun_generate_binding_request_long_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, strlen(username), non_valid, 5, (uint8_t *)nonce,
      strlen(nonce), (uint8_t *)key, strlen(key), false));

  testrun(!ov_stun_generate_binding_request_long_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, strlen(username), (uint8_t *)realm, strlen(realm),
      NULL, strlen(nonce), (uint8_t *)key, strlen(key), false));

  testrun(!ov_stun_generate_binding_request_long_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, strlen(username), (uint8_t *)realm, strlen(realm),
      (uint8_t *)nonce, 0, (uint8_t *)key, strlen(key), false));

  testrun(!ov_stun_generate_binding_request_long_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, strlen(username), (uint8_t *)realm, strlen(realm),
      non_valid, 5, (uint8_t *)key, strlen(key), false));

  testrun(!ov_stun_generate_binding_request_long_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, strlen(username), (uint8_t *)realm, strlen(realm),
      (uint8_t *)nonce, strlen(nonce), NULL, strlen(key), false));

  testrun(!ov_stun_generate_binding_request_long_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, strlen(username), (uint8_t *)realm, strlen(realm),
      (uint8_t *)nonce, strlen(nonce), (uint8_t *)key, 0, false));

  // check with whatever key
  testrun(ov_stun_generate_binding_request_long_term(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, strlen(username), (uint8_t *)realm, strlen(realm),
      (uint8_t *)nonce, strlen(nonce), non_valid, 5, false));

  len = next - buf;
  testrun(ov_stun_frame_is_valid(buf, len));
  testrun(ov_stun_frame_class_is_request(buf, len));
  testrun(ov_stun_method_is_binding(buf, len));
  testrun(ov_stun_frame_has_magic_cookie(buf, len));
  testrun(0 == memcmp(transaction_id, buf + 8, 12));
  testrun(ov_stun_frame_slice(buf, len, arr, az));
  testrun(ov_stun_attribute_frame_is_username(arr[0], len - (arr[0] - buf)));
  testrun(
      ov_stun_username_decode(arr[0], len - (arr[0] - buf), &content, &clen));
  testrun(0 == memcmp(username, content, clen));
  testrun(clen == strlen(username));
  testrun(ov_stun_attribute_frame_is_realm(arr[1], len - (arr[1] - buf)));
  testrun(ov_stun_realm_decode(arr[1], len - (arr[1] - buf), &content, &clen));
  testrun(0 == memcmp(realm, content, clen));
  testrun(clen == strlen(realm));
  testrun(ov_stun_attribute_frame_is_nonce(arr[2], len - (arr[2] - buf)));
  testrun(ov_stun_nonce_decode(arr[2], len - (arr[2] - buf), &content, &clen));
  testrun(0 == memcmp(nonce, content, clen));
  testrun(clen == strlen(nonce));
  testrun(ov_stun_attribute_frame_is_software(arr[3], len - (arr[3] - buf)));
  testrun(
      ov_stun_software_decode(arr[3], len - (arr[3] - buf), &content, &clen));
  testrun(0 == memcmp(software, content, clen));
  testrun(clen == strlen(software));
  testrun(ov_stun_attribute_frame_is_message_integrity(arr[4],
                                                       len - (arr[4] - buf)));
  testrun(!ov_stun_check_message_integrity(buf, len, arr, az, (uint8_t *)key,
                                           strlen(key), true));
  testrun(
      ov_stun_check_message_integrity(buf, len, arr, az, non_valid, 5, true));
  for (size_t i = 5; i < az; i++) {
    testrun(arr[i] == NULL);
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_generate_binding_request() {

  size_t az = STUN_FRAME_ATTRIBUTE_ARRAY_SIZE;
  uint8_t *arr[az];

  for (size_t i = 0; i < az; i++) {
    arr[i] = NULL;
  }

  size_t len = 0;
  size_t size = 100;
  uint8_t buf[size];
  memset(buf, 0, size);

  uint8_t *next = NULL;
  ov_list *list = NULL;

  uint8_t transaction_id[12] = {0};

  char *software = "software";
  char *username = "username";
  char *realm = "realm";
  char *nonce = "nonce";
  char *key = "key";

  uint8_t non_valid[6];
  memcpy(non_valid, "valid", 5);
  non_valid[3] = 0xff;

  uint8_t *content = NULL;
  size_t clen = 0;

  testrun(ov_stun_frame_generate_transaction_id(transaction_id));

  // check all set
  testrun(ov_stun_generate_binding_request(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      (uint8_t *)username, strlen(username), (uint8_t *)realm, strlen(realm),
      (uint8_t *)nonce, strlen(nonce), (uint8_t *)key, strlen(key), true,
      true));

  len = next - buf;
  testrun(ov_stun_frame_is_valid(buf, len));
  testrun(ov_stun_frame_class_is_request(buf, len));
  testrun(ov_stun_method_is_binding(buf, len));
  testrun(ov_stun_frame_has_magic_cookie(buf, len));
  testrun(0 == memcmp(transaction_id, buf + 8, 12));
  testrun(ov_stun_frame_slice(buf, len, arr, az));
  testrun(ov_stun_attribute_frame_is_username(arr[0], len - (arr[0] - buf)));
  testrun(
      ov_stun_username_decode(arr[0], len - (arr[0] - buf), &content, &clen));
  testrun(0 == memcmp(username, content, clen));
  testrun(clen == strlen(username));
  testrun(ov_stun_attribute_frame_is_realm(arr[1], len - (arr[1] - buf)));
  testrun(ov_stun_realm_decode(arr[1], len - (arr[1] - buf), &content, &clen));
  testrun(0 == memcmp(realm, content, clen));
  testrun(clen == strlen(realm));
  testrun(ov_stun_attribute_frame_is_nonce(arr[2], len - (arr[2] - buf)));
  testrun(ov_stun_nonce_decode(arr[2], len - (arr[2] - buf), &content, &clen));
  testrun(0 == memcmp(nonce, content, clen));
  testrun(clen == strlen(nonce));
  testrun(ov_stun_attribute_frame_is_software(arr[3], len - (arr[3] - buf)));
  testrun(
      ov_stun_software_decode(arr[3], len - (arr[3] - buf), &content, &clen));
  testrun(0 == memcmp(software, content, clen));
  testrun(clen == strlen(software));
  testrun(ov_stun_attribute_frame_is_message_integrity(arr[4],
                                                       len - (arr[4] - buf)));
  testrun(ov_stun_check_message_integrity(buf, len, arr, az, (uint8_t *)key,
                                          strlen(key), true));
  testrun(ov_stun_attribute_frame_is_fingerprint(arr[5], len - (arr[5] - buf)));
  testrun(ov_stun_check_fingerprint(buf, len, arr, az, true));
  for (size_t i = 6; i < az; i++) {
    testrun(arr[i] == NULL);
  }

  memset(buf, 0, size);

  // check input parameter
  testrun(!ov_stun_generate_binding_request(NULL, size, NULL, transaction_id,
                                            NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                            NULL, 0, false, false));

  testrun(!ov_stun_generate_binding_request(buf, 0, NULL, transaction_id, NULL,
                                            0, NULL, 0, NULL, 0, NULL, 0, NULL,
                                            0, false, false));

  testrun(!ov_stun_generate_binding_request(buf, size, NULL, NULL, NULL, 0,
                                            NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                            false, false));

  testrun(!ov_stun_generate_binding_request(buf, 19, NULL, transaction_id, NULL,
                                            0, NULL, 0, NULL, 0, NULL, 0, NULL,
                                            0, false, false));

  // min input parameter
  testrun(ov_stun_generate_binding_request(buf, 20, NULL, transaction_id, NULL,
                                           0, NULL, 0, NULL, 0, NULL, 0, NULL,
                                           0, false, false));

  testrun(ov_stun_frame_is_valid(buf, 20));
  testrun(ov_stun_frame_class_is_request(buf, 20));
  testrun(ov_stun_method_is_binding(buf, 20));
  testrun(ov_stun_frame_has_magic_cookie(buf, 20));
  testrun(0 == memcmp(transaction_id, buf + 8, 12));
  testrun(ov_stun_frame_slice(buf, 20, arr, az));
  testrun(arr[0] == NULL);
  // check bytes > 20 unset
  for (size_t i = 20; i < size; i++) {
    testrun(buf[i] == 0);
  }
  // reset
  list = ov_list_free(list);
  memset(buf, 0, size);

  // check message integrity
  testrun(!ov_stun_generate_binding_request(buf, size, &next, transaction_id,
                                            NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                            NULL, 0, true, false));
  testrun(!ov_stun_generate_binding_request(
      buf, 43, &next, transaction_id, NULL, 0, NULL, 0, NULL, 0, NULL, 0,
      (uint8_t *)key, strlen(key), true, false));
  testrun(ov_stun_generate_binding_request(
      buf, 44, &next, transaction_id, NULL, 0, NULL, 0, NULL, 0, NULL, 0,
      (uint8_t *)key, strlen(key), true, false));
  len = next - buf;
  testrun(len == 44);
  testrun(ov_stun_frame_is_valid(buf, len));
  testrun(ov_stun_frame_class_is_request(buf, len));
  testrun(ov_stun_method_is_binding(buf, len));
  testrun(ov_stun_frame_has_magic_cookie(buf, len));
  testrun(0 == memcmp(transaction_id, buf + 8, 12));
  testrun(ov_stun_frame_slice(buf, len, arr, az));
  testrun(ov_stun_attribute_frame_is_message_integrity(arr[0],
                                                       len - (arr[0] - buf)));
  testrun(ov_stun_check_message_integrity(buf, len, arr, az, (uint8_t *)key,
                                          strlen(key), true));
  testrun(arr[1] == NULL);
  // reset
  memset(buf, 0, size);

  // check fingerprint
  testrun(!ov_stun_generate_binding_request(buf, 27, &next, transaction_id,
                                            NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                            NULL, 0, false, true));
  testrun(ov_stun_generate_binding_request(buf, 28, &next, transaction_id, NULL,
                                           0, NULL, 0, NULL, 0, NULL, 0, NULL,
                                           0, false, true));
  len = next - buf;
  testrun(len == 28);
  testrun(ov_stun_frame_is_valid(buf, len));
  testrun(ov_stun_frame_class_is_request(buf, len));
  testrun(ov_stun_method_is_binding(buf, len));
  testrun(ov_stun_frame_has_magic_cookie(buf, len));
  testrun(0 == memcmp(transaction_id, buf + 8, 12));
  testrun(ov_stun_frame_slice(buf, len, arr, az));
  testrun(ov_stun_attribute_frame_is_fingerprint(arr[0], len - (arr[0] - buf)));
  testrun(ov_stun_check_fingerprint(buf, len, arr, az, true));
  testrun(arr[1] == NULL);
  memset(buf, 0, size);

  // check nonce
  testrun(!ov_stun_generate_binding_request(
      buf, size, &next, transaction_id, NULL, 0, NULL, 0, NULL, 0,
      (uint8_t *)nonce, 0, NULL, 0, false, false));
  testrun(!ov_stun_generate_binding_request(
      buf, size, &next, transaction_id, NULL, 0, NULL, 0, NULL, 0,
      (uint8_t *)nonce, 765, NULL, 0, false, false));
  testrun(!ov_stun_generate_binding_request(
      buf, size, &next, transaction_id, NULL, 0, NULL, 0, NULL, 0, non_valid, 5,
      NULL, 0, false, false));
  testrun(ov_stun_generate_binding_request(
      buf, size, &next, transaction_id, NULL, 0, NULL, 0, NULL, 0,
      (uint8_t *)nonce, strlen(nonce), NULL, 0, false, false));
  len = next - buf;
  testrun(len ==
          20 + ov_stun_nonce_encoding_length((uint8_t *)nonce, strlen(nonce)));
  testrun(ov_stun_frame_is_valid(buf, len));
  testrun(ov_stun_frame_class_is_request(buf, len));
  testrun(ov_stun_method_is_binding(buf, len));
  testrun(ov_stun_frame_has_magic_cookie(buf, len));
  testrun(0 == memcmp(transaction_id, buf + 8, 12));
  testrun(ov_stun_frame_slice(buf, len, arr, az));
  testrun(ov_stun_attribute_frame_is_nonce(arr[0], len - (arr[0] - buf)));
  testrun(ov_stun_nonce_decode(arr[0], len - (arr[0] - buf), &content, &clen));
  testrun(0 == memcmp(nonce, content, clen));
  testrun(clen == strlen(nonce));
  testrun(arr[1] == NULL);

  // reset
  memset(buf, 0, size);

  // check relam
  testrun(!ov_stun_generate_binding_request(buf, size, &next, transaction_id,
                                            NULL, 0, NULL, 0, (uint8_t *)realm,
                                            0, NULL, 0, NULL, 0, false, false));
  testrun(!ov_stun_generate_binding_request(
      buf, size, &next, transaction_id, NULL, 0, NULL, 0, (uint8_t *)realm, 765,
      NULL, 0, NULL, 0, false, false));
  testrun(!ov_stun_generate_binding_request(buf, size, &next, transaction_id,
                                            NULL, 0, NULL, 0, non_valid, 5,
                                            NULL, 0, NULL, 0, false, false));
  testrun(ov_stun_generate_binding_request(
      buf, size, &next, transaction_id, NULL, 0, NULL, 0, (uint8_t *)realm,
      strlen(realm), NULL, 0, NULL, 0, false, false));
  len = next - buf;
  testrun(len ==
          20 + ov_stun_realm_encoding_length((uint8_t *)realm, strlen(realm)));
  testrun(ov_stun_frame_is_valid(buf, len));
  testrun(ov_stun_frame_class_is_request(buf, len));
  testrun(ov_stun_method_is_binding(buf, len));
  testrun(ov_stun_frame_has_magic_cookie(buf, len));
  testrun(0 == memcmp(transaction_id, buf + 8, 12));
  testrun(ov_stun_frame_slice(buf, len, arr, az));
  testrun(ov_stun_attribute_frame_is_realm(arr[0], len - (arr[0] - buf)));
  testrun(ov_stun_realm_decode(arr[0], len - (arr[0] - buf), &content, &clen));
  testrun(0 == memcmp(realm, content, clen));
  testrun(clen == strlen(realm));
  testrun(arr[1] == NULL);
  // reset
  memset(buf, 0, size);

  // check username
  testrun(!ov_stun_generate_binding_request(
      buf, size, &next, transaction_id, NULL, 0, (uint8_t *)username, 0, NULL,
      0, NULL, 0, NULL, 0, false, false));
  testrun(!ov_stun_generate_binding_request(
      buf, size, &next, transaction_id, NULL, 0, (uint8_t *)username, 765, NULL,
      0, NULL, 0, NULL, 0, false, false));
  testrun(!ov_stun_generate_binding_request(buf, size, &next, transaction_id,
                                            NULL, 0, non_valid, 5, NULL, 0,
                                            NULL, 0, NULL, 0, false, false));
  testrun(ov_stun_generate_binding_request(
      buf, size, &next, transaction_id, NULL, 0, (uint8_t *)username,
      strlen(username), NULL, 0, NULL, 0, NULL, 0, false, false));
  len = next - buf;
  testrun(len == 20 + ov_stun_username_encoding_length((uint8_t *)username,
                                                       strlen(username)));
  testrun(ov_stun_frame_is_valid(buf, len));
  testrun(ov_stun_frame_class_is_request(buf, len));
  testrun(ov_stun_method_is_binding(buf, len));
  testrun(ov_stun_frame_has_magic_cookie(buf, len));
  testrun(0 == memcmp(transaction_id, buf + 8, 12));
  testrun(ov_stun_frame_slice(buf, len, arr, az));
  testrun(ov_stun_attribute_frame_is_username(arr[0], len - (arr[0] - buf)));
  testrun(
      ov_stun_username_decode(arr[0], len - (arr[0] - buf), &content, &clen));
  testrun(0 == memcmp(username, content, clen));
  testrun(clen == strlen(username));
  testrun(arr[1] == NULL);

  memset(buf, 0, size);

  // check software
  testrun(!ov_stun_generate_binding_request(
      buf, size, &next, transaction_id, (uint8_t *)software, 0, NULL, 0, NULL,
      0, NULL, 0, NULL, 0, false, false));
  testrun(!ov_stun_generate_binding_request(
      buf, size, &next, transaction_id, (uint8_t *)software, 765, NULL, 0, NULL,
      0, NULL, 0, NULL, 0, false, false));
  testrun(!ov_stun_generate_binding_request(buf, size, &next, transaction_id,
                                            non_valid, 5, NULL, 0, NULL, 0,
                                            NULL, 0, NULL, 0, false, false));
  testrun(ov_stun_generate_binding_request(
      buf, size, &next, transaction_id, (uint8_t *)software, strlen(software),
      NULL, 0, NULL, 0, NULL, 0, NULL, 0, false, false));
  len = next - buf;
  testrun(len == 20 + ov_stun_software_encoding_length((uint8_t *)software,
                                                       strlen(software)));
  testrun(ov_stun_frame_is_valid(buf, len));
  testrun(ov_stun_frame_class_is_request(buf, len));
  testrun(ov_stun_method_is_binding(buf, len));
  testrun(ov_stun_frame_has_magic_cookie(buf, len));
  testrun(0 == memcmp(transaction_id, buf + 8, 12));
  testrun(ov_stun_frame_slice(buf, len, arr, az));
  testrun(ov_stun_attribute_frame_is_software(arr[0], len - (arr[0] - buf)));
  testrun(
      ov_stun_software_decode(arr[0], len - (arr[0] - buf), &content, &clen));
  testrun(0 == memcmp(software, content, clen));
  testrun(clen == strlen(software));
  testrun(arr[1] == NULL);

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
  testrun_test(test_ov_stun_binding_class_support);
  testrun_test(test_ov_stun_method_is_binding);

  testrun_test(test_ov_stun_generate_binding_request_plain);
  testrun_test(test_ov_stun_generate_binding_request_short_term);
  testrun_test(test_ov_stun_generate_binding_request_long_term);
  testrun_test(test_ov_stun_generate_binding_request);

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
