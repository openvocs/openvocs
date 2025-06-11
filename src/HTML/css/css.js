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
    @file           css.js
        
    @author         Anja Bertard

    @date           2023-09-22

    @ingroup        css

    @brief          create constructed style sheets for common ov css files
    	
    ---------------------------------------------------------------------------
*/

export const normalize_style_sheet = fetch_style_sheet("/css/third-party/normalize.css");
export const loading_id_style_sheet = fetch_style_sheet("/css/third-party/loading_io.css");
export const ov_basic_style_sheet = fetch_style_sheet("/css/ov_basic.css");

export async function fetch_style_sheet(url) {
    let response = await fetch(url);

    let style = new CSSStyleSheet();
    let text = await response.text();

    await style.replace(text);
    return style;
} 