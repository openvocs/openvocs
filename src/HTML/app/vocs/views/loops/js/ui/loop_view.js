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
    @file           loop_view.js

    @ingroup        vocs

    @brief          handel display of loops

    ---------------------------------------------------------------------------
*/
import create_uuid from "/lib/ov_utils/ov_uuid.js";

import ov_Websocket from "/lib/ov_websocket.js";
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import * as ov_WebRTCs from "/lib/ov_media/ov_webrtc_list.js";
import * as ov_Vocs from "/lib/ov_vocs.js";

import ov_Loop_Pages from "/components/loops/pages/loop_pages.js";
import ov_Loop from "/components/loops/loop/loop.js";

var ov_SIP;

//-----------------------------------------------------------------------------
// Enum for loop layout options
//-----------------------------------------------------------------------------
export const LAYOUT = {
    GRID: "grid",
    AUTO: "auto",
    LIST: "list"
}

//-----------------------------------------------------------------------------
// init
//-----------------------------------------------------------------------------
const DOM = {};

export var pages;
var current_talk_loop;
var loop_settings;

export async function init() {
    if (SIP)
        ov_SIP = await import("/extensions/sip/ov_sip.js");
    DOM.loops = document.getElementById("loops");

    DOM.loading_screen = document.querySelector("#loading_screen");

    DOM.loop_pagination = document.getElementById("pagination");

    ov_Websockets.addEventListener(ov_Vocs.EVENT.SWITCH_LOOP_STATE, handle_switch_loop_state);
    ov_Websockets.addEventListener(ov_Vocs.EVENT.TALKING, handle_talk_in_loop);
    ov_Websockets.addEventListener(ov_Vocs.EVENT.SWITCH_LOOP_VOLUME, handle_volume_change);
    ov_Websockets.addEventListener(ov_Vocs.EVENT.VAD, handle_vad_in_loop);
}

//-----------------------------------------------------------------------------
// init
//-----------------------------------------------------------------------------
export async function draw() {

    current_talk_loop = undefined;

    let loops_data = await ov_Vocs.collect_loops(ov_Websockets.current_lead_websocket);

    let layout_name = window.innerHeight + "x" + window.innerWidth;
    let settings = await ov_Vocs.collect_keyset_layout(layout_name, ov_Websockets.current_lead_websocket);

    DOM.loops.draw(loops_data, settings, ov_Websockets.current_lead_websocket.server_name);

    pages = DOM.loops.pages;

    DOM.loops.addEventListener("state_change", (event) => {
        update_loop_state(event.detail.loop, event.detail.new_state);
    });

    DOM.loops.addEventListener("volume_change", async (event) => {
        DOM.loading_screen.show("Updating loop volume...");

        let loop = event.detail.loop;
        let volume = event.detail.value

        let response = await ov_Vocs.switch_loop_volume(loop.loop_id, parseInt(volume));
        if (response) {
            loop.volume = response.volume;
            DOM.loading_screen.hide();
        }
    });

    let promises = [];
    for (let page of pages) {
        for (let loop of page.values)
            promises.push(update_loop_state(loop, loop.state));
    }
    await Promise.allSettled(promises);

    if (pages[0].sip) {
        let sip = await ov_SIP.sip(ov_Websockets.current_lead_websocket);
        if (sip.connected) {
            let calls = await ov_SIP.sip_list_calls(ov_Websockets.current_lead_websocket);
            for (let call of Object.keys(calls)) {
                for (let page of pages) {
                    let loop = page.find(calls[call].loop);
                    if (loop)
                        loop.add_call(call, calls[call]);
                }
            }
        } else {
            for (let page of pages)
                for (let loop of page.values)
                    loop.sip_offline = true;
        }
    }

    DOM.loading_screen.hide();


}

export function resize(settings) {
    if (DOM.loops)
        DOM.loops.resize(settings);
}

export function scale_page(value) {
    DOM.loops.scale_page(value);
}

// ----------------------------------------------------------------------------
// handel incoming events from server (server or peer messages)
// ----------------------------------------------------------------------------
async function handle_switch_loop_state(event) {
    if (event.detail.sender.client !== ov_Websockets.current_lead_websocket.client_id) { //peer message
        let success = false;
        if (pages)
            for (let page of pages) {
                let loop = page.find(event.detail.message.loop);
                if (loop) {
                    success = true;
                    // if this is a user broadcast, check if we need to change state
                    if (event.detail.sender.message_type === ov_Websocket.MESSAGE_TYPE.USER_BROADCAST) {
                        if (event.detail.message.role === ov_Websockets.user().role && loop.state !== event.detail.message.state) {
                            if (success)
                                success = await update_loop_state(loop, event.detail.message.state);
                            console.log("sync loop state");
                        }
                    } else { // otherwise only update participants
                        loop.participants = event.detail.message.participants.length;
                    }
                }
            }
        if (!success)
            console.error("loop " + event.detail.message.loop + " not found, failed to switch state");
    } else {
        if (pages)
            for (let page of pages) {
                let loop = page.find(event.detail.message.loop);
                if (loop && loop.state !== event.detail.message.state) {
                    loop.state = event.detail.message.state;
                    loop.participants = event.detail.message.participants.length;
                }
            }
    }
}

function handle_volume_change(event) {
    if (pages && event.detail.sender.message_type === ov_Websocket.MESSAGE_TYPE.USER_BROADCAST &&
        event.detail.message.role === ov_Websockets.user().role) {
        // volume update already applied on server side, we only need a visual update
        for (let page of pages) {
            let loop = page.find(event.detail.message.loop);
            if (loop && loop.volume !== event.detail.message.volume)
                loop.volume = event.detail.message.volume;
        }
    }
}

function handle_talk_in_loop(event) {
    if (pages) {
        // we only need a visual update
        for (let page of pages) {
            let loop = page.find(event.detail.message.loop);
            if (loop) {
                if (event.detail.message.user || event.detail.message.role) {
                    if (event.detail.message.state)
                        loop.add_active_participant(event.detail.message.client, event.detail.message);
                    else
                        loop.remove_active_participant(event.detail.message.client);
                }
            }
        }
    }
    show_loop_activity(event.detail.message.loop, event.detail.message.state);
}

function handle_vad_in_loop(event) {
    if (pages) {
        // we only need a visual update
        for (let page of pages) {
            let loop = page.find(event.detail.message.loop);
            if (loop)
                loop.vad(event.detail.message.on);
        }
    }
    show_loop_activity(event.detail.message.loop, event.detail.message.on);
}

function show_loop_activity(loop_id, state) {
    if (pages) {
        // we only need a visual update 
        for (let page of pages) {
            let loop = page.find(loop_id);
            if (loop) {
                if (page.active)
                    break;

                let current_page = pages[DOM.loop_pagination.value];
                if (current_page && current_page.has(loop_id))
                    break;

                let page_index = pages.indexOf(page);

                if (state)
                    DOM.loop_pagination.highlight("page_" + page_index, true);
                else {
                    let continue_highlighting = false;
                    for (let l of pages[page_index].values) {
                        if (l.is_highlighted) {
                            continue_highlighting = true;
                            break;
                        }
                    }
                    if (!continue_highlighting)
                        DOM.loop_pagination.highlight("page_" + page_index, false);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
// state switch mechanics
//-----------------------------------------------------------------------------
export async function update_loop_state(loop, new_state, websocket) {
    DOM.loading_screen.show("Updating loop state...");

    let result = true;
    let mute = ov_WebRTCs.local_stream() ? !ov_WebRTCs.local_stream().mute : true;
    let response = await ov_Vocs.switch_loop_state(loop.loop_id, loop.state, new_state, mute, websocket);
    if (response.response) {
        loop.state = response.response.state;

        if (loop.state === ov_Loop.STATE.TALK) {
            if (!MULTI_TALK && current_talk_loop && current_talk_loop.loop_id !== loop.loop_id)
                update_loop_state(current_talk_loop, ov_Loop.STATE.MONITOR);
            current_talk_loop = loop;
        } else if (current_talk_loop && current_talk_loop.loop_id === loop.loop_id)
            current_talk_loop = null;

        loop.participants = response.response.participants.length;
        if (response.response.activity.state)
            loop.add_active_participant(response.response.activity.client, response.response.activity);
        else
            loop.remove_active_participant(response.response.activity.client);
        result = response.response;
    } else
        result = !response.error;

    DOM.loading_screen.hide();
    return result
}

export async function talk(unmute) {
    if (pages) {
        let promises = [];
        for (let page of pages) {
            page.values.forEach(async function (loop) {
                if (loop.state === ov_Loop.STATE.TALK)
                    promises.push(update_activity(loop, unmute));
            });
        }
        await Promise.allSettled(promises);
    }
}

async function update_activity(loop, unmute) {
    DOM.loading_screen.show("Updating loop activity...");

    let response = await ov_Vocs.talk_in_loop(loop.loop_id, !!unmute);
    if (response) {
        if (response.state)
            loop.add_active_participant(response.client, response);
        else
            loop.remove_active_participant(response.client);
        DOM.loading_screen.hide();
        return true;
    }
    return false;
}

export async function show_page(new_page) {

    DOM.loading_screen.show("Switching Page...");
    let role = ov_Websockets.user().role;

    if (new_page === undefined) {
        loop_settings = await ov_Vocs.collect_user_settings();
        new_page = loop_settings && loop_settings.roles && loop_settings.roles[role] ? loop_settings.roles[role].page : 0;
    }

    let role_settings = loop_settings && loop_settings.roles && loop_settings.roles[role] ? loop_settings.roles[role] : {};
    role_settings.page = new_page;
    if (!role_settings.loops)
        role_settings.loops = {};

    for (let [index, page] of pages.entries()) {
        if (page.active) { // prev active page
            role_settings.loops[index] = {}
            for (let loop of page.values)
                if (loop.state !== ov_Loop.STATE.NONE) {
                    role_settings.loops[index][loop.loop_id] = loop.state;
                    if (!pages[new_page].has(loop.loop_id))
                        update_loop_state(loop, ov_Loop.STATE.NONE);
                }
            break;
        }
    }

    if (DOM.loops.page_index !== undefined)
        for (let loop of pages[new_page].values) {
            let state;
            if (role_settings.loops[new_page])
                state = role_settings.loops[new_page][loop.loop_id];
            if (loop.state === ov_Loop.STATE.NONE && state) {
                if (state === ov_Loop.STATE.TALK && current_talk_loop && pages[new_page].has(current_talk_loop.loop_id))
                    update_loop_state(loop, ov_Loop.STATE.MONITOR);
                else
                    update_loop_state(loop, state);
            }
        }

    if (DOM.loops.page_index !== new_page) {
        let settings = {};
        settings[role] = role_settings;
        await ov_Vocs.update_user_role_settings(settings);
        DOM.loops.show_page(new_page);
    }

    console.log("show page", new_page);

    DOM.loading_screen.hide();

    return new_page;
}

export async function sync_loops(websocket) {
    for (let page of pages)
        for (let loop of page.values)
            await update_loop_state(loop, loop.state, websocket);
    await ov_Vocs.update_user_settings(loop_settings, websocket);
}