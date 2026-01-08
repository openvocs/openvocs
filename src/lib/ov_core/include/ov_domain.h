/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_domain.h
        @author         Markus Töpfer

        @date           2020-12-18

        ov_domain is a structure intendet do be used in ov_webserver as an array
        of domains and to hold all domain specific data.

        ------------------------------------------------------------------------
*/
#ifndef ov_domain_h
#define ov_domain_h

#include <limits.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_json.h>

#include <openssl/conf.h>
#include <openssl/ssl.h>

#include "ov_websocket_message.h"

typedef struct ov_domain ov_domain;
typedef struct ov_domain_config ov_domain_config;

#define OV_DOMAIN_MAGIC_BYTE 0x0D0C

/*----------------------------------------------------------------------------*/

struct ov_domain_config {

    /* Support UTF8 here for non ASCCI names */

    struct {

        uint8_t start[PATH_MAX];
        size_t length;

    } name;

    /* Default MAY be set in exactly of the domains,
     * of some domain configuration,
     * to allow IP based HTTPs access instead of SNI. */
    bool is_default;

    /* Certificate settings */

    struct {

        char cert[PATH_MAX];
        char key[PATH_MAX];

        struct {

            char file[PATH_MAX]; // path to CA verify file
            char path[PATH_MAX]; // path to CAs to use

        } ca;

    } certificate;

    /* Document root path */

    char path[PATH_MAX];
};

/*----------------------------------------------------------------------------*/

struct ov_domain {

    const uint16_t magic_byte;
    ov_domain_config config;

    struct {

        SSL_CTX *tls;

    } context;

    struct {

        ov_websocket_message_config config;

        /* dict for uri specific callbacks */
        ov_dict *uri;

    } websocket;

    struct {

        /* dict for uri specific callbacks */
        ov_dict *uri;

    } event_handler;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
    This function will set the magic byte,
    and clean all other values of the domain struct.
*/
bool ov_domain_init(ov_domain *domain);

/*----------------------------------------------------------------------------*/

/**
    Verify existence and read access to all required path parameter set in
    the config.
*/
bool ov_domain_config_verify(const ov_domain_config *config);

/*----------------------------------------------------------------------------*/

/**
    This function will read all JSON files at path and create the
    ov_domain array.

    It will initialized the domains TLS context and load the certificates.

    @param path     PATH to read
    @param size     pointer to size of array to be set
    @param array    pointer to array to create *array MUST be NULL!

    @retruns true if the array was created and all configs are set in array
    @returns false if ANY TLS context was NOT initialized
*/
bool ov_domain_load(const char *path, size_t *size, ov_domain **array);

/*----------------------------------------------------------------------------*/

/**
    This funcion will walk some array of ov_domain entires and
    clean all data contained, before actually freeing the array.

    It SHOULD be called before the array will run out of scope to free all
    SSL related content.

    @param size     size of the array
    @param array    pointer of the array
*/
bool ov_domain_array_clean(size_t size, ov_domain *array);

/*----------------------------------------------------------------------------*/

/**
    This function will clear all entry of some domain array, and free the
    pointer to the array.

    @param size     size of the array
    @param array    pointer of the array
*/
ov_domain *ov_domain_array_free(size_t size, ov_domain *array);

/*----------------------------------------------------------------------------*/

/**
    This function will unset the TLS context of a domain.
    It MUST be called to free up the SSL context.
*/
void ov_domain_deinit_tls_context(ov_domain *domain);

/*----------------------------------------------------------------------------*/

/**
    Create a config out of some JSON object.

    Will check for KEY OV_KEY_DOMAIN or try to parse the input direct.

    Expected input:

    {
        "name":"<some name>",
        "path":"<path document root>",
        "certificate":
        {
            "authority":
            {
                "file":"<path ca file>",
                "path":"<path ca dir>"
            },
            "file":"<path certificate file>",
            "key":"<path certificate key>"
        }
    }
    @param value    JSON value to parse
*/
ov_domain_config ov_domain_config_from_json(const ov_json_value *value);

/*----------------------------------------------------------------------------*/

/**
    Create the JSON config of form @see ov_domain_config_from_json
*/
ov_json_value *ov_domain_config_to_json(ov_domain_config config);

#endif /* ov_domain_h */
