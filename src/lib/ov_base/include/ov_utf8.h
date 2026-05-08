/***
        ------------------------------------------------------------------------

        Copyright 2017 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the openvocs project. http://openvocs.org

        ------------------------------------------------------------------------
*//**
        @file           ov_utf8.h
        @author         Markus Toepfer

        @date           2017-12-17

        @ingroup        ov_base

        @brief          Interface for UTF8 functionality. (RFC3629)

        This module is used to verify if some bytes are encoced as UTF8 or not.

        Overview byte coding scheme

        ASCII                                       01111111                0F
        2 byte                          11000010    10000000            C2  80
        2 byte                          11000010    10111111            C2  BF
                                                                          ...
        2 byte                          11011111    10000000            DF  80
        2 byte                          11011111    10111111            DF  BF

        3byte               11100000    10100000    10111111        E0  A0  BF
        3byte               11100000    10111111    10111111        E0  BF  BF
        3byte               11100001    10000000    10000000        E1  80  80
        3byte               11100001    10111111    10111111        E1  BF  BF
                                                                        ...
        3byte               11101100    10000000    10000000        EC  80  80
        3byte               11101100    10111111    10111111        EC  BF  BF
        3byte               11101101    10000000    10111111        ED  80  BF
        3byte               11101101    10011111    10111111        ED  9F  BF
        3byte               11101110    10000000    10000000        EE  80  80
        3byte               11101110    10111111    10111111        EE  BF  BF
        3byte               11101111    10000000    10000000        EF  80  80
        3byte               11101111    10111111    10111111        EF  BF  BF

        4byte   11110000    10010000    10111111    10111111    F0  90  BF  BF
        4byte   11110000    10111111    10111111    10111111    F0  BF  BF  BF
        4byte   11110001    10000000    10000000    10000000    F1  80  80  80
        4byte   11110001    10111111    10111111    10111111    F1  BF  BF  BF
        4byte   11110010    10000000    10000000    10000000    F2  80  80  80
        4byte   11110010    10111111    10111111    10111111    F2  BF  BF  BF
        4byte   11110011    10000000    10000000    10000000    F3  80  80  80
        4byte   11110011    10111111    10111111    10111111    F3  BF  BF  BF
        4byte   11110100    10000000    10111111    10111111    F4  80  BF  BF
        4byte   11110100    10001111    10111111    10111111    F4  8F  BF  BF

        ------------------------------------------------------------------------
*/
#ifndef ov_utf8_h
#define ov_utf8_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

/*----------------------------------------------------------------------------*/

/**
        Check a byte sequence for valid UTF8 and return the last valid UTF8
        character. If character_start is true, a pointer to the start of the
        byte with the last valid character is returned. If character_start is
        false, a pointer behind the last valid UTF8 character is returned.

        Check a byte sequence for valid UTF8 may be done using:

        if ((size_t)(ov_utf8_last_valid(start, size, false) - start) == size)
                return true;

        @param start            start of a byte buffer
        @param length           amount of bytes from start
        @param character_start  if true a pointer to the start of the last valid
                                UTF8 char will be returned, otherwise a pointer
                                behind the last UTF8 char will be returned.

        @returns                pointer to or behind last valid character
                                NULL if pointer to start is NULL (input error)

        @NOTE                   If the first byte is non valid UTF8, this
                                function will return a pointer to start.
                                This function will check input bytes for valid
                                UTF8 encoding until an encoding error
                                is identified.

                                Maximum distance between character_start (TRUE)
                                and character_start (FALSE) are 4 bytes.
                                Minimum distance between charater_start (TRUE)
                                and character_start (FALSE) is 1 byte. (ASCII)
*/
uint8_t *ov_utf8_last_valid(const uint8_t *start, uint64_t length,
                            bool character_start);

/*----------------------------------------------------------------------------*/

/**
        Check if a byte sequence is a valid UTF8 encoded sequence.
        The sequence MUST finish with a valid UTF8 character.

        @param start            start of a byte buffer
        @param length           amount of bytes from start
        @returns                true if the buffer contains a valid sequence
*/
bool ov_utf8_validate_sequence(const uint8_t *start, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Encode an unicode code point to an UTF8 character and
        write the character to a pointer.

        If the pointer points to NULL a new memory area will be
        allocated and the size_t given in open will be ignored.

        If the pointer points to an external byte array, the character will be
        written to the given pointer, if the size of open bytes is at least
        equal to the amount of bytes to write.

        In both cases, the used pointer will contain the amount of bytes
        written to *start.

        @param number           unicode code point
        @param start            pointer to pointer to write to
        @param open             amount of open bytes at pointer at start
        @param used             pointer to write used amount of bytes

        @returns                true if number was written to *start
*/
bool ov_utf8_encode_code_point(uint64_t number, uint8_t **start, uint64_t open,
                               uint64_t *used);

/*----------------------------------------------------------------------------*/

/**
        Decode an UTF8 charater to an unicode code point. This function
        encodes the first matching character. The bytes of the matching
        character do not need to match the input length, which may be longer.
        (e.g. use a length of an UTF8 string, and parse the first character)

        @param start            start of the character
        @param length           maximum length of the character
        @param bytes            bytes decoded with the codepoint.
        @returns                unicode code point or 0.

        @NOTE                   To check for success check if bytes > 0.

        @DISCUSSION             A '0' ascii input at the start of an UTF8 string
                                will return 0 as first character, but bytes used
                                set to 1.
                                Otherwise an ascii NULL at the start of an UTF8
                                string may be missed. Otherwise this means the
                                string is empty in terms of standard C strings.

        @IMPLEMENTATIONS        Implementations using this decode function need
                                to clarify: Do we want to support 0 as the first
                                UTF8 char of a valid UTF8 string?
*/
uint64_t ov_utf8_decode_code_point(const uint8_t *start, uint64_t length,
                                   uint64_t *bytes);

/*----------------------------------------------------------------------------*/

/**
        Generate a random unicode buffer. (Buffer generate/fill mode)

        The buffer will be filled up to size if a buffer is provided as input.
        If no buffer is provided, the buffer will be allocated.

        @param buffer           buffer to write to
        @param size             size written within the buffer (bytes)
        @param unicode_chars    amount of unicode chars to write
        @returns                true if the amount of unicode chars was
                                written to buffer.

        @NOTE
*/
bool ov_utf8_generate_random_buffer(uint8_t **buffer, size_t *size,
                                    uint16_t unicode_chars);

#endif /* ov_utf8_h */
