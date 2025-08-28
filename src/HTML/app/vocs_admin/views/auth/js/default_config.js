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

    @ingroup        vocs_admin/views/auth

    @brief          global vars for auth, other config files might
                    overwrite this variables

    ---------------------------------------------------------------------------
*/
WEBSOCKET = ["/db", "/admin"];

SIGNALING_SERVERS = [
    // User interface supports up to 1 back up server, more are not displayed.
    // If no server id is flagged as prime, than the first server is treated as prime.
    // If you want to use the host address only define a name
    // otherwise define a WEBSOCKET_URL
    {
        NAME: "local"
    }
]


//time to wait before resending a signaling request
SIGNALING_REQUEST_TIMEOUT = 10000;
TEMP_ERROR_TIMEOUT = 4000;

DEBUG_LOG_INCOMING_EVENTS = true;
DEBUG_LOG_OUTGOING_EVENTS = true;

SIP = false;
RECORDER = false;

DEFAULT_MULTICAST_ADDRESS = false;

BROADCAST_REGISTRATION = false;

ALLOW_IMPORT_EXPORT = true;