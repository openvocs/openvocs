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
        @file           ov_hashtable_test.c
        @author         Michael Beer

        @date           2019-02-20

        @ingroup        ov_hashtable

        @brief          Unit tests of


        ------------------------------------------------------------------------
*/
#include "ov_hashtable.c"
#include <ov_test/ov_test.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST HELPER
 *
 *      ------------------------------------------------------------------------
 */

bool count_value(void const *key, void const *value, void *additional_arg) {

    UNUSED(key);

    OV_ASSERT(additional_arg);

    size_t *value_counter = additional_arg;

    if (0 == value) return false;

    uint8_t const *index = value;

    ++value_counter[*index];

    return true;
}

/*---------------------------------------------------------------------------*/

bool dummy_processor(void const *key, void const *value, void *arg) {

    UNUSED(arg);
    UNUSED(key);
    UNUSED(value);

    return true;
}

/*---------------------------------------------------------------------------*/

bool free_value(void const *key, void const *value, void *additional_arg) {

    UNUSED(additional_arg);
    UNUSED(key);

    free((void *)value);

    return true;
}

/*---------------------------------------------------------------------------*/

uint64_t shash(const void *key) {

    OV_ASSERT(key);

    const char *key_char = (const char *)key;
    return (size_t)key_char[0];
}

/*---------------------------------------------------------------------------*/

void *dummy_copy(const void *key) { return (void *)key; }

/*---------------------------------------------------------------------------*/

int scmp(const void *key1, const void *key2) { return key1 - key2; }

/*----------------------------------------------------------------------------*/

int test_ov_hashtable_create() {

    testrun(0 == ov_hashtable_create(0, (ov_hashtable_funcs){0}));

    ov_hashtable *table =
        ov_hashtable_create(1,
                            (ov_hashtable_funcs){.key_free = free,
                                                 .key_copy = dummy_copy,
                                                 .key_cmp = scmp,
                                                 .hash = shash});

    testrun(table);
    testrun(free == table->funcs.key_free);
    testrun(dummy_copy == table->funcs.key_copy);
    testrun(table->funcs.key_copy);
    testrun(scmp == table->funcs.key_cmp);
    testrun(shash == table->funcs.hash);
    testrun(1 == table->number_of_buckets);
    testrun(table->entries);

    testrun(0 == ov_hashtable_free(table));

    table = ov_hashtable_create(1, (ov_hashtable_funcs){0});

    testrun(table);
    testrun(table->funcs.key_free);
    testrun(table->funcs.key_copy);
    testrun(table->funcs.key_cmp);
    testrun(table->funcs.hash);
    testrun(1 == table->number_of_buckets);
    testrun(table->entries);

    testrun(0 == ov_hashtable_free(table));

    table = ov_hashtable_create(17, (ov_hashtable_funcs){0});

    testrun(table);
    testrun(table->funcs.key_free);
    testrun(table->funcs.key_copy);
    testrun(table->funcs.key_cmp);
    testrun(table->funcs.hash);
    testrun(17 == table->number_of_buckets);
    testrun(table->entries);

    testrun(0 == ov_hashtable_free(table));

    table = ov_hashtable_create(107, (ov_hashtable_funcs){0});

    testrun(table);
    testrun(table->funcs.key_free);
    testrun(table->funcs.key_copy);
    testrun(table->funcs.key_cmp);
    testrun(table->funcs.hash);
    testrun(107 == table->number_of_buckets);
    testrun(table->entries);

    testrun(0 == ov_hashtable_free(table));

    table = ov_hashtable_create(
        1, (ov_hashtable_funcs){.key_cmp = scmp, .hash = shash});

    testrun(table);
    testrun(table->funcs.key_free);
    testrun(table->funcs.key_copy);
    testrun(scmp == table->funcs.key_cmp);
    testrun(shash == table->funcs.hash);
    testrun(1 == table->number_of_buckets);
    testrun(table->entries);

    testrun(0 == ov_hashtable_free(table));

    return testrun_log_success();
}

/*-----------------------------------------------------------------------------
  -                          check_get_entry_for()
  ----------------------------------------------------------------------------*/

int check_get_entry_for() {

    const size_t NUM_ELEMENTS = UINT8_MAX;

    int value[] = {17, 19, 21, 23, 29, 31, 37};

    const size_t value_len = sizeof(value) / sizeof(value[0]);

    char *key[] = {"abc", "bcd", "bcc"};

    testrun(shash(key[0]) != shash(key[1]));
    testrun(shash(key[0]) != shash(key[2]));
    testrun(shash(key[1]) == shash(key[2]));

    testrun(0 == get_entry_for(0, key[0]));
    testrun(0 == get_entry_for(0, key[1]));
    testrun(0 == get_entry_for(0, key[2]));

    ov_hashtable *table = ov_hashtable_create(
        NUM_ELEMENTS, (ov_hashtable_funcs){.key_cmp = scmp, .hash = shash});

    testrun(0 == get_entry_for(0, key[0]));
    testrun(0 == get_entry_for(0, key[1]));
    testrun(0 == get_entry_for(0, key[2]));

    struct table_entry *entry = get_entry_for(table, key[0]);

    testrun(entry && (0 == entry->next));

    entry->next = calloc(1, sizeof(struct table_entry));

    entry->next->key = key[0];

    entry->next->value = &value[0];

    for (size_t i = 1; i < value_len; ++i) {

        entry = get_entry_for(table, key[0]);

        /* Since key has been set before, the entry should be occupied
         */
        testrun(entry && (0 != entry->next));
        testrun(0 == strcmp(entry->next->key, key[0]));
        testrun(entry->next->value == &value[i - 1]);

        entry->next->value = &value[i];
    }

    /* Set different key - a new entry should be retrieved */
    entry = get_entry_for(table, key[1]);

    testrun(entry && (0 == entry->next));

    entry->next = calloc(1, sizeof(struct table_entry));

    entry->next->key = key[1];
    entry->next->value = &value[0];

    for (size_t i = 1; i < value_len; ++i) {

        entry = get_entry_for(table, key[1]);

        /* Since key has been set before, the entry should be occupied
         */
        testrun(entry && (0 != entry->next));
        testrun(0 == strcmp(entry->next->key, key[1]));
        testrun(entry->next->value == &value[i - 1]);

        entry->next->value = &value[i];
    }

    /* Set a different key with same hash value as previous key -
     * different entry should be used, but with its predecessor being the
     * last entry */
    testrun(shash(key[1]) == shash(key[2]));

    struct table_entry *another = get_entry_for(table, key[2]);

    testrun(another && (0 == another->next));

    testrun(0 == strcmp(another->key, key[1]));

    another->next = calloc(1, sizeof(struct table_entry));

    another->next->key = key[2];
    another->next->value = &value[0];

    for (size_t i = 1; i < value_len; ++i) {

        another = get_entry_for(table, key[2]);

        /* Since key has been set before, the another should be occupied
         */
        testrun(another && (0 != another->next));

        testrun(0 == strcmp(another->key, key[1]));

        testrun(0 == strcmp(another->next->key, key[2]));
        testrun(another->next->value == &value[i - 1]);

        another->next->value = &value[i];
    }

    testrun(0 == ov_hashtable_free(table));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

static int test_ov_hashtable_contains() {

    testrun(!ov_hashtable_contains(0, 0));

    char const *keys[] = {"abba", "babba", "cabba"};

    size_t num_keys = sizeof(keys) / sizeof(keys[0]);

    for (size_t i = 0; i < num_keys; ++i) {
        testrun(!ov_hashtable_contains(0, keys[i]));
    }

    ov_hashtable *table = ov_hashtable_create(
        1,
        (ov_hashtable_funcs){
            .key_cmp = (int (*)(const void *, const void *))strcmp,
            .hash = shash});

    testrun(!ov_hashtable_contains(table, 0));

    for (size_t i = 0; i < num_keys; ++i) {
        testrun(!ov_hashtable_contains(table, keys[i]));
    }

    testrun(0 == ov_hashtable_set(table, keys[0], (void *)keys[0]));

    testrun(ov_hashtable_contains(table, keys[0]));

    for (size_t i = 1; i < num_keys; ++i) {
        testrun(!ov_hashtable_contains(table, keys[i]));
    }

    testrun(0 == ov_hashtable_set(table, keys[1], (void *)keys[1]));

    testrun(ov_hashtable_contains(table, keys[0]));
    testrun(ov_hashtable_contains(table, keys[1]));

    for (size_t i = 2; i < num_keys; ++i) {
        testrun(!ov_hashtable_contains(table, keys[i]));
    }

    testrun(0 == ov_hashtable_set(table, keys[2], (void *)keys[2]));

    testrun(ov_hashtable_contains(table, keys[0]));
    testrun(ov_hashtable_contains(table, keys[1]));
    testrun(ov_hashtable_contains(table, keys[2]));

    for (size_t i = 3; i < num_keys; ++i) {
        testrun(!ov_hashtable_contains(table, keys[i]));
    }

    table = ov_hashtable_free(table);
    testrun(0 == table);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_hashtable_get() {

    const char *keys[] = {"a",
                          "b",
                          "aa",
                          "c",
                          "ab",
                          "bb",
                          "def",
                          "Imagine you were at my station"
                          "And you brought your motor to me"
                          "Your a burner yeah a real motor car"
                          "Said you wanna get your order filled"
                          "Made me shiver when I put it in"
                          "Pumping just won't do ya know luckily for you"
                          ""
                          "Whoever thought you'd be better"
                          "At turning a screw than me"
                          "I do it for my life"
                          "Made my drive shaft crank"
                          "Made my pistons bulge"
                          "Made my ball bearing melt from the heat"
                          "Oh yeah yeah"
                          ""
                          "We were shifting hard when we took off"
                          "Put tonight all four on the floor"
                          "When we hit top end you know it feels to slow"
                          "Said you wanna get your order filled"
                          "Made me shiver when I put it in"
                          "Pumping just won't do ya know luckily for you"
                          ""
                          "Whoever thought you'd be better"
                          "At turning a screw than me"
                          "I do it for my life"
                          "Made my drive shaft crank"
                          "Made my pistons bulge"
                          "Made my ball bearing melt from the heat"
                          "Oh yeah yeah"
                          "Oh!"
                          ""
                          "I'm giving you my room service"
                          "And ya know it's more than enough"
                          "Oh one more time ya know I'm in love"
                          ""
                          "Whoever thought you'd be better"
                          "At turning a screw than me"
                          "I do it for my life"
                          "Made my drive shaft crank"
                          "Made my pistons bulge"
                          "Made my ball bearing melt from the heat"
                          "Oh yeah yeah!"
                          ""
                          "Imagine you were at my station"
                          "And you brought your motor to me"
                          "With all four on the floor I feel what's in "
                          "store"
                          ""
                          "Whoever thought you'd be better"
                          "At turning a screw than me"
                          "I do it for my life"
                          "Made my drive shaft crank"
                          "Made my pistons bulge"
                          "Made my ball bearing melt from the heat"
                          "Oh yeah yeah!",
                          "screw it we ran out of chars"};

    const size_t keys_len = sizeof(keys) / sizeof(keys[0]);

    testrun(0 == ov_hashtable_get(0, 0));

    ov_hashtable *table = ov_hashtable_create(
        1,
        (ov_hashtable_funcs){
            .key_cmp = (int (*)(const void *, const void *))strcmp,
            .hash = shash});

    testrun(0 == ov_hashtable_get(table, 0));
    testrun(0 == ov_hashtable_get(table, keys[0]));

    testrun(0 == ov_hashtable_set(table, keys[0], (void *)keys[1]));
    testrun(keys[1] == ov_hashtable_get(table, keys[0]));

    testrun(keys[1] == ov_hashtable_set(table, keys[0], (void *)keys[2]));
    testrun(keys[2] == ov_hashtable_get(table, keys[0]));

    for (size_t i = 1; i < keys_len; ++i) {

        testrun(0 == ov_hashtable_set(table, keys[i], (void *)keys[i - 1]));
        testrun(keys[i - 1] == ov_hashtable_get(table, keys[i]));
    }

    testrun(0 == ov_hashtable_free(table));

    /* Repeat but with more buckets */
    table = ov_hashtable_create(
        3,
        (ov_hashtable_funcs){
            .key_cmp = (int (*)(const void *, const void *))strcmp,
            .hash = shash});

    testrun(0 == ov_hashtable_get(table, 0));
    testrun(0 == ov_hashtable_get(table, keys[0]));

    testrun(0 == ov_hashtable_set(table, keys[0], (void *)keys[1]));
    testrun(keys[1] == ov_hashtable_get(table, keys[0]));

    testrun(keys[1] == ov_hashtable_set(table, keys[0], (void *)keys[2]));
    testrun(keys[2] == ov_hashtable_get(table, keys[0]));

    for (size_t i = 1; i < keys_len; ++i) {

        testrun(0 == ov_hashtable_set(table, keys[i], (void *)keys[i - 1]));
        testrun(keys[i - 1] == ov_hashtable_get(table, keys[i]));
    }

    testrun(0 == ov_hashtable_free(table));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_ov_hashtable_set() {

    const size_t NUM_ELEMENTS = UINT8_MAX;

    int value[] = {17, 19, 21, 23, 29, 31, 37};

    const size_t value_len = sizeof(value) / sizeof(value[0]);

    char *key[] = {"abc", "bcd", "bcc"};

    testrun(shash(key[0]) != shash(key[1]));
    testrun(shash(key[0]) != shash(key[2]));
    testrun(shash(key[1]) == shash(key[2]));

    testrun(0 == ov_hashtable_set(0, 0, 0));
    testrun(0 == ov_hashtable_set(0, 0, &value[0]));
    testrun(0 == ov_hashtable_set(0, key[0], &value[0]));
    testrun(0 == ov_hashtable_set(0, key[0], 0));

    ov_hashtable *table = ov_hashtable_create(
        NUM_ELEMENTS,
        (ov_hashtable_funcs){
            .key_cmp = (int (*)(const void *, const void *))strcmp,
            .hash = shash});

    testrun(0 == ov_hashtable_set(table, key[0], &value[0]));

    for (size_t i = 1; i < value_len; ++i) {
        testrun(&value[i - 1] == ov_hashtable_set(table, key[0], &value[i]));
    }

    testrun(0 == ov_hashtable_set(table, key[1], &value[0]));

    for (size_t i = 1; i < value_len; ++i) {
        testrun(&value[i - 1] == ov_hashtable_set(table, key[1], &value[i]));
    }

    /* Can be inserted despite the fact that there is a clash in hash values
     * with key[1] */
    testrun(0 == ov_hashtable_set(table, key[2], &value[0]));

    for (size_t i = 1; i < value_len; ++i) {
        testrun(&value[i - 1] == ov_hashtable_set(table, key[2], &value[i]));
    }

    testrun(&value[value_len - 1] ==
            ov_hashtable_set(table, key[0], &value[0]));

    testrun(0 == ov_hashtable_free(table));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_ov_hashtable_remove() {

    const char *keys[] = {"a",
                          "b",
                          "aa",
                          "c",
                          "ab",
                          "bb",
                          "def",
                          "Imagine you were at my station"
                          "And you brought your motor to me"
                          "Your a burner yeah a real motor car"
                          "Said you wanna get your order filled"
                          "Made me shiver when I put it in"
                          "Pumping just won't do ya know luckily for you"
                          ""
                          "Whoever thought you'd be better"
                          "At turning a screw than me"
                          "I do it for my life"
                          "Made my drive shaft crank"
                          "Made my pistons bulge"
                          "Made my ball bearing melt from the heat"
                          "Oh yeah yeah"
                          ""
                          "We were shifting hard when we took off"
                          "Put tonight all four on the floor"
                          "When we hit top end you know it feels to slow"
                          "Said you wanna get your order filled"
                          "Made me shiver when I put it in"
                          "Pumping just won't do ya know luckily for you"
                          ""
                          "Whoever thought you'd be better"
                          "At turning a screw than me"
                          "I do it for my life"
                          "Made my drive shaft crank"
                          "Made my pistons bulge"
                          "Made my ball bearing melt from the heat"
                          "Oh yeah yeah"
                          "Oh!"
                          ""
                          "I'm giving you my room service"
                          "And ya know it's more than enough"
                          "Oh one more time ya know I'm in love"
                          ""
                          "Whoever thought you'd be better"
                          "At turning a screw than me"
                          "I do it for my life"
                          "Made my drive shaft crank"
                          "Made my pistons bulge"
                          "Made my ball bearing melt from the heat"
                          "Oh yeah yeah!"
                          ""
                          "Imagine you were at my station"
                          "And you brought your motor to me"
                          "With all four on the floor I feel what's in "
                          "store"
                          ""
                          "Whoever thought you'd be better"
                          "At turning a screw than me"
                          "I do it for my life"
                          "Made my drive shaft crank"
                          "Made my pistons bulge"
                          "Made my ball bearing melt from the heat"
                          "Oh yeah yeah!",
                          "screw it we ran out of chars"};

    const size_t keys_len = sizeof(keys) / sizeof(keys[0]);

    testrun(0 == ov_hashtable_remove(0, 0));

    ov_hashtable *table = ov_hashtable_create(
        1,
        (ov_hashtable_funcs){
            .key_cmp = (int (*)(const void *, const void *))strcmp,
            .hash = shash});

    testrun(0 == ov_hashtable_remove(table, 0));
    testrun(0 == ov_hashtable_remove(table, keys[0]));

    /* Ok, now put something in the table */
    testrun(0 == ov_hashtable_set(table, keys[0], (void *)keys[1]));
    testrun(keys[1] == ov_hashtable_remove(table, "a"));

    /* Table should be empty again */
    testrun(0 == ov_hashtable_for_each(table, dummy_processor, 0));

    testrun(0 == ov_hashtable_set(table, keys[0], (void *)keys[1]));
    testrun(keys[1] == ov_hashtable_get(table, keys[0]));

    testrun(keys[1] == ov_hashtable_set(table, keys[0], (void *)keys[2]));
    testrun(keys[2] == ov_hashtable_get(table, keys[0]));

    for (size_t i = 1; i < keys_len; ++i) {

        testrun(0 == ov_hashtable_set(table, keys[i], (void *)keys[i - 1]));
        testrun(keys[i - 1] == ov_hashtable_get(table, keys[i]));

        /* We expect the table to contain i + 1 entries */
        testrun(i + 1 == ov_hashtable_for_each(table, dummy_processor, 0));
    }

    /* We expect the table to contain  keys_len  entries */
    testrun(keys_len == ov_hashtable_for_each(table, dummy_processor, 0));

    testrun((void *)keys[3] == ov_hashtable_remove(table, "ab"));
    testrun(keys_len - 1 == ov_hashtable_for_each(table, dummy_processor, 0));

    testrun((void *)keys[0] == ov_hashtable_remove(table, "b"));
    testrun(keys_len - 2 == ov_hashtable_for_each(table, dummy_processor, 0));

    testrun((void *)keys[2] == ov_hashtable_remove(table, "c"));
    testrun(keys_len - 3 == ov_hashtable_for_each(table, dummy_processor, 0));

    /* We altered this one */
    testrun((void *)keys[2] == ov_hashtable_remove(table, "a"));
    testrun(keys_len - 4 == ov_hashtable_for_each(table, dummy_processor, 0));

    testrun((void *)keys[1] == ov_hashtable_remove(table, "aa"));
    testrun(keys_len - 5 == ov_hashtable_for_each(table, dummy_processor, 0));

    testrun(0 == ov_hashtable_free(table));

    /* Repeat but with more buckets */
    table = ov_hashtable_create(
        3,
        (ov_hashtable_funcs){
            .key_cmp = (int (*)(const void *, const void *))strcmp,
            .hash = shash});

    testrun(0 == ov_hashtable_remove(table, 0));
    testrun(0 == ov_hashtable_remove(table, keys[0]));

    /* Ok, now put something in the table */
    testrun(0 == ov_hashtable_set(table, keys[0], (void *)keys[1]));
    testrun(keys[1] == ov_hashtable_remove(table, "a"));

    /* Table should be empty again */
    testrun(0 == ov_hashtable_for_each(table, dummy_processor, 0));

    testrun(0 == ov_hashtable_set(table, keys[0], (void *)keys[1]));
    testrun(keys[1] == ov_hashtable_get(table, keys[0]));

    testrun(keys[1] == ov_hashtable_set(table, keys[0], (void *)keys[2]));
    testrun(keys[2] == ov_hashtable_get(table, keys[0]));

    for (size_t i = 1; i < keys_len; ++i) {

        testrun(0 == ov_hashtable_set(table, keys[i], (void *)keys[i - 1]));
        testrun(keys[i - 1] == ov_hashtable_get(table, keys[i]));

        /* We expect the table to contain i + 1 entries */
        testrun(i + 1 == ov_hashtable_for_each(table, dummy_processor, 0));
    }

    /* We expect the table to contain  keys_len  entries */
    testrun(keys_len == ov_hashtable_for_each(table, dummy_processor, 0));

    testrun((void *)keys[3] == ov_hashtable_remove(table, "ab"));
    testrun(keys_len - 1 == ov_hashtable_for_each(table, dummy_processor, 0));

    testrun((void *)keys[0] == ov_hashtable_remove(table, "b"));
    testrun(keys_len - 2 == ov_hashtable_for_each(table, dummy_processor, 0));

    testrun((void *)keys[2] == ov_hashtable_remove(table, "c"));
    testrun(keys_len - 3 == ov_hashtable_for_each(table, dummy_processor, 0));

    /* We altered this one */
    testrun((void *)keys[2] == ov_hashtable_remove(table, "a"));
    testrun(keys_len - 4 == ov_hashtable_for_each(table, dummy_processor, 0));

    testrun((void *)keys[1] == ov_hashtable_remove(table, "aa"));
    testrun(keys_len - 5 == ov_hashtable_for_each(table, dummy_processor, 0));

    testrun(0 == ov_hashtable_free(table));

    /* Repeat with using 'real' string values as keys and automatic
     * key mem management */

    table = ov_hashtable_create(
        3,
        (ov_hashtable_funcs){
            .key_free = free,
            .key_copy = (void *(*)(const void *))strdup,
            .key_cmp = (int (*)(const void *, const void *))strcmp,
            .hash = shash});

    testrun(0 == ov_hashtable_remove(table, 0));
    testrun(0 == ov_hashtable_remove(table, keys[0]));

    /* Ok, now put something in the table */
    testrun(0 == ov_hashtable_set(table, keys[0], (void *)keys[1]));
    testrun(keys[1] == ov_hashtable_remove(table, "a"));

    /* Table should be empty again */
    testrun(0 == ov_hashtable_for_each(table, dummy_processor, 0));

    testrun(0 == ov_hashtable_set(table, keys[0], (void *)keys[1]));
    testrun(keys[1] == ov_hashtable_get(table, keys[0]));

    testrun(keys[1] == ov_hashtable_set(table, keys[0], (void *)keys[2]));
    testrun(keys[2] == ov_hashtable_get(table, keys[0]));

    for (size_t i = 1; i < keys_len; ++i) {

        testrun(0 == ov_hashtable_set(table, keys[i], (void *)keys[i - 1]));
        testrun(keys[i - 1] == ov_hashtable_get(table, keys[i]));

        /* We expect the table to contain i + 1 entries */
        testrun(i + 1 == ov_hashtable_for_each(table, dummy_processor, 0));
    }

    /* We expect the table to contain  keys_len  entries */
    testrun(keys_len == ov_hashtable_for_each(table, dummy_processor, 0));

    testrun((void *)keys[3] == ov_hashtable_remove(table, "ab"));
    testrun(keys_len - 1 == ov_hashtable_for_each(table, dummy_processor, 0));

    testrun((void *)keys[0] == ov_hashtable_remove(table, "b"));
    testrun(keys_len - 2 == ov_hashtable_for_each(table, dummy_processor, 0));

    testrun((void *)keys[2] == ov_hashtable_remove(table, "c"));
    testrun(keys_len - 3 == ov_hashtable_for_each(table, dummy_processor, 0));

    /* We altered this one */
    testrun((void *)keys[2] == ov_hashtable_remove(table, "a"));
    testrun(keys_len - 4 == ov_hashtable_for_each(table, dummy_processor, 0));

    testrun((void *)keys[1] == ov_hashtable_remove(table, "aa"));
    testrun(keys_len - 5 == ov_hashtable_for_each(table, dummy_processor, 0));

    testrun(0 == ov_hashtable_free(table));
    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_ov_hashtable_for_each() {

    /* This test checks functionality of ov_hashtable_for_each.
     * But it also demonstrates a real-life application of
     * ov_hashtable_for_each():
     * Free values stored in the hashtable before termination of the hash
     * table
     */
    const uint8_t start_value = 30;
    const uint8_t end_value = 120;
    const size_t number_of_set_values = 1 + end_value - start_value;

    size_t counters[1 + UINT8_MAX] = {0};

    const size_t num_counters = sizeof(counters) / sizeof(size_t);

    testrun(0 == ov_hashtable_for_each(0, 0, 0));
    testrun(0 == ov_hashtable_for_each(0, count_value, 0));
    testrun(0 == ov_hashtable_for_each(0, count_value, &counters));

    ov_hashtable *table = ov_hashtable_create(
        5, (ov_hashtable_funcs){.key_cmp = scmp, .hash = shash});

    testrun(0 == ov_hashtable_for_each(table, count_value, &counters));

    for (size_t i = 0; i < num_counters; ++i) {

        testrun(0 == counters[i]);
    }

    for (uint8_t v = start_value; v <= end_value; ++v) {

        uint8_t *vp = calloc(1, sizeof(uint8_t));
        *vp = v;

        /*     Must be a pointer with different values, therefore we use
         *                    `address of v` plus `value of v`
         *                                     |
         *                                     V */
        testrun(0 == ov_hashtable_set(table, &v + v, vp));
    }

    testrun(number_of_set_values ==
            ov_hashtable_for_each(table, count_value, &counters));

    for (size_t i = 0; i < start_value; ++i) {

        testrun(0 == counters[i]);
    }

    for (size_t i = start_value; i <= end_value; ++i) {

        testrun(1 == counters[i]);
    }

    for (size_t i = 1 + end_value; i <= UINT8_MAX; ++i) {

        testrun(0 == counters[i]);
    }

    /* add one more value, check */

    uint8_t v = 0;
    uint8_t *vp = calloc(1, sizeof(uint8_t));
    *vp = v;

    testrun(0 == ov_hashtable_set(table, &v, vp));

    testrun(number_of_set_values + 1 ==
            ov_hashtable_for_each(table, count_value, &counters));

    testrun(1 == counters[0]);

    for (size_t i = 1; i < start_value; ++i) {

        testrun(0 == counters[i]);
    }

    for (size_t i = start_value; i <= end_value; ++i) {

        testrun(2 == counters[i]);
    }

    for (size_t i = 1 + end_value; i <= UINT8_MAX; ++i) {

        testrun(0 == counters[i]);
    }

    /* Free all values */
    testrun(number_of_set_values + 1 ==
            ov_hashtable_for_each(table, free_value, 0));

    testrun(0 == ov_hashtable_free(table));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_ov_hashtable_free() {

    testrun(0 == ov_hashtable_free(0));

    ov_hashtable *table = ov_hashtable_create(10, (ov_hashtable_funcs){0});
    testrun(0 == ov_hashtable_free(table));

    table = ov_hashtable_create(10, (ov_hashtable_funcs){.hash = shash});
    testrun(0 == ov_hashtable_free(table));

    table = ov_hashtable_create(10, (ov_hashtable_funcs){.key_cmp = scmp});
    testrun(0 == ov_hashtable_free(table));

    table = ov_hashtable_create(
        10, (ov_hashtable_funcs){.key_cmp = scmp, .hash = shash});
    testrun(0 == ov_hashtable_free(table));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int check_hashtable_clear() {

    /* ... */

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_ov_hashtable_create_c_string() {

    ov_hashtable *table = ov_hashtable_create_c_string(255);
    testrun(table);

    testrun(table->number_of_buckets == 255);
    testrun(table->funcs.key_free == free);
    testrun(table->funcs.key_copy != 0);
    testrun(table->funcs.key_cmp != 0);
    testrun(table->funcs.hash == ov_hash_simple_c_string);

    testrun(table->entries != NULL);
    testrun(NULL == ov_hashtable_free(table));

    return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

OV_TEST_RUN("ov_hashtable",
            test_ov_hashtable_create,
            check_get_entry_for,
            test_ov_hashtable_contains,
            test_ov_hashtable_get,
            test_ov_hashtable_set,
            test_ov_hashtable_remove,
            test_ov_hashtable_for_each,
            test_ov_hashtable_free,
            check_hashtable_clear,
            test_ov_hashtable_create_c_string);
