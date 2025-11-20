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
    @file           ov_recorder.js

    @ingroup        extensions/sip

    @brief          implements openvocs recorder protocol and wraps it
                    around a list of ov_websockets

    ---------------------------------------------------------------------------
*/

import * as ov_Websockets from "/lib/ov_websocket_list.js";

export const EVENT = {
    START_PLAYBACK: "playback_start",
    STOP_PLAYBACK: "playback_stop",
    START_RECORD: "record_start",
    STOP_RECORD: "record_stop",
    RECORDED_LOOPS: "get_recorded_loops",
    RECORDINGS: "get_recording"
};

var RETRIES_ON_TEMP_ERROR = 5;

// admin --------------------------------------------------------------------
export async function start_record(loop_id, websocket) {
    if (websocket)
        return await ws_start_record(loop_id, websocket);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.is_ready && ws.authenticated) {
            let promise = ws_start_record(loop_id, ws);
            if (ws === ov_Websockets.prime_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
}
export async function stop_record(loop_id, websocket) {
    if (websocket)
        return await ws_stop_record(loop_id, websocket);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.is_ready && ws.authenticated) {
            let promise = ws_stop_record(loop_id, ws);
            if (ws === ov_Websockets.prime_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
}

export async function get_recorded_loops(ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let result;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "get recorded loops...");
            result = await ws.send_event(EVENT.RECORDED_LOOPS);
            console.log(log_prefix(ws) + "get recorded loops successful");
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to get recorded loops again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "get recorded loops failed.", error);
                return false;
            }
        }
    }
    return result.result;
}

export async function get_recordings(loop_id, from, to, ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let parameter = {
        "loop": loop_id,
        "from": from,
        "to": to
    }
    let result;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "get recordings...");
            result = await ws.send_event(EVENT.RECORDINGS, parameter);
            console.log(log_prefix(ws) + "get recordings successful");
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to get recordings again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "get recordings failed.", error);
                return false;
            }
        }
    }
    return result;
}

async function ws_start_record(loop_id, websocket) {
    let result;
    let parameter = { loop: loop_id };
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "requesting to start recorder for loop " + loop_id + "...");
            result = await websocket.send_event(EVENT.START_RECORD, parameter);
            console.log(log_prefix(websocket) + "recorder for loop " + loop_id + " started");
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to start recorder for " + loop_id + " again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "requesting to start recorder for loop " + loop_id + " failed.", error.error);
                disconnect(websocket);
                return false;
            }
        }
    }
    return result;
}

async function ws_stop_record(loop_id, websocket) {
    let result;
    let parameter = { loop: loop_id };
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "requesting to stop recorder for loop " + loop_id + "...");
            result = await websocket.send_event(EVENT.STOP_RECORD, parameter);
            console.log(log_prefix(websocket) + "recorder for loop " + loop_id + " stopped");
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to stop recorder for " + loop_id + " again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "requesting to stop recorder for loop " + loop_id + " failed.", error.error);
                disconnect(websocket);
                return false;
            }
        }
    }
    return result;
}

function disconnect(websocket) {
    if (websocket.is_connecting) {
        console.warn(log_prefix(websocket) + "Pers error. Disconnect and try again.");
        websocket.disconnect();
    }
}

//-----------------------------------------------------------------------------
// logging
//-----------------------------------------------------------------------------
function log_prefix(ws) {
    return "(" + ws.server_name + " recorder) ";
}