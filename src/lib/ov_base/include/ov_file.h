/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_file.h
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2019-07-23

        @ingroup        ov_base

        @brief          Definition of some standard base file access functions.


        ------------------------------------------------------------------------
*/
#ifndef ov_file_h
#define ov_file_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/*----------------------------------------------------------------------------*/

#define OV_FILE_IS_DIR "file is directory"
#define OV_FILE_COULD_NOT_READ "could not read"
#define OV_FILE_UNKNOWN_ERROR "unknown error"
#define OV_FILE_ACCESS_FAILED "no access"

/*----------------------------------------------------------------------------*/

typedef enum {

    OV_FILE_ERROR,        // general error (unspecific, no log)
    OV_FILE_FAILURE,      // failure during processing (err log)
    OV_FILE_NOT_FOUND,    // not found
    OV_FILE_NO_ACCESS,    // no access
    OV_FILE_MEMORY_ERROR, // (e.g. input to small, out of memory)
    OV_FILE_SUCCESS

} ov_file_handle_state;

/*----------------------------------------------------------------------------*/

/**
 * Checks whether `path` is an entity in the file system or not.
 * It needs not be a file, just anything that lives within the file system
 * identified by `path`.
 */
bool ov_file_exists(char const *path);

/*----------------------------------------------------------------------------*/

/**
        Check if a file is readable.

        @param path     path to file
        @returns        NULL on success
                        errormsg on error
*/
const char *ov_file_read_check(const char *restrict path);

/*----------------------------------------------------------------------------*/

/**
        Check if a file is readable,
        and return bytes of the file.

        @param path     path to file
        @returns        bytes of file on success
                        -1 on error
*/
ssize_t ov_file_read_check_get_bytes(const char *restrict path);

/*----------------------------------------------------------------------------*/

/**
        Read the RAW file content to buffer of size.

        This function operations in FILL / CREATE mode.

                (A)     If some buffer is provided at *buffer,
                        the buffer will be filled,
                        if *size is big enough for the file content.

                (B)     If *buffer is NULL the buffer will be created
                        to hold the file size content.

        @param path     (mandatory) path to file to read
        @param buffer   (mandatory) pointer to buffer
        @param size     (mandatory) pointer to size of buffer, will be
                        set to content size of the file

        @return OV_FILE_SUCCESS if the full file was read.
*/
ov_file_handle_state ov_file_read(const char *path,
                                  uint8_t **buffer,
                                  size_t *size);

/*----------------------------------------------------------------------------*/

/**
        Read the RAW file content to buffer of size.

        This function operations in FILL / CREATE mode.

                (A)     If some buffer is provided at *buffer,
                        the buffer will be filled,
                        if *size is big enough for the file content.

                (B)     If *buffer is NULL the buffer will be created
                        to hold the file size content.

        @param path     (mandatory) path to file to read
        @param buffer   (mandatory) pointer to buffer
        @param size     (mandatory) pointer to size of buffer, will be
                        set to content size of the file

        @return OV_FILE_SUCCESS if the full file was read.
*/
ov_file_handle_state ov_file_read_partial(const char *path,
                                          uint8_t **buffer,
                                          size_t *size,
                                          size_t from,
                                          size_t to,
                                          size_t *all);

/*----------------------------------------------------------------------------*/

/**
        Write some RAW content to a file.

        @param path     path to write to
        @param buffer   buffer to write
        @param size     size of the buffer to write
        @param mode     write mode to use

        @return OV_FILE_SUCCESS if the full file was written.
*/
ov_file_handle_state ov_file_write(const char *path,
                                   const uint8_t *buffer,
                                   size_t size,
                                   const char *mode);

/******************************************************************************
 *                                Low-Level IO
 ******************************************************************************/

typedef enum {

    OV_FILE_RAW,
    OV_FILE_LITTLE_ENDIAN,
    OV_FILE_BIG_ENDIAN,
    OV_FILE_SWAP_BYTES

} ov_file_byteorder;

/**
 * Read a 16 bit value from rd_ptr.
 *
 * @param out pointer to write 16 bit value to
 * @param rd_ptr pointer to read raw bytes from. Will be propagated when read.
 * @param length number of bytes readable from rd_ptr. Will be decreased by
 * number of read bytes.
 * @param byte_order
 */
bool ov_file_get_16(uint16_t *out,
                    uint8_t **rd_ptr,
                    size_t *length,
                    ov_file_byteorder byte_order);

/*----------------------------------------------------------------------------*/

/**
 * Read a 32 bit value from rd_ptr.
 *
 * @param out pointer to write 16 bit value to
 * @param rd_ptr pointer to read raw bytes from. Will be propagated when read.
 * @param length number of bytes readable from rd_ptr. Will be decreased by
 * number of read bytes.
 * @param byte_order
 */
bool ov_file_get_32(uint32_t *out,
                    uint8_t **rd_ptr,
                    size_t *length,
                    ov_file_byteorder byte_order);

/*----------------------------------------------------------------------------*/

/**
 * Write a 16 bit value to write_ptr
 *
 * It does not take a byteorder argument, because it is easy to coerce
 * with OV_SWAP_16 / OV_H16TOLE / OV_H16TOBE macros:
 *
 * // Write 126 as 16 bit BIG ENDIAN
 * ov_file_write_16(&ptr, &length, OV_H16TOBE(126));
 *
 */
bool ov_file_write_16(uint8_t **write_ptr, size_t *length, uint16_t value);

/*----------------------------------------------------------------------------*/

/**
 * Write a 32 bit value to write_ptr
 *
 * It does not take a byteorder argument, because it is easy to coerce
 * with OV_SWAP_32 / OV_H32TOLE / OV_H32TOBE macros:
 *
 * // Write 126 as 32 bit BIG ENDIAN
 * ov_file_write_32(&ptr, &length, OV_H32TOBE(126));
 */
bool ov_file_write_32(uint8_t **write_ptr, size_t *length, uint32_t value);

/*----------------------------------------------------------------------------*/

#endif /* ov_file_h */
