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
    @file           branding.js

    @ingroup        components/branding

    @brief          custom web component

    ---------------------------------------------------------------------------
*/
import * as CSS from "/css/css.js";

export default class ov_Branding extends HTMLElement {
    #dom;

    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
        this.#dom = {};
    }

    static get observedAttributes() {
        return ["subtitle"];
    }

    attributeChangedCallback(name, oldValue, newValue) {
        if (oldValue === newValue)
            return;
        if (name === "subtitle")
            this.#update_subtitle(newValue);
    }

    async connectedCallback() {
        await this.#render();
        this.#dom.subtitle = this.shadowRoot.querySelector("#subtitle");
        this.#dom.branding = this.shadowRoot.querySelector("#branding");
        this.#update_subtitle(this.getAttribute("subtitle"));
    }

    async #render() {
        this.shadowRoot.adoptedStyleSheets = [await CSS.normalize_style_sheet, await CSS.ov_basic_style_sheet, await ov_Branding.style_sheet];
        this.shadowRoot.appendChild((await ov_Branding.template).content.cloneNode(true));
    }

    set subtitle(text) {
        this.setAttribute("subtitle", text);
    }

    #update_subtitle(text) {
        if (this.#dom.subtitle) {
            this.#dom.subtitle.innerText = text;
            this.#dom.branding.classList.toggle("subtitle_unset", !text);
        }
    }

    static style_sheet = CSS.fetch_style_sheet("/components/branding/branding.css");
    static template = async function () {
        const response = await fetch('/components/branding/branding.html');
        let dom = new DOMParser().parseFromString(await response.text(), 'text/html');
        return dom.querySelector('template');
    }();
}

customElements.define('ov-branding', ov_Branding);