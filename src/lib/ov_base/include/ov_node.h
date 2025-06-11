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
        @file           ov_node.h
        @author         Markus Töpfer

        @date           2019-11-28

        @ingroup        ov_node

        @brief          Abstract definition of a double linked list to
                        support inheritage of list functionality
                        in any struct.

                        This function may be used to make any struct a list.
                        ov_node MUST be the head of the struct. The new struct
                        will inheritage all node functions, as long as
                        ov_node is the the head of the new struct.

                        It is connecting memory areas as lists. Use with care,
                        as not typechecking is done (on purpuse) to speed
                        up development and simplyfy access.

                        This is a zero overhead double list functionality for
                        any struct. It is NOT covered with a magic_byte to
                        provide the functionality without additional costs.

                        Again, take care if you use this node list,
                        as no typechecking is done at all.

                        For an example use have a look @see check_node_list
                        within the tests.

        ------------------------------------------------------------------------
*/
#ifndef ov_node_h
#define ov_node_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct ov_node ov_node;

struct ov_node {

    uint16_t type; /* free slot for custom type casts */

    ov_node *next;
    ov_node *prev;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      ITERATION
 *
 *      ------------------------------------------------------------------------
 */

/**
        This function SHOULD be used with care,
        as it interpretes some pointer as a node.

        Nonetheless it is a pretty convinient function to iterate some
        custom lists based on ov_node inheritance.

        @param node     some struct with a first item of kind ov_node
*/
void *ov_node_next(void *node);

/*----------------------------------------------------------------------------*/

/**
        This function SHOULD be used with care,
        as it interpretes some pointer as a node.

        Nonetheless it is a pretty convinient function to iterate some
        custom lists based on ov_node inheritance.

        @param node     some struct with a first item of kind ov_node
*/
void *ov_node_prev(void *node);

/*
 *      ------------------------------------------------------------------------
 *
 *      ITEM ACCESS
 *
 *      ------------------------------------------------------------------------
 */

/**
        This function SHOULD be used with care,
        as it interpretes some pointer as a node.

        Nonetheless it is a pretty convinient function to get some element
        at pos out of some custom lists based on ov_node inheritance.

        @param head     head of the list
        @param pos      pos of the item (first == one)
*/
void *ov_node_get(void *head, uint64_t pos);

/*----------------------------------------------------------------------------*/

/**
        This function SHOULD be used with care,
        as it interpretes some pointer as a node.

        Nonetheless it is a pretty convinient function to set some element
        at pos out of some custom lists based on ov_node inheritance.

        If the pos is greater as the item count, the function will fail.

        @NOTE may change head if pos == 1

        @param head     head of the list
        @param pos      pos of the item (first == one)
        @param item     item to be set at pos
*/
bool ov_node_set(void **head, uint64_t pos, void *item);

/*----------------------------------------------------------------------------*/

/**
        This function SHOULD be used with care,
        as it interpretes some pointer as a node.

        Nonetheless it is a pretty convinient function to count
        some custom lists based on ov_node inheritance.

        @param head     head of the list
        @returns        0 on error
                        1 if only head is used
                        count of list items
*/
uint64_t ov_node_count(void *head);

/*
 *      ------------------------------------------------------------------------
 *
 *      LIST FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
        Unplug a node from a list,

        @param node     node to unplug
        @param head     head of the list
*/
bool ov_node_unplug(void **head, void *node);

/*----------------------------------------------------------------------------*/

/**
        Count up to the position of node within a list starting at head.
        Head may be some node element, counting is relative from head input.

        @param head     start of the count
        @param node     node to search

        @returns        0 if node is not in list
                        1 if node == head (position 1)
*/
uint64_t ov_node_get_position(const void *head, const void *node);

/*----------------------------------------------------------------------------*/

/**
        Check if a node is included in a list starting at head

        @param head     start of the search
        @param node     node to search
*/
bool ov_node_is_included(const void *head, const void *node);

/*----------------------------------------------------------------------------*/

/**
        Removes a node it it is included in a list.
        @NOTE may change head if node == head

        @param head     start of the search
        @param node     node to search
*/
bool ov_node_remove_if_included(void **head, void *node);

/*----------------------------------------------------------------------------*/

/**
        Push a node at the end of some list.
        @NOTE may change head if head is empty

        @param head     start of the list
        @param node     node to push
*/
bool ov_node_push(void **head, void *node);

/*----------------------------------------------------------------------------*/

/**
        Pop a node from the top of the list
        @NOTE may change head if head is empty

        @NOTE head MUST be the start of the list,
        otherwise the list may become unusable.

        @param head     start of the list
*/
void *ov_node_pop(void **head);

/*----------------------------------------------------------------------------*/

/**
        Push a node to the start of a list
        @NOTE changes head

        @param head     start of the list
        @param node     node to push
*/
bool ov_node_push_front(void **head, void *node);

/*----------------------------------------------------------------------------*/

/**
        Pop a node from the top of the list
        @NOTE may change head if head is empty

        @NOTE head MUST be the start of the list,
        otherwise the list may become unusable.

        @param head     start of the list
*/
void *ov_node_pop_first(void **head);

/*----------------------------------------------------------------------------*/

/**
        Push a node to the end of a list
        @NOTE may change head

        @param head     start of the list
        @param node     node to push
*/
bool ov_node_push_last(void **head, void *node);

/*----------------------------------------------------------------------------*/

/**
        Pop a node from the end of the list
        @NOTE may change head if head is empty

        @NOTE head MUST be the start of the list,
        otherwise the list may become unusable.

        @param head     start of the list
*/
void *ov_node_pop_last(void **head);

/*----------------------------------------------------------------------------*/

/**
        Order some node before next node within a list at head.

        @param head     start of the list
        @param node     node to be inserted
        @param next     next node (node will become prev of node)

        @returns        true on success

        @NOTE           on error the the list will be unchanged.
        @NOTE           this functions checks if next is part of
                        list at head before doing any changes.
*/
bool ov_node_insert_before(void **head, void *node, void *next);

/*----------------------------------------------------------------------------*/

/**
        Order some node after some prev node.

        @param head     start of the list
        @param node     node to be inserted
        @param prev     prev node (node will become prev->next)

        @returns        true on success

        @NOTE           on error the the list will be unchanged.
        @NOTE           this functions checks if prev is part of
                        list at head before doing any changes.
*/
bool ov_node_insert_after(void **head, void *node, void *prev);

#endif /* ov_node_h */
