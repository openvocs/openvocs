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
        @file           ov_sdp.h
        @author         Markus Töpfer

        @date           2019-12-08

        @ingroup        ov_sdp

        @brief          Definition of SDP functions.

                        @NOTE this implementation will use some internal buffer
                        to store the SDP data within a memory block and uses
                        masking and pointers to the internal buffer.

                        If a session is manipulated over a function volatile
                        data may be set, and the session MUST be persisted
                        before leaving functional scope, to carry that data.

                        e.g.

                        bool manipulate(ov_sdp_session *session) {

                            session->name = "test"
                            session->origin.name = "some origin"

                            if (!ov_sdp_session_persist(session))
                                return false;

                            return true;

                        }

                        For attribute manipulation @see ov_sdp_attribute
                        For general grammar @see ov_sdp_grammar.h

        ------------------------------------------------------------------------
*/
#ifndef ov_sdp_h
#define ov_sdp_h

#include <stdbool.h>
#include <stdlib.h>

#include "ov_buffer.h"
#include "ov_config_keys.h"
#include "ov_dict.h"
#include "ov_json.h"
#include "ov_node.h"

#define OV_SDP_SESSION_MAGIC_BYTE 0x5E50

// node types
#define OV_SDP_LIST_TYPE 0x5D01
#define OV_SDP_TIME_TYPE 0x5D02
#define OV_SDP_CONNECTION_TYPE 0x5D03
#define OV_SDP_DESCRIPTION_TYPE 0x5D04

/*
 *      ------------------------------------------------------------------------
 *
 *      STRUCTURES
 *
 *      ------------------------------------------------------------------------
 */

typedef struct ov_sdp_session ov_sdp_session;
typedef struct ov_sdp_description ov_sdp_description;

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_node node;
    const char *value; // standard value pointer
    const char *key;   // only used in attributes

} ov_sdp_list;

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_node node;

    uint64_t start;
    uint64_t stop;

    ov_sdp_list *repeat;
    ov_sdp_list *zone;

} ov_sdp_time;

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_node node;

    const char *nettype;
    const char *addrtype;
    const char *address;

} ov_sdp_connection;

/*----------------------------------------------------------------------------*/

struct ov_sdp_session {

    /*
     *      Implementation switches
     */

    uint16_t magic_byte;
    uint16_t type;

    /*
     *      Mandatory elements
     */

    uint8_t version;
    const char *name;

    struct {

        const char *name;
        uint64_t id;
        uint64_t version;
        ov_sdp_connection connection;

    } origin;

    ov_sdp_time *time;

    /*
     *      Optional elements
     */

    const char *info;
    const char *uri;
    const char *key;

    ov_sdp_connection *connection;

    ov_sdp_list *email;
    ov_sdp_list *phone;

    ov_dict *bandwidth; // dict of char*:intptr_t

    ov_sdp_list *attributes;
    ov_sdp_description *description;
};

/*----------------------------------------------------------------------------*/

struct ov_sdp_description {

    ov_node node;

    /*
     *      Mandatory elements
     */

    struct {

        const char *name;
        const char *protocol;

        uint16_t port;
        uint16_t port2;

        ov_sdp_list *formats;

    } media;

    /*
     *      Optional elements
     */

    const char *info;
    const char *key;

    ov_sdp_connection *connection;

    ov_dict *bandwidth; // dict of char*:intptr_t
    ov_sdp_list *attributes;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
        Validation of a string as a valid SDP message

        This method will check if the content is valid SDP using
        pointer based string matching.

        @param string   string to validate
        @param size     size of the string.
*/
bool ov_sdp_validate(const char *string, size_t size);

/*----------------------------------------------------------------------------*/

/**
        Parse some string as ov_sdp_session.

        This method will generate the session. The session will
        host all information required.

        @param string   string to validate
        @param size     size of the string
*/
ov_sdp_session *ov_sdp_parse(const char *string, size_t length);

/*----------------------------------------------------------------------------*/

/**
        Stringify an ov_sdp_session.

        @param session  session to stringify
        @returns        allocated SDP string of the session

        @NOTE   returned string MUST ne freed.
        @NOTE   stringify performs session validation of
                all items inline during the stringification.

        @NOTE   to create a JSON string conform string you may use:
                ov_sdp_stringify(session, true)
*/
char *ov_sdp_stringify(const ov_sdp_session *session, bool escaped);

/*----------------------------------------------------------------------------*/

/**
        Convert a SDP session description to JSON

        This function will create a JSON value of form:

        {
            "version" : 0,
            "name" : "session_name",
            ...
        }

        @param session          session to convert
        @param json             (optional) json instance
*/
ov_json_value *ov_sdp_to_json(const ov_sdp_session *session);

/*----------------------------------------------------------------------------*/

/**
        Convert a JSON session description to SDP

        This function expects some input of form:

        {
            "version" : 0,
            "name" : "session_name",
            ...
        }

        @param session          session to convert
*/
ov_sdp_session *ov_sdp_from_json(const ov_json_value *session);

/*----------------------------------------------------------------------------*/

/**
    This function parses some SDP session from a JSON string.
    The input may be of the following forms:

    { "v=0\\r\\n..." }
    { sdp: "v=0\\r\\n..." }

    Input SDP is not a JSON object, but content of some JSON string.

*/
ov_sdp_session *ov_sdp_session_from_json(const ov_json_value *value);

/*----------------------------------------------------------------------------*/

/**
    This function will create an ov_json_value with some escaped SDP string.

    { "v=0\\r\\n..." }

    Output JSON is not an object, but some JSON string.

    @param session  session to stringify
*/
ov_json_value *ov_sdp_session_to_json(const ov_sdp_session *session);

/*
 *      ------------------------------------------------------------------------
 *
 *      CACHING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Enable a global SDP cache.

        If enable is called, disable MUST be called before leaving the
        program scope to avoid memleaks.
*/
void ov_sdp_enable_caching(size_t capacity);

/*----------------------------------------------------------------------------*/

/**
        Disable the global SDP cache.
*/
void ov_sdp_disable_caching();

/*
 *      ------------------------------------------------------------------------
 *
 *      STRUCTURE FUNCTIONS
 *
 *      ... these function will use caching if enabled
 *      ... *_free will free all substructures contained (recache if enabled)
 *
 *      These structure functions SHOULD be used to manipulate / change
 *      some SDP session.
 *
 *      ------------------------------------------------------------------------
 */

ov_buffer *ov_sdp_buffer_create();
ov_buffer *ov_sdp_buffer_free(ov_buffer *buffer);

ov_sdp_description *ov_sdp_description_create();
ov_sdp_description *ov_sdp_description_free(ov_sdp_description *description);

ov_dict *ov_sdp_bandwidth_create();
ov_dict *ov_sdp_bandwidth_free(ov_dict *dict);

ov_sdp_list *ov_sdp_list_create();
ov_sdp_list *ov_sdp_list_free(ov_sdp_list *list);

ov_sdp_time *ov_sdp_time_create();
ov_sdp_time *ov_sdp_time_free(ov_sdp_time *time);

ov_sdp_connection *ov_sdp_connection_create();
ov_sdp_connection *ov_sdp_connection_free(ov_sdp_connection *connection);

/*
 *      ------------------------------------------------------------------------
 *
 *      SESSION
 *
 *      ------------------------------------------------------------------------
 */

ov_sdp_session *ov_sdp_session_create();

/*----------------------------------------------------------------------------*/

ov_sdp_session *ov_sdp_session_cast(const void *self);

/*----------------------------------------------------------------------------*/

/**
        Clear a session,

        if some cache is provided, all child structures will be recached.
        if no cache is provided all child structures will be freed

        @param session  session to clear

        @returns        true if the session was cleared.
                        false if the session was unexpected input
*/
bool ov_sdp_session_clear(ov_sdp_session *session);

/*----------------------------------------------------------------------------*/

/**
        Free a session,

        @param session  session to free
        @returns        true if the session was freed.
                        false if the session was unexpected input
*/
void *ov_sdp_session_free(void *session);

/*----------------------------------------------------------------------------*/

/**
        Persist a SDP session with volatile content data.

        This function should be called, whenever leaving scope with volatile
        items.

        On error the session status is unspecified.

            1) If the session is invalid, the session will be unchanged.
            2) On persistance error, the session content is undefined.

        On Success the sessions status is persisted and safe to use outside
        of the function scope using the pointer
*/
bool ov_sdp_session_persist(ov_sdp_session *session);

/*----------------------------------------------------------------------------*/

/**
        Copy a SDP session.
*/
ov_sdp_session *ov_sdp_session_copy(const ov_sdp_session *session);

#include "ov_sdp_attribute.h"
#include "ov_sdp_grammar.h"

#endif /* ov_sdp_h */
