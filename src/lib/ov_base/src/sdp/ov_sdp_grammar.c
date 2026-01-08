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
        @file           ov_sdp_grammar.c
        @author         Markus Toepfer

        @date           2018-04-12

        @ingroup        ov_sdp

        @brief          Implementation of some standard grammar of SDP.


        ------------------------------------------------------------------------
*/
#include "../../include/ov_sdp_grammar.h"

#include <arpa/inet.h>
#include <ctype.h>

bool ov_sdp_is_text(const char *buffer, uint64_t length) {

    // MUST be a valid UTF8 sequence
    return ov_utf8_validate_sequence((uint8_t *)buffer, length);
}

/*----------------------------------------------------------------------------*/

bool ov_sdp_is_username(const char *buffer, uint64_t length) {

    if (!buffer || length < 1)
        return false;

    // MUST be a valid SDP text sequence
    if (!ov_sdp_is_text(buffer, length))
        return false;

    // MUST be without whitespaces
    uint64_t i = 0;

    for (i = 0; i < length; i++) {

        if (isspace(buffer[i]))
            return false;
    }

    return true;
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_is_key(const char *buffer, uint64_t length) {

    if (!buffer || length < 5)
        return false;

    if (strncmp("prompt", (char *)buffer, length) == 0)
        return true;

    switch (buffer[0]) {

    case 'c':
        return ov_sdp_check_clear(buffer, length);
        break;

    case 'b':
        return ov_sdp_check_base64(buffer, length);
        break;

    case 'u':
        return ov_sdp_check_uri(buffer, length);
        break;
    }

    return false;
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_is_base64(const char *buffer, uint64_t length) {

    if (!buffer || length < 1)
        return false;

    uint8_t *ptr = NULL;
    size_t size = 0;

    if (ov_base64_decode((uint8_t *)buffer, length, &ptr, &size)) {
        free(ptr);
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_check_base64(const char *buffer, uint64_t length) {

    if (!buffer || length < 8)
        return false;

    if (strncmp(buffer, "base64:", 7) != 0)
        return false;

    return ov_sdp_is_base64(buffer + 7, length - 7);
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_check_clear(const char *buffer, uint64_t length) {

    if (!buffer || length < 7)
        return false;

    if (strncmp(buffer, "clear:", 6) != 0)
        return false;

    return ov_sdp_is_text(buffer + 6, length - 6);
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_check_uri(const char *buffer, uint64_t length) {

    if (!buffer || length < 5)
        return false;

    if (strncmp(buffer, "uri:", 4) != 0)
        return false;

    return ov_sdp_is_uri(buffer + 4, length - 4);
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_is_uri(const char *buffer, uint64_t length) {

    return ov_uri_string_is_valid(buffer, length);
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_is_digit(const char *buffer, uint64_t length) {

    if (!buffer || length < 1)
        return false;

    uint64_t i = 0;

    for (i = 0; i < length; i++) {

        if (buffer[i] > '9' || buffer[i] < '0')
            return false;
    }

    return true;
}

/*---------------------------------------------------------------------------*/

bool ov_sdp_is_integer(const char *buffer, uint64_t length) {

    if (!buffer || length < 1)
        return false;

    if ('0' == buffer[0])
        return false;

    return ov_sdp_is_digit(buffer, length);
}

/*---------------------------------------------------------------------------*/

bool ov_sdp_is_port(const char *buffer, uint64_t length) {

    if (!ov_sdp_is_integer(buffer, length))
        return false;

    char *ptr = NULL;
    int64_t number = strtoll(buffer, &ptr, 10);

    if ((number < 0) || (number > 65535))
        return false;

    return true;
}

/*---------------------------------------------------------------------------*/

bool ov_sdp_is_typed_time(const char *buffer, uint64_t length, bool negative) {

    if (!buffer || length < 1)
        return false;

    switch (buffer[length - 1]) {

    case 'd':
    case 'h':
    case 'm':
    case 's':
        length--;
        break;
    }

    if (length == 1)
        if (buffer[0] == '0')
            return true;

    if (negative)
        if (buffer[0] == '-')
            return ov_sdp_is_integer(buffer + 1, length - 1);

    return ov_sdp_is_integer(buffer, length);
}

/*---------------------------------------------------------------------------*/

bool ov_sdp_is_time(const char *buffer, uint64_t length) {

    if (!buffer || length < 1)
        return false;

    if (length == 1)
        if (buffer[0] == '0')
            return true;

    if (length < 10)
        return false;

    return ov_sdp_is_integer(buffer, length);
}

/*---------------------------------------------------------------------------*/

bool ov_sdp_is_token(const char *buffer, uint64_t length) {

    if (!buffer || length < 1)
        return false;

    uint64_t i = 0;

    for (i = 0; i < length; i++) {

        if (buffer[i] >= 0x5E) {
            if (buffer[i] <= 0x7E)
                continue;
            return false;
        }

        if (buffer[i] >= 0x41) {
            if (buffer[i] <= 0x5A)
                continue;
            return false;
        }

        if (buffer[i] >= 0x30) {
            if (buffer[i] <= 0x39)
                continue;
            return false;
        }

        if (buffer[i] >= 0x2D) {
            if (buffer[i] <= 0x2E)
                continue;
            return false;
        }

        if (buffer[i] >= 0x2A) {
            if (buffer[i] <= 0x2B)
                continue;
            return false;
        }

        if (buffer[i] >= 0x23) {
            if (buffer[i] <= 0x27)
                continue;
            return false;
        }

        if (buffer[i] == 0x21)
            continue;

        return false;
    }

    return true;
}

/*---------------------------------------------------------------------------*/

bool ov_sdp_is_address(const char *buffer, uint64_t length) {

    if (!buffer || length < 1)
        return false;

    /*
     * Weeeeeell, RFC 4566 specifies more or less anything except chars
     * 0x00 through 0x20 and 0x7f within a string of at least 1 char length
     * to be a potentially valid network address.
     *
     * Exerpt:
     *
     * ; sub-rules of 'c='
     *    connection-address =  multicast-address / unicast-address
     *
     * ; generic sub-rules: addressing
     *    unicast-address =     IP4-address / IP6-address / FQDN / extn-addr
     *
     * ; Generic for other address families
     *    extn-addr =           non-ws-string
     *
     *    non-ws-string =       1*(VCHAR/%x80-FF)
     *                          ;string of visible characters
     *
     * And finally from RFC 4234
     *
     *    VCHAR          =  %x21-7E
     *                      ; visible (printing) characters
     */

    for (size_t i = 0; i < length; i++) {

        if (0x7f <= buffer[i])
            return false;

        if (0x21 > buffer[i])
            return false;
    }

    return true;
}

/*---------------------------------------------------------------------------*/

bool ov_sdp_is_proto(const char *buffer, uint64_t length) {

    if (!buffer || length < 1)
        return false;

    char *token = NULL;
    uint64_t size = 0;
    size_t items = 0;

    ov_list *tokens = ov_string_pointer((char *)buffer, length, "/", 1);
    if (!tokens)
        return false;

    items = tokens->count(tokens);

    for (uint64_t i = 1; i <= items; i++) {

        token = tokens->get(tokens, i);

        if (i == items) {

            size = length - (tokens->get(tokens, i) - tokens->get(tokens, 1));

        } else {

            size = tokens->get(tokens, i + 1) - tokens->get(tokens, i) - 1;
        }

        if (!ov_sdp_is_token(token, size))
            goto error;
    }

    tokens->free(tokens);
    return true;
error:
    if (tokens)
        tokens->free(tokens);
    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_sdp_is_byte_string(const char *buffer, uint64_t length) {

    if (!buffer || length < 1)
        return false;

    for (size_t i = 0; i < length; i++) {

        switch (buffer[i]) {

        case 0x00:
        case '\r':
        case '\n':
            return false;
        default:
            break;
        }
    }

    return true;
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_verify_phone_buffer(const char *buffer, uint64_t length) {

    if (!buffer || length < 1)
        return false;

    /*
            phone = ["+"] DIGIT 1*(SP / "-" / DIGIT)
    */

    char *ptr = (char *)buffer;

    // offset optional +
    if (ptr[0] == '+') {
        ptr++;
        length--;
    }

    if (length < 2)
        return false;

    // MUST be a digit
    if (!isdigit(ptr[0]))
        return false;

    ptr++;
    length--;

    while (length > 0) {

        switch (*ptr) {

        case 0x20: // SP
        case 0x2D: // -
        case 0x30: // 0
        case 0x31: // 1
        case 0x32: // 2
        case 0x33: // 3
        case 0x34: // 4
        case 0x35: // 5
        case 0x36: // 6
        case 0x37: // 7
        case 0x38: // 8
        case 0x39: // 9
            ptr++;
            length--;
            break;
        default:
            return false;
        }
    }

    return true;
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_is_phone(const char *buffer, uint64_t length) {

    if (!buffer || length < 1)
        return false;

    /*
            ; sub-rules of 'p='
            phone-number =        phone *SP "(" 1*email-safe ")" /
                                  1*email-safe "<" phone ">" /
                                  phone

            phone =               ["+"] DIGIT 1*(SP / "-" / DIGIT)

    */

    if (!ov_sdp_is_text(buffer, length))
        return false;

    char *ptr = NULL;
    char *in = memchr(buffer, '(', length);

    uint64_t len = 0;

    if (in) {

        if (buffer[length - 1] != ')')
            return false;

        len = in - buffer;
        ptr = (char *)buffer;

        if (!ov_sdp_verify_phone_buffer(ptr, len))
            return false;

        ptr = in + 1;
        len = length - len - 2;
        if (!ov_sdp_is_email_safe(ptr, len))
            return false;

        return true;
    }

    in = memchr(buffer, '<', length);

    if (in) {

        if (buffer[length - 1] != '>')
            return false;

        len = in - buffer;
        ptr = (char *)buffer;

        if (!ov_sdp_is_email_safe(ptr, len))
            return false;

        ptr = in + 1;
        len = length - len - 2;
        if (!ov_sdp_verify_phone_buffer(ptr, len))
            return false;

        return true;
    }

    if (ov_sdp_verify_phone_buffer(buffer, length))
        return true;

    return false;
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_is_addr_spec(const char *buffer, uint64_t length) {

    if (!buffer || length < 1)
        return false;

    /* TBD FULL RFC 5322 implementation check required.

    addr-spec       =       local-part "@" domain

    local-part      =       dot-atom / quoted-string / obs-local-part

    domain          =       dot-atom / domain-literal / obs-domain

    domain-literal  =       [CFWS] "[" *([FWS] dcontent) [FWS] "]" [CFWS]

    dcontent        =       dtext / quoted-pair

    dtext           =       NO-WS-CTL /     ; Non white space controls

                            %d33-90 /       ; The rest of the US-ASCII
                            %d94-126        ;  characters not including "[",
                                            ;  "]", or "\"

    @NOTE short check for x@y implemented here.
    */

    char *ptr = memchr(buffer, '@', length);
    if (!ptr)
        return false;

    if (ptr == buffer)
        return false;

    if ((uint64_t)(ptr - buffer) == length - 1)
        return false;

    return true;
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_is_email(const char *buffer, uint64_t length) {

    /*

    email-address   =       address-and-comment /
                            dispname-and-address /
                            addr-spec

    address-and-comment  =  addr-spec 1*SP "(" 1*email-safe ")"
    dispname-and-address =  1*email-safe 1*SP "<" addr-spec ">"


    */

    if (!ov_sdp_is_text(buffer, length))
        return false;

    char *ptr = NULL;
    char *in = memchr(buffer, '(', length);

    uint64_t len = 0;

    if (in) {

        if (buffer[length - 1] != ')')
            return false;

        if ((in - buffer) < 2)
            return false;

        if (in[-1] != 0x20)
            return false;

        len = in - buffer - 1;
        ptr = (char *)buffer;

        if (!ov_sdp_is_addr_spec(ptr, length))
            return false;

        ptr = in + 1;
        len = length - len - 3;

        if (!ov_sdp_is_email_safe(ptr, len))
            return false;

        return true;
    }

    in = memchr(buffer, '<', length);

    if (in) {

        if (buffer[length - 1] != '>')
            return false;

        if ((in - buffer) < 2)
            return false;

        if (in[-1] != 0x20)
            return false;

        len = in - buffer;
        ptr = (char *)buffer;

        if (!ov_sdp_is_email_safe(ptr, len))
            return false;

        ptr = in + 1;
        len = length - len - 3;
        if (!ov_sdp_is_addr_spec(ptr, len))
            return false;

        return true;
    }

    if (ov_sdp_is_addr_spec(buffer, length))
        return true;

    return false;
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_is_email_safe(const char *buffer, uint64_t length) {

    if (!buffer || length < 1)
        return false;

    /*
            email-safe =
                    %x01-09/%x0B-0C/%x0E-27/%x2A-3B/%x3D/%x3F-FF
                    ;any byte except NUL, CR, LF, or the quoting
                    ;characters ()<>
    */

    for (size_t i = 0; i < length; i++) {

        switch (buffer[i]) {

        case 0x00:
        case 0x0A:
        case 0x0D:
        case 0x28:
        case 0x29:
        case 0x3C:
        case 0x3E:
            return false;
        default:
            break;
        }
    }

    return true;
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_is_ip6(const char *buffer, uint64_t length) {

    if (!buffer || length < 2)
        return false;

    struct sockaddr_storage sa;
    char buf[length + 1];

    memset(buf, 0, length + 1);

    // ensure /0 terminated string
    if (!memcpy(buf, buffer, length))
        return false;

    if (1 == inet_pton(AF_INET6, buf, &sa))
        return true;

    return false;
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_is_ip4(const char *buffer, uint64_t length) {

    if (!buffer || length < 1)
        return false;

    struct sockaddr_storage sa;
    char buf[length + 1];

    memset(buf, 0, length + 1);

    // ensure /0 terminated string
    if (!memcpy(buf, buffer, length))
        return false;

    if (1 == inet_pton(AF_INET, buf, &sa))
        return true;

    return false;
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_is_ip6_multicast(const char *buffer, uint64_t length) {

    if (!buffer || length < 2)
        return false;

    // IP6-multicast = hexpart [ "/" integer ]

    if (tolower(buffer[0]) != 'f')
        return false;

    if (tolower(buffer[1]) != 'f')
        return false;

    size_t len = 0;
    char *slash = memchr(buffer, '/', length);

    if (!slash)
        return ov_sdp_is_ip6(buffer, length);

    len = slash - buffer;
    if (!ov_sdp_is_ip6(buffer, len))
        return false;

    len = length - len - 1;
    return ov_sdp_is_port(slash + 1, len);
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_is_ip4_multicast(const char *buffer, uint64_t length) {

    if (!buffer || length < 7)
        return false;

    if (buffer[0] != '2')
        return false;

    char *slash = NULL;
    char *ttl = NULL;
    size_t ttl_len = 0;
    size_t ip_len = 0;

    switch (buffer[1]) {

    case '2':

        if (buffer[2] < 0x34)
            return false;

        if (buffer[2] > 0x39)
            return false;

        break;

    case '3':

        if (buffer[2] < 0x30)
            return false;

        if (buffer[2] > 0x39)
            return false;

        break;
    default:
        return false;
    }

    slash = memchr(buffer, '/', length);
    if (!slash)
        return false;

    ip_len = slash - buffer;

    ttl = slash + 1;
    slash = memchr(ttl, '/', length - ip_len - 1);

    if (slash) {

        ttl_len = slash - ttl;

    } else {

        ttl_len = length - (ttl - buffer);
    }

    if (ttl_len == 1) {

        if (ttl[0] < 0x30)
            return false;

        if (ttl[0] > 0x39)
            return false;

    } else if (ttl_len > 3) {

        return false;

    } else if (!ov_sdp_is_integer(ttl, ttl_len)) {

        return false;
    }

    // TTL is int of max length 3

    if (!ov_sdp_is_ip4(buffer, ip_len))
        return false;

    if (!slash)
        return true;

    return ov_sdp_is_port(slash + 1, length - ip_len - 1 - ttl_len - 1);
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_is_fqdn(const char *buffer, uint64_t length) {

    if (!buffer || length < 4)
        return false;

    for (uint64_t i = 0; i < length; i++) {

        if (!isalnum(buffer[i]))
            if (buffer[i] != '-')
                if (buffer[i] != '.')
                    return false;
    }

    return true;
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_is_extn_addr(const char *buffer, uint64_t length) {

    if (!buffer || length < 1)
        return false;

    return ov_sdp_is_address(buffer, length);
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_is_multicast_address(const char *buffer, uint64_t length) {

    if (!buffer || length < 1)
        return false;

    /*

    multicast-address =     IP4-multicast / IP6-multicast / FQDN
                            / extn-addr

    IP4-multicast =         m1 3( "." decimal-uchar )
                            "/" ttl [ "/" integer ]
                            ; IPv4 multicast addresses may be in the
                            ; range 224.0.0.0 to 239.255.255.255

    m1 =                    ("22" ("4"/"5"/"6"/"7"/"8"/"9")) /
                            ("23" DIGIT )

    IP6-multicast =         hexpart [ "/" integer ]
                            ; IPv6 address starting with FF

    ttl =                   (POS-DIGIT *2DIGIT) / "0"

    FQDN =                  4*(alpha-numeric / "-" / ".")
                            ; fully qualified domain name as specified
                            ; in RFC 1035 (and updates)
    */
    /*
            if (ov_sdp_is_ip6_multicast(buffer, length))
                    return true;

            if (ov_sdp_is_ip4_multicast(buffer, length))
                    return true;

            if (ov_sdp_is_fqdn(buffer, length))
                    return true;
    */
    if (ov_sdp_is_extn_addr(buffer, length))
        return true;

    return false;
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_is_unicast_address(const char *buffer, uint64_t length) {

    if (!buffer || length < 1)
        return false;
    /*
            if (ov_sdp_is_ip6(buffer, length))
                    return true;

            if (ov_sdp_is_ip4(buffer, length))
                    return true;

            if (ov_sdp_is_fqdn(buffer, length))
                    return true;
    */
    if (ov_sdp_is_extn_addr(buffer, length))
        return true;

    return false;
}

/*-------------------------------------------------------------------------*/

bool ov_sdp_is_bandwidth(const char *buffer, uint64_t length) {

    if (!buffer || length < 3)
        goto error;

    char *colon = NULL;
    char *ptr = NULL;

    colon = memchr(buffer, ':', length);
    if (!colon)
        goto error;

    if (!ov_sdp_is_token(buffer, (colon - buffer)))
        goto error;

    ptr = colon + 1;
    if (!ov_sdp_is_digit(ptr, length - (ptr - buffer)))
        goto error;

    return true;
error:
    return false;
}
