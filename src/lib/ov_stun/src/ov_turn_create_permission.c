/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_turn_create_permission.c
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#include "../include/ov_turn_create_permission.h"
#include "../include/ov_stun_fingerprint.h"
#include "../include/ov_stun_message_integrity.h"
#include "../include/ov_stun_nonce.h"
#include "../include/ov_stun_realm.h"
#include "../include/ov_stun_software.h"
#include "../include/ov_stun_username.h"
#include "../include/ov_turn_attr_requested_transport.h"
#include "../include/ov_turn_attr_xor_peer_address.h"

bool ov_turn_method_is_create_permission(const uint8_t *frame, size_t length) {

    if (TURN_CREATE_PERMISSION == ov_stun_frame_get_method(frame, length))
        return true;

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_turn_create_permission(uint8_t *start,
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
                               const struct sockaddr_storage *address,
                               bool fingerprint) {

    if (!start || length < 20 || !transaction_id) goto error;

    uint8_t *ptr = start;
    size_t required = 20;
    size_t len = 0;

    // check size and input

    if (software) {

        if (!ov_stun_software_validate(software, software_length)) goto error;

        len = ov_stun_software_encoding_length(software, software_length);
        if (len == 0) goto error;

        required += len;
    }

    required += ov_stun_message_integrity_encoding_length();
    required += ov_turn_attr_xor_peer_address_encoding_length(address);

    if (fingerprint) required += ov_stun_fingerprint_encoding_length();

    if (username) {

        len = ov_stun_username_encoding_length(username, username_length);
        if (len == 0) goto error;

        required += len;
    }

    if (realm) {

        len = ov_stun_realm_encoding_length(realm, realm_length);
        if (len == 0) goto error;

        required += len;
    }

    if (nonce) {

        len = ov_stun_nonce_encoding_length(nonce, nonce_length);
        if (len == 0) goto error;

        required += len;
    }

    if (length < required) goto error;

    // write header

    if (!memset(start, 0, required)) goto error;

    if (!ov_stun_frame_set_request(start, length)) goto error;

    if (!ov_stun_frame_set_method(start, length, TURN_CREATE_PERMISSION))
        goto error;

    if (!ov_stun_frame_set_magic_cookie(start, length)) goto error;

    if (!ov_stun_frame_set_length(start, length, required - 20)) goto error;

    if (!ov_stun_frame_set_transaction_id(start, length, transaction_id))
        goto error;

    // write content

    ptr = start + 20;

    if (!ov_turn_attr_xor_peer_address_encode(
            ptr, length - (ptr - start), start, &ptr, address))
        goto error;

    if (username)
        if (!ov_stun_username_encode(
                ptr, length - (ptr - start), &ptr, username, username_length))
            goto error;

    if (realm)
        if (!ov_stun_realm_encode(
                ptr, length - (ptr - start), &ptr, realm, realm_length))
            goto error;

    if (nonce)
        if (!ov_stun_nonce_encode(
                ptr, length - (ptr - start), &ptr, nonce, nonce_length))
            goto error;

    if (software)
        if (!ov_stun_software_encode(
                ptr, length - (ptr - start), &ptr, software, software_length))
            goto error;

    if (!ov_stun_add_message_integrity(
            start, length, ptr, &ptr, key, key_length))
        goto error;

    if (fingerprint)
        if (!ov_stun_add_fingerprint(start, length, ptr, &ptr)) goto error;

    if (next) *next = ptr;
    return true;
error:
    if (next) *next = NULL;
    return false;
}