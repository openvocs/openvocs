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
        @file           ov_vector_list_test.c
        @author         Markus Toepfer

        @date           2018-08-09

        @ingroup        ov_data_structures

        @brief          Unit tests of ov_vector_list.


        ------------------------------------------------------------------------
*/

#include "../../include/ov_list_test_interface.h"

#include "../../include/ov_linked_list.h"
#include "ov_vector_list.c"

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_vector_list_create() {

    ov_list_default_implementations default_implementations =
        ov_list_get_default_implementations();

    ov_list *list = ov_vector_list_create((ov_list_config){0});
    VectorList *vector = AS_VECTOR_LIST(list);

    testrun(list);
    testrun(vector);

    // check list specific items

    testrun(list->config.item.free == NULL);
    testrun(list->config.item.clear == NULL);
    testrun(list->config.item.copy == NULL);
    testrun(list->config.item.dump == NULL);

    testrun(list->is_empty == impl_vector_list_is_empty);
    testrun(list->clear == impl_vector_list_clear);
    testrun(list->free == impl_vector_list_free);
    testrun(list->copy == default_implementations.copy);
    testrun(list->get == impl_vector_list_get);
    testrun(list->get_pos == impl_vector_list_get_pos);
    testrun(list->set == impl_vector_list_set);
    testrun(list->insert == impl_vector_list_insert);
    testrun(list->remove == impl_vector_list_remove);
    testrun(list->push == impl_vector_list_push);
    testrun(list->pop == impl_vector_list_pop);
    testrun(list->count == impl_vector_list_count);
    testrun(list->for_each == impl_vector_list_for_each);
    testrun(list->iter == impl_vector_list_iter);
    testrun(list->next == impl_vector_list_next);

    // check vector specific items

    testrun(vector->config.shrink == false);
    testrun(vector->config.rate == VECTOR_DEFAULT_SIZE);
    testrun(vector->last == 0);
    testrun(vector->size == VECTOR_DEFAULT_SIZE);
    testrun(vector->items != NULL);

    list = list->free(list);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vector_list_set_shrink() {

    ov_list *list = ov_linked_list_create((ov_list_config){0});
    VectorList *vector = AS_VECTOR_LIST(list);

    testrun(list);
    testrun(!vector);
    testrun(!ov_vector_list_set_shrink(NULL, true));
    testrun(!ov_vector_list_set_shrink(list, true));
    testrun(!ov_vector_list_set_shrink(list, false));
    list = list->free(list);

    list = ov_vector_list_create((ov_list_config){0});
    testrun(list);
    vector = AS_VECTOR_LIST(list);
    testrun(vector);
    testrun(vector->config.shrink == false);
    testrun(ov_vector_list_set_shrink(list, true));
    testrun(vector->config.shrink == true);
    testrun(ov_vector_list_set_shrink(list, false));
    testrun(vector->config.shrink == false);
    list = list->free(list);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vector_list_set_rate() {

    ov_list *list = ov_linked_list_create((ov_list_config){0});
    VectorList *vector = AS_VECTOR_LIST(list);

    testrun(list);
    testrun(!vector);
    testrun(!ov_vector_list_set_rate(NULL, 0));
    testrun(!ov_vector_list_set_rate(list, 0));
    testrun(!ov_vector_list_set_rate(list, 100));
    list = list->free(list);

    list = ov_vector_list_create((ov_list_config){0});
    testrun(list);
    vector = AS_VECTOR_LIST(list);
    testrun(vector);
    testrun(vector->config.rate == VECTOR_DEFAULT_SIZE);
    testrun(!ov_vector_list_set_rate(list, 0));
    testrun(vector->config.rate == VECTOR_DEFAULT_SIZE);
    testrun(ov_vector_list_set_rate(list, 1));
    testrun(vector->config.rate == 1);
    list = list->free(list);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int testing_vector_resize() {

    ov_list *list = ov_vector_list_create((ov_list_config){0});
    VectorList *vector = AS_VECTOR_LIST(list);

    testrun(list);
    testrun(vector);

    testrun(!vector_resize(NULL, 0));
    testrun(!vector_resize(vector, 0));
    testrun(!vector_resize(NULL, 100));

    testrun(vector_resize(vector, 101));
    testrun(vector->last == 0);
    testrun(vector->size == 100);
    testrun(vector->items);
    testrun(vector_resize(vector, 100));
    testrun(vector->last == 0);
    testrun(vector->size == 99);
    testrun(vector->items);
    testrun(vector_resize(vector, 110));
    testrun(vector->last == 0);
    testrun(vector->size == 109);
    testrun(vector->items);

    testrun(vector_resize(vector, 10));
    testrun(vector->last == 0);
    testrun(vector->size == 9);
    testrun(vector->items);
    testrun(vector_resize(vector, 9));
    testrun(vector->last == 0);
    testrun(vector->size == 8);
    testrun(vector->items);
    testrun(vector_resize(vector, 8));
    testrun(vector->last == 0);
    testrun(vector->size == 7);
    testrun(vector->items);

    testrun(vector_resize(vector, 99));
    testrun(vector->last == 0);
    testrun(vector->size == 98);
    testrun(vector->items);

    vector->last = 70;
    testrun(!vector_resize(vector, 71));
    testrun(vector_resize(vector, 72));
    testrun(vector->last == 70);
    testrun(vector->size == 71);
    testrun(vector->items);
    testrun(vector_resize(vector, 72));
    testrun(vector->last == 70);
    testrun(vector->size == 71);
    testrun(vector->items);

    void *items = vector->items;
    vector->items = NULL;
    testrun(!vector_resize(vector, 123));
    vector->items = items;
    testrun(vector_resize(vector, 123));
    testrun(vector->last == 70);
    testrun(vector->size == 122);
    testrun(vector->items);

    list = list->free(list);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int testing_vector_auto_adjust() {

    ov_list *list = ov_vector_list_create((ov_list_config){0});
    VectorList *vector = AS_VECTOR_LIST(list);

    testrun(list);
    testrun(vector);

    testrun(!vector_auto_adjust(NULL));

    testrun(vector->last == 0);
    testrun(vector->size == VECTOR_DEFAULT_SIZE);
    testrun(vector->config.rate == VECTOR_DEFAULT_SIZE);

    testrun(vector_auto_adjust(vector));
    testrun(vector->last == 0);
    testrun(vector->size == VECTOR_DEFAULT_SIZE);
    testrun(vector->config.rate == VECTOR_DEFAULT_SIZE);
    testrun(vector->config.shrink == false);

    vector->last = 98;
    testrun(vector_auto_adjust(vector));
    testrun(vector->last == 98);
    testrun(vector->size == VECTOR_DEFAULT_SIZE);
    testrun(vector->config.rate == VECTOR_DEFAULT_SIZE);
    vector->last = 99;
    testrun(vector_auto_adjust(vector));
    testrun(vector->last == 99);
    testrun(vector->size == VECTOR_DEFAULT_SIZE + vector->config.rate);
    testrun(vector->config.rate == VECTOR_DEFAULT_SIZE);
    vector->last = 100;
    testrun(vector_auto_adjust(vector));
    testrun(vector->last == 100);
    testrun(vector->size == VECTOR_DEFAULT_SIZE + vector->config.rate);
    testrun(vector->config.rate == VECTOR_DEFAULT_SIZE);
    testrun(vector->config.shrink == false);

    vector->last = 98;
    testrun(vector_auto_adjust(vector));
    testrun(vector->last == 98);
    testrun(vector->size == VECTOR_DEFAULT_SIZE + vector->config.rate);
    testrun(vector->config.rate == VECTOR_DEFAULT_SIZE);
    testrun(vector->config.shrink == false);

    vector->config.shrink = true;
    testrun(vector_auto_adjust(vector));
    testrun(vector->last == 98);
    testrun(vector->size == 198);
    testrun(vector->config.rate == VECTOR_DEFAULT_SIZE);
    testrun(vector->config.shrink == true);

    vector->config.rate = 1;
    vector->size = 100;
    testrun(vector_auto_adjust(vector));
    testrun(vector->last == 98);
    testrun(vector->size == 99);
    testrun(vector->config.rate == 1);
    testrun(vector->config.shrink == true);
    testrun(vector_auto_adjust(vector));
    testrun(vector->last == 98);
    testrun(vector->size == 100);
    testrun(vector->config.rate == 1);
    testrun(vector->config.shrink == true);
    vector->last = 99;
    testrun(vector_auto_adjust(vector));
    testrun(vector->last == 99);
    testrun(vector->size == 101);
    testrun(vector->config.rate == 1);
    testrun(vector->config.shrink == true);
    vector->last = 70;
    testrun(vector_auto_adjust(vector));
    testrun(vector->last == 70);
    testrun(vector->size == 71);
    testrun(vector->config.rate == 1);
    testrun(vector->config.shrink == true);

    list = list->free(list);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int testing_vector_list_is_empty() {

    ov_list *list = ov_vector_list_create((ov_list_config){0});
    VectorList *vector = AS_VECTOR_LIST(list);

    testrun(list);
    testrun(vector);

    testrun(!vector_list_is_empty(NULL));
    testrun(vector_list_is_empty(vector));

    vector->last = 1;
    testrun(!vector_list_is_empty(vector));
    vector->last = 0;
    testrun(vector_list_is_empty(vector));

    // ignore 0
    vector->items[0] = vector;
    testrun(vector_list_is_empty(vector));

    list = list->free(list);
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

    testrun_test(test_ov_vector_list_set_shrink);
    testrun_test(test_ov_vector_list_set_rate);

    testrun_test(testing_vector_resize);
    testrun_test(testing_vector_auto_adjust);
    testrun_test(testing_vector_list_is_empty);

    OV_LIST_PERFORM_INTERFACE_TESTS(ov_vector_list_create);

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
