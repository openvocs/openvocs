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

    @ingroup        extensions/sip

    @brief          manage DOM objects for sip view
    	
    ---------------------------------------------------------------------------
*/
// import custom HTML elements
import ov_SIP_Loop from "/extensions/sip/components/config/loop/sip_loop.js";
import ov_SIP_Whitelist from "/extensions/sip/components/config/whitelist_entry/whitelist.js";
import ov_SIP_Role from "/extensions/sip/components/config/role/sip_role.js";

var DOM = {};
export var current_loop;

export function init(view_id) {

    DOM.loops = document.getElementById("sip_loops");
    DOM.whitelist = document.getElementById("whitelist_entries");
    DOM.roles = document.getElementById("sip_roles");
    DOM.add_whitelist = document.getElementById("add_whitelist_entry");

    DOM.add_whitelist.addEventListener("click", () => {
        let element = document.createElement("ov-sip-whitelist");
        DOM.whitelist.appendChild(element);
        let index = current_loop.add_whitelist_entry("", "");
        element.addEventListener("delete_entry", () => {
            DOM.whitelist.removeChild(element);
            current_loop.delete_whitelist_entry(index);
            trigger_update(current_loop);
        });
        element.addEventListener("change", () => {
            current_loop.update_whitelist_entry(index, element.caller, element.callee);
            trigger_update(current_loop);
        });
    });
}

export function add_loop(id, data, roles_data) {
    let loop = document.createElement("ov-sip-config-loop");

    loop.id = id;
    loop.name = data.name ? data.name : data.id;

    if (data.sip)
        for (let entry of data.sip.whitelist)
            loop.add_whitelist_entry(entry.caller, entry.callee);

    for (let role_id of Object.keys(data.roles)) {
        if (roles_data[role_id]) {
            let name = roles_data[role_id].name ? roles_data[role_id].name : roles_data[role_id].id;
            loop.add_role(role_id, data.sip && data.sip.roles ? data.sip.roles[role_id] : undefined, name);
        } else
            loop.add_role(role_id, data.sip && data.sip.roles ? data.sip.roles[role_id] : undefined, role_id, true);
    }

    loop.addEventListener("click", () => {
        this.select_loop(loop);
    });
    DOM.loops.appendChild(loop);

    return loop;
}

export function clear_loops() {
    DOM.loops.replaceChildren();
    current_loop = undefined;
}

export function select_loop(loop) {
    if (current_loop)
        current_loop.selected = false;
    current_loop = loop;

    loop.selected = true;
    DOM.add_whitelist.disabled = loop.disabled;

    DOM.whitelist.replaceChildren();
    for (const [index, entry] of loop.whitelist.entries()) {
        let element = document.createElement("ov-sip-whitelist");
        DOM.whitelist.appendChild(element);

        element.callee = entry.callee;
        element.caller = entry.caller;
        if (loop.disabled)
            element.disabled = true;

        element.addEventListener("delete_entry", () => {
            DOM.whitelist.removeChild(element);
            loop.delete_whitelist_entry(index);
            trigger_update(loop);
        });
        element.addEventListener("change", (event) => {
            loop.update_whitelist_entry(index, element.caller, element.callee);
            trigger_update(loop);
        });
    }

    DOM.roles.replaceChildren();
    for (let role_id of Object.keys(loop.roles)) {
        let element = document.createElement("ov-sip-config-role");
        DOM.roles.appendChild(element);

        element.id = role_id;
        element.name = loop.roles[role_id].name;
        if (loop.roles[role_id].hidden)
            element.hidden = true;
        else if (loop.disabled)
            element.disabled = true;

        let value = "none";
        if (loop.roles[role_id].value === true)
            value = "callout";
        else if (loop.roles[role_id].value === false)
            value = "hangup";
        element.value = value;

        element.addEventListener("change", (event) => {
            let value = element.value === "none" ? undefined : element.value === "callout";
            loop.add_role(element.id, value, element.name, element.hidden);
            trigger_update(loop);
        });
    }
}

function collect_loop(loop) {
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
    return { whitelist: whitelist, roles: roles };
}

function trigger_update(loop) {
    let node = {
        node_id: loop.id,
        type: "loop",
        data: {
            sip: collect_loop(loop)
        }
    }
    DOM.loops.dispatchEvent(new CustomEvent("save_node", {
        detail: { node: node, update: true }, bubbles: true, composed: true
    }));
}