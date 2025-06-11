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
    @file           ptt_view.js

    @ingroup        vocs

    @brief          handel and display ptt view

    ---------------------------------------------------------------------------
*/
import * as ov_WebRTCs from "/lib/ov_media/ov_webrtc_list.js";
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import * as ov_Auth from "/lib/ov_auth.js";

import * as Loop_View from "./loop_view.js";

export const DOM = {
    CLASS: {
        ptt_active: "active"
    }
};

const KEYBOARD_ESCAPE = "Escape";
const MOUSE_MIDDLE_CLICK = 1;

var mouse_middle_click = MUTE_ON_MOUSE_MIDDLE_CLICK;
var key_press = MUTE_KEY;
var use_ptt = PTT;
var in_form = false;
var soft_mic_active = true;
var audio_output_indicator;

export function init() {
    DOM.fptt_wrapper = document.getElementById("menu_fullscreen_option");
    DOM.fptt_area = document.getElementById("fptt_area");
    DOM.fptt_checkbox = document.getElementById("menu_fullscreen_checkbox");
    DOM.ptt_button = document.getElementById("push_to_talk_button");
    DOM.mute_button = document.getElementById("toggle_to_talk_button");
    DOM.audio_output_indicator = document.getElementById("audio_input_visualization");
    DOM.loop_pagination = document.getElementById("pagination");
    DOM.menu_warning = document.getElementById("menu_warnings");
    DOM.login_form = document.querySelector("#login_dialog ov-login-form");
    DOM.login_message = document.querySelector("#login_dialog>div:last-child");

    // PTT --------------------------------------------------------------------
    DOM.ptt_button.addEventListener("mousedown", function () {
        activate_mic_soft();
    });

    DOM.ptt_button.addEventListener("mouseup", function () {
        release_mic_soft();
    });

    DOM.ptt_button.addEventListener("mouseout", function () {
        // deactivate PTT when user moves out of the element 
        if (DOM.ptt_button.classList.contains(DOM.CLASS.ptt_active))
            release_mic_soft();
    });

    DOM.ptt_button.addEventListener("touchstart", function () {
        activate_mic_soft();
    });

    DOM.ptt_button.addEventListener("touchend", function () {
        release_mic_soft();
    });

    // mute -------------------------------------------------------------------
    DOM.mute_button.addEventListener("click", function () {
        if (DOM.mute_button.classList.contains(DOM.CLASS.ptt_active))
            release_mic_soft();
        else
            activate_mic_soft();
    });

    // FPTT -------------------------------------------------------------------
    DOM.fptt_area.classList.add("hidden");

    DOM.fptt_area.addEventListener("mousedown", function () {
        button_press_down();
    });

    DOM.fptt_area.addEventListener("mouseup", function () {
        button_release();
    });

    DOM.fptt_area.addEventListener("touchstart", function (event) {
        button_press_down();
        event.preventDefault()
    });

    DOM.fptt_area.addEventListener("touchend", function (event) {
        button_release();
        event.preventDefault();
    });

    document.addEventListener("keydown", function (event) {
        if (event.key === KEYBOARD_ESCAPE) {
            DOM.fptt_area.classList.add("hidden");
            release_mic_soft(DOM.fptt_area);
            DOM.fptt_checkbox.checked = false;
        }
    }, true);

    DOM.fptt_checkbox.addEventListener("change", function () {
        if (DOM.fptt_checkbox.checked) {
            DOM.fptt_area.classList.remove("hidden");
        } else {
            DOM.fptt_area.classList.add("hidden");
            release_mic_soft(DOM.fptt_area);
        }
    });

    // mute/PTT on key press --------------------------------------------------
    document.addEventListener("keydown", function (event) {
        if (key_press && !in_form && event.key === MUTE_KEY_DEF) {
            button_press_down();
            event.preventDefault();
            return false; // prevent scroll down
        }
    }, true);

    document.addEventListener("keyup", function (event) {
        if (key_press && !in_form && event.key === MUTE_KEY_DEF) {
            button_release();
            event.preventDefault();
            return false; // prevent scroll down
        }
    });

    // deactivate Shortcuts in forms
    for (let form of document.querySelectorAll("form")) {
        form.addEventListener("focusin", function () {
            in_form = true;
        });

        form.addEventListener("focusout", function () {
            in_form = false;
        });
    }

    // mute/PTT on middle mouse -----------------------------------------------
    document.addEventListener("mousedown", function (mouseEvent) {
        if (mouse_middle_click && mouseEvent.button === MOUSE_MIDDLE_CLICK)
            button_press_down();
    });

    document.addEventListener("mouseup", function (mouseEvent) {
        if (mouse_middle_click && mouseEvent.button === MOUSE_MIDDLE_CLICK)
            button_release();
    });

    // mute/PTT with Gamepad API ----------------------------------------------
    console.log("Navigator supports Gamepads: " + !!(navigator.getGamepads));
    if (!!navigator.getGamepads) {
        let gamepad_loops = new Array(4); //it's possible to connect 4 Gamepads at the same time
        navigator.getGamepads().forEach((device) => {
            if (device)
                connect_gamepad(device, device.index);
        });

        window.addEventListener("gamepadconnected", (event) => {
            connect_gamepad(event.gamepad, event.gamepad.index);
        });

        window.addEventListener("gamepaddisconnected", (event) => {
            clearInterval(gamepad_loops[event.gamepad.index]);
            console.log("Gamepad at index %d: %s disconnected.", event.gamepad.index, event.gamepad.id);
        });

        function connect_gamepad(gamepad, index) {
            console.log(
                "Gamepad connected at index %d: %s. %d buttons, %d axes.",
                gamepad.index,
                gamepad.id,
                gamepad.buttons.length,
                gamepad.axes.length,
            );

            let saved_button_pressed = false;

            gamepad_loops[index] = setInterval(function () {
                let current_button_pressed = navigator.getGamepads()[index].buttons[0].pressed;
                if (current_button_pressed !== saved_button_pressed) {
                    if (current_button_pressed)
                        button_press_down();
                    else
                        button_release()
                    saved_button_pressed = current_button_pressed;
                }
            }, 100);
        }
    }

    // mute/PTT with WebHDI API -----------------------------------------------
    console.log("Navigator supports HID Devices: " + !!(navigator.hid));
    if (!!navigator.hid) {

        window.addEventListener("new_hid", (device) => {
            connect_hid_device(device.detail);
        });

        // connect all registered HID devices
        navigator.hid.getDevices().then((devices) => {
            devices.forEach((device) => {
                connect_hid_device(device);
            });
        });

        navigator.hid.addEventListener("connect", (event) => {
            connect_hid_device(event.device);
        });

        navigator.hid.addEventListener("disconnect", (event) => {
            console.log("HID disconnected: " + event.device.productName, event);
        });

        async function connect_hid_device(device) {
            // Handle connect devices of Imtradex
            if (device.productId !== 256 && device.vendorId !== 8886)
                return;
            try {
                await device.open();
                device.addEventListener("inputreport", (event) => {
                    const value = event.data.getUint8(0);
                    if (value === 32)
                        button_press_down();
                    else
                        button_release();
                });
                console.log("HID connected: " + device.productName);
            } catch (error) {
                console.error("HID connection failed: ", error);
            }
        }
    }

    // stream activity --------------------------------------------------------
    if (ov_WebRTCs.local_stream()) {
        audio_output_indicator = ov_WebRTCs.local_stream().create_visualization(DOM.audio_output_indicator, "rgb(127, 127, 127)");

        ov_WebRTCs.local_stream().on_activity_detection = async function (stream_activity) {
            if (stream_activity) {
                DOM.ptt_button.classList.add(DOM.CLASS.ptt_active, "dark");
                DOM.mute_button.classList.add(DOM.CLASS.ptt_active, "dark");
                DOM.fptt_area.classList.add(DOM.CLASS.ptt_active, "dark");
                console.log("(vc) activate audio transfer");
            } else {
                DOM.ptt_button.classList.remove(DOM.CLASS.ptt_active, "dark");
                DOM.mute_button.classList.remove(DOM.CLASS.ptt_active, "dark");
                DOM.fptt_area.classList.remove(DOM.CLASS.ptt_active, "dark");
                console.log("(vc) deactivate audio transfer");
            }
            Loop_View.talk(stream_activity);
        };
    }

    // configure Mute/PTT behavior --------------------------------------------
    if (!SECURE_VOICE_PTT) {
        if (ov_WebRTCs.local_stream())
            ov_WebRTCs.mute_outbound_stream(true, true);
        enable_FPTT_Button(MUTE_FULLSCREEN_BUTTON);
        enable_PTT_button(PTT);
        enable_mouse_middle_click(MUTE_ON_MOUSE_MIDDLE_CLICK);
        enable_key_press(MUTE_KEY);
    } else {
        enable_FPTT_Button(false);
        enable_PTT_button(true); // display PTT button not mute button
        enable_mouse_middle_click(false);
        enable_key_press(false);
    }
    enable_soft_PTT_button(!SECURE_VOICE_PTT);

    let username = DOM.login_form.querySelector("#username");
    username.value = ov_Websockets.user().id;
    username.disabled = true;

    DOM.login_dialog = document.getElementById("login_dialog");

    document.getElementById("open_login").addEventListener("click", () => {
        DOM.login_dialog.show();
    });
}

async function login(username, password, websocket) {
    let result = await ov_Auth.login(username, password, websocket);
    if (result)
        result = await ov_Auth.collect_roles(websocket);
    if (result && ov_Websockets.user().role)
        result = await ov_Auth.authorize_role(ov_Websockets.user().role, websocket)
    return result;
}

export async function ask_for_relogin() {
    return new Promise((resolve) => {

        DOM.menu_warning.style.display = "initial";

        let handle_login = async function (event) {

            DOM.login_form.removeEventListener("login_triggered", handle_login);

            let promises = [];
            for (let ws of ov_Websockets.list)
                if (!ov_Auth.has_valid_session(ws))
                    promises.push(login(event.detail.username, event.detail.password, ws));


            let result = await Promise.all(promises);

            DOM.login_form.clear_password_field();
            DOM.login_form.stop_loading_animation();

            if (!result.includes(false)) {
                DOM.login_dialog.hide();
                DOM.menu_warning.style.display = "none";
                resolve(true);
            } else {
                let error = ov_Websockets.prime_websocket.server_error;
                let error_code = error ? error.code : undefined;
                if (error_code === 5000)
                    DOM.login_message.innerText = "You have entered an invalid username or password.";
                else if (error_code === undefined)
                    DOM.login_message.innerText = "Connection to server(s) lost. Please wait.";
                else
                    DOM.login_message.innerText = "Error: " + error.description + " (Code: " + error.code + ")";
                resolve(false);
            }
        }
        DOM.login_form.addEventListener("login_triggered", handle_login);
    });
}

function button_press_down() {
    if (use_ptt)
        activate_mic_soft();
}

function button_release() {
    if (use_ptt) {
        release_mic_soft();
    } else {
        if (DOM.mute_button.classList.contains(DOM.CLASS.ptt_active))
            release_mic_soft();
        else
            activate_mic_soft();
    }
}

function activate_mic_soft() {
    if (!soft_mic_active) {
        soft_mic_active = true;
        if (ov_WebRTCs.local_stream())
            ov_WebRTCs.mute_outbound_stream(false);
        DOM.ptt_button.classList.add(DOM.CLASS.ptt_active, "dark");
        DOM.mute_button.classList.add(DOM.CLASS.ptt_active, "dark");
        DOM.fptt_area.classList.add(DOM.CLASS.ptt_active, "dark");
        console.log("(vc) activate audio transfer");
        audio_output_indicator.waveform_color = "rgb(23, 142, 53)";
        Loop_View.talk(soft_mic_active);
    }
}

function release_mic_soft() {
    if (soft_mic_active) {
        soft_mic_active = false;
        if (ov_WebRTCs.local_stream())
            ov_WebRTCs.mute_outbound_stream(true);
        DOM.ptt_button.classList.remove(DOM.CLASS.ptt_active, "dark");
        DOM.mute_button.classList.remove(DOM.CLASS.ptt_active, "dark");
        DOM.fptt_area.classList.remove(DOM.CLASS.ptt_active, "dark");
        console.log("(vc) deactivate audio transfer");
        if (audio_output_indicator)
            audio_output_indicator.waveform_color = "rgb(127, 127, 127)";
        Loop_View.talk(soft_mic_active);
    }
}

export function enable_PTT_button(value) {
    if (!value) {
        use_ptt = false;
        DOM.ptt_button.classList.add("removed");
        DOM.mute_button.classList.remove("removed");
    } else {
        use_ptt = true;
        DOM.mute_button.classList.add("removed");
        DOM.ptt_button.classList.remove("removed");
        release_mic_soft();
    }
}

export function enable_FPTT_Button(value) {
    if (!value)
        DOM.fptt_wrapper.classList.add("removed");
    else {
        DOM.fptt_wrapper.classList.remove("removed");
        DOM.fptt_checkbox.checked = false;
    }
}

export function enable_mouse_middle_click(value) {
    mouse_middle_click = value;
}

export function enable_key_press(value) {
    key_press = value;
}

export function enable_soft_PTT_button(value) {
    DOM.ptt_button.disabled = !value;
    DOM.mute_button.disabled = !value;
    if (value) {
        release_mic_soft();
        if (ov_WebRTCs.local_stream())
            ov_WebRTCs.local_stream().stop_activity_analyses();
    } else {
        if (ov_WebRTCs.local_stream()) {
            ov_WebRTCs.mute_outbound_stream(false);
            ov_WebRTCs.local_stream().start_activity_analyses();
        }
    }
}

export async function setup_pages() {
    let number = Loop_View.pages.length;
    DOM.loop_pagination.clear();
    for (let i = 0; i < number; i++)
        await DOM.loop_pagination.add_item("page_" + i, i + 1, i);


    DOM.loop_pagination.onchange = () => {
        if (DOM.loop_pagination.value !== undefined)
            Loop_View.show_page(DOM.loop_pagination.value);
    };

    let page = await Loop_View.show_page();
    DOM.loop_pagination.value = page;
}