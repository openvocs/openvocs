/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_event_engine.c
        @author         Markus Töpfer

        @date           2023-10-08


    
*/
#include "../include/ov_event_engine.h"

#include <ov_base/ov_dict.h>

#define OV_EVENT_ENGINE_MAGIC_BYTES 0x1234

struct ov_event_engine {

  uint16_t magic_bytes;
  ov_dict *dict;
};

/*----------------------------------------------------------------------------*/

ov_event_engine *ov_event_engine_create() {

  ov_event_engine *self = calloc(1, sizeof(ov_event_engine));
  if (!self)
    goto error;

  self->magic_bytes = OV_EVENT_ENGINE_MAGIC_BYTES;
  self->dict = ov_dict_create(ov_dict_string_key_config(255));
  if (!self->dict)
    goto error;

  return self;

error:
  ov_event_engine_free(self);
  return NULL;
}

/*----------------------------------------------------------------------------*/

ov_event_engine *ov_event_engine_free(ov_event_engine *self) {

  if (!ov_event_engine_cast(self))
    goto error;

  self->dict = ov_dict_free(self->dict);
  self = ov_data_pointer_free(self);

error:
  return self;
}

/*----------------------------------------------------------------------------*/

ov_event_engine *ov_event_engine_cast(const void *self) {

  if (!self)
    goto error;

  if (*(uint16_t *)self == OV_EVENT_ENGINE_MAGIC_BYTES)
    return (ov_event_engine *)self;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_event_engine_register(
    ov_event_engine *self, const char *name,
    bool (*process)(void *userdata, const int socket,
                    const ov_event_parameter *parameter,
                    ov_json_value *input)) {

  if (!self || !name || !process)
    goto error;

  char *key = strdup(name);
  if (!ov_dict_set(self->dict, key, process, NULL)) {
    key = ov_data_pointer_free(key);
    goto error;
  }

  return true;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_event_engine_unregister(ov_event_engine *self, const char *name) {

  if (!self || !name)
    goto error;

  return ov_dict_del(self->dict, name);
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_event_engine_push(ov_event_engine *self, void *userdata, int socket,
                          ov_event_parameter parameter, ov_json_value *input) {

  if (!self || !input || !userdata)
    goto error;

  const char *event = ov_event_api_get_event(input);
  if (!event)
    goto error;

  bool (*process)(void *userdata, const int socket,
                  const ov_event_parameter *parameter, ov_json_value *input) =
      ov_dict_get(self->dict, event);

  if (!process)
    goto error;

  return process(userdata, socket, &parameter, input);
error:
  return false;
}
