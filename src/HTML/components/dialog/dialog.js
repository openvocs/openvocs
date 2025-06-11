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

export default class ov_Dialog extends HTMLElement {
    #dom = {};

    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
    }

    // attributes -------------------------------------------------------------
    static get observedAttributes() {
        return ["open", "position", "value"];
    }

    attributeChangedCallback(name, oldValue, newValue) {
        if (oldValue === newValue)
            return;
        if (name === "open" && this.#dom.dialog) {
            this.#open(this.hasAttribute("open"));
        } else if (name === "value" && this.#dom.title) {
            this.#change_title(newValue);
        } else if (name === "close-button") {
            this.#change_close_button(this.hasAttribute("close-button"));
        }
    }

    get open() {
        return this.hasAttribute("open");
    }

    show() {
        this.setAttribute("open", '');
    }

    hide() {
        this.removeAttribute("open");
    }

    toggle() {
        if (this.open)
            this.hide();
        else
            this.show();
    }

    #open(value) {
        this.#dom.dialog.classList.toggle("open", value);
    }

    set position(pos) {
        this.setAttribute("position", pos);
    }

    get position() {
        return this.getAttribute("position");
    }

    set value(val) {
        this.setAttribute("value", val);
    }

    get value() {
        return this.getAttribute("value");
    }

    #change_title(title) {
        this.#dom.title.innerText = title;
    }

    set close_button(value) {
        if (value)
            this.setAttribute("close-button", "");
        else
            this.removeAttribute("close-button");
    }

    get close_button() {
        return this.hasAttribute("close-button");
    }

    #change_close_button(value) {
        this.#dom.dialog.classList.toggle("hide_close", !value);
    }

    // -----------------------------------------------------------------
    async connectedCallback() {
        await this.#render();
        this.#dom.dialog = this.shadowRoot.querySelector("#dialog");
        this.#dom.title = this.shadowRoot.querySelector("#dialog_title")
        this.#dom.exit_area = this.shadowRoot.querySelector("#dialog_exit_area");
        this.#dom.close_button = this.shadowRoot.querySelector("#close_button");

        this.#dom.exit_area.addEventListener("click", () => {
            this.hide();
        });

        this.#dom.close_button.addEventListener("click", () => {
            this.hide();
        });

        this.#open(this.hasAttribute("open"));
        if (!this.hasAttribute("position"))
            this.setAttribute("position", "center");
        if (this.hasAttribute("value"))
            this.#change_title(this.getAttribute("value"));
        this.#change_close_button(this.hasAttribute("close-button"));
    }

    async #render() {
        this.shadowRoot.adoptedStyleSheets = [await CSS.normalize_style_sheet, await CSS.ov_basic_style_sheet, await ov_Dialog.style_sheet];
        this.shadowRoot.replaceChildren((await ov_Dialog.template).content.cloneNode(true));
    }

    static style_sheet = CSS.fetch_style_sheet("/components/dialog/dialog.css");
    static template = async function () {
        const response = await fetch('/components/dialog/dialog.html');
        let dom = new DOMParser().parseFromString(await response.text(), 'text/html');
        return dom.querySelector('template');
    }();
}

customElements.define('ov-dialog', ov_Dialog);