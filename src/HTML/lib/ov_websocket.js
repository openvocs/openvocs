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
    static EVENT = {
        EVENTS: "get_events",

        //auth events
        LOGIN: "login",
        EXTEND_SESSION: "extend_login_session",
        AUTHORIZE_ROLE: "authorize",
        LOGOUT: "logout",

        //system events
        REGISTER: "register",
        BROADCAST: "broadcast",
        LDAP_CHECK: "is_ldap_enabled",
        SIP: "sip_is_enabled",

        //DB events
        CHECK_ID: "db_check_id_exists",
        VERIFY: "db_verify",

        CREATE: "db_create", //project, user, role, loop
        LDAP_IMPORT: "db_ldap_import",

        UPDATE: "db_update", //project, user, role, loop
        UPDATE_KEY: "db_update_key",
        UPDATE_PASSWORD: "db_update_password",

        DELETE: "db_delete", //project, user, role, loop
        DELETE_KEY: "db_delete_key",

        GET: "db_get", //domain, project, user details
        GET_KEY: "db_get_key",
        USER_ROLES: "db_get_user_roles",
        LOOPS: "db_get_all_loops",
        ROLE_LOOPS: "db_get_role_loops",
        ADMIN_DOMAINS: "db_get_admin_domains",
        ADMIN_PROJECTS: "db_get_admin_projects",
        HIGHEST_MULTICAST_PORT: "db_get_highest_port",

        SET_KEYSET_LAYOUT: "db_set_keyset_layout",
        KEYSET_LAYOUT: "db_get_keyset_layout",

        UPDATE_USER_SETTINGS: "db_set_user_data",
        USER_SETTINGS: "db_get_user_data",

        PERSIST: "db_save",
        LOAD_DB_CONTENT: "db_load"
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
        CONNECTING: 1,
        CONNECTED: 2,
        AUTHENTICATING: 3,
        AUTHENTICATED: 4,
        AUTHORIZING: 5,
        AUTHORIZED: 6
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
    #session;

    #resend_events;

    #log_incoming_events;
    #log_outgoing_events;

    #pending_requests;

    #event_target;

    #record;

    constructor(name, url, session, record) {
        this.#name = name;
        this.#url = url;
        this.#session = session;
        this.#client_id = session ? session.client : create_uuid();
        this.resend_events_after_timeout = false;
        this.log_incoming_events = false;
        this.log_outgoing_events = false;
        this.#ws_state = ov_Websocket.WEBSOCKET_STATE.DISCONNECTED;
        this.#pending_requests = new Set();
        this.#record = record;

        this.#event_target = new EventTarget();
        this.#event_target.websocket = this;
        this.addEventListener = this.#event_target.addEventListener.bind(this.#event_target);
        this.removeEventListener = this.#event_target.removeEventListener.bind(this.#event_target);
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

    get websocket_url() {
        return this.#url;
    }

    get disconnected() {
        return this.#ws_state === ov_Websocket.WEBSOCKET_STATE.DISCONNECTED;
    }

    get connecting() {
        return this.#ws_state === ov_Websocket.WEBSOCKET_STATE.CONNECTING;
    }

    get connected() {
        return this.#ws_state > ov_Websocket.WEBSOCKET_STATE.CONNECTING;
    }

    get authenticated() {
        return this.#ws_state > ov_Websocket.WEBSOCKET_STATE.AUTHENTICATING;
    }

    get authorized() {
        return this.#ws_state > ov_Websocket.WEBSOCKET_STATE.AUTHORIZING;
    }

    get server_error() {
        return this.#error;
    }

    get user() {
        return this.#user;
    }

    get record() {
        return this.#record;
    }

    get session() {
        if (this.#session) {
            let current_time = new Date().getTime();
            if (current_time > this.#session.expiration)
                this.#session = null;
        }
        return this.#session;
    }

    //-----------------------------------------------------------------------------
    // websocket
    //-----------------------------------------------------------------------------
    connect() {
        return new Promise((resolve, reject) => {
            if (!this.connecting && !this.connected) {
                console.log(this.#log_prefix() + "client id: " + this.#client_id);
                console.log(this.#log_prefix() + "connect to websocket " + this.#url);
                this.#pending_requests = new Set();

                let error;
                this.#ws_state = ov_Websocket.WEBSOCKET_STATE.CONNECTING;
                this.#websocket = new WebSocket(this.#url);

                this.#websocket.onclose = (event) => {
                    this.#handel_close_websocket(event, error);
                    reject(false);
                };

                this.#websocket.onmessage = (response) => {
                    this.#handle_websocket_event(JSON.parse(response.data));
                };

                this.#websocket.onopen = async () => {
                    console.log(this.#log_prefix() + "connected");
                    this.#ws_state = ov_Websocket.WEBSOCKET_STATE.CONNECTED;
                    this.#event_target.dispatchEvent(new CustomEvent("connected"));
                    this.#error = undefined;
                    if (BROADCAST_REGISTRATION)
                        this.send_event(ov_Websocket.EVENT.REGISTER);
                    resolve(true);
                };

                this.#websocket.onerror = (error) => {
                    console.error(this.#log_prefix() + "websocket error");
                    this.#ws_state = ov_Websocket.WEBSOCKET_STATE.DISCONNECTED;
                    error = { description: "Websocket Error" };
                    reject(false);
                };
            } else
                resolve(this.connected);
        });
    }

    disconnect() {
        console.log(this.#log_prefix() + "disconnect websocket");
        this.#websocket.close();
    }

    #handel_close_websocket(event, error) {
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

        this.#event_target.dispatchEvent(new CustomEvent("disconnected", { detail: error }));

    }

    //-----------------------------------------------------------------------------
    // response handling
    //-----------------------------------------------------------------------------
    async #handle_websocket_event(event) {
        if (this.#log_incoming_events) {
            if (event.event === ov_Websocket.EVENT.LOGIN)
                event.request.parameter.password = "***";
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
            console.error(this.#log_prefix() + "Error " + this.#error.code + ": " + this.#error.description);

            if (error.code === 5000) { // Auth error
                console.log("(" + this.#url + ") clear session");
                ov_Web_Storage.clear(APP, this.#url);
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

        let message = await this.#process_incoming_event(event, error);
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
                    event.parameter.password = "***";
                console.log(this.#log_prefix() + "outgoing event", JSON.stringify(event));
            }
            if (event.event === ov_Websocket.EVENT.LOGIN)
                this.#ws_state = ov_Websocket.WEBSOCKET_STATE.AUTHENTICATING;
            if (event.event === ov_Websocket.EVENT.AUTHORIZE_ROLE)
                this.#ws_state = ov_Websocket.WEBSOCKET_STATE.AUTHORIZING;
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

            if (!this.connected)
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
    async #process_incoming_event(event, error) {
        let message = !!event.response || event.response === false ? event.response : event.parameter;

        if (event.type !== ov_Websocket.MESSAGE_TYPE.LOOP_BROADCAST) {
            if (typeof message !== "object")
                message = { response: message };
            message["client"] = event.client;
        }

        switch (event.event) {
            case ov_Websocket.EVENT.LOGIN:
                if (!error) {
                    this.#ws_state = ov_Websocket.WEBSOCKET_STATE.AUTHENTICATED;
                    this.#session = ov_Web_Storage.extend_session(APP, this.#url, this.#client_id, this.#user.id, message.session);
                    clearInterval(this.#extend_session_interval_id);
                    this.#extend_session_interval_id = setInterval(async () => {
                        if (this.session === null) {
                            console.warn("(" + this.server_name + ") session not found or expired.");
                        } else {
                            try {
                                console.log("(" + this.server_name + ") extend session...");
                                let result = await this.send_event(ov_Websocket.EVENT.EXTEND_SESSION, { session: this.#session.session, user: this.#user.id });
                                this.#session = ov_Web_Storage.extend_session(APP, this.#url, this.#client_id, this.#user.id, result.session);
                                console.log("(" + this.server_name + ") extended session");
                            } catch (error) {
                                console.warn("(" + this.server_name + ") extend session failed.", error);
                            }
                        }

                    }, ov_Websocket.#EXTEND_SESSION_INTERVAL);
                } else
                    this.#ws_state = ov_Websocket.WEBSOCKET_STATE.CONNECTED;
                break;
            case ov_Websocket.EVENT.AUTHORIZE_ROLE:
                if (!error) {
                    this.#ws_state = ov_Websocket.WEBSOCKET_STATE.AUTHORIZED;
                    this.#user.role = message.id;
                    if (!this.#user.roles)
                        await this.send_event(ov_Websocket.EVENT.USER_ROLES);
                    if (this.#user.roles) {
                        let role = this.#user.roles.find(this.#user.role);
                        if (role && role.project)
                            this.#user.project = role.project;
                    }
                    this.#session = ov_Web_Storage.add_role_to_session(APP, this.#url, this.#user.role);
                } else
                    this.#ws_state = ov_Websocket.WEBSOCKET_STATE.AUTHENTICATED;
                break;
            case ov_Websocket.EVENT.LOGOUT:
                this.#ws_state = ov_Websocket.WEBSOCKET_STATE.DISCONNECTED;
                break;

            case ov_Websocket.EVENT.GET:
                if (message.type === ov_Websocket.REQUEST_SCOPE.USER && message.data.id === this.#user.id) {
                    this.#user.parse_values(message.data);
                    // if (message.data.domain) {
                    //     this.#user.domain = message.data.domain.domain;
                    //     if (!this.#user.project)
                    //         this.#user.project = message.data.domain.project;
                    // }
                }
                break;
            case ov_Websocket.EVENT.ADMIN_DOMAINS:
                this.#user.domains = ov_Domain_Map.parse(message.domains);
                break;
            case ov_Websocket.EVENT.ADMIN_PROJECTS:
                this.#user.projects = ov_Project_Map.parse(message.projects);
                break;
            case ov_Websocket.EVENT.USER_ROLES:
                this.#user.roles = ov_Role_List.parse(message.roles);
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
        ov_Web_Storage.clear(APP, this.websocket_url);
        this.#session = null;
        if (this.connected || this.authenticated)
            return this.#request(this.#create_event(ov_Websocket.EVENT.LOGOUT));
        return true;
    }

    //-----------------------------------------------------------------------------
    // logging
    //-----------------------------------------------------------------------------
    #log_prefix() {
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