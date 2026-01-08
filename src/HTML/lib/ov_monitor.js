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

import * as ov_Websockets from "./ov_websocket_list.js";

var RETRIES_ON_TEMP_ERROR = 5;

const EVENT = {
    AUTHORIZE: "authorize",
    BROADCAST: "broadcast",
    LOOPS: "get_all_loops"
}

export async function broadcast_switch_server(server_id, ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let params = { type: "server_switch", server: server_id }
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "broadcast server switch to server " + server_id + "...");
            await ws.send_event(EVENT.BROADCAST, params);
            console.log(log_prefix(ws) + "server switch broadcast successful");
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to broadcast server switch again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "broadcast server switch failed.", error);
                return false;
            }
        }
    }
    return true;
}

export async function broadcast_update(ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let params = { type: "page_update" }
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "broadcast update page...");
            await ws.send_event(EVENT.BROADCAST, params);
            console.log(log_prefix(ws) + "page update broadcast successful");
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to broadcast update page again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "broadcast update page failed.", error);
                return false;
            }
        }
    }
    return true;
}

export async function get_loops(ws) {
    let result;
    ws = ws ? ws : ov_Websockets.prime_websocket;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "get all loops...");
            result = await ws.send_event(EVENT.LOOPS);
            console.log(log_prefix(ws) + "get all loops successful");
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to get all loops again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "get all loops failed.", error);
                return false;
            }
        }
    }
    return result.result;
}

//-----------------------------------------------------------------------------
// logging
//-----------------------------------------------------------------------------
function log_prefix(ws) {
    return "(" + ws.server_name + " monitor) ";
}