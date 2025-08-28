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
    @file           view.js

    @ingroup        extensions/recorder/views/config

    @brief          manage DOM objects for recorder config view
    	
    ---------------------------------------------------------------------------
*/
// import custom HTML elements
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import ov_Recorder_Loop from "/extensions/recorder/components/config/loop/recorder_loop.js";
import * as ov_Recorder from "/extensions/recorder/ov_recorder.js";

var DOM = {};

export var logout_triggered;

export function init(view_id) {

    DOM.loops = document.getElementById("recorder_loops");
    DOM.start_recording = document.getElementById("start_recorder");
    DOM.stop_recording = document.getElementById("stop_recorder");
    DOM.start_recording.disabled = true;
    DOM.stop_recording.disabled = true;

    DOM.start_recording.addEventListener("click", () => {
        for (let ws of ov_Websockets.list) {
            if (ws.port === "admin" && ws.record === true) {
                let loop = get_current_loop();
                ov_Recorder.start_record(loop.id, ws);
                loop.active = true;
                DOM.start_recording.disabled = true;
                DOM.stop_recording.disabled = false;
                break;
            }
        }
    });

    DOM.stop_recording.addEventListener("click", () => {
        for (let ws of ov_Websockets.list) {
            if (ws.port === "admin" && ws.record === true) {
                let loop = get_current_loop();
                ov_Recorder.stop_record(loop.id, ws);
                loop.active = false;
                DOM.start_recording.disabled = false;
                DOM.stop_recording.disabled = true;
                break;
            }
        }
    });
}

export function add_loop(id, data, active) {
    let loop = document.createElement("ov-recorder-config-loop");

    loop.id = id;
    loop.name = data.name ? data.name : data.id;
    loop.active = active;

    loop.addEventListener("click", () => {
        this.select_loop(loop);
    });
    DOM.loops.appendChild(loop);

    return loop;
}

export function clear_loops() {
    DOM.loops.replaceChildren();
    DOM.start_recording.disabled = true;
    DOM.stop_recording.disabled = true;
}

function get_current_loop() {
    let loops = document.querySelectorAll("ov-recorder-config-loop");
    for (let element of loops) {
        if (element.disabled)
            return element;
    }
}

export function select_loop(loop) {
    let prev_loop = get_current_loop();
    if (prev_loop)
        prev_loop.disabled = false;

    loop.disabled = true;
    DOM.start_recording.disabled = loop.active;
    DOM.stop_recording.disabled = !loop.active;
}