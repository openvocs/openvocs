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
    @file           view.js
        
    @author         Anja Bertard

    @date           2023-09-06

    @ingroup        vocs_admin/views/domain_settings

    @brief          manage DOM objects for admin domain settings view
    	
    ---------------------------------------------------------------------------
*/
const DOM = {};

var VIEW_ID;

export function init(view_id) {
    VIEW_ID = view_id;
    DOM.id = document.getElementById("edit_id");
    DOM.name = document.getElementById("edit_name");
    DOM.delete = document.getElementById("delete_domain");
    DOM.delete_button = document.getElementById("delete_button");
    DOM.ldap_host = document.getElementById("ldap_host");
    DOM.ldap_base = document.getElementById("ldap_base");
    DOM.ldap_user = document.getElementById("ldap_user");
    DOM.ldap_password = document.getElementById("ldap_password");
    DOM.ldap_button = document.getElementById("ldap_import_button");

    DOM.delete_button.addEventListener("click", () => {
        if (window.confirm("Do you really want to delete this domain?"))
            DOM.delete.dispatchEvent(new CustomEvent("delete_domain", {
                bubbles: true
            }));
    });

    DOM.ldap_button.addEventListener("click", () => {
        DOM.ldap_button.dispatchEvent(new CustomEvent("import_ldap_user", {
            detail: {
                host: DOM.ldap_host.value,
                base: DOM.ldap_base.value,
                user: DOM.ldap_user.value,
                password: DOM.ldap_password.value
            }, bubbles: true
        }));
        DOM.ldap_password.value = "";
    })

    DOM.name.addEventListener("change", changed_name);

    DOM.id.addEventListener("change", changed_name);

    check_ldap_form();

    DOM.ldap_host.onkeyup = () => check_ldap_form();
    DOM.ldap_host.onchange = () => check_ldap_form();
    DOM.ldap_base.onkeyup = () => check_ldap_form();
    DOM.ldap_base.onchange = () => check_ldap_form();
    DOM.ldap_user.onkeyup = () => check_ldap_form();
    DOM.ldap_user.onchange = () => check_ldap_form();
    DOM.ldap_password.onkeyup = () => check_ldap_form();
    DOM.ldap_password.onchange = () => check_ldap_form();
}

function changed_name() {
    if (DOM.name.value !== "")
        DOM.name.dispatchEvent(new CustomEvent("changed_name", {
            detail: DOM.name.value, bubbles: true
        }));
    else if (DOM.id.value !== "")
        DOM.id.dispatchEvent(new CustomEvent("changed_name", {
            detail: DOM.id.value, bubbles: true
        }));
    else
        DOM.id.dispatchEvent(new CustomEvent("changed_name", {
            detail: "[Domain]", bubbles: true
        }));
}

function check_ldap_form() {
    DOM.ldap_button.disabled = !(DOM.ldap_host.value && DOM.ldap_base.value &&
        DOM.ldap_user.value && DOM.ldap_password.value);
}

export function render(domain, id) {
    id = id ? id : domain.id;
    if (id)
        DOM.id.value = id;
    if (domain.name)
        DOM.name.value = domain.name;

    if (DOM.id.value)
        DOM.id.disabled = true;
    else
        DOM.delete_button.disabled = true;
}

export function collect() {
    let data = {};
    data.id = DOM.id.value;
    data.name = DOM.name.value ? DOM.name.value : null;
    return data;
}

export function offline_mode(value) {
    value = !value && !DOM.id.disabled ? true : value;
    DOM.delete_button.disabled = value;
}