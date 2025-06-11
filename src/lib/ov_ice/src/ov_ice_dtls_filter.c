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
        @file           ov_ice_core_dtls_filter.c
        @author         Markus

        @date           2024-09-11


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_dtls_filter.h"

#include <ov_base/ov_utils.h>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

static BIO_METHOD *ov_ice_dtls_filter_methods = NULL;

/*----------------------------------------------------------------------------*/

static long dtls_bio_ctrl(BIO *bio, int cmd, long num, void *ptr) {

    UNUSED(bio);
    UNUSED(num);
    UNUSED(ptr);

    switch (cmd) {
        case BIO_CTRL_FLUSH:
            return 1;
        case BIO_CTRL_WPENDING:
        case BIO_CTRL_PENDING:
            return 0L;
        default:
            break;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/

static int dtls_bio_write(BIO *bio, const char *in, int size) {

    if (size <= 0) goto error;

    ov_ice_pair *pair = ov_ice_pair_cast(BIO_get_data(bio));

    return ov_ice_pair_send(pair, (const uint8_t *)in, size);
error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static int dtls_bio_create(BIO *bio) {

    BIO_set_init(bio, 1);
    BIO_set_data(bio, NULL);
    BIO_set_shutdown(bio, 0);
    return 1;
}

/*----------------------------------------------------------------------------*/

static int dtls_bio_free(BIO *bio) {

    if (bio == NULL) {
        return 0;
    }

    BIO_set_data(bio, NULL);
    return 1;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_dtls_filter_init() {

    ov_ice_dtls_filter_methods = BIO_meth_new(BIO_TYPE_BIO, "DTLS writer");
    if (!ov_ice_dtls_filter_methods) {
        goto error;
    }
    BIO_meth_set_write(ov_ice_dtls_filter_methods, dtls_bio_write);
    BIO_meth_set_ctrl(ov_ice_dtls_filter_methods, dtls_bio_ctrl);
    BIO_meth_set_create(ov_ice_dtls_filter_methods, dtls_bio_create);
    BIO_meth_set_destroy(ov_ice_dtls_filter_methods, dtls_bio_free);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

void ov_ice_dtls_filter_deinit() {

    if (ov_ice_dtls_filter_methods) {
        BIO_meth_free(ov_ice_dtls_filter_methods);
        ov_ice_dtls_filter_methods = NULL;
    }

    return;
}

/*----------------------------------------------------------------------------*/

BIO *ov_ice_dtls_filter_pair_bio_create(ov_ice_pair *pair) {

    BIO *bio = BIO_new(ov_ice_dtls_filter_methods);
    if (bio == NULL) {
        return NULL;
    }

    BIO_set_data(bio, pair);
    return bio;
}