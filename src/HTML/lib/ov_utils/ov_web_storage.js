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
    @file           ov_web_storage.js

    @ingroup        ov_lib

    @brief          save to local web storage

    ---------------------------------------------------------------------------
*/

const SESSION_ID_DURATION = 3000000;//ms = 50min

/* based on https://www.sohamkamani.com/javascript/localstorage-with-ttl-expiry/ */
export function set_session(url, client_id, user_id, session_id) {
    console.log("set session", url, client_id, user_id, session_id);
    let current_time = new Date().getTime();
    let item = {
        user: user_id,
        session: session_id,
        client: client_id,
        expiration: current_time + SESSION_ID_DURATION
    }
    localStorage.setItem(url, JSON.stringify(item));
}

/* based on https://www.sohamkamani.com/javascript/localstorage-with-ttl-expiry/ */
export function get_session(url) {
    let item_string = localStorage.getItem(url);
    if (!item_string)
        return null;
    let item = JSON.parse(item_string);
    let current_time = new Date().getTime();
    if (current_time > item.expiration) {
        localStorage.removeItem(url);
        return null;
    }
    console.log("get session", url, item);
    return { user: item.user, session: item.session, client: item.client, role: item.role, domain: item.domain, project: item.project, page: item.page };
}

export function extend_session(url, client_id, user_id, session_id) {
    let session = get_session(url);
    if (session === null)
        set_session(url, client_id, user_id, session_id);
    else {
        let current_time = new Date().getTime();
        session.user = user_id;
        session.session = session_id;
        session.client = client_id;
        session.expiration = current_time + SESSION_ID_DURATION;
        localStorage.setItem(url, JSON.stringify(session));
        console.log("new session information", url, session);
    }
}

export function add_role_to_session(url, role_id) {
    let item_string = localStorage.getItem(url);
    if (!item_string)
        return null;
    let item = JSON.parse(item_string);
    item.role = role_id;
    console.log("new session information", url, item);
    localStorage.setItem(url, JSON.stringify(item));
}

export function add_anchor_to_session(url, domain, project, page) {
    let item_string = localStorage.getItem(url);
    if (!item_string)
        return null;
    let item = JSON.parse(item_string);
    item.project = project;
    item.domain = domain;
    item.page = page;
    console.log("new session information", url, item);
    localStorage.setItem(url, JSON.stringify(item));
}

export function clear(url) {
    console.log("clear session", url);
    localStorage.removeItem(url);
}