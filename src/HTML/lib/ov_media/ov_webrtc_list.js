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
    @file           ov_webrtc_list.js

    @ingroup        ov_lib/ov_media

    @brief          an array of ov_webrtc connections

    ---------------------------------------------------------------------------
*/
import * as ov_Websockets from "/lib/ov_websocket_list.js";
import ov_WebRTC from "/lib/ov_media/ov_webrtc.js";

export var list;

export function setup_connections() {
    list = new Map();

    for (let ws of ov_Websockets.list) {
        let media = new ov_WebRTC(ws);
        media.on_disconnect(() => {
            console.log(log_prefix(ws) + "media disconnected");
            if (ws.is_connecting)
                ws.disconnect();
        });
        list.set(ws.client_id, media);
    }
}

export function get(websocket) {
    return list.get(websocket.client_id);
}

export function get_lead() {
    return list.get(ov_Websockets.current_lead_websocket.client_id);
}

export function mute_outbound_stream(mute, force) {
    ov_WebRTC.local_stream.mute = mute;
    if (SHARED_MULTICAST_ADDRESSES && !force) {
        get_lead().mute_outbound_stream(mute);
    } else {
        for (let media of list.values()) {
            media.mute_outbound_stream(mute);
        }
    }
}

export function local_stream() {
    return ov_WebRTC.local_stream;
}

export async function create_local_media_stream(){
    await ov_WebRTC.create_local_media_stream();
}

function log_prefix(websocket) {
    return "(" + websocket.server_name + " webRTC) ";
}