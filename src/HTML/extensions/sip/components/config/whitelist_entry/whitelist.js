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
    @file           whitelist.js

    @author         Anja Bertard

    @date           2024-09-05

    @ingroup        extensions/sip/config/whitelist_entry

    @brief          custom web component

    ---------------------------------------------------------------------------
*/

import * as CSS from "/css/css.js";
export default class ov_SIP_Whitelist extends HTMLElement {

    #caller;
    #callee;

    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
    }

    static get observedAttributes() {
    }

    attributeChangedCallback(name, old_value, new_value) {
        if (old_value === new_value)
            return;
    }

    #update_caller() {
        let element = this.shadowRoot.querySelector("#caller");
        if (element) {
            if (this.#caller === undefined)
                this.#caller = "";
            element.value = this.#caller;
        }
    }

    set caller(value) {
        this.#caller = value;
        this.#update_caller();
    }

    get caller() {
        if (this.shadowRoot.querySelector("#caller"))
            return this.shadowRoot.querySelector("#caller").value;
        return this.#caller;
    }

    set callee(value) {
        this.#callee = value;
        this.#update_callee();
    }

    #update_callee() {
        let element = this.shadowRoot.querySelector("#callee");
        if (element) {
            if (this.#callee === undefined)
                this.#callee = "";
            element.value = this.#callee;
        }
    }

    get callee() {
        if (this.shadowRoot.querySelector("#callee"))
            return this.shadowRoot.querySelector("#callee").value;
        return this.#callee;
    }

    async connectedCallback() {
        await this.#render();

        this.#update_caller();
        this.#update_callee();

        this.shadowRoot.querySelector("#delete_whitelist_entry").addEventListener("click", () => {
            this.dispatchEvent(new CustomEvent("delete_entry"));
        });
    }

    async #render() {
        this.shadowRoot.adoptedStyleSheets = [await CSS.normalize_style_sheet, await CSS.ov_basic_style_sheet, await ov_SIP_Whitelist.style_sheet];
        this.shadowRoot.replaceChildren((await ov_SIP_Whitelist.template).content.cloneNode(true));
    }

    static style_sheet = CSS.fetch_style_sheet("/extensions/sip/components/config/whitelist_entry/whitelist.css");
    static template = async function () {
        const response = await fetch('/extensions/sip/components/config/whitelist_entry/whitelist.html');
        let dom = new DOMParser().parseFromString(await response.text(), 'text/html');
        return dom.querySelector('template');
    }();
}

customElements.define('ov-sip-whitelist', ov_SIP_Whitelist);