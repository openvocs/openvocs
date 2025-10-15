/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_callback.h
        @author         Markus Töpfer

        @date           2023-03-29


        ------------------------------------------------------------------------
*/
#ifndef ov_callback_h
#define ov_callback_h

#include <ov_base/ov_event_loop.h>

#define OV_CALLBACK_TIMEOUT_DEFAULT_USEC 1000000
#define OV_CALLBACK_TIMEOUT_UNLIMITED UINT64_MAX
#define OV_CALLBACK_DEFAULT_CACHE_SIZE 25

/*----------------------------------------------------------------------------*/

typedef struct ov_callback {

    void *userdata;
    void *function;
    int socket;

} ov_callback;

/*----------------------------------------------------------------------------*/

typedef struct ov_callback_registry ov_callback_registry;

/*----------------------------------------------------------------------------*/

typedef struct ov_callback_registry_config {

    ov_event_loop *loop;
    uint64_t timeout_usec;
    size_t cache_size;

} ov_callback_registry_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_callback_registry *ov_callback_registry_create(
    ov_callback_registry_config config);
ov_callback_registry *ov_callback_registry_free(ov_callback_registry *self);
ov_callback_registry *ov_callback_registry_cast(const void *data);

/*----------------------------------------------------------------------------*/

bool ov_callback_registry_register(ov_callback_registry *self,
                                   const char *key,
                                   const ov_callback callback,
                                   uint64_t timeout);

/*----------------------------------------------------------------------------*/

ov_callback ov_callback_registry_unregister(ov_callback_registry *self,
                                            const char *key);

/*----------------------------------------------------------------------------*/

ov_callback ov_callback_registry_get(ov_callback_registry *self,
                                     const char *key);

#endif /* ov_callback_h */
