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
        @file           ov_ice_core_dtls_cookie.h
        @author         Markus Töpfer

        @date           2024-09-11


        ------------------------------------------------------------------------
*/
#ifndef ov_ice_dtls_cookie_h
#define ov_ice_dtls_cookie_h

#include <openssl/ssl.h>
#include <stdbool.h>

typedef struct ov_ice_dtls_cookie_store ov_ice_dtls_cookie_store;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_ice_dtls_cookie_store *ov_ice_dtls_cookie_store_create();

/*----------------------------------------------------------------------------*/

ov_ice_dtls_cookie_store *
ov_ice_dtls_cookie_store_free(ov_ice_dtls_cookie_store *self);

/*----------------------------------------------------------------------------*/

/**
 *  (Re) initialize the store with the amount (quantity) of cookies with a given
 *  quality (length).
 *
 *  This function clears all previous cookies and generates quantity new cookies
 *
 *  @self       instance pointer
 *  @quantity   amount of cookies to use
 *  @length     length of the cookies to be created.
 */
bool ov_ice_dtls_cookie_store_initialize(ov_ice_dtls_cookie_store *self,
                                         size_t quantity, size_t length);

/*----------------------------------------------------------------------------*/

/**
 *  Function to be used in @SSL_CTX_set_cookie_verify_cb
 */
int ov_ice_dtls_cookie_verify(SSL *ssl, const unsigned char *cookie,
                              unsigned int cookie_len);

/*----------------------------------------------------------------------------*/

/**
 *  Function to be used in @SSL_CTX_set_cookie_generate_cb
 */
int ov_ice_dtls_cookie_generate(SSL *ssl, unsigned char *cookie,
                                unsigned int *cookie_len);

#endif /* ov_ice_core_dtls_cookie_h */
