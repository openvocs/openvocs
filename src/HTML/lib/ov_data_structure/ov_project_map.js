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
    @file           ov_project_list.js

    @ingroup        lib

    @brief          Data structure to handel Projects
    	
    ---------------------------------------------------------------------------
*/

import ov_Project from "/lib/ov_object_model/ov_project_model.js";

export default class ov_Project_Map extends Map {
    constructor() {
        super();
    }

    set(project) {
        return super.set(project.id, project);
    }

    sort() {
        let sorted_list = [...this].sort(
            (a, b) => (a[1].domain > b[1].domain) ? 1 : (
                (b[1].domain > a[1].domain) ? -1 : (
                    (a[1].name > b[1].name) ? 1 : (b[1].name > a[1].name) ? -1 : 0)));
        this.clear();
        for (let entry of sorted_list)
            super.set(entry[0], entry[1]);
    }

    toString() {
        return Array.from(this.keys()).toString();
    }

    static parse(json) {
        let projects = new this();
        for (let id of Object.keys(json)) {
            projects.set(ov_Project.parse(id, json[id]));
        }
        return projects;
    }
}