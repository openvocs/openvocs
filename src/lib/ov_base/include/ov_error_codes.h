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
        @file           ov_error_codes.h
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2019-06-05

        @ingroup        ov_error_codes

        @brief          Definition of ov_error_codes.

        ------------------------------------------------------------------------
*/
#ifndef ov_error_codes_h
#define ov_error_codes_h

#define OV_ERROR_IS_CRITICAL(error_code)                                       \
    ((OV_ERROR_NON_CRITICAL > error_code) ||                                   \
     (OV_ERROR_NON_CRITICAL_END < error_code))

#define OV_ERROR_NOERROR 0
#define OV_ERROR_NO_ERROR OV_ERROR_NOERROR

#define OV_ERROR_FUNCTION_INPUT "function input error"

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERAL ERRORS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_ERROR_CODE 42
#define OV_ERROR_DESC "some error"

#define OV_ERROR_CODE_UNKNOWN 998
#define OV_ERROR_DESC_UNKNOWN "unknown"

#define OV_ERROR_CODE_UNKNOWN_EVENT_ERROR 999
#define OV_ERROR_DESC_UNKNOWN_EVENT_ERROR "unknown event"

#define OV_ERROR_CODE_PROCESSING_ERROR 1000
#define OV_ERROR_DESC_PROCESSING_ERROR "server processing error"

#define OV_ERROR_CODE_INPUT_ERROR 1001
#define OV_ERROR_DESC_INPUT_ERROR "input error"

#define OV_ERROR_CODE_SIGNALING_ERROR 1002
#define OV_ERROR_DESC_SIGNALING_ERROR "signaling error"

#define OV_ERROR_CODE_COMMS_ERROR 1004
#define OV_ERROR_DESC_COMMS_ERROR "communication error"

#define OV_ERROR_CODE_NOT_IMPLEMENTED 9999
#define OV_ERROR_DESC_NOT_IMPLEMENTED "not implemented yet"

#define OV_ERROR_CODE_MAX_ERROR 1002
#define OV_ERROR_DESC_MAX_ERROR "max items reached"

#define OV_ERROR_CODE_NOT_ENABLED_ERROR 1003
#define OV_ERROR_DESC_NOT_ENABLED_ERROR "not enabled"

#define OV_ERROR_CODE_INTERFACE_UNKNOWN_ERROR 1004
#define OV_ERROR_DESC_INTERFACE_UNKNOWN_ERROR "interface unknown"

#define OV_ERROR_CODE_WITHDRAW_ERROR 1005
#define OV_ERROR_DESC_WITHDRAW_ERROR "withdraw input error"

#define OV_ERROR_CODE_UNKOWN_ERROR 1006
#define OV_ERROR_DESC_UNKOWN_ERROR "unknown"

#define OV_ERROR_CODE_CONNECTION_LOST 1007
#define OV_ERROR_DESC_CONNECTION_LOST "connection lost"

#define OV_ERROR_CODE_PARAMETER_ERROR 1008
#define OV_ERROR_DESC_PARAMETER_ERROR "parameter missing"

#define OV_ERROR_CODE_ALREADY_SET 1009
#define OV_ERROR_DESC_ALREADY_SET "already set"

#define OV_ERROR_CODE_INACTIVE 1010
#define OV_ERROR_DESC_INACTIVE "inactive"

#define OV_ERROR_CODE_NOT_FOUND_ERROR 1011
#define OV_ERROR_DESC_NOT_FOUND_ERROR "not found"
#define OV_ERROR_CODE_NOT_FOUND OV_ERROR_CODE_NOT_FOUND_ERROR
#define OV_ERROR_DESC_NOT_FOUND OV_ERROR_DESC_NOT_FOUND_ERROR

#define OV_ERROR_CODE_NOT_A_RESPONSE 1234
#define OV_ERROR_DESC_NOT_A_RESPONSE "not a response"

/*
 *      ------------------------------------------------------------------------
 *
 *      SESSION SPECIFIC ERRORS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_ERROR_CODE_SESSION_CREATE 2000
#define OV_ERROR_DESC_SESSION_CREATE "failed to create a session"

#define OV_ERROR_CODE_SESSION_DELETE 2001
#define OV_ERROR_DESC_SESSION_DELETE "failed to delete a session"

#define OV_ERROR_CODE_SESSION_UNKNOWN 2002
#define OV_ERROR_DESC_SESSION_UNKNOWN "session unknown"

#define OV_ERROR_CODE_STREAM_UNKNOWN 2003
#define OV_ERROR_DESC_STREAM_UNKNOWN "stream unknown"

#define OV_ERROR_CODE_DESTINATION_UNKNOWN 2004
#define OV_ERROR_DESC_DESTINATION_UNKNOWN "destination unknown"

#define OV_ERROR_CODE_CANDIDATE_PROCESSING 2005
#define OV_ERROR_DESC_CANDIDATE_PROCESSING "candidate processing"

#define OV_ERROR_CODE_SESSION_UPDATE 2006
#define OV_ERROR_DESC_SESSION_UPDATE "session update"

#define OV_ERROR_CODE_SESSION_CANCELLED 2010
#define OV_ERROR_DESC_SESSION_CANCELLED "session cancelled by peer"

/*
 *      ------------------------------------------------------------------------
 *
 *      CONFIGURATION ERRORS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_ERROR_CODE_CONFIG_ERROR 3000
#define OV_ERROR_DESC_CONFIG_ERROR "configuration error"

#define OV_ERROR_CODE_MEDIA_CONFIG_ERROR 3001
#define OV_ERROR_DESC_MEDIA_CONFIG_ERROR "media configuration error"

#define OV_ERROR_CODE_SESSION_CONFIG_ERROR 3002
#define OV_ERROR_DESC_SESSION_CONFIG_ERROR "session configuration error"

#define OV_ERROR_CODE_TYPE_ERROR 3003
#define OV_ERROR_DESC_TYPE_ERROR "type error"

#define OV_ERROR_CODE_SOCKET_CREATE_ERROR 3004
#define OV_ERROR_DESC_SOCKET_CREATE_ERROR "socket create error"

/*
 *      ------------------------------------------------------------------------
 *
 *      DECODE ERRORS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_ERROR_CODE_SDP_DECODE 4000
#define OV_ERROR_DESC_SDP_DECODE "failed to decode SDP"

#define OV_ERROR_CODE_SDP_ENCODE 4001
#define OV_ERROR_DESC_SDP_ENCODE "failed to encode SDP"

#define OV_ERROR_CODE_JSON_DECODE 4002
#define OV_ERROR_DESC_JSON_DECODE "failed to decode JSON"

#define OV_ERROR_CODE_JSON_ENCODE 4003
#define OV_ERROR_DESC_JSON_ENCODE "failed to encode JSON"

#define OV_ERROR_CODE_CANDIDATE_DECODE 4004
#define OV_ERROR_DESC_CANDIDATE_DECODE "failed to decode ICE candidate"

#define OV_ERROR_CODE_JSON_INCOMPLETE 4005
#define OV_ERROR_DESC_JSON_INCOMPLETE "JSON is incomplete"

#define OV_ERROR_CODE_CODEC_ERROR 4006
#define OV_ERROR_DESC_CODEC_ERROR "unsupported codec"

/*
 *      ------------------------------------------------------------------------
 *
 *      AUTHENTICATION ERRORS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_ERROR_CODE_AUTH 5000
#define OV_ERROR_DESC_AUTH "authentication or authorization failed"

#define OV_ERROR_CODE_AUTH_SWTICH_ROLE 5001
#define OV_ERROR_DESC_AUTH_SWITCH_ROLE "switch role failed"

#define OV_ERROR_CODE_ALREADY_AUTHENTICATED 5002
#define OV_ERROR_DESC_ALREADY_AUTHENTICATED "already authenticated"

#define OV_ERROR_CODE_NOT_AUTHENTICATED 5003
#define OV_ERROR_DESC_NOT_AUTHENTICATED "user not authenticated"

#define OV_ERROR_CODE_AUTH_PERMISSION 5004
#define OV_ERROR_DESC_AUTH_PERMISSION "permission error"

#define OV_ERROR_CODE_AUTH_LDAP_USED 5005
#define OV_ERROR_DESC_AUTH_LDAP_USED "ldap used cannot change values"

/*
 *      ------------------------------------------------------------------------
 *
 *      BACKEND ERRORS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_ERROR_CODE_LIFETIME 6000
#define OV_ERROR_DESC_LIFETIME "lifetime ended"

#define OV_ERROR_CODE_DELETE 6001
#define OV_ERROR_DESC_DELETE "deleted"

#define OV_ERROR_CODE_INCOMPLETE 6002
#define OV_ERROR_DESC_INCOMPLETE "incomplete"

#define OV_ERROR_CODE_LOST 6003
#define OV_ERROR_DESC_LOST "lost resource"

/*
 *      ------------------------------------------------------------------------
 *
 *      OTHER
 *
 *      ------------------------------------------------------------------------
 */

#define OV_ERROR_INTERNAL_SERVER 10000
#define OV_ERROR_DESC_INTERNAL_SERVER "Serious unspecified internal error"

#define OV_ERROR_NO_RESOURCE 10010
#define OV_ERROR_DESC_NO_RESOURCE "No more resources"

#define OV_ERROR_UNKNOWN_USER 10011
#define OV_ERROR_DESC_UNKNOWN_USER "Unknown user"

#define OV_ERROR_UNKNOWN_LOOP 10012
#define OV_ERROR_CODE_UNKNOWN_LOOP OV_ERROR_UNKNOWN_LOOP
#define OV_ERROR_DESC_UNKNOWN_LOOP "Unknown loop"

#define OV_ERROR_UNKNOWN_RECORDER 10013
#define OV_ERROR_DESC_UNKNOWN_RECORDER "Unknown recorder"

#define OV_ERROR_ALREADY_ACQUIRED 10014
#define OV_ERROR_DESC_ALREADY_ACQUIRED "Resource already acquired"

#define OV_ERROR_NOT_RECORDING 10020
#define OV_ERROR_DESC_NOT_RECORDING "Currently not recording"

#define OV_ERROR_TIMEOUT 20001
#define OV_ERROR_DESC_TIMEOUT "Run into timeout"

#define OV_ERROR_UNTERMINATED 30001
#define OV_ERROR_DESC_UNTERMINATED "Unterminated message received"

#define OV_ERROR_MALFORMED_REQUEST 30010
#define OV_ERROR_DESC_MALFORMED_REQUEST "Request is malformed"

#define OV_ERROR_BAD_ARG 30020
#define OV_ERROR_DESC_BAD_ARG "Bad argument"

#define OV_ERROR_INVALID_RESULT 30030
#define OV_ERROR_DESC_INVALID_RESULT "Result is invalid"

#define OV_ERROR_AUDIO_IO 40100
#define OV_ERROR_DESC_AUDIO_IO "Audio hardware I/O error"

#define OV_ERROR_AUDIO_UNDERRUN 40110
#define OV_ERROR_DESC_AUDIO_UNDERRUN "Audio hardware buffer underflow"

/*****************************************************************************
             TRY-AGAIN errors - resources temporarily not available
 ****************************************************************************/

#define OV_ERROR_NON_CRITICAL 50000

#define OV_ERROR_TRY_AGAIN 51000
#define OV_ERROR_DESC_TRY_AGAIN "Could not be done. Try again"

#define OV_ERROR_SAME_RESOURCE 51100
#define OV_ERROR_DESC_SAME_RESOURCE "Resource with same ID already there"

#define OV_ERROR_TABLE_FULL 52010
#define OV_ERROR_DESC_TABLE_FULL                                               \
    "Translation cannot be added. Translation table full"

#define OV_ERROR_ALREADY_IN_STATE 58000
#define OV_ERROR_DESC_ALREADY_IN_STATE "System already in desired state"

#define OV_ERROR_NON_CRITICAL_END 59000

#endif /* ov_error_codes_h */
