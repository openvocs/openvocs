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
    @file           ov_linked_list_test.c
    @author         Michael Beer

    @date           2018-07-02

    @ingroup        ov_data_structures

    @brief


    ------------------------------------------------------------------------
    */

#include "../../include/ov_list_test_interface.h"
#include "ov_linked_list.c"

int expand_list_to_position_test() {

  testrun(0 == expand_list_to_position(0, 0));
  testrun(0 == expand_list_to_position(0, 1));
  testrun(0 == expand_list_to_position(0, 3));

  ov_list *list = ov_linked_list_create((ov_list_config){
      .item = ov_data_string_data_functions(),
  });

  LinkedList *ll = AS_LINKED_LIST(list);

  testrun(!expand_list_to_position(NULL, 0));
  testrun(!expand_list_to_position(list, 0));
  testrun(!expand_list_to_position(NULL, 1));

  ListEntity *entity = expand_list_to_position(list, 0);
  testrun(0 == entity);
  testrun(0 == list->count(list));

  for (size_t i = 0; i < 10; i++) {
    testrun(0 == get_entity_at_position(list, i));
  }

  ListEntity *entity_at_0 = expand_list_to_position(list, 1);
  ListEntity *entity_at_1 = expand_list_to_position(list, 2);

  testrun(0 != entity_at_0);
  testrun(&ll->head == entity_at_0);

  for (size_t i = 2; i < 10; i++) {
    testrun(0 == get_entity_at_position(list, i));
  }
  testrun(1 == list->count(list));

  ListEntity *entity_at_3 = expand_list_to_position(list, 4);
  testrun(0 != entity_at_3);
  testrun(0 == entity_at_3->next);
  testrun(3 == list->count(list));

  testrun(NULL == get_entity_at_position(list, 0));
  testrun(entity_at_1 == get_entity_at_position(list, 1));
  testrun(NULL != get_entity_at_position(list, 2));
  testrun(entity_at_3 == get_entity_at_position(list, 3));

  for (size_t i = 4; i < 10; i++) {
    testrun(0 == get_entity_at_position(list, i));
  }

  testrun(entity_at_1 == get_entity_at_position(list, 1));
  testrun(entity_at_1->next == get_entity_at_position(list, 2));
  testrun(entity_at_3 == get_entity_at_position(list, 3));

  testrun(NULL == ov_list_free(list));

  return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int get_entity_at_position_test() {

  char *teststring = strdup("aaa");
  char *teststring2 = strdup("bbb");
  char *teststring3 = strdup("ab1ab123");

  testrun(0 == get_entity_at_position(0, 1));
  testrun(0 == get_entity_at_position(0, 3));
  testrun(0 == get_entity_at_position(0, 4));

  ov_list *list = ov_linked_list_create((ov_list_config){
      .item = ov_data_string_data_functions(),
  });

  testrun(0 == get_entity_at_position(list, 0));
  testrun(0 == get_entity_at_position(list, 1));
  testrun(0 == get_entity_at_position(list, 3));

  testrun(!list->set(list, 0, teststring, NULL));
  testrun(list->set(list, 1, teststring, NULL));

  ListEntity *entity = get_entity_at_position(list, 1);
  testrun(0 != entity);
  testrun(teststring == entity->content);
  testrun(0 == get_entity_at_position(list, 2));
  testrun(0 == get_entity_at_position(list, 3));
  testrun(0 == get_entity_at_position(list, 4));

  testrun(list->set(list, 3, teststring2, NULL));
  entity = get_entity_at_position(list, 1);
  testrun(0 != entity);
  testrun(teststring == entity->content);
  entity = get_entity_at_position(list, 3);
  testrun(0 != entity);
  testrun(teststring2 == entity->content);

  testrun(list->set(list, 4, teststring3, NULL));
  entity = get_entity_at_position(list, 1);
  testrun(0 != entity);
  testrun(teststring == entity->content);
  entity = get_entity_at_position(list, 3);
  testrun(0 != entity);
  testrun(teststring2 == entity->content);
  entity = get_entity_at_position(list, 4);
  testrun(0 != entity);
  testrun(teststring3 == entity->content);

  list = list->free(list);

  return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_ov_linked_list_create() {

  ov_list_default_implementations default_implementations =
      ov_list_get_default_implementations();

  ov_list *list = ov_linked_list_create((ov_list_config){0});
  LinkedList *ll = AS_LINKED_LIST(list);

  testrun(list);
  testrun(ll);

  // check list specific items

  testrun(list->config.item.free == NULL);
  testrun(list->config.item.clear == NULL);
  testrun(list->config.item.copy == NULL);
  testrun(list->config.item.dump == NULL);

  testrun(list->is_empty == impl_linked_list_is_empty);
  testrun(list->clear == impl_linked_list_clear);
  testrun(list->free == impl_linked_list_free);
  testrun(list->copy == default_implementations.copy);
  testrun(list->get == impl_linked_list_get);
  testrun(list->get_pos == impl_linked_list_get_pos);
  testrun(list->set == impl_linked_list_set);
  testrun(list->insert == impl_linked_list_insert);
  testrun(list->remove == impl_linked_list_remove);
  testrun(list->push == impl_linked_list_push);
  testrun(list->pop == impl_linked_list_pop);
  testrun(list->count == impl_linked_list_count);
  testrun(list->for_each == impl_linked_list_for_each);
  testrun(list->iter == impl_linked_list_iter);
  testrun(list->next == impl_linked_list_next);

  // check linked list specific items

  testrun(ll->head.next == NULL);
  testrun(ll->head.last == NULL);
  testrun(ll->head.content == NULL);

  list = list->free(list);
  return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int check_insert() {

  ov_list *list = ov_linked_list_create((ov_list_config){0});

  LinkedList *ll = AS_LINKED_LIST(list);

  void *eins = calloc(1, sizeof(uint64_t));
  void *zwei = calloc(1, sizeof(uint64_t));
  void *drei = calloc(1, sizeof(uint64_t));
  testrun(ov_list_insert(list, 1, drei));
  testrun(ll->head.next->content == drei);
  testrun(ll->head.last->content == drei);
  testrun(ll->head.next->last == NULL);

  testrun(ov_list_insert(list, 1, zwei));
  testrun(ll->head.next->content == zwei);
  testrun(ll->head.last->content == drei);
  testrun(ll->head.next->last == NULL);
  testrun(ll->head.next->next->content == drei);
  testrun(ll->head.next->next->last->content == zwei);

  testrun(ov_list_insert(list, 1, eins));
  testrun(ll->head.next->content == eins);
  testrun(ll->head.last->content == drei);
  testrun(ll->head.next->last == NULL);
  testrun(ll->head.next->next->content == zwei);
  testrun(ll->head.next->next->last->content == eins);
  testrun(ll->head.next->next->next->content == drei);
  testrun(ll->head.next->next->next->last->content == zwei);
  testrun(ll->head.next->next->next->next == NULL);
  testrun(ll->head.next->last == NULL);

  void *item = ov_list_pop(list);
  while (item) {
    free(item);
    item = ov_list_pop(list);
  }

  list = ov_list_free(list);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_linked_list_enable_caching() {

  /* Idea: hard to test, best we can do is perform as many list operations
   * as possible with caching enabled  and check for success */

  /* With default cache size */
  ov_linked_list_enable_caching(0);

  /* Required to use OV_LIST_PERFORM_INTERFACE_TESTS */
  int testrun_counter = 0;
  int result = 0;
  testrun(0 == testrun_counter);

  OV_LIST_PERFORM_INTERFACE_TESTS(ov_linked_list_create);

  ov_registered_cache_free_all();

  /* With small cache */
  ov_linked_list_enable_caching(3);

  OV_LIST_PERFORM_INTERFACE_TESTS(ov_linked_list_create);

  ov_registered_cache_free_all();

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_linked_list_disable_caching() {

  // Nothing to test ...

  return testrun_log_success();
}
/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

  testrun_init();

  testrun_test(test_ov_linked_list_create);

  testrun_test(expand_list_to_position_test);
  testrun_test(get_entity_at_position_test);

  testrun_test(check_insert);

  OV_LIST_PERFORM_INTERFACE_TESTS(ov_linked_list_create);

  testrun_test(test_ov_linked_list_enable_caching);

  return testrun_counter;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST EXECUTION                                                  #EXEC
 *
 *      ------------------------------------------------------------------------
 */

testrun_run(all_tests);
