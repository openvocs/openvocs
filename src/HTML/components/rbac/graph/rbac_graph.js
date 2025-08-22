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

    This file is part of the openvocs project. https://openvocs.orgender_edges

    ---------------------------------------------------------------------------
*//**
    @file           rbac_graph.js

    @ingroup        components/rbac/graph

    @brief          custom web component

    ---------------------------------------------------------------------------
*/
import * as Graph from "./js/graph.js";
import * as CSS from "/css/css.js";

export default class ov_RBAC_Graph extends HTMLElement {

    #dom = {};

    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
    }

    // attributes -------------------------------------------------------------
    static get observedAttributes() {
    }

    attributeChangedCallback(name, oldValue, newValue) {
        if (oldValue === newValue)
            return;
    }

    // -----------------------------------------------------------------
    async connectedCallback() {
        await this.#render();

        this.#dom.graph = this.shadowRoot.getElementById("graph");
        this.#dom.node_layer_1 = this.shadowRoot.getElementById("user_nodes");
        this.#dom.node_layer_2 = this.shadowRoot.getElementById("role_nodes");
        this.#dom.node_layer_3 = this.shadowRoot.getElementById("loop_nodes");
        this.#dom.edge_container_1 = this.shadowRoot.getElementById("user_role_connections");
        this.#dom.edge_layer_1 = this.shadowRoot.querySelector("#user_role_connections svg");
        this.#dom.edge_layer_2 = this.shadowRoot.querySelector("#role_loop_connections svg");
        this.#dom.add_user = this.shadowRoot.querySelector("#user_header .add_entry");
        this.#dom.add_role = this.shadowRoot.querySelector("#role_header .add_entry");
        this.#dom.add_loop = this.shadowRoot.querySelector("#loop_header .add_entry");

        let source_node;

        window.addEventListener("keydown", (event) => {
            if (event.code === "Escape" && source_node) {
              
                Graph.highlight_node(source_node, "select", false);

                for (let user of Graph.nodes.users.values()) {
                    user.classList.toggle("edit_" + source_node.type + "_edges", false);
                }
                for (let role of Graph.nodes.roles.values()) {
                    role.classList.toggle("edit_" + source_node.type + "_edges", false);
                }
                for (let loop of Graph.nodes.loops.values()) {
                    loop.classList.toggle("edit_" + source_node.type + "_edges", false);
                }
                source_node.classList.toggle("edit", false);
                source_node = undefined;
            }

        });

        this.#dom.graph.addEventListener("delete_node", (event) => {
            let node = event.detail.node;

            Graph.highlight_node(node, "highlight", false);
            Graph.highlight_node(node, "select", false);

            Graph.delete_node(node);

            this.#adjust_grid_size();

            Graph.group_nodes(this.shadowRoot, Graph.nodes);
            this.render_edges();
        });

        this.#dom.graph.addEventListener("edit_edges", (event) => {
            let node = event.detail.node;
            let value = event.detail.value;
            Graph.highlight_node(node, "select", value);

            for (let user of Graph.nodes.users.values()) {
                user.classList.toggle("edit_" + node.type + "_edges", value);
            }
            for (let role of Graph.nodes.roles.values()) {
                role.classList.toggle("edit_" + node.type + "_edges", value);
            }
            for (let loop of Graph.nodes.loops.values()) {
                loop.classList.toggle("edit_" + node.type + "_edges", value);
            }
            node.classList.toggle("edit", value);
            source_node = value ? node : undefined;
        });

        this.#dom.graph.addEventListener("add_edge", (event) => {
            Graph.highlight_node(source_node, "highlight", false);
            Graph.highlight_node(source_node, "select", false);

            Graph.add_link(source_node, event.detail.target, event.detail.value);

            Graph.group_nodes(this.shadowRoot, Graph.nodes);
            this.render_edges();

            Graph.highlight_node(source_node, "highlight", true);
            Graph.highlight_node(source_node, "select", true);
        });

        this.#dom.graph.addEventListener("delete_edge", (event) => {
            Graph.highlight_node(source_node, "highlight", false);
            Graph.highlight_node(source_node, "select", false);

            Graph.delete_link(source_node, event.detail.target);

            Graph.group_nodes(this.shadowRoot, Graph.nodes);
            this.render_edges();

            Graph.highlight_node(source_node, "highlight", true);
            Graph.highlight_node(source_node, "select", true);
        });

        this.#dom.graph.addEventListener("save_node", (event) => {
            if (!event.detail.update) {
                if (!Graph.nodes[event.detail.node.type + "s"].has(event.detail.node.node_id))
                    Graph.register_node(event.detail.node);
                else {
                    event.detail.node.node_id = undefined;
                    event.detail.node.show_settings("ID is already assigned to other node. Please change ID.");
                }
            }
        });

        this.#dom.add_user.addEventListener("click", () => {
            this.#render_node(undefined, "user");
        });

        this.#dom.add_role.addEventListener("click", () => {
            this.#render_node(undefined, "role");
        });

        this.#dom.add_loop.addEventListener("click", () => {
            this.#render_node(undefined, "loop");
        });

        /* tool bar ------
        this.shadowRoot.getElementById("node_search").onkeyup = function (event) {
            if (event.key === "Enter") {
                window.find(this.value, false, false, true);
            }
        };*/
    }

    set data(data) {
        this.clear();
        this.add_node_subset(data);
        this.fill_out_links();
        if (data.users || data.roles || data.loops)
            this.render_edges();
    }

    get data() {
        return collect_node_subset();
    }

    get users() {
        return Graph.nodes.users;
    }

    filter_unused_users(value) {
        for (let user of Graph.nodes.users.values()) {
            if (user.linked_nodes.size === 0) {
                if (value)
                    user.style.display = "none";
                else
                    user.style.display = "initial";
            }
        }
        this.render_edges();
    }

    filter_unused_subset_roles(value, subset_id) {
        for (let role of Graph.nodes.roles.values()) {
            if (role.subset === subset_id) {
                let linked_to_not_subset_loop = false;
                for (let link of role.linked_nodes.keys()) {
                    if (link.type === "loop" && link.subset !== subset_id) {
                        linked_to_not_subset_loop = true;
                        break;
                    }
                }
                if (!linked_to_not_subset_loop) {
                    if (value)
                        role.style.display = "none";
                    else
                        role.style.display = "initial";
                }
            }
        }
        this.render_edges();
    }

    filter_unused_subset_loops(value, subset_id) {
        for (let loop of Graph.nodes.loops.values()) {
            if (loop.subset === subset_id) {
                let linked_to_not_subset_role = false;
                for (let link of loop.linked_nodes.keys()) {
                    if (link.subset !== subset_id) {
                        linked_to_not_subset_role = true;
                        break;
                    }
                }
                if (!linked_to_not_subset_role) {
                    if (value)
                        loop.style.display = "none";
                    else
                        loop.style.display = "initial";
                }
            }
        }
        this.render_edges();
    }

    add_node_subset(data, id) {
        if (data.users || data.roles || data.loops) {
            // order is important! first users than roles than loops
            if (data.users)
                for (let node of Object.values(data.users))
                    this.#render_node(node, "user", id);

            if (data.roles) {
                let admin = false;
                for (let node of Object.values(data.roles)) {
                    this.#render_node(node, "role", id);
                    if (node.id === "admin")
                        admin = true;
                }
                if (!admin)
                    this.#render_node({ id: "admin" }, "role", id);

            }

            if (data.loops)
                for (let node of Object.values(data.loops))
                    this.#render_node(node, "loop", id);


            this.#dom.add_user.scrollIntoView({ behavior: "smooth", block: "start" });
        }
    }

    fill_out_links() {
        Graph.fill_out_links();
        Graph.group_nodes(this.shadowRoot, Graph.nodes);
    }

    collect_node_subset(id, include_unspecified) {
        let data = {
            users: {},
            roles: {},
            loops: {}
        };

        for (let user of Graph.nodes.users.values()) {
            if (!user.frozen && (!id || user.subset === id || (include_unspecified && user.subset === undefined)))
                data.users[user.node_id] = user.data;
        }
        for (let role of Graph.nodes.roles.values())
            if (!role.frozen && (!id || role.subset === id || (include_unspecified && role.subset === undefined))) {
                let role_data = role.data;
                data.roles[role_data.id] = role_data;
            }

        for (let loop of Graph.nodes.loops.values())
            if (!loop.frozen && (!id || loop.subset === id || (include_unspecified && loop.subset === undefined)))
                data.loops[loop.node_id] = loop.data;

        return data;
    }

    #adjust_grid_size() {
        let number_rows = Math.max(Graph.nodes.users.size, Graph.nodes.roles.size, Graph.nodes.loops.size);
        this.shadowRoot.host.style.setProperty("--rowNum", number_rows);
    }

    clear() {
        Graph.clear();
        this.#dom.edge_layer_1.replaceChildren();
        this.#dom.edge_layer_2.replaceChildren();
        this.#dom.node_layer_1.replaceChildren();
        this.#dom.node_layer_2.replaceChildren();
        this.#dom.node_layer_3.replaceChildren();
    }

    #render_node(node, type, subset_id) {
        let element = Graph.create_node(type, node, subset_id);
        let container;
        if (type === "user")
            container = this.#dom.node_layer_1;
        else if (type === "role")
            container = this.#dom.node_layer_2;
        else if (type === "loop")
            container = this.#dom.node_layer_3;
        container.appendChild(element);

        if (node && (node.ldap || node.frozen))
            element.frozen = true;
        this.#adjust_grid_size();
        element.scrollIntoView({ behavior: "smooth", block: "start" });
    }

    render_edges() {
        Graph.edges.clear();
        this.#dom.edge_layer_1.replaceChildren();
        this.#dom.edge_layer_2.replaceChildren();

        let layer_offset_top = this.#dom.edge_container_1.offsetTop;
        let layer_width = this.#dom.edge_container_1.offsetWidth;

        for (let role of Graph.nodes.roles.values()) {
            for (let node of role.linked_nodes.keys()) {
                if (node.style.display !== "none" && role.style.display !== "none") {
                    let edge;
                    if (node.type === "user" && Graph.nodes.users.has(node.node_id)) {
                        edge = Graph.create_edge(role, node, layer_offset_top, layer_width);
                        this.#dom.edge_layer_1.appendChild(edge);
                    } else if (node.type === "loop" && Graph.nodes.loops.has(node.node_id)) {
                        edge = Graph.create_edge(node, role, layer_offset_top, layer_width);
                        this.#dom.edge_layer_2.appendChild(edge);
                    }
                }
            }
        }
    }

    async #render() {
        this.shadowRoot.adoptedStyleSheets = [await CSS.normalize_style_sheet, await CSS.ov_basic_style_sheet, await ov_RBAC_Graph.style_sheet];
        this.shadowRoot.replaceChildren((await ov_RBAC_Graph.template).content.cloneNode(true));
    }

    static style_sheet = CSS.fetch_style_sheet("/components/rbac/graph/rbac_graph.css");
    static template = async function () {
        const response = await fetch('/components/rbac/graph/rbac_graph.html');
        let dom = new DOMParser().parseFromString(await response.text(), 'text/html');
        return dom.querySelector('template');
    }();
}


customElements.define('ov-rbac-graph', ov_RBAC_Graph);