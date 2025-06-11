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
    @file           dialog.js

    @ingroup        components/dialog/dialog

    @brief          custom web component

    ---------------------------------------------------------------------------
*/
import * as CSS from "/css/css.js";

export default class ov_Loading extends HTMLElement {
    #dom = {};

    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
    }

    // attributes -------------------------------------------------------------
    static get observedAttributes() {
    }

    /*attributeChangedCallback(name, oldValue, newValue) {
        if (oldValue === newValue)
            return;
        if (name === "open" && this.#dom.dialog) {
            this.#open(this.hasAttribute("open"));
        } else if (name === "value" && this.#dom.title) {
            this.#change_title(newValue);
        } else if (name === "close-button") {
            this.#change_close_button(this.hasAttribute("close-button"));
        }
    }*/

    show(message) {
        this.#dom.loading_screen.classList.add("loading");
        if (message) {
            this.#dom.message.innerText = message;
            this.#dom.message.classList.remove("hidden");
        }
    }

    hide() {
        this.#dom.loading_screen.classList.remove("loading");
        this.#dom.message.classList.add("hidden");
    }

    delayed_hide() {
        setTimeout(() => {
            this.hide(); //screen is shown for at least a second
        }, 1000);
    }

    // -----------------------------------------------------------------
    async connectedCallback() {
        await this.#render();

        this.#dom.loading_screen = this.shadowRoot.querySelector("#loading_screen");
        this.#dom.message = this.shadowRoot.querySelector("#message");
        this.#dom.button = this.shadowRoot.querySelector("#button");

        this.shadowRoot.getElementById("button").addEventListener("click", () => {
            this.dispatchEvent(new CustomEvent("loading_button_clicked", { bubbles: true }));
        });

        this.dispatchEvent(new CustomEvent("loaded"));

    }

    async #render() {
        this.shadowRoot.adoptedStyleSheets = [await CSS.normalize_style_sheet, await CSS.loading_id_style_sheet, await CSS.ov_basic_style_sheet, await ov_Loading.style_sheet];
        this.shadowRoot.replaceChildren((await ov_Loading.template).content.cloneNode(true));
    }

    static style_sheet = CSS.fetch_style_sheet("/components/loading/loading_screen.css");
    static template = async function () {
        const response = await fetch('/components/loading/loading_screen.html');
        let dom = new DOMParser().parseFromString(await response.text(), 'text/html');
        return dom.querySelector('template');
    }();
}

customElements.define('ov-loading', ov_Loading);