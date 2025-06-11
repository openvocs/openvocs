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

    @ingroup        vocs_admin/views/project_settings

    @brief          manage DOM objects for admin project settings view
    	
    ---------------------------------------------------------------------------
*/
const DOM = {};

var VIEW_ID;

export function init(view_id) {
    VIEW_ID = view_id;
    DOM.id = document.getElementById("edit_id");
    DOM.name = document.getElementById("edit_name");
    DOM.domain = document.getElementById("edit_domain");
    DOM.delete = document.getElementById("delete_project");
    DOM.delete_button = document.getElementById("delete_button");

    DOM.delete_button.addEventListener("click", () => {
        if (window.confirm("Do you really want to delete this project?"))
            DOM.delete.dispatchEvent(new CustomEvent("delete_project", { bubbles: true }));
    });

    DOM.name.addEventListener("change", changed_name);

    DOM.id.addEventListener("change", changed_name);
}

function changed_name() {
    if (DOM.name.value !== "")
        DOM.name.dispatchEvent(new CustomEvent("changed_name", { detail: DOM.name.value, bubbles: true }));
    else {
        if (DOM.id.value !== "")
            DOM.id.dispatchEvent(new CustomEvent("changed_name", { detail: DOM.id.value, bubbles: true }));
        else
            DOM.id.dispatchEvent(new CustomEvent("changed_name", { detail: "[Project]", bubbles: true }));
    }
}

export function render(project, id, domain) {
    id = id ? id : project.id;
    domain = domain ? domain : project.domain;
    if (id)
        DOM.id.value = id;
    if (project.name)
        DOM.name.value = project.name;
    DOM.domain.value = domain;

    if (DOM.id.value)
        DOM.id.disabled = true;
    else
        DOM.delete_button.disabled = true;
}

export function collect() {
    let data = {};
    data.id = DOM.id.value;
    data.name = DOM.name.value ? DOM.name.value : null;
    data.domain = DOM.domain.value;
    return data;
}

export function offline_mode(value) {
    value = !value && !DOM.id.disabled ? true : value;
    DOM.delete_button.disabled = value;
}