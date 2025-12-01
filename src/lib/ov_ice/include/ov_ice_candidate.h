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
        @file           ov_ice_candidate.h
        @author         Markus Töpfer

        @date           2024-09-18


        ------------------------------------------------------------------------
*/
#ifndef ov_ice_candidate_h
#define ov_ice_candidate_h

typedef struct ov_ice_candidate ov_ice_candidate;

#include "ov_ice_base.h"
#include "ov_ice_server.h"
#include "ov_ice_stream.h"

#include <ov_base/ov_buffer.h>
#include <ov_base/ov_node.h>

/*----------------------------------------------------------------------------*/

typedef enum {

  OV_ICE_INVALID = 0,
  OV_ICE_HOST = 1,
  OV_ICE_SERVER_REFLEXIVE = 2,
  OV_ICE_PEER_REFLEXIVE = 3,
  OV_ICE_RELAYED = 4

} ov_ice_candidate_type;

/*----------------------------------------------------------------------------*/

typedef enum {

  OV_ICE_GATHERING_FAILED = 0,
  OV_ICE_GATHERING = 1,
  OV_ICE_GATHERING_SUCCESS = 2

} ov_ice_candidate_gathering;

/*----------------------------------------------------------------------------*/

typedef struct {

  ov_node node;
  ov_buffer *key;
  ov_buffer *val;

} ov_ice_candidate_extension;

/*----------------------------------------------------------------------------*/

struct ov_ice_candidate {

  ov_node node;

  ov_ice_base *base;
  ov_ice_stream *stream;

  uint8_t transaction_id[13];
  bool trickled;

  ov_ice_candidate_gathering gathering;

  ov_ice_candidate_type type;
  ov_socket_transport transport;

  uint8_t foundation[33];
  uint16_t component_id;

  uint32_t priority;
  uint32_t priority_prflx;

  char addr[OV_HOST_NAME_MAX];
  uint16_t port;

  char raddr[OV_HOST_NAME_MAX];
  uint16_t rport;

  ov_ice_candidate_extension *ext;

  char *string;

  ov_ice_server server;

  struct {

    char *realm;
    char *nonce;

  } turn;
};

/*----------------------------------------------------------------------------*/

typedef struct {

  const char *candidate;
  uint64_t SDPMlineIndex;
  uint64_t SDPMid;
  const char *ufrag;

} ov_ice_candidate_info;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_ice_candidate *ov_ice_candidate_create(ov_ice_stream *stream);
ov_ice_candidate *ov_ice_candidate_cast(const void *data);
void *ov_ice_candidate_free(void *self);

ov_ice_candidate_type ov_ice_candidate_type_from_string(const char *string,
                                                        size_t length);
const char *ov_ice_candidate_type_to_string(ov_ice_candidate_type type);

/*----------------------------------------------------------------------------*/

/**
        Parse some candidate from string.

        Expects some candidate string of form:

            "x 1 udp 2122260223 1.2.3.4 37842 typ host"

        @NOTE "candidate:x 1 udp 2122260223 1.2.3.4 37842 typ host" will fail

        @param string   of type: |foundation SP id SP transport SP ...|
        @param length   length of the string

        @NOTE           will parse all extensions
*/
ov_ice_candidate *ov_ice_candidate_from_string(const char *start,
                                               size_t length);

/*----------------------------------------------------------------------------*/

/**
        Create a candidate string.

        e.g. "x 1 udp 2122260223 1.2.3.4 37842 typ host"

        @returns        allocated string, MUST be freed
        @NOTE           will encode all extensions
*/
char *ov_ice_candidate_to_string(const ov_ice_candidate *candidate);

/*----------------------------------------------------------------------------*/

/**
        Parse the candidate from string with length.

        Expects some candidate string of form:

            "x 1 udp 2122260223 1.2.3.4 37842 typ host"

        @NOTE "candidate:x 1 udp 2122260223 1.2.3.4 37842 typ host" will fail

        @param candidate        candidate to fill
        @param string   of type: |foundation SP id SP transport SP ...|
        @param length   length of the string
*/
bool ov_ice_candidate_parse(ov_ice_candidate *candidate, const char *string,
                            size_t length);

/*----------------------------------------------------------------------------*/

/**
        Parse some candidate from string.

        Expects some candidate string of form:

            "candidate:x 1 udp 2122260223 1.2.3.4 37842 typ host"

        @param string   of type: |foundation SP id SP transport SP ...|
        @param length   length of the string

        @NOTE           will parse all extensions
*/
ov_ice_candidate *ov_ice_candidate_from_string_candidate(const char *string,
                                                         size_t length);

ov_json_value *ov_ice_candidate_to_json(const ov_ice_candidate *candidate);
ov_ice_candidate ov_ice_candidate_from_json(const ov_json_value *candidate);

/*----------------------------------------------------------------------------*/

/**
    Expects some JSON based candidate of form:

    {
        "candidate" : "candidate:x 1 udp 2122260223 1.2.3.4 37842 typ host"
    }

    OR the ov_json_string data only

        "candidate:x 1 udp 2122260223 1.2.3.4 37842 typ host"

    @params val     object with key candidate or ov_json_string candidate
*/
ov_ice_candidate *ov_ice_candidate_from_json_string(const ov_json_value *input);

/*----------------------------------------------------------------------------*/

/**
    This function will parse some JSON value for the
    keys of the candidate info and point to the source
    within the JSON value. (pointer based parsing)
*/
ov_ice_candidate_info
ov_ice_candidate_info_from_json(const ov_json_value *input);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_candidate_info_to_json(ov_ice_candidate_info info);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_candidate_json_info(ov_ice_candidate *candidate,
                                          const char *session_uuid,
                                          const char *ufrag,
                                          const int stream_id);

/*----------------------------------------------------------------------------*/

bool ov_ice_candidate_calculate_priority(ov_ice_candidate *cand,
                                         uint32_t interfaces);

#endif /* ov_ice_candidate_h */
