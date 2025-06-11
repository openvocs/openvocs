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
    @file           settings.js

    @author         Anja Bertard

    @date           2023-09-06

    @ingroup        vocs_admin/views/domain_settings

    @brief          init and load admin domain settings view
    	
    ---------------------------------------------------------------------------
*/
import * as View from "./view.js";

export const VIEW_ID = "vocs_admin_domain_settings";
var view_container;

export async function init(container) {
    view_container = container;
    view_container.replaceChildren(await loadCSS(), await loadHtml());
    View.init(VIEW_ID);
}

export function render(domain, id) {
    View.render(domain, id);
    console.log("(domain settings) View rendered");
}

export { collect, offline_mode } from "./view.js";

export function remove() {
    console.log("(domain settings) unload");
    view_container.replaceChildren();
}

window.onbeforeunload = function () {
    remove();
}

async function loadHtml() {
    const response = await fetch('/app/vocs_admin/views/domain_settings/settings.html');
    const dom = new DOMParser().parseFromString(await response.text(), 'text/html');
    return dom.querySelector('.view');
}

async function loadCSS() {
    const response = await fetch('/app/vocs_admin/views/domain_settings/settings.css');
    const style = document.createElement('style');
    style.textContent = await response.text();
    return style;
}