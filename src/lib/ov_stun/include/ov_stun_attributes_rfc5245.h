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
        @file           ov_stun_attributes_rfc5245.h
        @author         Markus Toepfer

        @date           2018-11-21

        @ingroup        ov_ice

        @brief          Definition of additional STUN attributes defined in
                        RFC5245.


        ------------------------------------------------------------------------
*/
#ifndef ov_stun_attributes_rfc5245_h
#define ov_stun_attributes_rfc5245_h

#include "ov_stun_attribute.h"
#include <ov_base/ov_endian.h>

#define ICE_PRIORITY 0x0024
#define ICE_USE_CANDIDATE 0x0025
#define ICE_CONTROLLED 0x8029
#define ICE_CONTROLLING 0x802A

#define ICE_ROLE_CONFLICT 487

/*      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ... check if the attribute frame contains the attribute.
 *      ------------------------------------------------------------------------
 */

bool ov_stun_attribute_frame_is_priority(const uint8_t *buf, size_t len);
bool ov_stun_attribute_frame_is_use_candidate(const uint8_t *buf, size_t len);
bool ov_stun_attribute_frame_is_ice_controlled(const uint8_t *buf, size_t len);
bool ov_stun_attribute_frame_is_ice_controlling(const uint8_t *buf, size_t len);

/*
 *      ------------------------------------------------------------------------
 *
 *                        SERIALIZATION / DESERIALIZATION
 *
 *      ... encoding will write to buffer, which MUST be at least size of the
 *      attribute to encode and set (optional) next pointer to next byte after
 *      write
 *
 *      ... decoding will check if the buffer contains a valid attribute of type
 *      and encode the attribute content. The buffer MUST be at least length of
 *      the attribute, but MAY be longer.
 *
 *      ------------------------------------------------------------------------
 */

size_t ov_stun_ice_priority_encoding_length();
size_t ov_stun_ice_use_candidate_encoding_length();
size_t ov_stun_ice_controlled_encoding_length();
size_t ov_stun_ice_controlling_encoding_length();

/*----------------------------------------------------------------------------*/

bool ov_stun_ice_priority_encode(uint8_t *buffer, size_t length, uint8_t **next,
                                 uint32_t priority);

bool ov_stun_ice_priority_decode(const uint8_t *buffer, size_t length,
                                 uint32_t *priority);

/*----------------------------------------------------------------------------*/

bool ov_stun_ice_use_candidate_encode(uint8_t *buffer, size_t length,
                                      uint8_t **next);

/*----------------------------------------------------------------------------*/

bool ov_stun_ice_controlled_encode(uint8_t *buffer, size_t length,
                                   uint8_t **next, uint64_t number);

bool ov_stun_ice_controlled_decode(const uint8_t *buffer, size_t length,
                                   uint64_t *number);

/*----------------------------------------------------------------------------*/

bool ov_stun_ice_controlling_encode(uint8_t *buffer, size_t length,
                                    uint8_t **next, uint64_t number);

bool ov_stun_ice_controlling_decode(const uint8_t *buffer, size_t length,
                                    uint64_t *number);

/*----------------------------------------------------------------------------*/

bool ov_stun_error_code_set_ice_role_conflict(uint8_t *buffer, size_t length,
                                              uint8_t **next);

#endif /* ov_stun_attributes_rfc5245_h */
