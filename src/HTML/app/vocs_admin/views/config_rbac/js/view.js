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

    @ingroup        vocs_admin/views/config_rbac

    @brief          manage DOM objects for admin project rbac view
    	
    ---------------------------------------------------------------------------
*/
import create_uuid from "/lib/ov_utils/ov_uuid.js";

// import custom HTML elements
import ov_RBAC_Graph from "/components/rbac/graph/rbac_graph.js";

const DOM = {};

var VIEW_ID;

var domain_id;

export function init(view_id, ldap_auth) {
    DOM.graph = document.querySelector("ov-rbac-graph");
    DOM.menu_button = document.getElementById("open_graph_menu");
    DOM.menu = document.getElementById("graph_menu");
    DOM.filter_users = document.querySelector("#hide_users");
    DOM.filter_roles = document.querySelector("#hide_roles");
    DOM.filter_loops = document.querySelector("#hide_loops");

    DOM.menu_button.addEventListener("click", () => {
        if (DOM.menu.open)
            DOM.menu.close();
        else
            DOM.menu.show();
    });

    DOM.filter_users.addEventListener("change", () => {
        DOM.graph.filter_unused_users(DOM.filter_users.checked);
    });

    DOM.filter_roles.addEventListener("change", () => {
        DOM.graph.filter_unused_subset_roles(DOM.filter_roles.checked, domain_id);
    });

    DOM.filter_loops.addEventListener("change", () => {
        DOM.graph.filter_unused_subset_loops(DOM.filter_loops.checked, domain_id);
    });

    VIEW_ID = view_id;

    DOM.graph.allow_highlighted_loops = ALLOW_HIGHLIGHTED_LOOPS;
    DOM.graph.no_new_users = ldap_auth;
}

export function render(domain_data, project_data) {
    DOM.graph.clear();
    if (project_data) {
        DOM.graph.add_node_subset(project_data, project_data.id);
    } else {
        document.querySelector("#hide_roles + label").style.display = "none";
        document.querySelector("#hide_loops + label").style.display = "none";
    }
    if (domain_data) {
        DOM.graph.add_node_subset(domain_data, domain_data.id);
        domain_id = domain_data.id;
    }
    DOM.graph.fill_out_links();

    DOM.graph.filter_unused_users(DOM.filter_users.checked);
    DOM.graph.filter_unused_subset_roles(DOM.filter_roles.checked, domain_id);
    DOM.graph.filter_unused_subset_loops(DOM.filter_loops.checked, domain_id);
    DOM.graph.render_edges();
    console.log("(project rbac) View rendered");
}

export function refresh() {
    DOM.graph.render_edges();
}

export function collect(id, include_unspecified) {
    return DOM.graph.collect_node_subset(id, include_unspecified);
}

export function users() {
    return DOM.graph.users;
}

export function remove() {

}