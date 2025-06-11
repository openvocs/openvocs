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
        @file           ov_ice_core_dtls_filter.h
        @author         Markus Töpfer

        @date           2024-09-11


        ------------------------------------------------------------------------
*/
#ifndef ov_ice_dtls_filter_h
#define ov_ice_dtls_filter_h

#include "ov_ice_pair.h"
#include <stdbool.h>

#include <openssl/bio.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_ice_dtls_filter_init();
void ov_ice_dtls_filter_deinit();

/**
        Create a write bio for ov_ice_dtls_connection.
*/
BIO *ov_ice_dtls_filter_pair_bio_create(ov_ice_pair *pair);

#endif /* ov_ice_core_dtls_filter_h */
