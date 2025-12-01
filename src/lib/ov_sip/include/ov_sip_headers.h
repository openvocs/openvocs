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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_SIP_HEADERS_H
#define OV_SIP_HEADERS_H
/*----------------------------------------------------------------------------*/

#include <ov_base/ov_buffer.h>

/*----------------------------------------------------------------------------*/

#define OV_SIP_HEADER_CALL_ID "Call-ID"
#define OV_SIP_HEADER_FROM "From"
#define OV_SIP_HEADER_TO "To"
#define OV_SIP_HEADER_CONTACT "Contact"
#define OV_SIP_HEADER_VIA "Via"
#define OV_SIP_HEADER_MAX_FORWARS "Max-Forwards"
#define OV_SIP_HEADER_CSEQ "CSeq"

#define OV_SIP_HEADER_ALLOW "Allow"
#define OV_SIP_HEADER_ACCEPT "Accept"
#define OV_SIP_HEADER_EXPIRES "Expires"

#define OV_SIP_HEADER_AUTHORIZATION "Authorization"

#define OV_SIP_HEADER_WWW_AUTHENTICATE "WWW-Authenticate"

/*----------------------------------------------------------------------------*/

ov_buffer *ov_sip_headers_auth(char const *uri, char const *user,
                               char const *password, char const *realm,
                               char const *method, char const *nonce);

/*----------------------------------------------------------------------------*/

/**
 * Extract user/tag info from SIP To/From header info, e.g.
 *
 * <sip:anton@balgenhans.ir;transport=UDP>;tag=Allgaier
 *
 * -> user: `anton@balgenhans.ir     tag: `Allgaier`
 *
 * `user` and/or `tag` might be 0.
 *  Extracts user only if `user` is not 0.
 *  Extracts tag only if `tag is not 0.
 *
 *  `*user` and `*tag` might be 0 or point to an existing buffer.
 *  If non-0, the old buffer will be freed.
 *  `*user` and / or `*tag` will be pointed to the new buffer containing the
 *  extracted user/tag.
 *
 */
bool ov_sip_headers_get_user_and_tag(char const *str, ov_buffer **user,
                                     ov_buffer **tag);

/*----------------------------------------------------------------------------*/

ov_buffer *ov_sip_headers_contact(char const *contact_header);

/*----------------------------------------------------------------------------*/

void ov_sip_headers_enable_caching();

/*----------------------------------------------------------------------------*/
#endif
