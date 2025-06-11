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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_ANALOGUE_EVENTS_H
#define OV_ANALOGUE_EVENTS_H
/*----------------------------------------------------------------------------*/

#include <ov_base/ov_socket.h>

typedef enum {

    ANALOGUE_INVALID = -1,
    ANALOGUE_IN,
    ANALOGUE_OUT,

} ov_analogue_event_type;

char const *ov_analogue_event_type_to_string(ov_analogue_event_type type);

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_analogue_event_type type;

    struct {
        ov_socket_configuration mc_socket;
        ov_json_value const *codec_config;
        uint32_t ssid;
    } rtp;

    uint32_t channel;

} ov_analogue_event;

/**
 * On error, rtp.mc_socket.type is set to invalid
 */
ov_analogue_event ov_analogue_event_from_json(ov_json_value const *jval);

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_analogue_event_type type;
    uint32_t channel;

} ov_analogue_stop_event;

/**
 * On error, "type" is set to "INVALID"
 */
ov_analogue_stop_event ov_analogue_stop_event_from_json(
    ov_json_value const *jval);

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_analogue_event_type type;

} ov_analogue_list_channel_event;

ov_analogue_list_channel_event ov_analogue_list_channel_event_from_json(
    ov_json_value const *jval);

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_analogue_event_type type;
    size_t number;

} ov_analogue_channel;

ov_analogue_channel ov_analogue_channel_from_json(ov_json_value const *jval);

/*----------------------------------------------------------------------------*/
#endif
