/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_ice_definitions.h
        @author         Markus Töpfer

        @date           2024-09-18


        ------------------------------------------------------------------------
*/
#ifndef ov_ice_definitions_h
#define ov_ice_definitions_h

#include "ov_ice_dtls_config.h"
#include "ov_ice_string.h"

#define OV_ICE_MAGIC_BYTES 0x1ce0
#define OV_ICE_SESSION_MAGIC_BYTES 0x1ce1
#define OV_ICE_STREAM_MAGIC_BYTES 0x1ce2
#define OV_ICE_PAIR_MAGIC_BYTES 0x1ce3
#define OV_ICE_CANDIDATE_MAGIC_BYTES 0x1ce4
#define OV_ICE_BASE_MAGIC_BYTES 0x1ce5
#define OV_ICE_SERVER_MAGIC_BYTES 0x1ce6

#define OV_ICE_DEFAULT_TRANSACTION_LIFETIME_USECS 300000000 // 5min
#define OV_ICE_DEFAULT_CONNECTIVITY_PACE_USECS 50000  // 50ms (RFC default)
#define OV_ICE_DEFAULT_SESSION_TIMEOUT_USECS 60000000 // 60sec
#define OV_ICE_DEFAULT_KEEPALIVE_USECS 150000000      // 15 sec (RFC default)

#define OV_ICE_KEY_TRANSACTION_LIFETIME_USECS "transaction_usecs"
#define OV_ICE_KEY_STUN_PACE_USECS "pace_usecs"
#define OV_ICE_KEY_SESSION_TIMEOUT_USECS "session_timeout_usecs"
#define OV_ICE_KEY_KEEPALIVE_USECS "keepalive_usecs"

#define OV_ICE_STUN_USER_MAX 513 // RFC 5389 username length
#define OV_ICE_STUN_PASS_MAX 513 // may be of different length

#define OV_ICE_STUN_USER_MIN 4
#define OV_ICE_STUN_PASS_MIN 4

#define OV_ICE_STRING_TURN "turn"
#define OV_ICE_STRING_STUN "stun"

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

#endif /* ov_ice_definitions_h */
