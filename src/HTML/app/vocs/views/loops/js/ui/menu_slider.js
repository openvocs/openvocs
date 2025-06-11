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
    @file           menu_slider.js

    @ingroup        vocs

    @brief          handel display of settings slider with user options
    	
    ---------------------------------------------------------------------------
*/
import * as Loop_View from "./loop_view.js";

import * as ov_Websockets from "/lib/ov_websocket_list.js";
import * as ov_WebRTCs from "/lib/ov_media/ov_webrtc_list.js";
import * as ov_Auth from "/lib/ov_auth.js";

import * as ov_Web_Storage from "/lib/ov_utils/ov_web_storage.js";

import ov_Dialog from "/components/dialog/dialog.js";

const DOM = {
    CLASS: {
        server_name: "server_name",
        server_sync_level: "server_sync_level",

        prime_server: "prime_server",
        lead_server: "lead_server",

        working: "working",
        disconnected: "disconnected",
    }
};

export var logout_triggered = false;

export function init() {
    DOM.slider_button = document.getElementById("open_menu_button");

    DOM.slider = document.getElementById("menu_slider");
    DOM.role_select = document.getElementById("switch_role_select");
    DOM.server_list = document.getElementById("server_list");
    DOM.logout_button = document.getElementById("logout_button");

    DOM.loading_screen = document.querySelector("#loading_screen");

    DOM.TEMPLATE = {
        server: document.querySelector("#server_template")
    };

    DOM.slider_button.addEventListener("click", function () {
        if (DOM.slider.open)
            close_slider();
        else
            open_slider();
    });

    DOM.role_select.addEventListener("click", async function (event) {
        if (event.target.tagName === "INPUT") {
            await Loop_View.talk(false);
            for (let ws of ov_Websockets.list)
                ov_Web_Storage.add_role_to_session(ws.websocket_url, event.target.id);
            ov_Websockets.reload_page(); // reload page to auto-login in new role
        }
    });

    DOM.logout_button.addEventListener("click", async function () {
        await Loop_View.talk(false);
        try {
            DOM.loading_screen.show("Saving data and logging out...");
            ov_Auth.logout();
            logout_triggered = true;
            DOM.loading_screen.hide();
            console.log("(vc) logged out");
            //rest is handled in disconnect
        } catch (error) {
            console.warn("(vc) failed to log out, error:", error);
        }
    });

    document.querySelector("#page_reload_button").addEventListener("click", () => {
        ov_Websockets.reload_page();
    });

    ov_Websockets.addEventListener("server_switch", (event) => {
        console.log("sever switch", event);
    });

    draw_server();
}

var interval_id;

export function open_slider() {
    update_server_status();
    redraw_lead_server(ov_Websockets.current_lead_websocket.client_id);
    DOM.slider.show();
    interval_id = setInterval(update_server_status, 500);

    function update_server_status() {
        for (let ws of ov_Websockets.list)
            redraw_server_status(ws);
    }
}

function close_slider() {
    clearInterval(interval_id);
    DOM.slider.hide();
}

export function redraw_user() {
    if (ov_Websockets.user())
        DOM.slider.value = ov_Websockets.user().name;
}

export async function redraw_roles() {
    DOM.role_select.clear();
    if (ov_Websockets.user()) {
        for (let role of ov_Websockets.user().roles.values) {
            let name = role.name;
            if (!name)
                name = role.abbreviation;
            if (!name)
                name = role.id;
            // await DOM.role_select.add_item(role.id, name + "\n (" + role.project + ")", role.id);
            await DOM.role_select.add_item(role.id, name, role.id);
        }
        DOM.role_select.value = ov_Websockets.user().role;
    }
}

function draw_server() {
    for (let ws of ov_Websockets.list) {
        DOM.server_list.appendChild(document.importNode(DOM.TEMPLATE.server.content, true));
        let server_element = DOM.server_list.lastElementChild;

        server_element.id = "server-" + ws.client_id;

        server_element.querySelector("." + DOM.CLASS.server_name).innerHTML = ws.server_name;
        if (ov_Websockets.prime_websocket === ws.client_id)
            server_element.classList.add(DOM.CLASS.prime_server);
        if (ov_Websockets.current_lead_websocket === ws.client_id)
            server_element.classList.add(DOM.CLASS.lead_server);

        server_element.addEventListener("click", () => {
            if (!server_element.classList.contains(DOM.CLASS.lead_server)) {
                switch_server(ws);
            }
        });
    }
}

export function switch_server(websocket) {
    if (ov_Websockets.current_lead_websocket === websocket) {
        console.log("Server already is lead. Switching not necessary.");
    } else if (ov_Websockets.list.includes(websocket)) {
        DOM.loading_screen.show("switching server...");
        let media = ov_WebRTCs.get_lead();
        if (media)
            media.pause_playback();
        if (SHARED_MULTICAST_ADDRESSES && ov_WebRTCs.local_stream() && !ov_WebRTCs.local_stream().mute) {
            media.mute_outbound_stream(true);
        }

        ov_Websockets.switch_lead_websocket(websocket);

        media = ov_WebRTCs.get_lead();
        media.start_playback();
        if (SHARED_MULTICAST_ADDRESSES && ov_WebRTCs.local_stream() && !ov_WebRTCs.local_stream().mute) {
            media.mute_outbound_stream(false);
        }

        console.log("Switch lead server to " + ov_Websockets.current_lead_websocket.server_name);

        Loop_View.draw();

        DOM.loading_screen.hide();
        redraw_lead_server(websocket.client_id);

    } else {
        console.warn("Can not switch lead server. Couldn't find websocket.");
    }
}

function redraw_lead_server(new_lead) {
    let lead_server = DOM.server_list.querySelectorAll("." + DOM.CLASS.lead_server);
    for (let server of lead_server) {
        server.classList.remove(DOM.CLASS.lead_server, "dark");
    }
    DOM.server_list.querySelector("#server-" + new_lead).classList.add(DOM.CLASS.lead_server, "dark");
}

function redraw_server_status(ws) {
    let server_element = document.getElementById("server-" + ws.client_id);
    if (!ws.authorized || (ws.error && !ws.error.temp_error)) { // disconnected
        server_element.classList.remove(DOM.CLASS.working);
        server_element.classList.add(DOM.CLASS.disconnected);
        server_element.disabled = true;
    } else if (ws.error && ws.error.temp_error) { // working
        server_element.classList.add(DOM.CLASS.working);
        server_element.classList.remove(DOM.CLASS.disconnected);
        server_element.disabled = false;
    } else { // running
        server_element.classList.remove(DOM.CLASS.working);
        server_element.classList.remove(DOM.CLASS.disconnected);
        server_element.disabled = false;
    }
}