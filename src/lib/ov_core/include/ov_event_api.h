/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_event_api.h
        @author         Markus Töpfer

        @date           2021-02-04

        This API defines the general JSON API used within openvocs and provides
        some standard functions for this API.

        In addition API keys as well as EVENT keys SHOULD be defined here for
        generic events and in some actual implementation for the custom events
        of the implementation.

        e.g. voice client API events SHOULD be defined in some voice client api
             mixer events SHOULD be defined in some mixer API
             ...

        Standard message format:

            {
                "event"    : "name",
                "uuid"     : "080c1f1c-c457-44c8-82a3-8b6973558369",
                "type"     : "unicast",
                "version"  : 1,

                "request"  : { ... },
                "response" : { ... },
                "error"    : { ... }
            }


        ------------------------------------------------------------------------
*/
#ifndef ov_event_api_h
#define ov_event_api_h

#include <ov_base/ov_json.h>

#define OV_EVENT_API_KEY_EVENT "event"
#define OV_EVENT_API_KEY_UUID "uuid"
#define OV_EVENT_API_KEY_TYPE "type"
#define OV_EVENT_API_KEY_VERSION "version"

#define OV_EVENT_API_KEY_REQUEST "request"
#define OV_EVENT_API_KEY_RESPONSE "response"
#define OV_EVENT_API_KEY_PARAMETER "parameter"

#define OV_EVENT_API_KEY_ERROR "error"
#define OV_EVENT_API_KEY_CODE "code"
#define OV_EVENT_API_KEY_DESCRIPTION "description"

#define OV_EVENT_API_KEY_CLIENT "client"
#define OV_EVENT_API_KEY_CHANNEL "channel"
#define OV_EVENT_API_KEY_PATH "path"

#define OV_EVENT_API_KEY_SALT "salt"
#define OV_EVENT_API_KEY_ID "id"
#define OV_EVENT_API_KEY_USER "user"
#define OV_EVENT_API_KEY_ROLE "role"
#define OV_EVENT_API_KEY_PASSWORD "password"

#define OV_EVENT_API_KEY_TIMESTAMP "timestamp"

#define OV_EVENT_API_PING "ping"
#define OV_EVENT_API_PONG "pong"
#define OV_EVENT_API_LOGOUT "logout"

#define OV_EVENT_API_LOGIN "login"
#define OV_EVENT_API_AUTHENTICATE "authenticate" // alternative to login

#define OV_EVENT_API_UPDATE_LOGIN "update_login"

#define OV_EVENT_API_AUTHORISE "authorise"
#define OV_EVENT_API_AUTHORIZE "authorize" // alternative to authorise

#define OV_EVENT_API_PERMISSION "permission" // loop permissions

#define OV_EVENT_API_USER_ROLES "user_roles"
#define OV_EVENT_API_USER_LOOPS "user_loops"
#define OV_EVENT_API_ROLE_LOOPS "role_loops"
#define OV_EVENT_API_ADMIN_PROJECTS "admin_projects"
#define OV_EVENT_API_ADMIN_DOMAINS "admin_domains"

#define OV_EVENT_API_SET_LAYOUT "set_layout"
#define OV_EVENT_API_GET_LAYOUT "get_layout"

#define OV_EVENT_API_DELETE "delete"
#define OV_EVENT_API_CREATE "create"
#define OV_EVENT_API_UPDATE "update"
#define OV_EVENT_API_GET "get"
#define OV_EVENT_API_GET_KEY "get_key"
#define OV_EVENT_API_UPDATE_KEY "update_key"
#define OV_EVENT_API_DELETE_KEY "delete_key"
#define OV_EVENT_API_UPDATE_PASSWORD "update_password"

#define OV_EVENT_API_LDAP_IMPORT "ldap_import"

#define OV_EVENT_API_REGISTER "register"
#define OV_EVENT_API_MEDIA "media"

#define OV_EVENT_API_CREATE "create"
#define OV_EVENT_API_UPDATE "update"
#define OV_EVENT_API_GET "get"

#define OV_EVENT_API_REGISTER "register"
#define OV_EVENT_API_MEDIA "media"

#define OV_EVENT_API_SWITCH_LOOP_STATE "switch_loop_state"
#define OV_EVENT_API_SWITCH_LOOP_VOLUME "switch_loop_volume"
#define OV_EVENT_API_TALKING "talking"

#define OV_EVENT_API_STATE_MIXER "state_mixer"
#define OV_EVENT_API_STATE_CONNECTIONS "state_connections"
#define OV_EVENT_API_STATE_SESSION "state_session"

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_event_api_event_is(const ov_json_value *val, const char *event);

const char *ov_event_api_get_event(const ov_json_value *val);
const char *ov_event_api_get_uuid(const ov_json_value *val);
const char *ov_event_api_get_type(const ov_json_value *val);
const char *ov_event_api_get_channel(const ov_json_value *val);
double ov_event_api_get_version(const ov_json_value *val);

ov_json_value *ov_event_api_get_error(const ov_json_value *val);
ov_json_value *ov_event_api_get_request(const ov_json_value *val);
ov_json_value *ov_event_api_get_response(const ov_json_value *val);
ov_json_value *ov_event_api_get_parameter(const ov_json_value *val);

bool ov_event_api_add_error(ov_json_value *msg, uint64_t code,
                            const char *desc);

bool ov_event_api_get_error_parameter(const ov_json_value *val, uint64_t *code,
                                      const char **desc);

uint64_t ov_event_api_get_error_code(const ov_json_value *val);

bool ov_event_api_set_event(ov_json_value *val, const char *str);
bool ov_event_api_set_uuid(ov_json_value *val, const char *str);
bool ov_event_api_set_type(ov_json_value *val, const char *str);
bool ov_event_api_set_channel(ov_json_value *val, const char *str);
bool ov_event_api_set_version(ov_json_value *val, double version);
ov_json_value *ov_event_api_set_parameter(ov_json_value *val);

bool ov_event_api_set_current_timestamp(ov_json_value *val);

/*----------------------------------------------------------------------------*/

/**
        Create a standard event message.

        {
                "event"    : "name",
                "uuid"     : "080c1f1c-c457-44c8-82a3-8b6973558369",
                "type"     : "unicast",
                "version"  : 1
        }

        @param name     event key to be set
        @param uuid     (optional) uuid string to be set
        @param version  (optional) version to be set

*/
ov_json_value *ov_event_api_message_create(const char *name, const char *uuid,
                                           double version);

/*----------------------------------------------------------------------------*/

/**
        Create a standard success response from an input event.

        The input will be copied to request, the event name will be copied from
        input and the response will be empty.

        {
                "event"         : "name",
                "request"       : { ... },
                "response"      : { }
        }

        @param input    input event
*/
ov_json_value *ov_event_api_create_success_response(const ov_json_value *input);

/*----------------------------------------------------------------------------*/

/**
        Create a standard error response from an input event.

        The input will be copied to request, the event name will be copied from
        input and the response will be empty.

        {
                "event"         : "name",
                "request"       : { ... },
                "response"      : { },
                "error"   :
                {
                        "code"        : 100,
                        "description" : "some description"
                }
        }

        @param input    input event
        @param code     (optional) error code to set
        @param desc     (optional) error description to set
*/
ov_json_value *ov_event_api_create_error_response(const ov_json_value *input,
                                                  uint64_t code,
                                                  const char *desc);

/*----------------------------------------------------------------------------*/

/**
        Create a standard error without an input event.

        The input will be copied to request, the event name will be copied from
        input and the response will be empty.

        {
                "error"   :
                {
                        "code"        : 100,
                        "description" : "some description"
                }
        }

        @param code     (optional) error code to set
        @param desc     (optional) error description to set
*/
ov_json_value *ov_event_api_create_error(uint64_t code, const char *desc);

/*----------------------------------------------------------------------------*/

/**
        Create a standard error without an input event.

        {
            "code"        : 100,
            "description" : "some description"
        }

        @param code     (optional) error code to set
        @param desc     (optional) error description to set
*/
ov_json_value *ov_event_api_create_error_code(uint64_t code, const char *desc);

/*
 *      ------------------------------------------------------------------------
 *
 *      COMMON API EVENT MESSAGES
 *
 *      ------------------------------------------------------------------------
 */

ov_json_value *ov_event_api_create_ping();
ov_json_value *ov_event_api_create_pong();

ov_json_value *ov_event_api_create_path(const char *path);
const char *ov_event_api_get_path(const ov_json_value *input);

#endif /* ov_event_api_h */
