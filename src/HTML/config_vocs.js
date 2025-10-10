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
*/

// overwrites default_config.js files

// signaling connection -------------------------------------------------------
SIGNALING_SERVERS = [
    // User interface supports up to 1 back up server, more are not displayed.
    // If no server id is flagged as prime, than the first server is treated as prime.
    // If you want to use the host address only define a name
    // otherwise define a WEBSOCKET_URL
    {
        NAME: "local"
    // },
    // {
    //     WEBSOCKET_URL: "wss://openvocs.net",
    //     NAME: "openvocs.net",
    //     //RECORD: true
    //     //PRIME: true
    }
]

SHARED_MULTICAST_ADDRESSES = true;

// extensions -----------------------------------------------------------------
SIP = false;
RECORDER = false;

// general options ------------------------------------------------------------
ACTIVITY_CONTENT = "display_name"; // "username" || "display_name"

// voice interface ------------------------------------------------------------
SECURE_VOICE_PTT = false; //e.g. when using hardware ptt button with secure voice

PTT = true; // if ptt is false an mute/unmute button is displayed instead

MUTE_ON_MOUSE_MIDDLE_CLICK = true;
MUTE_KEY = true;
MUTE_KEY_DEF = " ";
MUTE_KEY_NAME = "space bar";
MUTE_FULLSCREEN_BUTTON = false;

MULTI_TALK = false; //allow to talk in several loops at the same time

SCREEN_KEYBOARD = false;

// admin interface ------------------------------------------------------------
DEFAULT_MULTICAST_ADDRESS = "";

// ALLOW_IMPORT_EXPORT = false;
