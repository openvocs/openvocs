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

    @ingroup        vocs

    @brief          init and load the voice client view
    	
    ---------------------------------------------------------------------------
*/
import * as Loop_View from "./loop_view.js";
import * as Menu_Slider from "./menu_slider.js";
import * as Settings_Slider from "./settings_slider.js";
import * as PTT_Bar from "./ptt_bar.js";

import ov_Loading from "/components/loading/loading_screen.js";
import ov_Login_Form from "/components/authentication_form/login_form.js";
import ov_Nav from "/components/nav/nav.js";

var DOM = {

};

export async function init() {
    Loop_View.init();
    Menu_Slider.init();
    PTT_Bar.init();
    Settings_Slider.init();

    document.documentElement.className = COLOR_MODE;

    draw_user();

    DOM.loading_screen = document.querySelector("#loading_screen");

    DOM.loading_screen.addEventListener("loading_button_clicked", () => {
        Menu_Slider.open_slider();
    });

    if (!document.adoptedStyleSheets.includes(await ov_Nav.style_sheet))
        document.adoptedStyleSheets = [...document.adoptedStyleSheets, await ov_Nav.style_sheet, await ov_Login_Form.style_sheet];

    if(HIDE_PTT_MENU)
        document.getElementById("push_to_talk_menu").style.visibility = "hidden";
}

function draw_user() {
    Menu_Slider.redraw_user();
    Menu_Slider.redraw_roles();
}

export { talk, resize, sync_loops } from "./loop_view.js";

export async function draw_loops() {
    await Loop_View.draw();
    PTT_Bar.setup_pages();
}

export function indicate_loading(value, message) {
    if (value)
        DOM.loading_screen.show(message);
    else
        DOM.loading_screen.hide();
};

export { ask_for_relogin } from "./ptt_bar.js";

export { logout_triggered, switch_server } from "./menu_slider.js";