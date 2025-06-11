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
    @file           sip.js

    @author         Tobias Kolb, Anja Bertard

    @date           2023-08-02

    @ingroup        extensions/sip/number_pad

    @brief          custom web component

    ---------------------------------------------------------------------------
*/

import * as CSS from "/css/css.js";
export default class ov_SIP_Calls extends HTMLElement {
    #calls;
    #loop;
    #allow_new_calls;

    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
        this.#calls = new Map();
    }

    static get observedAttributes() {
        return ["loop", "new_calls"];
    }

    attributeChangedCallback(name, old_value, new_value) {
        if (old_value === new_value)
            return;

        if (name === "loop") {
            this.#loop = new_value;
            let title = this.shadowRoot.querySelector('#loop');
            if (title)
                title.innerHTML = this.#loop;
        } else if (name === "new_calls") {
            this.#allow_new_calls = new_value === "true";
            this.#disable_new_calls(!this.#allow_new_calls);
        }
    }

    set loop(value) {
        this.setAttribute("loop", value);
    }

    get loop() {
        return this.#loop;
    }

    get calls() {
        return this.#calls;
    }

    set allow_new_calls(value) {
        this.setAttribute("new_calls", value);
    }

    async connectedCallback() {
        await this.#render();

        if (this.#loop)
            this.shadowRoot.querySelector('#loop').innerHTML = this.#loop;

        this.shadowRoot.querySelector("#new_call").onclick = () => {
            this.dispatchEvent(new CustomEvent("new_call"));
        }

        this.#disable_new_calls(!this.#allow_new_calls);
        this.dispatchEvent(new CustomEvent("loaded"));
    }

    async add_call(call_id, peer) {
        let current_calls = this.shadowRoot.querySelector("#current_calls");
        current_calls.appendChild((await ov_SIP_Calls.call_template).content.cloneNode(true));

        let call_entry = current_calls.lastElementChild;
        this.#calls.set(call_id, call_entry);

        call_entry.id = "call_" + call_id;
        call_entry.querySelector(".call_number").innerText = peer;
        this.change_call_status(call_id, "connecting...");
        call_entry.querySelector(".sip_hangup").onclick = (event) => {
            this.removeAttribute("error");
            this.dispatchEvent(new CustomEvent("trigger_hangup", {
                detail: { call: call_id, loop: this.loop }
            }));
        };
    }

    remove_call(call_id) {
        if (this.#calls.has(call_id)) {
            this.shadowRoot.querySelector("#current_calls").removeChild(this.#calls.get(call_id));
            this.#calls.delete(call_id);
        }
    }

    change_call_status(call_id, status) {
        let call_status = this.#calls.get(call_id).querySelector(".call_status");
        if (call_status)
            call_status.innerHTML = status;
    }

    deactivate_call(call_id) {
        this.#calls.get(call_id).querySelector(".sip_hangup").disabled = true;
    }

    #disable_new_calls(value) {
        let element = this.shadowRoot.querySelector("#new_call");
        if (element)
            element.disabled = value;
    }

    async #render() {
        this.shadowRoot.adoptedStyleSheets = [await CSS.normalize_style_sheet, await CSS.ov_basic_style_sheet, await ov_SIP_Calls.style_sheet];
        this.shadowRoot.replaceChildren((await ov_SIP_Calls.template).content.cloneNode(true));

        this.shadowRoot.querySelector("#new_call").onclick = (event) => {
            this.dispatchEvent(new CustomEvent("show_number_pad"));
        }
    }

    static call_template;

    static style_sheet = CSS.fetch_style_sheet("/extensions/sip/components/call_list/call_list.css");
    static template = async function () {
        const response = await fetch('/extensions/sip/components/call_list/call_list.html');
        let dom = new DOMParser().parseFromString(await response.text(), 'text/html');
        ov_SIP_Calls.call_template = dom.querySelector("#call_template");
        return dom.querySelector('template');
    }();
}

customElements.define('ov-sip-calls', ov_SIP_Calls);