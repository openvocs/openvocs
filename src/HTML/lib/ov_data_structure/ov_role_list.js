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
    @file           ov_role_list.js

    @author         Anja Bertard, Fabian Junge

    @date           2017-05-19

    @ingroup        lib

    @brief          Data structure to handel Roles

    ---------------------------------------------------------------------------
*/

import ov_Role from "/lib/ov_object_model/ov_role_model.js";

export default class ov_Role_List {
    constructor() {
        this.reset();
    }

    get values() {
        return Array.from(this._roles.values());
    }

    add(role) {
        this._roles.set(role.id, role);
    }

    find(id) {
        return this._roles.get(id);
    }

    sort() {
        this._roles = new Map([...this._roles].sort(
            (a, b) => (a[1].project > b[1].project) ? 1 : (
                (b[1].project > a[1].project) ? -1 : (
                    (a[1].name > b[1].name) ? 1 : (b[1].name > a[1].name) ? -1 : 0))));
    }

    reset() {
        this._roles = new Map();
    }

    toString() {
        return Array.from(this._roles.keys()).toString();
    }

    get length() {
        return this._roles.size;
    }

    static parse(json) {
        let roles = new this();
        for (let id of Object.keys(json)) {
            roles.add(ov_Role.parse(id, json[id]));
        }
        return roles;
    }
}