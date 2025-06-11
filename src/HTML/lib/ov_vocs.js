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

    @author         Anja Bertard

    @date           2024-04-24

    @ingroup        ov_lib

    @brief          implements openvocs vocs protocol and wraps it
                    around a list of ov_websockets

    ---------------------------------------------------------------------------
*/
import ov_Loop from "/components/loops/loop/loop.js";

import ov_Websocket from "./ov_websocket.js";
import * as ov_Websockets from "./ov_websocket_list.js";

var RETRIES_ON_TEMP_ERROR = 5;

export var EVENT = {
    KEYSET_LAYOUT: "get_keyset_layout",
    UPDATE_USER_SETTINGS: "set_user_data",
    USER_SETTINGS: "get_user_data",

    MEDIA: "media",
    CANDIDATE: "candidate",
    END_OF_CANDIDATES: "end_of_candidates",
    MEDIA_READY: "media_ready",

    ROLE_LOOPS: "role_loops",
    SWITCH_LOOP_STATE: "switch_loop_state",
    SWITCH_LOOP_VOLUME: "switch_loop_volume",
    TALKING: "talking",
    VAD: "vad"
};

export async function collect_keyset_layout(layout_id, websocket) {
    if (websocket)
        return await ws_collect_keyset_layout(websocket, layout_id);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.is_ready && ws.authenticated) {
            let promise = ws_collect_keyset_layout(ws, layout_id);
            if (ws === ov_Websockets.current_lead_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
}

export async function update_user_settings(settings, websocket) {
    if (websocket)
        return await ws_update_user_settings(websocket, settings);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.is_ready && ws.authenticated) {
            let promise = ws_update_user_settings(ws, settings);
            if (ws === ov_Websockets.current_lead_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
}

export async function update_user_role_settings(role_settings, websocket) {
    let parameter = await ws_collect_user_settings(websocket ? websocket : ov_Websockets.current_lead_websocket);
    if (!parameter)
        return false;
    if (!parameter.roles)
        parameter.roles = {};
    Object.assign(parameter.roles, role_settings); // merge
    if (websocket)
        return await ws_update_user_settings(websocket, parameter);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.is_ready && ws.authenticated) {
            let promise = ws_update_user_settings(ws, parameter);
            if (ws === ov_Websockets.current_lead_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
}

export async function collect_user_settings(websocket) {
    if (websocket)
        return await ws_collect_user_settings(websocket);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.is_ready && ws.authenticated) {
            let promise = ws_collect_user_settings(ws);
            if (ws === ov_Websockets.current_lead_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
}

export async function collect_loops(websocket) {
    if (websocket)
        return await ws_collect_loops(websocket);

    let lead_promise;
    for (let ws of ov_Websockets.list) {
        if (ws.is_ready && ws.authenticated) {
            let promise = ws_collect_loops(ws);
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
        if (ws.is_ready && ws.authenticated) {
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
        if (ws.is_ready && ws.authenticated) {
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
        if (ws.is_ready && ws.authenticated) {
            let promise = ws_talk_in_loop(loop_id, ptt, ws);
            if (ws === ov_Websockets.current_lead_websocket)
                lead_promise = promise;
        }
    }
    return await lead_promise;
}

export async function request_media_connection(websocket) {
    let media_string;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "requesting media connection...");
            media_string = await websocket.send_event(EVENT.MEDIA, { type: "request" });
            console.log(log_prefix(websocket) + "received media string");
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to requesting media connection again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "requesting media connection failed.", error);
                disconnect(websocket);
                return false;
            }
        }
    }
    return media_string;
}

export async function send_media_answer(sdp, websocket) {
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "send media answer...");
            await websocket.send_event(EVENT.MEDIA, { type: "answer", sdp: sdp });
            console.log(log_prefix(websocket) + "media answer send");
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to send media answer again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "sending media answer failed.", error);
                disconnect(websocket);
                return false;
            }
        }
    }
    return true;
}

export async function send_ice_candidate(candidate, sdpMid, sdpMLineIndex, ufrag, websocket) {
    let parameter = {
        candidate: candidate,
        sdpMid: sdpMid,
        sdpMLineIndex: sdpMLineIndex,
        ufrag: ufrag
    };

    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "send ice candidate...");
            await websocket.send_event(EVENT.CANDIDATE, parameter);
            console.log(log_prefix(websocket) + "ice candidate send");
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to send ice candidate again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "sending ice candidate failed.", error);
                disconnect(websocket);
                return false;
            }
        }
    }
    return true;
}

export async function send_end_of_ice_candidates(websocket) {
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "send end of ice candidates...");
            await websocket.send_event(EVENT.END_OF_CANDIDATES);
            console.log(log_prefix(websocket) + "end of ice candidates send");
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to send end of ice candidates again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "sending end of ice candidates failed.", error);
                disconnect(websocket);
                return false;
            }
        }
    }
    return true;
}

async function ws_collect_keyset_layout(websocket, layout_id) {
    let result;
    if (!layout_id)
        layout_id = "default";
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "collecting keyset layout...");
            result = await websocket.send_event(EVENT.KEYSET_LAYOUT, { domain: websocket.user.domain, layout: layout_id });
            console.log(log_prefix(websocket) + "received settings for layout" + layout_id);
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to collect keyset layout again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "collect keyset layout failed.", error);
                disconnect(websocket);
                return false;
            }
        }
    }
    return result.layout;
}

async function ws_collect_user_settings(websocket) {
    let result;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "collecting user settings...");
            result = await websocket.send_event(EVENT.USER_SETTINGS);
            console.log(log_prefix(websocket) + "received user settings");
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to collect user settings again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "collect user settings failed.", error);
                disconnect(websocket);
                return false;
            }
        }
    }
    return result.data;
}

async function ws_update_user_settings(websocket, parameter) {
    let result;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "update user settings...");
            result = await websocket.send_event(EVENT.UPDATE_USER_SETTINGS, parameter);
            console.log(log_prefix(websocket) + "updated user settings");
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to update user settings again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "update user settings failed.", error);
                disconnect(websocket);
                return false;
            }
        }
    }
    return result;
}

async function ws_collect_loops(websocket) {
    let result;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "collecting role loops...");
            result = await websocket.send_event(EVENT.ROLE_LOOPS);
            console.log(log_prefix(websocket) + "received loops");
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to collect role loops again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "collect role loops failed.", error);
                disconnect(websocket);
                return false;
            }
        }
    }
    return result.loops;
}

async function ws_switch_loop_state(loop_id, old_state, new_state, audio_activity, websocket) {
    let response, response_error = false;
    let activity = false;

    if (audio_activity && old_state === ov_Loop.STATE.TALK)
        activity = await ws_talk_in_loop(loop_id, false, websocket);

    let parameter = { loop: loop_id, state: new_state };
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "switch loop state...");
            response = await websocket.send_event(EVENT.SWITCH_LOOP_STATE, parameter);
            console.log(log_prefix(websocket) + "switched state of loop " + response.loop + " to " + response.state);
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to switch loop state again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "switch loop state failed.", error);
                response = error.response;
                response_error = true;
                disconnect(websocket);
                return false;
            }
        }
    }

    if (audio_activity && response.state === ov_Loop.STATE.TALK)
        activity = await ws_talk_in_loop(loop_id, true, websocket);

    response.activity = activity;

    return { response: response, error: response_error };
}

async function ws_switch_loop_volume(loop_id, volume, websocket) {
    let parameter = { loop: loop_id, volume: volume };

    let response;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "switching loop volume...");
            response = await websocket.send_event(EVENT.SWITCH_LOOP_VOLUME, parameter);
            console.log(log_prefix(websocket) + "switched volume of loop " + response.loop + " to " + response.volume);
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to switch loop volume again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "switch loop volume failed.", error);
                disconnect(websocket);
                return false;
            }
        }
    }
    return response;
}

async function ws_talk_in_loop(loop_id, ptt, websocket) {
    let parameter = { loop: loop_id, state: ptt };
    let response;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "signal talking in loop...");
            response = await websocket.send_event(EVENT.TALKING, parameter);
            console.log(log_prefix(websocket) + "signaled talking in loop " + response.loop);
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to signal talking in loop again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "signaling talking in loop failed.", error);
                disconnect(websocket);
                return false;
            }
        }
    }
    return response;
}

function disconnect(websocket) {
    if (websocket.is_connecting) {
        console.warn(log_prefix(websocket) + "Pers error. Disconnect and try again.");
        websocket.disconnect();
    }
}

//-----------------------------------------------------------------------------
// logging
//-----------------------------------------------------------------------------
function log_prefix(ws) {
    return "(" + ws.server_name + " vocs) ";
}