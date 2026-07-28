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
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import * as ov_Auth from "/lib/ov_auth.js";
import * as ov_DB from "/lib/ov_db.js";
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

var switch_triggered;

export async function init(view_id, authenticated) {
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
    switch_triggered = false;

    if (!authenticated)
        display_authentication();
    else
        display_authorization();
    set_server_id();

    document.getElementById("reload_page").onclick = async () => {
        ov_Websockets.reload_page();
    }

    document.getElementById("logout_button").onclick = async () => {
        await ov_Auth.logout();
        ov_Websockets.reload_page();
    }

    DOM.login_form.addEventListener("login_triggered", async (event) => {
        let result = await ov_Auth.login(event.detail.username, event.detail.password);

        DOM.login_form.clear_password_field();
        DOM.login_form.stop_loading_animation();

        if (result.authenticated) {
            display_authorization();
        } else
            display_disconnect_notice(result.error);
    });

    DOM.role_list.addEventListener("click", async (event) => {
        if (event.target.tagName === "INPUT") {
            DOM.role_list.indicate_loading(event.target.id, true);
            for (let ws of ov_Websockets.list) {
                ov_Auth.authorize_role(DOM.role_list.value, ws).then((result) => {
                    if (!switch_triggered) {
                        if (ov_Websockets.current_lead_websocket !== ws && !ov_Websockets.current_lead_websocket.connected)
                            ov_Websockets.switch_lead_websocket(ws);
                        if (ov_Websockets.current_lead_websocket === ws) {
                            switch_triggered = true;
                            if (result.authorized) {
                                DOM.view_container.dispatchEvent(new CustomEvent("switch_view", {
                                    detail: { origin: view_id }
                                }));
                            } else
                                set_message("Authorization failed.");
                            DOM.role_list.indicate_loading(event.target.id, false);
                        }
                    }
                });
            }
        }
    });

    const urlParams = new URLSearchParams(window.location.search);
    const keyset = urlParams.get('keysetname');
    document.getElementById("keyset").innerText = keyset;
    document.getElementById("version").innerText = VERSION_NUMBER;

    DOM.login_form.focus_user_input();
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
    if (user.roles.length !== 0) {
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
}

//-----------------------------------------------------------------------------
// Stepper
//-----------------------------------------------------------------------------
export function display_authentication() {
    DOM.authentication_step.classList.add(DOM.CLASS.active);
    DOM.authorization_step.classList.remove(DOM.CLASS.active);
}

export async function display_authorization() {
    set_message("");
    set_server_id();
    let result = await ov_DB.collect_roles();
    if (result.roles_collected)
        populate_role_list();
    else
        set_message("We failed to get the information to which roles " +
            "you have access from the server. " +
            "For more info please see the console output.");

    DOM.authentication_step.classList.remove(DOM.CLASS.active);
    DOM.authorization_step.classList.add(DOM.CLASS.active);
}