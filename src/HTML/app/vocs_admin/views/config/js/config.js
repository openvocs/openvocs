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

    @author         Anja Bertard

    @date           2023-08-31

    @ingroup        vocs_admin/views/config

    @brief          init and load admin config view
    	
    ---------------------------------------------------------------------------
*/
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import * as ov_DB from "/lib/ov_db.js";
import * as ov_Auth from "/lib/ov_auth.js";
import * as View from "./view.js";

export const VIEW_ID = "vocs_admin_config";
var view_container;

var logout_triggered = false;

export async function render_project(container, user) {
    const PROJECT = "project";
    const DOMAIN = "domain";
    await render(container);

    await View.init(VIEW_ID, view_container, PROJECT);

    reconnect();

    let domain_config;
    let project_config = {
        domain: user.domain
    };

    domain_config = await ov_DB.get_config(DOMAIN, user.domain);

    if (!user.domains.has(user.domain)) {
        if (domain_config.users)
            for (let user_id of Object.keys(domain_config.users))
                domain_config.users[user_id].frozen = true;

        if (domain_config.roles)
            for (let role_id of Object.keys(domain_config.roles))
                domain_config.roles[role_id].frozen = true;

        if (domain_config.loops)
            for (let loop_id of Object.keys(domain_config.loops))
                domain_config.loops[loop_id].frozen = true;
    }

    if (user.project && domain_config.projects) {
        project_config = domain_config.projects[user.project];
        project_config.domain = user.domain;
    }

    View.render_user(user);
    await View.render_project(project_config, domain_config);

    console.log("(project config) View rendered");
}

export async function render_domain(container, user) {
    const DOMAIN = "domain";
    await render(container);

    await View.init(VIEW_ID, view_container, DOMAIN);

    reconnect();

    let domain_config = {};

    if (user.domain) {
        domain_config = await ov_DB.get_config(DOMAIN, user.domain);
    }

    View.render_user(user);
    await View.render_domain(domain_config);

    console.log("(domain config) View rendered");
}

async function render(container) {
    view_container = container;
    view_container.appendChild(await loadCSS());
    view_container.appendChild(await loadHtml());
}

async function reconnect() {
    ov_Websockets.on_disconnect(async (websocket) => {
        if (View.logout_triggered) {
            ov_Websockets.reload_page();
            return;
        }
        View.offline_mode(true);
        if (await ov_Auth.relogin(websocket)) {
            if (ov_Websockets.disconnected_websockets.size === 0)
                View.offline_mode(false);
        }
    });

    if (ov_Websockets.disconnected_websockets.size !== 0) {
        View.offline_mode(true);
        for (let websocket of ov_Websockets.disconnected_websockets.values()) {
            if (await ov_Auth.relogin(websocket)) {
                if (ov_Websockets.disconnected_websockets.size === 0)
                    View.offline_mode(false);
            }
        }
    }
}

export function remove() {
    console.log("(config) unload");
    if (view_container)
        view_container.replaceChildren();
}

window.onbeforeunload = function () {
    //logout_triggered = true;
    //ov_Auth.logout();
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