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

// signaling connection -------------------------------------------------------
SIGNALING_SERVERS = [
    // User interface supports up to 3 back up server, more are not displayed.
    // If no server id is flagged as prime, than the first server is treated as prime.
    // For one server connection use either HOST_WEBSOCKET OR WEBSOCKET_URL,
    // if HOST_WEBSOCKET is set, WEBSOCKET_URL is ignored.
    // HOST_WEBSOCKET extends the host address
    {
        HOST_WEBSOCKET: ":8001/vocs",
        NAME: "local"
    }
]

PERS_ERROR_TIMEOUT = 5000;
TEMP_ERROR_TIMEOUT = 5000;
BROADCAST_REGISTRATION = false;
RECORDER = false;