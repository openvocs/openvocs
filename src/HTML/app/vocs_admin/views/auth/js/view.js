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

    @ingroup        vocs_admin/views/auth

    @brief          manage DOM objects for login view
    	
    ---------------------------------------------------------------------------
*/

import * as ov_Auth from "/lib/ov_auth.js";
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import ov_Login_Form from "/components/authentication_form/login_form.js";
import ov_Branding from "/components/branding/branding.js";

const DOM = {};

export async function init(view_id) {
    if (!document.adoptedStyleSheets.includes(await ov_Login_Form.style_sheet))
        document.adoptedStyleSheets = [...document.adoptedStyleSheets, await ov_Login_Form.style_sheet];

    document.documentElement.className = "light";
    //-------------------------------------------------------------------------
    // DOM elements
    //-------------------------------------------------------------------------
    DOM.view_container = document.getElementById("view_container");
    DOM.login_form = document.querySelector("ov-login-form");
    DOM.message = document.getElementById("login_message");
    DOM.server_url = document.getElementById("server_address");

    //-------------------------------------------------------------------------
    // set view state
    //-------------------------------------------------------------------------
    set_server_id(window.location.host);

    DOM.login_form.addEventListener("login_triggered", async (event) => {
        let result = await ov_Auth.login(event.detail.username, event.detail.password);

        DOM.login_form.clear_password_field();
        DOM.login_form.stop_loading_animation();

        if (result) {
            DOM.view_container.dispatchEvent(new CustomEvent("switch_view", {
                detail: { origin: view_id, target: VIEW.OVERVIEW }
            }));
        } else {
            display_disconnect_notice(ov_Websockets.current_lead_websocket.server_error);
        }
    });

    document.getElementById("version").innerText = VERSION_NUMBER;
}

export function display_disconnect_notice(error) {
    let error_code = error ? error.code : undefined;
    if (error_code === 5000)
        set_message("You have entered an invalid username or password.");
    else if (error_code === undefined)
        set_message("Connection to server lost. Please reload page.");
    else
        set_message("Error: " + error.description + " (Code: " + error.code + ")");
}

function set_message(message) {
    DOM.message.innerHTML = message;
}

function set_server_id(server_url, server_name) {
    if (!server_url)
        server_url = ov_Websockets.server_url();
    if (!server_name)
        server_name = ov_Websockets.server_name();
    let message = server_url;
    if (server_name)
        message += " (" + server_name + ")";
    DOM.server_url.innerHTML = message;
}