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
        @file           ov_vocs_connection.c
        @author         Markus Töpfer

        @date           2021-10-09


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_connection.h"

#include <ov_base/ov_data_function.h>
#include <ov_base/ov_socket.h>

#include <ov_base/ov_utils.h>

/*----------------------------------------------------------------------------*/

bool ov_vocs_connection_clear(ov_vocs_connection *self) {

  if (!self)
    goto error;

  self->remote = (ov_socket_data){0};
  self->local = (ov_socket_data){0};

  self->data = ov_json_value_free(self->data);
  self->socket = -1;

  self->media_ready = false;
  self->ice_success = false;
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_connection_is_authenticated(ov_vocs_connection *self) {

  if (ov_vocs_connection_get_user(self))
    return true;
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_connection_is_authorized(ov_vocs_connection *self) {

  if (ov_vocs_connection_get_role(self))
    return true;
  return false;
}

/*----------------------------------------------------------------------------*/

const char *ov_vocs_connection_get_user(ov_vocs_connection *self) {

  if (!self)
    return NULL;

  return ov_json_string_get(ov_json_get(self->data, "/" OV_KEY_USER));
}

/*----------------------------------------------------------------------------*/

const char *ov_vocs_connection_get_role(ov_vocs_connection *self) {

  if (!self)
    return NULL;

  return ov_json_string_get(ov_json_get(self->data, "/" OV_KEY_ROLE));
}

/*----------------------------------------------------------------------------*/

const char *ov_vocs_connection_get_client(ov_vocs_connection *self) {

  if (!self)
    return NULL;

  return ov_json_string_get(ov_json_get(self->data, "/" OV_KEY_CLIENT));
}

/*----------------------------------------------------------------------------*/

const char *ov_vocs_connection_get_session(ov_vocs_connection *self) {

  if (!self)
    return NULL;

  return ov_json_string_get(ov_json_get(self->data, "/" OV_KEY_SESSION));
}

/*----------------------------------------------------------------------------*/

static bool set_key_value(ov_vocs_connection *self, const char *key,
                          const char *val) {

  ov_json_value *out = NULL;

  if (!self || !key || !val)
    goto error;

  if (!self->data)
    self->data = ov_json_object();

  out = ov_json_string(val);
  if (!ov_json_object_set(self->data, key, out))
    goto error;

  return true;
error:
  ov_json_value_free(out);
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_connection_set_user(ov_vocs_connection *self, const char *val) {

  if (!self)
    return false;

  ov_log_info("set %i user %s", self->socket, val);

  return set_key_value(self, OV_KEY_USER, val);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_connection_set_role(ov_vocs_connection *self, const char *val) {

  if (!self)
    return false;

  ov_log_info("set %i role %s", self->socket, val);

  return set_key_value(self, OV_KEY_ROLE, val);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_connection_set_session(ov_vocs_connection *self, const char *val) {

  if (!self)
    return false;

  ov_log_info("set %i session %s", self->socket, val);

  return set_key_value(self, OV_KEY_SESSION, val);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_connection_set_client(ov_vocs_connection *self, const char *val) {

  if (!self)
    return false;

  ov_log_info("set %i client %s", self->socket, val);

  return set_key_value(self, OV_KEY_CLIENT, val);
}

/*----------------------------------------------------------------------------*/

const ov_json_value *get_loop(ov_vocs_connection *self, const char *id) {

  if (!self || !id)
    return NULL;

  ov_json_value const *loops = ov_json_get(self->data, "/" OV_KEY_LOOPS);
  return ov_json_object_get(loops, id);
}

/*----------------------------------------------------------------------------*/

ov_vocs_permission ov_vocs_connection_get_loop_state(ov_vocs_connection *self,
                                                     const char *id) {

  ov_json_value *state = ov_json_object_get(get_loop(self, id), OV_KEY_STATE);
  return ov_vocs_permission_from_string(ov_json_string_get(state));
}

/*----------------------------------------------------------------------------*/

uint8_t ov_vocs_connection_get_loop_volume(ov_vocs_connection *self,
                                           const char *id) {

  ov_json_value *nbr = ov_json_object_get(get_loop(self, id), OV_KEY_VOLUME);

  uint64_t num = (uint64_t)ov_json_number_get(nbr);
  OV_ASSERT(num < 255);
  return (uint8_t)num;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *get_set_loop(ov_vocs_connection *self, const char *id) {

  // I wonder how this never produces a MEM corruption -
  // it returns either a new object or a pointer to an existing object -
  // how is the caller supposed to know whether the returned value has to
  // be freed?
  ov_json_value *loops = NULL;
  ov_json_value *loop = NULL;

  if (!self || !id)
    goto error;

  if (!self->data)
    self->data = ov_json_object();

  loops = (ov_json_value *)ov_json_get(self->data, "/" OV_KEY_LOOPS);

  if (!loops) {

    loops = ov_json_object();
    if (!ov_json_object_set(self->data, OV_KEY_LOOPS, loops))
      goto error;
  }

  loop = ov_json_object_get(loops, id);
  if (!loop) {

    loop = ov_json_object();
    if (!ov_json_object_set(loops, id, loop))
      goto error;
  }

  return loop;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_connection_set_loop_state(ov_vocs_connection *self, const char *id,
                                       ov_vocs_permission state) {

  ov_json_value *loop = get_set_loop(self, id);
  if (!loop)
    goto error;

  const char *str = ov_vocs_permission_to_string(state);
  /*
      const char *user = ov_vocs_connection_get_user(self);
      const char *role = ov_vocs_connection_get_role(self);
      const char *sess = ov_vocs_connection_get_session(self);


      ov_log_info("set %i|%s user %s role %s loop %s state %s",
                  self->socket,
                  sess,
                  user,
                  role,
                  id,
                  str);
  */
  ov_json_value *val = ov_json_string(str);
  if (!ov_json_object_set(loop, OV_KEY_STATE, val)) {
    val = ov_json_value_free(val);
    goto error;
  }
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_connection_set_loop_volume(ov_vocs_connection *self,
                                        const char *id, uint8_t num) {

  ov_json_value *loop = get_set_loop(self, id);
  if (!loop || num > 100)
    goto error;
  /*
      const char *user = ov_vocs_connection_get_user(self);
      const char *role = ov_vocs_connection_get_role(self);
      const char *sess = ov_vocs_connection_get_session(self);

      ov_log_info("set %i|%s user %s role %s loop %s volume %i",
                  self->socket,
                  sess,
                  user,
                  role,
                  id,
                  num);
  */
  ov_json_value *val = ov_json_number(num);
  if (!ov_json_object_set(loop, OV_KEY_VOLUME, val)) {
    val = ov_json_value_free(val);
    goto error;
  }
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_connection_get_state(const ov_vocs_connection *self) {

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;

  if (!self || !self->data)
    goto error;

  if (self->data) {

    if (!ov_json_value_copy((void **)&out, self->data))
      goto error;

  } else {

    out = ov_json_object();
  }

  val = ov_json_number(self->socket);
  if (!ov_json_object_set(out, OV_KEY_SOCKET, val))
    goto error;
  /*
      val = ov_socket_data_to_json(&self->local);
      if (!ov_json_object_set(out, OV_KEY_LOCAL, val)) goto error;
  */
  val = ov_socket_data_to_json(&self->remote);
  if (!ov_json_object_set(out, OV_KEY_REMOTE, val))
    goto error;

  return out;
error:
  ov_json_value_free(val);
  ov_json_value_free(out);
  return NULL;
}
