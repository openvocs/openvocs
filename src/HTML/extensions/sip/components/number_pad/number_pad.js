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
export default class ov_SIP_Number_Pad extends HTMLElement {
    #error;
    #loop;
    #keyboard;
    #current_pointer_pos;
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

        const dialButtons = this.shadowRoot.querySelector(".number_pad");
        const callButton = this.shadowRoot.querySelector('#sip-call-button');
        this.#dom.phoneNumberField = this.shadowRoot.querySelector('#sip-phone-number-field');

        callButton.onclick = (event) => {
            this.removeAttribute("error");
            this.dispatchEvent(new CustomEvent("trigger_call", {
                detail: this.#dom.phoneNumberField.value
            }));
        }

        this.shadowRoot.querySelector('#number-field-wrapper span').onclick = () => {
            this.dispatchEvent(new CustomEvent("show_calls"));
        }

        this.#dom.phoneNumberField.addEventListener("click", (e) => {
            this.#current_pointer_pos = e.target.selectionStart;
            this.#keyboard.setCaretPosition(e.target.selectionStart);
        });

        let Keyboard = window.SimpleKeyboard.default;

        this.#keyboard = new Keyboard(dialButtons, {
            onChange: input => this.#on_change(input),
            onKeyPress: button => this.#on_key_press(button),
            layout: {
                default: ["1 2 3", "4 5 6", "7 8 9", "+ 0 {bksp}"]
            },
            theme: "hg-theme-default ov_sip_keyboard numeric-theme",
            display: {
                "{bksp}": '<img src="/images/fluent-ui-system-icons/backspace.svg">'
            },
            preventMouseDownDefault: false
        });
    }

    #on_change(input) {
        this.#dom.phoneNumberField.value = input;
    }

    #on_key_press(button) {
        if (this.#current_pointer_pos !== undefined)
            if (button === "{bksp}") {
                this.#keyboard.setCaretPosition(this.#current_pointer_pos);
                if (this.#current_pointer_pos > 0)
                    this.#current_pointer_pos--;
            } else
                this.#keyboard.setCaretPosition(this.#current_pointer_pos++);
    }

    clear_number() {
        this.#dom.phoneNumberField.value = "";
        this.#keyboard.clearInput();
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