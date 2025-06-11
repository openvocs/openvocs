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
    @file           sip_role.js

    @author         Anja Bertard

    @date           2024-09-06

    @ingroup        extensions/sip/config/role

    @brief          custom web component

    ---------------------------------------------------------------------------
*/

import * as CSS from "/css/css.js";
export default class ov_SIP_Role extends HTMLElement {

    #id;
    #name;
    #value;

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

    #update_name() {
        let element = this.shadowRoot.querySelector("#name");
        if (element)
            element.innerText = this.#name;
    }

    set id(value) {
        this.#id = value;
    }

    get id() {
        return this.#id;
    }

    set name(value) {
        this.#name = value;
        this.#update_name();
    }

    get name() {
        return this.#name;
    }

    #update_value() {
        let element = this.shadowRoot.querySelector("#sip_rights");
        if (element)
            element.value = this.#value;
    }

    set value(value) {
        this.#value = value;
        this.#update_value();
    }

    get value() {
        if (this.shadowRoot.querySelector("#sip_rights"))
            return this.shadowRoot.querySelector("#sip_rights").value;
        return this.#value;
    }

    async connectedCallback() {
        await this.#render();

        this.#update_name();
        this.#update_value();
    }

    async #render() {
        this.shadowRoot.adoptedStyleSheets = [await CSS.normalize_style_sheet, await CSS.ov_basic_style_sheet, await ov_SIP_Role.style_sheet];
        this.shadowRoot.replaceChildren((await ov_SIP_Role.template).content.cloneNode(true));
    }

    static style_sheet = CSS.fetch_style_sheet("/extensions/sip/components/config/role/sip_role.css");
    static template = async function () {
        const response = await fetch('/extensions/sip/components/config/role/sip_role.html');
        let dom = new DOMParser().parseFromString(await response.text(), 'text/html');
        return dom.querySelector('template');
    }();
}

customElements.define('ov-sip-config-role', ov_SIP_Role);