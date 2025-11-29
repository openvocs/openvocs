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
        @file           ov_item_test.c
        @author         Töpfer, Markus

        @date           2025-11-28


        ------------------------------------------------------------------------
*/
#include <ov_test/testrun.h>
#include "ov_item.c"

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

int test_ov_item_cast(){
    
    uint16_t magic_byte = 0;

    for (uint16_t i = 0; i < UINT16_MAX; i++){

        magic_byte = i;

        if (i == OV_ITEM_MAGIC_BYTES){
            testrun(ov_item_cast(&magic_byte));
        } else {
            testrun(!ov_item_cast(&magic_byte))
        }
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_object(){

    ov_item *item = ov_item_object();
    testrun(ov_item_is_object(item));
    testrun(ov_dict_cast(item->config.data));
    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_array(){
    
    ov_item *item = ov_item_array();
    testrun(ov_item_is_array(item));
    testrun(ov_list_cast(item->config.data));
    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_string(){

    ov_item *item = ov_item_string("test");
    testrun(ov_item_is_string(item));
    testrun(0 == ov_string_compare(item->config.data, "test"));
    testrun(NULL == ov_item_free(item));

    testrun(NULL == ov_item_string(NULL));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_number(){
    
    ov_item *item = ov_item_number(0);
    testrun(ov_item_is_number(item));
    testrun(0 == item->config.number);
    testrun(NULL == ov_item_free(item));

    for (size_t i = 0; i < 100; i++){

        ov_item *item = ov_item_number(i);
        testrun(item);
        testrun(ov_item_is_number(item));
        testrun(i == item->config.number);
        testrun(NULL == ov_item_free(item));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_true(){

    ov_item *item = ov_item_true();
    testrun(ov_item_is_true(item));
    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_false(){
    
    ov_item *item = ov_item_false();
    testrun(ov_item_is_false(item));
    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_null(){
    
    ov_item *item = ov_item_null();
    testrun(ov_item_is_null(item));
    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_free(){
    
    ov_item *item = ov_item_null();
    ov_list *list = ov_linked_list_create((ov_list_config){0});

    testrun(list == ov_item_free(list));
    testrun(NULL == ov_list_free(list));

    testrun(ov_item_is_null(item));
    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_clear(){
    
    ov_item *item = ov_item_null();
    ov_list *list = ov_linked_list_create((ov_list_config){0});

    testrun(!ov_item_clear(list));
    testrun(NULL == ov_list_free(list));

    testrun(ov_item_clear(item));
    testrun(ov_item_is_null(item));
    testrun(NULL == item->config.data);
    testrun(NULL == ov_item_free(item));

    item = ov_item_true();
    testrun(ov_item_clear(item));
    testrun(ov_item_is_null(item));
    testrun(NULL == item->config.data);
    testrun(NULL == ov_item_free(item));

    item = ov_item_false();
    testrun(ov_item_clear(item));
    testrun(ov_item_is_null(item));
    testrun(NULL == item->config.data);
    testrun(NULL == ov_item_free(item));

    item = ov_item_number(100);
    testrun(ov_item_clear(item));
    testrun(ov_item_is_null(item));
    testrun(NULL == item->config.data);
    testrun(NULL == ov_item_free(item));

    item = ov_item_string("test");
    testrun(ov_item_clear(item));
    testrun(ov_item_is_null(item));
    testrun(NULL == item->config.data);
    testrun(NULL == ov_item_free(item));

    item = ov_item_object();
    testrun(ov_item_clear(item));
    testrun(ov_item_is_null(item));
    testrun(NULL == item->config.data);
    testrun(NULL == ov_item_free(item));

    item = ov_item_array();
    testrun(ov_item_clear(item));
    testrun(ov_item_is_null(item));
    testrun(NULL == item->config.data);
    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_copy(){
    
    ov_item *item = ov_item_object();
    ov_item *copy = NULL;

    testrun(ov_item_object_set(item, "1", ov_item_null()));
    testrun(ov_item_object_set(item, "2", ov_item_true()));
    testrun(ov_item_object_set(item, "3", ov_item_false()));
    testrun(ov_item_object_set(item, "4", ov_item_string("test")));
    testrun(ov_item_object_set(item, "5", ov_item_array()));
    testrun(ov_item_object_set(item, "6", ov_item_object()));
    testrun(ov_item_object_set(item, "7", ov_item_number(7)));
    testrun(7 == ov_item_count(item));

    testrun(!ov_item_copy((void**)&copy, NULL));

    testrun(ov_item_copy((void**)&copy, item));
    testrun(copy);
    testrun(7 == ov_item_count(copy));
    testrun(copy != item);

    testrun(ov_item_is_null((ov_item*)ov_item_object_get(copy, "1")));
    testrun(ov_item_is_true((ov_item*)ov_item_object_get(copy, "2")));
    testrun(ov_item_is_false((ov_item*)ov_item_object_get(copy, "3")));
    testrun(ov_item_is_string((ov_item*)ov_item_object_get(copy, "4")));
    testrun(ov_item_is_array((ov_item*)ov_item_object_get(copy, "5")));
    testrun(ov_item_is_object((ov_item*)ov_item_object_get(copy, "6")));
    testrun(ov_item_is_number((ov_item*)ov_item_object_get(copy, "7")));

    testrun(NULL == ov_item_free(item));
    testrun(NULL == ov_item_free(copy));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_dump(){
    
    ov_item *item = ov_item_object();

    testrun(ov_item_object_set(item, "1", ov_item_null()));
    testrun(ov_item_object_set(item, "2", ov_item_true()));
    testrun(ov_item_object_set(item, "3", ov_item_false()));
    testrun(ov_item_object_set(item, "4", ov_item_string("test")));
    testrun(ov_item_object_set(item, "5", ov_item_array()));
    testrun(ov_item_object_set(item, "6", ov_item_object()));
    testrun(ov_item_object_set(item, "7", ov_item_number(7)));
    testrun(7 == ov_item_count(item));

    testrun(ov_item_dump(stdout, item));
    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_count(){
    
    ov_item *item = ov_item_null();
    testrun(ov_item_is_null(item));
    testrun(1 == ov_item_count(item));
    testrun(NULL == ov_item_free(item));

    item = ov_item_true();
    testrun(1 == ov_item_count(item));
    testrun(NULL == ov_item_free(item));

    item = ov_item_false();
    testrun(1 == ov_item_count(item));
    testrun(NULL == ov_item_free(item));

    item = ov_item_number(100);
    testrun(1 == ov_item_count(item));
    testrun(NULL == ov_item_free(item));

    item = ov_item_string("test");
    testrun(1 == ov_item_count(item));
    testrun(NULL == ov_item_free(item));

    item = ov_item_object();
    testrun(0 == ov_item_count(item));
    testrun(ov_item_object_set(item, "1", ov_item_null()));
    testrun(ov_item_object_set(item, "2", ov_item_true()));
    testrun(ov_item_object_set(item, "3", ov_item_false()));
    testrun(ov_item_object_set(item, "4", ov_item_string("test")));
    testrun(ov_item_object_set(item, "5", ov_item_array()));
    testrun(ov_item_object_set(item, "6", ov_item_object()));
    testrun(ov_item_object_set(item, "7", ov_item_number(7)));
    testrun(7 == ov_item_count(item));
    testrun(NULL == ov_item_free(item));

    item = ov_item_array();
    testrun(0 == ov_item_count(item));
    testrun(ov_item_array_push(item, ov_item_null()));
    testrun(ov_item_array_push(item, ov_item_true()));
    testrun(ov_item_array_push(item, ov_item_false()));
    testrun(ov_item_array_push(item, ov_item_string("test")));
    testrun(ov_item_array_push(item, ov_item_array()));
    testrun(ov_item_array_push(item, ov_item_object()));
    testrun(ov_item_array_push(item, ov_item_number(7)));
    testrun(7 == ov_item_count(item));
    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_is_object(){
    
    ov_item *n = ov_item_null();
    ov_item *f = ov_item_false();
    ov_item *t = ov_item_true();
    ov_item *nbr = ov_item_number(0);
    ov_item *str = ov_item_string("test");
    ov_item *arr = ov_item_array();
    ov_item *obj = ov_item_object();

    testrun(!ov_item_is_object(n));
    testrun(!ov_item_is_object(f));
    testrun(!ov_item_is_object(t));
    testrun(!ov_item_is_object(nbr));
    testrun(!ov_item_is_object(str));
    testrun(!ov_item_is_object(arr));
    testrun(ov_item_is_object(obj));

    testrun(NULL == ov_item_free(n));
    testrun(NULL == ov_item_free(f));
    testrun(NULL == ov_item_free(t));
    testrun(NULL == ov_item_free(nbr));
    testrun(NULL == ov_item_free(str));
    testrun(NULL == ov_item_free(arr));
    testrun(NULL == ov_item_free(obj));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_is_array(){
    
    ov_item *n = ov_item_null();
    ov_item *f = ov_item_false();
    ov_item *t = ov_item_true();
    ov_item *nbr = ov_item_number(0);
    ov_item *str = ov_item_string("test");
    ov_item *arr = ov_item_array();
    ov_item *obj = ov_item_object();

    testrun(!ov_item_is_array(n));
    testrun(!ov_item_is_array(f));
    testrun(!ov_item_is_array(t));
    testrun(!ov_item_is_array(nbr));
    testrun(!ov_item_is_array(str));
    testrun(ov_item_is_array(arr));
    testrun(!ov_item_is_array(obj));

    testrun(NULL == ov_item_free(n));
    testrun(NULL == ov_item_free(f));
    testrun(NULL == ov_item_free(t));
    testrun(NULL == ov_item_free(nbr));
    testrun(NULL == ov_item_free(str));
    testrun(NULL == ov_item_free(arr));
    testrun(NULL == ov_item_free(obj));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_is_string(){
    
    ov_item *n = ov_item_null();
    ov_item *f = ov_item_false();
    ov_item *t = ov_item_true();
    ov_item *nbr = ov_item_number(0);
    ov_item *str = ov_item_string("test");
    ov_item *arr = ov_item_array();
    ov_item *obj = ov_item_object();

    testrun(!ov_item_is_string(n));
    testrun(!ov_item_is_string(f));
    testrun(!ov_item_is_string(t));
    testrun(!ov_item_is_string(nbr));
    testrun(ov_item_is_string(str));
    testrun(!ov_item_is_string(arr));
    testrun(!ov_item_is_string(obj));

    testrun(NULL == ov_item_free(n));
    testrun(NULL == ov_item_free(f));
    testrun(NULL == ov_item_free(t));
    testrun(NULL == ov_item_free(nbr));
    testrun(NULL == ov_item_free(str));
    testrun(NULL == ov_item_free(arr));
    testrun(NULL == ov_item_free(obj));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_is_number(){
    
    ov_item *n = ov_item_null();
    ov_item *f = ov_item_false();
    ov_item *t = ov_item_true();
    ov_item *nbr = ov_item_number(0);
    ov_item *str = ov_item_string("test");
    ov_item *arr = ov_item_array();
    ov_item *obj = ov_item_object();

    testrun(!ov_item_is_number(n));
    testrun(!ov_item_is_number(f));
    testrun(!ov_item_is_number(t));
    testrun(ov_item_is_number(nbr));
    testrun(!ov_item_is_number(str));
    testrun(!ov_item_is_number(arr));
    testrun(!ov_item_is_number(obj));

    testrun(NULL == ov_item_free(n));
    testrun(NULL == ov_item_free(f));
    testrun(NULL == ov_item_free(t));
    testrun(NULL == ov_item_free(nbr));
    testrun(NULL == ov_item_free(str));
    testrun(NULL == ov_item_free(arr));
    testrun(NULL == ov_item_free(obj));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_is_null(){
    
    ov_item *n = ov_item_null();
    ov_item *f = ov_item_false();
    ov_item *t = ov_item_true();
    ov_item *nbr = ov_item_number(0);
    ov_item *str = ov_item_string("test");
    ov_item *arr = ov_item_array();
    ov_item *obj = ov_item_object();

    testrun(ov_item_is_null(n));
    testrun(!ov_item_is_null(f));
    testrun(!ov_item_is_null(t));
    testrun(!ov_item_is_null(nbr));
    testrun(!ov_item_is_null(str));
    testrun(!ov_item_is_null(arr));
    testrun(!ov_item_is_null(obj));

    testrun(NULL == ov_item_free(n));
    testrun(NULL == ov_item_free(f));
    testrun(NULL == ov_item_free(t));
    testrun(NULL == ov_item_free(nbr));
    testrun(NULL == ov_item_free(str));
    testrun(NULL == ov_item_free(arr));
    testrun(NULL == ov_item_free(obj));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_is_false(){
    
    ov_item *n = ov_item_null();
    ov_item *f = ov_item_false();
    ov_item *t = ov_item_true();
    ov_item *nbr = ov_item_number(0);
    ov_item *str = ov_item_string("test");
    ov_item *arr = ov_item_array();
    ov_item *obj = ov_item_object();

    testrun(!ov_item_is_false(n));
    testrun(ov_item_is_false(f));
    testrun(!ov_item_is_false(t));
    testrun(!ov_item_is_false(nbr));
    testrun(!ov_item_is_false(str));
    testrun(!ov_item_is_false(arr));
    testrun(!ov_item_is_false(obj));

    testrun(NULL == ov_item_free(n));
    testrun(NULL == ov_item_free(f));
    testrun(NULL == ov_item_free(t));
    testrun(NULL == ov_item_free(nbr));
    testrun(NULL == ov_item_free(str));
    testrun(NULL == ov_item_free(arr));
    testrun(NULL == ov_item_free(obj));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_is_true(){
    
    ov_item *n = ov_item_null();
    ov_item *f = ov_item_false();
    ov_item *t = ov_item_true();
    ov_item *nbr = ov_item_number(0);
    ov_item *str = ov_item_string("test");
    ov_item *arr = ov_item_array();
    ov_item *obj = ov_item_object();

    testrun(!ov_item_is_true(n));
    testrun(!ov_item_is_true(f));
    testrun(ov_item_is_true(t));
    testrun(!ov_item_is_true(nbr));
    testrun(!ov_item_is_true(str));
    testrun(!ov_item_is_true(arr));
    testrun(!ov_item_is_true(obj));

    testrun(NULL == ov_item_free(n));
    testrun(NULL == ov_item_free(f));
    testrun(NULL == ov_item_free(t));
    testrun(NULL == ov_item_free(nbr));
    testrun(NULL == ov_item_free(str));
    testrun(NULL == ov_item_free(arr));
    testrun(NULL == ov_item_free(obj));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_object_set(){
    
    ov_item *n = ov_item_null();
    ov_item *f = ov_item_false();
    ov_item *t = ov_item_true();
    ov_item *nbr = ov_item_number(0);
    ov_item *str = ov_item_string("test");
    ov_item *arr = ov_item_array();
    ov_item *obj = ov_item_object();

    ov_item *val = ov_item_number(1234);

    testrun(!ov_item_object_set(n, "key", val));
    testrun(!ov_item_object_set(f, "key", val));
    testrun(!ov_item_object_set(t, "key", val));
    testrun(!ov_item_object_set(str, "key", val));
    testrun(!ov_item_object_set(nbr, "key", val));
    testrun(!ov_item_object_set(arr, "key", val));
    testrun(ov_item_object_set(obj, "key", val));
    testrun(1 == ov_item_count(obj));
    testrun(1234 == ov_item_get_number((ov_item*)ov_item_object_get(obj, "key")));

    testrun(NULL == ov_item_free(n));
    testrun(NULL == ov_item_free(f));
    testrun(NULL == ov_item_free(t));
    testrun(NULL == ov_item_free(nbr));
    testrun(NULL == ov_item_free(str));
    testrun(NULL == ov_item_free(arr));
    testrun(NULL == ov_item_free(obj));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_object_delete(){
    
    ov_item *item = ov_item_object();
    ov_item *obj = ov_item_object();

    ov_item *val = ov_item_number(1234);

 
    testrun(ov_item_object_set(item, "1", val));
    testrun(ov_item_object_set(item, "2", obj));
    testrun(2 == ov_item_count(item));

    testrun(ov_item_object_delete(item, "2"));
    testrun(1 == ov_item_count(item));

    testrun(ov_item_object_delete(item, "1"));
    testrun(0 == ov_item_count(item));

    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_object_remove(){
    
    ov_item *item = ov_item_object();
    ov_item *obj = ov_item_object();

    ov_item *val = ov_item_number(1234);

 
    testrun(ov_item_object_set(item, "1", val));
    testrun(ov_item_object_set(item, "2", obj));
    testrun(2 == ov_item_count(item));

    testrun(obj == ov_item_object_remove(item, "2"));
    testrun(1 == ov_item_count(item));

    testrun(val == ov_item_object_remove(item, "1"));
    testrun(0 == ov_item_count(item));

    testrun(NULL == ov_item_free(item));
    testrun(NULL == ov_item_free(val));
    testrun(NULL == ov_item_free(obj));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_object_get(){
    
    ov_item *item = ov_item_object();
    ov_item *obj = ov_item_object();

    ov_item *val = ov_item_number(1234);

 
    testrun(ov_item_object_set(item, "1", val));
    testrun(ov_item_object_set(item, "2", obj));
    testrun(2 == ov_item_count(item));

    testrun(obj == ov_item_object_get(item, "2"));
    testrun(2 == ov_item_count(item));

    testrun(val == ov_item_object_get(item, "1"));
    testrun(2 == ov_item_count(item));

    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool clear_subitem(const char *key, ov_item const *val, void *userdata){

    if (!key) return true;
    ov_item_clear((ov_item*)val);
    UNUSED(userdata);
    return true;
}

/*----------------------------------------------------------------------------*/

int test_ov_item_object_for_each(){
    
    ov_item *item = ov_item_object();
    ov_item *obj = ov_item_object();
    ov_item *val = ov_item_number(1234);
 
    testrun(ov_item_object_set(item, "1", val));
    testrun(ov_item_object_set(item, "2", obj));

    testrun(2 == ov_item_count(item));

    testrun(ov_item_object_for_each(
        item, 
        clear_subitem,
        item));

    testrun(2 == ov_item_count(item));

    testrun(obj == ov_item_object_get(item, "2"));
    testrun(2 == ov_item_count(item));
    testrun(ov_item_is_null(obj));

    testrun(val == ov_item_object_get(item, "1"));
    testrun(2 == ov_item_count(item));
    testrun(ov_item_is_null(val));

    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_array_get(){
    
    ov_item *item = ov_item_array();
    ov_item *obj = ov_item_object();
    ov_item *val = ov_item_number(1234);

    testrun(ov_item_array_push(item, obj));
    testrun(ov_item_array_push(item, val));

    testrun(obj == ov_item_array_get(item, 1));
    testrun(val == ov_item_array_get(item, 2));
    testrun(NULL == ov_item_array_get(item, 3));

    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_array_set(){
    
    ov_item *item = ov_item_array();
    ov_item *obj = ov_item_object();
    ov_item *val = ov_item_number(1234);

    testrun(ov_item_array_push(item, obj));
    testrun(ov_item_array_push(item, val));

    testrun(ov_item_array_set(item, 1, ov_item_null()));
    testrun(ov_item_is_null(ov_item_array_get(item, 1)));
    testrun(val == ov_item_array_get(item, 2));
    testrun(NULL == ov_item_array_get(item, 3));
    testrun(NULL == ov_item_array_get(item, 4));

    testrun(ov_item_array_set(item, 3, ov_item_null()));
    testrun(ov_item_is_null(ov_item_array_get(item, 1)));
    testrun(val == ov_item_array_get(item, 2));
    testrun(ov_item_is_null(ov_item_array_get(item, 3)));
    testrun(NULL == ov_item_array_get(item, 4));
    testrun(NULL == ov_item_array_get(item, 5));
    testrun(NULL == ov_item_array_get(item, 6));
    testrun(NULL == ov_item_array_get(item, 7));
    testrun(ov_item_array_set(item, 6, ov_item_true()));
    testrun(ov_item_is_null(ov_item_array_get(item, 1)));
    testrun(val == ov_item_array_get(item, 2));
    testrun(ov_item_is_null(ov_item_array_get(item, 3)));
    testrun(NULL == ov_item_array_get(item, 4));
    testrun(NULL == ov_item_array_get(item, 5));
    testrun(ov_item_is_true(ov_item_array_get(item, 6)));
    testrun(NULL == ov_item_array_get(item, 7));

    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_array_push(){
    
    ov_item *item = ov_item_array();
    ov_item *obj = ov_item_object();
    ov_item *val = ov_item_number(1234);

    testrun(ov_item_array_push(item, obj));
    testrun(ov_item_array_push(item, val));

    testrun(obj == ov_item_array_get(item, 1));
    testrun(val == ov_item_array_get(item, 2));
    testrun(NULL == ov_item_array_get(item, 3));
    

    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_array_stack_pop(){
    
    ov_item *item = ov_item_array();
    ov_item *obj = ov_item_object();
    ov_item *val = ov_item_number(1234);

    testrun(ov_item_array_push(item, obj));
    testrun(ov_item_array_push(item, val));

    testrun(val == ov_item_array_stack_pop(item));
    testrun(obj == ov_item_array_stack_pop(item));
    testrun(NULL == ov_item_array_stack_pop(item));

    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_array_queue_pop(){
    
    ov_item *item = ov_item_array();
    ov_item *obj = ov_item_object();
    ov_item *val = ov_item_number(1234);

    testrun(ov_item_array_push(item, obj));
    testrun(ov_item_array_push(item, val));

    testrun(obj == ov_item_array_queue_pop(item));
    testrun(val == ov_item_array_queue_pop(item));
    testrun(NULL == ov_item_array_queue_pop(item));

    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_get_string(){
    
    ov_item *item = ov_item_string("test");
    testrun(0 == ov_string_compare("test", ov_item_get_string(item)));
    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_get_number(){
    
    ov_item *item = ov_item_number(1.001);
    testrun(1.001 == ov_item_get_number(item));
    testrun(NULL == ov_item_free(item));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_item_get_int(){
    
    ov_item *item = ov_item_number(1.001);
    testrun(1 == ov_item_get_int(item));
    testrun(NULL == ov_item_free(item));

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
    testrun_test(test_ov_item_cast);

    testrun_test(test_ov_item_object);
    testrun_test(test_ov_item_array);
    testrun_test(test_ov_item_string);
    testrun_test(test_ov_item_number);
    testrun_test(test_ov_item_true);
    testrun_test(test_ov_item_false);
    testrun_test(test_ov_item_null);

    testrun_test(test_ov_item_free);
    testrun_test(test_ov_item_clear);
    testrun_test(test_ov_item_copy);
    testrun_test(test_ov_item_dump);

    testrun_test(test_ov_item_count);

    testrun_test(test_ov_item_is_object);
    testrun_test(test_ov_item_is_array);
    testrun_test(test_ov_item_is_string);
    testrun_test(test_ov_item_is_number);
    testrun_test(test_ov_item_is_null);
    testrun_test(test_ov_item_is_true);
    testrun_test(test_ov_item_is_false);

    testrun_test(test_ov_item_object_set);
    testrun_test(test_ov_item_object_delete);
    testrun_test(test_ov_item_object_remove);
    testrun_test(test_ov_item_object_get);
    testrun_test(test_ov_item_object_for_each);

    testrun_test(test_ov_item_array_get);
    testrun_test(test_ov_item_array_set);
    testrun_test(test_ov_item_array_push);
    testrun_test(test_ov_item_array_stack_pop);
    testrun_test(test_ov_item_array_queue_pop);

    testrun_test(test_ov_item_get_string);
    testrun_test(test_ov_item_get_number);
    testrun_test(test_ov_item_get_int);

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
