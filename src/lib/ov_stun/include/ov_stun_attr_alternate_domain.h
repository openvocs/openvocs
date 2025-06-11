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
        @file           ov_stun_attr_alternate_domain.h
        @author         Markus Töpfer

        @date           2022-01-22


        ------------------------------------------------------------------------
*/
#ifndef ov_stun_attr_alternate_domain_h
#define ov_stun_attr_alternate_domain_h

#define STUN_ATTR_ALTERNATE_DOMAIN 0x8003

#include "ov_stun_attribute.h"

/*      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Check is an attribute buffer may contain an alternate server

        @param buffer           start of the attribute buffer
        @param length           length of the attribute buffer (at least)
*/
bool ov_stun_attribute_frame_is_alternate_domain(const uint8_t *buffer,
                                                 size_t length);

#endif /* ov_stun_attr_alternate_domain_h */
