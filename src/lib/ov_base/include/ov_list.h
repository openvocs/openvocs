/***
        ------------------------------------------------------------------------

        Copyright 2018 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_list.h
        @author         Michael Beer
        @author         Markus Toepfer

        @date           2018-07-02

        @ingroup        ov_base

        @brief          Definition of a standard interface for LIST
                        implementations used for openvocs.

        Supports both FIFO and LIFO functionality:

        LIFO

        * insert via ov_list_push(list, item) at end of list
        * get & remove via ov_list_pop(list) to retrive from end of list

        FIFO

        *  Insert via ov_list_insert(list, item, 0) at front of list
        *  get & remove via ov_list_pop(list) to retrieve from end of list

        BEWARE: Default implementation is not very performant in terms of
        FIFO operations - better use `ov_linked_list`  in that case.

        ------------------------------------------------------------------------
*/
#ifndef ov_list_h
#define ov_list_h

#include "ov_data_function.h"
#include <ov_log/ov_log.h>

#define OV_LIST_MAGIC_BYTE 0xabcd

typedef struct ov_list ov_list;
typedef struct ov_list_config ov_list_config;

/*---------------------------------------------------------------------------*/

struct ov_list_config {

    ov_data_function item;
};

/*---------------------------------------------------------------------------*/

struct ov_list {

    uint16_t magic_byte;
    uint16_t type;

    ov_list_config config;

    bool (*is_empty)(const ov_list *self);

    bool (*clear)(ov_list *self);
    /* Returns self on ERROR, NULL on SUCCESS */
    ov_list *(*free)(ov_list *self);

    /* Returns pointer to destination on success, NULL otherwise */
    ov_list *(*copy)(const ov_list *self, ov_list *destination);

    /* GET position of item within list, returns 0 on error */
    size_t (*get_pos)(const ov_list *self, const void *item);

    /* GET returns list item at pos FIRST item is at pos 1*/
    void *(*get)(ov_list *self, size_t pos);
    /* SET returns any previously set old_item over the replaced pointer */
    bool (*set)(ov_list *self, size_t pos, void *item, void **replaced);

    /* INSERT will move all following items to pos + 1 */
    bool (*insert)(ov_list *self, size_t pos, void *item);
    /* REMOVE will return a removed element, NULL on error, all following
     * will move pos - 1*/
    void *(*remove)(ov_list *self, size_t pos);

    bool (*push)(ov_list *self, void *item);
    void *(*pop)(ov_list *self);

    size_t (*count)(const ov_list *self);

    bool (*for_each)(ov_list *self,
                     void *data,
                     bool (*function)(void *item, void *data));

    /**
     * @return an iterator pointing at the front of the list. To be used
     * with next. Becomes invalid if list is modified.
     */
    void *(*iter)(ov_list *self);

    /**
     * @param iter an iterator retrieved with e.g. ov_llist->iter.
     * @param element receives a pointer to the element iter currently
     * points at
     * @return updated iterator
     */
    void *(*next)(ov_list *self, void *iter, void **element);
};

/*
 *      ------------------------------------------------------------------------
 *
 *                        DEFAULT STRUCTURE CREATION
 *
 *      ------------------------------------------------------------------------
 */

ov_list *ov_list_cast(const void *data);
ov_list *ov_list_create(ov_list_config config);

/*
 *      ------------------------------------------------------------------------
 *
 *                        DATA FUNCTIONS
 *
 *       ... used to use a ov_list within a parent container structure.
 *
 *      ------------------------------------------------------------------------
 */

bool ov_list_clear(void *data);
void *ov_list_free(void *data);
void *ov_list_copy(void **destination, const void *source);
bool ov_list_dump(FILE *stream, const void *data);
ov_data_function ov_list_data_functions();

/*
 *      ------------------------------------------------------------------------
 *
 *                        FUNCTIONS TO INTERNAL POINTERS
 *
 *
 *      ... following functions check if the list has the respective function
 * and execute the linked function.
 *      ------------------------------------------------------------------------
 */

bool ov_list_is_empty(const ov_list *list);
size_t ov_list_get_pos(const ov_list *list, void *item);
void *ov_list_get(const ov_list *list, size_t pos);
bool ov_list_set(ov_list *list, size_t pos, void *item, void **replaced);
bool ov_list_insert(ov_list *list, size_t pos, void *item);
void *ov_list_remove(ov_list *list, size_t pos);
bool ov_list_delete(ov_list *list, size_t pos);

/**
 * Insert element at end of list
 */
bool ov_list_push(ov_list *list, void *item);

/**
 * Retrieve * remove element from end of list
 */
void *ov_list_pop(ov_list *list);

size_t ov_list_count(const ov_list *list);
bool ov_list_for_each(ov_list *list,
                      void *data,
                      bool (*function)(void *item, void *data));

/******************************************************************************
 *    Implementation independent default versions of some member functions
 ******************************************************************************/

ov_list *ov_list_set_magic_bytes(ov_list *list);

typedef struct ov_list_default_implementations {

    ov_list *(*copy)(const ov_list *self, ov_list *destination);

} ov_list_default_implementations;

ov_list_default_implementations ov_list_get_default_implementations();

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
        Remove a item from a list, if it is included.
        This function checks if the item pointer is contained as list content.
        If so the value will be removed from the list. (first occurance only)
*/
bool ov_list_remove_if_included(ov_list *list, const void *item);

bool ov_list_queue_push(ov_list *list, void *item);
void *ov_list_queue_pop(ov_list *list);

#endif /* ov_list_h */
