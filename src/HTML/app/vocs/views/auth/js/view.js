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

    @ingroup        views/authentication_authorization

    @brief          manage DOM objects for auth view
    	
    ---------------------------------------------------------------------------
*/
import * as ov_Auth from "/lib/ov_auth.js";
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import * as CSS from "/css/css.js";

// import custom HTML elements
import ov_Nav from "/components/nav/nav.js";
import ov_Login_Form from "/components/authentication_form/login_form.js";
import ov_Branding from "/components/branding/branding.js";

const DOM = {
    CLASS: {
        active: "active",
        loading: "loading"
    }
};

export async function init(view_id) {
    if (!document.adoptedStyleSheets.includes(await ov_Login_Form.style_sheet))
        document.adoptedStyleSheets = [...document.adoptedStyleSheets, await ov_Login_Form.style_sheet];

    if (!document.adoptedStyleSheets.includes(await ov_Nav.style_sheet))
        document.adoptedStyleSheets = [...document.adoptedStyleSheets, await ov_Nav.style_sheet];

    if (!document.adoptedStyleSheets.includes(await CSS.loading_id_style_sheet))
        document.adoptedStyleSheets = [...document.adoptedStyleSheets, await CSS.loading_id_style_sheet];

    document.documentElement.className = COLOR_MODE;
    //-------------------------------------------------------------------------
    // DOM elements
    //-------------------------------------------------------------------------
    DOM.view_container = document.getElementById("view_container");

    DOM.login_form = document.querySelector("ov-login-form");
    DOM.role_list = document.getElementById("authorization_list");

    DOM.message = document.getElementById("login_message");
    DOM.server_url = document.getElementById("server_address");

    DOM.authentication_step = document.getElementById("authentication_step");
    DOM.authorization_step = document.getElementById("authorization_step");

    //-------------------------------------------------------------------------
    // set view state
    //-------------------------------------------------------------------------
    await ov_Auth.connect();

    activate_stepper(DOM.authentication_step);
    set_server_id();

    document.getElementById("reload_page").onclick = async () => {
        ov_Websockets.reload_page();
    }

    DOM.login_form.addEventListener("login_triggered", async (event) => {
        let result = await ov_Auth.login(event.detail.username, event.detail.password);

        DOM.login_form.clear_password_field();
        DOM.login_form.stop_loading_animation();

        if (result) {
            set_message("");
            set_server_id();
            if (await ov_Auth.collect_roles())
                populate_role_list();
            else
                set_message("We failed to get the information to which roles " +
                    "you have access from the server. " +
                    "For more info please see the console output.");
        } else {
            let error = ov_Websockets.current_lead_websocket.server_error;
            display_disconnect_notice(error);
        }
    });

    DOM.role_list.addEventListener("click", async (event) => {
        if (event.target.tagName === "INPUT") {
            DOM.role_list.indicate_loading(event.target.id, true);
            if (await ov_Auth.authorize_role(DOM.role_list.value)) {
                DOM.view_container.dispatchEvent(new CustomEvent("switch_view", {
                    detail: { origin: view_id }
                }));
            } else
                set_message("Authorization failed.");
            DOM.role_list.indicate_loading(event.target.id, false);
        }
    });

    const urlParams = new URLSearchParams(window.location.search);
    const keyset = urlParams.get('keysetname')
    document.getElementById("keyset").innerText = keyset;
    document.getElementById("version").innerText = VERSION_NUMBER;
}

export function display_disconnect_notice(error) {
    let error_code = error ? error.code : undefined;
    if (error_code === 5000)
        set_message("You have entered an invalid username or password.");
    else if (error_code === undefined)
        set_message("Connection to server(s) lost. Please wait.");
    else
        set_message("Error: " + error.description + " (Code: " + error.code + ")");
}

export function set_message(message) {
    DOM.message.innerHTML = message;
}

export function set_server_id(server_url, server_name) {
    if (!server_url)
        server_url = ov_Websockets.server_url();
    if (!server_name)
        server_name = ov_Websockets.server_name();
    let message = server_url;
    if (server_name)
        message += " (" + server_name + ")";
    DOM.server_url.innerHTML = message;
}

function populate_role_list() {
    let user = ov_Websockets.user();
    if (user.roles.length === 0) {
        set_message("You have no roles. " +
            "To gain access to roles please contact the project or domain admin.");
    } else {
        user.roles.sort();
        for (let role of user.roles.values) {
            if (role.id !== "admin") {
                let name = role.name;
                if (!name)
                    name = role.id;
                // DOM.role_list.add_item(role.dom_id, name + " (" + role.project + ")", role.id);
                DOM.role_list.add_item(role.dom_id, name, role.id);
            }
        }
    }
    activate_stepper(DOM.authorization_step);
}

//-----------------------------------------------------------------------------
// Stepper
//-----------------------------------------------------------------------------
function activate_stepper(stepper) {
    if (stepper === DOM.authentication_step) {
        DOM.authentication_step.classList.add(DOM.CLASS.active);
        DOM.authorization_step.classList.remove(DOM.CLASS.active);
    } else if (stepper === DOM.authorization_step) {
        DOM.authentication_step.classList.remove(DOM.CLASS.active);
        DOM.authorization_step.classList.add(DOM.CLASS.active);
    }
}