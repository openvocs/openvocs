/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_event_async.h
        @author         Markus Töpfer

        @date           2021-06-16


        ------------------------------------------------------------------------
*/
#ifndef ov_event_async_h
#define ov_event_async_h

#include "ov_event_io.h"
#include <ov_base/ov_event_loop.h>

typedef struct ov_event_async_store ov_event_async_store;

typedef struct {

    ov_event_loop *loop;

    uint64_t threadlock_timeout_usec;
    uint64_t invalidate_check_interval_usec;

    size_t cache;

} ov_event_async_store_config;

/*----------------------------------------------------------------------------*/

typedef struct ov_event_async_data ov_event_async_data;

struct ov_event_async_data {

    /* connection socket of the EVENT in value */
    int socket;

    /* event message */
    ov_json_value *value;

    /* optional send parameter */
    ov_event_parameter params;

    /* optional timedout callback data */

    struct {

        void *userdata;
        void (*callback)(void *userdata, ov_event_async_data data);

    } timedout;

    /* optional callback data */

    struct {

        void *userdata;
        void (*callback)(void *userdata,
                         int socket,
                         const ov_event_parameter *params,
                         ov_json_value *input,
                         ov_json_value *result);

    } callback;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_event_async_store *ov_event_async_store_create(
    ov_event_async_store_config config);

ov_event_async_store *ov_event_async_store_free(ov_event_async_store *self);

/*  Helper function to clear some data struct. This function will delete
 *  the json pointer if present and empty the structure */
void ov_event_async_data_clear(ov_event_async_data *data);

/*
 *      ------------------------------------------------------------------------
 *
 *      SESSION FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
 *  Store some JSON object within some id for a maximum lifetime in the
 *  store.
 *
 *  @param store                instance of the store
 *  @param id                   id to use for storage
 *  @param data                 data to store
 *  @param max_lifetime_usec    maximum time to store the data
 *
 *  @returns true if the data is stored
 *
 *  NOTE transfer of the json value to the store, do NOT free a stored JSON
 *  pointer!!!
 */
bool ov_event_async_set(ov_event_async_store *store,
                        const char *id,
                        ov_event_async_data data,
                        uint64_t max_lifetime_usec);

/*---------------------------------------------------------------------------*/

/**
 *  Unset some id from the store.
 *
 *  The data will no longer be present within the store on success. The json
 *  value set to data.value is the pointer, which was stored using set and
 *  contains the original event message.
 *
 *  @param store                instance of the store
 *  @param id                   id used for storage
 *
 *  NOTE transfer of the json value back from the store, the json value returned
 *  MUST be freed by the caller of the function.
 */
ov_event_async_data ov_event_async_unset(ov_event_async_store *store,
                                         const char *id);

/*---------------------------------------------------------------------------*/

/**
 *  Drop all events of socket.
 *
 *  @param store            instance of the store
 *  @param socket           socket id to drop
 */
bool ov_event_async_drop(ov_event_async_store *store, int socket);

#endif /* ov_event_async_h */
