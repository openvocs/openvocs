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

// import custom HTML elements
import ov_Nav from "/components/nav/nav.js";
import ov_Dialog from "/components/dialog/dialog.js";

import * as CSS from "/css/css.js";

var Config_SIP;
var ov_SIP;
var Config_Recorder;

var SIP_ONLINE;

var LDAP;

const DOM = {
};

var VIEW_ID;
var view_container;

var orig_domain;
var domain;

export async function init(view_id, container) {
    SIP_ONLINE = await ov_DB.check_sip(ov_Websockets.current_lead_websocket);
    if (SIP) {
        Config_SIP = await import("/extensions/sip/views/config/js/sip_config.js");
        ov_SIP = await import("/extensions/sip/ov_sip.js")
    }
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

    DOM.no_admin = document.getElementById("no_admin");

    DOM.sub_view_nav = document.getElementById("select_subview");
    DOM.sub_view = document.getElementById("config_administration");
    DOM.config_name = document.getElementById("config_name");
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
    } else if (!SIP_ONLINE) {
        document.getElementById("sip_page_button").disabled = true;
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

    LDAP = await ov_DB.check_ldap();
    await Settings.init(document.getElementById("settings_page"), LDAP);
    await Config_RBAC.init(document.getElementById("rbac_page"), LDAP);
    await Config_Layout.init(document.getElementById("layout_page"));
    if (SIP)
        await Config_SIP.init(document.getElementById("sip_page"));
    if (RECORDER)
        await Config_Recorder.init(document.getElementById("recorder_page"));

    DOM.save_button.addEventListener("click", async () => {
        await save();
    });

    let user = ov_Websockets.user();

    DOM.sub_view.addEventListener("change_domain", async (event) => {
        let domain_config = await ov_DB.get_config('domain', event.detail);
        render(domain_config, DOM.sub_view_nav.value);
    });

    DOM.sub_view.addEventListener("new_project", (event) => {
        domain.projects[event.detail] = {};
    });

    DOM.sub_view.addEventListener("changed_project", (event) => {
        user.project = event.detail;
        for (let ws of ov_Websockets.list)
            ov_Web_Storage.add_anchor_to_session(APP, ws.websocket_url, user.domain, user.project, DOM.sub_view_nav.value);
        let project = domain.projects[event.detail];
        if (project)
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
        if (project) {
            delete domain.projects[event.detail.old_id];
            project.id = event.detail.new_id;
            domain.projects[event.detail.new_id] = project;
            user.project = project.id;
            for (let ws of ov_Websockets.list)
                ov_Web_Storage.add_anchor_to_session(APP, ws.websocket_url, user.domain, user.project, DOM.sub_view_nav.value);
            update_project_name_display(project.name, project.id);
            domain.layout[project.id] = { grid_columns: 6, grid_rows: 5 }
        }
    });

    DOM.sub_view.addEventListener("changed_project_name", (event) => {
        let project = domain.projects[event.detail.id];
        if (project) {
            project.name = event.detail.name;
            update_project_name_display(event.detail.name, event.detail.id);
        }
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

    DOM.sub_view_nav.addEventListener("change", async () => {
        for (let ws of ov_Websockets.list)
            ov_Web_Storage.add_anchor_to_session(APP, ws.websocket_url, user.domain, user.project, DOM.sub_view_nav.value);
        DOM.sub_view.className = DOM.sub_view_nav.value;
        if (DOM.sub_view_nav.value === "rbac")
            Config_RBAC.render(domain, user.project, !user.domains.has(user.domain));
        else if (DOM.sub_view_nav.value === "layout")
            Config_Layout.render(domain, user.project, !user.domains.has(user.domain));
        else if (DOM.sub_view_nav.value === "sip" && SIP && SIP_ONLINE)
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
    let result;
    let user = ov_Websockets.user();
    if (domain.id) {
        if (user.admin === "domain") {
            DOM.loading_screen.show("Saving domain " + domain.id + " on server(s)...");
            if (domain.name !== orig_domain.name) {
                result = await ov_DB.update_key("domain", domain.id, "name", domain.name);
                if (!result.updated)
                    errors.push("Failed to update domain name: " + result.error.description);
            }
            if (!deep_equal(orig_domain.layout, domain.layout)) {
                result = await ov_DB.update_key("domain", domain.id, "layout", domain.layout);
                if (!result.updated)
                    errors.push("Failed to update layouts: " + result.error.description);
            }
            await delete_from_server("user", orig_domain, domain, errors);
            await delete_from_server("role", orig_domain, domain, errors);
            await delete_from_server("loop", orig_domain, domain, errors)
        }

        if (domain.projects)
            for (let project_id of Object.keys(domain.projects)) {
                if (user.admin === "domain" || user.projects.has(project_id)) {
                    let project = domain.projects[project_id];
                    if (!project) {
                        DOM.loading_screen.show("Delete project " + project.id + " on server(s)...");
                        result = await ov_DB.remove("project", project_id);
                        if (!result.updated)
                            errors.push("Failed to delete project " + id + ": " + result.error.description);
                        continue;
                    } else if (project.id) {
                        DOM.loading_screen.show("Saving project " + project.id + " on server(s)...");
                        let orig_project = orig_domain.projects[project.id];
                        if (!orig_project) {
                            result = await ov_DB.create("project", project.id, "domain", domain.id);
                            if (!result.updated) {
                                errors.push("Failed to create project " + project.id + ": " + result.error.description);
                                continue;
                            }
                            result = await ov_DB.update("project", project);
                            if (!result.updated) {
                                errors.push("Failed to update project " + project.id + ": " + result.error.description);
                                continue;
                            }
                            if (project.users)
                                for (let id of Object.keys(project.users)) {
                                    if (project.user[id].password) {
                                        result = await ov_DB.update_password(id, project.user[id].password);
                                        if (!result.updated)
                                            errors.push("Failed to update password of " + id + ": " + result.error.description);
                                    }
                                }
                        } else {
                            if (project.name !== orig_project.name) {
                                result = await ov_DB.update_key("project", project.id, "name", project.name);
                                if (!result.updated)
                                    errors.push("Failed to update project " + project.id + " name: " + result.error.description);
                            }
                            await delete_from_server("user", orig_project, project, errors);
                            await delete_from_server("role", orig_project, project, errors);
                            await delete_from_server("loop", orig_project, project, errors);
                            await update_on_server("user", orig_project, project, "project", errors);
                            await update_on_server("role", orig_project, project, "project", errors);
                            await update_on_server("loop", orig_project, project, "project", errors);
                        }
                    } else
                        errors.push("Project ID is missing\n\n");
                }
            }

        if (user.admin === "domain") {
            DOM.loading_screen.show("Saving domain " + domain.id + " on server(s)...");
            await update_on_server("user", orig_domain, domain, "domain", errors);
            await update_on_server("role", orig_domain, domain, "domain", errors);
            await update_on_server("loop", orig_domain, domain, "domain", errors);
        }
    } else
        errors.push("Domain ID is missing\n\n");

    await ov_DB.persist();

    DOM.loading_screen.delayed_hide();

    if (errors.length > 0) {
        DOM.error_dialog_title.innerText = "Saving failed:";
        DOM.error_report.innerHTML = "";
        for (let error of errors) {
            console.error(error);
            DOM.error_report.innerHTML += error + "<br/>"
        }
        DOM.error_dialog.showModal();
    } else
        render(await ov_DB.get_config("domain", user.domain), DOM.sub_view_nav.value);
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

function contains_whitelist_rule(rule_set, rule) {
    for (let set_rule of rule_set)
        if (rule.caller === set_rule.caller && rule.callee === set_rule.callee)
            return true;
    return false;
}

async function permit_whitelist_rule(id, caller, callee, errors) {
    let result = await ov_SIP.sip_permit(id, caller, callee);
    if (!result.updated)
        errors.push("Failed to permit sip rule for loop " + id + " with caller " + caller +
            " and callee " + callee + ": " + result.error.description);
    return result;
}

async function permit_whitelist_rule_set(loop_id, rule_set, errors) {
    let sip = Object.create(rule_set);
    let sip_error = false;
    for (let [index, rule] of rule_set.whitelist.entries()) {
        let result = await permit_whitelist_rule(loop_id, rule.caller, rule.callee, errors);
        if (!result) {
            sip.whitelist.splice(index, 1);
            sip_error = true;
        }
    }
    if (sip_error)
        await ov_DB.update_key("loop", loop_id, "sip", sip);
}

async function revoke_whitelist_rule(id, caller, callee, errors) {
    let result = await ov_SIP.sip_revoke(id, caller, callee);
    if (!result.updated)
        errors.push("Failed to revoke sip rule for loop " + id + " with caller " + caller +
            " and callee " + callee + ": " + result.error.description);
}

async function delete_from_server(type, orig, update, errors) {
    let collection = type + "s";
    if (orig[collection])
        for (let id of Object.keys(orig[collection])) {
            if (!update[collection][id]) {
                if (type === "loop" && SIP_ONLINE && orig[collection][id].sip) //todo prevent deletion in config if SIP is offline
                    for (let rule of orig[collection][id].sip.whitelist)
                        await revoke_whitelist_rule(id, rule.caller, rule.callee, errors);
                let result = await ov_DB.remove(type, id);
                if (!result.updated)
                    errors.push("Failed to delete " + type + " " + id + ": " + result.error.description);
            }
        }
}

async function update_on_server(type, orig, update, scope, errors) {
    let collection = type + "s";
    let result;
    if (update[collection])
        for (let id of Object.keys(update[collection])) {
            let password;
            if (!(orig[collection] && orig[collection][id])) {
                result = await ov_DB.create(type, id, scope, orig.id);
                if (!result.updated) {
                    errors.push("Failed to create " + type + " " + id + ": " + result.error.description);
                    continue;
                }
                if (update[collection][id].password) {
                    password = update[collection][id].password;
                    delete update[collection][id].password;
                }
                result = await ov_DB.update(type, update[collection][id]);
                if (!result.updated) {
                    errors.push("Failed to update " + type + " " + id + ": " + result.error.description);
                    continue;
                }
                if (type === "loop" && SIP_ONLINE && update[collection][id].sip && update[collection][id].sip.whitelist)
                    await permit_whitelist_rule_set(id, update[collection][id].sip, errors);
            } else if (!deep_equal(orig[collection][id], update[collection][id])) {
                result = await ov_DB.update(type, update[collection][id]);
                if (!result.updated) {
                    errors.push("Failed to update " + type + " " + id + ": " + result.error.description);
                    continue;
                }
                if (type === "loop" && SIP_ONLINE) {
                    if (!orig[collection][id].sip) {
                        if (update[collection][id].sip && update[collection][id].sip.whitelist)
                            await permit_whitelist_rule_set(id, update[collection][id].sip, errors);
                    } else if (!update[collection][id].sip) {
                        for (let orig_rule of orig[collection][id].sip.whitelist)
                            await revoke_whitelist_rule(id, orig_rule.caller, orig_rule.callee, errors);
                    } else if (!deep_equal(orig[collection][id].sip.whitelist, update[collection][id].sip.whitelist)) {
                        for (let orig_rule of orig[collection][id].sip.whitelist)
                            if (!contains_whitelist_rule(update[collection][id].sip.whitelist, orig_rule))
                                await revoke_whitelist_rule(id, orig_rule.caller, orig_rule.callee, errors);
                        for (let new_rule of update[collection][id].sip.whitelist)
                            if (!contains_whitelist_rule(orig[collection][id].sip.whitelist, new_rule))
                                await permit_whitelist_rule(id, new_rule.caller, new_rule.callee, errors);
                    }
                }
            }
            if (password) {
                result = await ov_DB.update_password(id, password);
                if (!result.updated)
                    errors.push("Failed to update password of " + id + ": " + result.error.description);
            }

        }
}

export function no_admin(){
    DOM.no_admin.style.display = "flex";
    DOM.config_name.style.display = "none";
    DOM.save_button.style.display = "none";
    DOM.sub_view_nav.style.display = "none";
}

export function render_user(user) {
    DOM.menu_slider.value = user.name;
}

export function render(domain_data, page) {
    orig_domain = domain_data;
    if (orig_domain.users && orig_domain.roles && LDAP.roles && LDAP.users) {
        let used_users = new Set();
        if (orig_domain.roles)
            for (let role_id of Object.keys(orig_domain.roles))
                if (orig_domain.roles[role_id].users)
                    for (let user_id of Object.keys(orig_domain.roles[role_id].users))
                        used_users.add(user_id);
        if (orig_domain.projects)
            for (let proj_id of Object.keys(orig_domain.projects))
                if (orig_domain.projects[proj_id].roles)
                    for (let role_id of Object.keys(orig_domain.projects[proj_id].roles))
                        if (orig_domain.projects[proj_id].roles[role_id].users)
                            for (let user_id of Object.keys(orig_domain.projects[proj_id].roles[role_id].users))
                                used_users.add(user_id);
        for (let user_id of Object.keys(orig_domain.users))
            if (!used_users.has(user_id))
                delete orig_domain.users[user_id];
    }
    domain = structuredClone(orig_domain);
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