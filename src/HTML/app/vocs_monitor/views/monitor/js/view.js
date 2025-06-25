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

    @ingroup        vocs_monitor

    @brief          init and load the monitor client view
    	
    ---------------------------------------------------------------------------
*/
//import * as ov_Auth from "/lib/ov_auth.js";
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import * as ov_Auth from "/lib/ov_auth.js";
import * as ov_Monitor from "/lib/ov_monitor.js";


import ov_Nav from "/components/nav/nav.js";
import ov_Dialog from "/components/dialog/dialog.js";

var VIEW_ID;

var Config_Recorder;

export var logout_triggered;

var DOM = {};
var selected_server;

export async function init(loops, view_id) {
    document.documentElement.className = "dark";

    if (RECORDER)
        Config_Recorder = await import("/extensions/recorder/views/config/js/recorder_config.js");

    if (!document.adoptedStyleSheets.includes(await ov_Nav.style_sheet))
        document.adoptedStyleSheets = [...document.adoptedStyleSheets, await ov_Nav.style_sheet];

    DOM.sub_view_nav = document.getElementById("select_subview");
    DOM.sub_view = document.getElementById("administration");
    DOM.config_name = document.getElementById("config_name");
    DOM.menu_slider = document.getElementById("menu_slider");
    DOM.menu_button = document.getElementById("menu_button");

    DOM.view_container = document.getElementById("view_container");
    DOM.reload_broadcast = document.getElementById("reload_broadcast");
    DOM.switch_server_id = document.getElementById("switch_server_id");
    DOM.switch_server_broadcast = document.getElementById("switch_server_broadcast");
    DOM.logout_button = document.getElementById("logout_button");
    DOM.loading_screen = document.getElementById("loading_screen");

    VIEW_ID = view_id;

    DOM.reload_broadcast.addEventListener("click", () => {
        display_loading_screen(true, "Sending update broadcast...");
        for (let ws of ov_Websockets.list)
            ov_Monitor.broadcast_update(ws);
        display_loading_screen(false);
    });
    DOM.switch_server_broadcast.disabled = true;
    DOM.switch_server_broadcast.addEventListener("click", () => {
        display_loading_screen(true, "Sending switch server broadcast...");
        for (let ws of ov_Websockets.list)
            ov_Monitor.broadcast_switch_server(selected_server.server_url, ws);
        display_loading_screen(false);
    });

    DOM.logout_button.addEventListener("click", () => {
        logout_triggered = true;
        ov_Auth.logout();
    });

    DOM.menu_button.addEventListener("click", () => {
        DOM.menu_slider.toggle();
    });


    for (let server of ov_Websockets.list) {
        console.log(server)
        let option = document.createElement("option");
        option.value = server.server_url;
        option.innerText = server.server_name;

        DOM.switch_server_id.appendChild(option);
        setInterval(() => {
            option.classList.toggle("off", !server.is_ready);
        }, 500);
    }

    DOM.switch_server_id.addEventListener("change", () => {
        selected_server = ov_Websockets.get_websocket(DOM.switch_server_id.value);
        if (DOM.switch_server_broadcast.disabled === true) {
            DOM.switch_server_broadcast.disabled = false;
            setInterval(() => {
                DOM.switch_server_id.classList.toggle("off", !selected_server.is_ready);
            }, 500);
        }
    });

    if (!RECORDER) {
        document.getElementById("recorder_page_button").style.display = "none";
        document.querySelector("#recorder_page_button+label").style.display = "none";
    }

    if (RECORDER)
        await Config_Recorder.init(document.getElementById("recorder_page"));

    DOM.sub_view_nav.addEventListener("change", () => {
        DOM.sub_view.className = DOM.sub_view_nav.value;
        if (DOM.sub_view_nav.value === "broadcast") {
        } else if (DOM.sub_view_nav.value === "recorder" && RECORDER) {
            Config_Recorder.render(loops);
        }
    });
    DOM.sub_view_nav.value = "broadcast";
}

export function display_loading_screen(value, message) {
    if (value)
        DOM.loading_screen.show(message);
    else
        DOM.loading_screen.delayed_hide();
}