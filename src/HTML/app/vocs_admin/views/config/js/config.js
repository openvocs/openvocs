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
    @file           config.js

    @ingroup        vocs_admin/views/config

    @brief          init and load admin config view
    	
    ---------------------------------------------------------------------------
*/
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import * as ov_DB from "/lib/ov_db.js";
import * as ov_Auth from "/lib/ov_auth.js";
import * as View from "./view.js";

export const VIEW_ID = VIEW.CONFIG;
var view_container;
const PROJECT = "project";
const DOMAIN = "domain";

export async function render(container, page) {
    view_container = container;
    view_container.replaceChildren(await loadCSS());
    view_container.appendChild(await loadHtml());

    ov_Websockets.on_disconnect(on_disconnect);

    if (!await ov_DB.domains() || !await ov_DB.projects())
        ov_Websockets.prime_websocket.disconnect(); 

    let user = ov_Websockets.user();
    if (!user.project){
        for (let project of user.projects.values()) {
            if (project.domain === user.domain) {
                user.project = project.id;
                break;
            }
        }
    }

    await View.init(VIEW_ID, view_container);

    let domain_config = await ov_DB.get_config(DOMAIN, user.domain);
    user.admin = user.domains.has(user.domain) ? DOMAIN : PROJECT;
    View.render_user(user);
    View.render(domain_config, page);

    console.log("(project config) View rendered");
}

async function on_disconnect(websocket, error) {
    console.warn("Disconnected from one or several servers. Trying to reconnect...");
    View.offline_mode(true);
    await ov_Websockets.sleep(PERS_ERROR_TIMEOUT, websocket);
    await ov_Auth.connect(websocket);
    let session = websocket.session;
    if (session) { //auto login with session
        await ov_Auth.login(session.user, session.session, websocket);
        await ov_DB.domains();
        await ov_DB.projects();
        let user = ov_Websockets.user();
        user.admin = user.domains && user.domains.has(user.domain) ? DOMAIN : PROJECT;
        if (websocket.authenticated && ov_Websockets.disconnected_websockets.size === 0) {
            View.offline_mode(false);
        }
    }
}

export function remove() {
    console.log("(config) unload");
    if (view_container)
        view_container.replaceChildren();
}

window.onbeforeunload = function () {
    remove();
}

async function loadHtml() {
    const response = await fetch('/app/vocs_admin/views/config/config.html');
    const dom = new DOMParser().parseFromString(await response.text(), 'text/html');
    return dom.querySelector('.view');
}

async function loadCSS() {
    const response = await fetch('/app/vocs_admin/views/config/config.css');
    const style = document.createElement('style');
    style.textContent = await response.text();
    return style;
}