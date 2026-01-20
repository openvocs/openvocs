/***
        ------------------------------------------------------------------------

        Copyright (c) 2026 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_interconnect_dtls_filter.h
        @author         Töpfer, Markus

        @date           2026-01-15


        ------------------------------------------------------------------------
*/
#ifndef ov_interconnect_dtls_filter_h
#define ov_interconnect_dtls_filter_h

#include "ov_interconnect_session.h"
#include <openssl/bio.h>
#include <stdbool.h>

bool ov_interconnect_dtls_filter_init();
void ov_interconnect_dtls_filter_deinit();

/**
        Create a write bio for the session.
*/
BIO *ov_interconnect_dtls_bio_create(ov_interconnect_session *session);

#endif /* ov_interconnect_dtls_filter_h */
