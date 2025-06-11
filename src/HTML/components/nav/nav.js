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
    @file           nav.js

    @author         Anja Bertard

    @date           2023-09-04

    @ingroup        components/nav

    @brief          custom web component, extents <nav>

    ---------------------------------------------------------------------------
*/
import * as CSS from "/css/css.js";

export default class ov_Nav extends HTMLElement {
    constructor() {
        super();
    }

    async connectedCallback() {
        this.dispatchEvent(new CustomEvent("loaded"));
    }

    // attributes -------------------------------------------------------------
    static get observedAttributes() {
        return ["name"];
    }

    set name(text) {
        this.setAttribute("name", text);
    }

    get name() {
        return this.getAttribute("name");
    }

    attributeChangedCallback(name, oldValue, newValue) {
        if (oldValue === newValue)
            return;
        if (name === "name")
            this.#update_name(newValue);
    }

    #update_name(text) {
        let input_list = this.querySelectorAll("input");
        for (let input of input_list) {
            input.name = text;
        }
    }

    // ------------------------------------------------------------------------
    async add_item(id, name, value, selected) {
        let input = (await ov_Nav.template).content.firstElementChild.cloneNode(true);
        let label = (await ov_Nav.template).content.lastElementChild.cloneNode(true);

        input.id = id;
        input.name = this.getAttribute("name");
        input.value = value;

        label.setAttribute("for", id);
        label.querySelector("span").innerText = name;

        this.appendChild(input);
        this.appendChild(label);

        if (selected)
            this.value = value;
    }

    clear() {
        let style = this.querySelector("style");
        this.replaceChildren();
        if (style)
            this.prepend(style);
    }

    highlight(id, value) {
        let element = this.querySelector("#" + id);
        if (element) {
            element.classList.toggle("highlight", value);
            void element.offsetWidth;
            element.classList.toggle("highlight_end", !value);
        }
    }

    indicate_loading(id, value) {
        let element = this.querySelector("#" + id + "+label");
        if (value)
            element.classList.add("loading");
        else
            element.classList.remove("loading");
    }

    set value(value) {
        let element = this.querySelector("input[value='" + value + "']");
        if (element) {
            element.checked = true;
            element.dispatchEvent(new Event('change', { bubbles: true }));
            element.scrollIntoView();
        }
    }

    get value() {
        let element = this.querySelector('input:checked');
        if (element)
            return element.value;
        return undefined;
    }

    static style_sheet = CSS.fetch_style_sheet("/components/nav/nav.css");
    static template = async function () {
        const response = await fetch('/components/nav/nav.html');
        let dom = new DOMParser().parseFromString(await response.text(), 'text/html');
        return dom.querySelector('template');
    }();
}

customElements.define('ov-nav', ov_Nav);