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
    @file           ov_user_model.js

    @ingroup        lib

    @brief          Blueprint for User object
    	
    ---------------------------------------------------------------------------
*/

import ov_Parse_Exception from "/lib/ov_exception/ov_parse_exception.js";
import create_uuid from "/lib/ov_utils/ov_uuid.js";

export const AUTH_SCOPE = {
    ROLE: "role",
    PROJECT: "project"
}

export const ADMIN_LEVEL = {
    NONE: "none",
    DOMAIN: "domain",
    PROJECT: "project"
}

export default class ov_User {
    #id;
    #name;
    #password;
    #avatar;

    #role;
    #roles;

    #project;
    #projects;

    #domain;
    #domains;

    #admin;

    #dom_id;

    constructor(id, name, avatar, project) {
        this.id = id;
        this.name = name;
        this.avatar = avatar;
        this.project = project;

        this.password = null;

        // dom id has to start with a letter, uuid may start with a number
        this.#dom_id = "user_" + create_uuid();
    }

    set id(id) {
        this.#id = id;
    }

    get id() {
        return this.#id;
    }

    set name(name) {
        this.#name = name;
    }

    get name() {
        return this.#name ? this.#name : this.#id;
    }

    set password(password) {
		this.#password = password;
	}

	get password() {
		return this.#password;
	}

    set avatar(avatar) {
        this.#avatar = avatar;
    }

    get avatar() {
        return this.#avatar;
    }

    set role(role) {
        this.#role = role;
    }

    get role() {
        return this.#role;
    }

    set project(project) {
        this.#project = project;
    }

    get project() {
        return this.#project;
    }

    set domain(domain) {
        this.#domain = domain;
    }

    get domain() {
        return this.#domain;
    }

    set roles(roles) {
        this.#roles = roles;
    }

    get roles() {
        return this.#roles;
    }

    set projects(projects) {
        this.#projects = projects;
    }

    get projects() {
        return this.#projects;
    }

    set domains(domains) {
        this.#domains = domains;
    }

    get domains() {
        return this.#domains;
    }

    set admin(level) {
        if (level !== ADMIN_LEVEL.DOMAIN && level !== ADMIN_LEVEL.PROJECT)
            this.#admin = ADMIN_LEVEL.NONE;
        else
            this.#admin = level;
    }

    get admin() {
        return this.#admin;
    }

	get dom_id(){
		return this.#dom_id;
	}

    static parse(id, json) {
        if (!id)
            throw new ov_Parse_Exception("no id", json);
        let name = json.hasOwnProperty("name") ? json.name : id;
        return new this(id, name, json.avatar, json.project);
    }

    parse_values(json) {
        for(let prop in json){
            this[prop] = json[prop];
        }
    }
}