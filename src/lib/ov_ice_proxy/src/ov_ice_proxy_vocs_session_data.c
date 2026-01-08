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
        @file           ov_ice_proxy_vocs_session_data.c
        @author         Markus

        @date           2024-05-08


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_proxy_vocs_session_data.h"

#include "../include/ov_ice_proxy_vocs_stream_forward.h"

ov_ice_proxy_vocs_session_data
ov_ice_proxy_vocs_session_data_clear(ov_ice_proxy_vocs_session_data *self) {

  if (!self)
    goto error;

  if (self->desc)
    self->desc = ov_sdp_session_free(self->desc);

error:
  return (ov_ice_proxy_vocs_session_data){0};
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_session_data_description_to_json(
    const ov_ice_proxy_vocs_session_data *data) {

  if (!data)
    goto error;
  if (!data->desc)
    goto error;

  return ov_sdp_session_to_json(data->desc);

error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *
stream_create_proxy_data(const ov_ice_proxy_vocs_session_data *in) {

  /*
      {
          "socket" : {
              host" : "<internal host>",
              "port" : <internal port>,
              "type" : "UDP"
          },
          "ssrc" : <ssrc>
      }
  */

  if (!in)
    goto error;

  ov_ice_proxy_vocs_stream_forward_data data =
      (ov_ice_proxy_vocs_stream_forward_data){
          .ssrc = in->ssrc,
          (ov_socket_configuration){.type = UDP, .port = in->proxy.port}};

  memcpy(data.socket.host, in->proxy.host, OV_HOST_NAME_MAX);

  return ov_ice_proxy_vocs_stream_forward_data_to_json(data);

error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_session_data_to_json(
    const ov_ice_proxy_vocs_session_data *data) {

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;

  if (!data)
    goto error;

  out = ov_json_array();
  val = stream_create_proxy_data(data);

  if (!ov_json_array_push(out, val))
    goto error;

  return out;
error:
  ov_json_value_free(out);
  ov_json_value_free(val);
  return NULL;
}