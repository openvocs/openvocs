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

    @ingroup        vocs_admin/views/auth

    @brief          init and load auth view
    	
    ---------------------------------------------------------------------------
*/
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import * as ov_Auth from "/lib/ov_auth.js";
import * as View from "./view.js";

export const VIEW_ID = VIEW.AUTH;
var view_container;

export async function render(container) {
    view_container = container;
    view_container.replaceChildren(await loadCSS());
    view_container.appendChild(await loadHtml());

    ov_Websockets.on_disconnect(disconnect_handler);

    View.init(VIEW_ID);

    console.log("(auth) View rendered");
}

export function remove() {
    console.log("(login) unload login client");
    if (view_container !== undefined)
        view_container.replaceChildren();
}

window.onbeforeunload = function () {
    remove();
}

async function loadHtml() {
    const response = await fetch('/app/vocs_admin/views/auth/auth.html');
    const dom = new DOMParser().parseFromString(await response.text(), 'text/html');
    return dom.querySelector('.view');
}

async function loadCSS() {
    const response = await fetch('/app/vocs_admin/views/auth/auth.css');
    const style = document.createElement('style');
    style.textContent = await response.text();
    return style;
}

async function disconnect_handler(websocket, error) {
    console.log("(login) logged out");
    console.warn("(login) Lead server disconnected.");
    View.display_disconnect_notice(error);
    if (error && error.code !== 5000)
        await ov_Websockets.sleep(PERS_ERROR_TIMEOUT, websocket);
    await ov_Auth.connect(websocket);
    View.set_message("");
}