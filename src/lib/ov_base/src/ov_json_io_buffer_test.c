/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_json_io_buffer_test.c
        @author         Markus Töpfer

        @date           2021-03-29


        ------------------------------------------------------------------------
*/
#include "ov_json_io_buffer.c"
#include <ov_test/testrun.h>

#include <ov_base/ov_random.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

struct dummy_userdata {

    ov_json_io_buffer *self;
    ov_list *list;
    int socket;

    bool error;
};

/*----------------------------------------------------------------------------*/

static void dummy_receive_no_drop(void *userdata,
                                  int socket,
                                  ov_json_value *value) {

    struct dummy_userdata *data = (struct dummy_userdata *)userdata;
    ov_list_push(data->list, value);
    data->socket = socket;
    data->error = false;
    return;
}

/*----------------------------------------------------------------------------*/

static void dummy_receive_drop(void *userdata,
                               int socket,
                               ov_json_value *value) {

    struct dummy_userdata *data = (struct dummy_userdata *)userdata;
    ov_list_push(data->list, value);
    data->socket = socket;
    ov_json_io_buffer_drop(data->self, socket);
    data->error = false;
    return;
}

/*----------------------------------------------------------------------------*/

static void dummy_error(void *userdata, int socket) {

    struct dummy_userdata *data = (struct dummy_userdata *)userdata;

    data->socket = socket;
    data->error = true;
    return;
}

/*----------------------------------------------------------------------------*/

static bool dummy_init(struct dummy_userdata *dummy) {

    if (!dummy) return false;

    dummy->socket = 0;

    dummy->list =
        ov_list_create((ov_list_config){.item.free = ov_json_value_free});
    return true;
}

/*----------------------------------------------------------------------------*/

static bool dummy_deinit(struct dummy_userdata *dummy) {

    if (!dummy) return false;

    dummy->self = NULL;
    dummy->list = ov_list_free(dummy->list);
    return true;
}

/*----------------------------------------------------------------------------*/

int test_ov_json_io_buffer_create() {

    struct dummy_userdata dummy;
    testrun(dummy_init(&dummy));

    ov_json_io_buffer_config config = (ov_json_io_buffer_config){0};

    testrun(!ov_json_io_buffer_create(config));

    config.callback.userdata = &dummy;
    config.callback.success = dummy_receive_no_drop;

    ov_json_io_buffer *self = ov_json_io_buffer_create(config);

    testrun(self);
    testrun(self->dict);
    testrun(self->config.debug == false);

    testrun(NULL == ov_json_io_buffer_free(self));

    testrun(dummy_deinit(&dummy));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_io_buffer_free() {

    struct dummy_userdata dummy;
    testrun(dummy_init(&dummy));

    ov_json_io_buffer_config config = (ov_json_io_buffer_config){
        .callback.userdata = &dummy, .callback.success = dummy_receive_no_drop};

    ov_json_io_buffer *self = ov_json_io_buffer_create(config);

    testrun(NULL == ov_json_io_buffer_free(NULL));
    testrun(NULL == ov_json_io_buffer_free(self));

    /* check with content */

    self = ov_json_io_buffer_create(config);

    char *valid_json = "{\"key\":";

    ov_memory_pointer ptr = (ov_memory_pointer){
        .start = (uint8_t *)valid_json, .length = strlen(valid_json)};

    dummy.socket = 0;
    testrun(0 == ov_list_count(dummy.list));

    testrun(ov_json_io_buffer_push(self, 1, ptr));
    testrun(ov_json_io_buffer_push(self, 2, ptr));
    testrun(ov_json_io_buffer_push(self, 3, ptr));

    testrun(0 == dummy.socket);
    testrun(0 == ov_list_count(dummy.list));

    testrun(ov_dict_get(self->dict, (void *)(intptr_t)1));
    testrun(ov_dict_get(self->dict, (void *)(intptr_t)2));
    testrun(ov_dict_get(self->dict, (void *)(intptr_t)3));

    testrun(NULL == ov_json_io_buffer_free(self));

    testrun(dummy_deinit(&dummy));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_io_buffer_push() {

    ov_json_value *val = NULL;
    ov_buffer *buffer = NULL;

    struct dummy_userdata dummy;
    testrun(dummy_init(&dummy));

    ov_json_io_buffer_config config =
        (ov_json_io_buffer_config){.callback.userdata = &dummy,
                                   .callback.success = dummy_receive_no_drop,
                                   .callback.failure = dummy_error};

    ov_json_io_buffer *self = ov_json_io_buffer_create(config);
    testrun(self);
    dummy.self = self;

    char *str = "{\"key\":";
    intptr_t key = 1;

    testrun(!ov_json_io_buffer_push(
        NULL,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));

    testrun(!ov_json_io_buffer_push(
        self, key, (ov_memory_pointer){.start = NULL, .length = strlen(str)}));

    testrun(!ov_json_io_buffer_push(
        self, key, (ov_memory_pointer){.start = (uint8_t *)str, .length = 0}));

    testrun(key == dummy.socket);
    testrun(true == dummy.error);
    testrun(0 == ov_list_count(dummy.list));

    // check negative indicies (allowed as ID based content from -INT to + INT)

    testrun(ov_json_io_buffer_push(
        self,
        -1,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));

    key = -1;

    // no callback yet
    testrun(1 == dummy.socket);
    testrun(true == dummy.error);
    // content added
    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 == memcmp(str, buffer->start, buffer->length));
    testrun(ov_json_io_buffer_drop(self, key));

    // check socket id
    key = 1;

    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));

    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 == memcmp(str, buffer->start, buffer->length));

    str = "\"val\"";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));

    // no callback yet
    testrun(1 == dummy.socket);
    testrun(true == dummy.error);
    // content added
    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 == memcmp("{\"key\":\"val\"", buffer->start, buffer->length));

    str = "}{\"next\":";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));

    testrun(1 == dummy.socket);
    testrun(1 == ov_list_count(dummy.list));

    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 == memcmp("{\"next\":", buffer->start, buffer->length));

    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_object(val));
    val = ov_json_value_free(val);

    // expect a drop of all content
    str = "invalid content in terms of json";
    testrun(!ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));

    testrun(!ov_dict_get(self->dict, (void *)key));
    testrun(0 == ov_list_count(dummy.list));

    // try to add invalid content (nothing added yet at socket id)
    testrun(ov_dict_is_empty(self->dict));
    testrun(!ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_get(self->dict, (void *)key));
    testrun(ov_dict_is_empty(self->dict));

    str = "{:";
    testrun(!ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(ov_dict_is_empty(self->dict));

    str = "[\"some valid array\"]";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 == buffer->length);
    testrun(1 == dummy.socket);
    testrun(1 == ov_list_count(dummy.list));
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_array(val));
    val = ov_json_value_free(val);

    str = "[\"some incomplete array ";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 ==
            memcmp("[\"some incomplete array ", buffer->start, buffer->length));
    testrun(ov_json_io_buffer_drop(self, key));

    str = "\"some valid string\"";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 == buffer->length);
    testrun(1 == dummy.socket);
    testrun(1 == ov_list_count(dummy.list));
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_string(val));
    val = ov_json_value_free(val);
    testrun(ov_json_io_buffer_drop(self, key));

    str = "\"some incomplete string";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 ==
            memcmp("\"some incomplete string", buffer->start, buffer->length));
    testrun(0 == ov_list_count(dummy.list));
    testrun(ov_json_io_buffer_drop(self, key));

    str = "null";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 == buffer->length);
    testrun(1 == dummy.socket);
    testrun(1 == ov_list_count(dummy.list));
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_null(val));
    val = ov_json_value_free(val);
    testrun(ov_json_io_buffer_drop(self, key));

    str = "true";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 == buffer->length);
    testrun(1 == dummy.socket);
    testrun(1 == ov_list_count(dummy.list));
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_true(val));
    val = ov_json_value_free(val);
    testrun(ov_json_io_buffer_drop(self, key));

    str = "false";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 == buffer->length);
    testrun(1 == dummy.socket);
    testrun(1 == ov_list_count(dummy.list));
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_false(val));
    val = ov_json_value_free(val);
    testrun(ov_json_io_buffer_drop(self, key));

    str = "tr";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 == memcmp("tr", buffer->start, buffer->length));
    testrun(ov_json_io_buffer_drop(self, key));

    str = "fal";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 == memcmp("fal", buffer->start, buffer->length));
    testrun(ov_json_io_buffer_drop(self, key));

    str = "n";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 == memcmp("n", buffer->start, buffer->length));
    testrun(ov_json_io_buffer_drop(self, key));

    str = "{\"some incomplete object";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 ==
            memcmp("{\"some incomplete object", buffer->start, buffer->length));
    testrun(ov_json_io_buffer_drop(self, key));

    testrun(0 == ov_list_count(dummy.list));
    str = "{\"key\":\"1\"} {";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    testrun(1 == ov_list_count(dummy.list));
    testrun(ov_list_clear(dummy.list));
    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 == memcmp(" {", buffer->start, buffer->length));
    testrun(ov_json_io_buffer_drop(self, key));

    str = "{\"key\":\"1\"} {    ";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    testrun(1 == ov_list_count(dummy.list));
    testrun(ov_list_clear(dummy.list));
    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 == memcmp(" {    ", buffer->start, buffer->length));
    testrun(ov_json_io_buffer_drop(self, key));

    // nothing added, as the whole buffer does not match
    testrun(0 == ov_list_count(dummy.list));
    testrun(dummy.error == false);
    str = "{\"key\":\"1\"} {   : ";
    testrun(!ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(0 == ov_list_count(dummy.list));
    testrun(ov_dict_is_empty(self->dict));
    testrun(dummy.error == true);

    // nothing added, as the whole buffer does not match
    dummy.error = false;
    str = "{\"key\":\"1\"} {[";
    testrun(!ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(0 == ov_list_count(dummy.list));
    testrun(ov_dict_is_empty(self->dict));
    testrun(dummy.error == true);

    str = "{} {\"key\":\"1\"} {\"key\":\"2\"} {\"key\":\"3\"} {";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 == memcmp(" {", buffer->start, buffer->length));
    testrun(4 == ov_list_count(dummy.list));
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_object(val));
    val = ov_json_value_free(val);
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_object(val));
    val = ov_json_value_free(val);
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_object(val));
    val = ov_json_value_free(val);
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_object(val));
    val = ov_json_value_free(val);
    testrun(ov_json_io_buffer_drop(self, key));
    testrun(ov_list_clear(dummy.list));

    str = "null null";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    testrun(2 == ov_list_count(dummy.list));
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_null(val));
    val = ov_json_value_free(val);
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_null(val));
    val = ov_json_value_free(val);
    testrun(ov_json_io_buffer_drop(self, key));
    testrun(ov_list_clear(dummy.list));

    str = "true true";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    testrun(2 == ov_list_count(dummy.list));
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_true(val));
    val = ov_json_value_free(val);
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_true(val));
    val = ov_json_value_free(val);
    testrun(ov_json_io_buffer_drop(self, key));
    testrun(ov_list_clear(dummy.list));

    str = "false false";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    testrun(2 == ov_list_count(dummy.list));
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_false(val));
    val = ov_json_value_free(val);
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_false(val));
    val = ov_json_value_free(val);
    testrun(ov_json_io_buffer_drop(self, key));
    testrun(ov_list_clear(dummy.list));

    str = "false true false true false";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    testrun(5 == ov_list_count(dummy.list));
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_false(val));
    val = ov_json_value_free(val);
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_true(val));
    val = ov_json_value_free(val);
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_false(val));
    val = ov_json_value_free(val);
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_true(val));
    val = ov_json_value_free(val);
    val = ov_list_pop(dummy.list);
    testrun(ov_json_is_false(val));
    val = ov_json_value_free(val);
    testrun(ov_json_io_buffer_drop(self, key));
    testrun(ov_list_clear(dummy.list));

    str = "false true false true null";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    testrun(5 == ov_list_count(dummy.list));
    val = ov_list_get(dummy.list, 1);
    testrun(ov_json_is_false(val));
    val = ov_list_get(dummy.list, 2);
    testrun(ov_json_is_true(val));
    val = ov_list_get(dummy.list, 3);
    testrun(ov_json_is_false(val));
    val = ov_list_get(dummy.list, 4);
    testrun(ov_json_is_true(val));
    val = ov_list_get(dummy.list, 5);
    testrun(ov_json_is_null(val));
    testrun(ov_json_io_buffer_drop(self, key));
    testrun(ov_list_clear(dummy.list));

    str = "null true false";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    testrun(3 == ov_list_count(dummy.list));
    val = ov_list_get(dummy.list, 1);
    testrun(ov_json_is_null(val));
    val = ov_list_get(dummy.list, 2);
    testrun(ov_json_is_true(val));
    val = ov_list_get(dummy.list, 3);
    testrun(ov_json_is_false(val));
    testrun(ov_json_io_buffer_drop(self, key));
    testrun(ov_list_clear(dummy.list));

    // we leave out number, as the implementation of
    // parsing currently requires a closing element ,}]
    // after optional whitespace
    str = "{} [] \"string\" true false null";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    testrun(6 == ov_list_count(dummy.list));
    val = ov_list_get(dummy.list, 1);
    testrun(ov_json_is_object(val));
    val = ov_list_get(dummy.list, 2);
    testrun(ov_json_is_array(val));
    val = ov_list_get(dummy.list, 3);
    testrun(ov_json_is_string(val));
    val = ov_list_get(dummy.list, 4);
    testrun(ov_json_is_true(val));
    val = ov_list_get(dummy.list, 5);
    testrun(ov_json_is_false(val));
    val = ov_list_get(dummy.list, 6);
    testrun(ov_json_is_null(val));
    testrun(ov_json_io_buffer_drop(self, key));
    testrun(ov_list_clear(dummy.list));

    str = "{}[]\"string\"truefalsenull";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    testrun(6 == ov_list_count(dummy.list));
    val = ov_list_get(dummy.list, 1);
    testrun(ov_json_is_object(val));
    val = ov_list_get(dummy.list, 2);
    testrun(ov_json_is_array(val));
    val = ov_list_get(dummy.list, 3);
    testrun(ov_json_is_string(val));
    val = ov_list_get(dummy.list, 4);
    testrun(ov_json_is_true(val));
    val = ov_list_get(dummy.list, 5);
    testrun(ov_json_is_false(val));
    val = ov_list_get(dummy.list, 6);
    testrun(ov_json_is_null(val));
    testrun(ov_json_io_buffer_drop(self, key));
    testrun(ov_list_clear(dummy.list));

    str = "{}[]\"string\"truefalsenull";
    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));
    testrun(!ov_dict_is_empty(self->dict));
    testrun(6 == ov_list_count(dummy.list));
    val = ov_list_get(dummy.list, 1);
    testrun(ov_json_is_object(val));
    val = ov_list_get(dummy.list, 2);
    testrun(ov_json_is_array(val));
    val = ov_list_get(dummy.list, 3);
    testrun(ov_json_is_string(val));
    val = ov_list_get(dummy.list, 4);
    testrun(ov_json_is_true(val));
    val = ov_list_get(dummy.list, 5);
    testrun(ov_json_is_false(val));
    val = ov_list_get(dummy.list, 6);
    testrun(ov_json_is_null(val));
    testrun(ov_json_io_buffer_drop(self, key));
    testrun(ov_list_clear(dummy.list));

    str = "{} {";

    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));

    testrun(!ov_dict_is_empty(self->dict));
    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 == memcmp(" {", buffer->start, buffer->length));
    testrun(1 == ov_list_count(dummy.list));
    testrun(ov_list_clear(dummy.list));

    str = "} {}{}";
    testrun(0 == ov_list_count(dummy.list));

    testrun(ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));

    testrun(!ov_dict_is_empty(self->dict));
    buffer = ov_dict_get(self->dict, (void *)key);
    testrun(buffer);
    testrun(0 == buffer->length);
    testrun(3 == ov_list_count(dummy.list));
    testrun(ov_list_clear(dummy.list));

    // check drop during receive
    self->config.callback.success = dummy_receive_drop;
    str = "{}{}{}";

    testrun(0 == ov_list_count(dummy.list));

    testrun(!ov_json_io_buffer_push(
        self,
        key,
        (ov_memory_pointer){.start = (uint8_t *)str, .length = strlen(str)}));

    // check we received the first JSON and dropped
    testrun(ov_dict_is_empty(self->dict));
    testrun(1 == ov_list_count(dummy.list));
    testrun(ov_list_clear(dummy.list));

    testrun(NULL == ov_json_io_buffer_free(self));

    testrun(dummy_deinit(&dummy));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_io_buffer_drop() {

    ov_buffer *buffer = NULL;

    struct dummy_userdata dummy;
    testrun(dummy_init(&dummy));

    ov_json_io_buffer_config config =
        (ov_json_io_buffer_config){.callback.userdata = &dummy,
                                   .callback.success = dummy_receive_no_drop,
                                   .callback.failure = dummy_error};

    ov_json_io_buffer *self = ov_json_io_buffer_create(config);
    testrun(self);
    dummy.self = self;

    char *str = "{\"key\":";
    intptr_t key = 1;

    for (size_t i = 1; i < 10; i++) {

        key = i;
        testrun(
            ov_json_io_buffer_push(self,
                                   key,
                                   (ov_memory_pointer){.start = (uint8_t *)str,
                                                       .length = strlen(str)}));

        buffer = ov_dict_get(self->dict, (void *)key);
        testrun(buffer);
        testrun(0 == memcmp(str, buffer->start, buffer->length));
    }

    testrun(9 == ov_dict_count(self->dict));
    testrun(!ov_json_io_buffer_drop(NULL, 1));
    testrun(ov_dict_get(self->dict, (void *)(intptr_t)1));

    testrun(ov_json_io_buffer_drop(self, 1));
    testrun(8 == ov_dict_count(self->dict));
    testrun(!ov_dict_get(self->dict, (void *)(intptr_t)1));

    testrun(ov_json_io_buffer_drop(self, 1));
    testrun(8 == ov_dict_count(self->dict));
    testrun(!ov_dict_get(self->dict, (void *)(intptr_t)1));

    testrun(ov_json_io_buffer_drop(self, 2));
    testrun(7 == ov_dict_count(self->dict));
    testrun(!ov_dict_get(self->dict, (void *)(intptr_t)1));
    testrun(!ov_dict_get(self->dict, (void *)(intptr_t)2));

    testrun(ov_json_io_buffer_drop(self, 3));
    testrun(6 == ov_dict_count(self->dict));
    testrun(!ov_dict_get(self->dict, (void *)(intptr_t)1));
    testrun(!ov_dict_get(self->dict, (void *)(intptr_t)2));
    testrun(!ov_dict_get(self->dict, (void *)(intptr_t)3));

    testrun(NULL == ov_json_io_buffer_free(self));

    testrun(dummy_deinit(&dummy));

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

    testrun_test(test_ov_json_io_buffer_create);
    testrun_test(test_ov_json_io_buffer_free);

    testrun_test(test_ov_json_io_buffer_push);
    testrun_test(test_ov_json_io_buffer_drop);
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
