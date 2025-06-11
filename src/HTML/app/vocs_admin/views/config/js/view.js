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
        
    @author         Anja Bertard

    @date           2023-08-16

    @ingroup        vocs_admin/views/project

    @brief          manage DOM objects for admin project view
    	
    ---------------------------------------------------------------------------
*/
import * as ov_Websockets from "/lib/ov_websocket_list.js";
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

const DOM = {
};

var VIEW_ID;
var view_container;
var Config_Settings;

export var logout_triggered;

export async function init(view_id, container, type) {
    if (SIP)
        Config_SIP = await import("/extensions/sip/views/config/js/sip_config.js");

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

    DOM.error_dialog_title.innerText = "Error Report"

    if (!SIP) {
        document.getElementById("sip_page_button").style.display = "none";
        document.querySelector("#sip_page_button+label").style.display = "none";
    }

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

    DOM.open_file_dialog = document.getElementById("open_file_dialog");
    DOM.import_button = document.getElementById("import_button");
    DOM.export_button = document.getElementById("export_button");

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

    view_container.addEventListener("delete_node", async (event) => {
        let node = event.detail.node;
        DOM.loading_screen.show("Deleting " + node.type + " with id " + node.node_id + " on server(s)...");
        let errors = [];
        for (let websocket of ov_Websockets.list) {
            if (!await ov_DB.erase(node.type, node.node_id, websocket)) {
                errors.push(websocket);
            }
        }

        DOM.loading_screen.hide();

        DOM.error_dialog_title.innerText = "Deleting " + node.type + " with id " + node.node_id + " failed on following server(s):";
        DOM.error_report.innerText = "";
        for (let error of errors) {
            console.log(error)
            DOM.error_report.innerText += error.server_name + "\n\n"
        }
        DOM.error_dialog.showModal();
    });

    view_container.addEventListener("delete_project", async () => {
        DOM.loading_screen.show("Deleting project on server(s)...");
        let errors = [];
        for (let websocket of ov_Websockets.list) {
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

async function request_settings() {
    let layout_name = window.innerHeight + "x" + window.innerWidth;
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

            //save layout
            if (type === "domain") {
                let layout_name = window.innerHeight + "x" + window.innerWidth;
                await ov_DB.set_keyset_layout(layout_name, new_config.id, Config_Layout.collect_page_layout(), websocket);
            }

            //save project or domain data
            let result = true;
            if (type === "project" && !await ov_DB.check_id(new_config.id, type, websocket))
                result = await ov_DB.create(type, new_config.id, "domain", new_config.domain, websocket);

            let server_conf;
            if (result)
                server_conf = await ov_DB.get_config(type, new_config.id, websocket);
            if (!server_conf)
                result = false;

            if (result) {
                let create_requests = [];
                for (let user_id of Object.keys(new_config.users)) {
                    if (!server_conf.users || !server_conf.users[user_id]) {
                        if ("admin" || !await ov_DB.check_id(user_id, "user", websocket))
                            create_requests.push(ov_DB.create("user", user_id, type, new_config.id, websocket));
                        else {
                            errors.push({
                                server_name: websocket.server_name,
                                description: "user ID " + user_id + " already exists."
                            });
                            result = false;
                        }
                    }
                }
                for (let role_id of Object.keys(new_config.roles)) {
                    if (!server_conf.roles || !server_conf.roles[role_id]) {
                        if ("admin" || !await ov_DB.check_id(role_id, "role", websocket))
                            create_requests.push(ov_DB.create("role", role_id, type, new_config.id, websocket));
                        else {
                            errors.push({
                                server_name: websocket.server_name,
                                description: "role ID " + role_id + " already exists."
                            });
                            result = false;
                        }
                    }
                }
                for (let loop_id of Object.keys(new_config.loops)) {
                    if (!server_conf.loops || !server_conf.loops[loop_id]) {
                        if (!await ov_DB.check_id(loop_id, "loop", websocket))
                            create_requests.push(ov_DB.create("loop", loop_id, type, new_config.id, websocket));
                        else {
                            errors.push({
                                server_name: websocket.server_name,
                                description: "loop ID " + loop_id + " already exists."
                            });
                            result = false;
                        }
                    }
                }
                if (result) {
                    result = await Promise.all(create_requests);
                    result = !result.includes(false);
                }
            }

            for (let user_id of Object.keys(new_config.users)) {
                let user = Config_RBAC.users().get(user_id);
                if (result && user.node_password)
                    result = await ov_DB.update_password(user.node_id, user.node_password);
            }

            if (result) {
                result = await ov_DB.update(type, new_config, websocket);
            }

            if (result) {
                let delete_requests = [];
                if (server_conf.users)
                    for (let user_id of Object.keys(server_conf.users)) {
                        if (!new_config.users[user_id])
                            delete_requests.push(ov_DB.delete_user(user_id, websocket));
                    }
                if (server_conf.roles)
                    for (let role_id of Object.keys(server_conf.roles)) {
                        if (!new_config.roles[role_id])
                            delete_requests.push(ov_DB.delete_role(role_id, websocket));
                    }
                if (server_conf.loops)
                    for (let loop_id of Object.keys(server_conf.loops)) {
                        if (!new_config.loops[loop_id])
                            delete_requests.push(ov_DB.delete_loop(loop_id, websocket));
                    }
                result = await Promise.all(delete_requests);
                result = !result.includes(false);
            }

            if (!result && websocket.server_error) {
                errors.push({
                    server_name: websocket.server_name,
                    description: websocket.server_error.description + "\n\n"
                });
            }
            if (persist)
                ov_DB.persist(websocket);
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

export async function render_project(project, domain, id, domain_id) {
    id = id ? id : project.id;
    domain_id = domain_id ? domain_id : project.domain;
    let name = project.name ? project.name : id;
    if (name)
        DOM.config_name.innerText = name;
    else
        DOM.config_name.innerText = "[New Project]";
    DOM.sub_view_nav.addEventListener("change", () => {
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
        }
    });
    DOM.sub_view_nav.value = "settings";

    Project_Settings.render(project, id, domain_id);
    Config_RBAC.render(domain, project);
    let loops = { ...project.loops, ...domain.loops };
    Config_Layout.render(project.roles, loops, await request_settings());
    Config_Layout.disable_settings(true);
    let roles = { ...project.roles, ...domain.roles };
    if (SIP)
        Config_SIP.render(project.loops, roles);
}

export async function render_domain(domain, id) {
    id = id ? id : domain.id;
    let name = domain.name ? domain.name : id;
    if (name)
        DOM.config_name.innerText = name;
    else
        DOM.config_name.innerText = "[New Domain]";
    DOM.sub_view_nav.addEventListener("change", () => {
        DOM.sub_view.className = DOM.sub_view_nav.value;
        if (DOM.sub_view_nav.value === "rbac")
            Config_RBAC.refresh();
        else if (DOM.sub_view_nav.value === "layout") {
            let config = collect_config();
            let loops = config.loops;
            for (let project_id of Object.keys(domain.projects)) {
                if (domain.projects[project_id].loops)
                    loops = { ...loops, ...domain.projects[project_id].loops };
            }
            Config_Layout.render(config.roles, loops);
        } else if (DOM.sub_view_nav.value === "sip" && SIP) {
            let config = collect_config();
            let roles = config.roles;
            for (let project_id of Object.keys(domain.projects)) {
                if (domain.projects[project_id].roles)
                    roles = { ...roles, ...domain.projects[project_id].roles };
            }
            Config_SIP.render(config.loops, roles);
        }
    });
    DOM.sub_view_nav.value = "settings";

    Domain_Settings.render(domain, id);
    Config_RBAC.render(domain);
    let loops = domain.loops;
    for (let project_id of Object.keys(domain.projects)) {
        if (domain.projects[project_id].loops)
            loops = { ...loops, ...domain.projects[project_id].loops };
    }
    Config_Layout.render(domain.roles, loops, await request_settings());
    Config_Layout.disable_settings(false);
    if (SIP) {
        let roles = domain.roles;
        for (let project_id of Object.keys(domain.projects)) {
            if (domain.projects[project_id].roles)
                roles = { ...roles, ...domain.projects[project_id].roles };
        }
        Config_SIP.render(domain.loops, roles);
    }
}

export function offline_mode(value) {
    view_container.classList.toggle("offline", value);
    DOM.save_button.disabled = value;
    Config_Settings.offline_mode(value);
}