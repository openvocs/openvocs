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
    @file           recorder_config.js

    @ingroup        extensions/recorder

    @brief          init and load recorder config view
    	
    ---------------------------------------------------------------------------
*/
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import * as ov_Recorder from "/extensions/recorder/ov_recorder.js";
import * as View from "./view.js";

export const VIEW_ID = "vocs_admin_recorder";
var view_container;

export { collect } from "./view.js";

export async function init(container) {
    view_container = container;
    view_container.replaceChildren(await loadCSS(), await loadHtml());
    View.init(VIEW_ID);
}

export async function render(loops) {
    let recorded_loops = await ov_Recorder.get_recorded_loops();
    let first_loop;

    View.clear_loops();

    if (loops)
        for (let id of Object.keys(loops)) {
            let active = recorded_loops.includes(id)
            let loop = View.add_loop(id, loops[id], active);
            if (!first_loop)
                first_loop = loop;
        }

    if (first_loop)
        View.select_loop(first_loop);

    /*if (!await ov_DB.domains() || !await ov_DB.projects()){
        ov_Websockets.prime_websocket.disconnect();
    }

    View.draw(ov_Websockets.user());

    console.log("(overview) View rendered");

    ov_Websockets.prime_websocket.addEventListener("disconnected", on_disconnect);*/
}

export function remove() {
    console.log("(recorder config) unload");
    ov_Websockets.prime_websocket.removeEventListener("disconnected", on_disconnect);
    if (view_container)
        view_container.replaceChildren();
}

async function loadHtml() {
    const response = await fetch('/extensions/recorder/views/config/recorder_config.html');
    const dom = new DOMParser().parseFromString(await response.text(), 'text/html');
    return dom.querySelector('.view');
}

async function loadCSS() {
    const response = await fetch('/extensions/recorder/views/config/recorder_config.css');
    const style = document.createElement('style');
    style.textContent = await response.text();
    return style;
}

async function on_disconnect() {
    /*if (View.logout_triggered) {
        ov_Websockets.reload_page();
        return;
    }
    console.warn("Disconnected from prime server. Trying to reconnect...");
    View.display_loading_screen(true, "Disconnected from prime server. Trying to reconnect...");
    if (await ov_Auth.relogin(ov_Websockets.prime_websocket))
        View.display_loading_screen(false);*/
}