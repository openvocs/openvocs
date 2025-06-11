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
        @file           ov_stun_attributes_rfc5245.c
        @author         Markus Toepfer

        @date           2018-11-21

        @ingroup        ov_ice

        @brief          Implementation of RFC 5245 STUN attributes

        ------------------------------------------------------------------------
*/
#include "../include/ov_stun_attributes_rfc5245.h"
#include "../include/ov_stun_error_code.h"

/*      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ... check if the attribute frame contains the attribute.
 *      ------------------------------------------------------------------------
 */

bool ov_stun_attribute_frame_is_priority(const uint8_t *buffer, size_t length) {

    if (!buffer || length < 8) goto error;

    uint16_t type = ov_stun_attribute_get_type(buffer, length);
    int64_t size = ov_stun_attribute_get_length(buffer, length);

    if (type != ICE_PRIORITY) goto error;

    if (size != 4) goto error;

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_attribute_frame_is_use_candidate(const uint8_t *buffer,
                                              size_t length) {

    if (!buffer || length < 4) goto error;

    uint16_t type = ov_stun_attribute_get_type(buffer, length);
    int64_t size = ov_stun_attribute_get_length(buffer, length);

    if (type != ICE_USE_CANDIDATE) goto error;

    if (size != 0) goto error;

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_attribute_frame_is_ice_controlled(const uint8_t *buffer,
                                               size_t length) {

    if (!buffer || length < 12) goto error;

    uint16_t type = ov_stun_attribute_get_type(buffer, length);
    int64_t size = ov_stun_attribute_get_length(buffer, length);

    if (type != ICE_CONTROLLED) goto error;

    if (size != 8) goto error;

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_attribute_frame_is_ice_controlling(const uint8_t *buffer,
                                                size_t length) {

    if (!buffer || length < 12) goto error;

    uint16_t type = ov_stun_attribute_get_type(buffer, length);
    int64_t size = ov_stun_attribute_get_length(buffer, length);

    if (type != ICE_CONTROLLING) goto error;

    if (size != 8) goto error;

    return true;

error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *                        SERIALIZATION / DESERIALIZATION
 *
 *      ------------------------------------------------------------------------
 */

size_t ov_stun_ice_priority_encoding_length() { return 8; }

/*----------------------------------------------------------------------------*/

size_t ov_stun_ice_use_candidate_encoding_length() { return 4; }

/*----------------------------------------------------------------------------*/

size_t ov_stun_ice_controlled_encoding_length() { return 12; }

/*----------------------------------------------------------------------------*/

size_t ov_stun_ice_controlling_encoding_length() { return 12; }

/*----------------------------------------------------------------------------*/

bool ov_stun_ice_priority_encode(uint8_t *buffer,
                                 size_t length,
                                 uint8_t **next,
                                 uint32_t priority) {

    if (!buffer || length < 8) goto error;

    if (!ov_stun_attribute_set_type(buffer, length, ICE_PRIORITY)) goto error;

    if (!ov_stun_attribute_set_length(buffer, length, 4)) goto error;

    return ov_endian_write_uint32_be(buffer + 4, length - 4, next, priority);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_ice_priority_decode(const uint8_t *buffer,
                                 size_t length,
                                 uint32_t *priority) {

    if (!buffer || length < 8 || !priority) goto error;

    if (!ov_stun_attribute_frame_is_priority(buffer, length)) goto error;

    if (ov_endian_read_uint32_be(buffer + 4, length - 4, priority)) return true;

    return true;

error:
    if (priority) *priority = 0;
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_ice_use_candidate_encode(uint8_t *buffer,
                                      size_t length,
                                      uint8_t **next) {

    if (!buffer || length < 4) goto error;

    if (!ov_stun_attribute_set_type(buffer, length, ICE_USE_CANDIDATE))
        goto error;

    if (!ov_stun_attribute_set_length(buffer, length, 0)) goto error;

    if (next) *next = buffer + 4;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_ice_controlled_encode(uint8_t *buffer,
                                   size_t length,
                                   uint8_t **next,
                                   uint64_t number) {

    if (!buffer || length < 12) goto error;

    if (!ov_stun_attribute_set_type(buffer, length, ICE_CONTROLLED)) goto error;

    if (!ov_stun_attribute_set_length(buffer, length, 8)) goto error;

    return ov_endian_write_uint64_be(buffer + 4, length - 4, next, number);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_ice_controlled_decode(const uint8_t *buffer,
                                   size_t length,
                                   uint64_t *number) {

    if (!buffer || length < 12 || !number) goto error;

    if (!ov_stun_attribute_frame_is_ice_controlled(buffer, length)) goto error;

    if (ov_endian_read_uint64_be(buffer + 4, length - 4, number)) return true;

error:
    if (number) *number = 0;
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_ice_controlling_encode(uint8_t *buffer,
                                    size_t length,
                                    uint8_t **next,
                                    uint64_t number) {

    if (!buffer || length < 12) goto error;

    if (!ov_stun_attribute_set_type(buffer, length, ICE_CONTROLLING))
        goto error;

    if (!ov_stun_attribute_set_length(buffer, length, 8)) goto error;

    return ov_endian_write_uint64_be(buffer + 4, length - 4, next, number);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_ice_controlling_decode(const uint8_t *buffer,
                                    size_t length,
                                    uint64_t *number) {

    if (!buffer || length < 12 || !number) goto error;

    if (!ov_stun_attribute_frame_is_ice_controlling(buffer, length)) goto error;

    if (ov_endian_read_uint64_be(buffer + 4, length - 4, number)) return true;

error:
    if (number) *number = 0;
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_error_code_set_ice_role_conflict(uint8_t *buffer,
                                              size_t length,
                                              uint8_t **next) {

    char *phrase = "role conflict";

    return ov_stun_error_code_encode(buffer,
                                     length,
                                     next,
                                     ICE_ROLE_CONFLICT,
                                     (uint8_t *)phrase,
                                     strlen(phrase));
}
