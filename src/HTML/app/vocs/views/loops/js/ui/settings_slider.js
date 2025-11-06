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
import * as ov_WebRTCs from "/lib/ov_media/ov_webrtc_list.js";

import * as PTT_Bar from "./ptt_bar.js";
import * as Loop_View from "./loop_view.js";

import ov_Dialog from "/components/dialog/dialog.js";

const DOM = {};

var mic_vis;

export async function init() {
    DOM.menu_button = document.getElementById("open_menu_button");
    DOM.slider_button = document.getElementById("open_settings_button");

    DOM.slider = document.getElementById("settings_slider");

    DOM.audio_details = DOM.slider.querySelector("#audio_options");
    DOM.toggle_audio_test = DOM.slider.querySelector("#speaker_test");
    DOM.test_audio = DOM.slider.querySelector("#speaker_test_audio");
    DOM.mic_oscillation = DOM.slider.querySelector("#microphone_test").querySelector("canvas");
    DOM.mic_list = DOM.slider.querySelector("#microphone_list");
    DOM.speaker_list = DOM.slider.querySelector("#speaker_list");

    DOM.multi_talk_checkbox = document.getElementById("multi_talk_checkbox");
    DOM.mute_hardware_checkbox = document.getElementById("hardware_checkbox");
    DOM.mute_browser_options = document.getElementById("ptt_mute_options");
    DOM.ptt_checkbox = document.getElementById("ptt_checkbox");
    DOM.fptt_checkbox = document.getElementById("fullscreen_checkbox");
    DOM.mute_key_checkbox = document.getElementById("key_checkbox");
    DOM.mute_mousewheel_checkbox = document.getElementById("mousewheel_checkbox");
    DOM.add_HID = document.getElementById("add_HID");
    DOM.hid_list = document.getElementById("hid_devices");

    hide_slider();

    DOM.slider_button.addEventListener("click", function () {
        if (!DOM.slider.open) {
            if (DOM.audio_details.open)
                start_mic_oscillation();
            //update_device_lists();
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

    // navigator.mediaDevices.ondevicechange = async (event) => {
    //     console.log(event);
    //     update_device_lists();
    // };



    //-------------------------------------------------------------------------
    // ui options
    //-------------------------------------------------------------------------
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
        navigator.hid.requestDevice({ filters: HID_FILTERS }).then((devices) => {
            window.dispatchEvent(new CustomEvent("new_hid", {
                detail: devices[0]
            }));
            update_hid_device_list();
        });
    });
    update_hid_device_list();


    if (SECURE_VOICE_PTT)
        DOM.mute_browser_options.classList.add("removed");
    else
        DOM.mute_browser_options.classList.remove("removed");

    const urlParams = new URLSearchParams(window.location.search);
    const keyset = urlParams.get('keysetname')
    document.getElementById("keyset_name").innerText = keyset;
}

function update_hid_device_list() {
    if (navigator.hid)
        navigator.hid.getDevices().then((devices) => {
            const device_names = devices.map(device => {
                return device.productName || "Unknown Device";
            }).join(", ");
            DOM.hid_list.innerText = device_names ? device_names : "-";
        });
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

async function update_device_lists() {
    await ov_WebRTCs.find_audio_devices();
    DOM.mic_list.replaceChildren();
    for (let mic of ov_WebRTCs.microphones) {
        let element = document.createElement("input");
        element.type = "radio";
        element.id = "mic_" + mic.deviceId;
        element.value = mic.deviceId;
        element.name = "mic";
        let label = document.createElement("label");
        label.htmlFor = "mic_" + mic.deviceId;
        label.textContent = mic.label || `Microphone ${mic.deviceId}`;
        label.classList.add("button");

        DOM.mic_list.appendChild(element);
        DOM.mic_list.appendChild(label);

        if (ov_WebRTCs.selected_microphones.includes(mic.deviceId))
            element.checked = true;

        element.addEventListener("change", () => {
            console.log("mic", element.value);
            ov_WebRTCs.switch_local_media_stream(element.value);
            ov_WebRTCs.mute_outbound_stream(true);
        });
    }

    DOM.speaker_list.replaceChildren();
    for (let speaker of ov_WebRTCs.speakers) {
        let element = document.createElement("input");
        element.type = "radio";
        element.id = "speaker_" + speaker.deviceId;
        element.value = speaker.deviceId;
        element.name = "speaker";
        let label = document.createElement("label");
        label.htmlFor = "speaker_" + speaker.deviceId;
        label.textContent = speaker.label || `Speaker ${speaker.deviceId}`;
        label.classList.add("button");

        if (ov_WebRTCs.selected_speaker === speaker.deviceId)
            element.checked = true;

        DOM.speaker_list.appendChild(element);
        DOM.speaker_list.appendChild(label);
    }
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