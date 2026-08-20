/***
        ------------------------------------------------------------------------

        Copyright (c) 2025 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the opendtn project. https://opendtn.com

        ------------------------------------------------------------------------
*//**
        @file           ov_cbor.h
        @author         Töpfer, Markus

        @date           2025-12-12

        Implementation of RFC 8949 Concise Binary Object Representation (CBOR)

        ------------------------------------------------------------------------
*/
#ifndef ov_cbor_h
#define ov_cbor_h

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

/*----------------------------------------------------------------------------*/

typedef enum ov_cbor_type {

    ov_CBOR_UNDEF = 0,
    ov_CBOR_FALSE,
    ov_CBOR_TRUE,
    ov_CBOR_NULL,
    ov_CBOR_UINT64,
    ov_CBOR_INT64,
    ov_CBOR_STRING,
    ov_CBOR_UTF8,
    ov_CBOR_ARRAY,
    ov_CBOR_MAP,
    ov_CBOR_DATE_TIME,
    ov_CBOR_DATE_TIME_EPOCH,
    ov_CBOR_UBIGNUM,
    ov_CBOR_IBIGNUM,
    ov_CBOR_DEC_FRACTION,
    ov_CBOR_BIGFLOAT,
    ov_CBOR_TAG,
    ov_CBOR_SIMPLE,
    ov_CBOR_FLOAT,
    ov_CBOR_DOUBLE

} ov_cbor_type;

/*----------------------------------------------------------------------------*/

typedef struct ov_cbor ov_cbor;

/*----------------------------------------------------------------------------*/

typedef struct ov_cbor_config {

    struct {

        uint64_t string_size;
        uint64_t utf8_string_size;
        uint64_t array_size;
        uint64_t undef_length_array;
        uint64_t map_size;
        uint64_t undef_length_map;

    } limits;

} ov_cbor_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_cbor *ov_cbor_map();
ov_cbor *ov_cbor_array();
ov_cbor *ov_cbor_string(const char *string);
ov_cbor *ov_cbor_utf8(const uint8_t *buffer, size_t size);
ov_cbor *ov_cbor_true();
ov_cbor *ov_cbor_false();
ov_cbor *ov_cbor_null();
ov_cbor *ov_cbor_undef();
ov_cbor *ov_cbor_uint(uint64_t value);
ov_cbor *ov_cbor_int(int64_t value);
ov_cbor *ov_cbor_time(const char *timestamp);
ov_cbor *ov_cbor_time_epoch(uint64_t value);
ov_cbor *ov_cbor_ubignum(const char *num);
ov_cbor *ov_cbor_ibignum(const char *num);
ov_cbor *ov_cbor_dec_fraction(ov_cbor *array);
ov_cbor *ov_cbor_bigfloat(ov_cbor *array);
ov_cbor *ov_cbor_tag(uint64_t tag);
ov_cbor *ov_cbor_simple(uint64_t nbr);
ov_cbor *ov_cbor_float(float nbr);
ov_cbor *ov_cbor_double(double nbr);

ov_cbor *ov_cbor_free(ov_cbor *self);
ov_cbor_type ov_cbor_get_type(const ov_cbor *self);

void *ov_cbor_copy(void **copy, void *self);

/*----------------------------------------------------------------------------*/

/**
 *  Configure MAX item values for the parser.
 *
 *  SHOULD be used for every usage szenario due to potentially very large
 *  allocation of memory.
 */
bool ov_cbor_configure(ov_cbor_config config);

/*
 *      ------------------------------------------------------------------------
 *
 *      DE/ENCODER
 *
 *      ------------------------------------------------------------------------
 */

typedef enum ov_cbor_match {

    ov_CBOR_NO_MATCH = 0,
    ov_CBOR_MATCH_PARTIAL = 1,
    ov_CBOR_MATCH_FULL = 2

} ov_cbor_match;

/*----------------------------------------------------------------------------*/

/**
 *  Decode a CBOR buffer to some value.
 *
 *  @param buffer   pointer to buffer to decode
 *  @param size     size of buffer
 *  @param out      pointer to decoded value
 *  @param next     pointer to next byte after decoded value
 */
ov_cbor_match ov_cbor_decode(const uint8_t *buffer, size_t size,
                               ov_cbor **out, uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
 *  Encode a CBOR value to some buffer.
 *
 *  @param value    value to encode
 *  @param buffer   pointer to buffer
 *  @param size     size of buffer
 *  @param next     pointer to next byte after encoded value
 */
bool ov_cbor_encode(const ov_cbor *value, uint8_t *buffer, size_t size,
                     uint8_t **next);

/*----------------------------------------------------------------------------*/

/**
 *  Encode a CBOR array as a indefinite_length value to some buffer.
 *  This is the implemetation required for DTN bundle protocol RFC 9171
 *
 *  @param value    value to encode
 *  @param buffer   pointer to buffer
 *  @param size     size of buffer
 *  @param next     pointer to next byte after encoded value
 */
bool ov_cbor_encode_array_of_indefinite_length(const ov_cbor *value,
                                                uint8_t *buffer, size_t size,
                                                uint8_t **next);

/*----------------------------------------------------------------------------*/

uint64_t ov_cbor_encoding_size(const ov_cbor *value);

/*
 *      ------------------------------------------------------------------------
 *
 *      ITEM CHECKS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_cbor_is_map(const ov_cbor *self);
bool ov_cbor_is_array(const ov_cbor *self);
bool ov_cbor_is_string(const ov_cbor *self);
bool ov_cbor_is_uft8(const ov_cbor *self);
bool ov_cbor_is_true(const ov_cbor *self);
bool ov_cbor_is_false(const ov_cbor *self);
bool ov_cbor_is_null(const ov_cbor *self);
bool ov_cbor_is_undef(const ov_cbor *self);
bool ov_cbor_is_uint(const ov_cbor *self);
bool ov_cbor_is_int(const ov_cbor *self);
bool ov_cbor_is_time(const ov_cbor *self);
bool ov_cbor_is_time_epoch(const ov_cbor *self);
bool ov_cbor_is_ubignum(const ov_cbor *self);
bool ov_cbor_is_ibignum(const ov_cbor *self);
bool ov_cbor_is_dec_fraction(const ov_cbor *self);
bool ov_cbor_is_bigfloat(const ov_cbor *self);
bool ov_cbor_is_tag(const ov_cbor *self);
bool ov_cbor_is_simple(const ov_cbor *self);
bool ov_cbor_is_float(const ov_cbor *self);
bool ov_cbor_is_double(const ov_cbor *self);

/*
 *      ------------------------------------------------------------------------
 *
 *      MAP GETTER / SETTER
 *
 *      ------------------------------------------------------------------------
 */

/**
 *  Set some key / value pair,
 *  on success key and value become part of the map
 *  @param map  map instance
 *  @param key  key to set in map
 *  @param val  value to set at key.
 */
bool ov_cbor_map_set(ov_cbor *map, ov_cbor *key, ov_cbor *val);

/*----------------------------------------------------------------------------*/

/**
 *  Get some map value based on a ov_cbor key.
 *  @param map  map instance
 *  @param key  key to get
 */
ov_cbor *ov_cbor_map_get(const ov_cbor *map, const ov_cbor *key);

/*----------------------------------------------------------------------------*/

/**
 *  Set some value at a string key, the ov_cbor_string for the key
 *  will be autgenerated.
 *  This is a convinience function for usage of string key based maps.
 *
 *  @param map  map instance
 *  @param key  string to set
 *  @param val  value to be set at key, value will become part of the map
 */
bool ov_cbor_map_set_string(ov_cbor *map, const char *key, ov_cbor *val);

/*----------------------------------------------------------------------------*/

/**
 *  Get some string based key using the string instead of a ov_cbor string.
 *  This is a convinience function for usage of string key based maps.
 *
 *  @param map  map instance
 *  @param key  keystring to search
 */
ov_cbor *ov_cbor_map_get_string(const ov_cbor *map, const char *key);

/*----------------------------------------------------------------------------*/

/**
 *  This function will apply the function input on any item of the map.
 *  Use with care and don't delete keys or values using this function.
 *
 *  @param map      map instance
 *  @param data     custom userdata to be used as input to the function
 *  @param function function to be applied to any key/value pair
 */
bool ov_cbor_map_for_each(ov_cbor *map, void *data,
                           bool (*function)(const void *key, void *val,
                                            void *data));

/*----------------------------------------------------------------------------*/

uint64_t ov_cbor_map_count(const ov_cbor *map);

/*
 *      ------------------------------------------------------------------------
 *
 *      ARRAY GETTER / SETTER
 *
 *      ------------------------------------------------------------------------
 */

/**
 *  Get some item out of the array.
 *
 *  @param self     array instance
 *  @param index    index 0 ... max
 */
ov_cbor *ov_cbor_array_get(const ov_cbor *self, uint64_t index);

bool ov_cbor_array_set(ov_cbor *self, uint64_t pos, ov_cbor *value);
/*----------------------------------------------------------------------------*/

/**
 *  Push some item to the end of the array.
 *
 *  @param self     array instance
 *  @param val      value to be set within the array.
 */
bool ov_cbor_array_push(ov_cbor *self, ov_cbor *val);

/*----------------------------------------------------------------------------*/

/**
 *  Pop some item from the front of the array FIFO.
 */
ov_cbor *ov_cbor_array_pop_queue(ov_cbor *self);

/*----------------------------------------------------------------------------*/

/**
 *  Pop some item from the back of the array LIFO.
 */
ov_cbor *ov_cbor_array_pop_stack(ov_cbor *self);

/*----------------------------------------------------------------------------*/

/**
 *  This function will apply the function input on any item of the array.
 *  Use with care and don't delete values using this function.
 *
 *  @param self     array instance
 *  @param data     custom userdata to be used as input to the function
 *  @param function function to be applied to any key/value pair
 */
bool ov_cbor_array_for_each(ov_cbor *self, void *data,
                             bool (*function)(void *item, void *data));

/*----------------------------------------------------------------------------*/

uint64_t ov_cbor_array_count(const ov_cbor *array);

/*
 *      ------------------------------------------------------------------------
 *
 *      GETTER / SETTER
 *
 *      ------------------------------------------------------------------------
 */

const char *ov_cbor_get_string(const ov_cbor *self);
bool ov_cbor_set_string(ov_cbor *self, const char *string);

bool ov_cbor_set_byte_string(ov_cbor *self, const uint8_t *byte, size_t size);
bool ov_cbor_get_byte_string(const ov_cbor *self, uint8_t **byte,
                              size_t *size);

/*----------------------------------------------------------------------------*/

bool ov_cbor_get_utf8(ov_cbor *self, uint8_t **buffer, size_t *size);
bool ov_cbor_set_utf8(ov_cbor *self, const uint8_t *buffer, size_t size);

/*----------------------------------------------------------------------------*/

uint64_t ov_cbor_get_uint(const ov_cbor *self);
bool ov_cbor_set_uint(ov_cbor *self, uint64_t value);

/*----------------------------------------------------------------------------*/

int64_t ov_cbor_get_int(const ov_cbor *self);
bool ov_cbor_set_int(ov_cbor *self, int64_t value);

/*----------------------------------------------------------------------------*/

const char *ov_cbor_get_time(const ov_cbor *self);
bool ov_cbor_set_time(ov_cbor *self, const char *timestamp);

/*----------------------------------------------------------------------------*/

uint64_t ov_cbor_get_time_epoch(const ov_cbor *self);
bool ov_cbor_set_time_epoch(ov_cbor *self, uint64_t value);

/*----------------------------------------------------------------------------*/

const char *ov_cbor_get_ubignum(const ov_cbor *self);
bool ov_cbor_set_ubignum(ov_cbor *self, const char *num);

/*----------------------------------------------------------------------------*/

const char *ov_cbor_get_ibignum(const ov_cbor *self);
bool ov_cbor_set_ibignum(ov_cbor *self, const char *num);

/*----------------------------------------------------------------------------*/

const ov_cbor *ov_cbor_get_dec_fraction(const ov_cbor *self);
bool ov_cbor_set_dec_fraction(ov_cbor *self, ov_cbor *array);

/*----------------------------------------------------------------------------*/

const ov_cbor *ov_cbor_get_bigfloat(const ov_cbor *self);
bool ov_cbor_set_bigfloat(ov_cbor *self, ov_cbor *array);

/*----------------------------------------------------------------------------*/

uint64_t ov_cbor_get_tag(const ov_cbor *self);
uint64_t ov_cbor_get_tag_value(const ov_cbor *self);
const ov_cbor *ov_cbor_get_tag_data(const ov_cbor *self);
bool ov_cbor_set_tag(ov_cbor *self, uint64_t tag);
bool ov_cbor_set_tag_data(ov_cbor *self, ov_cbor *data);
bool ov_cbor_set_tag_value(ov_cbor *self, uint64_t val);

/*----------------------------------------------------------------------------*/

uint64_t ov_cbor_get_simple_value(const ov_cbor *self);
uint64_t ov_cbor_get_simple(const ov_cbor *self);
bool ov_cbor_set_simple(ov_cbor *self, uint64_t nbr);
bool ov_cbor_set_simple_value(ov_cbor *self, uint64_t nbr);

/*----------------------------------------------------------------------------*/

float ov_cbor_get_float(const ov_cbor *self);
bool ov_cbor_set_float(ov_cbor *self, float nbr);

/*----------------------------------------------------------------------------*/

double ov_cbor_get_double(const ov_cbor *self);
bool ov_cbor_set_double(ov_cbor *self, double nbr);

#endif /* ov_cbor_h */