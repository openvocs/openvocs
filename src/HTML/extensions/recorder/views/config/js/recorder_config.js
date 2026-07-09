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

export async function init(container) {
    view_container = container;
    view_container.replaceChildren(await loadCSS(), await loadHtml());
    View.init(VIEW_ID);
}

export async function render(domain_data, project_id, view_domain_only) {
    let websocket;
    for (let ws of ov_Websockets.list) {
        if (ws.record === true) {
            websocket = ws;
            break;
        }
    }

    if (websocket) {
        let recorded_loops = await ov_Recorder.get_recorded_loops(websocket);
        if (recorded_loops)
            recorded_loops = recorded_loops.map((x) => x.loop);

        let current_loop_id = View.current_loop ? View.current_loop.id : undefined;
        let selected_loop;

        View.clear_loops();

        let proj = domain_data.projects[project_id];

        if (proj.loops)
            for (let id of Object.keys(proj.loops)) {
                let active = recorded_loops.includes(id);
                let loop = View.add_loop(id, proj.loops[id], active);
                if (loop.id === current_loop_id || !selected_loop)
                    selected_loop = loop;
            }

        if (domain_data.loops)
            for (let id of Object.keys(domain_data.loops)) {
                let active = recorded_loops.includes(id);
                let loop = View.add_loop(id, domain_data.loops[id], active);
                loop.domain = true;
                if (view_domain_only)
                    loop.disabled = true;
                if (loop.id === current_loop_id || !selected_loop)
                    selected_loop = loop;
            }

        if (selected_loop)
            View.select_loop(selected_loop);
    }
}

export function remove() {
    console.log("(recorder config) unload");
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