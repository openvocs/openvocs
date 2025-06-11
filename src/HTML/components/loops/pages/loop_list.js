

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
	@file           ov_loop_list.js       
		  
	@author         Anja Bertard, Fabian Junge  
			   
	@date           2017-05-19   
			  
	@ingroup        lib 
				
	@brief          Data structure to handel Loops 		        
		
	---------------------------------------------------------------------------
*/

import ov_Loop from "/components/loops/loop/loop.js";

export default class Loop_List {
	#rows = 0;
	#columns = 0;
	#loops;
	#active;
	constructor() {
		this.reset();
	}

	get values() {
		return Array.from(this.#loops.values());
	}

	add(loop) {
		this.#loops.set(loop.loop_id, loop);
		if (loop.layout_pos.row > this.#rows)
			this.#rows = loop.layout_pos.row;
		if (loop.layout_pos.column > this.#columns)
			this.#columns = loop.layout_pos.column;
	}

	remove(loop) {
		this.#loops.delete(loop.loop_id);
	}

	find(id) {
		return this.#loops.get(id);
	}

	has(id) {
		return this.#loops.has(id);
	}

	sort() {
		this.#loops = new Map([...this.#loops].sort(
			(a, b) => (a[1].layout_pos.page > b[1].layout_pos.page) ? 1 : ((b[1].layout_pos.page > a[1].layout_pos.page) ? -1 :
				(a[1].layout_pos.column > b[1].layout_pos.column) ? 1 : ((b[1].layout_pos.column > a[1].layout_pos.column) ? -1 :
					(a[1].layout_pos.row > b[1].layout_pos.row) ? 1 : ((b[1].layout_pos.row > a[1].layout_pos.row) ? -1 : 0)))));
	}

	reset() {
		this.#columns = 0;
		this.#rows = 0;
		this.#loops = new Map();
	}

	toString() {
		return Array.from(this.#loops.keys()).toString();
	}

	get length() {
		return this.#loops.size;
	}

	get sip() {
		return Loop_List.#Loop_Element === "ov-sip-loop";
	}

	get rows() {
		return this.#rows;
	}

	get columns() {
		return this.#columns;
	}

	get active() {
		return this.#active;
	}

	show() {
		for (let loop of this.values) {
			loop.style.display = "";
		}
		this.#active = true;
	}

	hide() {
		for (let loop of this.values) {
			loop.style.display = "none";
		}
		this.#active = false;
	}

	// returns array of loop pages
	static parse(json) {
		let pages = [new Loop_List()];
		for (let id of Object.keys(json)) {
			if (!json[id].layout_position)
				json[id].layout_position = [{}];
			for (let pos of json[id].layout_position) {
				if (pos) {
					let loop = document.createElement(Loop_List.#Loop_Element);
					loop.parse(id, json[id], pos);
					let page = parseInt(pos.page) - 1;
					if (page || page === 0) {
						for (let index = 0; index <= page; index++)
							if (!(pages[index] instanceof Loop_List))
								pages[index] = new Loop_List();
						pages[page].add(loop);
					}
				}
			}
		}
		return pages;
	}

	static create_loop(id, data, pos) {
		let loop = document.createElement(Loop_List.#Loop_Element);
		loop.parse(id, data, pos);
		return loop;
	}

	static #Loop_Element = function () {
		if (SIP === true) {
			import("/extensions/sip/components/loop/loop.js");
			return "ov-sip-loop";
		} else {
			return "ov-loop";
		}
	}();
}