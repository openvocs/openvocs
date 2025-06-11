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
    @file           test/ov_websocket_dummy.js

    @author         Anja Bertard

    @date           2023-08-17

    @ingroup        ov_lib

    @brief          implements a stub ov_websocket for testing purpose

    ---------------------------------------------------------------------------
*/
import create_uuid from "/lib/ov_utils/ov_uuid.js";

import ov_User from "/lib/ov_object_model/ov_user_model.js";
import ov_Domain from "/lib/ov_object_model/ov_domain_model.js";
import ov_Project from "/lib/ov_object_model/ov_project_model.js";
import ov_Role from "/lib/ov_object_model/ov_role_model.js";

import ov_Role_List from "/lib/ov_data_structure/ov_role_list.js";
import ov_Project_Map from "/lib/ov_data_structure/ov_project_map.js";
import ov_Domain_Map from "/lib/ov_data_structure/ov_domain_map.js";

export default class ov_Websocket {
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
        VAD: "vad",

        CREATE: "create", //project, user, role, loop
        UPDATE: "update", //project, user, role, loop
        DELETE: "delete", //project, user, role, loop
        CHECK_ID: "check_id_exists",
        LDAP_IMPORT: "ldap_import",
        UPDATE_PASSWORD: "update_password",
        SET_KEYSET_LAYOUT: "set_keyset_layout",
        PERSIST: "save"
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

    #client_id;

    #user;

    #name;
    #url;
    #error;

    #ws_state;

    #event_target;

    constructor(name, url) {
        this.#name = name;
        this.#url = url;
        this.#client_id = create_uuid();
        this.resend_events_after_timeout = false;
        this.log_incoming_events = false;
        this.log_outgoing_events = false;
        this.#ws_state = ov_Websocket.WEBSOCKET_STATE.DISCONNECTED;
        console.warn("USING DUMMY WEBSOCKET");

        this.#event_target = new EventTarget();
        this.addEventListener = this.#event_target.addEventListener.bind(this.#event_target);
        this.removeEventListener = this.#event_target.removeEventListener.bind(this.#event_target);
    }

    /**
    * @param {boolean} val
    */
    set resend_events_after_timeout(val) {
    }

    /**
     * @param {boolean} incoming
     */
    set log_incoming_events(incoming) {
    }

    /**
     * @param {boolean} outgoing
     */
    set log_outgoing_events(outgoing) {
    }

    get client_id() {
        return this.#client_id;
    }

    get server_name() {
        return this.#name;
    }

    get server_url() {
        return this.#url.slice(6, -5); // remove wss:// and /vocs from websocket url
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

    //-----------------------------------------------------------------------------
    // websocket
    //-----------------------------------------------------------------------------
    connect() {
        if (!this.is_connecting) {
            console.log("(" + this.#name + " gateway) connected");
            this.#ws_state = ov_Websocket.WEBSOCKET_STATE.CONNECTED;
            this.#event_target.dispatchEvent(new CustomEvent("connected"));
        }
        return true;
    }

    disconnect() {
        console.log("(" + this.#name + " gateway) disconnect websocket");
        this.#ws_state = ov_Websocket.WEBSOCKET_STATE.DISCONNECTED;
        this.#event_target.dispatchEvent(new CustomEvent("disconnected"));
    }

    get is_connecting() {
        return !ov_Websocket.WEBSOCKET_STATE.DISCONNECTED;
    }

    get is_ready() {
        return !ov_Websocket.WEBSOCKET_STATE.DISCONNECTED;
    }

    login(user_id, password) {
        this.#user = new ov_User(user_id);
        this.#user.password = password;

        this.#ws_state = ov_Websocket.WEBSOCKET_STATE.AUTHENTICATED;
        return true;
    }

    async send_event(event_name, parameter) {
        let message = event_name;
        switch (event_name) {
            case ov_Websocket.EVENT.AUTHORIZE_ROLE:
                this.#ws_state = ov_Websocket.WEBSOCKET_STATE.AUTHORIZED;
                this.#user.role = "r1";
                break;
            case ov_Websocket.EVENT.LOGOUT:
                this.#ws_state = ov_Websocket.WEBSOCKET_STATE.DISCONNECTED;
                break;

            case ov_Websocket.EVENT.GET:
                switch (parameter.type) {
                    case ov_Websocket.REQUEST_SCOPE.USER:
                        this.#user.name = "Name Lastname";
                        message = this.#user;
                        break;
                    case ov_Websocket.REQUEST_SCOPE.PROJECT: {
                        const response = await fetch("/lib/data/demoCOL2.json");
                        message = await response.json();
                        break;
                    } case ov_Websocket.REQUEST_SCOPE.DOMAIN: {
                        const response = await fetch("/lib/data/demo.json");
                        message = { result: await response.json() };
                        break;
                    }
                }
                break;
            case ov_Websocket.EVENT.CREATE:
                break;
            case ov_Websocket.EVENT.UPDATE:
                break;
            case ov_Websocket.EVENT.DELETE:

                break;

            case ov_Websocket.EVENT.ADMIN_DOMAINS:
                this.#user.domains = new ov_Domain_Map();
                this.#user.domains.set(new ov_Domain("domain_1"));
                this.#user.domains.set(new ov_Domain("domain_0"));
                this.#user.domains.set(new ov_Domain("domain_3"));
                break;
            case ov_Websocket.EVENT.ADMIN_PROJECTS:
                this.#user.projects = new ov_Project_Map();
                this.#user.projects.set(new ov_Project("p1_1", "Project D1 1", "domain_1"));
                this.#user.projects.set(new ov_Project("p2_1", "Project D1 2", "domain_1"));
                this.#user.projects.set(new ov_Project("p0_1", "Project D1 0", "domain_1"));
                this.#user.projects.set(new ov_Project("p1_2", "Project D2 1", "domain_2"));
                this.#user.projects.set(new ov_Project("p1_0", "Project D0 1", "domain_0"));
                this.#user.projects.set(new ov_Project("p0_0", "Project D0 0", "domain_0"));
                break;
            case ov_Websocket.EVENT.USER_ROLES:
                this.#user.roles = new ov_Role_List();
                this.#user.roles.add(new ov_Role("r1", "role 1", "r1", undefined, "p1_1", "domain_1"));
                this.#user.roles.add(new ov_Role("r3", "role 3", "r3", undefined, "p1_1", "domain_1"));
                this.#user.roles.add(new ov_Role("r2", "role 2", "r2", undefined, "p1_2", "domain_2"));
                this.#user.roles.add(new ov_Role("r0", "role 0", "r0", undefined, "p1_1", "domain_1"));
                break;
            case ov_Websocket.EVENT.ROLE_LOOPS:
                const response = await fetch("/lib/data/demo.json");
                message = await response.json();
                console.log(message)
                break;
            case ov_Websocket.EVENT.KEYSET_LAYOUT:
                message = { layout: { site_scaling: 1.0, name_scaling: 1.5, font_scaling: 1.0, layout: "auto" } };
                break;
            case ov_Websocket.EVENT.CHECK_ID:
                switch (parameter.scope) {
                    case ov_Websocket.REQUEST_SCOPE.PROJECT:
                        message = { result: this.#user.projects.has(parameter.id) };
                        break;
                    case ov_Websocket.REQUEST_SCOPE.PROJECT:
                        const response = await fetch("/lib/data/demoCOL2.json");
                        message = await response.json();
                        break;
                }
                break;
            case ov_Websocket.EVENT.SIP_CALL:
                let call_id = create_uuid();
                message = { call_id: call_id }
                setTimeout(() => {
                    this.#event_target.dispatchEvent(new CustomEvent("call", {
                        detail: {
                            sender: {
                                message_type: "loop_broadcast"
                            },
                            new_call: {
                                "call-id": call_id,
                                loop: parameter.loop
                            }
                        }
                    }));
                }, 1000);
                break;
            case ov_Websocket.EVENT.SIP_HANGUP:
                setTimeout(() => {
                    this.#event_target.dispatchEvent(new CustomEvent("hangup", {
                        detail: {
                            sender: {
                                message_type: "loop_broadcast"
                            },
                            call: {
                                "call-id": parameter.call,
                                loop: parameter.loop
                            }
                        }
                    }))
                }, 1000);
                break;
            default:
                message = null;
                console.error("(" + this.#name + " gateway) Event " + event_name + " is not supported");
        }
        //return message;
        return new Promise((resolve, reject) => {
            if (event_name === ov_Websocket.EVENT.DELETE)
                reject({ error: error, response: message });
            else
                resolve(message);
        });
    }
}