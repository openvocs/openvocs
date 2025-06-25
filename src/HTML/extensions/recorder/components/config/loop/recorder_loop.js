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
    @file           recorder_loop.js

    @ingroup        extensions/recorder/components/config/loop

    @brief          custom web component

    ---------------------------------------------------------------------------
*/

import * as CSS from "/css/css.js";

export default class ov_Recorder_Loop extends HTMLElement {

    #name;
    #project;
    #domain;
    #active;
    #disabled = false;

    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
    }

    static get observedAttributes() {
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

    set project(project) {
        this.#project = project;
    }

    get project() {
        return this.#project;
    }

    set domain(domain) {
        this.#domain = domain;
    }

    get domain() {
        return this.#domain;
    }

    set active(value) {
        this.#active = value;
        this.#update_activity_indicator();
    }

    get active() {
        return this.#active;
    }

    #update_disabled() {
        let button = this.shadowRoot.querySelector("button");
        if (button)
            button.disabled = this.#disabled;
    }

    set disabled(value) {
        this.#disabled = value;
        this.#update_disabled();
    }

    get disabled() {
        return this.#disabled;
    }

    attributeChangedCallback(name, old_value, new_value) {
        if (old_value === new_value)
            return;

    }

    async connectedCallback() {
        await this.#render();
        this.#update_name();
        this.#update_disabled();
        this.#update_activity_indicator();
    }

    #update_activity_indicator() {
        if (!this.#active) {
            let element = this.shadowRoot.querySelector(".recording_active");
            if (element)
                element.classList.remove("recording_active");
        } else {
            let element = this.shadowRoot.querySelector("#loop:not(.recording_active)");
            if (element)
                element.classList.add("recording_active");
        }
    }

    async #render() {
        this.shadowRoot.adoptedStyleSheets = [await CSS.normalize_style_sheet, await CSS.ov_basic_style_sheet, await ov_Recorder_Loop.style_sheet];
        this.shadowRoot.replaceChildren((await ov_Recorder_Loop.template).content.cloneNode(true));
    }

    static style_sheet = CSS.fetch_style_sheet("/extensions/recorder/components/config/loop/recorder_loop.css");
    static template = async function () {
        const response = await fetch('/extensions/recorder/components/config/loop/recorder_loop.html');
        let dom = new DOMParser().parseFromString(await response.text(), 'text/html');
        return dom.querySelector('template');
    }();
}

customElements.define('ov-recorder-config-loop', ov_Recorder_Loop);