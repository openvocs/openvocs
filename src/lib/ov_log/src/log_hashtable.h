/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @author         Michael Beer

        @date           2018-01-23

        Copy of ov_base/ov_log_hashtable

        This is a nasty hack.
        We will have to split ov_base into a 'real base' and the rest.
        The 'real base' should contain data structures like ov_log_hashtable etc.
        that do not need to log.

        Then ov_log_ng can be based upon that.

        ------------------------------------------------------------------------
*/
#ifndef log_hashtable_h
#define log_hashtable_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

/*----------------------------------------------------------------------------*/

#define UNUSED(x)                                                              \
  do {                                                                         \
    (void)(x);                                                                 \
  } while (0)

/*----------------------------------------------------------------------------*/

struct log_hashtable_struct;
typedef struct log_hashtable_struct log_hashtable;

log_hashtable *log_hashtable_create(size_t num_buckets);

/*---------------------------------------------------------------------------*/

void *log_hashtable_get(const log_hashtable *table, const void *key);

/*---------------------------------------------------------------------------*/

void *log_hashtable_set(log_hashtable *table, const void *key, void *value);

/*----------------------------------------------------------------------------*/

size_t log_hashtable_for_each(const log_hashtable *table,
                              bool (*process_func)(void const *key,
                                                   void const *value,
                                                   void *arg),
                              void *arg);

/*----------------------------------------------------------------------------*/

log_hashtable *log_hashtable_free(log_hashtable *table);

/*---------------------------------------------------------------------------*/

#endif /* log_hashtable_h */
