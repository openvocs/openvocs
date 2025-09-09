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
    @file           settings_slider.js

    @ingroup        vocs

    @brief          handel display of settings slider with admin options
    	
    ---------------------------------------------------------------------------
*/
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import * as ov_Vocs from "/lib/ov_vocs.js";

import ov_Audio from "/lib/ov_media/ov_audio.js";

import * as PTT_Bar from "./ptt_bar.js";
import * as Loop_View from "./loop_view.js";

import ov_Dialog from "/components/dialog/dialog.js";

const DOM = {};

var mic_vis;

export function init() {
    DOM.menu_button = document.getElementById("open_menu_button");
    DOM.slider_button = document.getElementById("open_settings_button");

    DOM.slider = document.getElementById("settings_slider");

    DOM.audio_details = DOM.slider.querySelector("#audio_options");
    DOM.toggle_audio_test = DOM.slider.querySelector("#speaker_test");
    DOM.test_audio = DOM.slider.querySelector("#speaker_test_audio");
    DOM.mic_oscillation = DOM.slider.querySelector("#microphone_test").querySelector("canvas");

    DOM.page_zoom_factor = document.getElementById("page_zoom_input");

    DOM.multi_talk_checkbox = document.getElementById("multi_talk_checkbox");
    DOM.mute_hardware_checkbox = document.getElementById("hardware_checkbox");
    DOM.mute_browser_options = document.getElementById("ptt_mute_options");
    DOM.ptt_checkbox = document.getElementById("ptt_checkbox");
    DOM.fptt_checkbox = document.getElementById("fullscreen_checkbox");
    DOM.mute_key_checkbox = document.getElementById("key_checkbox");
    DOM.mute_mousewheel_checkbox = document.getElementById("mousewheel_checkbox");
    DOM.add_HID = document.getElementById("add_HID");

    DOM.activity_content = document.getElementById("activity_content");

    DOM.color_mode_select = document.getElementById("color_mode_select");

    hide_slider();

    DOM.slider_button.addEventListener("click", function () {
        console.log(DOM.slider, DOM.slider.open)
        if (!DOM.slider.open) {
            DOM.page_zoom_factor.value = SITE_SCALING_FACTOR;
            if (DOM.audio_details.open)
                start_mic_oscillation();
            DOM.slider.show();
        } else
            hide_slider();
    });
    DOM.menu_button.addEventListener("click", hide_slider);

    document.getElementById("version_number").innerText = VERSION_NUMBER;

    //-------------------------------------------------------------------------
    // audio options
    //-------------------------------------------------------------------------
    DOM.audio_details.addEventListener("toggle", () => {
        if (DOM.audio_details.open)
            start_mic_oscillation();
        else
            mic_vis.stop();
    });

    DOM.toggle_audio_test.addEventListener("click", function () {
        if (DOM.test_audio.paused)
            DOM.test_audio.play();
        else
            DOM.test_audio.pause();
    });

    //-------------------------------------------------------------------------
    // ui options
    //-------------------------------------------------------------------------
    // page color -------------------------------------------------------------
    DOM.color_mode_select.value =
        document.querySelector("option[label=" + COLOR_MODE + "]").value;
    DOM.color_mode_select.addEventListener("change", function () {
        change_color_mode(DOM.color_mode_select.value,
            DOM.color_mode_select.options[DOM.color_mode_select.selectedIndex].label);
    });
    change_color_mode(DOM.color_mode_select.value, COLOR_MODE);

    // activity indicator -----------------------------------------------------
    DOM.activity_content.value = ACTIVITY_CONTENT;
    DOM.activity_content.addEventListener("change", function () {
        ACTIVITY_CONTENT = DOM.activity_content.value;
        for (let page of Loop_View.pages)
            for (let loop of page.values) {
                loop.update_active_participants_list();
            }
    });

    //-------------------------------------------------------------------------
    // advanced options
    //-------------------------------------------------------------------------
    DOM.page_zoom_factor.addEventListener("change", async function () {
        Loop_View.scale_page(DOM.page_zoom_factor.value);
        //Loop_View.resize();
    });

    // ptt options ------------------------------------------------------------
    DOM.mute_hardware_checkbox.checked = SECURE_VOICE_PTT;
    DOM.mute_hardware_checkbox.addEventListener("change", function () {
        let checked = DOM.mute_hardware_checkbox.checked;
        PTT_Bar.enable_soft_PTT_button(!checked);
        if (checked) {
            DOM.mute_browser_options.classList.add("removed");
            PTT_Bar.enable_mouse_middle_click(false);
            PTT_Bar.enable_key_press(false);
            PTT_Bar.enable_PTT_button(false);
            PTT_Bar.enable_FPTT_Button(false);
        } else {
            DOM.mute_browser_options.classList.remove("removed");
            PTT_Bar.enable_mouse_middle_click(DOM.mute_mousewheel_checkbox.checked);
            PTT_Bar.enable_key_press(DOM.mute_key_checkbox.checked);
            PTT_Bar.enable_PTT_button(DOM.ptt_checkbox.checked);
            PTT_Bar.enable_FPTT_Button(DOM.fptt_checkbox.checked);
        }
    });

    DOM.fptt_checkbox.checked = MUTE_FULLSCREEN_BUTTON;
    DOM.fptt_checkbox.addEventListener("change", function () {
        PTT_Bar.enable_FPTT_Button(DOM.fptt_checkbox.checked);
    });

    DOM.ptt_checkbox.checked = PTT;
    DOM.ptt_checkbox.addEventListener("change", function () {
        PTT_Bar.enable_PTT_button(DOM.ptt_checkbox.checked);
    });

    DOM.mute_mousewheel_checkbox.checked = MUTE_ON_MOUSE_MIDDLE_CLICK;
    DOM.mute_mousewheel_checkbox.addEventListener("change", function () {
        PTT_Bar.enable_mouse_middle_click(DOM.mute_mousewheel_checkbox.checked);
    });

    document.querySelector("#key_checkbox_label").innerHTML = MUTE_KEY_NAME;
    DOM.mute_key_checkbox.checked = MUTE_KEY;
    DOM.mute_key_checkbox.addEventListener("change", function () {
        PTT_Bar.enable_key_press(DOM.mute_key_checkbox.checked);
    });

    if (!navigator.hid)
        DOM.add_HID.disabled = true;
    DOM.add_HID.addEventListener("click", function () {
        navigator.hid.requestDevice({ filters: [] }).then((devices) => {
            window.dispatchEvent(new CustomEvent("new_hid", {
                detail: devices[0]
            }));
        });
    });

    if (SECURE_VOICE_PTT)
        DOM.mute_browser_options.classList.add("removed");
    else
        DOM.mute_browser_options.classList.remove("removed");

    const urlParams = new URLSearchParams(window.location.search);
    const keyset = urlParams.get('keysetname')
    document.getElementById("keyset_name").innerText = keyset;
}

function hide_slider() {
    if (DOM.slider.open) {
        if (DOM.audio_details.open && mic_vis)
            mic_vis.stop();
        DOM.slider.hide();
    }
}

function open_slider() {

}

function start_mic_oscillation() {
    try {
        ov_Audio.media_stream_from_microphone().then((audio) => {
            mic_vis = audio.create_visualization(DOM.mic_oscillation);
        });
    } catch (error) {
        console.warn("Can't display microphone visualization.");
    }
}

function change_color_mode(value, label) {
    for (let option of DOM.color_mode_select.options) {
        if (option.value === value)
            document.documentElement.classList.add(option.value);
        else
            document.documentElement.classList.remove(option.value);
    }
}