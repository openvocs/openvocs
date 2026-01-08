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
#include "ov_crc32.c"
#include <ov_test/ov_test.h>

uint32_t start_values[] = {18, 97, 98, 99};

char const *test_string_init_not_null = "ihgfedcba";

uint32_t ref_crcs_init_not_null[] = {0xc534d97f, 0xc1a266f, 0x35971aaa,
                                     0x22ec0ee9};

/*----------------------------------------------------------------------------*/

static uint32_t checksum(uint32_t init, char const *str, bool rfin, bool rfout,
                         uint32_t lookup[0x100]) {

  return ov_crc32_sum(init, (uint8_t const *)str, strlen(str), rfin, rfout,
                      lookup);
}

/*----------------------------------------------------------------------------*/

int test_ov_crc32_sum() {

  testrun(0 == ov_crc32_sum(0, 0, 0, false, false, 0));

  uint32_t table[0x100] = {0};

  // We checka against known CRC32/Posix checksum values.
  // CRC32/Posix requires no bit reflection on in- or output,
  // with a final xor with 0xffffffff .
  ov_crc32_generate_table_for(0x04c11db7, false, table);

  // Checked against https://crccalc.com
  // The results of CRC32/Posix are the inverted results of our function
  testrun(0x579b24df == (0xffffffff ^ checksum(0, "a", false, false, table)));
  testrun(0xa36a6c29 == (0xffffffff ^ checksum(0, "bac", false, false, table)));
  testrun(0xe6220edb ==
          (0xffffffff ^ checksum(0, "bacarac", false, false, table)));

  // Chaining

  testrun(0xe6220edb ==
          (0xffffffff ^ checksum(checksum(0, "bac", false, false, table),
                                 "arac", false, false, table)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_crc32_posix() {

  // Checked against https://crccalc.com

  testrun(0x579b24df == ov_crc32_posix((uint8_t *)"a", 1));
  testrun(0xa36a6c29 == ov_crc32_posix((uint8_t *)"bac", 3));
  testrun(0xe6220edb == ov_crc32_posix((uint8_t *)"bacarac", 7));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static uint32_t crc32_ogg_equals(char const *str) {

  uint32_t crc = ov_crc32_ogg(0, (uint8_t const *)str, strlen(str));

  // fprintf(stderr, "%s    %x\n", str, ~crc);

  return crc;
}

/*----------------------------------------------------------------------------*/

static int test_ov_crc32_ogg() {

  // Ref value found in the net for OGG CRC32
  testrun(0x9f858776 == ov_crc32_ogg(0, (uint8_t const *)"==!", 3));

  testrun(crc32_ogg_equals("a"));

  uint8_t real_ogg[] = {
      0x4f, 0x67, 0x67, 0x53, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x74, 0xa3, 0x90, 0x5b, 0x00, 0x00, 0x00, 0x00,
      // Next would come the CRC, but the RFC requires us to
      // calculate the CRC with the CRC field set to all 0
      // 0x6d, 0x94, 0x4e, 0x3d
      0x00, 0x00, 0x00, 0x00, 0x01, 0x1e, 0x01, 0x76, 0x6f, 0x72, 0x62, 0x69,
      0x73, 0x00, 0x00, 0x00, 0x00, 0x02, 0x44, 0xac, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x80, 0xb5, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb8, 0x01};

  testrun(0x3d4e946d == ov_crc32_ogg(0, real_ogg, sizeof(real_ogg)));

  // Chaining
  testrun(0x3d4e946d == ov_crc32_ogg(ov_crc32_ogg(0, real_ogg, 12),
                                     real_ogg + 12, sizeof(real_ogg) - 12));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_crc32_zlib() {

  testrun(0x0 == ov_crc32_zlib(0, (uint8_t const *)"", 0));
  testrun(0xe8b7be43 == ov_crc32_zlib(0, (uint8_t const *)"a", 1));
  testrun(0x2ca74a14 == ov_crc32_zlib(0, (uint8_t const *)"ba", 2));
  testrun(0xd8aef480 == ov_crc32_zlib(0, (uint8_t const *)"cba", 3));
  testrun(0xb2ef92da == ov_crc32_zlib(0, (uint8_t const *)"dcba", 4));
  testrun(0x045b42e6 == ov_crc32_zlib(0, (uint8_t const *)"edcba", 5));
  testrun(0xad16f81f == ov_crc32_zlib(0, (uint8_t const *)"fedcba", 6));
  testrun(0xcfa9f4a9 == ov_crc32_zlib(0, (uint8_t const *)"gfedcba", 7));
  testrun(0x34e94fb0 == ov_crc32_zlib(0, (uint8_t const *)"hgfedcba", 8));
  testrun(0xc534d97f == ov_crc32_zlib(18, (uint8_t const *)"ihgfedcba", 9));
  testrun(0x0c1a266f == ov_crc32_zlib(97, (uint8_t const *)"ihgfedcba", 9));
  testrun(0x35971aaa == ov_crc32_zlib(98, (uint8_t const *)"ihgfedcba", 9));
  testrun(0x22ec0ee9 == ov_crc32_zlib(99, (uint8_t const *)"ihgfedcba", 9));

  testrun(0x0 == ov_crc32_zlib(0, (uint8_t const *)"", 0));
  testrun(0x41d03a30 == ov_crc32_zlib(0, (uint8_t const *)"abac", 4));

  testrun(0x41d03a30 ==
          ov_crc32_zlib(ov_crc32_zlib(0, (uint8_t const *)"ab", 2),
                        (uint8_t const *)"ac", 2));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_crc32", test_ov_crc32_sum, test_ov_crc32_posix,
            test_ov_crc32_ogg, test_ov_crc32_zlib);
