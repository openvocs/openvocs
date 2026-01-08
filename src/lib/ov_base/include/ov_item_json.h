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
        @file           ov_item_json.h
        @author         Töpfer, Markus

        @date           2025-11-29


        ------------------------------------------------------------------------
*/
#ifndef ov_item_json_h
#define ov_item_json_h

#include "ov_item.h"
#include "ov_json_grammar.h"

/*---------------------------------------------------------------------------*/

ov_item *ov_item_from_json(const char *string);
char *ov_item_to_json(const ov_item *self);

/*
 *      ------------------------------------------------------------------------
 *
 *      STRINGIFY CONFIGS
 *
 *      ... full customizable stringify setup
 *
 *      ------------------------------------------------------------------------
 */

typedef struct ov_item_config {

  struct entry {

    char *intro;  //      ... item intro e.g. space, quote
    char *outro;  //      ... item intro e.g. space, quote
    bool depth;   //      ... enable depth
    char *indent; //      ... whitespace indent (e.g. tab)
  } entry;

  struct item {

    char *intro;     // {    ... intro after depth
    char *out;       // \n   ... out before depth
    char *outro;     // }    ... outro after depth
    char *separator; // ,    ... separation of collection items
    char *delimiter; // :    ... delimiter within object

  } item;

} ov_item_json_config;

/*----------------------------------------------------------------------------*/

typedef struct ov_item_json_stringify_config {

  char *intro;

  ov_item_json_config literal;
  ov_item_json_config number;
  ov_item_json_config string;
  ov_item_json_config array;
  ov_item_json_config object;

  char *outro;

} ov_item_json_stringify_config;

/*----------------------------------------------------------------------------*/

ov_item_json_stringify_config ov_item_json_config_stringify_minimal();
ov_item_json_stringify_config ov_item_json_config_stringify_default();

/*----------------------------------------------------------------------------*/

char *ov_item_to_json_with_config(const ov_item *self,
                                  ov_item_json_stringify_config config);

#endif /* ov_item_json_h */
