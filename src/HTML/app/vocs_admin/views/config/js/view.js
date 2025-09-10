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
    @file           view.js

    @ingroup        vocs_admin/views/project

    @brief          manage DOM objects for admin project view
    	
    ---------------------------------------------------------------------------
*/
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import * as ov_Web_Storage from "/lib/ov_utils/ov_web_storage.js";
import * as Project_Settings from "../../project_settings/js/settings.js";
import * as Domain_Settings from "../../domain_settings/js/settings.js";
import * as Config_RBAC from "../../config_rbac/js/rbac.js";
import * as Config_Layout from "../../layout/js/layout.js";

import * as ov_DB from "/lib/ov_db.js";
import * as ov_Auth from "/lib/ov_auth.js";
import * as ov_Vocs from "/lib/ov_vocs.js";
import * as FileIO from "./file_handler.js";

// import custom HTML elements
import ov_Nav from "/components/nav/nav.js";
import ov_Dialog from "/components/dialog/dialog.js";

import * as CSS from "/css/css.js";

var Config_SIP;
var Config_Recorder;

const DOM = {
};

var VIEW_ID;
var view_container;
var Config_Settings;

export var logout_triggered;

export async function init(view_id, container, type) {
    if (SIP)
        Config_SIP = await import("/extensions/sip/views/config/js/sip_config.js");
    if (RECORDER)
        Config_Recorder = await import("/extensions/recorder/views/config/js/recorder_config.js");

    VIEW_ID = view_id;
    view_container = container;
    document.documentElement.className = "dark";

    if (!document.adoptedStyleSheets.includes(await ov_Nav.style_sheet))
        document.adoptedStyleSheets = [...document.adoptedStyleSheets, await ov_Nav.style_sheet];

    if (!document.adoptedStyleSheets.includes(await CSS.loading_id_style_sheet))
        document.adoptedStyleSheets = [...document.adoptedStyleSheets, await CSS.loading_id_style_sheet];

    if (type === "project")
        Config_Settings = Project_Settings;
    else
        Config_Settings = Domain_Settings;

    DOM.loading_screen = document.getElementById("loading_screen");

    DOM.sub_view_nav = document.getElementById("select_subview");
    DOM.sub_view = document.getElementById("config_administration");
    DOM.config_name = document.getElementById("config_name");
    DOM.menu_slider = document.getElementById("menu_slider");
    DOM.menu_button = document.getElementById("menu_button");
    DOM.logout_button = document.getElementById("logout_button");
    DOM.back_button = document.getElementById("back_button");
    DOM.save_button = document.getElementById("save_button");
    DOM.error_report = document.getElementById("error_report");
    DOM.error_dialog = document.getElementById("error_dialog");
    DOM.error_dialog_title = DOM.error_dialog.querySelector("h3");

    DOM.error_dialog_title.innerText = "Error Report";

    DOM.open_file_dialog = document.getElementById("open_file_dialog");
    DOM.import_button = document.getElementById("import_button");
    DOM.export_button = document.getElementById("export_button");

    if (!SIP) {
        document.getElementById("sip_page_button").style.display = "none";
        document.querySelector("#sip_page_button+label").style.display = "none";
    }

    if (!RECORDER) {
        document.getElementById("recorder_page_button").style.display = "none";
        document.querySelector("#recorder_page_button+label").style.display = "none";
    }

    if (!ALLOW_IMPORT_EXPORT) {
        DOM.import_button.style.display = "none";
        DOM.export_button.style.display = "none";
    }

    if (type === "domain") {
        document.getElementById("layout_page_button").style.display = "none";
        document.querySelector("#layout_page_button + label").style.display = "none";
    }

    DOM.loading_screen.addEventListener("loading_button_clicked", () => {
        DOM.menu_slider.toggle();
    });

    DOM.menu_button.addEventListener("click", () => {
        DOM.menu_slider.toggle();
    });

    DOM.logout_button.addEventListener("click", () => {
        logout_triggered = true;
        ov_Auth.logout();
    });

    DOM.back_button.addEventListener("click", () => {
        view_container.dispatchEvent(new CustomEvent("switch_view", {
            detail: { origin: VIEW_ID }
        }));
    });

    DOM.import_button.onclick = function () {
        DOM.open_file_dialog.click();
    };

    DOM.open_file_dialog.onchange = function (event) {
        let settings = Config_Settings.collect();
        let domain = Config_RBAC.collect(settings.domain);
        let local_path = event.target.files[0];
        FileIO.open_local_file(local_path, function (config) {
            let current_config = Config_Settings.collect();
            if (current_config)
                render_project(config, domain, current_config.id, current_config.domain);
            else
                render_project(config, domain);
        });
    };

    DOM.export_button.onclick = function () {
        let config = collect_config();
        let name = config.name ? config.name : config.id;
        FileIO.save_as_json_file(JSON.stringify(config), name);
    };

    await Config_Settings.init(document.getElementById("settings_page"));
    await Config_RBAC.init(document.getElementById("rbac_page"));
    await Config_Layout.init(document.getElementById("layout_page"));
    if (SIP)
        await Config_SIP.init(document.getElementById("sip_page"));
    if (RECORDER)
        await Config_Recorder.init(document.getElementById("recorder_page"));

    DOM.save_button.addEventListener("click", async () => {
        let config;
        if (type === "project") {
            let settings = Config_Settings.collect();
            config = collect_config({ id: settings.domain });
            if (Object.keys(config.users).length || Object.keys(config.roles).length || Object.keys(config.loops).length)
                await save(config, "domain");
        }
        config = collect_config();
        await save(config, type, true);
    });

    view_container.addEventListener("delete_project", async () => {
        DOM.loading_screen.show("Deleting project on server(s)...");
        let errors = [];
        for (let websocket of ov_Websockets.list) {
            if (websocket.port !== "db")
                continue;
            let project = Project_Settings.collect();
            if (!await ov_DB.delete_project(project.domain, project.id, websocket)) {
                errors.push(websocket);
            }
        }

        DOM.loading_screen.hide();

        if (errors.length === 0)
            view_container.dispatchEvent(new CustomEvent("switch_view", {
                detail: { origin: VIEW_ID }
            }));
        else {
            //todo disconnect ?
            DOM.error_dialog_title.innerText = "Deleting project failed on following server(s):";
            DOM.error_report.innerText = "";
            for (let error of errors) {
                console.log(error)
                DOM.error_report.innerText += error.server_name + "\n\n"
            }
            DOM.error_dialog.showModal();
        }
    });

    view_container.addEventListener("delete_domain", async () => {
        DOM.loading_screen.show("Deleting domain on server(s)...");
        let errors = [];
        for (let websocket of ov_Websockets.list) {
            if (websocket.port !== "db")
                continue;
            let domain = Domain_Settings.collect();
            if (!await ov_DB.delete_domain(domain.id, websocket)) {
                errors.push(websocket);
            }
        }

        DOM.loading_screen.hide();
        if (errors.length === 0)
            view_container.dispatchEvent(new CustomEvent("switch_view", {
                detail: { origin: VIEW_ID }
            }));
        else {
            //todo disconnect ?
            DOM.error_dialog_title.innerText = "Deleting domain failed on following server(s):";
            DOM.error_report.innerText = "";
            for (let error of errors) {
                console.log(error)
                DOM.error_report.innerText += error.server_name + "\n\n"
            }
            DOM.error_dialog.showModal();
        }
    });

    view_container.addEventListener("changed_name", (event) => {
        DOM.config_name.innerText = event.detail;
    });

    view_container.addEventListener("import_ldap_user", async (event) => {
        DOM.loading_screen.show("Importing users from LDAP...");
        let settings = Config_Settings.collect();
        let errors = [];
        for (let websocket of ov_Websockets.list) {
            if (websocket.port !== "db")
                continue;
            if (!await ov_DB.user_ldap_import(event.detail.host, event.detail.base,
                settings.id, event.detail.user, event.detail.password, websocket)) {
                errors.push(websocket);
            }
        }

        DOM.loading_screen.hide();

        if (errors.length > 0) {
            DOM.error_dialog_title.innerText = "Importing LDAP users failed on following server(s):";
            DOM.error_report.innerText = "";
            for (let error of errors) {
                console.log(error);
                DOM.error_report.innerText += error.server_name + "\n\n"
            }
            DOM.error_dialog.showModal();
            //todo disconnect ?
        }
    });

    DOM.error_dialog.onclick = (e) => {
        if (e.target === DOM.error_dialog) {
            DOM.error_dialog.close();
        }
    }

    DOM.error_dialog.querySelector(".close_button").onclick = () => {
        DOM.error_dialog.close();
    };
}

async function request_settings(layout_name) {
    return await ov_Vocs.collect_keyset_layout(layout_name, ov_Websockets.current_lead_websocket);
}

function add_sip_to_config(config) {
    let sip = Config_SIP.collect();
    for (let loop_id of Object.keys(sip)) {
        let loop = config.loops[loop_id];
        if (loop)
            loop.sip = sip[loop_id];
        // else
        // console.error("cannot find loop to save SIP config", config.loops, loop_id);
    }
}

function collect_config(settings) {
    let collect_new_nodes = false;
    if (!settings) {
        settings = Config_Settings.collect();
        collect_new_nodes = true;
    }
    let config = { ...settings, ...Config_RBAC.collect(settings.id, collect_new_nodes) };
    let role_config = Config_Layout.collect_role_layout();
    for (let role_id of Object.keys(role_config)) {
        if (config.roles[role_id]) {
            if (role_config[role_id])
                config.roles[role_id].layout = role_config[role_id];
            else
                delete config.roles[role_id].layout;
        }
    }
    if (SIP)
        add_sip_to_config(config);
    return config;
}

async function save(new_config, type, persist) {
    if (new_config.id) {
        DOM.loading_screen.show("Saving " + type + " " + new_config.id + " on server(s)...");
        let errors = [];
        for (let websocket of ov_Websockets.list) {
            if (websocket.port !== "db" || websocket.server_name !== ov_Websockets.prime_websocket.server_name)
                continue;

            //save layout
            if (type === "project")
                await ov_DB.set_keyset_layout(new_config.id, new_config.domain, Config_Layout.collect_page_layout(), websocket);

            //save project or domain data
            let result = true;
            if (type === "project" && !await ov_DB.check_id(new_config.id, type, websocket))
                result = await ov_DB.create(type, new_config.id, "domain", new_config.domain, websocket);

            if (result) {
                result = await ov_DB.update(type, new_config, websocket);
            }

            for (let user_id of Object.keys(new_config.users)) {
                let user = Config_RBAC.users().get(user_id);
                if (result && user.node_password)
                    result = await ov_DB.update_password(user.node_id, user.node_password);
            }

            if (!result && websocket.server_error) {
                errors.push({
                    server_name: websocket.server_name,
                    description: websocket.server_error.description + "\n\n"
                });
            }
            if (persist)
                await ov_DB.persist(websocket);
        }

        DOM.loading_screen.delayed_hide();

        if (errors.length > 0) {
            DOM.error_dialog_title.innerText = "Saving " + type + " failed on following server(s):";
            DOM.error_report.innerText = "";
            for (let error of errors) {
                console.error(error);
                DOM.error_report.innerText += error.server_name + ": " + error.description + "\n\n"
            }
            DOM.error_dialog.showModal();
            //todo disconnect ?
        }

    } else {
        DOM.error_dialog_title.innerText = "Can't save " + type + ":"
        DOM.error_report.innerText = "ID is missing."
        DOM.error_dialog.showModal();
    }
}

export function render_user(user) {
    DOM.menu_slider.value = user.name;
}

export async function render_project(project, domain, id, domain_id, page) {
    id = id ? id : project.id;
    domain_id = domain_id ? domain_id : project.domain;
    let name = project.name ? project.name : id;
    if (name)
        DOM.config_name.innerText = name;
    else
        DOM.config_name.innerText = "[New Project]";
    DOM.sub_view_nav.addEventListener("change", () => {
        for(let ws of ov_Websockets.list){
            ov_Web_Storage.add_anchor_to_session(ws.websocket_url, domain_id, id, DOM.sub_view_nav.value);
        }
        DOM.sub_view.className = DOM.sub_view_nav.value;
        if (DOM.sub_view_nav.value === "rbac")
            Config_RBAC.refresh();
        else if (DOM.sub_view_nav.value === "layout") {
            let proj_config = collect_config();
            let dom_config = collect_config({ id: proj_config.domain });
            let loops = { ...proj_config.loops, ...dom_config.loops };
            Config_Layout.render(proj_config.roles, loops);
        } else if (DOM.sub_view_nav.value === "sip" && SIP) {
            let proj_config = collect_config();
            let dom_config = collect_config({ id: proj_config.domain });
            let roles = { ...proj_config.roles, ...dom_config.roles };
            Config_SIP.render(proj_config.loops, roles);
        } else if (DOM.sub_view_nav.value === "recorder" && RECORDER) {
            let proj_config = collect_config();
            Config_Recorder.render(proj_config.loops);
        }
    });
    if (page)
        DOM.sub_view_nav.value = page;
    else
        DOM.sub_view_nav.value = "settings";

    Project_Settings.render(project, id, domain_id);
    Config_RBAC.render(domain, project);
    let loops = { ...project.loops, ...domain.loops };
    Config_Layout.render(project.roles, loops, await request_settings(id));
    Config_Layout.disable_settings(false);
    let roles = { ...project.roles, ...domain.roles };
    if (SIP)
        Config_SIP.render(project.loops, roles);
    if (RECORDER)
        Config_Recorder.render(project.loops);

}

export async function render_domain(domain, id, page) {
    id = id ? id : domain.id;
    let name = domain.name ? domain.name : id;
    if (name)
        DOM.config_name.innerText = name;
    else
        DOM.config_name.innerText = "[New Domain]";
    DOM.sub_view_nav.addEventListener("change", () => {
        DOM.sub_view.className = DOM.sub_view_nav.value;
        for(let ws of ov_Websockets.list){
            ov_Web_Storage.add_anchor_to_session(ws.websocket_url, id, undefined, DOM.sub_view_nav.value);
        }
        if (DOM.sub_view_nav.value === "rbac")
            Config_RBAC.refresh();
        else if (DOM.sub_view_nav.value === "layout") {
            let config = collect_config();
            let loops = config.loops;
            for (let project_id of Object.keys(domain.projects)) {
                if (domain.projects[project_id].loops)
                    loops = { ...loops, ...domain.projects[project_id].loops };
            }
            // Config_Layout.render(config.roles, loops);
        } else if (DOM.sub_view_nav.value === "sip" && SIP) {
            let config = collect_config();
            let roles = config.roles;
            for (let project_id of Object.keys(domain.projects)) {
                if (domain.projects[project_id].roles)
                    roles = { ...roles, ...domain.projects[project_id].roles };
            }
            Config_SIP.render(config.loops, roles);
        } else if (DOM.sub_view_nav.value === "recorder" && RECORDER) {
            let config = collect_config();
            Config_Recorder.render(config.loops);
        }
    });
    if (page)
        DOM.sub_view_nav.value = page;
    else
        DOM.sub_view_nav.value = "settings";

    Domain_Settings.render(domain, id);
    Config_RBAC.render(domain);
    let loops = domain.loops;
    for (let project_id of Object.keys(domain.projects)) {
        if (domain.projects[project_id].loops)
            loops = { ...loops, ...domain.projects[project_id].loops };
    }
    if (SIP) {
        let roles = domain.roles;
        for (let project_id of Object.keys(domain.projects)) {
            if (domain.projects[project_id].roles)
                roles = { ...roles, ...domain.projects[project_id].roles };
        }
        Config_SIP.render(domain.loops, roles);
    }
    if (RECORDER)
        Config_Recorder.render(domain.loops);
}

export function offline_mode(value) {
    view_container.classList.toggle("offline", value);
    DOM.save_button.disabled = value;
    Config_Settings.offline_mode(value);
}

export function display_loading_screen(value, message) {
    if (value)
        DOM.loading_screen.show(message);
    else
        DOM.loading_screen.hide();
}