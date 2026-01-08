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
        @file           ov_json_number.c
        @author         Markus Toepfer

        @date           2018-12-03

        @ingroup        ov_json_number

        @brief          Implementation of 


        ------------------------------------------------------------------------
*/
#include "../../include/ov_json_number.h"

typedef struct {

  ov_json_value head;
  double data;

} JsonNumber;

#define AS_JSON_NUMBER(x)                                                      \
  (((ov_json_value_cast(x) != 0) &&                                            \
    (OV_JSON_NUMBER == ((ov_json_value *)x)->type))                            \
       ? (JsonNumber *)(x)                                                     \
       : 0)

/*----------------------------------------------------------------------------*/

bool json_number_init(JsonNumber *self, double content) {

  if (!self)
    goto error;

  self->head.magic_byte = OV_JSON_VALUE_MAGIC_BYTE;
  self->head.type = OV_JSON_NUMBER;
  self->head.parent = NULL;

  self->data = content;

  self->head.clear = ov_json_number_clear;
  self->head.free = ov_json_number_free;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_json_number(double content) {

  JsonNumber *number = calloc(1, sizeof(JsonNumber));
  if (!number)
    goto error;

  if (json_number_init(number, content))
    return (ov_json_value *)number;

  free(number);
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_json_is_number(const ov_json_value *value) {

  return AS_JSON_NUMBER(value);
}

/*----------------------------------------------------------------------------*/

double ov_json_number_get(const ov_json_value *self) {

  JsonNumber *number = AS_JSON_NUMBER(self);
  if (!number)
    return 0;

  return number->data;
}

/*----------------------------------------------------------------------------*/

bool ov_json_number_set(ov_json_value *self, double content) {

  JsonNumber *number = AS_JSON_NUMBER(self);
  if (!number)
    return false;

  number->data = content;
  return true;
}

/*----------------------------------------------------------------------------*/

bool ov_json_number_clear(void *self) {

  JsonNumber *number = AS_JSON_NUMBER(self);
  if (!number)
    goto error;

  number->data = 0;

  return true;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

void *ov_json_number_free(void *self) {

  JsonNumber *number = AS_JSON_NUMBER(self);
  if (!number)
    return self;

  // in case of parent loop over ov_json_value_free
  if (number->head.parent)
    return ov_json_value_free(self);

  free(number);
  return NULL;
}

/*----------------------------------------------------------------------------*/

void *ov_json_number_copy(void **dest, const void *self) {

  if (!dest || !self)
    goto error;

  JsonNumber *copy = NULL;
  JsonNumber *orig = AS_JSON_NUMBER(self);
  if (!orig)
    goto error;

  if (!*dest) {

    *dest = ov_json_number(orig->data);
    if (*dest)
      return *dest;
  }

  copy = AS_JSON_NUMBER(*dest);
  if (!copy)
    goto error;

  copy->data = orig->data;
  return copy;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_json_number_dump(FILE *stream, const void *self) {

  JsonNumber *orig = AS_JSON_NUMBER(self);
  if (!stream || !orig)
    goto error;

  if (!fprintf(stream, " %f ", orig->data))
    goto error;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

ov_data_function ov_json_number_functions() {

  ov_data_function f = {

      .clear = ov_json_number_clear,
      .free = ov_json_number_free,
      .dump = ov_json_number_dump,
      .copy = ov_json_number_copy};

  return f;
}
