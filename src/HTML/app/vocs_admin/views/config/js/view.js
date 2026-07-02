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
import * as Settings from "../../settings/js/settings.js";
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

var domain;

export async function init(view_id, container) {
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

    DOM.loading_screen = document.getElementById("loading_screen");

    DOM.sub_view_nav = document.getElementById("select_subview");
    DOM.sub_view = document.getElementById("config_administration");
    DOM.config_project_name = document.getElementById("config_project_name");
    DOM.config_domain_name = document.getElementById("config_domain_name");
    DOM.menu_slider = document.getElementById("menu_slider");
    DOM.menu_button = document.getElementById("menu_button");
    DOM.logout_button = document.getElementById("logout_button");
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

    DOM.loading_screen.addEventListener("loading_button_clicked", () => {
        DOM.menu_slider.toggle();
    });

    DOM.menu_button.addEventListener("click", () => {
        DOM.menu_slider.toggle();
    });

    DOM.logout_button.addEventListener("click", async () => {
        await ov_Auth.logout();
        ov_Websockets.reload_page();
    });

    // todo: rewrite import/export
    // DOM.import_button.onclick = function () {
    //     DOM.open_file_dialog.click();
    // };

    // DOM.open_file_dialog.onchange = function (event) {
    //     let settings = Settings.collect();
    //     let domain = Config_RBAC.collect(settings.id);
    //     let local_path = event.target.files[0];
    //     FileIO.open_local_file(local_path, function (config) {
    //         let current_config = Settings.collect();
    //         if (current_config)
    //             render_project(config, domain, current_config.id, current_config.domain);
    //         else
    //             render_project(config, domain);
    //     });
    // };

    // DOM.export_button.onclick = function () {
    //     let config = collect_config();
    //     let name = config.name ? config.name : config.id;
    //     FileIO.save_as_json_file(JSON.stringify(config), name);
    // };

    DOM.sub_view.addEventListener("ui_update_domain_users", (event) => {
        // let proj_config = collect_config();
        // let dom_config = collect_config({ id: proj_config.domain });
        // dom_config.users = event.detail;
        // Config_RBAC.render(dom_config, proj_config); -> Graph.clear() does not work properly
    });

    let auth_ldap = await ov_DB.check_ldap();
    await Settings.init(document.getElementById("settings_page"), auth_ldap);
    await Config_RBAC.init(document.getElementById("rbac_page"), auth_ldap);
    await Config_Layout.init(document.getElementById("layout_page"));
    if (SIP)
        await Config_SIP.init(document.getElementById("sip_page"));
    if (RECORDER)
        await Config_Recorder.init(document.getElementById("recorder_page"));

    DOM.save_button.addEventListener("click", async () => {
        // await save(collect_config());
        await save();
    });

    let user = ov_Websockets.user();

    DOM.sub_view.addEventListener("new_project", (event) => {
        domain.projects[event.detail] = {};
    });

    DOM.sub_view.addEventListener("changed_project", (event) => {
        user.project = event.detail;
        for (let ws of ov_Websockets.list)
            ov_Web_Storage.add_anchor_to_session(APP, ws.websocket_url, user.domain, user.project, DOM.sub_view_nav.value);
        let project = domain.projects[event.detail];
        update_project_name_display(project.name, project.id);
    });

    DOM.sub_view.addEventListener("delete_project", async (event) => {
        delete domain.projects[event.detail];
    });

    DOM.sub_view.addEventListener("changed_domain_name", (event) => {
        domain.name = event.detail;
        DOM.config_domain_name.innerText = event.detail;
    });

    DOM.sub_view.addEventListener("changed_project_id", (event) => {
        let project = domain.projects[event.detail.old_id];
        delete domain.projects[event.detail.old_id];
        project.id = event.detail.new_id;
        domain.projects[event.detail.new_id] = project;
        user.project = project.id;
        for (let ws of ov_Websockets.list)
            ov_Web_Storage.add_anchor_to_session(APP, ws.websocket_url, user.domain, user.project, DOM.sub_view_nav.value);
        update_project_name_display(project.name, project.id);
    });

    DOM.sub_view.addEventListener("changed_project_name", (event) => {
        domain.projects[event.detail.id].name = event.detail.name;
        update_project_name_display(event.detail.name, event.detail.id);
    });

    DOM.sub_view.addEventListener("save_node", (event) => {
        if (event.detail.update) { // update node
            let type = event.detail.node.type + "s";
            let new_data = event.detail.node.data;
            if (event.detail.node.node_password)
                new_data.password = event.detail.node.node_password;
            let node;
            if (!event.detail.node.subset) {
                node = domain[type] && domain[type][event.detail.node.node_id] ? domain[type][event.detail.node.node_id] :
                    domain.projects[user.project][type][event.detail.node.node_id];
            } else if (event.detail.node.subset === domain.id) {
                if (!domain[type] || !domain[type][event.detail.node.node_id]) {
                    if (!domain[type])
                        domain[type] = {};
                    domain[type][event.detail.node.node_id] = domain.projects[user.project][type][event.detail.node.node_id];
                    delete domain.projects[user.project][type][event.detail.node.node_id];
                }
                node = domain[type][event.detail.node.node_id];
            } else {
                if (!domain.projects[user.project][type] || !domain.projects[user.project][type][event.detail.node.node_id]) {
                    if (!domain.projects[user.project][type])
                        domain.projects[user.project][type] = {};
                    domain.projects[user.project][type][event.detail.node.node_id] = domain[type][event.detail.node.node_id];
                    delete domain[type][event.detail.node.node_id];
                }
                node = domain.projects[user.project][type][event.detail.node.node_id];
            }
            for (let field of Object.keys(new_data))
                node[field] = new_data[field];
        } else { // new node
            let type = event.detail.node.type + "s";
            let data = event.detail.node.data;
            data.password = event.detail.node.node_password;
            if (event.detail.node.subset === domain.id) {
                if (!domain[type])
                    domain[type] = {};
                domain[type][event.detail.node.node_id] = data;
            } else {
                if (!domain.projects[user.project][type])
                    domain.projects[user.project][type] = {};
                domain.projects[user.project][type][event.detail.node.node_id] = data;
            }
        }
    });

    DOM.sub_view.addEventListener("delete_node", (event) => {
        let type = event.detail.node.type + "s";
        if (event.detail.node.subset === domain.id && domain[type]) {
            delete domain[type][event.detail.node.node_id];
        } else if (event.detail.node.subset === user.project && domain.projects[user.project][type]) {
            delete domain.projects[user.project][type][event.detail.node.node_id];
        }
    });

    var source;
    DOM.sub_view.addEventListener("edit_edges", (event) => {
        source = event.detail.value ? event.detail.node : undefined;
    });

    DOM.sub_view.addEventListener("add_edge", (event) => {
        update_edge(source, event.detail.target, user.project);
    });

    DOM.sub_view.addEventListener("delete_edge", (event) => {
        update_edge(source, event.detail.target, user.project);
    });

    DOM.sub_view.addEventListener("change_grid", (event) => {
        domain.layout[user.project] = event.detail;
    });

    DOM.sub_view_nav.addEventListener("change", () => {
        for (let ws of ov_Websockets.list)
            ov_Web_Storage.add_anchor_to_session(APP, ws.websocket_url, user.domain, user.project, DOM.sub_view_nav.value);
        DOM.sub_view.className = DOM.sub_view_nav.value;
        if (DOM.sub_view_nav.value === "rbac")
            Config_RBAC.render(domain, user.project, !user.domains.has(user.domain));
        else if (DOM.sub_view_nav.value === "layout")
            Config_Layout.render(domain, user.project, !user.domains.has(user.domain));
        else if (DOM.sub_view_nav.value === "sip" && SIP)
            Config_SIP.render(domain, user.project, !user.domains.has(user.domain));
        else if (DOM.sub_view_nav.value === "recorder" && RECORDER)
            Config_Recorder.render(domain, user.project, !user.domains.has(user.domain));
    });

    DOM.error_dialog.onclick = (e) => {
        if (e.target === DOM.error_dialog)
            DOM.error_dialog.close();
    }

    DOM.error_dialog.querySelector(".close_button").onclick = () => {
        DOM.error_dialog.close();
    };
}

function update_project_name_display(name, id) {
    name = name ? name : id ? id : "[New Project]";
    DOM.config_project_name.innerText = name;
}

function update_edge(source, target, project) {
    let node;
    if (target.type === "loop") {
        if (target.subset === domain.id)
            node = domain.loops[target.node_id];
        else
            node = domain.projects[project].loops[target.node_id];
        let new_data = target.data;
        for (let field of Object.keys(new_data))
            node[field] = new_data[field];
    } else if (source.type === "loop") {
        if (source.subset === domain.id)
            node = domain.loops[source.node_id];
        else
            node = domain.projects[project].loops[source.node_id];
        let new_data = source.data;
        for (let field of Object.keys(new_data))
            node[field] = new_data[field];
    } else if (target.type === "user") {
        if (source.subset === domain.id)
            node = domain.roles[source.node_id];
        else
            node = domain.projects[project].roles[source.node_id];
        let new_data = source.data;
        for (let field of Object.keys(new_data))
            node[field] = new_data[field];
    } else if (source.type === "user") {
        if (target.subset === domain.id)
            node = domain.roles[target.node_id];
        else
            node = domain.projects[project].roles[target.node_id];
        let new_data = target.data;
        for (let field of Object.keys(new_data))
            node[field] = new_data[field];
    }
}

async function save() {
    let errors = [];
    let user = ov_Websockets.user();
    if (user.admin === "domain") {
        if (domain.id) {
            DOM.loading_screen.show("Saving domain " + domain.id + " on server(s)...");
            let orig_domain = await ov_DB.get_config("domain", user.domain);
            await delete_from_server("user", orig_domain, domain);
            await delete_from_server("role", orig_domain, domain);
            await delete_from_server("loop", orig_domain, domain)
            for (let project_id of Object.keys(orig_domain.projects)) {
                let project = domain.projects[project_id];
                if (!project) {
                    await ov_DB.remove("project", project_id);
                    continue;
                }
                let orig_project = orig_domain.projects[project_id];
                await delete_from_server("user", orig_project, project);
                await delete_from_server("role", orig_project, project);
                await delete_from_server("loop", orig_project, project);
            }
            // update domain name
            await update_on_server("user", orig_domain, domain, "domain");
            await update_on_server("role", orig_domain, domain, "domain");
            await update_on_server("loop", orig_domain, domain, "domain");

            for (let project_id of Object.keys(domain.projects)) {
                let project = domain.projects[project_id];
                if (project.id) {
                    DOM.loading_screen.show("Saving project " + project.id + " on server(s)...");
                    let orig_project = orig_domain.projects[project_id];
                    if (!orig_project) {
                        await ov_DB.create("project", project.id, "domain", domain.id);
                        await ov_DB.update("project", project);
                    } else {
                        //update project name
                        await update_on_server("user", orig_project, project, "project");
                        await update_on_server("role", orig_project, project, "project");
                        await update_on_server("loop", orig_project, project, "project");
                    }
                } else
                    errors.push("Project ID is missing\n\n");
            }

            // let result = { updated: true };
            // if (Object.keys(domain.users).length || Object.keys(domain.roles).length || Object.keys(domain.loops).length) {
            //     let tmp_domain_data = {
            //         id: domain.id,
            //         name: domain.name,
            //         users: domain.users,
            //         roles: domain.roles,
            //         loops: domain.loops,
            //         layout: domain.layout
            //     };
            //     result = await ov_DB.update("domain", tmp_domain_data);
            // }

            // if (domain.users)
            //     for (let user_id of Object.keys(domain.users))
            //         if (result.updated && domain.users[user_id].password)
            //             result = await ov_DB.update_password(user_id, domain.users[user_id].password);

            // for (let project_id of Object.keys(domain.projects)) {
            //     let project = domain.projects[project_id];
            //     if (project.id) {
            //         DOM.loading_screen.show("Saving project " + project.id + " on server(s)...");
            //         if (result.updated && !await ov_DB.check_id(project.id, "project"))
            //             result = await ov_DB.create("project", project.id, "domain", domain.id);
            //         if (result.updated)
            //             result = await ov_DB.update("project", project);

            //         if (project.users)
            //             for (let user_id of Object.keys(project.users))
            //                 if (result.updated && project.users[user_id].password)
            //                     result = await ov_DB.update_password(user_id, project.users[user_id].password);
            //     } else
            //         errors.push("Project ID is missing\n\n");
            // }

            // if (!result.updated)
            //     errors.push(result.error.description + "\n\n");
        } else
            errors.push("Domain ID is missing\n\n");
    }

    await ov_DB.persist();

    DOM.loading_screen.delayed_hide();

    if (errors.length > 0) {
        DOM.error_dialog_title.innerText = "Saving failed:";
        DOM.error_report.innerText = "";
        for (let error of errors) {
            console.error(error);
            DOM.error_report.innerText += error + "\n\n"
        }
        DOM.error_dialog.showModal();
    }
}

function deep_equal(a, b) {
    if (a === b)
        return true;
    if (a === null || b === null)
        return false;
    const a_type = typeof a, b_type = typeof b;
    if (a_type !== b_type)
        return false;
    // Arrays  
    if (Array.isArray(a) || Array.isArray(b)) {
        if (!Array.isArray(a) || !Array.isArray(b))
            return false;
        if (a.length !== b.length)
            return false;
        for (let i = 0; i < a.length; i++) {
            if (!deep_equal(a[i], b[i]))
                return false;
        }
        return true;
    }
    // Plain objects  
    if (a_type === "object") {
        const a_keys = Object.keys(a);
        const b_keys = Object.keys(b);
        if (a_keys.length !== b_keys.length)
            return false;
        for (const k of a_keys) {
            if (!Object.prototype.hasOwnProperty.call(b, k))
                return false;
            if (!deep_equal(a[k], b[k]))
                return false;
        }
        return true;
    }
    return false;
}

async function delete_from_server(type, orig, update) {
    let collection = type + "s";
    if (orig[collection])
        for (let id of Object.keys(orig[collection])) {
            if (!update[collection][id])
                await ov_DB.remove(type, id);
        }
}

async function update_on_server(type, orig, update, scope) {
    let collection = type + "s";
    if (update[collection])
        for (let id of Object.keys(update[collection])) {
            let password;
            if (!(orig[collection] && orig[collection][id])) {
                await ov_DB.create(type, id, scope, orig.id);
                if (update[collection][id].password) {
                    password = update[collection][id].password;
                    delete update[collection][id].password;
                }
                await ov_DB.update(type, update[collection][id]);
            } else if (!deep_equal(orig[collection][id], update[collection][id]))
                await ov_DB.update(type, update[collection][id]);
            if (password)
                await ov_DB.update_password(id, password);
        }
}

export function render_user(user) {
    DOM.menu_slider.value = user.name;
}

export function render(domain_data, page) {
    domain = domain_data;
    let user = ov_Websockets.user();
    let domain_name = domain.name ? domain.name : user.domain;
    if (domain_name)
        DOM.config_domain_name.innerText = domain_name;

    Settings.render(domain, user.project);

    if (page)
        DOM.sub_view_nav.value = page;
    else
        DOM.sub_view_nav.value = "settings";
}

export function offline_mode(value) {
    view_container.classList.toggle("offline", value);
    DOM.save_button.disabled = value;
    Settings.offline_mode(value);
}

export function display_loading_screen(value, message) {
    if (value)
        DOM.loading_screen.show(message);
    else
        DOM.loading_screen.hide();
}