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
    @file           ov_websocket_list.js

    @ingroup        ov_lib

    @brief          an array of ov_websockets

    ---------------------------------------------------------------------------
*/

import * as ov_Web_Storage from "/lib/ov_utils/ov_web_storage.js";

export var list;
export var prime_websocket;
export var current_lead_websocket;
export var auto_login = false;

var disconnect_callback;
export var disconnected_websockets = new Map();

var websocket_events = [];

export function setup_connections(ov_Websocket) {
    // parse server url(s) and check for autologin
    let servers = [];
    let temp_auto_login = false;
    for (let websocket of WEBSOCKET) {
        for (let server of SIGNALING_SERVERS) {
            let websocket_address = server.WEBSOCKET_URL ? server.WEBSOCKET_URL + websocket :
                "wss://" + window.location.hostname + websocket;

            let active_session = ov_Web_Storage.get_session(websocket_address);
            let client_id = active_session ? active_session.client : undefined;
            if (client_id !== undefined)
                temp_auto_login = true;
            if (server.PRIME && WEBSOCKET.indexOf(websocket) === 0)
                servers.unshift({ "name": server.NAME, "address": websocket_address, "client_id": client_id });
            else
                servers.push({ "name": server.NAME, "address": websocket_address, "client_id": client_id });
        }
    }
    auto_login = temp_auto_login;

    // setup server(s)
    list = [];
    for (let server of servers) {
        console.log("add server '" + server.name + "' -> " + server.address);
        let client_id = auto_login ? server.client_id : undefined;

        let websocket = new ov_Websocket(server.name, server.address, client_id);
        websocket.log_incoming_events = DEBUG_LOG_INCOMING_EVENTS;
        websocket.log_outgoing_events = DEBUG_LOG_OUTGOING_EVENTS;
        websocket.resend_events_after_timeout = false;

        list.push(websocket);

        if (prime_websocket === undefined) { // first server in list is PRIME
            prime_websocket = websocket;
            current_lead_websocket = websocket;
        }

        websocket.addEventListener("disconnected", () => {
            disconnected_websockets.set(websocket.client_id, websocket);
            if (disconnect_callback)
                disconnect_callback(websocket);
        });

        websocket.addEventListener("connected", () => {
            disconnected_websockets.delete(websocket.client_id);
        });
    }
}

export function get_websocket(url) {
    return list.find(ws => { return ws.server_url === url });
}

export function on_disconnect(callback) { //only one callback function can be registered
    if (typeof callback === "function")
        disconnect_callback = callback;
}

export function server_url() {
    if (current_lead_websocket.server_url)
        return current_lead_websocket.server_url;
    else
        return prime_websocket.server_url;
}

export function server_name() {
    if (current_lead_websocket.server_name)
        return current_lead_websocket.server_name;
    else
        return prime_websocket.server_name;
}

export function user() {
    if (current_lead_websocket.user)
        return current_lead_websocket.user;
    else
        return prime_websocket.user;
}

export function addEventListener(event, listener) {
    websocket_events.push({ event: event, listener: listener });
    current_lead_websocket.addEventListener(event, listener);
}

export function removeEventListeners() {
    for (let event of websocket_events)
        current_lead_websocket.removeEventListener(event.event, event.listener);

    websocket_events = [];
}

export function switch_lead_websocket(websocket) {
    if (list.includes(websocket)) {
        for (let event of websocket_events) {
            current_lead_websocket.removeEventListener(event.event, event.listener);
        }
        current_lead_websocket = websocket;
        for (let event of websocket_events) {
            current_lead_websocket.addEventListener(event.event, event.listener);
        }
    }
}

export async function reload_page() {
    // if (await is_location_online(window.location.origin))
    window.location.reload();
    // else if (await is_location_online(prime_websocket.server_url))
    //     window.location.replace(prime_websocket.server_url + "/app/vocs/");
    // else
    //     for (let ws of list)
    //         if (await is_location_online(ws.server_url)) {
    //             window.location.replace(ws.server_url + "/app/vocs/");
    //             break;
    //         }
}

// async function is_location_online(url) {
//     try {
//         await new Promise((resolve, reject) => {
//             var img = document.body.appendChild(document.createElement("img"));
//             img.onload = function () {
//                 document.body.removeChild(img);
//                 resolve();
//             };
//             img.onerror = function () {
//                 console.log("error")
//                 document.body.removeChild(img);
//                 reject();
//             };
//             img.src = url + "/images/nopic.png";
//         });
//         return true;
//     } catch (error) {
//         return false;
//     }
// }

export async function find_new_authorized_server() {
    return await find_new_server(true);
}

export async function find_new_connected_server() {
    return await find_new_server(false);
}

var finding_new_server = false;
async function find_new_server(already_authorized) {
    if (!finding_new_server) {
        console.log("look for server");
        finding_new_server = true;
        while (finding_new_server) {
            for (let ws of list) {
                if (already_authorized) {
                    if (ws.authorized) {
                        finding_new_server = false;
                        return ws;
                    }
                } else {
                    if (ws.is_ready) {
                        finding_new_server = false;
                        return ws;
                    }
                }
            }

            if (finding_new_server) {
                console.log("no server reachable, try again after timeout")
                await sleep(PERS_ERROR_TIMEOUT);
            }
        }
    }
    return undefined;
}

export async function sleep(time, ws) {
    await new Promise((resolve) => {
        if (ws)
            console.warn("(" + ws.server_name + ") Sleep for " + time + "ms.");
        else
            console.warn("Sleep for " + time + "ms.");
        setTimeout(() => { resolve(); }, time);
    });
}