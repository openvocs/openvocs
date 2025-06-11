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

    @author         Anja Bertard

    @date           2024-06-05

    @ingroup        components/sip/loop

    @brief          custom web component

    ---------------------------------------------------------------------------
*/
import * as CSS from "/css/css.js";
import ov_Loop from "/components/loops/loop/loop.js";
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import ov_Websocket from "/lib/ov_websocket.js";
import * as ov_SIP from "/extensions/sip/ov_sip.js";
import ov_SIP_Number_Pad from "/extensions/sip/components/number_pad/number_pad.js";
import ov_SIP_Calls from "/extensions/sip/components/call_list/call_list.js";

export default class ov_SIP_Loop extends ov_Loop {

    #sip_status; // off, waiting, call
    #sip_permission; // undefined, "hangup", "call"

    #sip_number_pad_dialog;
    #sip_number_pad;
    #sip_calls_dialog;
    #sip_calls;
    #sip_calls_icon;

    constructor() {
        super();
    }

    static get observedAttributes() {
        let array = super.observedAttributes;
        array.push("sip_status", "sip_permission");
        return array;
    }

    attributeChangedCallback(name, old_value, new_value) {
        super.attributeChangedCallback(name, old_value, new_value);

        if (name === "layout") {
            if (this.shadowRoot.isConnected) {
                this.update_layout();
            }
        } else if (name === "sip_status") {
            this.#sip_status = new_value;
        } else if (name === "sip_permission") {
            this.#sip_permission = new_value;
            if (this.#sip_calls)
                this.#sip_calls.allow_new_calls = this.#sip_permission === "call";
        }
    }

    get sip_status() {
        return this.#sip_status;
    }

    set sip_offline(value) {
        if (value) {
            this.setAttribute("sip_status", "off");
            this.shadowRoot.querySelector("#loop_sip").disabled = true;
            if (this.#sip_calls && this.#sip_calls.calls)
                for (let call_id of this.#sip_calls.calls.keys()) {
                    this.#sip_calls.remove_call(call_id);
                }
        } else {
            this.setAttribute("sip_status", "waiting");
            if ((this.state === ov_Loop.STATE.MONITOR || this.state === ov_Loop.STATE.TALK) && this.sip_permission !== undefined)
                this.shadowRoot.querySelector("#loop_sip").disabled = false;
        }
    }

    get sip_permission() {
        return this.#sip_permission;
    }

    set sip_permission(value) {
        if (value === "hangup" || value === "call")
            this.setAttribute("sip_permission", value);
        else
            this.removeAttribute("sip_permission");
    }

    async connectedCallback() {
        await this.#render();

        this.setup();

        this.update_layout();

        this.sip_offline = false;

        //SIP
        this.#sip_number_pad_dialog = this.shadowRoot.querySelector("#sip_number_pad");
        this.#sip_number_pad = this.shadowRoot.querySelector("ov-sip-number-pad");
        this.#sip_calls_dialog = this.shadowRoot.querySelector("#sip_calls");
        this.#sip_calls = this.shadowRoot.querySelector("ov-sip-calls");
        this.#sip_calls_icon = this.shadowRoot.querySelector("#sip_badge_icon");

        this.#sip_number_pad.loop = this.name;
        this.#sip_calls.loop = this.name;

        this.#sip_calls.allow_new_calls = this.#sip_permission === "call";

        this.#sip_number_pad_dialog.onclick = (event) => {
            if (event.target === this.#sip_number_pad_dialog) {
                this.#sip_number_pad_dialog.close();
            }
        }

        this.#sip_calls_dialog.onclick = (event) => {
            if (event.target === this.#sip_calls_dialog) {
                this.#sip_calls_dialog.close();
            }
        }

        this.shadowRoot.querySelector("#loop_sip").onclick = async () => {
            if (this.#sip_calls.calls.size === 0 && this.sip_permission === "call")
                this.#sip_number_pad_dialog.showModal();
            else
                this.#sip_calls_dialog.showModal();
        };

        this.#sip_number_pad.addEventListener("trigger_call", async (event) => {
            if (event.detail) {
                if (this.#sip_status !== "off") {
                    let result = await ov_SIP.sip_call(this.loop_id, undefined, event.detail, ov_Websockets.current_lead_websocket);
                    if (result) {
                        if (!this.#sip_calls.calls.has(result["call-id"]))
                            await this.#sip_calls.add_call(result["call-id"], result.callee);
                        this.#sip_calls_dialog.showModal();
                        this.#sip_number_pad_dialog.close();
                        if (this.#sip_calls.calls.size === 1)
                            this.setAttribute("sip_status", "call");
                    } else
                        this.#sip_number_pad.error = "Call failed.";
                } else
                    this.#sip_number_pad.error = "Telephone server offline.";
            } else {
                this.#sip_number_pad.error = "Please enter a number.";
            }
        });

        this.#sip_number_pad.addEventListener("show_calls", () => {
            this.#sip_calls_dialog.showModal();
            this.#sip_number_pad_dialog.close();
        });

        this.#sip_calls.addEventListener("new_call", () => {
            this.#sip_calls_dialog.close();
            this.#sip_number_pad_dialog.showModal();
        });

        this.#sip_calls.addEventListener("trigger_hangup", async (event) => {
            let result = await ov_SIP.sip_hangup(event.detail.loop, event.detail.call, ov_Websockets.current_lead_websocket);
            if (result)
                this.#sip_calls.change_call_status(event.detail.call, "disconnecting...");
            else
                this.#sip_calls.change_call_status(event.detail.call, "Hangup failed.");
        });

        this.shadowRoot.querySelector("#loop_volume").onclick = () => {
            if (this.layout === ov_Loop.LAYOUT.GRID) {
                let input = this.shadowRoot.querySelector("#loop_volume_input_container");
                if (getComputedStyle(input).display === "none")
                    input.style.display = "inherit";
                else
                    input.style.display = "none";
            }
        }

        ov_Websockets.addEventListener(ov_SIP.EVENT.SIP, this.#on_server_status.bind(this));
        ov_Websockets.addEventListener(ov_SIP.EVENT.SIP_CALL, this.#on_call.bind(this));
        ov_Websockets.addEventListener(ov_SIP.EVENT.SIP_HANGUP, this.#on_hangup.bind(this));
    }

    update_layout() {
        let input = this.shadowRoot.querySelector("#loop_volume_input");
        if (input && this.layout === ov_Loop.LAYOUT.GRID) {
            input.setAttribute("orient", "vertical");
            input.classList.add("vertical");
        } else if (input) {
            input.setAttribute("orient", "horizontal");
            input.classList.remove("vertical");
        }
    }

    update_state() {
        super.update_state();
        let sip = this.shadowRoot.querySelector("#loop_sip");
        if (sip) {
            if ((this.state === ov_Loop.STATE.MONITOR || this.state === ov_Loop.STATE.TALK) && this.sip_permission !== undefined)
                this.shadowRoot.querySelector("#loop_sip").disabled = false;
            else
                this.shadowRoot.querySelector("#loop_sip").disabled = true;
        }
    }

    async add_call(call_id, call) {
        if (!this.#sip_calls.calls.has(call_id)) {
            await this.#sip_calls.add_call(call_id, call.peer);
        }
        this.#sip_number_pad.clear_number();
        this.#sip_calls.change_call_status(call_id, "connected");
        this.#number_of_calls_badge();
        this.#sip_calls_dialog.close();
        if (this.#sip_calls.calls.size > 0)
            this.setAttribute("sip_status", "call");
    }

    #on_call(event) {
        if (this.loop_id === event.detail.message.loop) {
            if (!event.detail.error) {
                if (event.detail.sender.message_type === ov_Websocket.MESSAGE_TYPE.LOOP_BROADCAST)
                    this.add_call(event.detail.message["call-id"], event.detail.message);
            } else {
                let call_id = event.detail.message["call-id"];
                if (call_id === undefined) {
                    for (let [id, call] of this.#sip_calls.calls.entries()) {
                        if (call.querySelector(".call_number").innerText === event.detail.message.callee) {
                            call_id = id;
                            break;
                        }
                    }
                }
                this.#sip_calls.change_call_status(call_id, "Error: " + event.detail.error.description);
                this.#sip_calls.deactivate_call(call_id);
                setTimeout(() => {
                    this.#sip_calls.remove_call(call_id);
                    if (this.#sip_calls.calls.size === 0 && this.sip_status !== "off")
                        this.setAttribute("sip_status", "waiting");
                }, 5000);
            }
        }
    }

    #on_hangup(event) {
        if (!event.detail.error) {
            if (event.detail.sender.message_type === ov_Websocket.MESSAGE_TYPE.LOOP_BROADCAST &&
                this.loop_id === event.detail.message.loop) {
                let call_id = event.detail.message["call-id"];

                this.#sip_calls.remove_call(call_id);
                this.#number_of_calls_badge();
                this.#sip_calls_dialog.close();

                if (this.#sip_calls.calls.size === 0 && this.sip_status !== "off")
                    this.setAttribute("sip_status", "waiting");
            }
        } else {
            console.warn(event.detail);
        }
    }

    #on_server_status(event) {
        if (event.detail.message.connected)
            this.sip_offline = false;
        else
            this.sip_offline = true;
    }

    #number_of_calls_badge() {
        this.#sip_calls_icon.classList.remove("sip_badge_1", "sip_badge_2", "sip_badge_3", "sip_badge_4", "sip_badge_5", "sip_badge_6");
        let number = this.#sip_calls.calls.size > 5 ? 6 : this.#sip_calls.calls.size;
        if (number)
            this.#sip_calls_icon.classList.add("sip_badge_" + number);
    }

    parse(id, json, position) {
        super.parse(id, json, position);
        if (json.sip && json.sip.roles) {
            let sip_permission = json.sip.roles[ov_Websockets.user().role];
            if (sip_permission === true)
                this.sip_permission = "call";
            else if (sip_permission === false)
                this.sip_permission = "hangup";
        }
    }

    async #render() {
        this.shadowRoot.adoptedStyleSheets = [await CSS.normalize_style_sheet, await CSS.ov_basic_style_sheet, await ov_Loop.style_sheet, await ov_SIP_Loop.style_sheet];
        this.shadowRoot.replaceChildren((await ov_SIP_Loop.template).content.cloneNode(true));
    }

    static style_sheet = CSS.fetch_style_sheet("/extensions/sip/components/loop/loop.css");
    static template = async function () {
        const response = await fetch('/extensions/sip/components/loop/loop.html');
        let dom = new DOMParser().parseFromString(await response.text(), 'text/html');
        return dom.querySelector('template');
    }();
}

customElements.define('ov-sip-loop', ov_SIP_Loop);