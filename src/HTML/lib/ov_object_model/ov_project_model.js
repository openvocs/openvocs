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
    @file           ov_project_model.js
        
    @author         Anja Bertard, Fabian Junge

    @date           2017-05-19

    @ingroup        lib

	@brief          Blueprint for Project object
		
    ---------------------------------------------------------------------------
*/

import ov_Parse_Exception from "/lib/ov_exception/ov_parse_exception.js";
import create_uuid from "/lib/ov_utils/ov_uuid.js";

export default class ov_Project {
	#id; 
	#name; 
	#domain; 
	#logo; 
	#dom_id;

	constructor(id, name, domain, logo) {
		this.id = id;
		this.name = name;
		this.domain = domain;
		this.logo = logo;

        // dom id has to start with a letter, uuid may start with a number
        this.#dom_id = "project_" + create_uuid();
	}

	set id(user_id) {
		this.#id = user_id;
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

	set logo(logo){
		this.#logo = logo;
	}

	get logo(){
		return this.#logo;
	}

	// domain is not part of the server config
	set domain(domain) {
		this.#domain = domain;
	}

	get domain() {
		return this.#domain;
	}

    get dom_id(){
		return this.#dom_id;
	}

	static parse(id, json) {
		if (!id)
			throw new ov_Parse_Exception("no id", id);

		if (!json.hasOwnProperty("name"))
			json.name = id;

		return new this(id, json.name, json.domain, json.logo);
	}
}