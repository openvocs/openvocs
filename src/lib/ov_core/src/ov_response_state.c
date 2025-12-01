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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "../include/ov_response_state.h"
#include "../include/ov_event_api.h"

/*----------------------------------------------------------------------------*/

bool ov_response_state_from_message(ov_json_value const *response,
                                    ov_response_state *state) {
  bool result = false;

  if ((0 == response) || (0 == state)) {

    ov_log_error("Got 0 pointer");
    goto error;
  }

  if (0 == ov_event_api_get_request(response)) {

    ov_log_error("Message is not a response");
    goto error;
  }

  ov_json_value *error = ov_event_api_get_error(response);

  char const *message = (char *)ov_json_string_get(
      ov_json_get(error, "/" OV_EVENT_API_KEY_DESCRIPTION));

  *state = (ov_response_state){
      .id = ov_event_api_get_uuid(response),
      .result.error_code = ov_event_api_get_error_code(response),
      .result.message = message,
  };

  result = true;

error:

  return result;
}

/*----------------------------------------------------------------------------*/
