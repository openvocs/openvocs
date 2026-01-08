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
        @file           ov_stun_mapped_address_test.c
        @author         Markus Toepfer

        @date           2018-10-19

        @ingroup        ov_stun

        @brief          Unit tests of RFC 5389 attribute "mapped address"


        ------------------------------------------------------------------------
*/

#include "ov_stun_mapped_address.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_stun_attribute_frame_is_mapped_address() {

  size_t size = 100;
  uint8_t buf[size];
  uint8_t *buffer = buf;

  memset(buf, size, 100);

  // prepare valid frame
  testrun(ov_stun_attribute_set_type(buffer, size, STUN_MAPPED_ADDRESS));
  testrun(ov_stun_attribute_set_length(buffer, size, 8));
  *(buf + 5) = 0x01;

  testrun(ov_stun_attribute_frame_is_mapped_address(buffer, size));

  testrun(!ov_stun_attribute_frame_is_mapped_address(NULL, size));
  testrun(!ov_stun_attribute_frame_is_mapped_address(buffer, 0));
  testrun(!ov_stun_attribute_frame_is_mapped_address(buffer, 11));

  // min buffer min valid
  testrun(ov_stun_attribute_frame_is_mapped_address(buffer, 12));

  // check ipv4 + length
  for (int i = 0; i < 0xff; i++) {

    *(buf + 5) = i;
    if (i == 0x01) {
      testrun(ov_stun_attribute_frame_is_mapped_address(buffer, size));
    } else {
      testrun(!ov_stun_attribute_frame_is_mapped_address(buffer, size));
    }

    *(buf + 5) = 0x01;
    testrun(ov_stun_attribute_set_length(buffer, size, i));
    if (i == 8) {
      testrun(ov_stun_attribute_frame_is_mapped_address(buffer, size));
    } else {
      testrun(!ov_stun_attribute_frame_is_mapped_address(buffer, size));
    }

    *(buf + 5) = 0x01;
    testrun(ov_stun_attribute_set_length(buffer, size, 8));

    // ignore first bytes
    *(buf + 4) = i;
    testrun(ov_stun_attribute_frame_is_mapped_address(buffer, size));
  }

  // check ipv6 + length
  for (int i = 0; i < 0xff; i++) {

    *(buf + 5) = i;
    testrun(ov_stun_attribute_set_length(buffer, size, 20));
    if (i == 0x02) {
      testrun(ov_stun_attribute_frame_is_mapped_address(buffer, size));
    } else {
      testrun(!ov_stun_attribute_frame_is_mapped_address(buffer, size));
    }

    *(buf + 5) = 0x02;
    testrun(ov_stun_attribute_set_length(buffer, size, i));

    if (i == 20) {
      testrun(ov_stun_attribute_frame_is_mapped_address(buffer, size));
    } else {
      testrun(!ov_stun_attribute_frame_is_mapped_address(buffer, size));
    }

    *(buf + 5) = 0x02;
    testrun(ov_stun_attribute_set_length(buffer, size, 20));

    // ignore 4 bytes
    *(buf + 4) = i;
    testrun(ov_stun_attribute_frame_is_mapped_address(buffer, size));

    *(buf + 4) = 0x00;
    *(buf + 5) = 0x02;
    testrun(ov_stun_attribute_set_length(buffer, size, 20));

    testrun(ov_stun_attribute_set_type(buffer, size, i));
    if (i == STUN_MAPPED_ADDRESS) {
      testrun(ov_stun_attribute_frame_is_mapped_address(buffer, size));
    } else {
      testrun(!ov_stun_attribute_frame_is_mapped_address(buffer, size));
    }
    testrun(ov_stun_attribute_set_type(buffer, size, STUN_MAPPED_ADDRESS));
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_mapped_address_encoding_length() {

  struct sockaddr_storage sa = {0};

  sa.ss_family = AF_UNIX;
  testrun(0 == ov_stun_mapped_address_encoding_length(&sa));

  sa.ss_family = AF_INET;
  testrun(12 == ov_stun_mapped_address_encoding_length(&sa));

  sa.ss_family = AF_INET6;
  testrun(24 == ov_stun_mapped_address_encoding_length(&sa));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_mapped_address_encode() {

  size_t size = 100;
  uint8_t buf[size];
  uint8_t *buffer = buf;
  uint8_t *next = NULL;

  memset(buf, size, 100);

  struct sockaddr_storage sa = {0};
  struct sockaddr_in6 *sock6 = NULL;
  struct sockaddr_in *sock4 = NULL;

  sock4 = (struct sockaddr_in *)&sa;
  sock4->sin_family = AF_INET;
  sock4->sin_port = htons(0xff01);
  testrun(1 == inet_pton(AF_INET, "127.0.0.1", &sock4->sin_addr));

  testrun(!ov_stun_mapped_address_encode(NULL, 0, NULL, NULL));
  testrun(!ov_stun_mapped_address_encode(NULL, size, NULL, &sa));
  testrun(!ov_stun_mapped_address_encode(buffer, 0, NULL, &sa));
  testrun(!ov_stun_mapped_address_encode(buffer, size, NULL, NULL));

  testrun(!ov_stun_mapped_address_encode(buffer, 11, NULL, &sa));
  testrun(ov_stun_mapped_address_encode(buffer, 12, NULL, &sa));
  testrun(STUN_MAPPED_ADDRESS == ov_stun_attribute_get_type(buffer, size));
  testrun(8 == ov_stun_attribute_get_length(buffer, size));
  testrun(buffer[4] == 0);
  testrun(buffer[5] == 1);
  testrun(buffer[6] == 0xff);
  testrun(buffer[7] == 0x01);
  testrun(buffer[8] == 0x7f);
  testrun(buffer[9] == 0);
  testrun(buffer[10] == 0);
  testrun(buffer[11] == 0x01);

  testrun(ov_stun_mapped_address_encode(buffer, size, &next, &sa));
  testrun(STUN_MAPPED_ADDRESS == ov_stun_attribute_get_type(buffer, size));
  testrun(8 == ov_stun_attribute_get_length(buffer, size));
  testrun(buffer[4] == 0);
  testrun(buffer[5] == 1);
  testrun(buffer[6] == 0xff);
  testrun(buffer[7] == 0x01);
  testrun(buffer[8] == 0x7f);
  testrun(buffer[9] == 0);
  testrun(buffer[10] == 0);
  testrun(buffer[11] == 0x01);
  testrun(next == buffer + 12);

  sock4->sin_port = htons(0xface);
  testrun(1 == inet_pton(AF_INET, "255.255.255.255", &sock4->sin_addr));
  testrun(ov_stun_mapped_address_encode(buffer, size, &next, &sa));
  testrun(STUN_MAPPED_ADDRESS == ov_stun_attribute_get_type(buffer, size));
  testrun(8 == ov_stun_attribute_get_length(buffer, size));
  testrun(buffer[4] == 0);
  testrun(buffer[5] == 1);
  testrun(buffer[6] == 0xfa);
  testrun(buffer[7] == 0xce);
  testrun(buffer[8] == 0xff);
  testrun(buffer[9] == 0xff);
  testrun(buffer[10] == 0xff);
  testrun(buffer[11] == 0xff);
  testrun(next = buffer + 8);

  // write IPv6
  sock6 = (struct sockaddr_in6 *)&sa;
  sock6->sin6_family = AF_INET6;
  sock6->sin6_port = htons(0xface);
  testrun(1 == inet_pton(AF_INET6, "::", &sock6->sin6_addr));

  testrun(!ov_stun_mapped_address_encode(buffer, 22, &next, &sa));
  testrun(!ov_stun_mapped_address_encode(buffer, 23, &next, &sa));

  testrun(ov_stun_mapped_address_encode(buffer, 24, &next, &sa));
  testrun(STUN_MAPPED_ADDRESS == ov_stun_attribute_get_type(buffer, size));
  testrun(20 == ov_stun_attribute_get_length(buffer, size));
  testrun(buffer[4] == 0);
  testrun(buffer[5] == 1);
  testrun(buffer[6] == 0xfa);
  testrun(buffer[7] == 0xce);
  testrun(buffer[8] == 0x00);
  testrun(buffer[9] == 0x00);
  testrun(buffer[10] == 0x00);
  testrun(buffer[11] == 0x00);
  testrun(buffer[12] == 0x00);
  testrun(buffer[13] == 0x00);
  testrun(buffer[14] == 0x00);
  testrun(buffer[15] == 0x00);
  testrun(buffer[16] == 0x00);
  testrun(buffer[17] == 0x00);
  testrun(buffer[18] == 0x00);
  testrun(buffer[19] == 0x00);
  testrun(buffer[20] == 0x00);
  testrun(buffer[21] == 0x00);
  testrun(buffer[22] == 0x00);
  testrun(buffer[23] == 0x00);
  testrun(next = buffer + 24);

  sock6 = (struct sockaddr_in6 *)&sa;
  sock6->sin6_family = AF_INET6;
  sock6->sin6_port = htons(0xcaad);
  testrun(1 == inet_pton(AF_INET6, "face:aaaa:bbbb:cccc:dddd:eeee:ffff:1234",
                         &sock6->sin6_addr));
  testrun(ov_stun_mapped_address_encode(buffer, 24, &next, &sa));
  testrun(STUN_MAPPED_ADDRESS == ov_stun_attribute_get_type(buffer, size));
  testrun(20 == ov_stun_attribute_get_length(buffer, size));
  testrun(buffer[4] == 0);
  testrun(buffer[5] == 1);
  testrun(buffer[6] == 0xca);
  testrun(buffer[7] == 0xad);
  testrun(buffer[8] == 0xfa);
  testrun(buffer[9] == 0xce);
  testrun(buffer[10] == 0xaa);
  testrun(buffer[11] == 0xaa);
  testrun(buffer[12] == 0xbb);
  testrun(buffer[13] == 0xbb);
  testrun(buffer[14] == 0xcc);
  testrun(buffer[15] == 0xcc);
  testrun(buffer[16] == 0xdd);
  testrun(buffer[17] == 0xdd);
  testrun(buffer[18] == 0xee);
  testrun(buffer[19] == 0xee);
  testrun(buffer[20] == 0xff);
  testrun(buffer[21] == 0xff);
  testrun(buffer[22] == 0x12);
  testrun(buffer[23] == 0x34);
  testrun(next = buffer + 24);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_mapped_address_decode() {

  size_t size = 100;
  uint8_t buf[size];
  uint8_t *buffer = buf;
  memset(buf, size, 100);

  struct sockaddr_storage *address = NULL;
  ;
  struct sockaddr_in6 *sock6 = NULL;
  struct sockaddr_in *sock4 = NULL;

  char string[INET6_ADDRSTRLEN] = {0};

  // prepare valid IPv4
  testrun(ov_stun_attribute_set_type(buffer, size, STUN_MAPPED_ADDRESS));
  testrun(ov_stun_attribute_set_length(buffer, size, 8));
  buffer[4] = 0;
  buffer[5] = 1;
  *(uint16_t *)(buffer + 6) = htons(66);
  testrun(1 == inet_pton(AF_INET, "127.0.0.1", buffer + 8));

  testrun(!ov_stun_mapped_address_decode(NULL, 0, NULL));
  testrun(!ov_stun_mapped_address_decode(buffer, size, NULL));
  testrun(!ov_stun_mapped_address_decode(buffer, 0, &address));
  testrun(!ov_stun_mapped_address_decode(NULL, size, &address));

  // check min
  testrun(!ov_stun_mapped_address_decode(buffer, 11, &address));
  testrun(ov_stun_mapped_address_decode(buffer, 12, &address));
  testrun(address);
  sock4 = (struct sockaddr_in *)address;
  testrun(sock4->sin_family == AF_INET);
  testrun(sock4->sin_port == htons(66));
  testrun(inet_ntop(AF_INET, &sock4->sin_addr, string, INET6_ADDRSTRLEN));
  testrun(0 == strncmp(string, "127.0.0.1", strlen("127.0.0.1")));

  // ipv4 type wrong
  testrun(ov_stun_attribute_set_type(buffer, size, 0));
  testrun(!ov_stun_mapped_address_decode(buffer, size, &address));
  testrun(ov_stun_attribute_set_type(buffer, size, STUN_MAPPED_ADDRESS));
  testrun(ov_stun_mapped_address_decode(buffer, size, &address));

  // ipv4 length wrong
  testrun(ov_stun_attribute_set_length(buffer, size, 20));
  testrun(!ov_stun_mapped_address_decode(buffer, size, &address));
  testrun(ov_stun_attribute_set_length(buffer, size, 8));
  testrun(ov_stun_mapped_address_decode(buffer, size, &address));

  // ipv4 family wrong
  buffer[5] = 3;
  testrun(!ov_stun_mapped_address_decode(buffer, size, &address));
  buffer[5] = 1;
  testrun(ov_stun_mapped_address_decode(buffer, size, &address));

  // prepare valid IPv6
  testrun(ov_stun_attribute_set_type(buffer, size, STUN_MAPPED_ADDRESS));
  testrun(ov_stun_attribute_set_length(buffer, size, 20));
  buffer[4] = 0;
  buffer[5] = 2;
  *(uint16_t *)(buffer + 6) = htons(12345);
  testrun(1 == inet_pton(AF_INET6, "2001:db8:85a3:8d3:1319:8a2e:370:7348",
                         buffer + 8));
  testrun(ov_stun_mapped_address_decode(buffer, size, &address));
  testrun(address);
  sock6 = (struct sockaddr_in6 *)address;
  testrun(sock6->sin6_family == AF_INET6);
  testrun(sock6->sin6_port == htons(12345));
  testrun(inet_ntop(AF_INET6, &sock6->sin6_addr, string, INET6_ADDRSTRLEN));
  testrun(0 == strncmp(string, "2001:db8:85a3:8d3:1319:8a2e:370:7348",
                       strlen("2001:db8:85a3:8d3:1319:8a2e:370:7348")));

  // ipv6 type wrong
  testrun(ov_stun_attribute_set_type(buffer, size, 0));
  testrun(!ov_stun_mapped_address_decode(buffer, size, &address));
  testrun(ov_stun_attribute_set_type(buffer, size, STUN_MAPPED_ADDRESS));
  testrun(ov_stun_mapped_address_decode(buffer, size, &address));

  // ipv6 length wrong
  testrun(ov_stun_attribute_set_length(buffer, size, 8));
  testrun(!ov_stun_mapped_address_decode(buffer, size, &address));
  testrun(ov_stun_attribute_set_length(buffer, size, 20));
  testrun(ov_stun_mapped_address_decode(buffer, size, &address));

  // ipv6 family wrong
  buffer[5] = 3;
  testrun(!ov_stun_mapped_address_decode(buffer, size, &address));
  buffer[5] = 2;
  testrun(ov_stun_mapped_address_decode(buffer, size, &address));

  free(address);
  return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

  testrun_init();
  testrun_test(test_ov_stun_attribute_frame_is_mapped_address);

  testrun_test(test_ov_stun_mapped_address_encoding_length);
  testrun_test(test_ov_stun_mapped_address_encode);
  testrun_test(test_ov_stun_mapped_address_decode);

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
