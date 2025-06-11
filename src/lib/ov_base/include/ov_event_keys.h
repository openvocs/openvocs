/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_signaling_event_keys.h
        @author         Markus Toepfer

        @date           2019-01-23

        @ingroup        ov_event_keys_h

        @brief          Definition of keys for signaling events.

        TBD to be deleted in favor of ov_config_keys???

        ------------------------------------------------------------------------
*/
#ifndef ov_event_keys_h
#define ov_event_keys_h

#define OV_EVENT_MAX_KEY_LENGTH 80

#define OV_EVENT_KEY_PING "ping"
#define OV_EVENT_KEY_PONG "pong"

#define OV_EVENT_KEY_EVENT "event"

#define OV_EVENT_PARAMETER "parameter"

#define OV_EVENT_HELP "help"

/******************************************************************************
 *                               minion events
 ******************************************************************************/

#define OV_EVENT_REGISTER "register_service"
#define OV_EVENT_RECONFIGURE "reconfigure"

#define OV_EVENT_ADD_DESTINATION "add_destination"
#define OV_EVENT_REMOVE_DESTINATION "remove_destination"
#define OV_EVENT_LIST_DESTINATIONS "list_destinations"

#define OV_EVENT_ADD_SOURCE "add_source"
#define OV_EVENT_REMOVE_SOURCE "remove_source"
#define OV_EVENT_LIST_SOURCES "list_sources"

#define OV_EVENT_ADD_TRANSLATION "add_stream_translation"
#define OV_EVENT_LIST_TRANSLATIONS "list_stream_translations"
#define OV_EVENT_REMOVE_TRANSLATION "remove_stream_translation"
#define OV_KEY_TRANSLATION "translation"

#define OV_EVENT_GET_CONFIGURATION "get_configuration"
#define OV_EVENT_SHUTDOWN "shutdown"

#define OV_EVENT_SET_SOURCE_VOLUME "set_source_volume"

#define OV_EVENT_RECONFIGURE_ROLE "reconfigure_role"

#define OV_EVENT_SET_VAD_PARAMETERS "set_vad_parameters"

#define OV_EVENT_CACHE_REPORT "cache_report"

/******************************************************************************
 *                                 statistics
 ******************************************************************************/

#define OV_EVENT_GET_MINION_STATISTICS "get_minions_statistics"
#define OV_EVENT_LIST_MINIONS "list_minions"

#define OV_EVENT_LIST_REQUESTS "list_requests"

#define OV_KEY_UNDERFLOWS "underflows"
#define OV_KEY_NO_DATA "no_data"
#define OV_KEY_FRAMES_PER_WRITE "frames_per_write"
#define OV_KEY_PLAYBACK "playback"
#define OV_KEY_PLAYBACKS "playbacks"
#define OV_KEY_STREAM_STATE "state"
#define OV_KEY_RECORD "record"
#define OV_KEY_RECORDING "recording"
#define OV_KEY_RECORDINGS "recordings"
#define OV_KEY_GET_RECORDING "get_recording"

#define OV_EVENT_LIST_RUNNING_RECORDINGS "list_running_recordings"
#define OV_EVENT_LIST_AVAILABLE_RECORDINGS "list_available_recordings"
#define OV_EVENT_LIST_RUNNING_PLAYBACKS "list_running_playbacks"

/******************************************************************************
 *                                loop related
 ******************************************************************************/

#define OV_EVENT_ACQUIRE_LOOP "acquire_loop"
#define OV_EVENT_RELEASE_LOOP "release_loop"
#define OV_EVENT_LIST_LOOPS "list_loops"
#define OV_EVENT_GET_LOOP_RESOURCES "get_loop_resources"
#define OV_EVENT_GET_LOOP_STATE "get_loop_state"

/******************************************************************************
 *                                user related
 ******************************************************************************/

#define OV_EVENT_ACQUIRE_USER "acquire_user"
#define OV_EVENT_RELEASE_USER "release_user"
#define OV_EVENT_LIST_USERS "list_users"
#define OV_EVENT_GET_USER_RESOURCES "get_user_resources"
#define OV_EVENT_GET_USER_STATE "get_user_state"

#define OV_EVENT_SET_USER_EXT_MEDIA "set_user_ext_media"

#define OV_EVENT_LISTEN_ON "listen_on"
#define OV_EVENT_LISTEN_OFF "listen_off"

#define OV_EVENT_TALK_ON "talk_on"
#define OV_EVENT_TALK_OFF "talk_off"

#define OV_EVENT_USER_SET_LOOP_VOL "user_set_loop_volume"

#define OV_FRAME_LENGTH_USECS "frame_length_usecs"

/*****************************************************************************
                                   Recording
 ****************************************************************************/

#define OV_EVENT_START_PLAYBACK "playback_start"
#define OV_EVENT_STOP_PLAYBACK "playback_stop"
#define OV_EVENT_START_RECORD "record_start"
#define OV_EVENT_STOP_RECORD "record_stop"

/******************************************************************************
 *                               Notifications
 ******************************************************************************/

#define OV_EVENT_NOTIFY "notify"
#define OV_EVENT_ENTITY_LOST "entity_lost"
#define OV_EVENT_RECORDING_STOPPED "recording_stopped"
#define OV_EVENT_PLAYBACK_STOPPED "playback_stopped"

/******************************************************************************
 *                                   Other
 ******************************************************************************/

#define OV_EVENT_RESET "reset"

#endif /* ov_event_keys_h */
