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

    This file is part of the openvocs domains. https://openvocs.org

    ---------------------------------------------------------------------------
*//**
    @file           ov_domains_list.js
        
    @author         Anja Bertard, Fabian Junge

    @date           2017-05-19

    @ingroup        lib

    @brief          Data structure to handel Projects
    	
    ---------------------------------------------------------------------------
*/

import ov_Domain from "/lib/ov_object_model/ov_domain_model.js";

export default class ov_Domain_Map extends Map {
    constructor() {
        super();
    }

    set(domain) {
        return super.set(domain.id, domain);
    }

    sort() {
        let sorted_list = [...this].sort(
            (a, b) => (a[1].id > b[1].id) ? 1 : ((b[1].id > a[1].id) ? -1 : 0));
        this.clear();
        for (let entry of sorted_list)
            super.set(entry[0], entry[1]);
    }

    toString() {
        return Array.from(this.keys()).toString();
    }

    static parse(json) {
       
        /*let domains = new this();
        for (let id of Object.keys(json)) {
            domains.set(ov_Domain.parse(json[id]));
            //domains.add({id: json[id]});
        }
        return domains;*/

        let domains = new this();
        for (let id of json) { // json = array
            domains.set(ov_Domain.parse(id));
        }
        return domains;
    }
}