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
    @file           ov_parse_exception.js

    @ingroup        lib

    @brief          These exceptions are triggered if fields are missing in 
                    the JSON string, while parsing it into an ov_object.
		
    ---------------------------------------------------------------------------
*/

export default class ov_Parse_Exception extends Error {
    constructor(message) {
        super(message);
        this.name = "ov_Parse_Exception";
    }
 }