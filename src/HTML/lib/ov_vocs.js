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
    @file           ov_vocs.js

    @ingroup        ov_lib

    @brief          implements openvocs vocs protocol and wraps it
                    around a list of ov_websockets

    ---------------------------------------------------------------------------
*/
import ov_Loop from "/components/loops/loop/loop.js";

import * as ov_Websockets from "./ov_websocket_list.js";
import * as ov_DB from "./ov_db.js";

export var EVENT = {
    MEDIA: "media",
    CANDIDATE: "candidate",
    END_OF_CANDIDATES: "end_of_candidates",
    MEDIA_READY: "media_ready",

    SWITCH_LOOP_STATE: "client_switch_loop_state",
    SWITCH_LOOP_VOLUME: "client_switch_loop_volume",
    TALKING: "client_talking",
    VAD: "client_vad"
};

export async function update_user_role_settings(role_settings, websocket) {
    let parameter = await ov_DB.collect_user_settings(websocket ? websocket : ov_Websockets.current_lead_websocket);
    if (!parameter)
        return false;
    if (!parameter.roles)
        parameter.roles = {};
    Object.assign(parameter.roles, role_settings); // merge
    if (websocket)
        return await ov_DB.update_user_settings(parameter, websocket);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.authenticated) {
            let promise = ov_DB.update_user_settings(parameter, ws);
            if (ws === ov_Websockets.current_lead_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
}

export async function switch_loop_state(loop_id, old_state, new_state, audio_activity, websocket) {
    if (websocket)
        return await ws_switch_loop_state(loop_id, old_state, new_state, audio_activity, websocket);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.authenticated) {
            let promise = ws_switch_loop_state(loop_id, old_state, new_state, audio_activity, ws);
            if (ws === ov_Websockets.current_lead_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
}

export async function switch_loop_volume(loop_id, volume, websocket) {
    if (websocket)
        return await ws_switch_loop_volume(loop_id, volume, websocket);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.authenticated) {
            let promise = ws_switch_loop_volume(loop_id, volume, ws);
            if (ws === ov_Websockets.current_lead_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
}

export async function talk_in_loop(loop_id, ptt, websocket) {
    if (websocket)
        return await ws_talk_in_loop(loop_id, ptt, websocket);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.authenticated) {
            let promise = ws_talk_in_loop(loop_id, ptt, ws);
            if (ws === ov_Websockets.current_lead_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
}

export async function request_media_connection(websocket) {
    try {
        console.log(log_prefix(websocket) + "requesting media connection...");
        let media_string = await websocket.send_event(EVENT.MEDIA, { type: "request" });
        console.log(log_prefix(websocket) + "received media string");
        return media_string;
    } catch (error) {
        console.warn(log_prefix(websocket) + "requesting media connection failed.", error.error);
        return false;
    }
}

export async function send_media_answer(sdp, websocket) {
    try {
        console.log(log_prefix(websocket) + "send media answer...");
        await websocket.send_event(EVENT.MEDIA, { type: "answer", sdp: sdp });
        console.log(log_prefix(websocket) + "media answer send");
        return true;
    } catch (error) {
        console.warn(log_prefix(websocket) + "sending media answer failed.", error.error);
        return false;
    }
}

export async function send_ice_candidate(candidate, sdpMid, sdpMLineIndex, ufrag, websocket) {
    try {
        console.log(log_prefix(websocket) + "send ice candidate...");
        let parameter = {
            candidate: candidate,
            sdpMid: sdpMid,
            sdpMLineIndex: sdpMLineIndex,
            ufrag: ufrag
        };
        await websocket.send_event(EVENT.CANDIDATE, parameter);
        console.log(log_prefix(websocket) + "ice candidate send");
        return true;
    } catch (error) {
        console.warn(log_prefix(websocket) + "sending ice candidate failed.", error.error);
        return false;
    }
}

export async function send_end_of_ice_candidates(websocket) {
    try {
        console.log(log_prefix(websocket) + "send end of ice candidates...");
        await websocket.send_event(EVENT.END_OF_CANDIDATES);
        console.log(log_prefix(websocket) + "end of ice candidates send");
        return true;
    } catch (error) {
        console.warn(log_prefix(websocket) + "sending end of ice candidates failed.", error.error);
        return false;
    }
}

async function ws_switch_loop_state(loop_id, old_state, new_state, audio_activity, websocket) {
    let response, response_error = false;
    let activity = false;

    if (audio_activity && old_state === ov_Loop.STATE.TALK)
        activity = await ws_talk_in_loop(loop_id, false, websocket);

    try {
        console.log(log_prefix(websocket) + "switch loop state...");
        let parameter = { loop: loop_id, state: new_state };
        response = await websocket.send_event(EVENT.SWITCH_LOOP_STATE, parameter);
        console.log(log_prefix(websocket) + "switched state of loop " + response.loop + " to " + response.state);
    } catch (error) {
        console.warn(log_prefix(websocket) + "switch loop state failed.", error.error);
        response = error.response;
        response_error = true;
    }

    if (audio_activity && response.state === ov_Loop.STATE.TALK)
        activity = await ws_talk_in_loop(loop_id, true, websocket);

    response.activity = activity;

    return { response: response, error: response_error };
}

async function ws_switch_loop_volume(loop_id, volume, websocket) {
    try {
        console.log(log_prefix(websocket) + "switching loop volume...");
        let parameter = { loop: loop_id, volume: volume };
        let response = await websocket.send_event(EVENT.SWITCH_LOOP_VOLUME, parameter);
        console.log(log_prefix(websocket) + "switched volume of loop " + response.loop + " to " + response.volume);
        return response;
    } catch (error) {
        console.warn(log_prefix(websocket) + "switch loop volume failed.", error.error);
        return false;
    }
}

async function ws_talk_in_loop(loop_id, ptt, websocket) {
    try {
        console.log(log_prefix(websocket) + "signal talking in loop...");
        let parameter = { loop: loop_id, state: ptt };
        let response = await websocket.send_event(EVENT.TALKING, parameter);
        console.log(log_prefix(websocket) + "signaled talking in loop " + response.loop);
        return response;
    } catch (error) {
        console.warn(log_prefix(websocket) + "signaling talking in loop failed.", error.error);
        return false;
    }
}

//-----------------------------------------------------------------------------
// logging
//-----------------------------------------------------------------------------
function log_prefix(ws) {
    return "(" + ws.server_name + " vocs) ";
}