/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_mc_loop_data.c
        @author         Markus

        @date           2022-11-10


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_loop_data.h"

ov_mc_loop_data ov_mc_loop_data_from_json(const ov_json_value *value) {

  ov_mc_loop_data out = {0};

  if (!value)
    goto error;

  const ov_json_value *socket = ov_json_get(value, "/" OV_KEY_SOCKET);
  const char *name = ov_json_string_get(ov_json_get(value, "/" OV_KEY_NAME));

  uint64_t vol = ov_json_number_get(ov_json_get(value, "/" OV_KEY_VOLUME));

  out.socket =
      ov_socket_configuration_from_json(socket, (ov_socket_configuration){0});
  out.socket.type = UDP;

  if (name)
    strncpy(out.name, name, OV_MC_LOOP_NAME_MAX);

  if (vol > 100)
    vol = 100;

  out.volume = vol;

  return out;
error:
  return (ov_mc_loop_data){0};
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_loop_data_to_json(ov_mc_loop_data data) {

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;

  if (0 == data.socket.host[0])
    goto error;
  if (0 == data.name[0])
    goto error;

  out = ov_json_object();

  val = NULL;
  ov_socket_configuration_to_json(data.socket, &val);
  if (!ov_json_object_set(out, OV_KEY_SOCKET, val))
    goto error;

  val = ov_json_string(data.name);
  if (!ov_json_object_set(out, OV_KEY_NAME, val))
    goto error;

  val = ov_json_number(data.volume);
  if (!ov_json_object_set(out, OV_KEY_VOLUME, val))
    goto error;

  return out;
error:
  ov_json_value_free(out);
  ov_json_value_free(val);
  return NULL;
}