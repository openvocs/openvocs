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
    @file           ov_role_model.js

    @ingroup        lib

    @brief          Blueprint for Role object
    	
    ---------------------------------------------------------------------------
*/
import ov_Parse_Exception from "/lib/ov_exception/ov_parse_exception.js";
import create_uuid from "/lib/ov_utils/ov_uuid.js";

export default class ov_Role {
    constructor(id, name, abbreviation, color, project, domain) {
        this.id = id;
        this.name = name;
        this.abbreviation = abbreviation;
        this.color = color;
        this.project = project;
        this.domain = domain;

        // dom id has to start with a letter, uuid may start with a number
        this._dom_id = "role_" + create_uuid();
    }

    set id(id) {
        this._id = id;
    }

    get id() {
        return this._id;
    }

    set name(name) {
        this._name = name;
    }

    get name() {
        return this._name;
    }

    set abbreviation(abbreviation) {
        this._abbreviation = abbreviation;
    }

    get abbreviation() {
        return this._abbreviation;
    }

    set color(color) {
        this._color = color;
    }

    get color() {
        return this._color;
    }

    set project(project) {
        this._project = project;
    }

    get project() {
        return this._project;
    }

    set domain(domain) {
        this._domain = domain;
    }

    get domain() {
        return this._domain;
    }

    set users(array) {
        this._users = array;
    }

    get users() {
        if (!this._users)
            console.warn("(ov) RoleModel: Array of Users is not set.");
        return this._users;
    }

    get dom_id() {
        return this._dom_id;
    }

    static parse(id, json) {
        if (!id)
            throw new ov_Parse_Exception("no role id");

        let name = json.hasOwnProperty("name") ? json.name : id;

        if (!json.hasOwnProperty("abbreviation")) // optional
            json.abbreviation = name;

        /*if (!json.hasOwnProperty("color")) // optional
            json.color = "#909090"; // grey*/

        return new this(id, name, json.abbreviation, json.color, json.project, json.domain);
    }
}