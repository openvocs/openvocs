/***
        ------------------------------------------------------------------------

        Copyright (c) 2018 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_dict.h
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-08-15

        @ingroup        ov_base

        @brief          Definition of a standard interface for KEY/VALUE
                        based implementations used for openvocs.


        ------------------------------------------------------------------------
*/
#ifndef ov_dict_h
#define ov_dict_h

#include "ov_data_function.h"
#include "ov_hash_functions.h"
#include "ov_list.h"
#include "ov_match_functions.h"

#include <ov_log/ov_log.h>

#define OV_DICT_MAGIC_BYTE 0xaabb

typedef struct ov_dict ov_dict;
typedef struct ov_dict_config ov_dict_config;

struct ov_dict_config {

    /* Buckets to be used */
    uint64_t slots;

    /* Key confguration */
    struct {

        /* Key content configuration */
        ov_data_function data_function;

        /* Key functions configuration */
        uint64_t (*hash)(const void *key);
        bool (*match)(const void *key, const void *value);

        bool (*validate_input)(const void *data);
    } key;

    struct {

        /* Value content configuration */
        ov_data_function data_function;
        bool (*validate_input)(const void *data);
    } value;
};

/*---------------------------------------------------------------------------*/

struct ov_dict {

    uint16_t magic_byte;
    uint16_t type;

    ov_dict_config config;

    /*      Check if any ANY item is set within the dict */
    bool (*is_empty)(const ov_dict *self);

    /*
     *      Function pointer to own create.
     */
    ov_dict *(*create)(ov_dict_config config);

    /*
     *      Clear MUST delete all key/value pairs,
     *      using the configured configuration.
     */
    bool (*clear)(ov_dict *self);

    /*
     *      Free MUST delete all key/value pairs,
     *      and free the dict pointer.
     *      @returns NULL on success, self on error!
     */
    ov_dict *(*free)(ov_dict *self);

    /*
     *      Check at all keys if the value pointer is contained.
     *      If value is 0, all keys are returned.
     *      return a list with pointers to all keys.
     */
    ov_list *(*get_keys)(const ov_dict *self, const void *value);

    /*
     *      Get MUST return the pointer to value used
     *      at key.
     */
    void *(*get)(const ov_dict *self, const void *key);

    /*
     *      Set MUST set the value of key within the dict.
     *      If replaced is NULL, ANY existing value MUST be
     *      freed using the value configuration, if replaced is
     *      NOT NULL, any old value MUST be returned over replaced.
     */
    bool (*set)(ov_dict *self, void *key, void *value, void **replaced);

    /*
     *      Del MUST remove the key/value pair and free the pointers
     *      using the dict configuration.
     */
    bool (*del)(ov_dict *self, const void *key);

    /*
     *      Remove MUST remove the key/value pair and return the value,
     *      without deleting it automatically.
     */
    void *(*remove)(ov_dict *self, const void *key);

    /*
     *      For_each MUST apply function at each key value pair.
     */
    bool (*for_each)(ov_dict *self, void *data,
                     bool (*function)(const void *key, void *value,
                                      void *data));
};

/*
 *      ------------------------------------------------------------------------
 *
 *                        DEFAULT STRUCTURE CREATION
 *
 *      ------------------------------------------------------------------------
 */

/**
        Create a dict with a config. The config MUST be valid,
        which means it need to include a hash as well as a match
        function for keys, and a given slotsize > 0
        MOST used config will be @see ov_dict_string_key_config
*/
ov_dict *ov_dict_create(ov_dict_config config);

ov_dict *ov_dict_cast(const void *data);
ov_dict *ov_dict_set_magic_bytes(ov_dict *dict);

/*
 *      ------------------------------------------------------------------------
 *
 *                        GENERAL TESTING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Checks config and if all function pointers are set.
*/
bool ov_dict_is_valid(const ov_dict *dict);
bool ov_dict_config_is_valid(const ov_dict_config *config);
bool ov_dict_is_set(const ov_dict *dict, const void *key);
int64_t ov_dict_count(const ov_dict *dict);

/*---------------------------------------------------------------------------*/

/**
    Dump the load at the slots to file,
    NOTE will only work with standard dict yet.
*/
bool ov_dict_dump_load_factor(FILE *stream, const ov_dict *dict);

/*
 *      ------------------------------------------------------------------------
 *
 *                        FUNCTIONS TO INTERNAL POINTERS
 *
 *
 *      ... following functions check if the dict has the respective function
 * and execute the linked function.
 *      ------------------------------------------------------------------------
 */

bool ov_dict_is_empty(const ov_dict *dict);
ov_list *ov_dict_get_keys(const ov_dict *dict, const void *value);
void *ov_dict_get(const ov_dict *dict, const void *key);
bool ov_dict_set(ov_dict *dict, void *key, void *value, void **replaced);
bool ov_dict_del(ov_dict *dict, const void *key);
void *ov_dict_remove(ov_dict *dict, const void *key);
bool ov_dict_for_each(ov_dict *dict, void *data,
                      bool (*function)(const void *key, void *value,
                                       void *data));

/*
 *      ------------------------------------------------------------------------
 *
 *                        DEFAULT CONFIGURATIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
        Creates a config, which expects allocated strings as
        keys for a dictionary.

        {
                .slots                          = slots,

                .key.data_function.free         = ov_data_string_free,
                .key.data_function.clear        = ov_data_string_clear,
                .key.data_function.copy         = ov_data_string_copy,
                .key.data_function.dump         = ov_data_string_dump,

                .key.hash                       = ov_hash_pearson_c_string,
                .key.match                      = ov_match_c_string_strict,

                .value.data_function.free       = NULL,
                .value.data_function.clear      = NULL,
                .value.data_function.copy       = NULL,
                .value.data_function.dump       = NULL,

        }
*/
ov_dict_config ov_dict_string_key_config(size_t slots);

/*----------------------------------------------------------------------------*/

/**
        Creates a config, which expects intptr_t
        keys for a dictionary.

        {
                .slots                          = slots,

                .key.data_function.free         = NULL,
                .key.data_function.clear        = NULL,
                .key.data_function.copy         = NULL,
                .key.data_function.dump         = ov_data_intptr_dump,

                .key.hash                       = ov_hash_intptr,
                .key.match                      = ov_match_intptr,

                .value.data_function.free       = NULL,
                .value.data_function.clear      = NULL,
                .value.data_function.copy       = NULL,
                .value.data_function.dump       = NULL,

        }
*/
ov_dict_config ov_dict_intptr_key_config(size_t slots);

/*----------------------------------------------------------------------------*/

/**
        Creates a config, which expects uint64_t pointer
        keys for a dictionary.

        {
                .slots                          = slots,

                .key.data_function.free         = ov_data_uint64_free,
                .key.data_function.clear        = ov_data_uint64_clear,
                .key.data_function.copy         = ov_data_uint64_copy,
                .key.data_function.dump         = ov_data_uint64_dump,

                .key.hash                       = ov_hash_uint64,
                .key.match                      = ov_match_uint64,

                .value.data_function.free       = NULL,
                .value.data_function.clear      = NULL,
                .value.data_function.copy       = NULL,
                .value.data_function.dump       = NULL,

        }
*/
ov_dict_config ov_dict_uint64_key_config(size_t slots);

/*
 *      ------------------------------------------------------------------------
 *
 *                        DATA FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_data_function ov_dict_data_functions();
bool ov_dict_clear(void *data);
void *ov_dict_free(void *data);
void *ov_dict_copy(void **destination, const void *data);
bool ov_dict_dump(FILE *stream, const void *data);

/*
 *      ------------------------------------------------------------------------
 *
 *                        GENERIC FUNCTIONS
 *
 *       ... definition of common generic list functions
 *
 *      ------------------------------------------------------------------------
 */

/**
        Remove a value from a dict, if it is included.
        This function checks if the item pointer is contained as list content.
        If so the value will be removed from the list. (first occurance only)
*/
bool ov_dict_remove_if_included(ov_dict *dict, const void *value);

#endif /* ov_dict_h */
