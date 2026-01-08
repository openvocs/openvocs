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
        @file           ov_ice_string.c
        @author         Markus Toepfer

        @date           2018-05-28

        @ingroup        ov_ice

        @brief          Implementation ov_ice sdp grammar.


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_string.h"

#include <ov_base/ov_sdp_grammar.h>
#include <ov_base/ov_time.h>

bool ov_ice_string_fill_random(char *buffer, size_t length) {

    if (!buffer || length < 1)
        return false;

    srandom(ov_time_get_current_time_usecs());
    uint64_t number = 0;

    for (size_t i = 0; i < length; i++) {

        while (true) {

            number = random();
            number = (number * 0xFF) / RAND_MAX;

            if (isalnum(number) || (number == '+') || (number == '/'))
                break;
        }

        buffer[i] = number;
    }

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *                               GRAMMAR
 *
 *      ------------------------------------------------------------------------
 *
 */

bool ov_ice_string_is_ice_char(const char *start, size_t length) {

    if (!start || length == 0)
        return false;

    for (size_t i = 0; i < length; i++) {

        if (!isalnum(start[i]))
            if (start[i] != '+')
                if (start[i] != '/')
                    return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_foundation(const char *start, size_t length) {

    if (!start || length == 0 || length > 32)
        return false;

    return ov_ice_string_is_ice_char(start, length);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_component_id(const char *start, size_t length) {

    if (!start || length == 0 || length > 5)
        return false;

    for (size_t i = 0; i < length; i++) {

        if (!isdigit(start[i]))
            return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_transport(const char *start, size_t length) {

    return ov_ice_string_is_token(start, length);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_transport_extension(const char *start, size_t length) {

    return ov_sdp_is_token(start, length);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_priority(const char *start, size_t length) {

    if (!start || length == 0 || length > 10)
        return false;

    for (size_t i = 0; i < length; i++) {

        if (!isdigit(start[i]))
            return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_candidate_type(const char *start, size_t length) {

    if (!start || length < 5)
        return false;

    if (0 != strncmp(start, "typ ", 4))
        return false;

    return ov_sdp_is_token(start + 4, length - 4);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_host(const char *start, size_t length) {

    if (!start || length != 4)
        return false;

    if (0 == strncmp(start, "host", 4))
        return true;

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_srflx(const char *start, size_t length) {

    if (!start || length != 5)
        return false;

    if (0 == strncmp(start, "srflx", 5))
        return true;

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_prflx(const char *start, size_t length) {

    if (!start || length != 5)
        return false;

    if (0 == strncmp(start, "prflx", 5))
        return true;

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_relay(const char *start, size_t length) {

    if (!start || length != 5)
        return false;

    if (0 == strncmp(start, "relay", 5))
        return true;

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_token(const char *start, size_t length) {

    return ov_sdp_is_token(start, length);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_connection_address(const char *start, size_t length) {

    return ov_sdp_is_address(start, length);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_connection_port(const char *start, size_t length) {

    if (!start || length == 0 || length > 5)
        return false;

    char *next = NULL;
    int64_t number = 0;

    number = strtoll(start, &next, 10);
    if (number == 0)
        return false;

    if (next != start + length)
        return false;

    if ((number < 1) || (number > 65535))
        return false;

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_related_address(const char *start, size_t length) {

    if (!start || length < 7)
        return false;

    if (0 != strncmp(start, "raddr ", 6))
        return false;

    return ov_ice_string_is_connection_address(start + 6, length - 6);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_related_port(const char *start, size_t length) {

    if (!start || length < 7)
        return false;

    if (0 != strncmp(start, "rport ", 6))
        return false;

    return ov_ice_string_is_connection_port(start + 6, length - 6);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_extension_name(const char *start, size_t length) {

    return ov_sdp_is_byte_string(start, length);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_extension_value(const char *start, size_t length) {

    return ov_sdp_is_byte_string(start, length);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_ice_lite_key(const char *start, size_t length) {

    if (!start || length != 8)
        return false;

    if (0 == strncmp("ice-lite", start, length))
        return true;

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_ice_mismatch_key(const char *start, size_t length) {

    if (!start || length != 12)
        return false;

    if (0 == strncmp("ice-mismatch", start, length))
        return true;

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_ice_pwd_key(const char *start, size_t length) {

    if (!start || length != 7)
        return false;

    if (0 == strncmp("ice-pwd", start, length))
        return true;

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_ice_ufrag_key(const char *start, size_t length) {

    if (!start || length != 9)
        return false;

    if (0 == strncmp("ice-ufrag", start, length))
        return true;

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_ice_options_key(const char *start, size_t length) {

    if (!start || length != 11)
        return false;

    if (0 == strncmp("ice-options", start, length))
        return true;

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_ice_pwd(const char *start, size_t length) {

    if (!start || length < 22 || length > 256)
        return false;

    return ov_ice_string_is_ice_char(start, length);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_ice_ufrag(const char *start, size_t length) {

    if (!start || length < 4 || length > 256)
        return false;

    return ov_ice_string_is_ice_char(start, length);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_string_is_ice_option_tag(const char *start, size_t length) {

    if (!start || length == 0)
        return false;

    return ov_ice_string_is_ice_char(start, length);
}