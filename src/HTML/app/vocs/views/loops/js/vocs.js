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
    @file           vocs.js

    @ingroup        vocs

    @brief          init and load the voice client

    ---------------------------------------------------------------------------
*/

import * as View from "./ui/view.js";

import * as ov_Websockets from "/lib/ov_websocket_list.js";
import * as ov_WebRTCs from "/lib/ov_media/ov_webrtc_list.js";
import * as ov_Auth from "/lib/ov_auth.js";
import * as ov_Vocs from "/lib/ov_vocs.js";

var view_container;

export async function render(container) {
    view_container = container;
    view_container.appendChild(await loadCSS());
    view_container.appendChild(await loadHtml());

    console.log("(vc) voice client loaded");

    // event handling ---------------------------------------------------------
    window.addEventListener("resize", function () {
        console.log("(vc) resize voice client");
        setTimeout(async function () {
            let layout_name = window.innerHeight + "x" + window.innerWidth;
            let settings = await ov_Vocs.collect_keyset_layout(layout_name, ov_Websockets.current_lead_websocket);
            View.resize(settings);
        }, 150);
    });

    window.addEventListener("beforeunload", function () {
        // at this point the event loop (async content) is no longer available
        // console output is displayed on reloaded page
        if (ov_Websockets.current_lead_websocket.is_ready && ov_WebRTCs.get_lead().is_ready) {
            View.talk(false);
            console.log("(vc) unload voice client");
        }
    });

    ov_Websockets.addEventListener("broadcast", (event) => {
        if (event.detail.message.type === "page_update") {
            if ('caches' in window) {
                caches.keys().then(function (cache_name_list) {
                    cache_name_list.forEach(function (cache_name) {
                        caches.delete(cache_name);
                    });
                });
            }
            ov_Websockets.reload_page();
        } else if (event.detail.message.type === "server_switch") {
            View.switch_server(ov_Websockets.get_websocket(event.detail.message.server));
        }
    });


    // init app parts ---------------------------------------------------------
    try {
        await ov_WebRTCs.create_local_media_stream();
        for (let ws of ov_Websockets.list) {
            // configure ov_Websocket -----------------------------------------
            ws.log_incoming_events = DEBUG_LOG_INCOMING_EVENTS;
            ws.log_outgoing_events = DEBUG_LOG_OUTGOING_EVENTS;
            ws.resend_events_after_timeout = false;
        }
        ov_WebRTCs.setup_connections();

        await View.init();
        View.indicate_loading(true, "Establishing connection to server...");
    } catch (error) {
        await View.init();
        View.indicate_loading(true, "Please connect a microphone to your device and reload the page!");
    }

    ov_Websockets.addEventListener("webpage_update", (event) => {
        console.log("webpage update", event);
    });

    ov_Websockets.on_disconnect(async (websocket) => {
        console.error(log_prefix(websocket) + "disconnected from signaling server");

        let media = ov_WebRTCs.get(websocket);
        if (media && media.is_connected)
            media.disconnect();

        if (View.logout_triggered) {
            ov_Websockets.reload_page();
            return;
        }

        // if disconnect from current prio server -> switch server
        if (websocket === ov_Websockets.current_lead_websocket) {
            let message;
            if (!websocket.server_error)
                message = "Server closed connection. Trying to reconnect..."
            else {
                message = "Client disconnected! Trying to repair connection...\n"
                if (websocket.server_error.description)
                    message = message + " ERROR: " + websocket.server_error.description;
                if (websocket.server_error.code)
                    message = message + " CODE: " + websocket.server_error.code;
            }
            View.indicate_loading(true, message);

            ov_Websockets.find_new_authorized_server().then((ws) => {
                if (ws !== undefined && ws !== ov_Websockets.current_lead_websocket)
                    View.switch_server(ws);
            });
        }

        let all_off = true;
        for (let ws of ov_Websockets.list)
            if (ws.authorized || ov_Auth.has_valid_session(ws))
                all_off = false;

        if (all_off)
            ov_Websockets.reload_page();

        console.log(log_prefix(websocket) + "attempt relogin after time out");
        if (!websocket.server_error || websocket.server_error.code !== 5000)
            await ov_Websockets.sleep(PERS_ERROR_TIMEOUT, websocket);

        console.log(log_prefix(websocket) + "reconnect");
        await ov_Auth.connect(websocket);

        if (!websocket.is_ready)
            websocket.disconnect();

        else if (await establish_connection(websocket)) {
            if (websocket === ov_Websockets.current_lead_websocket) {
                console.log(log_prefix(websocket) + "reconnected to server - redraw frontend");
                let media = ov_WebRTCs.get_lead();
                media.start_playback();
                View.draw_loops(); //otherwise the var references will be faulty
                View.indicate_loading(false);
            } else {
                View.sync_loops(websocket);
            }
            return;
        }

        console.warn(log_prefix(websocket) + "Pers error. Disconnect and try again.");
        websocket.disconnect();
        if (websocket === ov_Websockets.current_lead_websocket)
            View.indicate_loading(true, "Automatic login reconnect failed. Triggered disconnect from server to try again after timeout. ");

    });

    let promise = establish_connection(ov_Websockets.current_lead_websocket);
    for (let ws of ov_Websockets.list) {
        if (ws !== ov_Websockets.current_lead_websocket) {
            if (ws.is_ready)
                establish_connection(ws).then((value) => {
                    if (!value)
                        ws.disconnect();
                });
            else if (!ws.is_connecting)
                ov_Auth.connect(ws).then((value) => {
                    if (value)
                        establish_connection(ws).then((value2) => {
                            if (!value2)
                                ws.disconnect();
                        });
                });
            else
                ws.disconnect();
        }
    }

    if (await promise) {
        let media = ov_WebRTCs.get_lead();
        media.start_playback();
        View.draw_loops();
        View.indicate_loading(false);
    } else {
        View.indicate_loading(true, "Establishing connection to server failed. Page will reload in 5 seconds...");
        await ov_Websockets.sleep(PERS_ERROR_TIMEOUT);
        ov_Websockets.reload_page();
    }
}

async function establish_connection(websocket) {
    console.log(log_prefix(websocket) + "analyzing connection");
    let result = true;
    if (!websocket.authenticated || !websocket.authorized) {
        result = await ov_Auth.relogin(websocket);
        if (result && (!websocket.authenticated || !websocket.authorized)) {
            console.error("Incomplete session information. Clear incomplete session.");
            ov_Auth.clear_session(websocket);
            result = false;
        }

        if (!result && websocket.is_ready && !ov_Auth.has_valid_session(websocket) && websocket !== ov_Websockets.current_lead_websocket) {
            result = await View.ask_for_relogin();
        }
    }
    if (result && !ov_Websockets.user().roles)
        result = await ov_Auth.collect_roles(websocket);
    if (result && websocket.is_ready && websocket.authorized) {
        let media = ov_WebRTCs.get(websocket);
        if (!media.is_connected)
            await media.init_media_connection();
        if (media.is_connected)
            return true;
    }
    websocket.disconnect();
    return false;
}

async function loadHtml() {
    const response = await fetch('/app/vocs/views/loops/vocs.html');
    const dom = new DOMParser().parseFromString(await response.text(), 'text/html');
    return dom.querySelector(".view");
}

async function loadCSS() {
    const response = await fetch('/app/vocs/views/loops/vocs.css');
    const style = document.createElement('style');
    style.textContent = await response.text();
    return style;
}

export function remove() {
    console.log("(vc) unload");
    ov_Websockets.removeEventListeners();
    if (view_container)
        view_container.replaceChildren();
}

function log_prefix(websocket) {
    return "(" + websocket.server_name + " vocs) ";
}