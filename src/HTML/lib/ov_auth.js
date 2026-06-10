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
    @file           ov_auth.js

    @ingroup        ov_lib

    @brief          implements openvocs auth protocol and wraps it
                    around a list of ov_websocket

    ---------------------------------------------------------------------------
*/

import ov_Websocket from "./ov_websocket.js";
import * as ov_Websockets from "./ov_websocket_list.js";
import * as ov_Web_Storage from "./ov_utils/ov_web_storage.js";
import * as ov_DB from "/lib/ov_db.js";

export async function connect(websocket) {
    if (websocket)
        return await ws_connect(websocket);

    let lead_promise;
    let lead_is_resolved = false;
    let promises = [];

    for (let ws of ov_Websockets.list) {
        let promise = ws_connect(ws);
        promises.push(promise);
        if (ws === ov_Websockets.prime_websocket) {
            lead_promise = promise;
            promise.then(async (value) => {
                lead_is_resolved = value;
            });
        } else {
            promise.then(async (value) => {
                if (value && !lead_is_resolved) {
                    lead_is_resolved = true;
                    lead_promise = promise;
                    console.log("switch server to", ws.server_name);
                    ov_Websockets.switch_lead_websocket(ws);
                }
            });
        }
    }

    await Promise.any(promises);

    if (!lead_is_resolved)
        await Promise.allSettled(promises);

    return await lead_promise;
}

async function ws_connect(websocket) {
    let result = websocket.connected;
    if (!result && !websocket.connecting) {
        try {
            console.log(log_prefix(websocket) + "connect to server...");
            result = await websocket.connect();
        } catch (error) {
            console.warn(log_prefix(websocket) + "failed to connect to server");
            return false;
        }
    }
    return result;
}

export async function login(username, password, websocket) {
    if (websocket)
        return await ws_login(username, password, websocket);

    let result = await ws_login(username, password, ov_Websockets.current_lead_websocket);
    if (result.authenticated)
        for (let ws of ov_Websockets.list) {
            if (ws !== ov_Websockets.current_lead_websocket) {
                ws_login(username, password, ws);
            }
        }
    // failed login attempt is handled in disconnect,
    // but only if connect attempt before was successful
    return result;
}

async function ws_login(username, password, websocket) {
    if (!await ws_connect(websocket))
        return { authenticated: false, error: { description: "Not connected to server." } };

    if (!websocket.authenticated) {
        try {
            console.log(log_prefix(websocket) + "logging in...");
            await websocket.login(username, password);
            console.log(log_prefix(websocket) + "authenticated as " + websocket.user.id);
        } catch (error) {
            console.warn(log_prefix(websocket) + "failed to login.", error.error);
            return { authenticated: false, error: error.error };
        }
    }

    try {
        console.log(log_prefix(websocket) + "collecting user information...");
        await websocket.send_event(ov_Websocket.EVENT.GET, { type: "user", id: username });
        console.log(log_prefix(websocket) + "received user information for user " + websocket.user.id);
        return { authenticated: true };
    } catch (error) {
        console.warn(log_prefix(websocket) + " collect user information failed.", error.error);
        return { authenticated: false, error: error.error };
    }
}

export async function authorize_role(role_id, websocket) {
    websocket = websocket ? websocket : ov_Websockets.current_lead_websocket;
    try {
        console.log(log_prefix(websocket) + "authorize role...");
        if (!role_id)
            role_id = websocket.session.role;
        await websocket.send_event(ov_Websocket.EVENT.AUTHORIZE_ROLE, { role: role_id });
        console.log(log_prefix(websocket) + "role " + websocket.user.role + " authorized");
        return { authorized: true };
    } catch (error) {
        console.warn(log_prefix(websocket) + " authorize role failed.", error.error);
        return { authorized: false, error: error.error };
    }
}

export async function logout(websocket) {
    if (websocket)
        return await ws_logout(websocket);

    let promises = []
    for (let ws of ov_Websockets.list)
        promises.push(ws_logout(ws));
    await Promise.allSettled(promises);
    return true;
}

async function ws_logout(websocket) {
    try {
        await websocket.logout();
    } catch (error) {
        if (error.error !== "server disconnected")
            console.error(error.error);
    }
    if (!websocket.authenticated)
        return true;
}

export function clear_session(ws) {
    if (ws) {
        ov_Web_Storage.clear(APP, ws.websocket_url);
        return;
    }
    for (let ws of ov_Websockets.list)
        ov_Web_Storage.clear(APP, ws.websocket_url);
}

export function has_valid_session(ws) {
    return ov_Web_Storage.get_session(APP, ws.websocket_url) !== null;
}

//-----------------------------------------------------------------------------
// logging
//-----------------------------------------------------------------------------
function log_prefix(ws) {
    return "(" + ws.server_name + " auth) ";
}