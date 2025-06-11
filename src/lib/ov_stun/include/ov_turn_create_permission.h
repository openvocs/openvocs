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
        @file           ov_turn_create_permission.h
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#ifndef ov_turn_create_permission_h
#define ov_turn_create_permission_h

#define TURN_CREATE_PERMISSION 0x008

#include "ov_stun_frame.h"
#include <stdbool.h>
#include <stdlib.h>

/*      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Check if the frame method is create permission.

        This function will only check the method contained in the frame and
        not validate the frame itself.

        @param frame           start of the frame buffer
        @param length          length of the attribute buffer (at least)
*/
bool ov_turn_method_is_create_permission(const uint8_t *frame, size_t length);

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
                               bool fingerprint);

#endif /* ov_turn_create_permission_h */
