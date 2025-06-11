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
        @file           ov_hashtable.h
        @author         Michael Beer

        @date           2018-01-23

        @ingroup        ov_hashtable

        @brief          Definition of a HASH TABLE

        Provides a hash table of arbitrary, variable size.

        Basically, you can
         * ov_hashtable_set(table, key, value)
         * ov_hashtable_get(table, key)
         * ov_hashtable_remove(table, key)
         * iterate over all entries using ov_hashtable_for_each(table, func)

        E.g. to find out how many entries a table contains, just use
        ov_hashtable_for_each() like so:

         bool dummy_proc(const void* a, void* b) {
             return true;
         }

         ...

         ov_hashtable* table = ...;

         size_t number_of_entries = ov_hashtable_for_each(table, dummy_proc, 0);

        The hash table is unaware of the kind of data stored within it.

        This implies:

        A hash table does NOT PERFORM ANY KIND OF MEMORY MANAGEMENT for values.
        You can control mem management of the keys by the handlers you pass
        to ov_hashtable_create() however.

        Example to use this hash table with c strings:

             // Simple 'hash' function that just uses the first char as hash val
             size_t shash(const void* key) {
                 return ((const char*)key)[0];
             }

             // There is a wrapper for this: ov_hashtable_create_c_string
             ov_hashtable str_table = ov_hashtable_create(
                         NUMBER_OF_BUCKETS_I_WANT,
                         (ov_hashtable_funcs) {
                         .key_free = free,
                         .key_copy = (void* (*)(const void*)) strdup,
                         .key_cmp = (int (*)(const void*, const void*)) strcmp,
                         .hash    = ov_hash_simple_c_string
                         });

             // Use the hash table

             char* four = strdup("four");
             int* four_pointer = calloc(1, sizeof(int));
             *four_pointer = 5;

             ov_hashtable_set(str_table, "four", (void*) 4);
             assert(4 == (int) ov_hashtable_get(str_table, "four"));
             // because strcmp(four, "four) == 0 this works fine
             assert(4 == (int) ov_hashtable_get(str_table, four));

             // Value of four is copied into hash table
             ov_hashtable_set(str_table, four, four_pointer);
             free(four);
             // four_pointer is NOT copied - the pointer in the hash table
             // is the same as four_pointer
             assert(four_pointer == (int) ov_hashtable_get(str_table, "four"));

             str_hash = ov_hashtable_free(str_hash);
             // Internally, the strings have been copied and freed
             // automatically...
             free(five);

        If values need to be freed, you might use
        ov_hashtable_for_each() and free everything you iterate over
        before performing an or ov_hashtable_free() like so in the example
        above:

             bool free_value(void* arg, const void* key, void* value) {

                 UNUSED(arg);
                 UNUSED(key);

                 free(value);

                 return true;
             }

             ov_hashtable_for_each(str_table, free_value, 0);


        Of course, we would have to drop the line `free(five)` in the example then.

        Beware: Be aware, that if you free a value that is still contained within
        a hash table, ov_hashtable_get() will still return its pointer on request.
        The requester has no means to figure out out of the box whether the
        pointer was freed, thus it might be a good idea to only free values
        AFTER you removed them from the hash or nesure that the hash table is not
        used further after starting to free values contained within.


        ------------------------------------------------------------------------
*/
#ifndef ov_hashtable_h
#define ov_hashtable_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "ov_hash_functions.h"

struct ov_hashtable_struct;
typedef struct ov_hashtable_struct ov_hashtable;

typedef struct {

    void (*key_free)(void *key);
    void *(*key_copy)(const void *key);
    int (*key_cmp)(const void *key, const void *value);
    uint64_t (*hash)(const void *);

} ov_hashtable_funcs;

/**
        Creates a new hashtable.

        If you do not supply the functions below, the key pointer values will
        be used as keys directly.
        You are encouraged to provide ov_hahshtable_funcs appropriate for the
        type of keys you want to use - otherwise, the keys will be just their
        mem addresses, which MIGHT NOT BE what you want since e.g.
        "a" and strdup("a") are treated as different keys!

        @param funcs functions that enable the hash table to
        copy/free/hash various kinds of keys.

 */
ov_hashtable *ov_hashtable_create(size_t num_buckets, ov_hashtable_funcs funcs);

/*----------------------------------------------------------------------------*/

bool ov_hashtable_contains(const ov_hashtable *table, const void *key);

/*---------------------------------------------------------------------------*/

/**
        Returns the value stored in hashtable for a dedicated key.
        If key is not found, returns 0.
 */
void *ov_hashtable_get(const ov_hashtable *table, const void *key);

/*----------------------------------------------------------------------------*/

/**
        Stores a value for a dedicated key in hashtable.
        Copies key if required, does not copy value.
        Returns old value stored in hashtable or 0 if key had not been defined
        before.
 */
void *ov_hashtable_set(ov_hashtable *table, const void *key, void *value);

/*---------------------------------------------------------------------------*/

/**
        Removes an entry from hashtable.
        You will receive the original void pointer stored as key thus that
        you can free / reuse it ...
        @return void pointer that was stored as value in hashtable
 */
void *ov_hashtable_remove(ov_hashtable *table, const void *key);

/*---------------------------------------------------------------------------*/

/**
        Iterates over all entries in hash and calls process() on them.
        keys/values are precisely those stored in the hashtable.
        @return number of processed entries.
 */
size_t ov_hashtable_for_each(const ov_hashtable *table,
                             bool (*process)(void const *key,
                                             void const *value,
                                             void *arg),
                             void *arg);

/*---------------------------------------------------------------------------*/

/**
        Removes all entries from hashtable.
        Does not free the values.
        If you need to free values beforehand,
        use ov_hashtable_for_each() to free them.
 */
ov_hashtable *ov_hashtable_free(ov_hashtable *table);

/*----------------------------------------------------------------------------*/

void ov_hashtable_enable_caching(size_t capacity);

/******************************************************************************
 *              example: create HASH TABLE for C-STRINGS as keys
 ******************************************************************************/

ov_hashtable *ov_hashtable_create_c_string(uint8_t num_buckets);

/*---------------------------------------------------------------------------*/

#endif /* ov_hashtable_h */
