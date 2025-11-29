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

        This file is part of the openvocs project. https://openvocs.org

        ------------------------------------------------------------------------
*//**
        @file           ov_item.h
        @author         Töpfer, Markus

        @date           2025-11-28


        ------------------------------------------------------------------------
*/
#ifndef ov_item_h
#define ov_item_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_item ov_item;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_item *ov_item_cast(const void *self);

/*
 *      ------------------------------------------------------------------------
 *
 *      DATA FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

void *ov_item_free(void *self);
bool ov_item_clear(void *self);
void *ov_item_copy(void **destination, const void *self);
bool ov_item_dump(FILE *stream, const void *self);

size_t ov_item_count(ov_item *self);

/*
 *      ------------------------------------------------------------------------
 *
 *      OBJECT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_item *ov_item_object();
bool ov_item_is_object(ov_item *self);

/*---------------------------------------------------------------------------*/

/**
 *  Threadsafe set val as new item in self.
 */
bool ov_item_object_set(ov_item *self, const char *string, ov_item *val);

/*---------------------------------------------------------------------------*/

/**
 *  Threadsafe del val item in self.
 */
bool ov_item_object_delete(ov_item *self, const char *string);

/*---------------------------------------------------------------------------*/

/**
 *  Threadsafe remove val item in self.
 */
ov_item *ov_item_object_remove(ov_item *self, const char *string);

/*---------------------------------------------------------------------------*/

/**
 *  NOT threadsafe get item of self.
 */
ov_item *ov_item_object_get(ov_item *self, const char *string);

/*---------------------------------------------------------------------------*/

/**
 *  Threadsafe for each for all elements of self.
 */
bool ov_item_object_for_each(ov_item *self, 
                             bool (*function)(
                                char const *key,
                                ov_item const *val,
                                void *userdata),
                             void *userdata);
/*
 *      ------------------------------------------------------------------------
 *
 *      ARRAY FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_item *ov_item_array();
bool ov_item_is_array(ov_item *self);

/**
 *  NOT threadsafe get item of self.
 */
ov_item *ov_item_array_get(ov_item *self, uint64_t pos);

/*---------------------------------------------------------------------------*/

/**
 *  Threadsafe set val as new item in self.
 */
bool ov_item_array_set(ov_item *self, uint64_t pos, ov_item *val);

/*---------------------------------------------------------------------------*/

/**
 *  Threadsafe push val as new item to self. (push to end)
 */
bool ov_item_array_push(ov_item *self, ov_item *val);

/*---------------------------------------------------------------------------*/

/**
 *  Threadsafe pop val from end of array from self.
 *  LIFO
 */
ov_item *ov_item_array_stack_pop(ov_item *self);
ov_item *ov_item_array_lifo(ov_item *self);

/*---------------------------------------------------------------------------*/

/**
 *  Threadsafe pop val from front of array from self.
 *  FIFO
 */
ov_item *ov_item_array_queue_pop(ov_item *self);
ov_item *ov_item_array_fifo(ov_item *self);

/*
 *      ------------------------------------------------------------------------
 *
 *      STRING FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_item *ov_item_string(const char *string);
bool ov_item_is_string(ov_item *self);
const char *ov_item_get_string(ov_item *self);

/*
 *      ------------------------------------------------------------------------
 *
 *      NUMBER FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_item *ov_item_number(double number);
bool ov_item_is_number(ov_item *self);
double ov_item_get_number(ov_item *self);
int64_t ov_item_get_int(ov_item *self);

/*
 *      ------------------------------------------------------------------------
 *
 *      LITERAL FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_item *ov_item_null();
bool ov_item_is_null(ov_item *self);

ov_item *ov_item_true();
bool ov_item_is_true(ov_item *self);

ov_item *ov_item_false();
bool ov_item_is_false(ov_item *self);


#endif /* ov_item_h */
