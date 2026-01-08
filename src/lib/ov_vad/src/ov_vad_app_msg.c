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
        @file           ov_vad_app_msg.c
        @author         Markus Töpfer

        @date           2025-01-11


        ------------------------------------------------------------------------
*/
#include "../include/ov_vad_app_msg.h"

#include <ov_base/ov_config_keys.h>
#include <ov_core/ov_event_api.h>

/*---------------------------------------------------------------------------*/

ov_json_value *ov_vad_app_msg_register() {

  ov_json_value *out = ov_event_api_message_create(OV_KEY_REGISTER, NULL, 0);
  return out;
}

/*---------------------------------------------------------------------------*/

ov_json_value *ov_vad_app_msg_loops() {

  ov_json_value *out = ov_event_api_message_create(OV_KEY_LOOPS, NULL, 0);
  return out;
}
