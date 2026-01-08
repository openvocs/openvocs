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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_CRC32_H
#define OV_CRC32_H
/*----------------------------------------------------------------------------*/

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Generate a CRC32 lookup table for a specific polynome.
 * If reveted_in is true, the table will be calculated for a crc32 that
 * expects its input bits to be reverted.
 */
bool ov_crc32_generate_table_for(uint32_t polynomial, bool reverted_in,
                                 uint32_t lookup[0x100]);

/**
 * Calculate a CRC32 checksum.
 *
 * The algorithm is such that for generator polynome 0x04C11DB7 and initial
 * value of 0 you get CRC32/OGG checksum.
 *
 * uint32_t ogg_table[0x100] = {0};
 * ov_crc32_generate_table_for(0x04c11db7, false, ogg_table);
 *
 * uint8_t chunk1[] = {...};
 * uint8_t chunk2[] = {...};
 *
 * uint32_t crc = ov_crc32_sum(0, chunk1, sizeof(chunk1), false, false,
 * ogg_table);
 *
 * You can do a rolling update of your CRC:
 *
 * crc = ov_crc32_sum(crc, chunk2, sizeof(chunk2), false, false, ogg_table);
 *
 *
 * Some checksum flavors require you to xor some value onto your checksum
 * after calculation:
 *
 * uint32_t crc32_posix = ov_crc32_sum(
 *                          0, chunk1, sizeof(chunk1), false, false, ogg_table);
 * crc32_posix = crc32_posix ^ 0xffffffff;
 *
 * Unfortunately, when doing a rolling update, beware of doing the xor
 * only after you are done:
 *
 * uint32_t crc32_posix = ov_crc32_sum(
 *                        0, chunk1, sizeof(chunk1), false, false, ogg_table);
 * crc32_posix = ov_crc32_sum(
 *               crc32_posix, chunk2, sizeof(chunk2), false, false, ogg_table);
 * crc32_posix = crc32_posix ^ 0xffffffff;
 *
 * Needless to say once you did the xor, you cannot update the crc further.
 *
 */
uint32_t ov_crc32_sum(uint32_t crc, uint8_t const *data, size_t len_octets,
                      bool rfin, bool rfout, uint32_t lookup[0x100]);

/*----------------------------------------------------------------------------*/

/**
 * Compute CRC32/Posix checksum.
 * Since the CRC has to be XORed with 0xffffffff, this function does not
 * provide rolling updates and hence no init value.
 * If you want do to a rolling posix crc calculation, use `ov_crc32_sum`
 * and finalize with XOR:
 *
 * uint32_t posix_table = {0};
 * ov_crc32_generate_table_for(0x04c11db7, false, posix_table);
 *
 * uint32_t crc32_posix = ov_crc32_sum(
 *                        0, chunk1, sizeof(chunk1), false, false, posix_table);
 *
 * Update the CRC:
 * crc32_posix = ov_crc32_sum(
 *               crc32_posix, chunk2, sizeof(chunk2), false, false,
 * posix_table);
 *
 * Finalize:
 * crc32_posix = crc32_posix ^ 0xffffffff;
 *
 *
 */
uint32_t ov_crc32_posix(uint8_t const *data, size_t len_octets);

/*----------------------------------------------------------------------------*/

/**
 * Compute CRC32 checksum to be used in an OGG header.
 * @param init Must be 0 for a fresh check sum.
 */
uint32_t ov_crc32_ogg(uint32_t init, uint8_t const *data, size_t len_octets);

/**
 * Calculate CRC32 in flavour of ZLIB.
 * Use init value of 0.
 */
uint32_t ov_crc32_zlib(uint32_t init, uint8_t const *data, size_t len_octets);

/*----------------------------------------------------------------------------*/
#endif
