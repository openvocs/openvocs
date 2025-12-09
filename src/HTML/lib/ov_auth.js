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

var RETRIES_ON_TEMP_ERROR = 5;

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

export async function login(username, password, websocket) {
    if (websocket)
        return await ws_login(username, password, websocket);

    let result = await ws_login(username, password, ov_Websockets.current_lead_websocket);
    if (result)
        for (let ws of ov_Websockets.list) {
            if (ws !== ov_Websockets.current_lead_websocket) {
                ws_login(username, password, ws);
            }
        }
    // failed login attempt is handled in disconnect,
    // but only if connect attempt before was successful
    return result;
}

export async function relogin(websocket) {
    if (!websocket)
        websocket = ov_Websockets.current_lead_websocket

    let session = ov_Web_Storage.get_session(APP, websocket.websocket_url);
    if (!session) {
        console.error("Session timed out or is undefined e.g. because of AUTH Error. You need to manually login again.");
        // for (let ws of ov_Websockets.list)
        //     ov_Web_Storage.clear(APP, ws.websocket_url);
        // ov_Websockets.reload_page();
        return false;
    }

    let result = false;
    if (session.user && session.session)
        result = await ws_login(session.user, session.session, websocket);
    if (result && session.role) {
        if (!websocket.user.roles)
            result = await collect_roles(websocket);
        result = await ws_authorize_role(session.role, websocket);
    }
    return result;
}

export async function collect_roles(websocket) {
    if (websocket)
        return await ws_collect_roles(websocket);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.is_connecting && ws.authenticated) {
            let promise = ws_collect_roles(ws);
            if (ws === ov_Websockets.current_lead_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
}

export async function authorize_role(role_id, websocket) {
    if (websocket)
        return await ws_authorize_role(role_id, websocket);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.is_connecting && ws.authenticated) {
            let promise = ws_authorize_role(role_id, ws);
            if (ws === ov_Websockets.current_lead_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
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

async function ws_connect(websocket) {
    let result = websocket.is_ready;
    if (!websocket.is_connecting) {
        try {
            console.log(log_prefix(websocket) + "connect to server...");
            result = await websocket.connect();
            if (BROADCAST_REGISTRATION)
                ws_register(websocket);
        } catch (error) {
            console.warn(log_prefix(websocket) + "failed to connect to server", error);
            return false;
        }
    }
    return result;
}

async function ws_register(websocket) {
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "register for broadcast...");
            await websocket.send_event(ov_Websocket.EVENT.REGISTER);
            console.log(log_prefix(websocket) + "registered for broadcast");
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to register for broadcast again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "registering for broadcast failed.", error);
                return false;
            }
        }
    }
    return true;
}

async function ws_login(username, password, websocket) {
    if (!await ws_connect(websocket))
        return false;

    if (!websocket.authenticated) {
        for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) { // retry x times on temp error
            try {
                console.log(log_prefix(websocket) + "logging in...");
                await websocket.login(username, password);
                console.log(log_prefix(websocket) + "authenticated as " + websocket.user.id);
                break;
            } catch (error) {
                if (websocket.is_connecting && error.temp_error) {
                    console.log(log_prefix(websocket) + "temp error - try to login again after timeout");
                    await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
                } else {
                    break;
                }
            }
        }
    }

    if (!websocket.authenticated) {
        console.warn(log_prefix(websocket) + "failed to login.");
        return false;
    }

    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "collecting user information...");
            await websocket.send_event(ov_Websocket.EVENT.GET, { type: "user", id: username });
            console.log(log_prefix(websocket) + "received user information for user " + websocket.user.id);
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + " temp error - try to collect user information again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + " collect user information failed.", error);
                return false;
            }
        }
    }
    return true;
}

async function ws_collect_roles(websocket) {
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "collecting user roles...");
            let roles = await websocket.send_event(ov_Websocket.EVENT.USER_ROLES);
            console.log(log_prefix(websocket) + "received " + roles.length + " roles: " + roles.toString());
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + " temp error - try to collect user roles again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + " collect user roles failed.", error);
                return false;
            }
        }
    }
    return true;
}

async function ws_authorize_role(role_id, websocket) {
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "authorize role...");
            await websocket.send_event(ov_Websocket.EVENT.AUTHORIZE_ROLE, { role: role_id });
            console.log(log_prefix(websocket) + "role " + websocket.user.role + " authorized");
            //this.#authorization_successful();
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + " temp error - try to authorize role again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + " authorize role failed.", error);
                return false;
            }
        }
    }
    return true;
}

async function ws_logout(websocket) {
    try {
        await websocket.logout();
    } catch (error) {
        if (error.error !== "server disconnected")
            console.error(error);
    }
    if (!websocket.is_connecting || !websocket.authenticated)
        return true;
}

//-----------------------------------------------------------------------------
// logging
//-----------------------------------------------------------------------------
function log_prefix(ws) {
    return "(" + ws.server_name + " auth) ";
}