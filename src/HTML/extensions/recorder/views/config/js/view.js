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
import ov_Player_List from '/extensions/recorder/components/player_list/recorder.js';

var DOM = {};

export var logout_triggered;

export function init(view_id) {

    DOM.loading_screen = document.getElementById("loading_screen");
    DOM.loops = document.getElementById("recorder_loops");
    DOM.start_recording = document.getElementById("start_stop_recorder");
    DOM.playback_search_start = document.getElementById("start_time");
    DOM.playback_search_stop = document.getElementById("stop_time");
    DOM.playback_search = document.getElementById("search_records");
    DOM.playback_list = document.querySelector('ov-player-list');
    DOM.message = document.getElementById("message");

    DOM.start_recording.addEventListener("click", async () => {
        if (DOM.start_recording.classList.contains("recording")) {
            for (let ws of ov_Websockets.list) {
                if (ws.record === true) {
                    let loop = get_current_loop();
                    if (await ov_Recorder.stop_record(loop.id, ws)) {
                        loop.active = false;
                        DOM.start_recording.classList.toggle("recording", false);
                        DOM.message.innerText = "Recording was stopped";
                        DOM.message.className = "success";
                        setTimeout(() => {
                            DOM.message.innerText = "";
                            DOM.message.className = "";
                        }, 30000);
                    } else {
                        DOM.message.innerText = "Error " + ws.server_error.code + ": " + ws.server_error.description;
                        DOM.message.className = "error";
                    }
                }
            }
        } else {
            for (let ws of ov_Websockets.list) {
                if (ws.record === true) {
                    let loop = get_current_loop();
                    if (await ov_Recorder.start_record(loop.id, ws)) {
                        loop.active = true;
                        DOM.start_recording.classList.toggle("recording", true);
                        DOM.message.innerText = "Recording was started";
                        DOM.message.className = "success";
                        setTimeout(() => {
                            DOM.message.innerText = "";
                            DOM.message.className = "";
                        }, 30000);
                    } else {
                        DOM.message.innerText = "Error " + ws.server_error.code + ": " + ws.server_error.description;
                        DOM.message.className = "error";
                    }
                }
            }
        }
    });

    const now = new Date();
    now.setMinutes(now.getMinutes() - now.getTimezoneOffset());
    DOM.playback_search_stop.value = now.toISOString().slice(0, 16);
    now.setTime(now.getTime() - 7 * 24 * 60 * 60 * 1000);
    DOM.playback_search_start.value = now.toISOString().slice(0, 16);

    DOM.playback_search.addEventListener("click", async () => {
        let start = Math.floor(new Date(DOM.playback_search_start.value).getTime() / 1000);
        let finish = Math.floor(new Date(DOM.playback_search_stop.value).getTime() / 1000);
        for (let ws of ov_Websockets.list) {
            if (ws.record === true) {
                let loop = get_current_loop();
                let recorded_loops = await ov_Recorder.get_recordings(loop.id, start, finish, ws);
                DOM.playback_list.draw_recordings(recorded_loops, ws.server_url + "audio/");
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
}

function get_current_loop() {
    let loops = document.querySelectorAll("ov-recorder-config-loop");
    for (let element of loops) {
        if (element.disabled)
            return element;
    }
}

export function select_loop(loop) {
    DOM.loading_screen.show("Load loop " + loop.id + "...");
    let prev_loop = get_current_loop();
    if (prev_loop)
        prev_loop.disabled = false;

    loop.disabled = true;
    DOM.start_recording.classList.toggle("recording", loop.active);
    // DOM.start_recording.disabled = loop.active;
    // DOM.stop_recording.disabled = !loop.active;
    DOM.playback_search.click();
    DOM.loading_screen.hide();
}