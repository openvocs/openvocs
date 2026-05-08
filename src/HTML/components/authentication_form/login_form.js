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
    @file           login_form.js

    @ingroup        components/authentication_form

    @brief          custom web component

    ---------------------------------------------------------------------------
*/

import * as CSS from "/css/css.js";

export default class ov_Login_Form extends HTMLElement {
    #dom;
    #current_input;
    #login_keyboard;

    constructor() {
        super();
        this.#dom = {
            class: {
                loading: "loading"
            }
        }
    }

    attributeChangedCallback(name, oldValue, newValue) {
    }

    async connectedCallback() {
        this.appendChild((await ov_Login_Form.template).content.cloneNode(true));

        this.#dom.login_form = document.getElementById("login_form");
        this.#dom.user_field = document.getElementById("username");
        this.#dom.password_field = document.getElementById("password");
        this.#dom.login_button = document.getElementById("login_button");

        this.#check_form();

        this.#dom.login_form.onsubmit = (event) => {
            event.preventDefault();
            this.#process_login();
            return false;
        };

        this.#dom.user_field.onkeyup = () => this.#check_form();
        this.#dom.user_field.onchange = () => this.#check_form();
        this.#dom.user_field.addEventListener("value_changed", () => this.#check_form());
        this.#dom.password_field.onkeyup = () => this.#check_form();
        this.#dom.password_field.onchange = () => this.#check_form();
        this.#dom.password_field.addEventListener("value_changed", () => this.#check_form());

        if (SCREEN_KEYBOARD && window.SimpleKeyboard && matchMedia("(width > 480px)").matches) {
            const Keyboard = window.SimpleKeyboard.default;
            this.#login_keyboard = new Keyboard({
                onChange: input => this.#on_change(input),
                onKeyPress: button => this.#on_key_press(button),
                onKeyReleased: button => this.#on_key_released(button),
                theme: "hg-theme-default ov_dark_keyboard",
                layout: {
                    'default': [
                        '` 1 2 3 4 5 6 7 8 9 0 - = {bksp}',
                        '€ q w e r t y u i o p [ ] \\',
                        '@ a s d f g h j k l ; \' {enter}',
                        '{shift} z x c v b n m , . / {shift}'
                    ],
                    'shift': [
                        '~ ! # $ % ^ & * ( ) _ + {bksp}',
                        '€ Q W E R T Y U I O P { } |',
                        '@ A S D F G H J K L : " {enter}',
                        '{shift} Z X C V B N M < > ? {shift}'
                    ]
                },
                display: {
                    '{enter}': '<img src="/images/fluent-ui-system-icons/arrow-enter-left.svg">',
                    "{shift}": '<img src="/images/fluent-ui-system-icons/keyboard-shift.svg">',
                    "{bksp}": '<img src="/images/fluent-ui-system-icons/backspace.svg">'
                }
            });
        }

        this.#dom.user_field.addEventListener("click", () => {
            this.#current_input = this.#dom.user_field;
            if (this.#login_keyboard)
                this.#login_keyboard.setOptions({
                    inputName: this.#current_input.id
                });
        });
        this.#dom.password_field.addEventListener("click", () => {
            this.#current_input = this.#dom.password_field;
            if (this.#login_keyboard)
                this.#login_keyboard.setOptions({
                    inputName: this.#current_input.id
                });
        });
        this.#dom.user_field.click();

        this.#dom.show_password = document.getElementById("show_password");
        this.#dom.show_password.addEventListener("touchstart", () => {
            this.#dom.password_field.type = "text";
        });

        this.#dom.show_password.addEventListener("touchend", () => {
            this.#dom.password_field.type = "password";
        });

        this.#dom.show_password.addEventListener("mousedown", () => {
            this.#dom.password_field.type = "text";
        });

        this.#dom.show_password.addEventListener("mouseup", () => {
            this.#dom.password_field.type = "password";
        });
    }

    async #process_login() {
        this.#indicate_loading(true);
        this.dispatchEvent(new CustomEvent("login_triggered", {
            detail: { username: this.#dom.user_field.value, password: this.#dom.password_field.value }
        }));
    }

    #on_change(input) {
        this.#current_input.value = input;
        this.#current_input.dispatchEvent(new CustomEvent("value_changed"));
    }

    #on_key_press(button) {
        if (button === "{shift}") {
            let currentLayout = this.#login_keyboard.options.layoutName;
            let shiftToggle;
            if (currentLayout === "default") {
                shiftToggle = "shift";
                this.#login_keyboard.addButtonTheme("{shift}", "active_button");
            } else {
                shiftToggle = "default";
                this.#login_keyboard.removeButtonTheme("{shift}", "active_button");
            }

            this.#login_keyboard.setOptions({
                layoutName: shiftToggle
            });
        } else if (button === "{enter}") {
            this.#login_keyboard.addButtonTheme("{enter}", "active_button");
            if (this.#dom.login_button.disabled) {
                if (this.#current_input === this.#dom.password_field)
                    this.#dom.user_field.click();
                else
                    this.#dom.password_field.click();
            }
            else
                this.#dom.login_button.click();
        }
    }

    #on_key_released(button) {
        if (button !== "{shift}" && button !== "{enter}" && button !== "{bksp}" && this.#login_keyboard.options.layoutName === "shift") {
            this.#login_keyboard.setOptions({
                layoutName: "default"
            });
            this.#login_keyboard.removeButtonTheme("{shift}", "active_button");
        }
        this.#login_keyboard.removeButtonTheme("{enter}", "active_button");
    }

    #check_form() {
        this.#dom.login_button.disabled = !(this.#dom.user_field.value && this.#dom.password_field.value);
    }

    clear_password_field() {
        this.#dom.password_field.value = "";
        if (this.#login_keyboard)
            this.#login_keyboard.clearInput(this.#dom.password_field.id);
        this.#check_form();
    }

    #indicate_loading(value) {
        if (value === true)
            this.#dom.login_button.classList.add(this.#dom.class.loading);
        else
            this.#dom.login_button.classList.remove(this.#dom.class.loading);
    }

    stop_loading_animation() {
        this.#indicate_loading(false);
    }

    static style_sheet = CSS.fetch_style_sheet("/components/authentication_form/style.css");
    static template = async function () {
        const response = await fetch('/components/authentication_form/login_form.html');
        let dom = new DOMParser().parseFromString(await response.text(), 'text/html');
        return dom.querySelector('template');
    }();
}

customElements.define('ov-login-form', ov_Login_Form);