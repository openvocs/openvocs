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

    @ingroup        vocs_admin/views/overview

    @brief          manage DOM objects for overview view
    	
    ---------------------------------------------------------------------------
*/
//import * as ov_Auth from "/lib/ov_auth.js";
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import ov_Domain from "/lib/ov_object_model/ov_domain_model.js";
import ov_Domain_Map from "/lib/ov_data_structure/ov_domain_map.js";
import ov_Project from "/lib/ov_object_model/ov_project_model.js";
import ov_Project_Map from "/lib/ov_data_structure/ov_project_map.js";
import * as ov_Auth from "/lib/ov_auth.js";

// import custom HTML elements
import ov_Nav from "/components/nav/nav.js";
import ov_Dialog from "/components/dialog/dialog.js";

const DOM = {
    CLASS: {
        active: "active",
        loading: "loading"
    }
};

var VIEW_ID;

export var logout_triggered;

export async function init(view_id) {
    document.documentElement.className = "light";

    if (!document.adoptedStyleSheets.includes(await ov_Nav.style_sheet))
        document.adoptedStyleSheets = [...document.adoptedStyleSheets, await ov_Nav.style_sheet];

    DOM.view_container = document.getElementById("view_container");
    DOM.domains = document.getElementById("domains");
    DOM.add_domain_button = document.getElementById("add_domain");
    DOM.menu_slider = document.getElementById("menu_slider");
    DOM.menu_button = document.getElementById("menu_button");
    DOM.logout_button = document.getElementById("logout_button");
    DOM.loading_screen = document.getElementById("loading_screen");

    DOM.TEMPLATE = {
        domain: document.getElementById("domain_template")
    };

    VIEW_ID = view_id;

    DOM.add_domain_button.addEventListener("click", () => {
        DOM.view_container.dispatchEvent(new CustomEvent("switch_view", {
            detail: { origin: VIEW_ID, target: "domain", domain: undefined }
        }));
    });

    DOM.menu_button.addEventListener("click", () => {
        DOM.menu_slider.toggle();
    });

    DOM.logout_button.addEventListener("click", () => {
        logout_triggered = true;
        ov_Auth.logout();
    });
}

export function draw(user) {
    DOM.menu_slider.value = user.name;
    let domains = extract_all_domains(user.projects, user.domains);
    domains.sort();
    for (let domain of domains.values()) {
        let domain_element = add_domain(domain.dom_id, domain.name, domain.id);
        if (user.domains.has(domain.id))
            domain_element.classList.add("domain_admin");
        if (domain.projects.size !== 0) {
            domain.projects.sort();
            let projects_element = domain_element.querySelector(".projects");
            for (let project of domain.projects.values()) {
                let name = project.name ? project.name : project.id;
                projects_element.add_item(project.dom_id, name, project.id);
            }
            projects_element.addEventListener("change", (event) => {
                DOM.view_container.dispatchEvent(new CustomEvent("switch_view", {
                    detail: { origin: VIEW_ID, target: "project", project: event.target.value, "domain": domain.id }
                }));
            });
        }

        domain_element.querySelector(".add_project").addEventListener("click", () => {
            DOM.view_container.dispatchEvent(new CustomEvent("switch_view", {
                detail: { origin: VIEW_ID, target: "project", project: undefined, "domain": domain.id }
            }));
        });
    }
}

export function display_loading_screen(value, message) {
    if (value)
        DOM.loading_screen.show(message);
    else
        DOM.loading_screen.hide();
}

function extract_all_domains(admin_projects, admin_domains) {
    let domains = new ov_Domain_Map();
    for (let domain of admin_domains.values())
        domains.set(domain);
    for (let project of admin_projects.values()) {
        if (!domains.has(project.domain))
            domains.set(new ov_Domain(project.domain));
        domains.get(project.domain).projects.set(project);
    }
    return domains;
}

function add_domain(id, name, value) {
    let domain_item = DOM.TEMPLATE.domain.content.firstElementChild.cloneNode(true);

    domain_item.id = id;
    domain_item.querySelector(".domain_name").innerText = name;
    domain_item.querySelector(".domain_settings_button").addEventListener("click", () => {
        ov_Websockets.user().domain = value;
        DOM.view_container.dispatchEvent(new CustomEvent("switch_view", {
            detail: { origin: VIEW_ID, target: "domain", domain: value }
        }));
    });
    DOM.domains.appendChild(domain_item);

    return domain_item;
}