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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_RECORDER_EVENTS_H
#define OV_RECORDER_EVENTS_H
/*----------------------------------------------------------------------------*/

#include <ov_base/ov_id.h>
#include <ov_base/ov_json_value.h>

/*****************************************************************************
                                  record_start
 ****************************************************************************/

typedef struct {

    char *loop;
    char *mc_ip;
    uint16_t mc_port;
    uint16_t roll_after_secs;

} ov_recorder_event_start;

/*----------------------------------------------------------------------------*/

/**
 * Frees all resources kept within `event` .
 * BEWARE: Don't use this on events, whose `loop` or `mc_ip` has not been
 * allocated
 */
bool ov_recorder_event_start_clear(ov_recorder_event_start *event);

/*----------------------------------------------------------------------------*/

/**
 * @return if event1 is the same as event2 (or if both event1 and event2 are 5
 */
bool ov_recorder_event_start_equal(ov_recorder_event_start const *event1,
                                   ov_recorder_event_start const *event2);

/*----------------------------------------------------------------------------*/

/**
 * Create a 'recorder_start' signaling message like that:
 *
 * ov_recorder_event_start start_event = { ... };
 *
 * msg = ov_event_api_message_create(OV_EVENT_START_RECORD, 0, 0);
 * ov_json_value *params = ov_event_api_set_parameter(msg);
 * ov_recorder_event_start_to_json(params, &start_event);
 */
bool ov_recorder_event_start_to_json(ov_json_value *target,
                                     ov_recorder_event_start const *event);

/*----------------------------------------------------------------------------*/

/**
 * Use like:
 *
 * ov_json_value *msg = ...; // Usually read from network...
 *
 * ov_recorder_event_start start_event = {0};
 *
 * if(ov_recorder_event_start_from_json(ov_event_api_get_parameter(msg),
 *                                   &start_event)) {
 *
 *       printf("Congrats! You got recorder start event for loop %s "
 *              "to record mc ip %s\n", start_event.loop, start_event.mc_ip);
 *
 * }
 *
 *
 */
bool ov_recorder_event_start_from_json(ov_json_value const *json,
                                       ov_recorder_event_start *event);

/*****************************************************************************
                             record_start_response
 ****************************************************************************/

typedef struct {

    ov_id id;
    char *filename;

} ov_recorder_response_start;

/*----------------------------------------------------------------------------*/

/**
 * Frees all resources kept within `response` .
 * BEWARE: Don't use this on responses, whose `filename` is set but was not
 * allocated!
 */
bool ov_recorder_response_start_clear(ov_recorder_response_start *response);

/*----------------------------------------------------------------------------*/

/**
 * @return if resp1 is the same as resp2 (or if both resp1 and resp2 are 0)
 */
bool ov_recorder_response_start_equal(ov_recorder_response_start const *resp1,
                                      ov_recorder_response_start const *resp2);

/*----------------------------------------------------------------------------*/

/**
 * Create a 'recorder_start' signaling response like that:
 *
 * ov_recorder_response_start start_response = { ... };
 *
 * response = ov_json_value *ov_event_api_create_success_response(msg);
 * ov_json_value *params = ov_event_api_get_response(response);
 * ov_recorder_response_start_to_json(params, &start_response);
 */
bool ov_recorder_response_start_to_json(
    ov_json_value *target, ov_recorder_response_start const *event);

/*----------------------------------------------------------------------------*/

/**
 * Use like:
 *
 * ov_json_value *msg = ...; // Usually read from network...
 *
 * ov_recorder_response_start resp = {0};
 *
 * if(ov_recorder_response_start_from_json(ov_event_api_get_response(msg),
 *                                   &resp));
 *
 */
bool ov_recorder_response_start_from_json(ov_json_value const *json,
                                          ov_recorder_response_start *response);

/*****************************************************************************
                                  record_stop
 ****************************************************************************/

typedef struct {

    ov_id id;

} ov_recorder_event_stop;

/*----------------------------------------------------------------------------*/

/**
 * Frees all resources kept within `event` .
 */
bool ov_recorder_event_stop_clear(ov_recorder_event_stop *event);

/*----------------------------------------------------------------------------*/

/**
 * @return if event1 is the same as event2 (or if both event1 and event2 are 5
 */
bool ov_recorder_event_stop_equal(ov_recorder_event_stop const *event1,
                                  ov_recorder_event_stop const *event2);

/*----------------------------------------------------------------------------*/

/**
 * Create a 'recorder_stop' signaling message like that:
 *
 * ov_recorder_event_stop stop_event = { ... };
 *
 * msg = ov_event_api_message_create(OV_EVENT_STOP_RECORD, 0, 0);
 * ov_json_value *params = ov_event_api_set_parameter(msg);
 * ov_recorder_event_stop_to_json(params, &stop_event);
 */
bool ov_recorder_event_stop_to_json(ov_json_value *target,
                                    ov_recorder_event_stop const *event);

/*----------------------------------------------------------------------------*/

/**
 * Use like:
 *
 * ov_json_value *msg = ...; // Usually read from network...
 *
 * ov_recorder_event_stop stop_event = {0};
 *
 * if(ov_recorder_event_stop_from_json(ov_event_api_get_parameter(msg),
 *                                   &stop_event)) {
 *
 *       printf("Congrats! You got recorder stop event for loop %s "
 *              "to record mc ip %s\n", stop_event.loop, stop_event.mc_ip);
 *
 * }
 *
 *
 */
bool ov_recorder_event_stop_from_json(ov_json_value const *json,
                                      ov_recorder_event_stop *event);

/*----------------------------------------------------------------------------*/
#endif
