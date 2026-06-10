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
    SIP_CALL: "sip_call",
    SIP_HANGUP: "sip_hangup",
    SIP_PERMIT: "sip_permit_call",
    SIP_REVOKE: "sip_revoke_call",
    SIP_LIST_CALLS: "sip_get_calls",
    SIP_LIST_CALL_PERMISSIONS: "sip_get_call_permissions",
    SIP_LIST_STATUS: "sip_get_status"
};

// SIP --------------------------------------------------------------------
export async function sip(websocket) {
    if (websocket)
        return await ws_sip(websocket);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.authenticated) {
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
        if (ws.authenticated) {
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
        if (ws.authenticated) {
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
        if (ws.authenticated) {
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
        if (ws.authenticated) {
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
        if (ws.authenticated) {
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

async function ws_sip_call(loop_id, from, to, websocket) {
    try {
        console.log(log_prefix(websocket) + "requesting sip connection from loop " + loop_id + " to " + to + "...");
        let parameter = { loop: loop_id, destination: to, from: from };
        let result = await websocket.send_event(EVENT.SIP_CALL, parameter);
        console.log(log_prefix(websocket) + "waiting for sip to connect...");
        return result;
    } catch (error) {
        console.warn(log_prefix(websocket) + "requesting sip connection failed.", error.error);
        return false;
    }
}

async function ws_sip_hangup(loop_id, call_id, websocket) {
    try {
        console.log(log_prefix(websocket) + "requesting sip hangup for call " + call_id + "...");
        let parameter = { call: call_id, loop: loop_id };
        let result = await websocket.send_event(EVENT.SIP_HANGUP, parameter);
        console.log(log_prefix(websocket) + "call " + call_id + " ended");
        return result;
    } catch (error) {
        console.warn(log_prefix(websocket) + "requesting sip connection failed.", error);
        return false;
    }
}

async function ws_sip_list_calls(websocket) {
    try {
        console.log(log_prefix(websocket) + "requesting current list of sip calls...");
        let result = await websocket.send_event(EVENT.SIP_LIST_CALLS);
        console.log(log_prefix(websocket) + "received list of sip calls");
        return result.calls;
    } catch (error) {
        console.warn(log_prefix(websocket) + "requesting list of sip calls failed.", error);
        return false;
    }
}

async function ws_sip_permit(loop_id, caller, callee, websocket) {
    try {
        console.log(log_prefix(websocket) + "add sip permit...");
        let parameter = { loop: loop_id, caller: caller, callee: callee };
        let result = await websocket.send_event(EVENT.SIP_PERMIT, parameter);
        console.log(log_prefix(websocket) + "added sip permit");
        return result;
    } catch (error) {
        console.warn(log_prefix(websocket) + "adding sip permit failed.", error);
        return false;
    }
}

async function ws_sip_revoke(loop_id, caller, callee, websocket) {
    try {
        console.log(log_prefix(websocket) + "revoke sip permit...");
        let parameter = { loop: loop_id, caller: caller, callee: callee };
        let result = await websocket.send_event(EVENT.SIP_REVOKE, parameter);
        console.log(log_prefix(websocket) + "revoked sip permit");
        return result;
    } catch (error) {
        console.warn(log_prefix(websocket) + "revoking sip permit failed.", error);
        return false;
    }
}

//-----------------------------------------------------------------------------
// logging
//-----------------------------------------------------------------------------
function log_prefix(ws) {
    return "(" + ws.server_name + " sip) ";
}