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
    @file           rbac_node.js

    @ingroup        components/rbac/node

    @brief          custom web component

    ---------------------------------------------------------------------------
*/
import create_uuid from "/lib/ov_utils/ov_uuid.js";
import * as CSS from "/css/css.js";

import ov_Dialog from "/components/dialog/dialog.js";

export default class ov_RBAC_Node extends HTMLElement {
    #type;
    #value;
    #frozen;
    #global;
    #subset;

    #node_name;
    #node_id;
    #node_abbreviation;
    #node_multicast_ip;
    #node_multicast_port;
    #node_password;
    #linked_nodes;
    #passive_linked_entries;

    #dom = {};

    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
    }

    toString() {
        return this.#node_id;
    }

    generate_id() {
        if (!this.type)
            this.type = "node";
        this.id = this.type + "_" + create_uuid();
    }

    // attributes -------------------------------------------------------------
    static get observedAttributes() {
        return ["type", "value", "frozen", "global"];
    }

    attributeChangedCallback(name, oldValue, newValue) {
        if (oldValue === newValue)
            return;
        if (name === "type") {
            this.#type = newValue;
        } else if (name === "value") {
            this.#update_value(newValue);
        } else if (name === "frozen") {
            this.#update_frozen(this.hasAttribute("frozen"));
        } else if (name === "global") {
            this.#update_global(this.hasAttribute("global"));
        }
    }

    set type(text) {
        this.setAttribute("type", text);
    }

    get type() {
        return this.#type;
    }

    set value(text) {
        this.setAttribute("value", text);
    }

    get value() {
        return this.#value;
    }

    set frozen(boolean) {
        if (boolean)
            this.setAttribute("frozen", "");
        else
            this.removeAttribute("frozen")
    }

    get frozen() {
        return this.hasAttribute("frozen");
    }

    set global(boolean) {
        if (boolean)
            this.setAttribute("global", "");
        else
            this.removeAttribute("global")
    }

    get global() {
        return this.hasAttribute("global");
    }

    set subset(id) {
        this.#subset = id;
    }

    get subset() {
        return this.#subset;
    }

    set node_id(text) {
        this.#node_id = text;
        if (text && (!this.node_name || !this.#node_abbreviation))
            this.value = text.length > 16 ? text.slice(0, 13) + "..." : text;
    }

    get node_id() {
        return this.#node_id;
    }

    set node_name(text) {
        this.#node_name = text;
        if (text && text.length <= 16)
            this.value = text;
    }

    get node_name() {
        return this.#node_name;
    }

    set node_abbreviation(text) {
        this.#node_abbreviation = text;
        if (text && (!this.node_name || this.node_name.length > 16))
            this.value = text.length > 16 ? text.slice(0, 13) + "..." : text;
    }

    get node_abbreviation() {
        return this.#node_abbreviation;
    }

    set node_password(text) {
        this.#node_password = text;
    }

    get node_password() {
        return this.#node_password;
    }

    set node_multicast_ip(ip) {
        this.#node_multicast_ip = ip;
    }

    get node_multicast_ip() {
        return this.#node_multicast_ip;
    }

    set node_multicast_port(port) {
        this.#node_multicast_port = port;
    }

    get node_multicast_port() {
        return this.#node_multicast_port;
    }

    set linked_nodes(obj) {
        this.#linked_nodes = obj;
    }

    get linked_nodes() {
        return this.#linked_nodes;
    }

    set passive_linked_entries(obj) {
        this.#passive_linked_entries = obj;
    }

    get passive_linked_entries() {
        return this.#passive_linked_entries;
    }

    get data() {
        let data = {
            id: this.node_id,
            name: this.node_name ? this.node_name : null
        }

        if (this.node_abbreviation)
            data.abbreviation = this.node_abbreviation;

        if (this.node_multicast_ip && this.node_multicast_port)
            data.multicast = { host: this.node_multicast_ip, port: parseInt(this.node_multicast_port) };

        if (!this.passive_linked_entries)
            this.passive_linked_entries = {};

        if (this.type === "loop")
            data.roles = { ...this.passive_linked_entries, ...Object.fromEntries(this.linked_nodes) };
        else if (this.type === "role") {
            data.users = {
                ...this.passive_linked_entries,
                ...Object.fromEntries([...this.linked_nodes.entries()].filter(([node, permission]) => permission === null))
            };
        }
        return data;
    }

    #update_value(text) {
        this.#value = text;
        let name = this.shadowRoot.querySelector("#node_name");
        if (name)
            name.innerText = text;
    }

    #update_frozen(boolean) {
        this.#frozen = boolean;
        this.edit_id = this.shadowRoot.querySelector("#edit_id");
        let edit_name = this.shadowRoot.querySelector("#edit_name");
        let edit_abbr = this.shadowRoot.querySelector("#edit_abbreviation");
        let edit_pass = this.shadowRoot.querySelector("#edit_password");
        let edit_multicast_ip = this.shadowRoot.querySelector("#edit_multicast_ip");
        let edit_multicast_port = this.shadowRoot.querySelector("#edit_multicast_port");
        let delete_button = this.shadowRoot.querySelector("#delete_element");
        if (edit_id)
            edit_id.disabled = boolean;
        if (edit_name)
            edit_name.disabled = boolean;
        if (edit_abbr)
            edit_abbr.disabled = boolean;
        if (edit_pass)
            edit_pass.disabled = boolean;
        if (edit_multicast_ip)
            edit_multicast_ip.disabled = boolean;
        if (edit_multicast_port)
            edit_multicast_port.disabled = boolean;
        if (delete_button)
            delete_button.disabled = boolean;

    }

    #update_global(boolean) {
        this.#global = boolean;
    }

    // -----------------------------------------------------------------
    async connectedCallback() {
        await this.#render();

        this.#dom.name = this.shadowRoot.querySelector("#node_name");
        this.#dom.dialog = this.shadowRoot.querySelector("#settings");
        this.#dom.dialog.querySelector("h3").innerText = "Edit " + this.#type;
        this.#dom.edit_id = this.shadowRoot.querySelector("#edit_id");
        this.#dom.edit_name = this.shadowRoot.querySelector("#edit_name");
        this.#dom.edit_abbr = this.shadowRoot.querySelector("#edit_abbreviation");
        this.#dom.edit_pass = this.shadowRoot.querySelector("#edit_password");
        this.#dom.edit_multicast_ip = this.shadowRoot.querySelector("#edit_multicast_ip");
        this.#dom.edit_multicast_port = this.shadowRoot.querySelector("#edit_multicast_port");
        this.#dom.error_msg = this.shadowRoot.querySelector("#dialog_error_message");

        this.shadowRoot.querySelector("#edit_icon").onclick = () => {
            this.show_settings();
        };

        this.shadowRoot.querySelector("#save_element").onclick = () => {
            this.#save();
        };

        this.shadowRoot.querySelector("#delete_element").onclick = () => {
            this.#delete();
        };

        this.#dom.dialog.onclick = (e) => {
            if (e.target === this.#dom.dialog) {
                this.#abort();
            }
        }

        this.#dom.dialog.querySelector(".close_button").onclick = () => {
            this.#abort();
        }

        this.#dom.dialog.addEventListener("keydown", (event) => {
            if (event.keyCode === 13)
                this.#save();
            else if (event.keyCode === 27)
                this.#abort();
        });

        this.shadowRoot.querySelector("#network_icon").onclick = (event) => {
            this.dispatchEvent(new CustomEvent("edit_edges", {
                detail: { node: this, value: true }, bubbles: true
            }));
        };

        this.shadowRoot.querySelector("#exit_network_icon").onclick = (event) => {
            this.dispatchEvent(new CustomEvent("edit_edges", {
                detail: { node: this, value: false }, bubbles: true, composed: true
            }));
        };

        this.shadowRoot.querySelector("#headphone_icon").onclick = (event) => {
            this.dispatchEvent(new CustomEvent("add_edge", {
                detail: { target: this, value: false }, bubbles: true
            }));
        };

        this.shadowRoot.querySelector("#microphone_icon").onclick = (event) => {
            this.dispatchEvent(new CustomEvent("add_edge", {
                detail: { target: this, value: true }, bubbles: true
            }));
        };

        this.shadowRoot.querySelector("#deconnect_icon").onclick = (event) => {
            this.dispatchEvent(new CustomEvent("delete_edge", {
                detail: { target: this }, bubbles: true
            }));
        };

        this.shadowRoot.querySelector("#connect_icon").onclick = (event) => {
            this.dispatchEvent(new CustomEvent("add_edge", {
                detail: { target: this, value: null }, bubbles: true
            }));
        };

        if (this.frozen)
            this.#update_frozen(this.frozen);

        if (this.global)
            this.#update_global(this.global);

        if (this.value)
            this.#update_value(this.value);
        else
            this.show_settings();
    }

    show_settings(error) {
        if (error)
            this.#dom.error_msg.innerText = error;
        else
            this.#dom.error_msg.innerText = "";

        if (this.node_id) {
            this.#dom.edit_id.value = this.node_id;
            this.#dom.edit_id.disabled = true;
        } else
            this.#dom.edit_id.value = create_uuid();

        this.#dom.edit_name.value = this.node_name ? this.node_name : null;
        this.#dom.edit_abbr.value = this.node_abbreviation ? this.node_abbreviation : null;
        this.#dom.edit_pass.value = this.node_password ? this.node_password : null;
        this.#dom.edit_multicast_ip.value = this.node_multicast_ip ? this.node_multicast_ip :
            ((DEFAULT_MULTICAST_ADDRESS && DEFAULT_MULTICAST_ADDRESS !== "") ? DEFAULT_MULTICAST_ADDRESS : null);
        this.#dom.edit_multicast_port.value = this.node_multicast_port ? this.node_multicast_port : null;

        this.#dom.dialog.showModal();
    }

    #save() {
        if (this.frozen)
            this.#dom.dialog.close();
        if (!this.#dom.edit_id.disabled && this.type === "user" && (!this.#dom.edit_id.value || !this.#dom.edit_pass.value)) {
            this.#dom.error_msg.innerText = "Please set ID and password."
        } else if (!this.#dom.edit_id.disabled && !this.#dom.edit_id.value) {
            this.#dom.error_msg.innerText = "Please set an ID."
        } else {
            this.node_name = this.#dom.edit_name.value ? this.#dom.edit_name.value : undefined;
            this.node_abbreviation = this.#dom.edit_abbr.value ? this.#dom.edit_abbr.value : undefined;
            this.node_password = this.#dom.edit_pass.value ? this.#dom.edit_pass.value : undefined;
            this.node_multicast_ip = this.#dom.edit_multicast_ip.value ? this.#dom.edit_multicast_ip.value : undefined;
            this.node_multicast_port = this.#dom.edit_multicast_port.value ? this.#dom.edit_multicast_port.value : undefined;
            let update = true;
            if (!this.node_id) {
                this.node_id = this.#dom.edit_id.value;
                update = false;
            }
            this.#dom.dialog.close();
            this.dispatchEvent(new CustomEvent("save_node", {
                detail: { node: this, update: update }, bubbles: true, composed: true
            }));
        }
    }

    #abort() {
        this.#dom.dialog.close();
        if (!this.node_id)
            this.#delete();
    }

    #delete() {
        this.dispatchEvent(new CustomEvent("delete_node", {
            detail: { node: this }, bubbles: true
        }));
    }

    async #render() {
        this.shadowRoot.adoptedStyleSheets = [await CSS.normalize_style_sheet, await CSS.ov_basic_style_sheet, await ov_RBAC_Node.style_sheet];
        this.shadowRoot.replaceChildren((await ov_RBAC_Node.template).content.cloneNode(true));
    }

    static style_sheet = CSS.fetch_style_sheet("/components/rbac/node/rbac_node.css");
    static template = async function () {
        const response = await fetch('/components/rbac/node/rbac_node.html');
        let dom = new DOMParser().parseFromString(await response.text(), 'text/html');
        return dom.querySelector('template');
    }();
}

customElements.define('ov-rbac-node', ov_RBAC_Node);