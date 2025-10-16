/***
    ---------------------------------------------------------------------------

    Copyright (c) 2018 German Aerospace Center DLR e.V. (GSOC)

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

    ---------------------------------------------------------------------------
*//**
    @file           default_config.js

    @ingroup        vocs

    @brief          vars for openvocs voice client
                    This file overwrites variable from 
                    /views/auth/js/default_config.js

    ---------------------------------------------------------------------------
*/

// audio connection -----------------------------------------------------------
ICE_SERVERS = [
    {
      urls: "turn:openvocs.net:33533",
      username: "openvocs",
      credential: "2simple!"
    }
];

SHARED_MULTICAST_ADDRESSES = true;

PERS_ERROR_TIMEOUT = 5000;

// extensions
SIP = false;

// ui options -----------------------------------------------------------------
COLOR_MODE = "dark"; // "dark" || "light"
ACTIVITY_CONTENT = "display_name";

LOOP_GRID_MIN_ROWS = 1;
LOOP_GRID_MIN_COLUMNS = 1;

// mute/ptt trigger -----------------------------------------------------------
SECURE_VOICE_PTT = false; //e.g. when using hardware ptt button with secure voice

PTT = false; // if ptt is false an mute/unmute button is displayed instead
HIDE_PTT_MENU = false; //hide ptt buttons on website

MUTE_ON_MOUSE_MIDDLE_CLICK = true;
MUTE_KEY = true;
MUTE_KEY_DEF = " ";
MUTE_KEY_NAME = "space bar";
MUTE_FULLSCREEN_BUTTON = false;

MULTI_TALK = false; //allow to talk in several loops at the same time

// debug vars -----------------------------------------------------------------
DEBUG_USE_MEDIA_STREAM_FROM_FILE = false;
DEBUG_MEDIA_STREAM_FILE = "./resources/sounds/Apollo13-wehaveaproblem.ogg";

// developer vars -------------------------------------------------------------
RETRIES_ON_TEMP_ERROR = 5;
DEFAULT_LOOP_VOLUME = "50";