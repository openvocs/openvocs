/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "../include/ov_sip_headers.h"
#include <ov_base/ov_convert.h>
#include <ov_base/ov_random.h>
#include <ov_base/ov_string.h>
#include <ov_encryption/ov_hash.h>

/*----------------------------------------------------------------------------*/

static char const *buffer_as_string(ov_buffer const *buf) {
    if (ov_ptr_valid(buf, "Invalid buffer")) {
        return (char const *)buf->start;
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ov_buffer *to_hex(ov_buffer *in) {

    static char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                           '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    if (ov_ptr_valid(in, "Invalid buffer - cannot convert to hex string")) {
        size_t target_len = 2 * in->length;
        ov_buffer *target = ov_buffer_create(target_len);
        target->length = target_len;

        char *str = (char *)target->start;

        for (size_t i = 0; i < in->length; ++i, str += 2) {
            uint8_t b = in->start[i];
            str[0] = hex[(b >> 4) & 0x0f];
            str[1] = hex[b & 0x0f];
        }

        return target;
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ov_buffer *hash_buffer(ov_buffer const *input) {

    if (ov_ptr_valid(input, "Invalid input to hash")) {

        size_t bytes_in_md5 = 128 / 8;

        ov_buffer *hash = ov_buffer_create(bytes_in_md5);
        hash->start[0] = 0;
        hash->length = bytes_in_md5;

        bool hash_successful =
            ov_hash_string(OV_HASH_MD5, input->start, input->length,
                           &hash->start, &hash->length);

        ov_buffer *hex = to_hex(hash);
        hash = ov_buffer_free(hash);

        if (!hash_successful) {
            ov_log_error("Could not hash value %s", (char *)input->start);
            return ov_buffer_free(hex);

        } else {
            hex->start[hex->length] = 0;
            return hex;
        }
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ov_buffer *create_auth_buffer(char const *uri, char const *realm,
                                     char const *user, char const *nonce,
                                     char const *hash) {

    if (ov_ptr_valid(nonce, "Nonce invalid") &&
        ov_ptr_valid(uri, "URI invalid") &&
        ov_ptr_valid(realm, "URI invalid") &&
        ov_ptr_valid(user, "User invalid") &&
        ov_ptr_valid(hash, "Hash invalid")) {

        return ov_buffer_from_strlist("Digest realm=\"", realm, "\", nonce=\"",
                                      nonce, "\", algorithm=MD5, username=\"",
                                      user, "\", uri=\"", uri,
                                      "\", response=\"", hash, "\"");

    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ov_buffer *auth_hash(char const *uri, char const *user,
                            char const *password, char const *realm,
                            char const *sip_method, char const *nonce) {

    if (ov_ptr_valid(uri, "No URI for SIP auth") &&
        ov_ptr_valid(user, "No user for SIP auth") &&
        ov_ptr_valid(password, "No password for SIP auth") &&
        ov_ptr_valid(realm, "No realm for SIP auth") &&
        ov_ptr_valid(sip_method, "No SIP method for SIP auth") &&
        ov_ptr_valid(nonce, "No nonce for SIP auth")) {

        // HA1=MD5(username:realm:password)
        // HA2=MD5(SIP-method:digestURI)
        // response=MD5(HA1:nonce:HA2)
        //
        // See https://nickvsnetworking.com/reverse-md5-on-sip-auth/

        // HA1=MD5(username:realm:password)
        ov_buffer *challenge =
            ov_buffer_from_strlist(user, ":", realm, ":", password);
        ov_buffer *hash1 = hash_buffer(challenge);
        challenge = ov_buffer_free(challenge);

        // HA2=MD5(SIP-method:digestURI)
        challenge = ov_buffer_from_strlist(sip_method, ":", uri);
        ov_buffer *hash2 = hash_buffer(challenge);
        challenge = ov_buffer_free(challenge);

        // response=MD5(HA1:nonce:HA2)
        challenge = ov_buffer_from_strlist(buffer_as_string(hash1), ":", nonce,
                                           ":", buffer_as_string(hash2));

        hash1 = ov_buffer_free(hash1);
        hash2 = ov_buffer_free(hash2);

        hash1 = hash_buffer(challenge);
        challenge = ov_buffer_free(challenge);

        return hash1;

    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

ov_buffer *ov_sip_headers_auth(char const *uri, char const *user,
                               char const *password, char const *realm,
                               char const *method, char const *nonce) {

    ov_buffer *hash = auth_hash(uri, user, password, realm, method, nonce);

    ov_buffer *auth =
        create_auth_buffer(uri, realm, user, nonce, buffer_as_string(hash));
    hash = ov_buffer_free(hash);

    return auth;
}

/*****************************************************************************
                                Get user and tag
 ****************************************************************************/

static char const *pos_of_char(char const *start, char const *end, char c) {

    if (0 == start) {
        return 0;
    } else {

        char const *pos = start;

        for (; (0 != *pos); ++pos) {

            if (*pos == c) {
                break;
            } else if ((0 != end) && (end <= pos)) {
                break;
            }
        }

        return pos;
    }
}

/*----------------------------------------------------------------------------*/

/**
 * If valid, points to first char AFTER the user string, e.g.
 *
 * "anton" = {a, n, t, o, n, 0}
 *                           ^
 *                           |
 *                       user_end_sign
 *
 * "anton;tag=other"
 *       ^
 *       |
 *       user_end_sign
 *
 * <sip:a@bertram;transport=tcp>;tag=cccc
 *                             ^
 *                             |
 *                             user end char
 */
static char const *user_end_char(char const *str) {

    if (!ov_ptr_valid(str, "No string given")) {
        return 0;

    } else if ('<' == str[0]) {
        return strchr(str, '>');

    } else {
        return pos_of_char(str, 0, ';');
    }
}

/*----------------------------------------------------------------------------*/

static char const *skip_prefix(char const *str, char const *end) {

    // BEWARE: This function assumes that it is fed a properly terminated
    // string, even if there is an end pointer

    char const *last_char_of_prefix = ov_string_chr(str, ':');

    if ((0 == last_char_of_prefix) || (end < last_char_of_prefix)) {
        return str;

    } else {
        return last_char_of_prefix + 1;
    }
}

/*----------------------------------------------------------------------------*/

/**
 * We got the user-part without the enclosing '<' and '>'.
 * Hence, it still looks like
 *
 *    [prefix:]user[;tagA=BBBB]
 *             ^  ^
 *             |  |
 *    We copy only this part
 *
 */
static ov_buffer *dup_pure_user_part_stripped(char const *start,
                                              char const *end) {

    end = pos_of_char(start, end, ';'); // Removes a possible postfix
    start = skip_prefix(start, end);

    ov_buffer *user = ov_buffer_create(end - start + 1);

    size_t i = 0;
    for (; start < end; ++start, ++i) {
        user->start[i] = *start;
    }
    user->start[i] = 0;
    user->length = strlen((char const *)user->start);

    return user;
}

/*----------------------------------------------------------------------------*/

static ov_buffer *dup_pure_user_part(char const *start, char const *end) {

    if ((0 == start) || (end < start)) {
        ov_log_error("Invalid user string");
        return 0;
    } else if ('<' == *start) {
        return dup_pure_user_part_stripped(start + 1, end);
    } else {
        return dup_pure_user_part_stripped(start, end);
    }
}

/*----------------------------------------------------------------------------*/

static char const *skip_alias(char const *str) {

    if (0 == str) {
        return 0;
    } else if ('\"' != str[0]) {
        return str;
    } else {

        for (++str; (0 != *str) && ('"' != *str); ++str)
            ;
        if ('"' == *str) {
            ++str;
        }
        return str;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_sip_headers_get_user_and_tag(char const *str, ov_buffer **user,
                                     ov_buffer **tag) {

    UNUSED(tag);

    char const *start =
        ov_string_ltrim(skip_alias(ov_string_ltrim(str, " ")), " ");
    char const *user_end = user_end_char(start);

    bool ok = true;

    if (0 != user) {
        *user = ov_buffer_free(*user);
        *user = dup_pure_user_part(start, user_end);
        ok = (0 != *user);
    }

    if (0 != tag) {
        *tag = ov_buffer_free(*tag);
        *tag = ov_string_value_for_key(str, "tag", '=', ";");
        ok = (0 != *tag);
    }

    return ok;
}

/*----------------------------------------------------------------------------*/

ov_buffer *ov_sip_headers_contact(char const *contact_header) {

    ov_buffer *contact = 0;
    ov_sip_headers_get_user_and_tag(contact_header, &contact, 0);

    return contact;
}

/*----------------------------------------------------------------------------*/

void ov_sip_headers_enable_caching() { ov_buffer_enable_caching(4); }

/*----------------------------------------------------------------------------*/
