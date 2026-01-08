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
        @file           ov_stun_xor_mapped_address_test.c
        @author         Markus Toepfer

        @date           2018-10-19

        @ingroup        ov_stun

        @brief          Unit tests of RFC 5389 attribute "xor mapped address"


        ------------------------------------------------------------------------
*/

#include "ov_stun_xor_mapped_address.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_stun_attribute_frame_is_xor_mapped_address() {

  size_t size = 100;
  uint8_t buf[size];
  uint8_t *buffer = buf;

  memset(buf, size, 100);

  // prepare valid frame
  testrun(ov_stun_attribute_set_type(buffer, size, STUN_XOR_MAPPED_ADDRESS));
  testrun(ov_stun_attribute_set_length(buffer, size, 8));
  *(buf + 5) = 0x01;

  testrun(ov_stun_attribute_frame_is_xor_mapped_address(buffer, size));

  testrun(!ov_stun_attribute_frame_is_xor_mapped_address(NULL, size));
  testrun(!ov_stun_attribute_frame_is_xor_mapped_address(buffer, 0));
  testrun(!ov_stun_attribute_frame_is_xor_mapped_address(buffer, 11));

  // min buffer min valid
  testrun(ov_stun_attribute_frame_is_xor_mapped_address(buffer, 12));

  // check ipv4 + length
  for (int i = 0; i < 0xff; i++) {

    *(buf + 5) = i;
    if (i == 0x01) {
      testrun(ov_stun_attribute_frame_is_xor_mapped_address(buffer, size));
    } else {
      testrun(!ov_stun_attribute_frame_is_xor_mapped_address(buffer, size));
    }

    *(buf + 5) = 0x01;
    testrun(ov_stun_attribute_set_length(buffer, size, i));
    if (i == 8) {
      testrun(ov_stun_attribute_frame_is_xor_mapped_address(buffer, size));
    } else {
      testrun(!ov_stun_attribute_frame_is_xor_mapped_address(buffer, size));
    }

    *(buf + 5) = 0x01;
    testrun(ov_stun_attribute_set_length(buffer, size, 8));

    // ignore first bytes
    *(buf + 4) = i;
    testrun(ov_stun_attribute_frame_is_xor_mapped_address(buffer, size));
  }

  // check ipv6 + length
  for (int i = 0; i < 0xff; i++) {

    *(buf + 5) = i;
    testrun(ov_stun_attribute_set_length(buffer, size, 20));
    if (i == 0x02) {
      testrun(ov_stun_attribute_frame_is_xor_mapped_address(buffer, size));
    } else {
      testrun(!ov_stun_attribute_frame_is_xor_mapped_address(buffer, size));
    }

    *(buf + 5) = 0x02;
    testrun(ov_stun_attribute_set_length(buffer, size, i));

    if (i == 20) {
      testrun(ov_stun_attribute_frame_is_xor_mapped_address(buffer, size));
    } else {
      testrun(!ov_stun_attribute_frame_is_xor_mapped_address(buffer, size));
    }

    *(buf + 5) = 0x02;
    testrun(ov_stun_attribute_set_length(buffer, size, 20));

    // ignore 4 bytes
    *(buf + 4) = i;
    testrun(ov_stun_attribute_frame_is_xor_mapped_address(buffer, size));

    *(buf + 4) = 0x00;
    *(buf + 5) = 0x02;
    testrun(ov_stun_attribute_set_length(buffer, size, 20));

    testrun(ov_stun_attribute_set_type(buffer, size, i));
    if (i == STUN_XOR_MAPPED_ADDRESS) {
      testrun(ov_stun_attribute_frame_is_xor_mapped_address(buffer, size));
    } else {
      testrun(!ov_stun_attribute_frame_is_xor_mapped_address(buffer, size));
    }
    testrun(ov_stun_attribute_set_type(buffer, size, STUN_XOR_MAPPED_ADDRESS));
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_xor_mapped_address_encoding_length() {

  struct sockaddr_storage sa = {0};

  sa.ss_family = AF_UNIX;
  testrun(0 == ov_stun_xor_mapped_address_encoding_length(&sa));

  sa.ss_family = AF_INET;
  testrun(12 == ov_stun_xor_mapped_address_encoding_length(&sa));

  sa.ss_family = AF_INET6;
  testrun(24 == ov_stun_xor_mapped_address_encoding_length(&sa));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_xor_mapped_address_encode() {

  size_t size = 100;
  uint8_t buf[size];
  uint8_t *buffer = buf;
  uint8_t *next = NULL;

  uint8_t head[20] = {0};
  uint16_t port = 0;
  uint32_t ip = 0;

  memset(buf, size, 100);

  struct sockaddr_storage sa = {0};
  struct sockaddr_in6 *sock6 = NULL;
  struct sockaddr_in *sock4 = NULL;

  // prepare a (min required) head
  head[0] = 0;
  head[1] = 0;
  head[2] = 0;
  head[3] = 0;
  head[4] = 0x21;
  head[5] = 0x12;
  head[6] = 0xA4;
  head[7] = 0x42;
  head[8] = 0;
  head[9] = 0;
  head[10] = 0;
  head[11] = 0;
  head[12] = 0;
  head[13] = 0;
  head[14] = 0;
  head[15] = 0;
  head[16] = 0;
  head[17] = 0;
  head[18] = 0;
  head[19] = 0;

  // prepare sock4
  sock4 = (struct sockaddr_in *)&sa;
  sock4->sin_family = AF_INET;
  sock4->sin_port = htons(0xff01);
  testrun(1 == inet_pton(AF_INET, "127.0.0.1", &sock4->sin_addr));

  testrun(!ov_stun_xor_mapped_address_encode(NULL, 0, NULL, NULL, NULL));
  testrun(!ov_stun_xor_mapped_address_encode(NULL, size, head, NULL, &sa));
  testrun(!ov_stun_xor_mapped_address_encode(buffer, 0, head, NULL, &sa));
  testrun(!ov_stun_xor_mapped_address_encode(buffer, size, head, NULL, NULL));
  testrun(!ov_stun_xor_mapped_address_encode(buffer, size, NULL, NULL, &sa));

  testrun(!ov_stun_xor_mapped_address_encode(buffer, 11, head, NULL, &sa));
  testrun(ov_stun_xor_mapped_address_encode(buffer, 12, head, NULL, &sa));
  testrun(STUN_XOR_MAPPED_ADDRESS == ov_stun_attribute_get_type(buffer, size));
  testrun(8 == ov_stun_attribute_get_length(buffer, size));
  testrun(buffer[4] == 0);
  testrun(buffer[5] == 1);
  port = *(uint16_t *)(buffer + 6);
  testrun(port == htons(0xff01 ^ 0x2112));
  // check IP encoding
  ip = (ntohl(*(uint32_t *)(buffer + 8)) ^ 0x2112A442);
  testrun(ip == 0x7f000001);

  // different data
  sock4->sin_port = htons(12345);
  testrun(1 == inet_pton(AF_INET, "255.1.2.3", &sock4->sin_addr));
  testrun(ov_stun_xor_mapped_address_encode(buffer, 12, head, NULL, &sa));
  testrun(ov_stun_xor_mapped_address_encode(buffer, 12, head, &next, &sa));
  testrun(STUN_XOR_MAPPED_ADDRESS == ov_stun_attribute_get_type(buffer, size));
  testrun(8 == ov_stun_attribute_get_length(buffer, size));
  testrun(next = buffer + 12);
  testrun(buffer[4] == 0);
  testrun(buffer[5] == 1);
  port = *(uint16_t *)(buffer + 6);
  testrun(port == htons(12345 ^ 0x2112));
  // check IP encoding
  ip = (ntohl(*(uint32_t *)(buffer + 8)) ^ 0x2112A442);
  testrun(ip == 0xff010203);

  // IPv6
  sock6 = (struct sockaddr_in6 *)&sa;
  sock6->sin6_family = AF_INET6;
  sock6->sin6_port = htons(0xff01);
  testrun(1 == inet_pton(AF_INET6, "face:aaaa:bbbb:cccc:dddd:eeee:f1f1:1234",
                         &sock6->sin6_addr));
  testrun(!ov_stun_xor_mapped_address_encode(buffer, 12, head, &next, &sa));
  testrun(!ov_stun_xor_mapped_address_encode(buffer, 23, head, &next, &sa));
  testrun(ov_stun_xor_mapped_address_encode(buffer, 24, head, &next, &sa));
  testrun(STUN_XOR_MAPPED_ADDRESS == ov_stun_attribute_get_type(buffer, size));
  testrun(20 == ov_stun_attribute_get_length(buffer, size));
  testrun(next = buffer + 24);
  testrun(buffer[4] == 0);
  testrun(buffer[5] == 2);
  port = *(uint16_t *)(buffer + 6);
  testrun(port == htons(0xff01 ^ 0x2112));

  // check IP
  testrun(buffer[8 + 0] == (0xfa ^ head[4]));
  testrun(buffer[8 + 1] == (0xce ^ head[5]));
  testrun(buffer[8 + 2] == (0xaa ^ head[6]));
  testrun(buffer[8 + 3] == (0xaa ^ head[7]));
  testrun(buffer[8 + 4] == (0xbb ^ head[8]));
  testrun(buffer[8 + 5] == (0xbb ^ head[9]));
  testrun(buffer[8 + 6] == (0xcc ^ head[10]));
  testrun(buffer[8 + 7] == (0xcc ^ head[11]));
  testrun(buffer[8 + 8] == (0xdd ^ head[12]));
  testrun(buffer[8 + 9] == (0xdd ^ head[13]));
  testrun(buffer[8 + 10] == (0xee ^ head[14]));
  testrun(buffer[8 + 11] == (0xee ^ head[15]));
  testrun(buffer[8 + 12] == (0xf1 ^ head[16]));
  testrun(buffer[8 + 13] == (0xf1 ^ head[17]));
  testrun(buffer[8 + 14] == (0x12 ^ head[18]));
  testrun(buffer[8 + 15] == (0x34 ^ head[19]));

  // different setting
  sock6->sin6_port = htons(12345);
  testrun(1 == inet_pton(AF_INET6, "::1", &sock6->sin6_addr));
  testrun(ov_stun_xor_mapped_address_encode(buffer, size, head, &next, &sa));
  testrun(STUN_XOR_MAPPED_ADDRESS == ov_stun_attribute_get_type(buffer, size));
  testrun(20 == ov_stun_attribute_get_length(buffer, size));
  testrun(next = buffer + 24);
  testrun(buffer[4] == 0);
  testrun(buffer[5] == 2);
  port = *(uint16_t *)(buffer + 6);
  testrun(port == htons(12345 ^ 0x2112));

  // check IP
  testrun(buffer[8 + 0] == (0x00 ^ head[4]));
  testrun(buffer[8 + 1] == (0x00 ^ head[5]));
  testrun(buffer[8 + 2] == (0x00 ^ head[6]));
  testrun(buffer[8 + 3] == (0x00 ^ head[7]));
  testrun(buffer[8 + 4] == (0x00 ^ head[8]));
  testrun(buffer[8 + 5] == (0x00 ^ head[9]));
  testrun(buffer[8 + 6] == (0x00 ^ head[10]));
  testrun(buffer[8 + 7] == (0x00 ^ head[11]));
  testrun(buffer[8 + 8] == (0x00 ^ head[12]));
  testrun(buffer[8 + 9] == (0x00 ^ head[13]));
  testrun(buffer[8 + 10] == (0x00 ^ head[14]));
  testrun(buffer[8 + 11] == (0x00 ^ head[15]));
  testrun(buffer[8 + 12] == (0x00 ^ head[16]));
  testrun(buffer[8 + 13] == (0x00 ^ head[17]));
  testrun(buffer[8 + 14] == (0x00 ^ head[18]));
  testrun(buffer[8 + 15] == (0x01 ^ head[19]));

  // check with different transaction id
  head[0] = 0;
  head[1] = 0;
  head[2] = 0;
  head[3] = 0;
  head[4] = 0x21;
  head[5] = 0x12;
  head[6] = 0xA4;
  head[7] = 0x42;
  head[8] = 0x1f;
  head[9] = 0x2f;
  head[10] = 0x3f;
  head[11] = 0x4f;
  head[12] = 0x5f;
  head[13] = 0x6f;
  head[14] = 0x7f;
  head[15] = 0x8f;
  head[16] = 0x9f;
  head[17] = 0xaf;
  head[18] = 0xbf;
  head[19] = 0xcf;

  testrun(ov_stun_xor_mapped_address_encode(buffer, size, head, &next, &sa));
  testrun(STUN_XOR_MAPPED_ADDRESS == ov_stun_attribute_get_type(buffer, size));
  testrun(20 == ov_stun_attribute_get_length(buffer, size));
  testrun(next = buffer + 24);
  testrun(buffer[4] == 0);
  testrun(buffer[5] == 2);
  port = *(uint16_t *)(buffer + 6);
  testrun(port == htons(12345 ^ 0x2112));

  // check IP
  testrun(buffer[8 + 0] == (0x00 ^ head[4]));
  testrun(buffer[8 + 1] == (0x00 ^ head[5]));
  testrun(buffer[8 + 2] == (0x00 ^ head[6]));
  testrun(buffer[8 + 3] == (0x00 ^ head[7]));
  testrun(buffer[8 + 4] == (0x00 ^ head[8]));
  testrun(buffer[8 + 5] == (0x00 ^ head[9]));
  testrun(buffer[8 + 6] == (0x00 ^ head[10]));
  testrun(buffer[8 + 7] == (0x00 ^ head[11]));
  testrun(buffer[8 + 8] == (0x00 ^ head[12]));
  testrun(buffer[8 + 9] == (0x00 ^ head[13]));
  testrun(buffer[8 + 10] == (0x00 ^ head[14]));
  testrun(buffer[8 + 11] == (0x00 ^ head[15]));
  testrun(buffer[8 + 12] == (0x00 ^ head[16]));
  testrun(buffer[8 + 13] == (0x00 ^ head[17]));
  testrun(buffer[8 + 14] == (0x00 ^ head[18]));
  testrun(buffer[8 + 15] == (0x01 ^ head[19]));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_xor_mapped_address_decode() {

  struct sockaddr_storage *address = NULL;
  ;
  struct sockaddr_in6 *sock6 = NULL;
  struct sockaddr_in *sock4 = NULL;

  uint32_t cookie = 0x2112A442;

  size_t size = 100;
  uint8_t buf[size];
  memset(buf, 0, size);
  uint8_t *buffer = buf;

  char string[INET6_ADDRSTRLEN] = {0};
  uint8_t head[20] = {0};

  uint32_t ip = 0xdadaface;

  // prepare valid IPv4
  testrun(ov_stun_attribute_set_type(buffer, size, STUN_XOR_MAPPED_ADDRESS));
  testrun(ov_stun_attribute_set_length(buffer, size, 8));
  buffer[4] = 0;
  buffer[5] = 1;
  *(uint16_t *)(buffer + 6) = htons(1 ^ 0x2112);
  *(uint32_t *)(buffer + 8) = htonl(ip ^ cookie);

  testrun(!ov_stun_xor_mapped_address_decode(NULL, 0, head, NULL));
  testrun(!ov_stun_xor_mapped_address_decode(buffer, size, head, NULL));
  testrun(!ov_stun_xor_mapped_address_decode(buffer, 0, head, &address));
  testrun(!ov_stun_xor_mapped_address_decode(NULL, size, head, &address));
  testrun(!ov_stun_xor_mapped_address_decode(buffer, size, NULL, &address));

  // check min
  testrun(!ov_stun_xor_mapped_address_decode(buffer, 11, head, &address));
  testrun(ov_stun_xor_mapped_address_decode(buffer, 12, head, &address));
  testrun(address);
  sock4 = (struct sockaddr_in *)address;
  testrun(sock4->sin_family == AF_INET);
  testrun(sock4->sin_port == htons(1));
  testrun(sock4->sin_addr.s_addr == htonl(ip));

  // ipv4 type wrong
  testrun(ov_stun_attribute_set_type(buffer, size, 0));
  testrun(!ov_stun_xor_mapped_address_decode(buffer, size, head, &address));
  testrun(ov_stun_attribute_set_type(buffer, size, STUN_XOR_MAPPED_ADDRESS));
  testrun(ov_stun_xor_mapped_address_decode(buffer, size, head, &address));

  // ipv4 length wrong
  testrun(ov_stun_attribute_set_length(buffer, size, 20));
  testrun(!ov_stun_xor_mapped_address_decode(buffer, size, head, &address));
  testrun(ov_stun_attribute_set_length(buffer, size, 8));
  testrun(ov_stun_xor_mapped_address_decode(buffer, size, head, &address));

  // ipv4 family wrong
  buffer[5] = 3;
  testrun(!ov_stun_xor_mapped_address_decode(buffer, size, head, &address));
  buffer[5] = 1;
  testrun(ov_stun_xor_mapped_address_decode(buffer, size, head, &address));

  memset(buffer, 0, 100);
  testrun(ov_stun_attribute_set_type(buffer, size, STUN_XOR_MAPPED_ADDRESS));
  testrun(ov_stun_attribute_set_length(buffer, size, 8));
  buffer[4] = 0;
  buffer[5] = 1;
  *(uint16_t *)(buffer + 6) = htons(1 ^ 0x2112);
  testrun(1 == inet_pton(AF_INET, "1.2.3.4", buffer + 8));

  ip = ntohl(*(uint32_t *)(buffer + 8));
  *(uint32_t *)(buffer + 8) = htonl(ip ^ cookie);
  memset(string, 0, INET6_ADDRSTRLEN);

  testrun(ov_stun_xor_mapped_address_decode(buffer, 12, head, &address));
  testrun(address);
  sock4 = (struct sockaddr_in *)address;
  testrun(sock4->sin_family == AF_INET);
  testrun(sock4->sin_port == htons(1));
  testrun(sock4->sin_addr.s_addr == htonl(ip));
  testrun(inet_ntop(AF_INET, &sock4->sin_addr, string, INET6_ADDRSTRLEN));
  testrun(0 == strncmp(string, "1.2.3.4", strlen("1.2.3.4")));

  // IPv6
  testrun(ov_stun_attribute_set_type(buffer, size, STUN_XOR_MAPPED_ADDRESS));
  testrun(ov_stun_attribute_set_length(buffer, size, 20));
  buffer[4] = 0;
  buffer[5] = 2;
  *(uint16_t *)(buffer + 6) = htons(1 ^ 0x2112);
  buffer[8] = 0xfa;
  buffer[9] = 0xce;
  buffer[10] = 0xaa;
  buffer[11] = 0xaa;
  buffer[12] = 0xbb;
  buffer[13] = 0xbb;
  buffer[14] = 0xcc;
  buffer[15] = 0xcc;
  buffer[16] = 0xdd;
  buffer[17] = 0xdd;
  buffer[18] = 0xee;
  buffer[19] = 0xee;
  buffer[20] = 0xf1;
  buffer[21] = 0xf1;
  buffer[22] = 0x12;
  buffer[23] = 0x34;

  // prepare a head
  head[0] = 0;
  head[1] = 0;
  head[2] = 0;
  head[3] = 0;
  head[4] = 0x21;
  head[5] = 0x12;
  head[6] = 0xA4;
  head[7] = 0x42;
  head[8] = 0;
  head[9] = 0;
  head[10] = 0;
  head[11] = 0;
  head[12] = 0;
  head[13] = 0;
  head[14] = 0;
  head[15] = 0;
  head[16] = 0;
  head[17] = 0;
  head[18] = 0;
  head[19] = 0;

  testrun(ov_stun_xor_mapped_address_decode(buffer, size, head, &address));
  testrun(address);
  sock6 = (struct sockaddr_in6 *)address;
  testrun(sock6->sin6_family == AF_INET6);
  testrun(sock6->sin6_port == htons(1));

  // check IP
  testrun(sock6->sin6_addr.s6_addr[0] == (0xfa ^ head[4]));
  testrun(sock6->sin6_addr.s6_addr[1] == (0xce ^ head[5]));
  testrun(sock6->sin6_addr.s6_addr[2] == (0xaa ^ head[6]));
  testrun(sock6->sin6_addr.s6_addr[3] == (0xaa ^ head[7]));
  testrun(sock6->sin6_addr.s6_addr[4] == (0xbb ^ head[8]));
  testrun(sock6->sin6_addr.s6_addr[5] == (0xbb ^ head[9]));
  testrun(sock6->sin6_addr.s6_addr[6] == (0xcc ^ head[10]));
  testrun(sock6->sin6_addr.s6_addr[7] == (0xcc ^ head[11]));
  testrun(sock6->sin6_addr.s6_addr[8] == (0xdd ^ head[12]));
  testrun(sock6->sin6_addr.s6_addr[9] == (0xdd ^ head[13]));
  testrun(sock6->sin6_addr.s6_addr[10] == (0xee ^ head[14]));
  testrun(sock6->sin6_addr.s6_addr[11] == (0xee ^ head[15]));
  testrun(sock6->sin6_addr.s6_addr[12] == (0xf1 ^ head[16]));
  testrun(sock6->sin6_addr.s6_addr[13] == (0xf1 ^ head[17]));
  testrun(sock6->sin6_addr.s6_addr[14] == (0x12 ^ head[18]));
  testrun(sock6->sin6_addr.s6_addr[15] == (0x34 ^ head[19]));

  // use of different transaction id (same input buffer)

  head[0] = 0;
  head[1] = 0;
  head[2] = 0;
  head[3] = 0;
  head[4] = 0x21;
  head[5] = 0x12;
  head[6] = 0xA4;
  head[7] = 0x42;
  head[8] = 0x1f;
  head[9] = 0x1f;
  head[10] = 0x1f;
  head[11] = 0x1f;
  head[12] = 0x1f;
  head[13] = 0x1f;
  head[14] = 0x1f;
  head[15] = 0x1f;
  head[16] = 0x1f;
  head[17] = 0x1f;
  head[18] = 0x1f;
  head[19] = 0x1f;
  testrun(ov_stun_xor_mapped_address_decode(buffer, size, head, &address));
  testrun(address);
  sock6 = (struct sockaddr_in6 *)address;
  testrun(sock6->sin6_family == AF_INET6);
  testrun(sock6->sin6_port == htons(1));

  // check IP
  testrun(sock6->sin6_addr.s6_addr[0] == (0xfa ^ head[4]));
  testrun(sock6->sin6_addr.s6_addr[1] == (0xce ^ head[5]));
  testrun(sock6->sin6_addr.s6_addr[2] == (0xaa ^ head[6]));
  testrun(sock6->sin6_addr.s6_addr[3] == (0xaa ^ head[7]));
  testrun(sock6->sin6_addr.s6_addr[4] == (0xbb ^ head[8]));
  testrun(sock6->sin6_addr.s6_addr[5] == (0xbb ^ head[9]));
  testrun(sock6->sin6_addr.s6_addr[6] == (0xcc ^ head[10]));
  testrun(sock6->sin6_addr.s6_addr[7] == (0xcc ^ head[11]));
  testrun(sock6->sin6_addr.s6_addr[8] == (0xdd ^ head[12]));
  testrun(sock6->sin6_addr.s6_addr[9] == (0xdd ^ head[13]));
  testrun(sock6->sin6_addr.s6_addr[10] == (0xee ^ head[14]));
  testrun(sock6->sin6_addr.s6_addr[11] == (0xee ^ head[15]));
  testrun(sock6->sin6_addr.s6_addr[12] == (0xf1 ^ head[16]));
  testrun(sock6->sin6_addr.s6_addr[13] == (0xf1 ^ head[17]));
  testrun(sock6->sin6_addr.s6_addr[14] == (0x12 ^ head[18]));
  testrun(sock6->sin6_addr.s6_addr[15] == (0x34 ^ head[19]));

  // ipv6 type wrong
  testrun(ov_stun_attribute_set_type(buffer, size, 0));
  testrun(!ov_stun_xor_mapped_address_decode(buffer, size, head, &address));
  testrun(ov_stun_attribute_set_type(buffer, size, STUN_XOR_MAPPED_ADDRESS));
  testrun(ov_stun_xor_mapped_address_decode(buffer, size, head, &address));

  // ipv6 length wrong
  testrun(ov_stun_attribute_set_length(buffer, size, 8));
  testrun(!ov_stun_xor_mapped_address_decode(buffer, size, head, &address));
  testrun(ov_stun_attribute_set_length(buffer, size, 20));
  testrun(ov_stun_xor_mapped_address_decode(buffer, size, head, &address));

  // ipv6 family wrong
  buffer[5] = 3;
  testrun(!ov_stun_xor_mapped_address_decode(buffer, size, head, &address));
  buffer[5] = 2;
  testrun(ov_stun_xor_mapped_address_decode(buffer, size, head, &address));

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
  testrun_test(test_ov_stun_attribute_frame_is_xor_mapped_address);

  testrun_test(test_ov_stun_xor_mapped_address_encoding_length);
  testrun_test(test_ov_stun_xor_mapped_address_encode);
  testrun_test(test_ov_stun_xor_mapped_address_decode);

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
