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
        @file           ov_sdp_attribute.h
        @author         Markus Töpfer

        @date           2019-12-07

        @ingroup        ov_sdp_attribute

        @brief          Definition of some SDP attribute access functions

        ------------------------------------------------------------------------
*/
#ifndef ov_sdp_attribute_h
#define ov_sdp_attribute_h

#include "ov_json_value.h"
#include "ov_sdp.h"

/*----------------------------------------------------------------------------*/

/**
        Check if some SDP attribute with name is set

        @param attributes       head pointer of the attribute list
        @param name             name of the attribute
*/
bool ov_sdp_attribute_is_set(const ov_sdp_list *attributes, const char *name);

/*----------------------------------------------------------------------------*/

/**
        Delete some SDP attribute with name is set

        @param attributes       head pointer of the attribute list
        @param name             name of the attribute
*/
bool ov_sdp_attribute_del(ov_sdp_list **attributes, const char *name);

/*----------------------------------------------------------------------------*/

/**
        Get some attribute key (will deliver the first found)

        @param attributes       head pointer of the attribute list
        @param name             name of the attribute

        @returns                value of the attribute
*/
const char *ov_sdp_attribute_get(const ov_sdp_list *attributes,
                                 const char *name);

/*----------------------------------------------------------------------------*/

/**
        Get all attributes with key (will deliver all found)

        usage:

        const char *value;
        ov_sdp_attribute *attribute = session->attributes;
        OR
        ov_sdp_attribute *attribute = session->description->attributes;

        while(ov_sdp_attributes_iterate(&attribute, &value)){

                <do something with value>
        }

        @param attributes       head pointer of the attribute list
        @param name             name of the attribute

        @returns                true as long as attribute name is found

        @NOTE You MUST use a dedicated pointer to the attributes,
        as the iteration moves the external provided pointer to save
        position state during iteration.

        NEVER EVER use like this:
        while(ov_sdp_attributes_iterate(&session->attributes, &value))
*/
bool ov_sdp_attributes_iterate(ov_sdp_list **attributes, const char *name,
                               const char **out);

/*----------------------------------------------------------------------------*/

/**
        Add some attribute to the SDP instance.

        @NOTE this function will validate the input as valid SDP key or
        valid SDP key:value pair.

        @NOTE this will add the pointers to key and value, so do not
        free key and value, before unusing the ov_sdp_session.

        @NOTE this function is intendet to be used in volatite session
        parameter processing e.g. answer creation.

        EXAMPLE

        char *create_answer(const char *in, size_t len) {

                const char *ssrc  = "ssrc";
                const char *value = "3835337359";

                ov_sdp_session *session = ov_sdp_parse(in, len, NULL);

                < do something with session and create answer >

                < finaly add own ssrc >
                if (!ov_sdp_attribute_add(&session->description->attributes,
                        ssrc, value)) goto error;

                < create answer string >

                char *answer = ov_sdp_stringify(session);

                < remove volatile session >
                session = ov_sdp_session_free(session, NULL);

                return answer;

        }

        @param attributes       pointer to head pointer of the attribute list
        @param key              key to add
        @param value            (optional) value to add
        @param cache            (optional) cache to use

        @NOTE NEVER EVER leave function context, without taking care of the
        memory areas of added key value pairs with this function. A save way
        to protect memory areas is to stringify the SDP session and reparse
        on reusage. ov_sdp_session is intendet to be used with the
        stringify/parse approach.
*/
bool ov_sdp_attribute_add(ov_sdp_list **attributes, const char *key,
                          const char *value);

/*
 *      ------------------------------------------------------------------------
 *
 *      SPECIAL ACCESS FUNCTIONS (RFC 4566 attributes)
 *
 *      ------------------------------------------------------------------------
 */

bool ov_sdp_is_recvonly(const ov_sdp_description *description);
bool ov_sdp_is_sendrecv(const ov_sdp_description *description);
bool ov_sdp_is_sendonly(const ov_sdp_description *description);
bool ov_sdp_is_inactive(const ov_sdp_description *description);

/*----------------------------------------------------------------------------*/

const char *ov_sdp_get_orientation(const ov_sdp_description *description);

const char *ov_sdp_get_type(const ov_sdp_description *description);

const char *ov_sdp_get_charset(const ov_sdp_description *description);

const char *ov_sdp_get_sdplang(const ov_sdp_description *description);

const char *ov_sdp_get_lang(const ov_sdp_description *description);

uint64_t ov_sdp_get_framerate(const ov_sdp_description *description);

uint64_t ov_sdp_get_quality(const ov_sdp_description *description);

/*----------------------------------------------------------------------------*/

/**
        Searches the format fmt within the attributes
        and returns the value on success.

        e.g.    a=fmtp:100 parameter
                ov_sdp_get_rtpmap(description, "100")
                returns string "parameter\0"

        @param description      description to search
        @param fmt              format to search
*/
const char *ov_sdp_get_fmtp(const ov_sdp_description *description,
                            const char *fmt);

/*----------------------------------------------------------------------------*/

/**
        Delete a format with name.

        @param description      description to search
        @param name             name to search
        @param cache            (optional) cache
*/
bool ov_sdp_del_fmtp(const ov_sdp_description *description, const char *name);

/*----------------------------------------------------------------------------*/

/**
        Searches the rtpmap fmt within the attributes
        and returns the value on success.

        e.g.    a=rtpmap:98 L16/16000/2
                ov_sdp_get_rtpmap(description, "98")
                returns string "L16/16000/2\0"

        @param description      description to search
        @param fmt              format to search
*/
const char *ov_sdp_get_rtpmap(const ov_sdp_description *description,
                              const char *fmt);

/*----------------------------------------------------------------------------*/

/**
        Delete a rtpmap with name.

        @param description      description to search
        @param name             name to search
        @param cache            (optional) cache
*/
bool ov_sdp_del_rtpmap(const ov_sdp_description *description, const char *name);

/*----------------------------------------------------------------------------*/

/**
        Add some format to the media formats of a SDP description.

        @param description      media description to use
        @param fmt              format string to add
        @param cache            optional cache to use
*/
bool ov_sdp_add_fmt(ov_sdp_description *description, const char *fmt);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_sdp_rtpmap_to_json(const ov_sdp_description *desc,
                                     const char *fmt);

#endif /* ov_sdp_attribute_h */
