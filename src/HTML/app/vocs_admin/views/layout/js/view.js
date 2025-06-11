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

    @ingroup        vocs_admin/views/layout

    @brief          manage DOM objects for layout view
    	
    ---------------------------------------------------------------------------
*/
import ov_Loop_Pages from "/components/loops/pages/loop_pages.js";
const DOM = {};

var VIEW_ID;

var loops_data;
var roles_data;
var current_role;

export function init(view_id) {
    VIEW_ID = view_id;
    DOM.loops = document.getElementById("loops");
    DOM.roles = document.getElementById("choose_role");
    DOM.pages = document.getElementById("number_pages");
    DOM.page_scale = document.getElementById("page_zoom_input");
    DOM.loop_layout = document.getElementById("loop_layout_select");
    DOM.grid_options = document.getElementById("grid_options_panel");
    DOM.grid_columns = document.getElementById("grid_columns_input");
    DOM.grid_rows = document.getElementById("grid_rows_input");
    DOM.loop_name_size = document.getElementById("loop_name_size_input");
    DOM.loop_font_size = document.getElementById("loop_font_size_input");
    DOM.select_loop_dialog = document.getElementById("choose_loop_dialog");
    DOM.select_loop = document.getElementById("loop_chooser");
    DOM.role_selector = document.querySelector("#choose_role");
    DOM.page_selector = document.querySelector("#choose_page");

    let menu = document.querySelector("#menu");
    menu.onclick = (event) => {
        if (event.target === menu)
            menu.close();
    }
    document.querySelector("#open_menu").addEventListener("click", () => {
        menu.showModal();
    });

    DOM.role_selector.addEventListener("change", render_role);
    DOM.page_selector.addEventListener("change", show_page);

    DOM.pages.addEventListener("change", () => {
        DOM.loops.setup_pages(DOM.pages.value);
        setup_pages();
    });

    DOM.page_scale.addEventListener("change", change_setting);
    DOM.loop_layout.addEventListener("change", change_layout);
    DOM.grid_columns.addEventListener("change", change_setting);
    DOM.grid_rows.addEventListener("change", change_setting);
    DOM.loop_name_size.addEventListener("change", change_setting);
    DOM.loop_font_size.addEventListener("change", change_setting);

    DOM.loops.addEventListener("loop_selected", (event) => {
        if (event.detail.loop.id)
            DOM.select_loop.value = event.detail.loop;
        else
            DOM.select_loop.value = "";

        DOM.select_loop.onchange = () => {
            DOM.loops.remove_loop(event.detail.loop);
            if (DOM.select_loop.value)
                DOM.loops.add_loop(DOM.select_loop.value, event.detail.column, event.detail.row);
            DOM.select_loop_dialog.close();
            event.detail.loop.style.removeProperty("border");
        }

        event.detail.loop.style.border = "solid var(--green)";

        DOM.select_loop_dialog.showModal();

        DOM.select_loop_dialog.onclick = (e) => {
            if (e.target === DOM.select_loop_dialog) {
                DOM.select_loop_dialog.close();
                event.detail.loop.style.removeProperty("border");
            }
        }
    });
}

export async function render(roles, loops, settings) {
    if (!settings)
        settings = collect_page_layout();
    loops_data = loops;
    roles_data = roles;
    DOM.role_selector.replaceChildren();
    if (roles)
        for (let role_id of Object.keys(roles)) {
            let role_option = document.createElement("option");
            role_option.value = role_id;
            role_option.innerText = roles[role_id].name ? roles[role_id].name : role_id;
            DOM.role_selector.appendChild(role_option);
        }

    if (settings) {
        if (settings.layout)
            DOM.loop_layout.value = settings.layout;
        if (settings.grid_columns)
            DOM.grid_columns.value = settings.grid_columns;
        if (settings.grid_rows)
            DOM.grid_rows.value = settings.grid_rows;
        if (settings.site_scaling)
            DOM.page_scale.value = settings.site_scaling;
        if (settings.name_scaling)
            DOM.loop_name_size.value = settings.name_scaling;
        if (settings.font_scaling)
            DOM.loop_font_size.value = settings.font_scaling;
    }

    if (!DOM.grid_columns.value || DOM.loops.columns > DOM.grid_columns.value)
        DOM.grid_columns.value = DOM.loops.columns;

    if (!DOM.grid_rows.value || DOM.loops.rows > DOM.grid_rows.value)
        DOM.grid_rows.value = DOM.loops.rows;

    if (!DOM.loop_layout.value)
        DOM.loop_layout.value = ov_Loop_Pages.LAYOUT.AUTO;
    if (!DOM.grid_columns.value)
        DOM.grid_columns.value = 2;
    if (!DOM.grid_rows.value)
        DOM.grid_rows.value = 2;
    if (!DOM.page_scale.value)
        DOM.page_scale.value = 1.0;
    if (!DOM.loop_name_size.value)
        DOM.loop_name_size.value = 1.5;
    if (!DOM.loop_font_size.value)
        DOM.loop_font_size.value = 1.0;

    DOM.role_selector.selectedIndex = 0;
    await render_role();
    current_role = DOM.role_selector.value;
}

export function disable_settings(value) {
    DOM.loop_layout.disabled = value;
    DOM.grid_columns.disabled = value;
    DOM.grid_rows.disabled = value;
    DOM.page_scale.disabled = value;
    DOM.loop_name_size.disabled = value;
    DOM.loop_font_size.disabled = value;
}

async function render_role() {
    if (loops_data) {
        save_role();
        current_role = DOM.role_selector.value

        let role_loops = {};
        for (let loop_id of Object.keys(loops_data)) {
            loops_data[loop_id].layout_position = [];
            if (loops_data[loop_id].roles.hasOwnProperty(DOM.role_selector.value)) {
                role_loops[loop_id] = loops_data[loop_id];
                if (roles_data[DOM.role_selector.value].layout && roles_data[DOM.role_selector.value].layout[loop_id])
                    role_loops[loop_id].layout_position = roles_data[DOM.role_selector.value].layout[loop_id];
            }
        }
        await render_loops(role_loops);
        setup_pages(DOM.loops.pages.length);
        show_page();
    }
}

function setup_pages(number) {
    if (number)
        DOM.pages.value = number;
    DOM.page_selector.replaceChildren();
    for (let index = 1; index <= DOM.pages.value; index++) {
        let page_option = document.createElement("option");
        page_option.value = index - 1;
        page_option.innerText = "Page " + index;
        DOM.page_selector.appendChild(page_option);
    }
}

function show_page() {
    DOM.loops.show_page(DOM.page_selector.value);
}

function save_role() {
    if (DOM.loops.pages && current_role) {
        let layout = {};
        for (let page of DOM.loops.pages) {
            for (let loop of page.values) {
                if (!layout[loop.loop_id])
                    layout[loop.loop_id] = [];
                layout[loop.loop_id].push(loop.layout_pos)
            }
        }
        roles_data[current_role].layout = layout;
    }
}

async function render_loops(loops) {
    await DOM.loops.draw(loops, collect_page_layout());

    resize();

    let default_option = document.createElement("option");
    default_option.value = "";
    default_option.selected = true;
    DOM.select_loop.replaceChildren(default_option);
    let options = [];
    for (let loop_id of Object.keys(loops)) {
        let element = document.createElement("option");
        element.value = loop_id;
        element.innerText = loops[loop_id].name ? loops[loop_id].name : loop_id;
        options.push(element);
    }
    options.sort((a, b) => a.innerText > b.innerText ? 1 : 0);
    for (let option of options)
        DOM.select_loop.appendChild(option);
}

export function collect_page_layout() {
    let settings = {};
    settings.layout = DOM.loop_layout.value;
    settings.grid_columns = parseInt(DOM.grid_columns.value);
    settings.grid_rows = parseInt(DOM.grid_rows.value);
    settings.site_scaling = parseFloat(DOM.page_scale.value);
    settings.name_scaling = parseFloat(DOM.loop_name_size.value);
    settings.font_scaling = parseFloat(DOM.loop_font_size.value);
    return settings;
}

function change_setting() {
    resize();
}

function change_layout() {
    if (DOM.loop_layout.value === ov_Loop_Pages.LAYOUT.LIST) {
        DOM.grid_options.style.display = "none";
    } else {
        DOM.grid_options.style.display = "inherit";
    }
    resize();
}

export function resize(settings) {
    if (settings)
        DOM.loops.resize(settings);
    else
        DOM.loops.resize(collect_page_layout());
}

export function collect_role_layout() {
    save_role();
    let roles = {}
    if (roles_data)
        for (let role_id of Object.keys(roles_data))
            roles[role_id] = roles_data[role_id].layout;
    return roles;
}

export function offline_mode(value) {
    value = !value && !DOM.id.disabled ? true : value;
    DOM.delete_button.disabled = value;
}