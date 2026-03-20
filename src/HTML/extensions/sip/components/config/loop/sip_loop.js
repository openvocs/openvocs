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
    @file           sip_loop.js

    @ingroup        extensions/sip/config/loop

    @brief          custom web component

    ---------------------------------------------------------------------------
*/

import * as CSS from "/css/css.js";

export default class ov_SIP_Loop extends HTMLElement {

    #name;
    #whitelist = [];
    #roles = {};
    #selected = false;
    #disabled = false;
    #global = false;

    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
    }

    static get observedAttributes() {
        return ["disabled", "global", "selected"]
    }

    #update_name() {
        let element = this.shadowRoot.querySelector("#loop_name");
        if (element)
            element.innerText = this.#name;
    }

    set name(name) {
        this.#name = name;
        this.#update_name();
    }

    get name() {
        return this.#name;
    }

    clear_whitelist() {
        this.#whitelist = [];
        this.#update_sip_indicator();
    }

    add_whitelist_entry(caller, callee) {
        this.#whitelist.push({ caller: caller, callee: callee });
        this.#update_sip_indicator();
    }

    delete_whitelist_entry(entry) {
        let index = this.#whitelist.indexOf(entry);
        this.#whitelist.splice(index, 1);
        this.#update_sip_indicator();
    }

    get whitelist() {
        return this.#whitelist;
    }

    clear_roles() {
        this.#roles = {};
    }

    add_role(role, value, name, hidden) {
        this.#roles[role] = { value: value, name: name, hidden: hidden };
    }

    get roles() {
        return this.#roles;
    }

    #update_selected() {
        let button = this.shadowRoot.querySelector("button");
        if (button)
            button.disabled = this.#selected;
    }

    set selected(value) {
        if (!value)
            this.removeAttribute("selected")
        else
            this.setAttribute("selected", value);
    }

    get selected() {
        return this.#selected;
    }

    set disabled(value) {
        if (!value)
            this.removeAttribute("disabled")
        else
            this.setAttribute("disabled", value);
    }

    get disabled() {
        return this.#disabled;
    }

    set global(value) {
        if (!value)
            this.removeAttribute("global")
        else
            this.setAttribute("global", value);
    }

    get global() {
        return this.#global;
    }

    attributeChangedCallback(name, old_value, new_value) {
        if (old_value === new_value)
            return;
        if (name === "selected") {
            this.#selected = new_value;
            this.#update_selected();
        }
        if (name === "disabled") {
            this.#disabled = new_value;
        }
        if (name === "global") {
            this.#global = new_value;
        }
    }

    async connectedCallback() {
        await this.#render();
        this.#update_name();
        this.#update_selected();
        this.#update_sip_indicator();
    }

    #update_sip_indicator() {
        let roles_set = Object.values(this.#roles).some(role => role.value !== undefined);
        if (this.#whitelist.length !== 0 || roles_set) {
            let element = this.shadowRoot.querySelector("#loop:not(.calls_allowed)");
            if (element)
                element.classList.add("calls_allowed");
        } else {
            let element = this.shadowRoot.querySelector(".calls_allowed");
            if (element)
                element.classList.remove("calls_allowed");
        }
    }

    async #render() {
        this.shadowRoot.adoptedStyleSheets = [await CSS.normalize_style_sheet, await CSS.ov_basic_style_sheet, await ov_SIP_Loop.style_sheet];
        this.shadowRoot.replaceChildren((await ov_SIP_Loop.template).content.cloneNode(true));
    }

    static style_sheet = CSS.fetch_style_sheet("/extensions/sip/components/config/loop/sip_loop.css");
    static template = async function () {
        const response = await fetch('/extensions/sip/components/config/loop/sip_loop.html');
        let dom = new DOMParser().parseFromString(await response.text(), 'text/html');
        return dom.querySelector('template');
    }();
}

customElements.define('ov-sip-config-loop', ov_SIP_Loop);