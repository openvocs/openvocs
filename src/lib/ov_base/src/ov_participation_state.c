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

#include "../include/ov_participation_state.h"
#include "../include/ov_config_keys.h"
#include "../include/ov_string.h"

/*----------------------------------------------------------------------------*/

char const *ov_participation_state_to_string(ov_participation_state state) {

    switch (state) {

        case OV_PARTICIPATION_STATE_RECV:
            return OV_KEY_RECV;

        case OV_PARTICIPATION_STATE_RECV_OFF:
            return OV_KEY_RECV "_OFF";

        case OV_PARTICIPATION_STATE_SEND:
            return OV_KEY_SEND;

        case OV_PARTICIPATION_STATE_SEND_OFF:
            return OV_KEY_SEND "_OFF";

        case OV_PARTICIPATION_STATE_PTT_OFF:
            return OV_KEY_PTT "_OFF";

        case OV_PARTICIPATION_STATE_PTT:
            return OV_KEY_PTT;

        default:
            return OV_KEY_NONE;
    }
}

/*----------------------------------------------------------------------------*/

ov_participation_state ov_participation_state_from_string(char const *state) {

    if (ov_string_equal(state, OV_KEY_SEND)) {

        return OV_PARTICIPATION_STATE_SEND;

    } else if (ov_string_equal(state, OV_KEY_SEND "_OFF")) {

        return OV_PARTICIPATION_STATE_SEND_OFF;

    } else if (ov_string_equal(state, OV_KEY_RECV)) {

        return OV_PARTICIPATION_STATE_RECV_OFF;

    } else if (ov_string_equal(state, OV_KEY_RECV "_OFF")) {

        return OV_PARTICIPATION_STATE_RECV_OFF;

    } else if (ov_string_equal(state, OV_KEY_PTT "_OFF")) {

        return OV_PARTICIPATION_STATE_PTT_OFF;

    } else if (ov_string_equal(state, OV_KEY_PTT)) {

        return OV_PARTICIPATION_STATE_PTT;

    } else {

        return OV_PARTICIPATION_STATE_NONE;
    }
}

/*----------------------------------------------------------------------------*/
