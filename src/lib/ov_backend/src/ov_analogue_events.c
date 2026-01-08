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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "../include/ov_analogue_events.h"
#include "ov_base/ov_socket.h"
#include <netinet/in.h>
#include <ov_base/ov_config.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_mc_socket.h>
#include <ov_base/ov_string.h>

/*----------------------------------------------------------------------------*/

char const *ov_analogue_event_type_to_string(ov_analogue_event_type type) {

    switch (type) {

    case ANALOGUE_IN:
        return "IN";

    case ANALOGUE_OUT:
        return "OUT";

    case ANALOGUE_INVALID:
    default:
        return "INVALID";
    };
}

/*----------------------------------------------------------------------------*/

ov_analogue_event_type ov_analogue_event_type_from_string(char const *type) {

    if (ov_string_equal_nocase(type, "IN")) {
        return ANALOGUE_IN;
    } else if (ov_string_equal_nocase(type, "OUT")) {
        return ANALOGUE_OUT;
    } else {
        return ANALOGUE_INVALID;
    }
}

/*----------------------------------------------------------------------------*/

ov_analogue_event ov_analogue_event_from_json(ov_json_value const *jval) {

    ov_json_value const *rtp = ov_json_get(jval, "/" OV_KEY_RTP);

    ov_analogue_event rpe = {.rtp.mc_socket.type =
                                 NETWORK_TRANSPORT_TYPE_ERROR};

    char const *loop = ov_json_string_get(ov_json_get(rtp, "/" OV_KEY_LOOP));
    ov_json_value const *codec = ov_json_get(rtp, "/" OV_KEY_CODEC);

    if (ov_ptr_valid(loop, OV_KEY_LOOP " not found in JSON")) {

        ov_socket_configuration scfg = {0};

        strncpy(rpe.rtp.mc_socket.host, loop, sizeof(scfg.host));
        rpe.rtp.mc_socket.port = OV_MC_SOCKET_PORT;
        rpe.rtp.mc_socket.type = UDP;

        rpe.rtp.ssid = ov_config_u64_or_default(rtp, OV_KEY_SSRC, 0);

        rpe.rtp.codec_config = codec;
        rpe.channel = ov_config_u32_or_default(jval, OV_KEY_CHANNEL, 0);

        //        {
        //            "rtp": {
        //              "loop": "abc",
        //              "codec": { "codec": "opus"},
        //              "ssid": 1
        //            },
        //            "analogue": {
        //              "channel": 0
        //            }
        //        }
    }

    return rpe;
}

/*----------------------------------------------------------------------------*/

ov_analogue_stop_event
ov_analogue_stop_event_from_json(ov_json_value const *jval) {

    ov_analogue_stop_event event = {
        .type = ov_analogue_event_type_from_string(
            ov_json_string_get(ov_json_get(jval, "/" OV_KEY_TYPE))),
        .channel = ov_config_u32_or_default(jval, OV_KEY_CHANNEL, 0),

    };

    return event;
}

/*----------------------------------------------------------------------------*/

ov_analogue_list_channel_event
ov_analogue_list_channel_event_from_json(ov_json_value const *jval) {

    ov_analogue_list_channel_event event = {
        .type = ov_analogue_event_type_from_string(
            ov_json_string_get(ov_json_get(jval, "/" OV_KEY_TYPE))),

    };

    return event;
}

/*----------------------------------------------------------------------------*/

ov_analogue_channel ov_analogue_channel_from_json(ov_json_value const *jval) {

    ov_analogue_channel event = {
        .type = ov_analogue_event_type_from_string(
            ov_json_string_get(ov_json_get(jval, "/" OV_KEY_TYPE))),
        .number = ov_config_u64_or_default(jval, OV_KEY_CHANNEL, 0),

    };

    return event;
}

/*----------------------------------------------------------------------------*/
