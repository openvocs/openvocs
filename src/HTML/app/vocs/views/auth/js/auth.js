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
    @file           auth.js

    @ingroup        vocs/views/auth

    @brief          init and load auth view
    	
    ---------------------------------------------------------------------------
*/
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import * as ov_Auth from "/lib/ov_auth.js";
import * as View from "./view.js";

export const VIEW_ID = "authentication_authorization";
var view_container;
var removed = false;

export async function render(container) {
    view_container = container;
    view_container.appendChild(await loadCSS());
    view_container.appendChild(await loadHtml());

    ov_Websockets.on_disconnect(disconnect_handler);// prime_websocket.addEventListener("disconnected", disconnect_handler);

    View.init(VIEW_ID);

    console.log("(auth) View rendered");
    removed = false;

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
        }
    });
}

export function remove() {
    console.log("(auth) Unload view");
    ov_Websockets.removeEventListeners();//prime_websocket.removeEventListener("disconnected", disconnect_handler);
    if (!!view_container)
        view_container.replaceChildren();
    removed = true;
}

window.onbeforeunload = function () {
    remove();
}

async function loadHtml() {
    const response = await fetch('/app/vocs/views/auth/auth.html');
    const dom = new DOMParser().parseFromString(await response.text(), 'text/html');
    return dom.querySelector('.view');
}

async function loadCSS() {
    const response = await fetch('/app/vocs/views/auth/auth.css');
    const style = document.createElement('style');
    style.textContent = await response.text();
    return style;
}

async function disconnect_handler(websocket) {
    if (ov_Websockets.list.length === ov_Websockets.disconnected_websockets.size) {
        console.log("(auth) Logged out");
        console.warn("(auth) Lead server disconnected.");
        View.display_disconnect_notice(websocket.server_error);
    }
    if (websocket.server_error && (websocket.server_error.code === 5000))
        ov_Auth.connect(websocket);
    else {
        if (websocket === ov_Websockets.current_lead_websocket) {
            ov_Websockets.find_new_connected_server().then((ws) => {
                if (ws !== undefined && ws !== ov_Websockets.current_lead_websocket) {
                    console.log("switch lead server to", ws.server_name);
                    ov_Websockets.switch_lead_websocket(ws);
                    View.set_server_id();
                }
            });
        }
        await ov_Websockets.sleep(PERS_ERROR_TIMEOUT, websocket);
        if (!removed && await ov_Auth.connect(websocket))
            View.set_message("");
    }
}