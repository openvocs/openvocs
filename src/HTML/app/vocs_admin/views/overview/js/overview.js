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
    @file           overview.js

    @ingroup        vocs_admin/views/overview

    @brief          init and load overview view
    	
    ---------------------------------------------------------------------------
*/
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import * as ov_Web_Storage from "/lib/ov_utils/ov_web_storage.js";
import * as ov_Auth from "/lib/ov_auth.js";
import * as ov_DB from "/lib/ov_db.js";
import * as View from "./view.js";

export const VIEW_ID = "vocs_admin_overview";
var view_container;

export async function render(container) {
    view_container = container;
    view_container.appendChild(await loadCSS());
    view_container.appendChild(await loadHtml());

    View.init(VIEW_ID);

    if (!await ov_DB.domains() || !await ov_DB.projects()) {
        ov_Websockets.prime_websocket.disconnect();
    }

    View.draw(ov_Websockets.user());

    for (let ws of ov_Websockets.list) {
        ov_Web_Storage.add_anchor_to_session(APP, ws.websocket_url, undefined, undefined, "overview");
    }

    console.log("(overview) View rendered");

    ov_Websockets.on_disconnect(on_disconnect);
}
export function remove() {

    console.log("(overview) unload");
    if (view_container)
        view_container.replaceChildren();
}

async function loadHtml() {
    const response = await fetch('/app/vocs_admin/views/overview/overview.html');
    const dom = new DOMParser().parseFromString(await response.text(), 'text/html');
    return dom.querySelector('.view');
}

async function loadCSS() {
    const response = await fetch('/app/vocs_admin/views/overview/overview.css');
    const style = document.createElement('style');
    style.textContent = await response.text();
    return style;
}

async function on_disconnect(ws) {
    if (View.logout_triggered) {
        ov_Websockets.reload_page();
        return;
    }
    console.warn("Disconnected from one or several servers. Trying to reconnect...");
    View.display_loading_screen(true, "Disconnected from one or several servers. Trying to reconnect...");
    await ov_Websockets.sleep(PERS_ERROR_TIMEOUT, ws);
    if (await ov_Auth.relogin(ws) && ov_Websockets.disconnected_websockets.size === 0)
        if (await ov_DB.domains() && await ov_DB.projects())
            View.display_loading_screen(false);
}