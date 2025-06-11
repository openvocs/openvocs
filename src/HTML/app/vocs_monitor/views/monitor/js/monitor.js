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
    @file           monitor.js

    @author         Anja Bertard

    @date           2025-04-29

    @ingroup        vocs_monitor

    @brief          init and load the monitor client

    ---------------------------------------------------------------------------
*/

import * as View from "./view.js";

import * as ov_Websockets from "/lib/ov_websocket_list.js";
import * as ov_Auth from "/lib/ov_auth.js";
import * as ov_Monitor from "/lib/ov_monitor.js";

export const VIEW_ID = "vocs_monitor";
var view_container;

export async function render(container) {
    view_container = container;
    view_container.appendChild(await loadCSS());
    view_container.appendChild(await loadHtml());

    View.init(VIEW_ID);

    console.log("(monitor) View rendered");

    ov_Websockets.prime_websocket.addEventListener("disconnected", on_disconnect);
}

export function remove() {
    console.log("(monitor) unload");
    ov_Websockets.prime_websocket.removeEventListener("disconnected", on_disconnect);
    if (view_container)
        view_container.replaceChildren();
}

async function loadHtml() {
    const response = await fetch('/app/vocs_monitor/views/monitor/monitor.html');
    const dom = new DOMParser().parseFromString(await response.text(), 'text/html');
    return dom.querySelector('.view'); 
}

async function loadCSS() {
    const response = await fetch('/app/vocs_monitor/views/monitor/monitor.css');
    const style = document.createElement('style');
    style.textContent = await response.text();
    return style;
}

async function on_disconnect() {
    if (View.logout_triggered) {
        ov_Websockets.reload_page();
        return;
    }
    console.warn("Disconnected from server. Trying to reconnect...");
    View.display_loading_screen(true, "Disconnected from server. Trying to reconnect...");
    if (await ov_Auth.relogin(ov_Websockets.prime_websocket))
        View.display_loading_screen(false);
}