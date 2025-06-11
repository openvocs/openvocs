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
    @file           node_groups.js

    @ingroup        components/rbac/graph

    @brief          group nodes with same edges

    ---------------------------------------------------------------------------
*/

import create_uuid from "/lib/ov_utils/ov_uuid.js";

export function group_nodes(root, nodes){
    clear_node_groups(root);
    find_and_create_groups(nodes.users);
    find_and_create_groups(nodes.loops);
}

function clear_node_groups(root) {
    for (let node_group of root.querySelectorAll(".node_group")) {
        node_group.replaceWith(...node_group.childNodes)
    }
}

function find_and_create_groups(nodes) {
    for (let node2 of nodes.values()) {
        for (let node1 of nodes.values()) {
            if (node1 === node2)
                break;
            if (compare_linked_nodes(node1.linked_nodes, node2.linked_nodes)) {
                let wrapper;
                if (!node1.parentElement.classList.contains("node_group")) {
                    wrapper = document.createElement("div");
                    wrapper.id = "group_" + create_uuid();
                    wrapper.className = "node_group";
                    node1.parentNode.insertBefore(wrapper, node1);
                    wrapper.appendChild(node1);
                } else
                    wrapper = node1.parentNode;
                wrapper.appendChild(node2);
                break;
            }
        }
    }
}

function compare_linked_nodes(linked_nodes_1, linked_nodes_2) {
    if (!linked_nodes_1 || !linked_nodes_2)
        return false;
    if (linked_nodes_1.size <= 0 || linked_nodes_1.size !== linked_nodes_2.size)
        return false;
    for (let [key, value] of linked_nodes_1.entries()) {
        if (!linked_nodes_2.has(key))
            return false;
        if (value !== linked_nodes_2.get(key))
            return false;
    }
    return true;
}