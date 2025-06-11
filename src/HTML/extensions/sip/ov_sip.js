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
    @file           ov_sip.js

    @ingroup        extensions/sip

    @brief          implements openvocs sip protocol and wraps it
                    around a list of ov_websockets

    ---------------------------------------------------------------------------
*/

import * as ov_Websockets from "/lib/ov_websocket_list.js";

export const EVENT = {
    SIP: "sip",
    SIP_CALL: "call",
    SIP_HANGUP: "hangup",
    SIP_PERMIT: "permit_call",
    SIP_REVOKE: "revoke_call",
    SIP_LIST_CALLS: "list_calls",
    SIP_LIST_CALL_PERMISSIONS: "list_call_permissions",
    SIP_LIST_STATUS: "list_sip_status"
};

var RETRIES_ON_TEMP_ERROR = 5;

// SIP --------------------------------------------------------------------
export async function sip(websocket) {
    if (websocket)
        return await ws_sip(websocket);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.is_ready && ws.authenticated) {
            let promise = ws_sip(ws);
            if (ws === ov_Websockets.prime_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
}
export async function sip_call(loop_id, from, to, websocket) {
    if (websocket)
        return await ws_sip_call(loop_id, from, to, websocket);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.is_ready && ws.authenticated) {
            let promise = ws_sip_call(loop_id, from, to, ws);
            if (ws === ov_Websockets.prime_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
}

export async function sip_hangup(loop_id, call_id, websocket) {
    if (websocket)
        return await ws_sip_hangup(loop_id, call_id, websocket);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.is_ready && ws.authenticated) {
            let promise = ws_sip_call(call_id, ws);
            if (ws === ov_Websockets.prime_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
}

export async function sip_list_calls(websocket) {
    if (websocket)
        return await ws_sip_list_calls(websocket);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.is_ready && ws.authenticated) {
            let promise = ws_sip_list_calls(ws);
            if (ws === ov_Websockets.prime_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
}

export async function sip_permit(loop_id, caller, callee, websocket) {
    if (websocket)
        return await ws_sip_permit(loop_id, caller, callee, websocket);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.is_ready && ws.authenticated) {
            let promise = ws_sip_permit(loop_id, caller, callee, websocket);
            if (ws === ov_Websockets.prime_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
}

export async function sip_revoke(loop_id, caller, callee, websocket) {
    if (websocket)
        return await ws_sip_revoke(loop_id, caller, callee, websocket);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.is_ready && ws.authenticated) {
            let promise = ws_sip_revoke(loop_id, caller, callee, websocket);
            if (ws === ov_Websockets.prime_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
}

/*list_permissions() {
    return this.#request(this.#create_event(EVENT.SIP_LIST_CALL_PERMISSIONS));
}

list_status() {
    return this.#request(this.#create_event(EVENT.SIP_LIST_STATUS));
}*/

async function ws_sip(websocket) {
    let result;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "requesting sip status...");
            result = await websocket.send_event(EVENT.SIP);
            console.log(log_prefix(websocket) + "sip server connected: " + result.connected);
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to request sip status again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "requesting sip status failed.", error.error);
                disconnect(websocket);
                return false;
            }
        }
    }
    return result;
}

async function ws_sip_call(loop_id, from, to, websocket) {
    let result;
    let parameter = { loop: loop_id, destination: to, from: from };
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "requesting sip connection from loop " + loop_id + " to " + to + "...");
            result = await websocket.send_event(EVENT.SIP_CALL, parameter);
            console.log(log_prefix(websocket) + "waiting for sip to connect...");
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to request sip connection again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "requesting sip connection failed.", error.error);
                disconnect(websocket);
                return false;
            }
        }
    }
    return result;
}

async function ws_sip_hangup(loop_id, call_id, websocket) {
    let result;
    let parameter = { call: call_id, loop: loop_id };
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "requesting sip hangup for call " + call_id + "...");
            result = await websocket.send_event(EVENT.SIP_HANGUP, parameter);
            console.log(log_prefix(websocket) + "call " + call_id + " ended");
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to request sip connection again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "requesting sip connection failed.", error);
                disconnect(websocket);
                return false;
            }
        }
    }
    return result;
}

async function ws_sip_list_calls(websocket) {
    let result;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "requesting current list of sip calls...");
            result = await websocket.send_event(EVENT.SIP_LIST_CALLS);
            console.log(log_prefix(websocket) + "received list of sip calls");
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to request list of sip calls again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "requesting list of sip calls failed.", error);
                disconnect(websocket);
                return false;
            }
        }
    }
    return result.calls;
}

async function ws_sip_permit(loop_id, caller, callee, websocket) {
    let result;
    let parameter = { loop: loop_id, caller: caller, callee: callee };
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "add sip permit...");
            result = await websocket.send_event(EVENT.SIP_PERMIT, parameter);
            console.log(log_prefix(websocket) + "added sip permit");
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to add sip permit again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "adding sip permit failed.", error);
                disconnect(websocket);
                return false;
            }
        }
    }
    return result;
}

async function ws_sip_revoke(loop_id, caller, callee, websocket) {
    let result;
    let parameter = { loop: loop_id, caller: caller, callee: callee };
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "revoke sip permit...");
            result = await websocket.send_event(EVENT.SIP_REVOKE, parameter);
            console.log(log_prefix(websocket) + "revoked sip permit");
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to revoke sip permit again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "revoking sip permit failed.", error);
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
    return "(" + ws.server_name + " sip) ";
}