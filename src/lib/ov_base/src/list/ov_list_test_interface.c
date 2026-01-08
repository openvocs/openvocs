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
        @file           ov_list_test_interface.c
        @author         Michael Beer

        @date           2018-07-09

        @ingroup        ov_basics

        @brief


    ------------------------------------------------------------------------
    */

#include "../../include/ov_list_test_interface.h"

/******************************************************************************
 *                           FAKE testrun interface
 ******************************************************************************/

/*---------------------------------------------------------------------------*/

ov_list *(*list_creator)(ov_list_config);

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST HELPER                                                     #HELPER
 *
 *      ------------------------------------------------------------------------
 */

bool count_items(void *item, void *data) {

    if (item) { /*unused */
    };

    size_t *counter = data;
    *counter = *counter + 1;

    return true;
}

/*---------------------------------------------------------------------------*/

typedef struct {

    char **teststrings;
    size_t counter;
    size_t num_strings;

} check_teststring_arg;

bool check_teststrings(void *item, void *data) {

    check_teststring_arg *cta = data;

    char *string = item;

    if (0 == string)
        return false;

    if ((0 != cta->teststrings) && (cta->counter < cta->num_strings) &&
        (0 != strcmp(cta->teststrings[cta->counter], string))) {
        return false;
    }

    ++cta->counter;

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_impl_list_is_empty() {

    ov_list *list = list_creator((ov_list_config){
        .item = ov_data_string_data_functions(),
    });

    testrun(list->is_empty);
    testrun(list->is_empty(list));

    char *str1 = "content1";
    char *str2 = "content2";
    char *str3 = "content3";

    testrun(0 == list->count(list));
    testrun(list->is_empty(list));

    list->push(list, str1);
    list->push(list, str2);
    list->push(list, str3);
    testrun(3 == list->count(list));
    testrun(!list->is_empty(list));

    testrun(list->pop(list));
    testrun(!list->is_empty(list));
    testrun(list->pop(list));
    testrun(!list->is_empty(list));
    testrun(list->pop(list));
    testrun(list->is_empty(list));

    list->free(list);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_list_clear() {

    ov_list *list = list_creator((ov_list_config){
        .item = ov_data_string_data_functions(),
    });

    testrun(list->config.item.free == ov_data_string_free);
    testrun(list->config.item.clear == ov_data_string_clear);
    testrun(list->config.item.copy == ov_data_string_copy);
    testrun(list->config.item.dump == ov_data_string_dump);

    char *str1 = strdup("content1");
    char *str2 = strdup("content2");
    char *str3 = strdup("content3");

    testrun(list->clear);

    testrun(0 == list->count(list));
    testrun(!list->clear(NULL));
    testrun(list->clear(list));
    testrun(0 == list->count(list));

    list->push(list, str1);
    list->push(list, str2);
    list->push(list, str3);
    testrun(3 == list->count(list));

    // NOT a list
    list->magic_byte = OV_LIST_MAGIC_BYTE | 0x1234;
    testrun(!list->clear(list));
    list->magic_byte = OV_LIST_MAGIC_BYTE;

    testrun(3 == list->count(list));
    testrun(list->config.item.free == ov_data_string_free);
    testrun(list->clear(list));
    testrun(0 == list->count(list));

    // check content is freed with valgrind

    list->config.item.free = NULL;
    list->push(list, "non allocated content");
    list->push(list, "non allocated content");
    list->push(list, "non allocated content");
    testrun(3 == list->count(list));
    testrun(list->clear(list));
    testrun(0 == list->count(list));

    list->free(list);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_list_free() {

    ov_list *list = list_creator((ov_list_config){
        .item = ov_data_string_data_functions(),
    });

    testrun(list->free);

    char *str1 = strdup("content1");
    char *str2 = strdup("content2");
    char *str3 = strdup("content3");

    testrun(0 == list->count(list));
    testrun(NULL == list->free(NULL));
    testrun(NULL == list->free(list));

    list = list_creator((ov_list_config){
        .item = ov_data_string_data_functions(),
    });

    list->push(list, str1);
    list->push(list, str2);
    list->push(list, str3);
    testrun(3 == list->count(list));

    // NOT a list
    list->magic_byte = OV_LIST_MAGIC_BYTE | 0x1234;
    testrun(list == list->free(list));
    list->magic_byte = OV_LIST_MAGIC_BYTE;

    testrun(3 == list->count(list));
    testrun(NULL == list->free(list));

    // check content is freed with valgrind

    list = list_creator((ov_list_config){0});
    list->push(list, "non allocated content");
    list->push(list, "non allocated content");
    list->push(list, "non allocated content");
    testrun(3 == list->count(list));
    testrun(NULL == list->free(list));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_list_copy() {

    ov_list *list = list_creator((ov_list_config){
        .item = ov_data_string_data_functions(),
    });

    testrun(list->copy);

    ov_list *copy = 0;

    testrun(0 == list->copy(0, 0));

    copy = list->copy(list, 0);
    testrun(0 == copy);

    copy = list_creator((ov_list_config){
        .item = ov_data_string_data_functions(),
    });

    ov_list *copy_2 = list->copy(list, copy);
    testrun(0 != copy);
    testrun(copy == copy_2);
    testrun(copy_2->count(copy_2) == copy->count(copy));
    copy_2 = 0;

    char *teststring = strdup("aaa");
    char *teststring2 = strdup("bbb");
    char *teststring3 = strdup("cccc");
    char *teststring4 = strdup("xyz");
    char *teststring5 = strdup("abc");

    testrun(list->push(list, teststring));
    testrun(list->push(list, 0));
    testrun(list->push(list, teststring4));
    testrun(list->push(list, teststring2));
    testrun(list->push(list, teststring5));
    testrun(list->push(list, 0));
    testrun(list->push(list, teststring3));

    list->copy(list, copy);

    while (list->count(list)) {

        char *s1 = list->pop(list);
        char *s2 = copy->pop(copy);

        testrun(((0 == s1) && (0 == s2)) || (0 == strcmp(s1, s2)));

        if (0 != s2)
            free(s2);
        if (0 != s1)
            free(s1);
    }

    testrun(0 == list->free(list));
    testrun(0 == copy->free(copy));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_list_set() {

    ov_list *list = list_creator((ov_list_config){
        .item = ov_data_string_data_functions(),
    });

    testrun(list->set);

    char *str1 = strdup("aaa");
    char *str2 = strdup("bbb");
    char *str3 = strdup("ab1ab123");

    void *old = NULL;

    testrun(false == list->set(0, 0, str1, NULL));
    testrun(false == list->set(list, 0, str1, NULL));
    testrun(true == list->set(list, 1, str1, NULL));
    testrun(true == list->set(list, 1, str1, &old));
    testrun(old == str1);
    testrun(str1 == list->get(list, 1));
    old = NULL;
    testrun(true == list->set(list, 1, str2, &old));
    testrun(old == str1);
    testrun(str2 == list->get(list, 1));
    testrun(true == list->set(list, 2, str1, NULL));
    testrun(str1 == list->get(list, 2));
    testrun(true == list->set(list, 11, str3, NULL));
    testrun(str3 == list->get(list, 11));

    for (size_t i = 3; i < 11; ++i) {

        testrun(0 == list->get(list, i));
    }
    testrun(list->get(list, 1) == str2);
    testrun(list->get(list, 2) == str1);
    testrun(list->get(list, 11) == str3);

    list = list->free(list);
    testrun(0 == list);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_list_get_pos() {

    ov_list *list = list_creator((ov_list_config){
        .item = ov_data_string_data_functions(),
    });

    testrun(list);
    testrun(list->push);
    testrun(list->get_pos);

    char *str1 = strdup("aaa");
    char *str2 = strdup("bbb");
    char *str3 = strdup("ab1ab123");

    testrun(0 == list->get_pos(list, str1));
    testrun(0 == list->get_pos(list, str2));
    testrun(0 == list->get_pos(list, str3));

    testrun(list->push(list, str1));
    testrun(1 == list->get_pos(list, str1));
    testrun(0 == list->get_pos(list, str2));
    testrun(0 == list->get_pos(list, str3));

    testrun(list->push(list, str2));
    testrun(1 == list->get_pos(list, str1));
    testrun(2 == list->get_pos(list, str2));
    testrun(0 == list->get_pos(list, str3));

    testrun(list->push(list, str3));
    testrun(1 == list->get_pos(list, str1));
    testrun(2 == list->get_pos(list, str2));
    testrun(3 == list->get_pos(list, str3));

    list = list->free(list);
    testrun(0 == list);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_list_get() {

    ov_list *list = list_creator((ov_list_config){
        .item = ov_data_string_data_functions(),
    });

    testrun(list);
    testrun(list->get);

    char *str1 = strdup("aaa");
    char *str2 = strdup("bbb");
    char *str3 = strdup("ab1ab123");

    testrun(0 == list->get(0, 0));
    testrun(0 == list->get(list, 0));

    testrun(true == list->set(list, 1, str1, NULL));
    testrun(str1 == list->get(list, 1));
    testrun(true == list->set(list, 1, str2, NULL));
    testrun(str2 == list->get(list, 1));
    testrun(true == list->set(list, 2, str1, NULL));
    testrun(str1 == list->get(list, 2));
    testrun(true == list->set(list, 11, str3, NULL));
    testrun(str3 == list->get(list, 11));

    for (size_t i = 3; i < 11; ++i) {

        testrun(0 == list->get(list, i));
    }
    testrun(str2 == list->get(list, 1));
    testrun(str1 == list->get(list, 2));
    testrun(str3 == list->get(list, 11));

    list = list->free(list);
    testrun(0 == list);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_list_insert() {

    ov_list *list = list_creator((ov_list_config){
        .item = ov_data_string_data_functions(),
    });

    testrun(list->insert);

    char *teststring = strdup("aaa");
    char *teststring2 = strdup("bbb");
    char *teststring3 = strdup("ccc");
    char *teststring4 = strdup("xyz");

    testrun(!list->insert(0, 1, teststring));

    /* insert at beginning of list - replacement for 'enqueue_head()' */

    testrun(list->insert(list, 1, teststring));
    testrun(list->get(list, 1) == teststring);
    testrun(1 == list->count(list));

    testrun(list->insert(list, 1, teststring2));
    testrun(2 == list->count(list));

    testrun(list->get(list, 1) == teststring2);
    testrun(list->get(list, 2) == teststring);

    /* Insert before existing element */

    testrun(list->insert(list, 2, teststring3));
    testrun(3 == list->count(list));

    testrun(list->get(list, 1) == teststring2);
    testrun(list->get(list, 2) == teststring3);
    testrun(list->get(list, 3) == teststring);

    /* Insert after last element */

    testrun(list->insert(list, 4, teststring4));
    testrun(4 == list->count(list));

    testrun(list->get(list, 0) == NULL);
    testrun(list->get(list, 1) == teststring2);
    testrun(list->get(list, 2) == teststring3);
    testrun(list->get(list, 3) == teststring);
    testrun(list->get(list, 4) == teststring4);

    list = list->free(list);
    testrun(0 == list);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_list_remove() {

    ov_list *list = list_creator((ov_list_config){0});

    testrun(list->remove);

    char *teststring = strdup("aaa");
    char *teststring2 = strdup("bbb");
    char *teststring3 = strdup("ab1ab123");

    testrun(0 == list->remove(0, 0));

    testrun(list->insert(list, 1, teststring));
    testrun(list->get(list, 1) == teststring);
    testrun(list->remove(list, 1) == teststring);
    testrun(0 == list->count(list));

    testrun(list->insert(list, 1, teststring));
    testrun(list->insert(list, 1, teststring2));

    testrun(list->remove(list, 2) == teststring);
    testrun(list->remove(list, 1) == teststring2);
    testrun(0 == list->count(list));

    testrun(list->insert(list, 1, teststring));
    testrun(list->insert(list, 2, teststring2));
    testrun(list->insert(list, 3, teststring3));
    testrun(3 == list->count(list));

    testrun(teststring3 == list->remove(list, 3));
    testrun(teststring2 == list->remove(list, 2));
    testrun(teststring == list->remove(list, 1));
    testrun(0 == list->count(list));

    testrun(list->insert(list, 1, teststring));
    testrun(list->insert(list, 2, teststring2));
    testrun(list->insert(list, 3, teststring3));

    /* Remove out of bounds */
    testrun(0 == list->remove(list, 5));

    testrun(teststring2 == list->remove(list, 2));
    testrun(teststring3 == list->remove(list, 2));
    testrun(teststring == list->remove(list, 1));

    list = list->free(list);
    testrun(0 == list);

    free(teststring);
    free(teststring2);
    free(teststring3);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_list_push() {

    ov_list *list = list_creator((ov_list_config){
        .item = ov_data_string_data_functions(),
    });

    testrun(list->push);

    char *teststring = strdup("aaa");
    char *teststring2 = strdup("bbb");
    char *teststring3 = strdup("ab1ab123");

    testrun(!list->push(0, teststring));

    testrun(list->push(list, teststring));

    testrun(list->get(list, 1) == teststring);

    testrun(list->push(list, teststring2));

    testrun(list->get(list, 1) == teststring);
    testrun(list->get(list, 2) == teststring2);

    testrun(list->push(list, teststring3));

    testrun(list->get(list, 1) == teststring);
    testrun(list->get(list, 2) == teststring2);
    testrun(list->get(list, 3) == teststring3);

    testrun(list->remove(list, 2) == teststring2);

    testrun(list->get(list, 1) == teststring);
    testrun(list->get(list, 2) == teststring3);

    list = list->free(list);
    testrun(0 == list);

    free(teststring2);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_list_pop() {

    ov_list *list = list_creator((ov_list_config){
        .item = ov_data_string_data_functions(),
    });

    testrun(list->pop);

    char *teststring = strdup("aaa");
    char *teststring2 = strdup("bbb");
    char *teststring3 = strdup("ab1ab123");

    testrun(list->pop(0) == 0);

    testrun(list->pop(list) == 0);

    testrun(list->push(list, teststring));

    testrun(list->pop(list) == teststring);

    testrun(list->push(list, teststring));
    testrun(list->push(list, teststring2));

    testrun(list->pop(list) == teststring2);
    testrun(list->pop(list) == teststring);

    testrun(list->push(list, teststring));
    testrun(list->push(list, teststring2));
    testrun(list->push(list, teststring3));
    testrun(list->push(list, teststring2));

    testrun(list->pop(list) == teststring2);
    testrun(list->pop(list) == teststring3);
    testrun(list->pop(list) == teststring2);
    testrun(list->pop(list) == teststring);

    testrun(list->push(list, teststring));
    testrun(list->push(list, teststring2));
    testrun(list->push(list, teststring3));
    testrun(list->push(list, teststring2));

    testrun(list->pop(list) == teststring2);
    testrun(list->pop(list) == teststring3);

    testrun(list->get(list, 1) == teststring);
    testrun(list->get(list, 2) == teststring2);
    testrun(list->get(list, 3) == 0);

    testrun(list->push(list, teststring));

    testrun(list->get(list, 1) == teststring);
    testrun(list->get(list, 2) == teststring2);
    testrun(list->get(list, 3) == teststring);
    testrun(list->get(list, 4) == 0);

    testrun(list->pop(list) == teststring);

    testrun(list->get(list, 1) == teststring);
    testrun(list->get(list, 2) == teststring2);
    testrun(list->get(list, 3) == 0);

    testrun(list->pop(list) == teststring2);

    testrun(list->get(list, 1) == teststring);
    testrun(list->get(list, 2) == 0);

    testrun(list->pop(list) == teststring);

    testrun(list->get(list, 1) == 0);

    testrun(list->pop(list) == 0);

    testrun(0 == list->count(list));
    list = list->free(list);
    testrun(0 == list);

    list = list_creator((ov_list_config){
        .item = ov_data_string_data_functions(),
    });

    testrun(0 != list);

    testrun(list->insert(list, 1, teststring));

    void *popped_item = list->pop(list);
    testrun(teststring == popped_item);

    testrun(0 == list->count(list));

    popped_item = list->pop(list);
    testrun(0 == popped_item);

    list = list->free(list);
    testrun(0 == list);

    free(teststring);
    free(teststring2);
    free(teststring3);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_list_count() {

    ov_list *list = list_creator((ov_list_config){0});

    testrun(list->count);

    char *teststring = "aaa";

    testrun(0 == list->count(0));
    testrun(0 == list->count(list));

    testrun(true == list->set(list, 1, teststring, NULL));
    testrun(1 == list->count(list));

    testrun(true == list->set(list, 2, teststring, NULL));
    testrun(2 == list->count(list));

    testrun(true == list->set(list, 3, teststring, NULL));
    testrun(3 == list->count(list));

    testrun(true == list->set(list, 4, teststring, NULL));
    testrun(4 == list->count(list));

    testrun(true == list->set(list, 5, teststring, NULL));
    testrun(5 == list->count(list));

    /* TBD 10 or 6? We have set only 6 items!. */
    testrun(true == list->set(list, 10, teststring, NULL));
    testrun(10 == list->count(list));

    list = list->free(list);
    testrun(0 == list);

    return testrun_log_success();
}
/*---------------------------------------------------------------------------*/

int test_impl_list_for_each() {

    ov_list *list = list_creator((ov_list_config){
        .item = ov_data_string_data_functions(),
    });

    testrun(list->for_each);

    char *teststring = strdup("aaa");
    char *teststring2 = strdup("bbb");
    char *teststring3 = strdup("ab1ab123");

    testrun(0 == list->for_each(0, 0, count_items));
    testrun(0 == list->get(list, 1));

    check_teststring_arg cta = (check_teststring_arg){
        .teststrings = (char *[]){teststring, teststring2, teststring3},
        .num_strings = 3,
    };

    testrun(list->for_each(list, &cta, check_teststrings) &&
            (0 == cta.counter));

    testrun(true == list->set(list, 1, teststring, NULL));

    cta.counter = 0;
    testrun(list->for_each(list, &cta, check_teststrings) &&
            (1 == cta.counter));

    testrun(true == list->set(list, 2, teststring2, NULL));
    cta.counter = 0;
    testrun(list->for_each(list, &cta, check_teststrings) &&
            (2 == cta.counter));

    testrun(true == list->set(list, 3, teststring3, NULL));
    cta.counter = 0;
    testrun(list->for_each(list, &cta, check_teststrings) &&
            (3 == cta.counter));

    list = list->free(list);
    testrun(0 == list);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_list_iter() {

    ov_list *list = list_creator((ov_list_config){
        .item = ov_data_string_data_functions(),
    });

    testrun(list->iter);
    testrun(list->next);

    testrun(!list->iter(NULL));

    void *item = NULL;
    void *iter = list->iter(list);
    testrun(iter == NULL);

    char *teststring1 = strdup("aaa");
    char *teststring2 = strdup("bbb");
    char *teststring3 = strdup("ab1ab123");

    testrun(ov_list_push(list, teststring1));
    testrun(!ov_list_is_empty(list));

    iter = list->iter(list);
    testrun(iter);

    // test position over content, content will be given over next
    iter = list->next(list, iter, &item);
    testrun(iter == NULL);
    testrun(item != NULL);
    testrun(0 == strncmp((char *)item, teststring1, strlen(teststring1)));

    testrun(ov_list_push(list, teststring2));
    testrun(ov_list_push(list, teststring3));

    iter = list->iter(list);
    testrun(iter);
    iter = list->next(list, iter, &item);
    testrun(iter != NULL);
    testrun(item != NULL);
    testrun(0 == strncmp((char *)item, teststring1, strlen(teststring1)));
    iter = list->next(list, iter, &item);
    testrun(iter != NULL);
    testrun(item != NULL);
    testrun(0 == strncmp((char *)item, teststring2, strlen(teststring2)));
    iter = list->next(list, iter, &item);
    testrun(iter == NULL);
    testrun(item != NULL);
    testrun(0 == strncmp((char *)item, teststring3, strlen(teststring3)));

    testrun(NULL == list->free(list));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_list_next() {

    ov_list *list = list_creator((ov_list_config){
        .item = ov_data_string_data_functions(),
    });

    testrun(list->iter);
    testrun(list->next);

    char *teststring1 = strdup("aaa");
    char *teststring2 = strdup("bbb");
    char *teststring3 = strdup("ab1ab123");

    testrun(ov_list_push(list, teststring1));
    testrun(ov_list_push(list, teststring2));
    testrun(ov_list_push(list, teststring3));

    void *iter = NULL;
    void *next = NULL;
    void *item = NULL;

    iter = list->iter(list);

    testrun(NULL == list->next(NULL, NULL, NULL));
    testrun(NULL == list->next(list, NULL, &item));
    testrun(NULL == list->next(NULL, iter, &item));

    next = list->next(list, iter, &item);
    testrun(next != NULL);
    testrun(item != NULL);
    testrun(0 == strncmp((char *)item, teststring1, strlen(teststring1)));

    next = list->next(list, iter, &item);
    testrun(next != NULL);
    testrun(item != NULL);
    testrun(0 == strncmp((char *)item, teststring1, strlen(teststring1)));

    next = list->next(list, iter, &item);
    testrun(next != NULL);
    testrun(item != NULL);
    testrun(0 == strncmp((char *)item, teststring1, strlen(teststring1)));

    next = list->next(list, next, &item);
    testrun(next != NULL);
    testrun(item != NULL);
    testrun(0 == strncmp((char *)item, teststring2, strlen(teststring2)));

    next = list->next(list, iter, &item);
    testrun(next != NULL);
    testrun(item != NULL);
    testrun(0 == strncmp((char *)item, teststring1, strlen(teststring1)));

    next = list->next(list, next, &item);
    testrun(next != NULL);
    testrun(item != NULL);
    testrun(0 == strncmp((char *)item, teststring2, strlen(teststring2)));

    next = list->next(list, next, &item);
    testrun(next == NULL);
    testrun(item != NULL);
    testrun(0 == strncmp((char *)item, teststring3, strlen(teststring3)));

    next = list->next(list, next, &item);
    testrun(next == NULL);
    testrun(item == NULL);

    next = list->next(list, iter, NULL);
    testrun(next != NULL);

    next = list->next(list, next, NULL);
    testrun(next != NULL);

    // check 3 steps with last item check
    next = list->next(list, next, &item);
    testrun(next == NULL);
    testrun(item != NULL);
    testrun(0 == strncmp((char *)item, teststring3, strlen(teststring3)));

    testrun(NULL == list->free(list));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/
