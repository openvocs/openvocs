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
import * as ov_Auth from "/lib/ov_auth.js";
import * as ov_DB from "/lib/ov_db.js";
import * as View from "./view.js";

export const VIEW_ID = "vocs_admin_sip";
var view_container;

export { collect } from "./view.js";

export async function init(container) {
    view_container = container;
    view_container.replaceChildren(await loadCSS(), await loadHtml());
    View.init(VIEW_ID);
}

export function render(loops, roles) {
    let first_loop;

    View.clear_loops();

    if (loops)
        for (let id of Object.keys(loops)) {
            let loop = View.add_loop(id, loops[id], roles);
            if(loops[id].global)
                loop.global = true;
            if (loops[id].frozen)
                loop.disabled = true;
            if (!first_loop)
                first_loop = loop;
        }

    if (first_loop)
        View.select_loop(first_loop);
}

export function remove() {
    console.log("(overview) unload");
    if (view_container)
        view_container.replaceChildren();
}

async function loadHtml() {
    const response = await fetch('/extensions/sip/views/config/sip_config.html');
    const dom = new DOMParser().parseFromString(await response.text(), 'text/html');
    return dom.querySelector('.view');
}

async function loadCSS() {
    const response = await fetch('/extensions/sip/views/config/sip_config.css');
    const style = document.createElement('style');
    style.textContent = await response.text();
    return style;
}