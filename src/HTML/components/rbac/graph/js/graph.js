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
    @file           graph.js

    @author         Anja Bertard

    @date           2023-09-27

    @ingroup        components/rbac/graph

    @brief          create and handle edges

    ---------------------------------------------------------------------------
*/

// import custom HTML elements
import ov_RBAC_Node from "/components/rbac/node/rbac_node.js";

export { group_nodes } from "./node_groups.js";

export var edges = new Map();
export var nodes = {
    users: new Map(),
    roles: new Map(),
    loops: new Map()
};

export function clear() {
    nodes.users.clear();
    nodes.roles.clear();
    nodes.loops.clear();
    edges.clear();
}

export function create_node(type, data, subset_id) {
    let element = document.createElement("ov-rbac-node");
    element.type = type;
    element.generate_id();
    element.linked_nodes = new Map();
    if (subset_id)
        element.subset = subset_id;
    if (data) {
        if (type === "role" && data.id === "admin")
            element.node_id = data.id + "@" + subset_id;
        else
            element.node_id = data.id;
        element.node_name = data.name;
        element.node_abbreviation = data.abbreviation;
        if (type === "loop" && data.roles) {
            for (let role_id of Object.keys(data.roles))
                element.linked_nodes.set(role_id, data.roles[role_id]);
            if (data.multicast) {
                element.node_multicast_ip = data.multicast.host;
                element.node_multicast_port = data.multicast.port;
            }
        } else if (type === "role" && data.users) {
            for (let user_id of Object.keys(data.users))
                element.linked_nodes.set(user_id, data.users[user_id]);
        }
        register_node(element);
    }
    element.onmouseenter = function () {
        highlight_node(element, "highlight", true);
    }
    element.onmouseleave = function () {
        highlight_node(element, "highlight", false);
    };
    return element;
}

export function register_node(node) {
    if (node.type === "loop")
        nodes.loops.set(node.node_id, node);
    else if (node.type === "role")
        nodes.roles.set(node.node_id, node);
    else
        nodes.users.set(node.node_id, node);
}

export function delete_node(node) {
    if (node.type === "role") {
        for (let [linked_node, permission] of node.linked_nodes.entries()) {
            if (permission === null) //user
                nodes.users.get(linked_node.node_id).linked_nodes.delete(node);
            else
                nodes.loops.get(linked_node.node_id).linked_nodes.delete(node);
        }
        nodes.roles.delete(node.node_id);
    } else {
        for (let linked_node of node.linked_nodes.keys())
            nodes.roles.get(linked_node.node_id).linked_nodes.delete(node);
        if (node.type === "user")
            nodes.users.delete(node.node_id);
        else
            nodes.loops.delete(node.node_id);
    }
    node.remove();
}

export function fill_out_links() {
    for (let role of nodes.roles.values()) {
        let linked_nodes = new Map(role.linked_nodes);
        role.linked_nodes = new Map();
        for (let [user_id, permission] of linked_nodes.entries()) {
            let user = user_id;
            if (typeof user_id === "string")
                user = nodes.users.get(user_id);
            if (user) {
                if (!user.linked_nodes)
                    user.linked_nodes = new Map();
                user.linked_nodes.set(role, permission);
                role.linked_nodes.set(user, permission);
            } else {
                if (!role.passive_linked_entries)
                    role.passive_linked_entries = {};
                role.passive_linked_entries[user_id] = permission;
            }
        }
    }

    for (let loop of nodes.loops.values()) {
        let linked_nodes = new Map(loop.linked_nodes);
        loop.linked_nodes = new Map();
        for (let [role_id, permission] of linked_nodes.entries()) {
            let role = role_id;
            if (typeof role_id === "string")
                role = nodes.roles.get(role_id);
            if (role) {
                if (!role.linked_nodes)
                    role.linked_nodes = new Map();
                role.linked_nodes.set(loop, permission);
                loop.linked_nodes.set(role, permission);
            } else {
                if (!loop.passive_linked_entries)
                    loop.passive_linked_entries = {};
                loop.passive_linked_entries[role_id] = permission;
            }
        }
    }
}

export function add_link(source_node, target_node, value) {
    source_node.linked_nodes.set(target_node, value);
    target_node.linked_nodes.set(source_node, value);
}

export function delete_link(source_node, target_node) {
    source_node.linked_nodes.delete(target_node);
    target_node.linked_nodes.delete(source_node);
}

export function create_edge(source_node, target_node, layer_offset_top, layer_width) {
    let path = document.createElementNS("http://www.w3.org/2000/svg", "path");
    path.id = source_node.node_id + ";" + target_node.node_id;
    path.setAttribute("class", "link");
    path.setAttribute("d", draw_line_curve(source_node, target_node, layer_offset_top, layer_width));
    path.onmouseenter = function () {
        highlight_link(path, "highlight", true);
    }
    path.onmouseleave = function () {
        highlight_link(path, "highlight", false);
    };
    edges.set(path.id, path);
    return path;
}

function draw_line_curve(source, target, layer_offset_top, layer_width) {
    let curvature = 0.5;

    let source_group_modifier = 1;
    let target_group_modifier = 1;

    if (source.parentNode && source.parentNode.classList.contains("node_group")) {
        source = source.parentNode.firstElementChild;
        source_group_modifier = source.parentNode.childElementCount;
    }
    if (target.parentNode && target.parentNode.classList.contains("node_group")) {
        target = target.parentNode.firstElementChild;
        target_group_modifier = target.parentNode.childElementCount;
    }

    let ySource = source.offsetTop + (source_group_modifier * source.offsetHeight * 0.5) - layer_offset_top;
    let yTarget = target.offsetTop + (target_group_modifier * target.offsetHeight * 0.5) - layer_offset_top;

    let x0 = layer_width,
        x1 = 0,
        x2 = (x0 - x1) * curvature,
        y0 = ySource,
        y1 = yTarget;
    return "M " + x0 + "," + y0 + " C " + x2 + "," + y0 + " " + x2 + "," + y1 + " " + x1 + "," + y1;
}

export function highlight_node(node, class_name, value, filter) {
    node.classList.toggle(class_name, value);
    for (let [linked_node, permission] of node.linked_nodes.entries()) {
        let link;
        if (node.type === "user") {
            link = edges.get(linked_node.node_id + ";" + node.node_id);
            highlight_node(linked_node, class_name, value, "loop");
        } else if (node.type === "loop") {
            link = edges.get(node.node_id + ";" + linked_node.node_id);
            highlight_node(linked_node, class_name, value, "user");
        } else if ((!filter || filter === "user") && linked_node.type === "user") {
            link = edges.get(node.node_id + ";" + linked_node.node_id);
        } else if ((!filter || filter === "loop") && linked_node.type === "loop") {
            link = edges.get(linked_node.node_id + ";" + node.node_id);
        }
        if (link) {
            linked_node.classList.toggle(class_name, value);
            if ((node.type === "loop" || node.type === "role") && permission === true)
                linked_node.classList.toggle(class_name + "_talk", value);
            else if ((node.type === "loop" || node.type === "role") && permission === false)
                linked_node.classList.toggle(class_name + "_monitor", value);
            link.classList.toggle(class_name, value);
            if (value === true)
                link.parentNode.appendChild(link);
        }
    }
}

export function highlight_link(link, class_name, value) {
    link.classList.toggle(class_name, value);
    if (value === true)
        link.parentNode.appendChild(link);

    let node_array = link.id.split(";");
    if (nodes.roles.has(node_array[0]) && nodes.users.has(node_array[1])) {
        _highlight_node(nodes.roles.get(node_array[0]));
        _highlight_node(nodes.users.get(node_array[1]));
    } else {
        let loop = nodes.loops.get(node_array[0]);
        let role = nodes.roles.get(node_array[1]);
        let permission = loop.linked_nodes.get(role);
        _highlight_node(loop, permission);
        _highlight_node(role, permission);
    }

    function _highlight_node(node, permission) {
        if (node.parentNode.classList.contains("node_group")) {
            for (let sibling_node of node.parentNode.children) {
                sibling_node.classList.toggle(class_name, value);
                if (permission === true)
                    sibling_node.classList.toggle(class_name + "_talk", value);
                else if (permission === false)
                    sibling_node.classList.toggle(class_name + "_monitor", value);
            }
        } else {
            node.classList.toggle(class_name, value);
            if (permission === true)
                node.classList.toggle(class_name + "_talk", value);
            else if (permission === false)
                node.classList.toggle(class_name + "_monitor", value);
        }
    }
}
