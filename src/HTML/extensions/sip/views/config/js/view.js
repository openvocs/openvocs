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

    @ingroup        vocs_admin/views/overview

    @brief          manage DOM objects for overview view
    	
    ---------------------------------------------------------------------------
*/
// import custom HTML elements
import ov_SIP_Loop from "/extensions/sip/components/config/loop/sip_loop.js";
import ov_SIP_Whitelist from "/extensions/sip/components/config/whitelist_entry/whitelist.js";
import ov_SIP_Role from "/extensions/sip/components/config/role/sip_role.js";

var DOM = {};

export var logout_triggered;

export function init(view_id) {

    DOM.loops = document.getElementById("sip_loops");
    DOM.whitelist = document.getElementById("whitelist_entries");
    DOM.roles = document.getElementById("sip_roles");
    DOM.add_whitelist = document.getElementById("add_whitelist_entry");

    DOM.add_whitelist.addEventListener("click", () => {
        let element = document.createElement("ov-sip-whitelist");
        DOM.whitelist.appendChild(element);
        element.addEventListener("delete_entry", () => {
            DOM.whitelist.removeChild(element);
        });
    });
}

export function collect() {
    let result = {};

    save_settings_of_current_loop();

    let loops = document.querySelectorAll("ov-sip-config-loop");
    for (let loop of loops) {

        let whitelist = loop.whitelist;
        for (let entry of whitelist) {
            if (entry.callee === undefined || entry.callee === "")
                delete entry.callee;
            if (entry.caller === undefined || entry.caller === "")
                delete entry.caller;
        }

        let roles = {};
        for (let role_id of Object.keys(loop.roles)) {
            if (loop.roles[role_id].value !== undefined)
                roles[role_id] = loop.roles[role_id].value;
        }
        result[loop.id] = { whitelist: whitelist, roles: roles };
    }
    return result;
}

export function add_loop(id, data, roles_data) {
    let loop = document.createElement("ov-sip-config-loop");

    loop.id = id;
    loop.name = data.name ? data.name : data.id;

    if (data.sip)
        for (let entry of data.sip.whitelist)
            loop.add_whitelist_entry(entry.caller, entry.callee);

    for (let role_id of Object.keys(data.roles)) {
        if (roles_data[role_id])
            loop.add_role(role_id, data.sip ? data.sip.roles[role_id] : undefined, roles_data[role_id].name);
        else
            loop.add_role(role_id, data.sip ? data.sip.roles[role_id] : undefined, role_id);
    }

    loop.addEventListener("click", () => {
        this.select_loop(loop);
    });
    DOM.loops.appendChild(loop);

    return loop;
}

export function clear_loops() {
    DOM.loops.replaceChildren();
}

function get_current_loop() {
    let loops = document.querySelectorAll("ov-sip-config-loop");
    for (let element of loops) {
        if (element.selected)
            return element;
    }
}

function save_settings_of_current_loop() {
    let loop = get_current_loop();
    if (loop) {
        loop.clear_whitelist();
        for (let whitelist of DOM.whitelist.children)
            if (whitelist.callee || whitelist.caller)
                loop.add_whitelist_entry(whitelist.caller, whitelist.callee);

        for (let role of DOM.roles.children) {
            if (role.value !== "none")
                loop.add_role(role.id, role.value === "callout", role.name);
            else
                loop.add_role(role.id, undefined, role.name);
        }
    }
    return loop;
}

export function select_loop(loop) {
    let prev_loop = save_settings_of_current_loop();
    if (prev_loop)
        prev_loop.selected = false;

    loop.selected = true;
    DOM.add_whitelist.disabled = loop.disabled;

    DOM.whitelist.replaceChildren();
    for (let entry of loop.whitelist) {
        let element = document.createElement("ov-sip-whitelist");
        DOM.whitelist.appendChild(element);

        element.callee = entry.callee;
        element.caller = entry.caller;
        if (loop.disabled)
            element.disabled = true;

        element.addEventListener("delete_entry", () => {
            DOM.whitelist.removeChild(element);
        });
    }

    DOM.roles.replaceChildren();

    for (let role_id of Object.keys(loop.roles)) {
        let element = document.createElement("ov-sip-config-role");
        DOM.roles.appendChild(element);

        element.id = role_id;
        element.name = loop.roles[role_id].name ? loop.roles[role_id].name : role_id;
        if (loop.disabled)
            element.disabled = true;

        let value = "none";
        if (loop.roles[role_id].value === true)
            value = "callout";
        else if (loop.roles[role_id].value === false)
            value = "hangup";
        element.value = value
    }
}