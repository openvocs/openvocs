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
    @file           ov_monitor.js

    @ingroup        ov_lib

    @brief          implements openvocs monitor protocol and wraps it
                    around a list of ov_websocket

    ---------------------------------------------------------------------------
*/
import ov_Websocket from "./ov_websocket.js";
import * as ov_Websockets from "./ov_websocket_list.js";

const EVENT = {
    CLIENTS: "monitor_get_clients",
    MIXER_STATES: "monitor_mixer_state",
    CONNECTIONS_STATE: "monitor_connections_state",
    MIXER_OVERVIEW: "monitor_mixer_overview",
    SESSION_STATE: "monitor_session_state",

}

export async function broadcast_switch_server(server_id, ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    try {
        console.log(log_prefix(ws) + "broadcast server switch to server " + server_id + "...");
        let params = { type: "server_switch", server: server_id }
        await ws.send_event(ov_Websocket.EVENT.BROADCAST, params);
        console.log(log_prefix(ws) + "server switch broadcast successful");
        return true;
    } catch (error) {
        console.warn(log_prefix(ws) + "broadcast server switch failed.", error.error);
        return false;
    }
}

export async function broadcast_update(ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    try {
        console.log(log_prefix(ws) + "broadcast update page...");
        let params = { type: "page_update" }
        await ws.send_event(ov_Websocket.EVENT.BROADCAST, params);
        console.log(log_prefix(ws) + "page update broadcast successful");
        return true;
    } catch (error) {
        console.warn(log_prefix(ws) + "broadcast update page failed.", error.error);
        return false;
    }
}

//-----------------------------------------------------------------------------
// logging
//-----------------------------------------------------------------------------
function log_prefix(ws) {
    return "(" + ws.server_name + " monitor) ";
}