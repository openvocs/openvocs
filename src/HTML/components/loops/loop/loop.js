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
    @file           loop.js

    @ingroup        components/loops/loop

    @brief          custom web component

    ---------------------------------------------------------------------------
*/
import * as CSS from "/css/css.js";

import ov_User from "/lib/ov_object_model/ov_user_model.js";

import ov_Dialog from "/components/dialog/dialog.js";

export default class ov_Loop extends HTMLElement {
    // attributes
    #state;

    // properties
    #loop_id;
    #type;
    #name;
    #abbreviation;
    #project;
    #permission;
    #volume;
    #layout_page;
    #layout_row;
    #layout_column;
    #participants;
    #active_participants;
    #links;

    static CONTENT = {
        ACTIVITY: "loop_activity",
        PARTICIPANTS: "loop_participants",
        PERMISSION: "loop_permission",
        LEAVE: "leave_loop",
        NAME: "loop_name",
        VOLUME: "loop_volume"
    }

    static STATE = {
        TALK: "send",
        MONITOR: "recv",
        NONE: "none"
    }

    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
        this.#active_participants = new Map();
    }

    toString() {
        return this.#loop_id;
    }

    // attributes -------------------------------------------------------------
    static get observedAttributes() {
        return ["layout", "state", "name", "permission"];
    }

    attributeChangedCallback(name, oldValue, newValue) {
        if (oldValue === newValue)
            return;
        if (name === "state") {
            this.#state = newValue;
            if (this.shadowRoot.isConnected) {
                this.update_state();
            }
        }
        if (name === "name") {
            this.#name = newValue.replace(/\\n/g, "<br> ");
            if (this.shadowRoot.isConnected)
                this.update_name();
        }
        if (name === "permission") {
            this.#permission = newValue;
        }
    }

    set state(state) {
        if (ov_Loop.validate_state(state))
            this.setAttribute("state", state);
        else
            console.warn("loop: tried to set state of loop " +
                this.loop_id + " to invalid value " + state + ". Loop remains in state " +
                this.state);
    }

    get state() {
        return this.#state;
    }

    set loop_id(id) {
        this.#loop_id = id;
    }

    get loop_id() {
        return this.#loop_id;
    }

    set type(type) {
        this.#type = type;
    }

    get type() {
        return this.#type;
    }

    set name(name) {
        if (name === "undefined" || !name || name === " ")
            name = this.#loop_id;

        this.setAttribute("name", name);
    }

    get name() {
        return this.#name;
    }

    set abbreviation(abbreviation) {
        this.#abbreviation = abbreviation;
    }

    get abbreviation() {
        return this.#abbreviation;
    }

    set project(project) {
        this.#project = project;
    }

    get project() {
        return this.#project;
    }

    set permission(permission) {
        this.setAttribute("permission", permission);
    }

    get permission() {
        return this.#permission;
    }

    set volume(volume) {
        this.#volume = volume;
        let vol_indicator = this.shadowRoot.querySelector("#loop_volume_indicator_value");
        if (vol_indicator) {
            vol_indicator.textContent = this.#volume;
            this.shadowRoot.querySelector("#loop_volume_input").value = this.#volume;
        }
    }

    get volume() {
        return this.#volume;
    }

    set layout_pos(layout_pos) {
        //this.#layout_pos = parseInt(layout_pos);
        this.#layout_page = parseInt(layout_pos.page);
        this.#layout_column = parseInt(layout_pos.column);
        this.#layout_row = parseInt(layout_pos.row);
    }

    get layout_pos() {
        return { page: this.#layout_page, column: this.#layout_column, row: this.#layout_row };
    }

    set participants(participants) {
        if (!participants && participants !== 0)
            return;
        this.#participants = participants;
        if (this.shadowRoot.isConnected) {
            this.shadowRoot.querySelector("#loop_participants_indicator_value").textContent = this.#participants;
            console.log("number of participants in loop " + this.#loop_id + " changed to " + this.#participants);
        }
    }

    get participants() {
        if (this.#participants === undefined)
            return 0;
        return this.#participants;
    }

    add_active_participant(key, active_participant) {
        if (!(active_participant instanceof ov_User)) {
            let role = active_participant.role;
            active_participant = ov_User.parse(active_participant.user, active_participant);
            active_participant.role = role;
        }
        this.#active_participants.set(key, active_participant);
        this.update_active_participants_list();

        console.log("voice activity: " + active_participant.id + " starts speaking in loop " + this.#loop_id);
    }

    remove_active_participant(key) {
        if (key) {
            this.#active_participants.delete(key);
            this.update_active_participants_list();
            console.log("voice activity: " + key + " finished speaking in loop " + this.#loop_id);
        }
    }

    update_active_participants_list() {
        let value;
        if (ACTIVITY_CONTENT === "display_name")
            value = this.active_participants.map(participant => participant["name"]).toString();
        else
            value = this.active_participants.map(participant => participant["id"]).toString();
        if (this.shadowRoot.isConnected)
            this.shadowRoot.querySelector("#loop_activity").textContent = value;
        this.shadowRoot.host.classList.toggle("activity", this.has_active_participants());
    }

    vad(value) {
        this.shadowRoot.host.classList.toggle("vad", value);
    }

    get is_highlighted() {
        return (this.shadowRoot.host.classList.contains("vad") || this.has_active_participants()) && this.state !== ov_Loop.STATE.NONE;
    }

    get active_participants() {
        return Array.from(this.#active_participants.values());
    }

    has_active_participants() {
        return this.#active_participants.size > 0;
    }

    set roles(links) {
        this.#links = links;
    }

    get roles() {
        return this.#links;
    }

    // -----------------------------------------------------------------
    async connectedCallback() {
        await this.#render();

        this.setup();
    }

    setup() {
        this.update_name();
        this.participants = this.participants;
        this.update_state();
        this.volume = this.#volume;

        this.shadowRoot.querySelector("#join_loop").onclick = () => {
            let next_state = this.#determine_next_loop_state();
            this.dispatchEvent(new CustomEvent("state_change", {
                bubbles: true,
                composed: true,
                detail: {
                    loop: this,
                    new_state: next_state
                }
            }));
        }

        this.shadowRoot.querySelector("#leave_loop").onclick = () => {
            this.dispatchEvent(new CustomEvent("state_change", {
                bubbles: true,
                composed: true,
                detail: {
                    loop: this,
                    new_state: ov_Loop.STATE.NONE
                }
            }));
        }

        this.shadowRoot.querySelector("#loop_volume_input_container").onchange = (event) => {
            this.dispatchEvent(new CustomEvent("volume_change", {
                bubbles: true,
                composed: true,
                detail: {
                    loop: this,
                    value: event.target.value
                }
            }));
        };

        this.shadowRoot.querySelector("#loop_volume_input").oninput = (event) => {
            // only update view
            this.volume = event.target.value;
        };
    }

    update_name() {
        let element = this.shadowRoot.querySelector("#loop_name");
        let string = this.name;
        if (string.length > 30)
            string = string.slice(0, 30) + "...";
        if (element)
            element.innerText = string;
    }

    update_state() {
        let leave_button = this.shadowRoot.querySelector("#leave_loop");
        let volume_button = this.shadowRoot.querySelector("#loop_volume");
        let volume_slider = this.shadowRoot.querySelector("#loop_volume_input");
        if (leave_button && volume_button) {
            console.log("state of loop " + this.#loop_id + " changed to " + this.#state);
            if (this.#state === ov_Loop.STATE.MONITOR || this.#state === ov_Loop.STATE.TALK) {
                this.shadowRoot.host.classList.add("dark");
                leave_button.disabled = false;
                volume_slider.disabled = false;
                volume_button.classList.remove("disabled");
            } else {
                this.shadowRoot.host.classList.remove("dark");
                leave_button.disabled = true;
                volume_slider.disabled = true;
                volume_button.classList.add("disabled");
            }
        }
    }

    /* if loop not joined, press loop to
     * join (monitor)
     * if in monitor mode, press loop to
     * A) switch to talk if permission allows it
     * B) leave loop otherwise
     * if in talk mode, press loop to
     * switch to monitor mode
     */
    #determine_next_loop_state() {
        let next_state;
        if (this.state == ov_Loop.STATE.NONE) {
            next_state = ov_Loop.STATE.MONITOR;
        } else if (this.state == ov_Loop.STATE.MONITOR) {
            if (this.permission == ov_Loop.STATE.MONITOR)
                next_state = ov_Loop.STATE.NONE;
            else
                next_state = ov_Loop.STATE.TALK;
        } else if (this.state == ov_Loop.STATE.TALK)
            next_state = ov_Loop.STATE.MONITOR;
        return next_state;
    }

    parse(id, json, position) {
        if (!json.hasOwnProperty("name"))
            json.name = id;

        if (!json.hasOwnProperty("abbreviation"))
            json.abbreviation = json.name;

        if (!json.hasOwnProperty("state"))
            json.state = ov_Loop.STATE.NONE;

        if (!json.hasOwnProperty("permission"))
            json.permission = ov_Loop.STATE.MONITOR;

        if (!json.hasOwnProperty("volume"))
            json.volume = DEFAULT_LOOP_VOLUME;
        else if (json.volume === 0 && DEFAULT_LOOP_VOLUME)
            json.volume = DEFAULT_LOOP_VOLUME;

        if (!position)
            position = 0;

        this.loop_id = id;
        this.type = json.type;
        this.name = json.name;
        this.abbreviation = json.abbreviation;
        this.project = json.project;
        this.participants = json.participants;
        this.state = json.state;
        this.permission = json.permission;
        this.volume = json.volume;

        if (typeof position !== "object") {
            this.layout_pos = {};
        } else {
            this.layout_pos = position;
        }
    }

    static validate_state(state) {
        if (!(state === ov_Loop.STATE.NONE || state === ov_Loop.STATE.MONITOR || state === ov_Loop.STATE.TALK))
            return false;
        return true;
    }

    async #render() {
        this.shadowRoot.adoptedStyleSheets = [await CSS.normalize_style_sheet, await CSS.ov_basic_style_sheet, await ov_Loop.style_sheet];
        this.shadowRoot.replaceChildren((await ov_Loop.template).content.cloneNode(true));
    }

    static style_sheet = CSS.fetch_style_sheet("/components/loops/loop/loop.css");
    static template = async function () {
        const response = await fetch('/components/loops/loop/loop.html');
        let dom = new DOMParser().parseFromString(await response.text(), 'text/html');
        return dom.querySelector('template');
    }();
}

customElements.define('ov-loop', ov_Loop);