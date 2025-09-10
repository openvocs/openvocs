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
    @file           loop_page.js

    @ingroup        components/loops/loop_page

    @brief          custom web component

    ---------------------------------------------------------------------------
*/
import ov_Loop from "../loop/loop.js";
import Loop_List from "./loop_list.js";
import * as CSS from "/css/css.js";

import create_uuid from "/lib/ov_utils/ov_uuid.js";

export default class ov_Loop_Pages extends HTMLElement {
    #loop_elements;

    // attributes
    #layout;
    #columns;
    #rows;

    //properties
    #pages;

    //intern data
    #loops;
    #settings;
    #current_page_index;

    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
    }

    // attributes -------------------------------------------------------------
    static get observedAttributes() {
        return ["layout", "columns", "rows"];
    }

    attributeChangedCallback(name, old_value, new_value) {
        if (old_value === new_value)
            return;
        if (name === "layout") {
            this.#layout = new_value;
        }
        if (name === "columns") {
            this.#columns = new_value;
        }
        if (name === "rows") {
            this.#rows = new_value;
        }
    }

    set layout(layout) {
        if (layout === undefined)
            this.removeAttribute("layout");
        this.setAttribute("layout", layout);
    }

    get layout() {
        return this.#layout;
    }

    set columns(number) {
        this.setAttribute("columns", number);
    }

    get columns() {
        return this.#columns;
    }

    set rows(number) {
        this.setAttribute("rows", number);
    }

    get rows() {
        return this.#rows;
    }

    get pages() {
        return this.#pages;
    }

    get page_index() {
        return this.#current_page_index;
    }

    async connectedCallback() {
        await this.#render();
        this.#loop_elements = this.shadowRoot.getElementById("loops");

        if (this.#loops && this.#settings)
            this.draw(this.#loops, this.#settings);
    }

    async draw(loops_data, settings) {
        this.#loops = loops_data;
        this.#settings = settings;

        if (this.#loop_elements) {
            this.layout = undefined;//current_layout = undefined;
            this.#loop_elements.replaceChildren();

            this.#pages = Loop_List.parse(this.#loops);

            if (!this.pages)
                return;

            this.set_layout(settings);

            this.setup_pages(this.pages.length);
            if (!this.#current_page_index)
                this.#current_page_index = 0;
            this.show_page(this.#current_page_index);

            for (let page of this.pages) {
                page.sort();
                for (let loop of page.values) {
                    await this.#loop_elements.appendChild(loop);
                    loop.id = "loop_" + create_uuid();
                    loop.addEventListener("click", () => {
                        let style = window.getComputedStyle(loop);
                        this.dispatchEvent(new CustomEvent("loop_selected", {
                            bubbles: true, detail: {
                                loop: loop,
                                column: style.gridColumn,
                                row: style.gridRow
                            }
                        }));
                    });
                }
            }
        }
    }

    remove_loop(loop) {
        this.#pages[this.#current_page_index].remove(loop);
        this.#loop_elements.removeChild(loop);

        this.set_layout(this.#settings);
        this.show_page(this.#current_page_index);
    }

    remove_loop_with_id(loop_id) {
        let loop = this.#pages[this.#current_page_index].find(loop_id);
        this.remove_loop(loop);
    }

    has_loop(loop_id) {
        return this.#pages[this.#current_page_index].has(loop_id);
    }

    async add_loop(loop_id, column, row) {
        let loop = Loop_List.create_loop(loop_id, this.#loops[loop_id], { page: parseInt(this.#current_page_index) + 1, row: row, column: column });
        await this.#loop_elements.appendChild(loop);
        loop.id = "loop_" + create_uuid();
        loop.addEventListener("click", () => {
            let style = window.getComputedStyle(loop);
            this.dispatchEvent(new CustomEvent("loop_selected", {
                bubbles: true, detail: {
                    loop: loop,
                    column: style.gridColumn,
                    row: style.gridRow
                }
            }));
        });

        this.#pages[this.#current_page_index].add(loop);

        this.set_layout(this.#settings);
    }

    reset_grid() {
        this.rows = 0;
        this.columns = 0;
        for (let page of this.pages) {
            if (page.rows > this.rows)
                this.rows = page.rows;
            if (page.columns > this.columns)
                this.columns = page.columns;
        }
    }

    set_layout(settings) {
        this.#settings = settings;
        this.reset_grid();
        this.rows = settings.grid_rows > this.rows ? settings.grid_rows : this.rows;
        this.columns = settings.grid_columns > this.columns ? settings.grid_columns : this.columns;
        if (this.shadowRoot.host)
            this.#format_grid(this.columns, this.rows);

        for (let page of this.pages) {
            for (let loop of page.values) {
                loop.style.gridColumn = loop.layout_pos.column;
                loop.style.gridRow = loop.layout_pos.row;
            }
        }

        return { columns: this.columns, rows: this.rows };
    }

    scale_page(value) {
        SITE_SCALING_FACTOR = value;
        let orig_font_size = parseFloat(getComputedStyle(document.documentElement)
            .getPropertyValue('--orig_base_font_size'));
        let new_font_size = orig_font_size * parseFloat(value);
        document.documentElement.style.setProperty('--base_font_size', new_font_size + "px");
    }

    //-----------------------------------------------------------------------------
    // pages
    //-----------------------------------------------------------------------------

    setup_pages(number) {
        if (number > this.pages.length) {
            for (let index = this.pages.length - 1; index < number; index++)
                this.pages.push(new Loop_List());
        }
    }

    show_page(page_index) {
        for (let page of this.pages) {
            if (this.pages.indexOf(page) === parseInt(page_index)) {
                page.show();
                this.#current_page_index = page_index;

                this.shadowRoot.querySelectorAll(".loopelement_grid_empty").forEach((element) => {
                    element.parentElement.removeChild(element);
                });
                if (window.matchMedia("(width > 480px)").matches) {
                    let number_of_empty_cells = (this.rows * this.columns) - page.length;
                    for (let i = 0; i < number_of_empty_cells; i++) {
                        let empty_loop_element = document.createElement("div");
                        empty_loop_element.classList.add("loopelement_grid_empty");
                        this.#loop_elements.appendChild(empty_loop_element);

                        empty_loop_element.addEventListener("click", () => {
                            let loop_pos = empty_loop_element.getBoundingClientRect();
                            this.dispatchEvent(new CustomEvent("loop_selected", {
                                bubbles: true, detail: {
                                    loop: empty_loop_element,
                                    row: Math.floor(loop_pos.top / loop_pos.height) + 1,
                                    column: Math.floor(loop_pos.left / loop_pos.width) + 1
                                }
                            }));
                        });
                    }
                }
            } else
                page.hide();
        }
    }

    //-----------------------------------------------------------------------------
    // grid
    //-----------------------------------------------------------------------------
    #format_grid(columns, rows) {
        /**
        * Updates loop grid values by writing preset row and column values inline 
        * into the loop grid DOM element.
        **/
        if (columns < LOOP_GRID_MIN_COLUMNS) {
            this.#log_grid_layout_error(columns, rows);
            columns = LOOP_GRID_MIN_COLUMNS;
        }

        if (rows < LOOP_GRID_MIN_ROWS) {
            this.#log_grid_layout_error(columns, rows);
            rows = LOOP_GRID_MIN_ROWS;
        }

        this.shadowRoot.host.style.setProperty("--rows", rows);
        this.shadowRoot.host.style.setProperty("--columns", columns);
    }

    #log_grid_layout_error(columns, rows) {
        console.log("(vc) LoopView: forbidden grid layout values: " + columns +
            " x " + rows + " (columns x rows)");
        console.log("(vc) LoopView: adapting columns and rows values automatically");
    }

    async #render() {
        this.shadowRoot.adoptedStyleSheets = [await CSS.normalize_style_sheet, await CSS.ov_basic_style_sheet, await ov_Loop_Pages.style_sheet, await CSS.loading_id_style_sheet];
        this.shadowRoot.replaceChildren((await ov_Loop_Pages.template).content.cloneNode(true));
    }

    static style_sheet = CSS.fetch_style_sheet("/components/loops/pages/loop_pages.css");
    static template = async function () {
        const response = await fetch('/components/loops/pages/loop_pages.html');
        let dom = new DOMParser().parseFromString(await response.text(), 'text/html');
        return dom.querySelector('template');
    }();
}

customElements.define('ov-loop-pages', ov_Loop_Pages);