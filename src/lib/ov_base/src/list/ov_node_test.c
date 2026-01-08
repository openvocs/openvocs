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
        @file           ov_node_test.c
        @author         Markus Töpfer

        @date           2019-11-28

        @ingroup        ov_node

        @brief          Unit tests of


        ------------------------------------------------------------------------
*/
#include "ov_node.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

struct data {

  ov_node node;
  int value;
};

/*----------------------------------------------------------------------------*/

int check_node_list() {

  struct data item1 = (struct data){.value = 1};
  struct data item2 = (struct data){.value = 2};
  struct data item3 = (struct data){.value = 3};
  struct data item4 = (struct data){.value = 4};
  struct data item5 = (struct data){.value = 5};

  void *head = NULL;

  testrun(ov_node_push(&head, &item1));
  testrun(head == &item1);
  testrun(1 == ov_node_count(head));
  testrun(item1.node.next == NULL);
  testrun(item1.node.prev == NULL);

  testrun(ov_node_push(&head, &item2));
  testrun(head == &item1);
  testrun(((ov_node *)head)->next == (ov_node *)&item2);
  testrun(2 == ov_node_count(head));
  testrun(item1.node.next == (ov_node *)&item2);
  testrun(item1.node.prev == NULL);
  testrun(item2.node.next == NULL);
  testrun(item2.node.prev == (ov_node *)&item1);

  testrun(ov_node_push(&head, &item3));
  testrun(head == &item1);
  testrun(((ov_node *)head)->next == (ov_node *)&item2);
  testrun(((ov_node *)head)->next->next == (ov_node *)&item3);
  testrun(3 == ov_node_count(head));
  testrun(item1.node.next == (ov_node *)&item2);
  testrun(item1.node.prev == NULL);
  testrun(item2.node.next == (ov_node *)&item3);
  testrun(item2.node.prev == (ov_node *)&item1);
  testrun(item3.node.next == NULL);
  testrun(item3.node.prev == (ov_node *)&item2);

  testrun(ov_node_push_front(&head, &item4));
  testrun(head == &item4);
  testrun(((ov_node *)head)->next == (ov_node *)&item1);
  testrun(((ov_node *)head)->next->next == (ov_node *)&item2);
  testrun(4 == ov_node_count(head));
  testrun(item1.node.next == (ov_node *)&item2);
  testrun(item1.node.prev == (ov_node *)&item4);
  testrun(item2.node.next == (ov_node *)&item3);
  testrun(item2.node.prev == (ov_node *)&item1);
  testrun(item3.node.next == NULL);
  testrun(item3.node.prev == (ov_node *)&item2);
  testrun(item4.node.next == (ov_node *)&item1);
  testrun(item4.node.prev == NULL);

  testrun(ov_node_set(&head, 2, &item5));
  testrun(head == &item4);
  testrun(((ov_node *)head)->next == (ov_node *)&item5);
  testrun(((ov_node *)head)->next->next == (ov_node *)&item1);
  testrun(5 == ov_node_count(head));
  testrun(item1.node.next == (ov_node *)&item2);
  testrun(item1.node.prev == (ov_node *)&item5);
  testrun(item2.node.next == (ov_node *)&item3);
  testrun(item2.node.prev == (ov_node *)&item1);
  testrun(item3.node.prev == (ov_node *)&item2);
  testrun(item4.node.next == (ov_node *)&item5);
  testrun(item4.node.prev == NULL);
  testrun(item5.node.next == (ov_node *)&item1);
  testrun(item5.node.prev == (ov_node *)&item4);

  // try to set some item already in a list
  testrun(!ov_node_set(&head, 2, &item5));
  testrun(!ov_node_push(&head, &item1));

  // check nothing changed
  testrun(head == &item4);
  testrun(((ov_node *)head)->next == (ov_node *)&item5);
  testrun(((ov_node *)head)->next->next == (ov_node *)&item1);
  testrun(5 == ov_node_count(head));
  testrun(item1.node.next == (ov_node *)&item2);
  testrun(item1.node.prev == (ov_node *)&item5);
  testrun(item2.node.next == (ov_node *)&item3);
  testrun(item2.node.prev == (ov_node *)&item1);
  testrun(item3.node.prev == (ov_node *)&item2);
  testrun(item4.node.next == (ov_node *)&item5);
  testrun(item4.node.prev == NULL);
  testrun(item5.node.next == (ov_node *)&item1);
  testrun(item5.node.prev == (ov_node *)&item4);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_node_next() {

  struct data item1 = (struct data){.value = 1};
  struct data item2 = (struct data){.value = 2};
  struct data item3 = (struct data){.value = 3};
  struct data item4 = (struct data){.value = 4};
  struct data item5 = (struct data){.value = 5};

  void *head = NULL;
  void *next = NULL;

  testrun(ov_node_push(&head, &item1));
  testrun(ov_node_push(&head, &item2));
  testrun(ov_node_push(&head, &item3));
  testrun(ov_node_push(&head, &item4));
  testrun(ov_node_push(&head, &item5));
  testrun(5 == ov_node_count(head));

  next = ov_node_next(head);
  testrun(next == &item2);
  next = ov_node_next(next);
  testrun(next == &item3);
  next = ov_node_next(next);
  testrun(next == &item4);
  next = ov_node_next(next);
  testrun(next == &item5);
  next = ov_node_next(next);
  testrun(next == NULL);

  next = ov_node_next(&item2);
  testrun(next == &item3);

  next = ov_node_next(&item1);
  testrun(next == &item2);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_node_prev() {

  struct data item1 = (struct data){.value = 1};
  struct data item2 = (struct data){.value = 2};
  struct data item3 = (struct data){.value = 3};
  struct data item4 = (struct data){.value = 4};
  struct data item5 = (struct data){.value = 5};

  void *head = NULL;
  void *next = NULL;

  testrun(ov_node_push(&head, &item1));
  testrun(ov_node_push(&head, &item2));
  testrun(ov_node_push(&head, &item3));
  testrun(ov_node_push(&head, &item4));
  testrun(ov_node_push(&head, &item5));
  testrun(5 == ov_node_count(head));

  testrun(NULL == ov_node_prev(head));
  next = &item5;
  next = ov_node_prev(next);
  testrun(next == &item4);
  next = ov_node_prev(next);
  testrun(next == &item3);
  next = ov_node_prev(next);
  testrun(next == &item2);
  next = ov_node_prev(next);
  testrun(next == &item1);
  next = ov_node_prev(next);
  testrun(next == NULL);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_node_get() {

  struct data item1 = (struct data){.value = 1};
  struct data item2 = (struct data){.value = 2};
  struct data item3 = (struct data){.value = 3};
  struct data item4 = (struct data){.value = 4};
  struct data item5 = (struct data){.value = 5};

  void *head = NULL;

  testrun(ov_node_push(&head, &item1));
  testrun(ov_node_push(&head, &item2));
  testrun(ov_node_push(&head, &item3));
  testrun(ov_node_push(&head, &item4));
  testrun(ov_node_push(&head, &item5));
  testrun(5 == ov_node_count(head));

  testrun(NULL == ov_node_get(head, 0));
  testrun(&item1 == ov_node_get(head, 1));
  testrun(&item2 == ov_node_get(head, 2));
  testrun(&item3 == ov_node_get(head, 3));
  testrun(&item4 == ov_node_get(head, 4));
  testrun(&item5 == ov_node_get(head, 5));
  testrun(NULL == ov_node_get(head, 6));
  testrun(NULL == ov_node_get(head, 7));
  testrun(NULL == ov_node_get(head, 8));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_node_set() {

  struct data item1 = (struct data){.value = 1};
  struct data item2 = (struct data){.value = 2};
  struct data item3 = (struct data){.value = 3};
  struct data item4 = (struct data){.value = 4};
  struct data item5 = (struct data){.value = 5};

  void *head = NULL;

  testrun(!ov_node_set(&head, 0, &item1));
  testrun(!ov_node_set(&head, 2, &item1));
  testrun(!ov_node_set(&head, 3, &item1));

  /*
   *      Set first item
   */

  testrun(ov_node_set(&head, 1, &item1));
  testrun(head == &item1);
  testrun(ov_node_set(&head, 1, &item2));
  testrun(head == &item2);
  testrun(2 == ov_node_count(head));
  testrun(&item2 == ov_node_get(head, 1));
  testrun(&item1 == ov_node_get(head, 2));

  /*
   *      Set behind last item
   */

  testrun(ov_node_set(&head, 3, &item3));
  testrun(head == &item2);
  testrun(3 == ov_node_count(head));
  testrun(&item2 == ov_node_get(head, 1));
  testrun(&item1 == ov_node_get(head, 2));
  testrun(&item3 == ov_node_get(head, 3));

  /*
   *      Set middle
   */

  testrun(ov_node_set(&head, 2, &item4));
  testrun(head == &item2);
  testrun(4 == ov_node_count(head));
  testrun(&item2 == ov_node_get(head, 1));
  testrun(&item4 == ov_node_get(head, 2));
  testrun(&item1 == ov_node_get(head, 3));
  testrun(&item3 == ov_node_get(head, 4));

  testrun(ov_node_set(&head, 3, &item5));
  testrun(head == &item2);
  testrun(5 == ov_node_count(head));
  testrun(&item2 == ov_node_get(head, 1));
  testrun(&item4 == ov_node_get(head, 2));
  testrun(&item5 == ov_node_get(head, 3));
  testrun(&item1 == ov_node_get(head, 4));
  testrun(&item3 == ov_node_get(head, 5));

  testrun(item1.node.next == (ov_node *)&item3);
  testrun(item1.node.prev == (ov_node *)&item5);
  testrun(item2.node.next == (ov_node *)&item4);
  testrun(item2.node.prev == NULL);
  testrun(item3.node.prev == (ov_node *)&item1);
  testrun(item3.node.next == NULL);
  testrun(item4.node.next == (ov_node *)&item5);
  testrun(item4.node.prev == (ov_node *)&item2);
  testrun(item5.node.next == (ov_node *)&item1);
  testrun(item5.node.prev == (ov_node *)&item4);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_node_count() {

  struct data item1 = (struct data){.value = 1};
  struct data item2 = (struct data){.value = 2};
  struct data item3 = (struct data){.value = 3};
  struct data item4 = (struct data){.value = 4};
  struct data item5 = (struct data){.value = 5};

  void *head = NULL;

  testrun(0 == ov_node_count(head));
  testrun(ov_node_push(&head, &item1));
  testrun(1 == ov_node_count(head));
  testrun(ov_node_push(&head, &item2));
  testrun(2 == ov_node_count(head));
  testrun(ov_node_set(&head, 2, &item4));
  testrun(3 == ov_node_count(head));
  testrun(ov_node_push_front(&head, &item3));
  testrun(4 == ov_node_count(head));
  testrun(ov_node_push_last(&head, &item5));
  testrun(5 == ov_node_count(head));

  // check list order

  testrun(&item3 == ov_node_get(head, 1));
  testrun(&item1 == ov_node_get(head, 2));
  testrun(&item4 == ov_node_get(head, 3));
  testrun(&item2 == ov_node_get(head, 4));
  testrun(&item5 == ov_node_get(head, 5));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_node_unplug() {

  struct data item1 = (struct data){.value = 1};
  struct data item2 = (struct data){.value = 2};
  struct data item3 = (struct data){.value = 3};
  struct data item4 = (struct data){.value = 4};
  struct data item5 = (struct data){.value = 5};

  void *head = NULL;

  testrun(ov_node_push(&head, &item1));
  testrun(ov_node_push(&head, &item2));
  testrun(ov_node_push(&head, &item3));
  testrun(ov_node_push(&head, &item4));
  testrun(ov_node_push(&head, &item5));
  testrun(5 == ov_node_count(head));

  testrun(!ov_node_unplug(NULL, NULL));
  testrun(!ov_node_unplug(&head, NULL));
  testrun(!ov_node_unplug(NULL, &item1));

  testrun(&item1 == ov_node_get(head, 1));
  testrun(&item2 == ov_node_get(head, 2));
  testrun(&item3 == ov_node_get(head, 3));
  testrun(&item4 == ov_node_get(head, 4));
  testrun(&item5 == ov_node_get(head, 5));

  /*
   *      Unplug first
   */

  testrun(ov_node_unplug(&head, &item1));
  testrun(4 == ov_node_count(head));
  testrun(&item2 == ov_node_get(head, 1));
  testrun(&item3 == ov_node_get(head, 2));
  testrun(&item4 == ov_node_get(head, 3));
  testrun(&item5 == ov_node_get(head, 4));
  testrun(item1.node.next == NULL);
  testrun(item1.node.prev == NULL);

  /*
   *      Unplug unpluged
   */

  testrun(ov_node_unplug(&head, &item1));
  testrun(4 == ov_node_count(head));
  testrun(&item2 == ov_node_get(head, 1));
  testrun(&item3 == ov_node_get(head, 2));
  testrun(&item4 == ov_node_get(head, 3));
  testrun(&item5 == ov_node_get(head, 4));
  testrun(item1.node.next == NULL);
  testrun(item1.node.prev == NULL);

  /*
   *      Unplug middle
   */

  testrun(ov_node_unplug(&head, &item3));
  testrun(3 == ov_node_count(head));
  testrun(&item2 == ov_node_get(head, 1));
  testrun(&item4 == ov_node_get(head, 2));
  testrun(&item5 == ov_node_get(head, 3));
  testrun(NULL == ov_node_get(head, 4));
  testrun(item3.node.next == NULL);
  testrun(item3.node.prev == NULL);

  /*
   *      Unplug end
   */

  testrun(ov_node_unplug(&head, &item5));
  testrun(2 == ov_node_count(head));
  testrun(&item2 == ov_node_get(head, 1));
  testrun(&item4 == ov_node_get(head, 2));
  testrun(NULL == ov_node_get(head, 3));
  testrun(NULL == ov_node_get(head, 4));
  testrun(item5.node.next == NULL);
  testrun(item5.node.prev == NULL);

  testrun(ov_node_unplug(&head, &item2));
  testrun(1 == ov_node_count(head));
  testrun(&item4 == ov_node_get(head, 1));
  testrun(NULL == ov_node_get(head, 2));
  testrun(NULL == ov_node_get(head, 3));
  testrun(NULL == ov_node_get(head, 4));
  testrun(item2.node.next == NULL);
  testrun(item2.node.prev == NULL);

  testrun(ov_node_unplug(&head, &item4));
  testrun(0 == ov_node_count(head));
  testrun(NULL == ov_node_get(head, 1));
  testrun(NULL == ov_node_get(head, 2));
  testrun(NULL == ov_node_get(head, 3));
  testrun(NULL == ov_node_get(head, 4));
  testrun(item4.node.next == NULL);
  testrun(item4.node.prev == NULL);
  testrun(head == NULL);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_node_get_position() {

  struct data item1 = (struct data){.value = 1};
  struct data item2 = (struct data){.value = 2};
  struct data item3 = (struct data){.value = 3};
  struct data item4 = (struct data){.value = 4};
  struct data item5 = (struct data){.value = 5};

  void *head = NULL;

  testrun(0 == ov_node_get_position(head, &item1));
  testrun(0 == ov_node_get_position(head, &item2));
  testrun(0 == ov_node_get_position(head, &item3));
  testrun(0 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  testrun(ov_node_push(&head, &item1));
  testrun(ov_node_push(&head, &item2));
  testrun(ov_node_push(&head, &item3));
  testrun(ov_node_push(&head, &item4));
  testrun(ov_node_push(&head, &item5));
  testrun(5 == ov_node_count(head));

  testrun(1 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(3 == ov_node_get_position(head, &item3));
  testrun(4 == ov_node_get_position(head, &item4));
  testrun(5 == ov_node_get_position(head, &item5));

  /*
   *      Unplug
   */

  testrun(ov_node_unplug(&head, &item1));
  testrun(0 == ov_node_get_position(head, &item1));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_node_is_included() {

  struct data item1 = (struct data){.value = 1};
  struct data item2 = (struct data){.value = 2};
  struct data item3 = (struct data){.value = 3};
  struct data item4 = (struct data){.value = 4};
  struct data item5 = (struct data){.value = 5};

  void *head = NULL;

  testrun(!ov_node_is_included(head, &item1));
  testrun(!ov_node_is_included(head, &item2));
  testrun(!ov_node_is_included(head, &item3));
  testrun(!ov_node_is_included(head, &item4));
  testrun(!ov_node_is_included(head, &item5));

  testrun(ov_node_push(&head, &item1));
  testrun(ov_node_push(&head, &item2));
  testrun(ov_node_push(&head, &item3));
  testrun(ov_node_push(&head, &item4));
  testrun(ov_node_push(&head, &item5));
  testrun(5 == ov_node_count(head));

  testrun(ov_node_is_included(head, &item1));
  testrun(ov_node_is_included(head, &item2));
  testrun(ov_node_is_included(head, &item3));
  testrun(ov_node_is_included(head, &item4));
  testrun(ov_node_is_included(head, &item5));

  /*
   *      Unplug
   */

  testrun(ov_node_unplug(&head, &item1));
  testrun(!ov_node_is_included(head, &item1));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_node_remove_if_included() {

  struct data item1 = (struct data){.value = 1};
  struct data item2 = (struct data){.value = 2};
  struct data item3 = (struct data){.value = 3};
  struct data item4 = (struct data){.value = 4};
  struct data item5 = (struct data){.value = 5};

  void *head = NULL;

  testrun(ov_node_remove_if_included(&head, &item1));
  testrun(ov_node_remove_if_included(&head, &item2));
  testrun(ov_node_remove_if_included(&head, &item3));
  testrun(ov_node_remove_if_included(&head, &item4));
  testrun(ov_node_remove_if_included(&head, &item5));

  testrun(ov_node_push(&head, &item1));
  testrun(ov_node_remove_if_included(&head, &item2));
  testrun(ov_node_remove_if_included(&head, &item3));
  testrun(ov_node_remove_if_included(&head, &item4));
  testrun(ov_node_remove_if_included(&head, &item5));

  testrun(ov_node_push(&head, &item2));
  testrun(ov_node_push(&head, &item3));
  testrun(ov_node_push(&head, &item4));
  testrun(ov_node_push(&head, &item5));
  testrun(5 == ov_node_count(head));

  testrun(ov_node_remove_if_included(&head, &item3));
  testrun(1 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(0 == ov_node_get_position(head, &item3));
  testrun(3 == ov_node_get_position(head, &item4));
  testrun(4 == ov_node_get_position(head, &item5));
  testrun(4 == ov_node_count(head));
  testrun(head == &item1);

  testrun(ov_node_remove_if_included(&head, &item3));
  testrun(4 == ov_node_count(head));
  testrun(1 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(0 == ov_node_get_position(head, &item3));
  testrun(3 == ov_node_get_position(head, &item4));
  testrun(4 == ov_node_get_position(head, &item5));

  testrun(head == &item1);
  testrun(ov_node_remove_if_included(&head, &item1));
  testrun(head == &item2);
  testrun(0 == ov_node_get_position(head, &item1));
  testrun(1 == ov_node_get_position(head, &item2));
  testrun(0 == ov_node_get_position(head, &item3));
  testrun(2 == ov_node_get_position(head, &item4));
  testrun(3 == ov_node_get_position(head, &item5));
  testrun(3 == ov_node_count(head));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_node_pop() {

  struct data item1 = (struct data){.value = 1};
  struct data item2 = (struct data){.value = 2};
  struct data item3 = (struct data){.value = 3};
  struct data item4 = (struct data){.value = 4};
  struct data item5 = (struct data){.value = 5};

  void *head = NULL;

  testrun(!ov_node_pop(head));
  testrun(ov_node_push(&head, &item1));
  testrun(&item1 == ov_node_pop(&head));
  testrun(0 == ov_node_count(head));
  testrun(NULL == head);
  testrun(item1.node.prev == NULL);
  testrun(item1.node.next == NULL);

  testrun(ov_node_push(&head, &item1));
  testrun(ov_node_push(&head, &item2));
  testrun(ov_node_push(&head, &item3));
  testrun(ov_node_push(&head, &item4));
  testrun(ov_node_push(&head, &item5));
  testrun(1 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(3 == ov_node_get_position(head, &item3));
  testrun(4 == ov_node_get_position(head, &item4));
  testrun(5 == ov_node_get_position(head, &item5));

  testrun(&item1 == ov_node_pop(&head));
  testrun(&item2 == ov_node_pop(&head));
  testrun(&item3 == ov_node_pop(&head));
  testrun(&item4 == ov_node_pop(&head));
  testrun(&item5 == ov_node_pop(&head));
  testrun(head == NULL);
  testrun(NULL == ov_node_pop(&head));
  testrun(NULL == ov_node_pop(&head));
  testrun(NULL == ov_node_pop(&head));
  testrun(NULL == ov_node_pop(&head));
  testrun(NULL == ov_node_pop(&head));
  testrun(item1.node.prev == NULL);
  testrun(item1.node.next == NULL);
  testrun(item2.node.prev == NULL);
  testrun(item2.node.next == NULL);
  testrun(item3.node.prev == NULL);
  testrun(item3.node.next == NULL);
  testrun(item4.node.prev == NULL);
  testrun(item4.node.next == NULL);
  testrun(item5.node.prev == NULL);
  testrun(item5.node.next == NULL);
  testrun(head == NULL);

  testrun(ov_node_push(&head, &item1));
  testrun(ov_node_push(&head, &item2));
  testrun(ov_node_push(&head, &item3));
  testrun(ov_node_push(&head, &item4));
  testrun(ov_node_push(&head, &item5));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_node_pop_first() {

  struct data item1 = (struct data){.value = 1};
  struct data item2 = (struct data){.value = 2};
  struct data item3 = (struct data){.value = 3};
  struct data item4 = (struct data){.value = 4};
  struct data item5 = (struct data){.value = 5};

  void *head = NULL;

  testrun(!ov_node_pop(head));
  testrun(ov_node_push(&head, &item1));
  testrun(&item1 == ov_node_pop_first(&head));
  testrun(0 == ov_node_count(head));
  testrun(NULL == head);
  testrun(item1.node.prev == NULL);
  testrun(item1.node.next == NULL);

  testrun(ov_node_push(&head, &item1));
  testrun(ov_node_push(&head, &item2));
  testrun(ov_node_push(&head, &item3));
  testrun(ov_node_push(&head, &item4));
  testrun(ov_node_push(&head, &item5));
  testrun(1 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(3 == ov_node_get_position(head, &item3));
  testrun(4 == ov_node_get_position(head, &item4));
  testrun(5 == ov_node_get_position(head, &item5));

  testrun(&item1 == ov_node_pop_first(&head));
  testrun(&item2 == ov_node_pop_first(&head));
  testrun(&item3 == ov_node_pop_first(&head));
  testrun(&item4 == ov_node_pop_first(&head));
  testrun(&item5 == ov_node_pop_first(&head));
  testrun(NULL == ov_node_pop_first(&head));
  testrun(NULL == ov_node_pop_first(&head));
  testrun(NULL == ov_node_pop_first(&head));
  testrun(NULL == ov_node_pop_first(&head));
  testrun(NULL == ov_node_pop_first(&head));

  testrun(item1.node.prev == NULL);
  testrun(item1.node.next == NULL);
  testrun(item2.node.prev == NULL);
  testrun(item2.node.next == NULL);
  testrun(item3.node.prev == NULL);
  testrun(item3.node.next == NULL);
  testrun(item4.node.prev == NULL);
  testrun(item4.node.next == NULL);
  testrun(item5.node.prev == NULL);
  testrun(item5.node.next == NULL);
  testrun(head == NULL);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_node_pop_last() {

  struct data item1 = (struct data){.value = 1};
  struct data item2 = (struct data){.value = 2};
  struct data item3 = (struct data){.value = 3};
  struct data item4 = (struct data){.value = 4};
  struct data item5 = (struct data){.value = 5};

  void *head = NULL;

  testrun(!ov_node_pop_last(head));
  testrun(ov_node_push(&head, &item1));
  testrun(&item1 == ov_node_pop_last(&head));
  testrun(0 == ov_node_count(head));
  testrun(NULL == head);
  testrun(item1.node.prev == NULL);
  testrun(item1.node.next == NULL);

  testrun(ov_node_push(&head, &item1));
  testrun(ov_node_push(&head, &item2));
  testrun(ov_node_push(&head, &item3));
  testrun(ov_node_push(&head, &item4));
  testrun(ov_node_push(&head, &item5));
  testrun(1 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(3 == ov_node_get_position(head, &item3));
  testrun(4 == ov_node_get_position(head, &item4));
  testrun(5 == ov_node_get_position(head, &item5));

  testrun(&item5 == ov_node_pop_last(&head));
  testrun(&item4 == ov_node_pop_last(&head));
  testrun(&item3 == ov_node_pop_last(&head));
  testrun(&item2 == ov_node_pop_last(&head));
  testrun(&item1 == ov_node_pop_last(&head));
  testrun(NULL == ov_node_pop_last(&head));
  testrun(NULL == ov_node_pop_last(&head));
  testrun(NULL == ov_node_pop_last(&head));
  testrun(NULL == ov_node_pop_last(&head));
  testrun(NULL == ov_node_pop_last(&head));
  testrun(item1.node.prev == NULL);
  testrun(item1.node.next == NULL);
  testrun(item2.node.prev == NULL);
  testrun(item2.node.next == NULL);
  testrun(item3.node.prev == NULL);
  testrun(item3.node.next == NULL);
  testrun(item4.node.prev == NULL);
  testrun(item4.node.next == NULL);
  testrun(item5.node.prev == NULL);
  testrun(item5.node.next == NULL);
  testrun(head == NULL);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_node_push() {

  struct data item1 = (struct data){.value = 1};
  struct data item2 = (struct data){.value = 2};
  struct data item3 = (struct data){.value = 3};
  struct data item4 = (struct data){.value = 4};
  struct data item5 = (struct data){.value = 5};

  void *head = NULL;

  testrun(ov_node_push(&head, &item1));
  testrun(1 == ov_node_get_position(head, &item1));
  testrun(0 == ov_node_get_position(head, &item2));
  testrun(0 == ov_node_get_position(head, &item3));
  testrun(0 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  testrun(ov_node_push(&head, &item2));
  testrun(1 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(0 == ov_node_get_position(head, &item3));
  testrun(0 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  testrun(ov_node_push(&head, &item3));
  testrun(1 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(3 == ov_node_get_position(head, &item3));
  testrun(0 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  testrun(ov_node_push(&head, &item4));
  testrun(1 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(3 == ov_node_get_position(head, &item3));
  testrun(4 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  testrun(ov_node_push(&head, &item5));
  testrun(1 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(3 == ov_node_get_position(head, &item3));
  testrun(4 == ov_node_get_position(head, &item4));
  testrun(5 == ov_node_get_position(head, &item5));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_node_push_last() {

  struct data item1 = (struct data){.value = 1};
  struct data item2 = (struct data){.value = 2};
  struct data item3 = (struct data){.value = 3};
  struct data item4 = (struct data){.value = 4};
  struct data item5 = (struct data){.value = 5};

  void *head = NULL;

  testrun(ov_node_push_last(&head, &item1));
  testrun(1 == ov_node_get_position(head, &item1));
  testrun(0 == ov_node_get_position(head, &item2));
  testrun(0 == ov_node_get_position(head, &item3));
  testrun(0 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  testrun(ov_node_push_last(&head, &item2));
  testrun(1 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(0 == ov_node_get_position(head, &item3));
  testrun(0 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  testrun(ov_node_push_last(&head, &item3));
  testrun(1 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(3 == ov_node_get_position(head, &item3));
  testrun(0 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  testrun(ov_node_push_last(&head, &item4));
  testrun(1 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(3 == ov_node_get_position(head, &item3));
  testrun(4 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  testrun(ov_node_push_last(&head, &item5));
  testrun(1 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(3 == ov_node_get_position(head, &item3));
  testrun(4 == ov_node_get_position(head, &item4));
  testrun(5 == ov_node_get_position(head, &item5));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_node_push_front() {

  struct data item1 = (struct data){.value = 1};
  struct data item2 = (struct data){.value = 2};
  struct data item3 = (struct data){.value = 3};
  struct data item4 = (struct data){.value = 4};
  struct data item5 = (struct data){.value = 5};

  void *head = NULL;

  testrun(ov_node_push_front(&head, &item1));
  testrun(1 == ov_node_get_position(head, &item1));
  testrun(0 == ov_node_get_position(head, &item2));
  testrun(0 == ov_node_get_position(head, &item3));
  testrun(0 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  testrun(ov_node_push_front(&head, &item2));
  testrun(2 == ov_node_get_position(head, &item1));
  testrun(1 == ov_node_get_position(head, &item2));
  testrun(0 == ov_node_get_position(head, &item3));
  testrun(0 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  testrun(ov_node_push_front(&head, &item3));
  testrun(3 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(1 == ov_node_get_position(head, &item3));
  testrun(0 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  testrun(ov_node_push_front(&head, &item4));
  testrun(4 == ov_node_get_position(head, &item1));
  testrun(3 == ov_node_get_position(head, &item2));
  testrun(2 == ov_node_get_position(head, &item3));
  testrun(1 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  testrun(ov_node_push_front(&head, &item5));
  testrun(5 == ov_node_get_position(head, &item1));
  testrun(4 == ov_node_get_position(head, &item2));
  testrun(3 == ov_node_get_position(head, &item3));
  testrun(2 == ov_node_get_position(head, &item4));
  testrun(1 == ov_node_get_position(head, &item5));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_node_insert_before() {

  struct data item1 = (struct data){.value = 1};
  struct data item2 = (struct data){.value = 2};
  struct data item3 = (struct data){.value = 3};
  struct data item4 = (struct data){.value = 4};
  struct data item5 = (struct data){.value = 5};

  void *head = NULL;

  testrun(ov_node_push_front(&head, &item1));
  testrun(1 == ov_node_get_position(head, &item1));
  testrun(0 == ov_node_get_position(head, &item2));
  testrun(0 == ov_node_get_position(head, &item3));
  testrun(0 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  testrun(!ov_node_insert_before(NULL, NULL, NULL));
  testrun(!ov_node_insert_before(NULL, &item2, &item1));
  testrun(!ov_node_insert_before(&head, NULL, &item1));
  testrun(!ov_node_insert_before(&head, &item2, NULL));

  /*
   *      Insert new item before node at head
   */
  testrun(ov_node_insert_before(&head, &item2, &item1));

  testrun(2 == ov_node_get_position(head, &item1));
  testrun(1 == ov_node_get_position(head, &item2));
  testrun(0 == ov_node_get_position(head, &item3));
  testrun(0 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  /*
   *      Insert self
   */
  testrun(!ov_node_insert_before(&head, &item2, &item2));
  testrun(!ov_node_insert_before(&head, &item1, &item1));

  testrun(2 == ov_node_get_position(head, &item1));
  testrun(1 == ov_node_get_position(head, &item2));
  testrun(0 == ov_node_get_position(head, &item3));
  testrun(0 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  /*
   *      Insert new item before node NOT head
   */
  testrun(ov_node_insert_before(&head, &item3, &item1));

  testrun(3 == ov_node_get_position(head, &item1));
  testrun(1 == ov_node_get_position(head, &item2));
  testrun(2 == ov_node_get_position(head, &item3));
  testrun(0 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  /*
   *      Insert new item before node at head
   */
  testrun(ov_node_insert_before(&head, &item4, &item2));

  testrun(4 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(3 == ov_node_get_position(head, &item3));
  testrun(1 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  /*
   *      Insert item already ordered before
   */
  testrun(ov_node_insert_before(&head, &item2, &item1));
  testrun(4 == ov_node_get_position(head, &item1));
  testrun(3 == ov_node_get_position(head, &item2));
  testrun(2 == ov_node_get_position(head, &item3));
  testrun(1 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  testrun(ov_node_insert_before(&head, &item4, &item1));
  testrun(4 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(1 == ov_node_get_position(head, &item3));
  testrun(3 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  /*
   *      Insert after item to head
   */
  testrun(ov_node_insert_before(&head, &item1, &item2));

  testrun(2 == ov_node_get_position(head, &item1));
  testrun(3 == ov_node_get_position(head, &item2));
  testrun(1 == ov_node_get_position(head, &item3));
  testrun(4 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  /*
   *      Insert after item to head
   */
  testrun(ov_node_insert_before(&head, &item2, &item3));

  testrun(3 == ov_node_get_position(head, &item1));
  testrun(1 == ov_node_get_position(head, &item2));
  testrun(2 == ov_node_get_position(head, &item3));
  testrun(4 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  /*
   *      Insert item lase
   */
  testrun(ov_node_insert_before(&head, &item5, &item4));

  testrun(3 == ov_node_get_position(head, &item1));
  testrun(1 == ov_node_get_position(head, &item2));
  testrun(2 == ov_node_get_position(head, &item3));
  testrun(5 == ov_node_get_position(head, &item4));
  testrun(4 == ov_node_get_position(head, &item5));

  /*
   *      Check ordering
   */

  while (head)
    ov_node_pop(&head);

  testrun(ov_node_push_front(&head, &item3));
  testrun(ov_node_push_front(&head, &item4));
  testrun(ov_node_push_front(&head, &item1));
  testrun(ov_node_push_front(&head, &item2));
  testrun(ov_node_push_front(&head, &item5));

  testrun(3 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(5 == ov_node_get_position(head, &item3));
  testrun(4 == ov_node_get_position(head, &item4));
  testrun(1 == ov_node_get_position(head, &item5));

  struct data *node = NULL;
  struct data *next = head;
  struct data *test = NULL;

  /*
   *      We perform some list ordering,
   *      without inline rewalk.
   *
   *      This algorithm will order a list
   *      descending.
   *
   *      Insertion sort.
   */

  while (next) {

    node = next;
    next = ov_node_next(next);

    test = head;
    while (test) {

      if (test == node)
        break;

      if (test->value < node->value) {
        ov_node_insert_before(&head, node, test);
        break;
      }

      test = ov_node_next(test);
    }
  }

  next = head;
  while (next) {
    testrun_log("ordered descending %i", next->value);
    next = ov_node_next(next);
  }

  testrun(5 == ov_node_get_position(head, &item1));
  testrun(4 == ov_node_get_position(head, &item2));
  testrun(3 == ov_node_get_position(head, &item3));
  testrun(2 == ov_node_get_position(head, &item4));
  testrun(1 == ov_node_get_position(head, &item5));

  /*
   *      We perform some list ordering,
   *      without inline rewalk.
   *
   *      This algorithm will order a list
   *      ascending.
   */

  next = head;
  while (next) {

    node = next;
    next = ov_node_next(next);

    test = head;
    while (test) {

      if (test == node)
        break;

      if (test->value < node->value) {

        test = ov_node_next(test);

      } else {
        ov_node_insert_before(&head, node, test);
        break;
      }
    }
  }

  testrun_log("------------");

  next = head;
  while (next) {
    testrun_log("ordered ascending %i", next->value);
    next = ov_node_next(next);
  }

  testrun(1 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(3 == ov_node_get_position(head, &item3));
  testrun(4 == ov_node_get_position(head, &item4));
  testrun(5 == ov_node_get_position(head, &item5));

  ov_node_pop(&head);

  testrun(0 == ov_node_get_position(head, &item1));
  testrun(1 == ov_node_get_position(head, &item2));
  testrun(2 == ov_node_get_position(head, &item3));
  testrun(3 == ov_node_get_position(head, &item4));
  testrun(4 == ov_node_get_position(head, &item5));

  testrun(!ov_node_insert_before(&head, &item5, &item1));

  testrun(0 == ov_node_get_position(head, &item1));
  testrun(1 == ov_node_get_position(head, &item2));
  testrun(2 == ov_node_get_position(head, &item3));
  testrun(3 == ov_node_get_position(head, &item4));
  testrun(4 == ov_node_get_position(head, &item5));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_node_insert_after() {

  struct data item1 = (struct data){.value = 1};
  struct data item2 = (struct data){.value = 2};
  struct data item3 = (struct data){.value = 3};
  struct data item4 = (struct data){.value = 4};
  struct data item5 = (struct data){.value = 5};

  void *head = NULL;

  testrun(ov_node_push_front(&head, &item1));
  testrun(1 == ov_node_get_position(head, &item1));
  testrun(0 == ov_node_get_position(head, &item2));
  testrun(0 == ov_node_get_position(head, &item3));
  testrun(0 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  testrun(!ov_node_insert_after(NULL, NULL, NULL));
  testrun(!ov_node_insert_after(NULL, &item2, &item1));
  testrun(!ov_node_insert_after(&head, NULL, &item1));
  testrun(!ov_node_insert_after(&head, &item2, NULL));

  /*
   *      Insert new item after head
   */
  testrun(ov_node_insert_after(&head, &item2, &item1));

  testrun(1 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(0 == ov_node_get_position(head, &item3));
  testrun(0 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  /*
   *      Insert self
   */
  testrun(!ov_node_insert_before(&head, &item2, &item2));
  testrun(!ov_node_insert_before(&head, &item1, &item1));

  /*
   *      Insert new item after head
   */
  testrun(ov_node_insert_after(&head, &item3, &item1));

  testrun(1 == ov_node_get_position(head, &item1));
  testrun(3 == ov_node_get_position(head, &item2));
  testrun(2 == ov_node_get_position(head, &item3));
  testrun(0 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  /*
   *      Insert new item after end
   */
  testrun(ov_node_insert_after(&head, &item4, &item2));

  testrun(1 == ov_node_get_position(head, &item1));
  testrun(3 == ov_node_get_position(head, &item2));
  testrun(2 == ov_node_get_position(head, &item3));
  testrun(4 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  /*
   *      Unplug and insert existing item
   */
  testrun(ov_node_insert_after(&head, &item2, &item4));

  testrun(1 == ov_node_get_position(head, &item1));
  testrun(4 == ov_node_get_position(head, &item2));
  testrun(2 == ov_node_get_position(head, &item3));
  testrun(3 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  /*
   *      Unplug and insert head
   */
  testrun(ov_node_insert_after(&head, &item1, &item4));

  testrun(3 == ov_node_get_position(head, &item1));
  testrun(4 == ov_node_get_position(head, &item2));
  testrun(1 == ov_node_get_position(head, &item3));
  testrun(2 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  /*
   *      Unplug and insert end
   */
  testrun(ov_node_insert_after(&head, &item2, &item3));

  testrun(4 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(1 == ov_node_get_position(head, &item3));
  testrun(3 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  /*
   *      insert behind item not in list
   */
  testrun(!ov_node_insert_after(&head, &item2, &item5));

  testrun(4 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(1 == ov_node_get_position(head, &item3));
  testrun(3 == ov_node_get_position(head, &item4));
  testrun(0 == ov_node_get_position(head, &item5));

  /*
   *      insert item
   */
  testrun(ov_node_insert_after(&head, &item5, &item1));

  testrun(4 == ov_node_get_position(head, &item1));
  testrun(2 == ov_node_get_position(head, &item2));
  testrun(1 == ov_node_get_position(head, &item3));
  testrun(3 == ov_node_get_position(head, &item4));
  testrun(5 == ov_node_get_position(head, &item5));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

  testrun_init();
  testrun_test(check_node_list);

  testrun_test(test_ov_node_next);
  testrun_test(test_ov_node_prev);

  testrun_test(test_ov_node_get);
  testrun_test(test_ov_node_set);

  testrun_test(test_ov_node_count);

  testrun_test(test_ov_node_unplug);
  testrun_test(test_ov_node_get_position);

  testrun_test(test_ov_node_is_included);
  testrun_test(test_ov_node_remove_if_included);

  testrun_test(test_ov_node_pop);
  testrun_test(test_ov_node_pop_first);
  testrun_test(test_ov_node_pop_last);

  testrun_test(test_ov_node_push);
  testrun_test(test_ov_node_push_last);
  testrun_test(test_ov_node_push_front);

  testrun_test(test_ov_node_insert_before);
  testrun_test(test_ov_node_insert_after);

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
