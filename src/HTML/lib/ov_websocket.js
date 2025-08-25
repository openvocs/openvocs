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
    @file           ov_Websocket.js

    @ingroup        ov_lib

    @brief          implements openvocs signaling protocol and wraps it
                    around a websocket

    ---------------------------------------------------------------------------
*/
import create_uuid from "./ov_utils/ov_uuid.js";

import * as ov_Web_Storage from "./ov_utils/ov_web_storage.js";
import ov_User from "./ov_object_model/ov_user_model.js";
import ov_Role_List from "./ov_data_structure/ov_role_list.js";
import ov_Project_Map from "./ov_data_structure/ov_project_map.js";
import ov_Domain_Map from "./ov_data_structure/ov_domain_map.js";

export default class ov_Websocket {
    // core events
    static EVENT = {
        REGISTER: "register",
        LOGIN: "login",
        EXTEND_SESSION: "update_login",
        AUTHORIZE_ROLE: "authorize",
        LOGOUT: "logout",

        GET: "get", //domain, project, user details
        USER_ROLES: "user_roles",
        ADMIN_DOMAINS: "admin_domains",
        ADMIN_PROJECTS: "admin_projects",

        BROADCAST: "broadcast"
    };

    static REQUEST_SCOPE = {
        DOMAIN: "domain",
        PROJECT: "project",
        USER: "user",
        LOOP: "loop",
        ROLE: "role"
    };

    static MESSAGE_TYPE = {
        UNICAST: "unicast",
        LOOP_BROADCAST: "loop_broadcast",
        USER_BROADCAST: "user_broadcast"
    };

    static WEBSOCKET_STATE = {
        DISCONNECTED: 0,
        CONNECTED: 1,
        AUTHENTICATED: 2,
        AUTHORIZED: 3
    }

    static #EXTEND_SESSION_INTERVAL = 1800000; // 30min
    #extend_session_interval_id;

    #client_id;

    #user;

    #name;
    #url;
    #error;

    #websocket = null;
    #ws_state;

    #resend_events;

    #log_incoming_events;
    #log_outgoing_events;

    #pending_requests;

    #event_target;

    #show_port = false;

    constructor(name, url, client_id) {
        this.#name = name;
        this.#url = url;
        this.#client_id = client_id ? client_id : create_uuid();
        this.resend_events_after_timeout = false;
        this.log_incoming_events = false;
        this.log_outgoing_events = false;
        this.#ws_state = ov_Websocket.WEBSOCKET_STATE.DISCONNECTED;
        this.#pending_requests = new Set();

        this.#event_target = new EventTarget();
        this.addEventListener = this.#event_target.addEventListener.bind(this.#event_target);
        this.removeEventListener = this.#event_target.removeEventListener.bind(this.#event_target);

        if (this.port !== "vocs")
            this.#show_port = true;
    }

    /**
    * @param {boolean} val
    */
    set resend_events_after_timeout(val) {
        this.#resend_events = !!val;
    }

    /**
     * @param {boolean} incoming
     */
    set log_incoming_events(incoming) {
        this.#log_incoming_events = incoming;
    }

    /**
     * @param {boolean} outgoing
     */
    set log_outgoing_events(outgoing) {
        this.#log_outgoing_events = outgoing;
    }

    get client_id() {
        return this.#client_id;
    }

    get server_name() {
        return this.#name;
    }

    get server_url() {
        return "https://" + this.#url.slice(6, this.#url.lastIndexOf("/") + 1); // remove wss:// and /vocs from websocket url
    }

    get port() {
        return this.#url.slice(this.#url.lastIndexOf("/") + 1);
    }

    get websocket_url() {
        return this.#url;
    }

    get authenticated() {
        return this.#ws_state === ov_Websocket.WEBSOCKET_STATE.AUTHENTICATED || this.authorized;
    }

    get authorized() {
        return this.#ws_state === ov_Websocket.WEBSOCKET_STATE.AUTHORIZED;
    }

    get server_error() {
        return this.#error;
    }

    get user() {
        return this.#user;
    }

    get tasks_running() {
        if (!this.is_connecting)
            return -1;
        return this.#pending_requests.size;
    }

    //-----------------------------------------------------------------------------
    // websocket
    //-----------------------------------------------------------------------------
    connect() {
        return new Promise((resolve, reject) => {
            if (!this.is_connecting) {
                console.log(this.#log_prefix() + "client id: " + this.#client_id);
                console.log(this.#log_prefix() + "connect to websocket " + this.#url);
                this.#pending_requests = new Set();

                this.#websocket = new WebSocket(this.#url);

                this.#websocket.onclose = (event) => {
                    console.log(this.#log_prefix() + "close")
                    this.#handel_close_websocket(event);
                    reject(false);
                };

                this.#websocket.onmessage = (response) => {
                    this.#handle_websocket_event(JSON.parse(response.data));
                };

                this.#websocket.onopen = () => {
                    console.log(this.#log_prefix() + "connected");
                    this.#ws_state = ov_Websocket.WEBSOCKET_STATE.CONNECTED;
                    this.#event_target.dispatchEvent(new CustomEvent("connected"));
                    this.#error = undefined;
                    resolve(true);
                };

                this.#websocket.onerror = (error) => {
                    console.error(this.#log_prefix() + "websocket error", error);
                    this.#ws_state = ov_Websocket.WEBSOCKET_STATE.DISCONNECTED;
                    this.#error = { description: "Websocket Error. Websocket closed with", code: 1006 };
                    reject(false);
                };
            } else
                resolve(this.is_ready);
        });
    }



    disconnect() {
        console.log(this.#log_prefix() + "disconnect websocket");
        this.#websocket.close();
    }

    get is_connecting() {
        return this.#websocket && (this.#websocket.readyState === WebSocket.CONNECTING ||
            this.#websocket.readyState === WebSocket.OPEN);
    }

    get is_ready() {
        return this.#websocket && this.#websocket.readyState === WebSocket.OPEN;
    }

    #handel_close_websocket(event) {
        if (event)
            console.warn(this.#log_prefix() + "Websocket closed. Code: " + event.code +
                ", reason: " + event.reason, + ", clean: " + event.wasClean);
        else
            console.warn(this.#log_prefix() + "Websocket closing. Triggered by client.");
        this.#ws_state = ov_Websocket.WEBSOCKET_STATE.DISCONNECTED;

        if (this.#user) {
            let password = this.#user.password;
            let id = this.#user.id;
            let role = this.#user.role;
            this.#user = new ov_User(id);
            if (password)
                this.#user.password = password;
            this.#user.role = role;
        }
        this.#pending_requests = new Set();

        clearInterval(this.#extend_session_interval_id);

        this.#event_target.dispatchEvent(new CustomEvent("disconnected"));

    }

    //-----------------------------------------------------------------------------
    // response handling
    //-----------------------------------------------------------------------------
    #handle_websocket_event(event) {
        if (this.#log_incoming_events) {
            if (event.event === ov_Websocket.EVENT.LOGIN)
                console.log(this.#log_prefix() + "incoming event: LOGIN (content hidden)");
            else if (event.event === "ldap_import")
                console.log(this.#log_prefix() + "incoming event: LDAP IMPORT (content hidden)");
            else
                console.log(this.#log_prefix() + "incoming event", JSON.stringify(event));
        }

        if (!event.hasOwnProperty("event")) {
            console.error(this.#log_prefix() + "No event ID in incoming event.");
            return;
        }

        let sender = {
            message_id: event.uuid,
            client: event.client,
            message_type: event.type
        }

        if (event.type === ov_Websocket.MESSAGE_TYPE.LOOP_BROADCAST ||
            event.type === ov_Websocket.MESSAGE_TYPE.USER_BROADCAST) {
            if (event.parameter)
                sender.client = event.parameter.client;
        }

        let error;
        this.#error = undefined;
        if (event.hasOwnProperty("error")) {
            this.#error = event.error;
            error = event.error;
            error.temp_error = error.code >= 50000;
            console.error(this.#log_prefix() + "Error " + this.#error.code + ": " + this.#error.description);

            if (error.code === 5000) { // Auth error
                console.log("(" + this.#url + ") clear session");
                ov_Web_Storage.clear(this.#url);
            }
        } else if (!event.hasOwnProperty("response") && !event.hasOwnProperty("parameter")) {
            console.error(this.#log_prefix() + "received no response or parameter in " + event.event);
            if (this.#client_id === event.client) {// answer to request
                if (!error)
                    error = new Format_Error();
                this.#event_target.dispatchEvent(new CustomEvent(event.event, { detail: { message: null, sender: sender, error: error } }));
            }
            return;
        }

        let message = this.#process_incoming_event(event, error);
        if (error)
            this.#event_target.dispatchEvent(new CustomEvent(event.event, { detail: { message: message, sender: sender, error: error } }));
        else
            this.#event_target.dispatchEvent(new CustomEvent(event.event, { detail: { message: message, sender: sender } }));
    }

    //-----------------------------------------------------------------------------
    // request handling
    //-----------------------------------------------------------------------------
    #send_event(event) {
        if (this.#websocket.readyState === WebSocket.OPEN) {
            let message = JSON.stringify(event);
            if (this.#log_outgoing_events) {
                if (event.event === ov_Websocket.EVENT.LOGIN)
                    console.log(this.#log_prefix() + "outgoing event: LOGIN (content hidden)");
                else if (event.event === "ldap_import")
                    console.log(this.#log_prefix() + "incoming event: LDAP IMPORT (content hidden)");
                else
                    console.log(this.#log_prefix() + "outgoing event", message);
            }
            this.#pending_requests.add(event.uuid);
            this.#websocket.send(message);
        } else
            console.error(this.#log_prefix() + "WebSocket is not open - WebSocket readyState is " +
                this.#websocket.readyState);
    }

    #create_event(event_name, parameter) {
        let event = {
            event: event_name,
            uuid: create_uuid(),
            client: this.#client_id,
            type: ov_Websocket.MESSAGE_TYPE.UNICAST
        };
        if (!parameter)
            parameter = {};
        event.parameter = parameter;
        return event;
    }

    #request(event) {
        return new Promise((resolve, reject) => {
            let timeout;
            let timeout_handler = () => {
                if (this.#resend_events) {
                    console.warn(this.#log_prefix() + "Still waiting for response. Resending event...",
                        event.event, event.uuid);
                    this.#send_event(event);
                    if (!!SIGNALING_REQUEST_TIMEOUT)
                        timeout = setTimeout(timeout_handler, SIGNALING_REQUEST_TIMEOUT);
                } else if (event.event !== ov_Websocket.EVENT.LOGOUT) {
                    console.warn(this.#log_prefix() + "Still waiting for response...", event.event, event.uuid);
                }
            }

            let event_handler = (ev) => {
                if (ev.detail.sender.client === event.client && ev.detail.sender.message_id === event.uuid &&
                    ev.detail.sender.message_type === ov_Websocket.MESSAGE_TYPE.UNICAST || ev.detail.sender.message_type === undefined) {
                    if (timeout)
                        clearTimeout(timeout);
                    this.removeEventListener(event.event, event_handler);
                    this.removeEventListener("disconnected", disconnect_handler);
                    this.#pending_requests.delete(event.uuid);
                    if (!ev.detail.error) {//error from server
                        resolve(ev.detail.message);
                    } else
                        reject({ error: ev.detail.error, response: ev.detail.message });
                }
            }

            let disconnect_handler = () => {
                this.removeEventListener(event.event, event_handler);
                this.removeEventListener("disconnected", disconnect_handler);
                reject({ error: "server disconnected" });
            }

            if (!this.is_ready)
                reject({ error: "not connected to server" });

            this.addEventListener(event.event, event_handler);
            this.addEventListener("disconnected", disconnect_handler);
            this.#send_event(event);

            if (!!SIGNALING_REQUEST_TIMEOUT)
                timeout = setTimeout(timeout_handler, SIGNALING_REQUEST_TIMEOUT);
        });
    }

    //-----------------------------------------------------------------------------
    // implementation of ov signaling protocol
    //-----------------------------------------------------------------------------
    #process_incoming_event(event, error) {
        let message = !!event.response ? event.response : event.parameter;

        if (event.type !== ov_Websocket.MESSAGE_TYPE.LOOP_BROADCAST)
            message["client"] = event.client;

        switch (event.event) {
            case ov_Websocket.EVENT.LOGIN:
                if (!error) {
                    this.#ws_state = ov_Websocket.WEBSOCKET_STATE.AUTHENTICATED;
                    let session = !message.session ? event.session : message.session;
                    ov_Web_Storage.extend_session(this.#url, this.#client_id, this.#user.id, session);
                    clearInterval(this.#extend_session_interval_id);
                    this.#extend_session_interval_id = setInterval(async () => {
                        let session = ov_Web_Storage.get_session(this.#url);
                        if (session === null) {
                            console.warn("(" + this.server_name + ") session not found or expired.");
                        } else {
                            try {
                                console.log("(" + this.server_name + ") extend session...");
                                let result = await this.send_event(ov_Websocket.EVENT.EXTEND_SESSION, { session: session.session, user: this.#user.id });
                                ov_Web_Storage.extend_session(this.#url, this.#client_id, this.#user.id, result.session);
                                console.log("(" + this.server_name + ") extended session");
                            } catch (error) {
                                console.warn("(" + this.server_name + ") extend session failed.", error);
                            }
                        }

                    }, ov_Websocket.#EXTEND_SESSION_INTERVAL);
                }
                break;
            case ov_Websocket.EVENT.AUTHORIZE_ROLE:
                if (!error) {
                    this.#ws_state = ov_Websocket.WEBSOCKET_STATE.AUTHORIZED;
                    this.#user.role = event.response.id;
                    ov_Web_Storage.add_role_to_session(this.#url, this.#user.role);
                }
                message = event.response.id;
                break;
            case ov_Websocket.EVENT.LOGOUT:
                this.#ws_state = ov_Websocket.WEBSOCKET_STATE.DISCONNECTED;
                break;

            case ov_Websocket.EVENT.GET:
                if (event.response.type === ov_Websocket.REQUEST_SCOPE.USER) {
                    this.#user.parse_values(event.response.result);
                    if (event.response.domain) {
                        this.#user.domain = event.response.domain.domain;
                        this.#user.project = event.response.domain.project;
                    }
                    message = this.#user;
                } else if (event.response.type === ov_Websocket.REQUEST_SCOPE.PROJECT)
                    message = event.response.result;
                break;

            case ov_Websocket.EVENT.ADMIN_DOMAINS:
                this.#user.domains = ov_Domain_Map.parse(event.response.domains);
                break;
            case ov_Websocket.EVENT.ADMIN_PROJECTS:
                this.#user.projects = ov_Project_Map.parse(event.response.projects);
                break;
            case ov_Websocket.EVENT.USER_ROLES:
                this.#user.roles = ov_Role_List.parse(event.response.roles);
                message = this.#user.roles;
                break;
        }
        return message;
    }

    // authentication and authorization -------------------------------------------
    send_event(event_name, parameter) {
        return this.#request(this.#create_event(event_name, parameter));
    }

    login(user_id, password) {
        let parameter = {
            user: user_id,
            password: password
        };

        this.#user = new ov_User(user_id);

        return this.#request(this.#create_event(ov_Websocket.EVENT.LOGIN, parameter));
    }

    logout() {
        ov_Web_Storage.clear(this.websocket_url);
        if (this.is_ready || this.authenticated)
            return this.#request(this.#create_event(ov_Websocket.EVENT.LOGOUT));
        return true;
    }

    //-----------------------------------------------------------------------------
    // logging
    //-----------------------------------------------------------------------------
    #log_prefix() {
        if (this.#show_port)
            return "(" + this.server_name + "/" + this.port + " websocket) ";
        return "(" + this.server_name + " websocket) ";
    }
}

//-----------------------------------------------------------------------------
// Error handling
//-----------------------------------------------------------------------------
export class Support_Error extends Error {
    constructor(message) {
        message = message ? message :
            "Event not supported. Client can't process event.";
        super(message);
        this.name = "Support_Error";
    }
}

export class Format_Error extends Error {
    constructor(message) {
        message = message ? message : "Event format is wrong.";
        super(message);
        this.name = "Format_Error";
    }
}