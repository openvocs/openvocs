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

        https://github.com/RustAudio/ogg/blob/master/src/crc.rs
        https://crccalc.com/

        ------------------------------------------------------------------------
*/

#include "../include/ov_crc32.h"
#include "../include/ov_utils.h"

/*----------------------------------------------------------------------------*/

static uint32_t reflect_bits(uint32_t n) {

  uint32_t reflected = 0;

  for (size_t i = 0; i < 32; ++i) {
    reflected = (reflected << 1) | (n & 1);
    n = (n >> 1) & 0x7fffffff;
  }

  return reflected;
}

/*----------------------------------------------------------------------------*/

void print_table(FILE *out, uint32_t lookup[0xff]) {

  for (size_t i = 0; i < 0xff; ++i) {
    fprintf(out, "%zu: %" PRIu32 "    ", i, lookup[i]);
  }
}

/*----------------------------------------------------------------------------*/

static uint32_t calc_lookup_entry(uint32_t i, uint32_t poly) {

  uint32_t r = (i << 24) & 0xffffff00;

  for (size_t i = 0; i < 8; ++i) {

    if (0x80000000 == (r & 0x80000000)) {
      r = (r << 1) ^ poly;
    } else {
      r = (r << 1);
    }
  }

  return r;
}

/*----------------------------------------------------------------------------*/

static uint32_t calc_lookup_entry_reflected(uint32_t i, uint32_t poly) {

  // Performance of this function does not matter too much
  uint32_t r = i;

  for (size_t i = 0; i < 8; ++i) {

    if (1 == (r & 1)) {
      r = (r >> 1) & 0x7fffffff;
      r ^= poly;
    } else {
      r = (r >> 1) & 0x7fffffff;
    }
  }

  return r;
}

/*----------------------------------------------------------------------------*/

bool ov_crc32_generate_table_for(uint32_t polynomial, bool reverted_in,
                                 uint32_t lookup[0x100]) {

  if (ov_ptr_valid(lookup, "Cannot generate CRC32 table: No target memory")) {

    if (reverted_in) {

      polynomial = reflect_bits(polynomial);

      for (uint32_t i = 0; i < 0x100; ++i) {
        lookup[i] = calc_lookup_entry_reflected(i, polynomial);
      }

    } else {

      for (uint32_t i = 0; i < 0x100; ++i) {
        lookup[i] = calc_lookup_entry(i, polynomial);
      }
    }

    return true;

  } else {
    return false;
  }
}

/*----------------------------------------------------------------------------*/

static uint32_t crc32_straight_in(uint32_t crc, uint8_t const *data,
                                  size_t len_octets, uint32_t lookup[0x100]) {

  // fprintf(stderr, "Calculating checksum: ");
  for (size_t i = 0; i < len_octets; ++i) {
    //     fprintf(stderr, "%" PRIu8 " ", data[i]);
    crc = (crc << 8) ^ lookup[data[i] ^ ((crc >> 24) & 0x000000ff)];
  }
  fprintf(stderr, "\n");

  return crc;
}

/*----------------------------------------------------------------------------*/

static uint32_t crc32_reflected_in(uint32_t crc, uint8_t const *data,
                                   size_t len_octets, uint32_t lookup[0x100]) {

  for (size_t i = 0; i < len_octets; ++i) {
    crc = (crc >> 8) ^ lookup[data[i] ^ (crc & 0xff)];
  }

  return crc;
}

/*----------------------------------------------------------------------------*/

uint32_t ov_crc32_sum(uint32_t crc, uint8_t const *data, size_t len_octets,
                      bool rfin, bool rfout, uint32_t lookup[0x100]) {

  if (rfin) {

    crc = crc32_reflected_in(crc, data, len_octets, lookup);

    if (rfout) {
      return crc;
    } else {
      return reflect_bits(crc);
    }

  } else {

    crc = crc32_straight_in(crc, data, len_octets, lookup);

    if (rfout) {
      return reflect_bits(crc);
    } else {
      return crc;
    }
  }
}

/*----------------------------------------------------------------------------*/

uint32_t ov_crc32_posix(uint8_t const *data, size_t len_octets) {

  // Parameters are the same as for OGG, just the final checksum has to be
  // xored with 0xffffffff

  return 0xffffffff ^ ov_crc32_ogg(0, data, len_octets);
}

/*----------------------------------------------------------------------------*/

uint32_t ov_crc32_ogg(uint32_t init, uint8_t const *data, size_t len_octets) {

  static bool initialized = false;
  static uint32_t lookup[0x100] = {0};

  if (!initialized) {
    ov_crc32_generate_table_for(0x04c11db7, false, lookup);
    initialized = true;
  }

  return ov_crc32_sum(init, data, len_octets, false, false, lookup);
}

/*----------------------------------------------------------------------------*/

uint32_t ov_crc32_zlib(uint32_t init, uint8_t const *data, size_t len_octets) {

  static uint32_t table[0x100] = {0};
  static bool initialized = false;

  if (!initialized) {
    ov_crc32_generate_table_for(0x04c11db7, true, table);
    initialized = true;
  }

  return 0xffffffff ^
         ov_crc32_sum(0xffffffff ^ init, data, len_octets, true, true, table);
}

/*----------------------------------------------------------------------------*/
