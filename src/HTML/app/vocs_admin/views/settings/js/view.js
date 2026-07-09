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

    @ingroup        vocs_admin/views/settings

    @brief          manage DOM objects for admin project settings view
    	
    ---------------------------------------------------------------------------
*/
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import ov_Domain_Map from "/lib/ov_data_structure/ov_domain_map.js";
import create_uuid from "/lib/ov_utils/ov_uuid.js";

const DOM = {};

var VIEW_ID;
var domains;

export function init(view_id, ldap_auth) {
    VIEW_ID = view_id;
    DOM.domain_list = document.getElementById("domains");
    DOM.domain_name = document.getElementById("edit_domain_name");

    DOM.ldap_button = document.getElementById("ldap_import_button");
    DOM.ldap_notice = document.getElementById("ldap_notice");
    DOM.error_report = document.getElementById("error_report");
    DOM.error_dialog = document.getElementById("error_dialog");
    DOM.error_dialog_title = DOM.error_dialog.querySelector("h3");

    DOM.project_id = document.getElementById("edit_project_id");
    DOM.project_name = document.getElementById("edit_project_name");
    // DOM.domain = document.getElementById("edit_domain");
    DOM.delete = document.getElementById("delete_project");
    DOM.delete_button = document.getElementById("delete_button");

    DOM.loading_screen = document.getElementById("loading_screen");
    DOM.reload_broadcast = document.getElementById("reload_broadcast");
    DOM.switch_server_id = document.getElementById("switch_server_id");
    DOM.switch_server_broadcast = document.getElementById("switch_server_broadcast");

    DOM.domain_settings = document.getElementById("domain_settings");

    DOM.project_list = document.querySelector("#project_list .projects");
    DOM.new_project = document.querySelector("#new_project")

    if (!ldap_auth.users) {
        DOM.ldap_button.style.display = "none";
        DOM.ldap_notice.style.display = "none";
    }

    // client broadcasts
    DOM.reload_broadcast.addEventListener("click", () => {
        DOM.loading_screen.show("Sending update broadcast...");
        for (let ws of ov_Websockets.list)
            ov_Monitor.broadcast_update(ws);
        DOM.loading_screen.delayed_hide();
    });
    DOM.switch_server_broadcast.disabled = true;
    DOM.switch_server_broadcast.addEventListener("click", () => {
        DOM.loading_screen.show("Sending switch server broadcast...");
        for (let ws of ov_Websockets.list)
            ov_Monitor.broadcast_switch_server(selected_server.server_url, ws);
        DOM.loading_screen.delayed_hide();
    });

    for (let server of ov_Websockets.list) {
        let option = document.createElement("option");
        option.value = server.server_url;
        option.innerText = server.server_name;

        DOM.switch_server_id.appendChild(option);
        setInterval(() => {
            option.classList.toggle("off", !server.connected);
        }, 500);
    }
    DOM.switch_server_id.addEventListener("change", () => {
        selected_server = ov_Websockets.get_websocket(DOM.switch_server_id.value);
        if (DOM.switch_server_broadcast.disabled === true) {
            DOM.switch_server_broadcast.disabled = false;
            setInterval(() => {
                DOM.switch_server_id.classList.toggle("off", !selected_server.connected);
            }, 500);
        }
    });

    //domain
    domains = extract_all_domains(ov_Websockets.user());
    domains.sort();
    DOM.domain_list.replaceChildren();
    for (let domain of domains.values()) {
        let domain_element = document.createElement("option");
        domain_element.value = domain.id;
        domain_element.label = domain.name;
        DOM.domain_list.appendChild(domain_element);
    }

    DOM.domain_list.addEventListener("change", (event) => {
        //todo switch domain
    });

    DOM.ldap_button.addEventListener("click", async () => {
        DOM.ldap_notice.innerText = "Importing...";
        let ldap_import = await ov_DB.user_ldap_import(DOM.ldap_host.value, DOM.ldap_base.value,
            DOM.id.value, DOM.ldap_user.value, DOM.ldap_password.value);
        if (ldap_import.error) {
            DOM.error_dialog_title.innerText = "LDAP import failed";
            DOM.error_report.innerText = ldap_import.error.description;
            DOM.error_dialog.showModal();
            DOM.ldap_notice.innerText = "";
        } else {
            DOM.ldap_notice.innerText = "Imported from LDAP";
            let domain_config = await ov_DB.get_config('domain', DOM.id.value);
            DOM.ldap_button.dispatchEvent(new CustomEvent("ui_update_domain_users", {
                detail: domain_config.users,
                bubbles: true
            }));
        }
        DOM.ldap_password.value = "";
    });

    DOM.project_list.addEventListener("change", (event) => {
        let project = domains.get(DOM.domain_list.value).projects.get(DOM.project_list.value);
        if (project && project.id) {
            DOM.project_id.value = project.id;
            DOM.project_id.disabled = true;
        } else {
            DOM.project_id.value = "";
            DOM.project_id.disabled = false;
        }
        if (project && project.name) {
            DOM.project_name.value = project.name;
        } else {
            DOM.project_name.value = "";
        }
        DOM.delete.open = false
        DOM.project_name.dispatchEvent(new CustomEvent("changed_project", {
            detail: DOM.project_list.value, bubbles: true
        }));
    });

    DOM.new_project.addEventListener("click", () => {
        new_project();
    });

    DOM.delete_button.addEventListener("click", () => {
        if (window.confirm("Do you really want to delete this project?")) {
            DOM.delete.dispatchEvent(new CustomEvent("delete_project", {
                detail: DOM.project_list.value, bubbles: true
            }));
            domains.get(DOM.domain_list.value).projects.delete(DOM.project_list.value);
            DOM.project_list.remove_item(DOM.project_list.value);
        }
    });

    DOM.project_name.addEventListener("change", () => {
        DOM.domain_name.dispatchEvent(new CustomEvent("changed_project_name", {
            detail: { id: DOM.project_list.value, name: DOM.project_name.value }, bubbles: true
        }));
        domains.get(DOM.domain_list.value).projects.get(DOM.project_list.value).name = DOM.project_name.value;
        let name = DOM.project_name.value ? DOM.project_name.value : DOM.project_id.value;
        DOM.project_list.update_item(DOM.project_list.value, name);
    });

    DOM.project_id.addEventListener("change", () => {
        DOM.domain_name.dispatchEvent(new CustomEvent("changed_project_id", {
            detail: { old_id: DOM.project_list.value, new_id: DOM.project_id.value }, bubbles: true
        }));
        let projects = domains.get(DOM.domain_list.value).projects;
        let project = projects.get(DOM.project_list.value);
        if (project)
            projects.delete(DOM.project_list.value);
        else
            project = {};
        project.id = DOM.project_id.value;
        projects.set(project);
        let name = DOM.project_name.value ? DOM.project_name.value : DOM.project_id.value;
        DOM.project_list.update_item(DOM.project_list.value, name, DOM.project_id.value);
    });

    DOM.domain_name.addEventListener("change", () => {
        DOM.domain_name.dispatchEvent(new CustomEvent("changed_domain_name", {
            detail: DOM.domain_name.value, bubbles: true
        }));
        domains.get(DOM.domain_list.value).name = DOM.domain_name.value;
    });
}

export async function render(domain_data, project_id) {
    DOM.domain_list.value = domain_data.id;

    if (domain_data.name)
        DOM.domain_name.value = domain_data.name;

    let domain = domains.get(domain_data.id);
    if (domain.projects.size !== 0) {
        domain.projects.sort();
        for (let project of domain.projects.values()) {
            let name = project.name ? project.name : project.id;
            await DOM.project_list.add_item(project.dom_id, name, project.id, project_id === project.id);
        }
    }

    if (!DOM.project_list.value || !project_id) {
        new_project();
    }
}

function new_project() {
    let id = "project_" + create_uuid();
    DOM.project_list.add_item(id, "[New Project]", id, true);
    DOM.project_list.dispatchEvent(new CustomEvent("new_project", {
        detail: id, bubbles: true
    }));
}

export function offline_mode(value) {
    value = !value && !DOM.id.disabled ? true : value;
    DOM.delete_button.disabled = value;
}

function extract_all_domains(user) {
    let domains = new ov_Domain_Map();
    for (let domain of user.domains.values())
        domains.set(domain);
    for (let project of user.projects.values()) {
        if (!domains.has(project.domain))
            domains.set(new ov_Domain(project.domain));
        domains.get(project.domain).projects.set(project);
    }
    return domains;
}