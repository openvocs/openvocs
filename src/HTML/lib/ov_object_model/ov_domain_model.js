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
    @file           ov_domain_model.js

    @ingroup        lib

	@brief          Blueprint for Domain object
		
    ---------------------------------------------------------------------------
*/

import ov_Project_Map from "/lib/ov_data_structure/ov_domain_map.js";
import ov_Parse_Exception from "/lib/ov_exception/ov_parse_exception.js";
import create_uuid from "/lib/ov_utils/ov_uuid.js";

export default class ov_Domain {

    #id;
    #name;
    #projects;
    #dom_id;
    
	constructor(id) {
		this.id = id;
        this.name = id;
        this.#projects = new ov_Project_Map();

        // dom id has to start with a letter, uuid may start with a number
        this.#dom_id = "domain_" + create_uuid();
	}

	set id(domain_id) {
		this.#id = domain_id;
	}

	get id() {
		return this.#id;
	}

    set name(name) {
        this.#name = name;
    }

    get name() {
        return this.#name;
    }

    get projects() {
        return this.#projects;
    }

    get dom_id(){
		return this.#dom_id;
	}

	static parse(id, json) {
		if (!id)
			throw new ov_Parse_Exception("no id", id);

		let domain = new this(id);
        domain.name = json && json.hasOwnProperty("name") ? json.name : id;
        return domain;
	}
}