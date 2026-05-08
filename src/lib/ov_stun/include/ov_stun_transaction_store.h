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
        @file           ov_stun_transaction_store.h
        @author         Markus

        @date           2024-04-24


        ------------------------------------------------------------------------
*/
#ifndef ov_stun_transaction_store_h
#define ov_stun_transaction_store_h

#include <ov_base/ov_event_loop.h>

#define OV_STUN_TRANSACTION_STORE_DEFAULT_INVALIDATION_USEC 150000000

/*---------------------------------------------------------------------------*/

typedef struct ov_stun_transaction_store ov_stun_transaction_store;

/*----------------------------------------------------------------------------*/

typedef struct ov_stun_transaction_store_config {

    ov_event_loop *loop;

    struct {

        uint64_t invalidation_usec;

    } timer;

} ov_stun_transaction_store_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_stun_transaction_store *
ov_stun_transaction_store_create(ov_stun_transaction_store_config config);

/*----------------------------------------------------------------------------*/

ov_stun_transaction_store *
ov_stun_transaction_store_free(ov_stun_transaction_store *self);

/*----------------------------------------------------------------------------*/

ov_stun_transaction_store *ov_stun_transaction_store_cast(const void *self);

/*
 *      ------------------------------------------------------------------------
 *
 *      TRANSACTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_transaction_store_create_transaction(
    ov_stun_transaction_store *self, uint8_t *ptr, void *data);

/*----------------------------------------------------------------------------*/

void *ov_stun_transaction_store_unset(ov_stun_transaction_store *self,
                                      const uint8_t *transaction_id);

#endif /* ov_stun_transaction_store_h */
