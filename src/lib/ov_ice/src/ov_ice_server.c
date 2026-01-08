/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_ice_server.c
        @author         Markus

        @date           2024-09-18


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_server.h"
#include "../include/ov_ice_string.h"

/*----------------------------------------------------------------------------*/

const char *ov_ice_server_type_to_string(ov_ice_server_type type) {

  switch (type) {

  case OV_ICE_STUN_SERVER:
    return OV_ICE_STRING_STUN;

  case OV_ICE_TURN_SERVER:
    return OV_ICE_STRING_TURN;
  }

  return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice_server_type ov_ice_server_type_from_string(const char *string) {

  if (!string)
    goto error;

  if (0 == strncmp(string, OV_ICE_STRING_TURN, strlen(OV_ICE_STRING_TURN)))
    return OV_ICE_TURN_SERVER;

error:
  return OV_ICE_STUN_SERVER;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_server_config_to_json(const ov_ice_server *config) {

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;

  if (!config)
    goto error;

  out = ov_json_object();
  if (!out)
    goto error;

  const char *type = OV_ICE_STRING_STUN;
  if (config->type == OV_ICE_TURN_SERVER)
    type = OV_ICE_STRING_TURN;

  val = ov_json_string(type);
  if (!ov_json_object_set(out, OV_KEY_TYPE, val))
    goto error;

  val = NULL;
  if (!ov_socket_configuration_to_json(config->socket, &val))
    goto error;

  if (!ov_json_object_set(out, OV_KEY_SOCKET, val))
    goto error;

  if (0 != config->auth.user[0]) {
    val = ov_json_string(config->auth.user);
    if (!ov_json_object_set(out, OV_KEY_USER, val))
      goto error;
  }

  if (0 != config->auth.pass[0]) {
    val = ov_json_string(config->auth.pass);
    if (!ov_json_object_set(out, OV_KEY_PASSWORD, val))
      goto error;
  }

  return out;
error:
  ov_json_value_free(out);
  ov_json_value_free(val);
  return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice_server ov_ice_server_config_from_json(ov_json_value *value) {

  ov_ice_server config = {0};

  if (!value)
    goto done;

  ov_json_value *conf = ov_json_object_get(value, OV_KEY_SERVER);

  if (!conf)
    conf = value;

  config.type = ov_ice_server_type_from_string(
      ov_json_string_get(ov_json_object_get(conf, OV_KEY_TYPE)));

  config.socket = ov_socket_configuration_from_json(
      ov_json_object_get(conf, OV_KEY_SOCKET), (ov_socket_configuration){0});

  const char *string =
      ov_json_string_get(ov_json_object_get(conf, OV_KEY_USER));

  if (string)
    strncpy(config.auth.user, string, OV_ICE_STUN_USER_MAX);

  string = ov_json_string_get(ov_json_object_get(conf, OV_KEY_PASSWORD));

  if (string)
    strncpy(config.auth.pass, string, OV_ICE_STUN_PASS_MAX);
done:
  return config;
}

/*----------------------------------------------------------------------------*/

ov_ice_server *ov_ice_server_create(ov_event_loop *loop) {

  ov_ice_server *self = calloc(1, sizeof(ov_ice_server));
  self->node.type = OV_ICE_SERVER_MAGIC_BYTES;
  self->loop = loop;

  return self;
}

/*----------------------------------------------------------------------------*/

ov_ice_server *ov_ice_server_cast(const void *data) {

  if (!data)
    goto error;

  ov_node *node = (ov_node *)data;

  if (node->type == OV_ICE_SERVER_MAGIC_BYTES)
    return (ov_ice_server *)data;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

void *ov_ice_server_free(void *self) {

  ov_ice_server *server = ov_ice_server_cast(self);
  if (!server)
    goto error;

  if (OV_TIMER_INVALID != server->timer.keepalive) {

    ov_event_loop_timer_unset(server->loop, server->timer.keepalive, NULL);

    server->timer.keepalive = OV_TIMER_INVALID;
  }

  server = ov_data_pointer_free(server);
error:
  return self;
}