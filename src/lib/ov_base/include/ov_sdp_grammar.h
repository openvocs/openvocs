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
        @file           ov_sdp_grammar.h
        @author         Markus Toepfer

        @date           2018-04-12

        @ingroup        ov_sdp

        @brief          Definition of some standard grammar of SDP
                        string items.

                        Functions within this file may be used to check
                        content added to an ov_sdp_session.

        ------------------------------------------------------------------------
*/
#ifndef ov_sdp_grammar_h
#define ov_sdp_grammar_h

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>

#include "ov_base64.h"
#include "ov_string.h"
#include "ov_uri.h"
#include "ov_utf8.h"

#define OV_SDP_SEND "send"
#define OV_SDP_RECV "recv"
#define OV_SDP_SEND_ONLY "sendonly"
#define OV_SDP_RECV_ONLY "recvonly"
#define OV_SDP_SEND_RECV "sendrecv"

#define OV_SDP_ORIENTATION "orientation"
#define OV_SDP_CHARSET "charset"

#define OV_SDP_TYPE "type"
#define OV_SDP_SDPLANG "sdplang"
#define OV_SDP_LANG "lang"
#define OV_SDP_FRAMERATE "framerate"
#define OV_SDP_QUALITY "quality"

#define OV_SDP_RTPMAP "rtpmap"
#define OV_SDP_FMTP "fmtp"

#define OV_SDP_INACTIVE "inactive"
#define OV_SDP_ACTIVE "active"

#define OV_SDP_CNAME "cname"

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is valid text of RFC 4566.
*/
bool ov_sdp_is_text(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is a valid user name.
*/
bool ov_sdp_is_username(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is a valid key.
*/
bool ov_sdp_is_key(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is base64 encoded.
*/
bool ov_sdp_is_base64(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Check if a buffer SHALL be evaluated as base64.
        Preamble + content test e.g. "base64:xxxxxxxx"
*/
bool ov_sdp_check_base64(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Check if a buffer SHALL be evaluated as clear text.
        Preamble + content test e.g. "clear:xxxxxxxx"
*/
bool ov_sdp_check_clear(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Check if a buffer SHALL be evaluated as URI text.
        Preamble + content test e.g. "uri:xxxxxxxx"
*/
bool ov_sdp_check_uri(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is a valid URI
*/
bool ov_sdp_is_uri(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is a digit
*/
bool ov_sdp_is_digit(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is an integer (digit > 0)
*/
bool ov_sdp_is_integer(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is an integer (0 ... 65535)
*/
bool ov_sdp_is_port(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is typed time (digit + optional char [dhms])
*/
bool ov_sdp_is_typed_time(const char *buffer, uint64_t length,
                          bool allow_negative);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is a 10 digit time (or 0)
*/
bool ov_sdp_is_time(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is a SDP token.
*/
bool ov_sdp_is_token(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is an address.
*/
bool ov_sdp_is_address(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is a protocol.
*/
bool ov_sdp_is_proto(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is a byte string.
*/
bool ov_sdp_is_byte_string(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is a byte string.
*/
bool ov_sdp_is_phone(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is a byte string.
*/
bool ov_sdp_is_email(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is a byte string.
*/
bool ov_sdp_is_email_safe(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is a mulitcast address.
*/
bool ov_sdp_is_multicast_address(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is a unicast address.
*/
bool ov_sdp_is_unicast_address(const char *buffer, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Test if a buffer is some bandwidth content
*/
bool ov_sdp_is_bandwidth(const char *buffer, uint64_t length);

#endif /* ov_sdp_grammar_h */
