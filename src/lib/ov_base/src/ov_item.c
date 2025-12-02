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
        @file           ov_item.c
        @author         Töpfer, Markus

        @date           2025-11-28


        ------------------------------------------------------------------------
*/
#include "../include/ov_item.h"

#include <stdlib.h>
#include <string.h>

#include "../include/ov_data_function.h"
#include "../include/ov_dict.h"
#include "../include/ov_linked_list.h"
#include "../include/ov_string.h"
#include <ov_log/ov_log.h>

/*---------------------------------------------------------------------------*/

#define OV_ITEM_MAGIC_BYTES 0x17e9

/*---------------------------------------------------------------------------*/

typedef enum ov_item_literal {

  OV_ITEM_NULL = 0,
  OV_ITEM_TRUE = 1,
  OV_ITEM_FALSE = 2,
  OV_ITEM_ARRAY = 3,
  OV_ITEM_OBJECT = 4,
  OV_ITEM_STRING = 6,
  OV_ITEM_NUMBER = 7

} ov_item_literal;

/*---------------------------------------------------------------------------*/

typedef struct ov_item_config {

  ov_item_literal literal;

  union {
    double number;
    void *data;
  };

  struct {

    uint64_t thread_lock_timeout_usecs;

  } limits;

} ov_item_config;

/*---------------------------------------------------------------------------*/

struct ov_item {

  uint16_t magic_bytes;
  ov_item_config config;
  ov_item *parent;
};

/*---------------------------------------------------------------------------*/

static ov_item root_item = (ov_item){
                                     .magic_bytes = OV_ITEM_MAGIC_BYTES,
                                     .config = (ov_item_config){0},
                                     .parent = NULL
                                   };

/*---------------------------------------------------------------------------*/

static ov_item *item_create(ov_item_config config) {

  ov_item *item = calloc(1, sizeof(ov_item));
  if (!item)
    goto error;

  memcpy(item, &root_item, sizeof(ov_item));

  item->config = config;
  item->parent = NULL;

  return item;
error:
  ov_item_free(item);
  return NULL;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_cast(const void *self) {

  if (!self)
    return NULL;

  if (*(uint16_t *)self == OV_ITEM_MAGIC_BYTES)
    return (ov_item *)self;

  return NULL;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_get_parent(const ov_item *self) {

  if (!self)
    return NULL;
  return self->parent;
}

/*---------------------------------------------------------------------------*/

void *ov_item_free(void *self) {

  ov_item *item = ov_item_cast(self);
  if (!item)
    return self;

  switch (item->config.literal) {

  case OV_ITEM_NULL:
  case OV_ITEM_TRUE:
  case OV_ITEM_FALSE:
  case OV_ITEM_NUMBER:
    break;

  case OV_ITEM_STRING:

    if (item->config.data)
      item->config.data = ov_data_pointer_free(item->config.data);

    break;

  case OV_ITEM_ARRAY:

    if (item->config.data)
      item->config.data = ov_list_free(item->config.data);

    break;

  case OV_ITEM_OBJECT:

    if (item->config.data)
      item->config.data = ov_dict_free(item->config.data);

    break;

  default:
    break;
  }

  item = ov_data_pointer_free(item);
  return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_item_clear(void *self) {

  ov_item *item = ov_item_cast(self);
  if (!item)
    return false;

  switch (item->config.literal) {

  case OV_ITEM_NULL:
  case OV_ITEM_TRUE:
  case OV_ITEM_FALSE:
  case OV_ITEM_NUMBER:
    break;

  case OV_ITEM_STRING:

    if (item->config.data)
      item->config.data = ov_data_pointer_free(item->config.data);

    break;

  case OV_ITEM_ARRAY:

    if (item->config.data)
      item->config.data = ov_list_free(item->config.data);

    break;

  case OV_ITEM_OBJECT:

    if (item->config.data)
      item->config.data = ov_dict_free(item->config.data);

    break;

  default:
    break;
  }

  item->config.literal = OV_ITEM_NULL;
  item->config.number = 0;

  return true;
}

/*---------------------------------------------------------------------------*/

bool ov_item_dump(FILE *stream, const void *self) {

  if (!stream || !self)
    return false;
  ov_item *item = ov_item_cast(self);
  if (!item)
    goto error;

  switch (item->config.literal) {

  case OV_ITEM_NULL:

    if (!fprintf(stream, "\nnull"))
      goto error;
    break;

  case OV_ITEM_TRUE:

    if (!fprintf(stream, "\ntrue"))
      goto error;
    break;

  case OV_ITEM_FALSE:

    if (!fprintf(stream, "\nfalse"))
      goto error;
    break;

  case OV_ITEM_NUMBER:

    if (!fprintf(stream, "\n%f", item->config.number))
      goto error;
    break;

  case OV_ITEM_STRING:

    if (item->config.data)
      if (!fprintf(stream, "\n%s", (char *)item->config.data))
        goto error;

    break;

  case OV_ITEM_ARRAY:

    if (item->config.data)
      ov_list_dump(stream, item->config.data);

    break;

  case OV_ITEM_OBJECT:

    if (item->config.data)
      ov_dict_dump(stream, item->config.data);

    break;

  default:
    break;
  }

  return true;
error:
  return false;
}

/*---------------------------------------------------------------------------*/

void *ov_item_copy(void **destination, const void *self) {

  ov_item *copy = NULL;

  if (!self)
    goto error;

  ov_item *source = ov_item_cast(self);
  if (!source)
    goto error;

  copy = NULL;

  bool result = true;

  switch (source->config.literal) {

  case OV_ITEM_NULL:
    copy = ov_item_null();
    break;
  case OV_ITEM_TRUE:
    copy = ov_item_true();
    break;
  case OV_ITEM_FALSE:
    copy = ov_item_false();
    break;

  case OV_ITEM_NUMBER:

    copy = ov_item_number(source->config.number);
    break;

  case OV_ITEM_STRING:

    copy = ov_item_string(source->config.data);

    break;

  case OV_ITEM_ARRAY:

    copy = ov_item_array();

    if (!ov_list_copy((void **)&copy->config.data, source->config.data)) {

      result = false;
    }

    break;

  case OV_ITEM_OBJECT:

    copy = ov_item_object();

    if (!ov_dict_copy((void **)&copy->config.data, source->config.data)) {

      result = false;
    }

    break;

  default:
    break;
  }

  if (!result)
    goto error;
  *destination = copy;
  return copy;
error:
  copy = ov_item_free(copy);
  return NULL;
}

/*---------------------------------------------------------------------------*/

size_t ov_item_count(ov_item *self) {

  if (!self)
    goto error;

  uint64_t count = 0;

  switch (self->config.literal) {

  case OV_ITEM_NULL:
  case OV_ITEM_TRUE:
  case OV_ITEM_FALSE:
  case OV_ITEM_NUMBER:
  case OV_ITEM_STRING:

    count = 1;
    break;

  case OV_ITEM_ARRAY:

    count = ov_list_count(self->config.data);
    break;

  case OV_ITEM_OBJECT:

    count = ov_dict_count(self->config.data);
    break;

  default:
    break;
  }

  return count;
error:
  return 0;
}

/*---------------------------------------------------------------------------*/

bool ov_item_is_empty(ov_item *self) {

  size_t count = ov_item_count(self);
  if (0 == count)
    return true;
  return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #OBJECT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_item *ov_item_object() {

  ov_item *item = item_create(
      (ov_item_config){.literal = OV_ITEM_OBJECT,
                       .limits.thread_lock_timeout_usecs =
                           root_item.config.limits.thread_lock_timeout_usecs});

  if (!item)
    goto error;

  ov_dict_config d_config = ov_dict_string_key_config(255);
  d_config.value.data_function.free = ov_item_free;
  d_config.value.data_function.clear = ov_item_clear;
  d_config.value.data_function.dump = ov_item_dump;
  d_config.value.data_function.copy = ov_item_copy;

  item->config.data = ov_dict_create(d_config);
  if (!item->config.data)
    goto error;

  return item;
error:
  ov_item_free(item);
  return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_item_is_object(ov_item *self) {

  if (!self)
    goto error;

  if (self->config.literal == OV_ITEM_OBJECT)
    return true;

error:
  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_item_object_set(ov_item *self, const char *string, ov_item *val) {

  char *key = NULL;

  if (!ov_item_is_object(self) || !string || !val)
    goto error;

  key = ov_string_dup(string);

  if (!ov_dict_set(self->config.data, key, val, NULL))
    goto error;

  val->parent = self;
  return true;

error:
  key = ov_data_pointer_free(key);
  return false;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_object_get(ov_item *self, const char *string) {

  ov_item *out = NULL;

  if (!ov_item_is_object(self) || !string)
    goto error;

  out = ov_item_cast(ov_dict_get(self->config.data, (void *)string));

  return out;
error:
  return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_item_object_for_each(ov_item *self,
                             bool (*function)(char const *key,
                                              ov_item const *val,
                                              void *userdata),
                             void *userdata) {

  if (!ov_item_is_object(self) || !function)
    goto error;

  bool result = ov_dict_for_each(self->config.data, userdata, (void *)function);

  return result;
error:
  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_item_object_delete(ov_item *self, const char *string) {

  if (!ov_item_is_object(self) || !string)
    goto error;

  bool result = ov_dict_del(self->config.data, string);

  return result;
error:
  return false;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_object_remove(ov_item *self, const char *string) {

  if (!ov_item_is_object(self) || !string)
    goto error;

  ov_item *out = NULL;

  out = ov_dict_remove(self->config.data, string);

  if (out)
    out->parent = NULL;

  return out;
error:
  return NULL;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #ARRAY FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_item *ov_item_array() {

  ov_item *item = item_create(
      (ov_item_config){.literal = OV_ITEM_ARRAY,
                       .limits.thread_lock_timeout_usecs =
                           root_item.config.limits.thread_lock_timeout_usecs});

  if (!item)
    goto error;

  ov_list_config l_config = (ov_list_config){.item.free = ov_item_free,
                                             .item.clear = ov_item_clear,
                                             .item.dump = ov_item_dump,
                                             .item.copy = ov_item_copy};

  item->config.data = ov_linked_list_create(l_config);
  if (!item->config.data)
    goto error;

  return item;
error:
  ov_item_free(item);
  return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_item_is_array(ov_item *self) {

  if (!self)
    goto error;

  if (self->config.literal == OV_ITEM_ARRAY)
    return true;

error:
  return false;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_array_get(ov_item *self, uint64_t pos) {

  ov_item *out = NULL;

  if (!self)
    goto error;

  out = ov_list_get(self->config.data, pos);

  return out;
error:
  return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_item_array_set(ov_item *self, uint64_t pos, ov_item *val) {

  if (!self)
    goto error;
  ov_item *out = NULL;

  bool result = ov_list_set(self->config.data, pos, val, (void **)&out);

  if (out)
    out = ov_item_free(out);

  val->parent = self;
  return result;
error:
  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_item_array_push(ov_item *self, ov_item *val) {

  if (!self)
    goto error;

  bool result = ov_list_push(self->config.data, val);

  val->parent = self;
  return result;
error:
  return false;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_array_stack_pop(ov_item *self) {

  ov_item *out = NULL;

  if (!self)
    goto error;

  out = ov_list_pop(self->config.data);

  if (out)
    out->parent = NULL;

  return out;
error:
  return false;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_array_lifo(ov_item *self) {

  return ov_item_array_stack_pop(self);
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_array_queue_pop(ov_item *self) {

  ov_item *out = NULL;

  if (!self)
    goto error;

  out = ov_list_remove(self->config.data, 1);
  if (out)
    out->parent = NULL;

  return out;
error:
  return false;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_array_fifo(ov_item *self) {

  return ov_item_array_queue_pop(self);
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #STRING FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_item *ov_item_string(const char *string) {

  if (!string)
    return NULL;

  ov_item *item = item_create(
      (ov_item_config){.literal = OV_ITEM_STRING,
                       .limits.thread_lock_timeout_usecs =
                           root_item.config.limits.thread_lock_timeout_usecs});

  if (!item)
    goto error;

  item->config.data = ov_string_dup(string);

  return item;
error:
  ov_item_free(item);
  return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_item_is_string(ov_item *self) {

  if (!self)
    goto error;

  if (self->config.literal == OV_ITEM_STRING)
    return true;

error:
  return false;
}

/*---------------------------------------------------------------------------*/

const char *ov_item_get_string(ov_item *self) {

  if (!self)
    goto error;
  if (!ov_item_is_string(self))
    goto error;

  return (const char *)self->config.data;

error:
  return 0;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #NUMBER FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_item *ov_item_number(double number) {

  ov_item *item = item_create(
      (ov_item_config){.literal = OV_ITEM_NUMBER,
                       .limits.thread_lock_timeout_usecs =
                           root_item.config.limits.thread_lock_timeout_usecs});

  if (!item)
    goto error;

  item->config.number = number;

  return item;
error:
  ov_item_free(item);
  return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_item_set_number(ov_item *self, double number) {

  if (ov_item_is_number(self)) {
    self->config.number = number;
    return true;
  }

  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_item_is_number(ov_item *self) {

  if (!self)
    goto error;

  if (self->config.literal == OV_ITEM_NUMBER)
    return true;

error:
  return false;
}

/*---------------------------------------------------------------------------*/

double ov_item_get_number(ov_item *self) {

  if (!self)
    goto error;
  if (!ov_item_is_number(self))
    goto error;

  return self->config.number;

error:
  return 0;
}

/*---------------------------------------------------------------------------*/

int64_t ov_item_get_int(ov_item *self) {

  if (!self)
    goto error;
  if (!ov_item_is_number(self))
    goto error;

  return (int64_t)self->config.number;

error:
  return 0;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #LITERAL FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_item *ov_item_null() {

  ov_item *item = item_create(
      (ov_item_config){.literal = OV_ITEM_NULL,
                       .limits.thread_lock_timeout_usecs =
                           root_item.config.limits.thread_lock_timeout_usecs});

  if (!item)
    goto error;

  return item;
error:
  ov_item_free(item);
  return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_item_is_null(ov_item *self) {

  if (!self)
    goto error;

  if (self->config.literal == OV_ITEM_NULL)
    return true;

error:
  return false;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_true() {

  ov_item *item = item_create(
      (ov_item_config){.literal = OV_ITEM_TRUE,
                       .limits.thread_lock_timeout_usecs =
                           root_item.config.limits.thread_lock_timeout_usecs});

  if (!item)
    goto error;

  return item;
error:
  ov_item_free(item);
  return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_item_is_true(ov_item *self) {

  if (!self)
    goto error;

  if (self->config.literal == OV_ITEM_TRUE)
    return true;

error:
  return false;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_false() {

  ov_item *item = item_create(
      (ov_item_config){.literal = OV_ITEM_FALSE,
                       .limits.thread_lock_timeout_usecs =
                           root_item.config.limits.thread_lock_timeout_usecs});

  if (!item)
    goto error;

  return item;
error:
  ov_item_free(item);
  return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_item_is_false(ov_item *self) {

  if (!self)
    goto error;

  if (self->config.literal == OV_ITEM_FALSE)
    return true;

error:
  return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #POINTER FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static ov_item *object_get_with_length(ov_item *object, const char *key,
                                       size_t length) {

  if (!object || !key || length < 1)
    return NULL;

  char buffer[length + 1];
  memset(buffer, 0, length + 1);

  if (!strncat(buffer, key, length))
    goto error;

  return ov_item_object_get(object, buffer);
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static bool pointer_parse_token(const char *start, size_t size, char **token,
                                size_t *length) {

  if (!start || size < 1 || !token || !length)
    return false;

  if (start[0] != '/')
    goto error;

  if (size == 1) {
    *token = (char *)start;
    return true;
  }

  *token = (char *)start + 1;

  size_t len = 0;

  while (len < (size - 1)) {

    switch ((*token + len)[0]) {

    case '/':
    case '\0':
      goto done;
      break;

    default:
      len++;
    }
  }

done:
  *length = len;
  return true;

error:
  *token = NULL;
  *length = 0;
  return false;
}

/*----------------------------------------------------------------------------*/

static bool pointer_replace_special_encoding(char *result, size_t size,
                                             char *source, char *to_replace,
                                             char *replacement) {

  if (!result || !source || !to_replace || !replacement || size < 1)
    return false;

  size_t length_replace = strlen(to_replace);
  size_t length_replacement = strlen(replacement);

  uint64_t i = 0;
  uint64_t k = 0;
  uint64_t r = 0;

  for (i = 0; i < size; i++) {

    if (source[i] != to_replace[0]) {
      result[r] = source[i];
      r++;
      continue;
    }

    if ((i + length_replace) > size)
      goto error;

    if ((i + length_replacement) > size)
      goto error;

    for (k = 0; k < length_replace; k++) {

      if (source[i + k] != to_replace[k])
        break;
    }

    if (k == (length_replace)) {

      if (!memcpy(result + r, replacement, length_replacement))
        goto error;

      i += length_replace - 1;
      r += length_replacement;

    } else {

      // not a full match
      result[r] = source[i];
      r++;
    }
  }

  return true;
error:
  if (result) {
    memset(result, '\0', size);
  }

  return false;
}

/*----------------------------------------------------------------------------*/

static bool pointer_escape_token(char *start, size_t size, char *token,
                                 size_t length) {

  if (!start || size < 1 || !token || length < size)
    return false;

  // replacements ~1 -> \  and ~0 -> ~ (both smaller then source)

  char replace1[size + 1];
  memset(replace1, '\0', size + 1);

  if (!pointer_replace_special_encoding(replace1, size, (char *)start, "~1",
                                        "\\"))
    return false;

  if (!pointer_replace_special_encoding((char *)token, size, replace1, "~0",
                                        "~"))
    return false;

  return true;
}

/*----------------------------------------------------------------------------*/

static ov_item *pointer_get_token_in_parent(ov_item *parent, char *token,
                                            size_t length) {

  if (!parent || !token)
    return NULL;

  if (length == 0)
    return parent;

  ov_item *result = NULL;
  char *ptr = NULL;
  uint64_t number = 0;

  switch (parent->config.literal) {

  case OV_ITEM_OBJECT:

    // token is the keyname
    result = object_get_with_length(parent, token, length);

    break;

  case OV_ITEM_ARRAY:

    if (token[0] == '-') {

      // new array member after last array element
      if (strnlen((char *)token, length) == 1) {

        result = ov_item_null();

        if (!ov_item_array_push(parent, result))
          result = ov_item_free(result);
      }

    } else {

      // parse for INT64
      number = strtoll((char *)token, &ptr, 10);
      if (ptr[0] != '\0')
        return NULL;

      result = ov_item_array_get(parent, number + 1);
    }

    break;

  default:
    return NULL;
  }

  return result;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_get(const ov_item *self, const char *pointer) {

  if (!self)
    return NULL;
  if (!pointer)
    return (ov_item *)self;

  ov_item *result = NULL;
  size_t size = strlen(pointer);

  if (0 == size)
    return (ov_item *)self;

  bool parsed = false;

  char *parse = NULL;
  size_t length = 0;

  char token[size + 1];
  memset(token, 0, size + 1);
  char *token_ptr = token;

  const char *ptr = pointer;

  result = (ov_item *)self;

  while (pointer_parse_token(ptr, size, &parse, &length)) {

    if (length == 0)
      break;

    parsed = true;
    ptr = ptr + length + 1;

    memset(token, 0, size + 1);
    token_ptr = token;

    if (!pointer_escape_token(parse, length, token_ptr, length))
      goto error;

    result = pointer_get_token_in_parent(result, token_ptr, length);

    if (!result)
      goto error;
  }

  if (!parsed)
    goto error;

  return result;

error:
  return NULL;
}