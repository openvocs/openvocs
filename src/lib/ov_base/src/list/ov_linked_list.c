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
        @file           ov_linked_list.c
        @author         Michael Beer
        @author         Markus Toepfer

        @date           2018-07-09


    ------------------------------------------------------------------------
    */

#include "../../include/ov_utils.h"

#include "../../include/ov_constants.h"
#include "../../include/ov_registered_cache.h"

#include "../../include/ov_linked_list.h"

/******************************************************************************
 *                            Process wide caches
 ******************************************************************************/

/* REMEMBER: Caches are thread-safe ! */
static ov_registered_cache *g_list_cache = 0;
static ov_registered_cache *g_list_entity_cache = 0;

/******************************************************************************
 *                          INTERNAL DATA STRUCTURES
 ******************************************************************************/

typedef struct ListEntity {

  void *content;
  struct ListEntity *next;
  struct ListEntity *last;

} ListEntity;

/*---------------------------------------------------------------------------*/

typedef struct {

  ov_list public;
  ListEntity head;

} LinkedList;

/*----------------------------------------------------------------------------*/

/*
 For each function *must* hold:

        OV_ASSERT((0 == list->head.next) || (0 == list->head.next->last));
        OV_ASSERT((0 == list->head.last) || (0 == list->head.last->next));

On exit!

The list is either empty:

        OV_ASSERT((0 == list->head.next) && (0 == list->head.last));

or

        OV_ASSERT((0 != list->head.next) && (0 != list->head.last));

Each function assumes that the above conditions hold ond function entry!


*/

#define ASSERT_LIST_INVARIANTS(list)                                           \
  OV_ASSERT((0 == list->head.next) || (0 == list->head.next->last));           \
  OV_ASSERT((0 == list->head.last) || (0 == list->head.last->next));           \
  OV_ASSERT(((0 != list->head.next) && (0 != list->head.last)) ||              \
            ((0 == list->head.next) && (0 == list->head.last)));

/*----------------------------------------------------------------------------*/

const uint16_t LINKED_LIST_TYPE = 0x4c4c;

#define AS_LINKED_LIST(x)                                                      \
  (((ov_list_cast(x) != 0) && (LINKED_LIST_TYPE == ((ov_list *)x)->type))      \
       ? (LinkedList *)(x)                                                     \
       : 0)

/******************************************************************************
 *                            release/free methods
 ******************************************************************************/

static void *free_list_entity(void *entity) {

  if (0 != entity)
    free(entity);

  return 0;
}

/*----------------------------------------------------------------------------*/

static void release_list_entity(ListEntity *entity) {

  if (0 == entity) {
    return;
  }

  memset(entity, 0, sizeof(ListEntity));
  entity = ov_registered_cache_put(g_list_entity_cache, entity);

  if (0 != entity) {

    free_list_entity(entity);
    entity = 0;
  }
}

/*----------------------------------------------------------------------------*/

static ListEntity *acquire_list_entity() {

  ListEntity *entity = ov_registered_cache_get(g_list_entity_cache);

  if (0 == entity) {

    entity = calloc(1, sizeof(ListEntity));
  }

  return entity;
}

/*----------------------------------------------------------------------------*/

void release_list(LinkedList *list) {

  if (0 == list)
    return;

  memset(list, 0, sizeof(LinkedList));
  list = ov_registered_cache_put(g_list_cache, list);

  if (0 != list) {

    list = ov_list_free(&list->public);
  }

  OV_ASSERT(0 == list);
}

/******************************************************************************
 *                             INTERNAL FUNCTIONS
 ******************************************************************************/

static ListEntity *get_entity_at_position(const ov_list *list, size_t pos);
/**
 * Runs to list and returns entity at pos - 1.
 * Expands list if list is shorter than pos - 1.
 */
static ListEntity *expand_list_to_position(ov_list *list, size_t pos);

static bool impl_linked_list_is_empty(const ov_list *self);
static bool impl_linked_list_clear(ov_list *self);
static ov_list *impl_linked_list_free(ov_list *self);
static size_t impl_linked_list_get_pos(const ov_list *self, const void *item);
static void *impl_linked_list_get(ov_list *self, size_t pos);
static bool impl_linked_list_set(ov_list *self, size_t pos, void *item,
                                 void **old);
static bool impl_linked_list_insert(ov_list *self, size_t pos, void *item);
static void *impl_linked_list_remove(ov_list *self, size_t pos);
static bool impl_linked_list_push(ov_list *self, void *item);
static void *impl_linked_list_pop(ov_list *self);
static size_t impl_linked_list_count(const ov_list *self);
static bool impl_linked_list_for_each(ov_list *self, void *data,
                                      bool (*function)(void *item, void *data));

static void *impl_linked_list_iter(ov_list *self);
static void *impl_linked_list_next(ov_list *self, void *iter, void **element);

/******************************************************************************
 *                              PUBLIC FUNCTIONS
 ******************************************************************************/

ov_list *ov_linked_list_create(ov_list_config config) {

  ov_list_default_implementations default_implementations =
      ov_list_get_default_implementations();

  LinkedList *list = ov_registered_cache_get(g_list_cache);

  if (0 == list) {
    list = calloc(1, sizeof(LinkedList));
  }

  *list = (LinkedList){
      .public =
          (ov_list){
              .type = LINKED_LIST_TYPE,
              .config = config,
              .is_empty = impl_linked_list_is_empty,
              .clear = impl_linked_list_clear,
              .free = impl_linked_list_free,
              .copy = default_implementations.copy,
              .get_pos = impl_linked_list_get_pos,
              .get = impl_linked_list_get,
              .set = impl_linked_list_set,
              .insert = impl_linked_list_insert,
              .remove = impl_linked_list_remove,
              .push = impl_linked_list_push,
              .pop = impl_linked_list_pop,
              .count = impl_linked_list_count,
              .for_each = impl_linked_list_for_each,
              .iter = impl_linked_list_iter,
              .next = impl_linked_list_next,
          },

      .head = (ListEntity){0},

  };

  return ov_list_set_magic_bytes((ov_list *)list);
}

/******************************************************************************
 *                             PRIVATE FUNCTIONS
 ******************************************************************************/

static inline ListEntity *get_entity_at_position(const ov_list *list,
                                                 size_t pos) {

  const LinkedList *ll = AS_LINKED_LIST(list);

  if (0 == ll || (pos == 0))
    goto no_list_error;

  ASSERT_LIST_INVARIANTS(ll);

  size_t counter = 0;

  ListEntity *entity = ll->head.next;

  while (entity) {

    counter++;

    if (counter == pos)
      break;

    entity = entity->next;
  }

  // pos > as list ?
  if (counter != pos)
    goto error;

  ASSERT_LIST_INVARIANTS(ll);

  return entity;
error:

  ASSERT_LIST_INVARIANTS(ll);

no_list_error:

  return 0;
}

/*---------------------------------------------------------------------------*/

static inline ListEntity *expand_list_to_position(ov_list *self, size_t pos) {

  LinkedList *ll = AS_LINKED_LIST(self);
  if (0 == ll)
    goto error;
  if (0 == pos)
    goto error;

  ListEntity *entity = &ll->head;
  ListEntity *next = entity->next;

  for (size_t i = 1; i < pos; i++) {

    if (!next) {

      // add NULL to list
      if (!self->push(self, NULL))
        goto error;

      entity = ll->head.last;

    } else {

      // walk the list
      entity = next;
      next = entity->next;
    }
  }

  return entity;

error:

  return 0;
}

/*---------------------------------------------------------------------------*/

static bool impl_linked_list_is_empty(const ov_list *self) {

  LinkedList *ll = AS_LINKED_LIST(self);
  if (0 == ll)
    goto error;

  ASSERT_LIST_INVARIANTS(ll);

  if (NULL == ll->head.next)
    return true;
error:

  return false;
}

/*----------------------------------------------------------------------------*/

static bool impl_linked_list_clear(ov_list *self) {

  LinkedList *ll = AS_LINKED_LIST(self);
  if (0 == ll)
    goto no_list_error;

  ASSERT_LIST_INVARIANTS(ll);

  ListEntity *entity = NULL;
  ListEntity *next = ll->head.next;

  while (next) {

    entity = next;
    next = entity->next;

    /* Use configured item free by default. */

    if (self->config.item.free)
      entity->content = self->config.item.free(entity->content);

    release_list_entity(entity);
  }

  ll->head.next = NULL;
  ll->head.last = NULL;

  ASSERT_LIST_INVARIANTS(ll);

  return true;

no_list_error:

  return false;
}

/*---------------------------------------------------------------------------*/

static ov_list *impl_linked_list_free(ov_list *self) {

  LinkedList *ll = AS_LINKED_LIST(self);

  if (0 == ll)
    goto error;

  ASSERT_LIST_INVARIANTS(ll);

  if (!self->clear(self)) {
    /* The list has been screwed up ...
     * no assumptions are possible any more ...*/
    goto error;
  }

  self = ov_registered_cache_put(g_list_cache, self);

  if (0 != self) {

    free(self);
  }

  return NULL;

error:

  return self;
}

/*---------------------------------------------------------------------------*/

static size_t impl_linked_list_get_pos(const ov_list *self, const void *item) {

  LinkedList *ll = AS_LINKED_LIST(self);
  if (!ll)
    goto error;

  ASSERT_LIST_INVARIANTS(ll);

  if (0 == item) {
    goto error;
  }

  size_t count = 1;

  ListEntity *next = ll->head.next;

  while (next) {

    if (next->content == item) {
      ASSERT_LIST_INVARIANTS(ll);
      return count;
    }

    count++;
    next = next->next;
  }

  ASSERT_LIST_INVARIANTS(ll);

error:

  return 0;
}

/*---------------------------------------------------------------------------*/

static void *impl_linked_list_get(ov_list *self, size_t pos) {

  ListEntity *entity = get_entity_at_position(self, pos);
  if (0 == entity)
    goto error;

  return entity->content;

error:

  return 0;
}

/*---------------------------------------------------------------------------*/

static bool impl_linked_list_set(ov_list *self, size_t pos, void *item,
                                 void **old) {

  LinkedList *ll = AS_LINKED_LIST(self);
  if (0 == ll)
    goto no_list_error;

  if (0 == pos)
    goto error;

  ASSERT_LIST_INVARIANTS(ll);

  ListEntity *entity = expand_list_to_position(self, pos);
  if (!entity)
    goto error;

  if (!entity->next)
    return self->push(self, item);

  if (old)
    *old = entity->next->content;

  entity->next->content = item;

  ASSERT_LIST_INVARIANTS(ll);

  return true;

error:

  ASSERT_LIST_INVARIANTS(ll);

no_list_error:

  return false;
}

/*---------------------------------------------------------------------------*/

static bool impl_linked_list_insert(ov_list *self, size_t pos, void *item) {

  LinkedList *ll = AS_LINKED_LIST(self);
  if (!ll || (pos == 0))
    goto no_list_error;

  ASSERT_LIST_INVARIANTS(ll);

  ListEntity *new = NULL;
  ListEntity *entity = expand_list_to_position(self, pos);
  if (!entity)
    goto error;

  new = acquire_list_entity();
  new->last = entity;
  new->next = entity->next;
  new->content = item;

  if (entity->next)
    entity->next->last = new;

  entity->next = new;

  /* Special cases */

  /* If the new element is the last element */
  if (entity == ll->head.last)
    ll->head.last = new;

  /* If the new element is the first element */
  if (entity == &ll->head) {
    new->last = 0;
  }

  /* If new element is the only element in the list,
   * head.next ought to point to it ...*/
  if (0 == ll->head.last) {
    ll->head.last = new;
  }

  ASSERT_LIST_INVARIANTS(ll);

  return true;

error:
  ASSERT_LIST_INVARIANTS(ll);

no_list_error:

  return false;
}

/*---------------------------------------------------------------------------*/

static void *impl_linked_list_remove(ov_list *self, size_t pos) {

  LinkedList *ll = AS_LINKED_LIST(self);

  if (0 == ll)
    goto no_list_error;

  ASSERT_LIST_INVARIANTS(ll);

  ListEntity *entity = get_entity_at_position(self, pos);

  if (0 == entity)
    goto error;

  void *content = entity->content;

  // plug out of list
  if (ll->head.next == entity)
    ll->head.next = entity->next;

  if (ll->head.last == entity)
    ll->head.last = entity->last;

  // plug out of chain
  if (entity->last)
    entity->last->next = entity->next;

  if (entity->next)
    entity->next->last = entity->last;

  release_list_entity(entity);

  ASSERT_LIST_INVARIANTS(ll);

  return content;

error:

  ASSERT_LIST_INVARIANTS(ll);

no_list_error:

  return 0;
}

/*---------------------------------------------------------------------------*/

static bool impl_linked_list_push(ov_list *self, void *item) {

  LinkedList *ll = AS_LINKED_LIST(self);
  if (0 == ll)
    goto error;

  ASSERT_LIST_INVARIANTS(ll);

  ListEntity *new = acquire_list_entity();

  new->content = item;

  if (NULL == ll->head.next) {

    // empty
    ll->head.next = new;
    ll->head.last = new;

  } else {

    ll->head.last->next = new;
    new->last = ll->head.last;
    ll->head.last = new;
  }

  ASSERT_LIST_INVARIANTS(ll);

  return true;

error:

  return false;
}

/*---------------------------------------------------------------------------*/

static void *impl_linked_list_pop(ov_list *self) {

  LinkedList *ll = AS_LINKED_LIST(self);
  if (0 == ll)
    goto no_list_error;

  ASSERT_LIST_INVARIANTS(ll);

  ListEntity *last = ll->head.last;

  if (!last)
    goto error;

  ll->head.last = last->last;
  void *content = last->content;

  if (last->last) {

    last->last->next = NULL;
    release_list_entity(last);

  } else {

    release_list_entity(last);
    ll->head.last = NULL;
    ll->head.next = NULL;
  }

  ASSERT_LIST_INVARIANTS(ll);

  return content;

error:

  ASSERT_LIST_INVARIANTS(ll);

no_list_error:

  return 0;
}

/*---------------------------------------------------------------------------*/

static size_t impl_linked_list_count(const ov_list *self) {

  LinkedList *ll = AS_LINKED_LIST(self);
  if (0 == ll)
    goto no_list_error;

  ASSERT_LIST_INVARIANTS(ll);

  size_t count = 0;
  ListEntity *next = ll->head.next;

  while (next) {

    count++;
    next = next->next;
  }

  ASSERT_LIST_INVARIANTS(ll);

  return count;

no_list_error:

  return 0;
}

/*---------------------------------------------------------------------------*/

static bool impl_linked_list_for_each(ov_list *self, void *data,
                                      bool (*function)(void *item,
                                                       void *data)) {

  LinkedList *ll = AS_LINKED_LIST(self);

  if (0 == ll)
    goto no_list_error;

  ASSERT_LIST_INVARIANTS(ll);

  if (0 == function)
    goto error;

  ListEntity *next = ll->head.next;

  while (next) {

    if (!function(next->content, data))
      goto error;
    next = next->next;
  }

  ASSERT_LIST_INVARIANTS(ll);

  return true;

error:
  ASSERT_LIST_INVARIANTS(ll);

no_list_error:

  return false;
}

/*---------------------------------------------------------------------------*/

void *impl_linked_list_iter(ov_list *self) {

  LinkedList *ll = AS_LINKED_LIST(self);

  if (0 == ll)
    goto error;

  ASSERT_LIST_INVARIANTS(ll);

  return ll->head.next;

error:

  return 0;
}

/*---------------------------------------------------------------------------*/

void *impl_linked_list_next(ov_list *self, void *iter, void **element) {

  LinkedList *ll = AS_LINKED_LIST(self);

  if (0 == ll)
    goto no_list_error;

  ASSERT_LIST_INVARIANTS(ll);

  if (0 == iter)
    goto error;

  // @NOTE cannot easy check if iter is part of the list

  ListEntity *entity = iter;

  if (0 != element)
    *element = entity->content;

  ASSERT_LIST_INVARIANTS(ll);

  return entity->next;

error:

  if (0 != element) {
    *element = 0;
  }

  ASSERT_LIST_INVARIANTS(ll);

no_list_error:

  return 0;
}

/*----------------------------------------------------------------------------*/

static void *linked_list_free(void *vptr) {

  ov_list *ll = ov_list_cast(vptr);

  if (0 == ll)
    goto error;

  if (!ll->clear(ll)) {
    /* The list has been screwed up ...
     * no assumptions are possible any more ...*/
    goto error;
  }

  free(ll);

  return 0;

error:

  return 0;
}

/*----------------------------------------------------------------------------*/

void ov_linked_list_enable_caching(size_t capacity) {

  ov_registered_cache_config cfg = {

      .capacity = capacity,
      .item_free = linked_list_free,

  };

  g_list_cache = ov_registered_cache_extend("linked_list", cfg);

  cfg.item_free = free_list_entity;
}

/*----------------------------------------------------------------------------*/
