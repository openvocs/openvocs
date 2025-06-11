/***
        ------------------------------------------------------------------------

        Copyright 2018 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_ice_string.h
        @author         Markus Toepfer

        @date           2018-05-28

        @ingroup        ov_ice

        @brief          Definition of ICE related strings.

        ------------------------------------------------------------------------
*/
#ifndef ov_ice_string_h
#define ov_ice_string_h

#include <inttypes.h>
#include <ov_base/ov_sdp.h>
#include <stdbool.h>
#include <string.h>

#define OV_ICE_STRING_ACTIVE_PASSIVE "actpass"
#define OV_ICE_STRING_ACTIVE "active"
#define OV_ICE_STRING_PASSIVE "passive"
#define OV_ICE_STRING_INACTIVE "inactive"

#define OV_ICE_STRING_CANDIDATE "candidate"
#define OV_ICE_STRING_END_OF_CANDIDATES "end_of_candidates"

#define OV_ICE_STRING_LITE "ice-lite"
#define OV_ICE_STRING_MISMATCH "ice-mismatch"
#define OV_ICE_STRING_OPTIONS "ice-options"

#define OV_ICE_STRING_USER "ice-ufrag"
#define OV_ICE_STRING_PASS "ice-pwd"
#define OV_ICE_STRING_UFRAG "ufrag"
#define OV_ICE_STRING_SDP_MID "sdpMid"
#define OV_ICE_STRING_SDP_MLINEINDEX "sdpMLineIndex"

#define OV_ICE_STRING_ICE1 "ice1"
#define OV_ICE_STRING_ICE2 "ice2"
#define OV_ICE_STRING_TRICKLE "trickle"

#define OV_ICE_STRING_TRICKLE_ICE2 "trickle ice2"

#define OV_ICE_STRING_CNAME "cname"
#define OV_ICE_STRING_INACTIVE "inactive"

#define ICE_STRING_TYPE "typ"
#define ICE_STRING_RADDR "raddr"
#define ICE_STRING_RPORT "rport"

#define ICE_TRANSPORT_UDP "udp"

#define ICE_STRING_HOST "host"
#define ICE_STRING_SERVER_REFLEXIVE "srflx"
#define ICE_STRING_PEER_REFLEXIVE "prflx"
#define ICE_STRING_RELAYED "relay"

/*----------------------------------------------------------------------------*/

bool ov_ice_string_fill_random(char *start, size_t length);

/*
 *      ------------------------------------------------------------------------
 *
 *                               GRAMMAR
 *
 *      ------------------------------------------------------------------------
 *
 */

bool ov_ice_string_is_ice_char(const char *start, size_t length);

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_foundation(const char *start, size_t length);
bool ov_ice_string_is_component_id(const char *start, size_t length);
bool ov_ice_string_is_transport(const char *start, size_t length);
bool ov_ice_string_is_transport_extension(const char *start, size_t length);
bool ov_ice_string_is_priority(const char *start, size_t length);

bool ov_ice_string_is_candidate_type(const char *start, size_t length);
bool ov_ice_string_is_host(const char *start, size_t length);
bool ov_ice_string_is_srflx(const char *start, size_t length);
bool ov_ice_string_is_prflx(const char *start, size_t length);
bool ov_ice_string_is_relay(const char *start, size_t length);
bool ov_ice_string_is_token(const char *start, size_t length);

bool ov_ice_string_is_connection_address(const char *start, size_t length);
bool ov_ice_string_is_connection_port(const char *start, size_t length);
bool ov_ice_string_is_related_address(const char *start, size_t length);
bool ov_ice_string_is_related_port(const char *start, size_t length);

bool ov_ice_string_is_extension_name(const char *start, size_t length);
bool ov_ice_string_is_extension_value(const char *start, size_t length);

/*
 *      ------------------------------------------------------------------------
 *
 *                                KEYS
 *
 *      ------------------------------------------------------------------------
 *
 */

bool ov_ice_string_is_ice_lite_key(const char *start, size_t length);
bool ov_ice_string_is_ice_mismatch_key(const char *start, size_t length);
bool ov_ice_string_is_ice_pwd_key(const char *start, size_t length);
bool ov_ice_string_is_ice_ufrag_key(const char *start, size_t length);
bool ov_ice_string_is_ice_options_key(const char *start, size_t length);

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_ice_pwd(const char *start, size_t length);
bool ov_ice_string_is_ice_ufrag(const char *start, size_t length);
bool ov_ice_string_is_ice_option_tag(const char *start, size_t length);

#endif /* ov_ice_string_h */
