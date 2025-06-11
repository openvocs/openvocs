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

        @date           2019-02-20

        This is a COPY of ov_base/log_hashtable

        We should effectively split ov_base into the real base part, containing
        only stuff that does not log like data structures etc.

        And the rest.

        Then we can facilitate the 'real base' in here.

        ------------------------------------------------------------------------
*/
#include "log_hashtable.h"
#include "log_assert.h"
#include <string.h>

/*---------------------------------------------------------------------------*/

static const uint8_t TYPE_ID_CHARS[] = {'h', 'A', 's', 1};

#define TYPE_ID (*(uint32_t *)&TYPE_ID_CHARS[0])

typedef struct {

    void (*key_free)(void *key);
    void *(*key_copy)(const void *key);
    int (*key_cmp)(const void *key, const void *value);
    uint64_t (*hash)(const void *);

} log_hashtable_funcs;

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

struct log_hashtable_struct {

    /* 'Magic number to prevent casting errors */
    uint32_t type;

    log_hashtable_funcs funcs;

    unsigned number_of_buckets;
    struct table_entry *entries;
};

/******************************************************************************
 *
 *  INTERNAL FUNCTIONS
 *
 ******************************************************************************/

static struct table_entry *get_entry_for(const log_hashtable *table,
                                         const void *key);

static bool hashtable_clear(log_hashtable *table);

/*---------------------------------------------------------------------------*/

static int string_compare(void const *s1, void const *s2) {

    if (s1 == s2) return 0;

    if (0 == s1) return -1;
    if (0 == s2) return 1;

    return strcmp(s1, s2);
}

/*----------------------------------------------------------------------------*/

static void *string_copy(void const *s) {

    if (0 == s) return 0;

    return strdup(s);
}

/*----------------------------------------------------------------------------*/

static uint64_t hash_c_string(const void *c_string) {

    if (0 == c_string) return 0;

    const char *s = c_string;

    uint8_t hash = 0;

    uint8_t c = (uint8_t)*s;

    while (0 != c) {

        /* Use a prime as factor to prevent short cycles  -
         * if you dont understand, leave it as it is ... */
        hash += 13 * c;
        c = (uint8_t) * (++s);
    }

    return hash;
}

/******************************************************************************
 *
 *  PUBLIC FUNCTIONS
 *
 ******************************************************************************/

log_hashtable *log_hashtable_create(size_t num_buckets) {

    if (0 == num_buckets) {
        goto error;
    }

    log_hashtable *table = calloc(1, sizeof(log_hashtable));

    table->type = TYPE_ID;

    table->number_of_buckets = num_buckets;
    table->entries = calloc(num_buckets, sizeof(struct table_entry));

    table->funcs = (log_hashtable_funcs){.key_free = free,
                                         .key_copy = string_copy,
                                         .key_cmp = string_compare,
                                         .hash = hash_c_string};

    return table;

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

void *log_hashtable_get(const log_hashtable *table, const void *key) {

    struct table_entry *entry = get_entry_for(table, key);

    if (0 == entry) goto error;
    if (0 == entry->next) goto error;

    return entry->next->value;

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

void *log_hashtable_set(log_hashtable *table, const void *key, void *value) {

    struct table_entry *entry = get_entry_for(table, key);

    if (0 == entry) goto error;

    void *old_value = 0;

    if (0 == entry->next) {

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

log_hashtable *log_hashtable_free(log_hashtable *table) {

    if (!table) goto error;

    LOG_ASSERT(TYPE_ID == table->type);

    if (!hashtable_clear(table)) goto error;

    if (0 == table->entries) goto error;

    free(table->entries);
    free(table);

    return 0;

error:

    return table;
}

/******************************************************************************
 *                             INTERNAL FUNCTIONS
 ******************************************************************************/

static struct table_entry *get_entry_for(const log_hashtable *table,
                                         const void *key) {

    if (0 == table) goto error;

    LOG_ASSERT(TYPE_ID == table->type);

    LOG_ASSERT(table->funcs.hash);
    LOG_ASSERT(table->funcs.key_cmp);

    if (0 == table->entries) goto error;
    if (0 == key) goto error;

    unsigned hash = table->funcs.hash(key) % table->number_of_buckets;

    int (*compare)(const void *, const void *) = table->funcs.key_cmp;

    struct table_entry *entry = &table->entries[hash];
    struct table_entry *c = entry;

    while (entry->next != 0) {

        c = entry;

        entry = entry->next;

        if (0 == compare(entry->key, key)) return c;
    };

    return entry;

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

static bool hashtable_clear(log_hashtable *table) {

    if (!table) goto error;

    LOG_ASSERT(TYPE_ID == table->type);

    LOG_ASSERT(table->funcs.key_free);

    struct table_entry *entry = 0;

    for (size_t i = 0; i < table->number_of_buckets; ++i) {

        entry = &table->entries[i];

        if (0 == entry->next) continue;

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

/*---------------------------------------------------------------------------*/

size_t log_hashtable_for_each(const log_hashtable *table,
                              bool (*process_func)(void const *key,
                                                   void const *value,
                                                   void *arg),
                              void *arg) {

    if (0 == table) goto error;

    LOG_ASSERT(TYPE_ID == table->type);

    if (0 == table->entries) goto error;
    if (0 == table->number_of_buckets) goto error;
    if (0 == process_func) goto error;

    size_t count = 0;

    struct table_entry *entry = 0;

    for (size_t i = 0; i < table->number_of_buckets; ++i) {

        entry = &table->entries[i];

        if (0 == entry->next) continue;

        entry = entry->next;

        while (0 != entry) {

            ++count;

            if (!process_func(entry->key, entry->value, arg)) goto finish;

            entry = entry->next;
        }
    }

finish:

    return count;

error:

    return 0;
}
