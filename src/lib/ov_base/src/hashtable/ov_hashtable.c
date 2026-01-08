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
        @file           ov_hashtable.c
        @author         Michael Beer

        @date           2019-02-20

        @ingroup        ov_hashtable

        @brief          Implementation of


        ------------------------------------------------------------------------
*/
#include "../../include/ov_hashtable.h"
#include "../../include/ov_utils.h"

/*---------------------------------------------------------------------------*/

static const uint8_t TYPE_ID_CHARS[] = {'h', 'A', 's', 1};

#define TYPE_ID (*(uint32_t *)&TYPE_ID_CHARS[0])

/******************************************************************************
 *
 *  INTERNAL HASHTABLE STRUCTURE
 *
 ******************************************************************************/

struct table_entry {

    void *key;
    void *value;

    struct table_entry *next;
};

/*---------------------------------------------------------------------------*/

struct ov_hashtable_struct {

    /* 'Magic number to prevent casting errors */
    uint32_t type;

    ov_hashtable_funcs funcs;

    unsigned number_of_buckets;
    struct table_entry *entries;
};

/******************************************************************************
 *
 *  INTERNAL FUNCTIONS
 *
 ******************************************************************************/

static struct table_entry *get_entry_for(const ov_hashtable *table,
                                         const void *key);

static bool hashtable_clear(ov_hashtable *table);

static void key_free_func_default(void *key);
static void *key_copy_func_default(const void *key);
static int key_cmp_func_default(const void *key1, const void *key2);
static uint64_t hash_func_default(const void *key);

/******************************************************************************
 *
 *  PUBLIC FUNCTIONS
 *
 ******************************************************************************/

ov_hashtable *ov_hashtable_create(size_t num_buckets,
                                  ov_hashtable_funcs funcs) {

    if (0 == num_buckets)
        goto error;

    ov_hashtable *table = calloc(1, sizeof(ov_hashtable));

    table->type = TYPE_ID;

    table->number_of_buckets = num_buckets;
    table->entries = calloc(num_buckets, sizeof(struct table_entry));

    if (!funcs.key_free)
        funcs.key_free = key_free_func_default;
    if (!funcs.key_copy)
        funcs.key_copy = key_copy_func_default;
    if (!funcs.key_cmp)
        funcs.key_cmp = key_cmp_func_default;
    if (!funcs.hash)
        funcs.hash = hash_func_default;

    table->funcs = funcs;

    return table;

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

bool ov_hashtable_contains(const ov_hashtable *table, const void *key) {

    struct table_entry *entry = get_entry_for(table, key);

    if (0 == entry) {
        return false;
    }

    return 0 != entry->next;
}

/*---------------------------------------------------------------------------*/

void *ov_hashtable_get(const ov_hashtable *table, const void *key) {

    struct table_entry *entry = get_entry_for(table, key);

    if (0 == entry)
        goto error;
    if (0 == entry->next)
        goto error;

    return entry->next->value;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

void *ov_hashtable_set(ov_hashtable *table, const void *key, void *value) {

    struct table_entry *entry = get_entry_for(table, key);

    if (0 == entry)
        goto error;

    /* entry always is a valid pointer with entry->next being the entry to
     * use.
     * Its either 0 or already used ( depending whether key was set before
     */

    void *old_value = 0;

    if (0 == entry->next) {

        /* TODO: CACHE ? */
        entry->next = calloc(1, sizeof(struct table_entry));
        entry = entry->next;

        entry->key = table->funcs.key_copy(key);

    } else {

        entry = entry->next;
    }

    old_value = entry->value;
    entry->value = value;

    return old_value;

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

void *ov_hashtable_remove(ov_hashtable *table, const void *key) {

    struct table_entry *entry = get_entry_for(table, key);

    if (0 == entry)
        goto error;
    if (0 == entry->next)
        goto error;

    struct table_entry *to_del = entry->next;

    table->funcs.key_free(to_del->key);
    void *original_value = to_del->value;
    entry->next = to_del->next;

    /* TODO: Cache? */
    free(to_del);

    return original_value;

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

size_t ov_hashtable_for_each(const ov_hashtable *table,
                             bool (*process_func)(void const *key,
                                                  void const *value, void *arg),
                             void *arg) {

    if (0 == table)
        goto error;

    OV_ASSERT(TYPE_ID == table->type);

    if (0 == table->entries)
        goto error;
    if (0 == table->number_of_buckets)
        goto error;
    if (0 == process_func)
        goto error;

    size_t count = 0;

    struct table_entry *entry = 0;

    for (size_t i = 0; i < table->number_of_buckets; ++i) {

        entry = &table->entries[i];

        if (0 == entry->next)
            continue;

        entry = entry->next;

        while (0 != entry) {

            ++count;

            if (!process_func(entry->key, entry->value, arg))
                goto finish;

            entry = entry->next;
        }
    }

finish:

    return count;

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

ov_hashtable *ov_hashtable_free(ov_hashtable *table) {

    if (!table)
        goto error;

    OV_ASSERT(TYPE_ID == table->type);

    if (!hashtable_clear(table))
        goto error;

    if (!table->entries)
        goto error;

    free(table->entries);
    free(table);

    return 0;

error:

    return table;
}

/******************************************************************************
 *                             INTERNAL FUNCTIONS
 ******************************************************************************/

static struct table_entry *get_entry_for(const ov_hashtable *table,
                                         const void *key) {

    if (0 == table)
        goto error;

    OV_ASSERT(TYPE_ID == table->type);

    OV_ASSERT(table->funcs.hash);
    OV_ASSERT(table->funcs.key_cmp);

    if (0 == table->entries)
        goto error;
    if (0 == key)
        goto error;

    unsigned hash = table->funcs.hash(key) % table->number_of_buckets;

    int (*compare)(const void *, const void *) = table->funcs.key_cmp;

    struct table_entry *entry = &table->entries[hash];
    struct table_entry *c = entry;

    while (entry->next != 0) {

        c = entry;

        entry = entry->next;

        if (0 == compare(entry->key, key))
            return c;
    };

    return entry;

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

static bool hashtable_clear(ov_hashtable *table) {

    if (!table)
        goto error;

    OV_ASSERT(TYPE_ID == table->type);

    OV_ASSERT(table->funcs.key_free);

    struct table_entry *entry = 0;

    for (size_t i = 0; i < table->number_of_buckets; ++i) {

        entry = &table->entries[i];

        if (0 == entry->next)
            continue;

        entry = entry->next;

        while (0 != entry) {

            struct table_entry *current = entry;
            entry = entry->next;

            table->funcs.key_free(current->key);
            free(current);
        }

        table->entries[i].next = 0;
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

void ov_hashtable_enable_caching(size_t capacity) {
    UNUSED(capacity);
    TODO("IMPLEMENT CACHING");
}

/******************************************************************************
 *                              DEFAULT HANDLERS
 ******************************************************************************/

static void key_free_func_default(void *key) { UNUSED(key); }

/*---------------------------------------------------------------------------*/

static void *key_copy_func_default(const void *key) {

    /* Default: We 'copy' only the pointer itself */
    return (void *)key;
}

/*---------------------------------------------------------------------------*/

static int key_cmp_func_default(const void *key1, const void *key2) {

    return key1 - key2;
}

/*---------------------------------------------------------------------------*/

static uint64_t hash_func_default(const void *key) {

    uintptr_t key_ptr = (uintptr_t)key;
    /* Really simple hash function ;) */
    unsigned int key_int = (unsigned int)key_ptr;
    return (uint64_t)key_int;
}

/******************************************************************************
 *              example: create HASH TABLE for C-STRINGS as keys
 ******************************************************************************/

/* BEWARE:
 * Direct casts of int strcmp(const char *...) to int(*)(void *, ...) are
 * allowe, but actually calling the cast function pointer yields
 * UNDEFINED behaviour!
 *
 * Moreover, strcpy, strdup etc are not 0-pointer-safe
 */

static int string_compare(void const *s1, void const *s2) {

    if (s1 == s2)
        return 0;

    if (0 == s1)
        return -1;
    if (0 == s2)
        return 1;

    return strcmp(s1, s2);
}

/*----------------------------------------------------------------------------*/

static void *string_copy(void const *s) {

    if (0 == s)
        return 0;

    return strdup(s);
}

/*----------------------------------------------------------------------------*/

ov_hashtable *ov_hashtable_create_c_string(uint8_t num_buckets) {

    /* since the hash function only allows for 256 distinct values,
     * more than 256 buckets dont make sens ... */

    return ov_hashtable_create(
        num_buckets, (ov_hashtable_funcs){.key_free = free,
                                          .key_copy = string_copy,
                                          .key_cmp = string_compare,
                                          .hash = ov_hash_simple_c_string});
}
/*---------------------------------------------------------------------------*/
