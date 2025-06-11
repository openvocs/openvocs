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
        @file           ov_stun_binding.h
        @author         Markus Toepfer

        @date           2018-10-23

        @ingroup        ov_stun

        @brief          Definition of RFC 5389 method "binding"

        ------------------------------------------------------------------------
*/
#ifndef ov_stun_binding_h
#define ov_stun_binding_h

#define STUN_BINDING 0x001

#include "ov_stun_attributes_rfc5389.h"
#include "ov_stun_frame.h"

/*      ------------------------------------------------------------------------
 *
 *                              METHOD
 *
 *      ------------------------------------------------------------------------
 */

/**
        Checks if the class of the frame is supported by binding.

        This function will only check the class contained in the frame and
        if this class is supported by @see ov_stun_binding_process
*/
bool ov_stun_binding_class_support(const uint8_t *frame, size_t length);

/*      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Check if the frame method is binding.

        This function will only check the method contained in the frame and
        not validate the frame itself.

        @param frame           start of the frame buffer
        @param length          length of the attribute buffer (at least)
*/
bool ov_stun_method_is_binding(const uint8_t *frame, size_t length);

/*      ------------------------------------------------------------------------
 *
 *                              REQUEST CREATION
 *
 *      ------------------------------------------------------------------------
 */

/**
        Generate a plain binding request without any attributes set.
        (optional with attribute software and fingerprint)

        @param start            pointer to head
        @param length           length of the buffer at head
        @param next             (optional) pointer to next after write
        @param transaction_id   transaction_id to use
        @param software         (optional) pointer to content buffer
        @param software_length  (optional) length of content buffer
        @param fingerprint      if true fingerprint will be written
        @returns true           if all attributes are written to start.
*/
bool ov_stun_generate_binding_request_plain(uint8_t *start,
                                            size_t length,
                                            uint8_t **next,
                                            const uint8_t *transaction_id,
                                            const uint8_t *software,
                                            size_t software_length,
                                            bool fingerprint);

/*----------------------------------------------------------------------------*/

/**
        Generate binding request with short term credentials

        @param start            pointer to head
        @param length           length of the buffer at head
        @param next             (optional) pointer to next after write
        @param transaction_id   transaction_id to use
        @param username         (mandatory) pointer to content buffer
        @param username_length  (mandatory) length of content buffer
        @param key              (mandatory) MUST be present for integrity
        @param key_length       (mandatory) length of content buffer
        @param software         (optional) pointer to content buffer
        @param software_length  (optional) length of content buffer
        @param fingerprint      if true fingerprint will be written
        @returns true           if all attributes are written to start.
*/
bool ov_stun_generate_binding_request_short_term(uint8_t *start,
                                                 size_t length,
                                                 uint8_t **next,
                                                 const uint8_t *transaction_id,
                                                 const uint8_t *software,
                                                 size_t software_length,
                                                 const uint8_t *username,
                                                 size_t username_length,
                                                 const uint8_t *key,
                                                 size_t key_length,
                                                 bool fingerprint);

/*----------------------------------------------------------------------------*/

/**
        Generate binding request with short term credentials

        @param start            pointer to head
        @param length           length of the buffer at head
        @param next             (optional) pointer to next after write
        @param transaction_id   transaction_id to use
        @param username         (mandatory) pointer to content buffer
        @param username_length  (mandatory) length of content buffer
        @param key              (mandatory) MUST be present for integrity
        @param key_length       (mandatory) length of content buffer
        @param software         (optional) pointer to content buffer
        @param software_length  (optional) length of content buffer
        @param fingerprint      if true fingerprint will be written
        @returns true           if all attributes are written to start.
*/
bool ov_stun_generate_binding_request_short_term(uint8_t *start,
                                                 size_t length,
                                                 uint8_t **next,
                                                 const uint8_t *transaction_id,
                                                 const uint8_t *software,
                                                 size_t software_length,
                                                 const uint8_t *username,
                                                 size_t username_length,
                                                 const uint8_t *key,
                                                 size_t key_length,
                                                 bool fingerprint);

/*----------------------------------------------------------------------------*/

/**
        Generate binding request with long term credentials.

        @param start            pointer to head
        @param length           length of the buffer at head
        @param next             (optional) pointer to next after write
        @param transaction_id   transaction_id to use
        @param software         (optional) pointer to content buffer
        @param software_length  (optional) length of content buffer
        @param username         (mandatory) pointer to content buffer
        @param username_length  (mandatory) length of content buffer
        @param realm            (mandatory) pointer to content buffer
        @param realm_length     (mandatory) length of content buffer
        @param nonce            (mandatory) pointer to content buffer
        @param nonce_length     (mandatory) length of content buffer
        @param key              (mandatory) MUST be present for integrity
        @param key_length       (mandatory) length of content buffer
        @param fingerprint      if true fingerprint will be written
        @returns true           if all attributes are written to start.
*/
bool ov_stun_generate_binding_request_long_term(uint8_t *start,
                                                size_t length,
                                                uint8_t **next,
                                                const uint8_t *transaction_id,
                                                const uint8_t *software,
                                                size_t software_length,
                                                const uint8_t *username,
                                                size_t username_length,
                                                const uint8_t *realm,
                                                size_t realm_length,
                                                const uint8_t *nonce,
                                                size_t nonce_length,
                                                const uint8_t *key,
                                                size_t key_length,
                                                bool fingerprint);

/*----------------------------------------------------------------------------*/

/**
        Full custom binding generation.
        To be used for testing, validation and creation of more strict
        requests.

        @param start            pointer to head
        @param length           length of the buffer at head
        @param next             (optional) pointer to next after write
        @param transaction_id   transaction_id to use
        @param software         (optional) pointer to content buffer
        @param software_length  (optional) length of content buffer
        @param username         (optional) pointer to content buffer
        @param username_length  (optional) length of content buffer
        @param realm            (optional) pointer to content buffer
        @param realm_length     (optional) length of content buffer
        @param nonce            (optional) pointer to content buffer
        @param nonce_length     (optional) length of content buffer
        @param key              (optional) MUST be present for integrity
        @param key_length       (optional) length of content buffer
        @param integrity        if true message integrity will be written
        @param fingerprint      if true fingerprint will be written
        @returns true           if all attributes are written to start.
*/
bool ov_stun_generate_binding_request(uint8_t *start,
                                      size_t length,
                                      uint8_t **next,
                                      const uint8_t *transaction_id,
                                      const uint8_t *software,
                                      size_t software_length,
                                      const uint8_t *username,
                                      size_t username_length,
                                      const uint8_t *realm,
                                      size_t realm_length,
                                      const uint8_t *nonce,
                                      size_t nonce_length,
                                      const uint8_t *key,
                                      size_t key_length,
                                      bool integrity,
                                      bool fingerprint);

#endif /* ov_stun_binding_h */
