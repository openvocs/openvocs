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

    @ingroup        extensions/sip/number_pad

    @brief          custom web component

    ---------------------------------------------------------------------------
*/

import * as CSS from "/css/css.js";
export default class ov_SIP_Number_Pad extends HTMLElement {
    #error;
    #loop;
    #keyboard;
    #dom = {};

    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
    }

    static get observedAttributes() {
        return ["error", "loop"];
    }

    attributeChangedCallback(name, old_value, new_value) {
        if (old_value === new_value)
            return;

        if (name === "error") {
            this.#error = new_value;
        }

        if (name === "loop") {
            this.#loop = new_value;
            let title = this.shadowRoot.querySelector('#loop');
            if (title)
                title.innerHTML = this.#loop;
        }
    }

    set error(value) {
        this.setAttribute("error", value);
    }

    get error() {
        return this.#error;
    }

    set loop(value) {
        this.setAttribute("loop", value);
    }

    get loop() {
        return this.#loop;
    }

    async connectedCallback() {
        await this.#render();

        if (this.#loop)
            this.shadowRoot.querySelector('#loop').innerHTML = this.#loop;

        const callButton = this.shadowRoot.querySelector('#sip-call-button');
        this.#dom.phoneNumberField = this.shadowRoot.querySelector('#sip-phone-number-field');
        this.#dom.phoneNumberField.focus();

        callButton.onclick = (event) => {
            this.removeAttribute("error");
            this.dispatchEvent(new CustomEvent("trigger_call", {
                detail: this.#dom.phoneNumberField.value
            }));
        }

        this.shadowRoot.querySelector('#number-field-wrapper span').onclick = () => {
            this.dispatchEvent(new CustomEvent("show_calls"));
        }

        if (window.SimpleKeyboard && matchMedia("(width <= 480px)").matches) {
            this.shadowRoot.querySelector(".ov_sip_keyboard").style.display = none;
        } else {
            this.#keyboard = this.shadowRoot.querySelector(".ov_sip_keyboard");
            this.#keyboard.addEventListener("click", (event) => {
                let input = this.#dom.phoneNumberField;
                let index = input.selectionStart;
                if (event.target.dataset.func === "backspace" && index !== 0) {
                    input.value = input.value.slice(0, index - 1) + input.value.slice(index);
                    index--;
                }
                if (event.target.dataset.value !== undefined) {
                    input.value = input.value.slice(0, index) + event.target.dataset.value + input.value.slice(index);
                    index++;
                }
                input.focus();
                input.selectionStart = index;
                input.selectionEnd = index;
            })
        }

    }

    clear_number() {
        this.#dom.phoneNumberField.value = "";
    }

    async #render() {
        this.shadowRoot.adoptedStyleSheets = [await CSS.normalize_style_sheet, await CSS.ov_basic_style_sheet, await ov_SIP_Number_Pad.simple_keyboard_style, await ov_SIP_Number_Pad.style_sheet];
        this.shadowRoot.replaceChildren((await ov_SIP_Number_Pad.template).content.cloneNode(true));
    }

    static style_sheet = CSS.fetch_style_sheet("/extensions/sip/components/number_pad/number_pad.css");
    static simple_keyboard_style = CSS.fetch_style_sheet("/plugin/simple-keyboard-3.8.0/css/index.css");
    static template = async function () {
        const response = await fetch('/extensions/sip/components/number_pad/number_pad.html');
        let dom = new DOMParser().parseFromString(await response.text(), 'text/html');
        return dom.querySelector('template');
    }();
}

customElements.define('ov-sip-number-pad', ov_SIP_Number_Pad);