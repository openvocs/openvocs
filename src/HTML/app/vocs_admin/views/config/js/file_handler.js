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
    @file           file_handler.js
        
    @author         Anja Bertard

    @date           2017-01-17

    @ingroup        rbac_client

    @brief          loads files; displays files in new tabs
		
    ---------------------------------------------------------------------------
*/
export function request_remote_file(path, callback_function) {
    console.log("(rc) file handler: open remote file:", path);
    let xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (xhttp.readyState == 4 && xhttp.status == 200) {
            console.log("(rc) file handler: file loaded", path);
            callback_function(JSON.parse(xhttp.responseText));
        }
    };
    xhttp.open("GET", path, true);
    xhttp.send();
}

export function open_local_file(path, callback_function) {
    if (!path) {
        console.warn("(rc) file handler: file not specified");
        return;
    }
    console.log("(rc) file handler: open local file:", path);
    let reader = new FileReader();
    reader.onload = function (e) {
        console.log("(rc) file handler: file loaded");
        callback_function(JSON.parse(e.target.result));
    };
    reader.readAsText(path);
}

export function save_as_json_file(json, title) {
    console.log("(rc) file handler: save file " + title + ".json");
    let data_str = "data:text/json;charset=utf-8," + encodeURIComponent(json);
    let element = document.createElement("a");
    element.setAttribute("href", data_str);
    element.setAttribute("download", title + ".json");
    element.setAttribute("target", "_blank");
    document.body.appendChild(element);
    element.click();
    element.remove();
}