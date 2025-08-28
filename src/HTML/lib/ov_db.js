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
    @file           ov_db.js

    @ingroup        ov_lib

    @brief          implements openvocs db protocol and wraps it
                    around a list of ov_websocket

    ---------------------------------------------------------------------------
*/

import * as ov_Websockets from "./ov_websocket_list.js";
import ov_Websocket from "./ov_websocket.js";

var RETRIES_ON_TEMP_ERROR = 5;

const EVENT = {
    CREATE: "create", //project, user, role, loop
    UPDATE: "update", //project, user, role, loop
    DELETE: "delete", //project, user, role, loop
    CHECK_ID: "check_id_exists",
    LDAP_IMPORT: "ldap_import",
    UPDATE_PASSWORD: "update_password",
    SET_KEYSET_LAYOUT: "set_keyset_layout",
    PERSIST: "save"
}

// retrieve admin domains of single server - specified or lead
export async function domains(ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "collecting domains with admin rights...");
            await ws.send_event(ov_Websocket.EVENT.ADMIN_DOMAINS);
            console.log(log_prefix(ws) + "received " + ws.user.domains.size +
                " domains with admin rights: " + ws.user.domains.toString());
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to collect domains with admin rights again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "collect domains with admin rights failed.", error);
                return false;
            }
        }
    }
    return true;
}

// retrieve admin projects of single server - specified or lead
export async function projects(ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "collecting projects with admin rights...");
            await ws.send_event(ov_Websocket.EVENT.ADMIN_PROJECTS);
            console.log(log_prefix(ws) + "received " + ws.user.projects.size +
                " projects with admin rights: " + ws.user.projects.toString());
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to collect projects with admin rights again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "collect projects with admin rights failed.", error);
                return false;
            }
        }
    }
    return true;
}

export async function get_config(type, id, ws) {
    let result = false;
    ws = ws ? ws : ov_Websockets.prime_websocket;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "collecting " + type + " " + id + " config...");
            let parameter = {
                type: type,
                id: id
            };
            result = await ws.send_event(ov_Websocket.EVENT.GET, parameter);
            console.log(log_prefix(ws) + "received " + type + " configuration for " + id);
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to collect " + type + " " + id + " config again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "collect " + type + " " + id + " config failed.", error);
                return false;
            }
        }
    }
    if (type === "domain")
        return result.result;
    return result;
}

export async function check_id(id, scope, ws) {
    let result = false;
    ws = ws ? ws : ov_Websockets.prime_websocket;
    scope = scope ? scope : "all";
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "checking id " + id + " of scope " + scope + "...");
            let parameter = {
                id: id
            };
            if (scope !== "all")
                parameter.scope = scope;
            result = await ws.send_event(EVENT.CHECK_ID, parameter);
            if (result.result)
                console.log(log_prefix(ws) + id + " in scope " + scope + " exists");
            else
                console.log(log_prefix(ws) + id + " in scope " + scope + " doesn't exist");
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to check id " + id + " in scope " + scope + " again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "checking id " + id + " in scope " + scope + " failed.", error);
                return false;
            }
        }
    }
    return result.result;
}

export async function verify(type, config, ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let result = false;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "verify " + type + " config...");
            let parameter = {
                type: type,
                id: config.id,
                data: config
            };
            result = await ws.send_event(EVENT.UPDATE, parameter);
            console.log(log_prefix(ws) + "verified " + type + " config");
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to verify " + type + " config again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "verifying " + type + " failed.", error);
                return false;
            }
        }
    }
    return result;
}

export async function create(type, id, scope, scope_id, ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let result = false;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "create " + type + " " + id + "...");
            let parameter = {
                type: type,
                id: id,
                scope: {
                    type: scope,
                    id: scope_id
                }
            };
            await ws.send_event(EVENT.CREATE, parameter);
            result = true;
            console.log(log_prefix(ws) + "created new " + type + " " + id);
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to create " + type + " " + id + " again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "creating " + type + " failed.", error);
                return false;
            }
        }
    }
    return result;
}

export async function update(type, config, ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let result = false;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "update " + type + " " + config.id + "...");
            let parameter = {
                type: type,
                id: config.id,
                data: config
            };
            result = await ws.send_event(EVENT.UPDATE, parameter);
            console.log(log_prefix(ws) + "updated " + type + " " + config.id);
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to update " + type + " " + config.id + " again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "update " + type + " " + config.id + " failed.", error);
                return false;
            }
        }
    }
    return result;
}

export async function update_password(id, password, ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let result = false;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "update password for user " + id + "...");
            let parameter = {
                password: password,
                user: id
            };
            result = await ws.send_event(EVENT.UPDATE_PASSWORD, parameter);
            console.log(log_prefix(ws) + "updated password for user " + id);
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to update password of user " + id + " again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "update password of user " + id + " failed.", error);
                return false;
            }
        }
    }
    return result;
}

export async function delete_domain(domain_id, ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let result = false;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "delete domain " + domain_id + "...");
            let parameter = {
                type: ov_Websocket.REQUEST_SCOPE.DOMAIN,
                id: domain_id
            };
            result = await ws.send_event(EVENT.DELETE, parameter);
            console.log(log_prefix(ws) + "deleted domain " + domain_id);
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to delete domain " + domain_id + " again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "delete domain " + domain_id + " failed.", error);
                return false;
            }
        }
    }
    return result;
}

export async function delete_project(domain_id, project_id, ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let result = false;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "delete project " + project_id + "...");
            let parameter = {
                type: ov_Websocket.REQUEST_SCOPE.PROJECT,
                id: project_id
            };
            result = await ws.send_event(EVENT.DELETE, parameter);
            console.log(log_prefix(ws) + "deleted project " + project_id + " in domain " + domain_id);
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to delete project " + project_id + " again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "delete project " + project_id + " failed.", error);
                return false;
            }
        }
    }
    return result;
}

export async function delete_user(user_id, ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let result = false;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "delete user " + user_id + "...");
            let parameter = {
                type: ov_Websocket.REQUEST_SCOPE.USER,
                id: user_id
            };
            result = await ws.send_event(EVENT.DELETE, parameter);
            console.log(log_prefix(ws) + "deleted user " + user_id);
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to delete user " + user_id + " again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "delete user " + user_id + " failed.", error);
                return false;
            }
        }
    }
    return result;
}

export async function delete_role(role_id, ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let result = false;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "delete role " + role_id + "...");
            let parameter = {
                type: ov_Websocket.REQUEST_SCOPE.ROLE,
                id: role_id
            };
            result = await ws.send_event(EVENT.DELETE, parameter);
            console.log(log_prefix(ws) + "deleted role " + role_id);
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to delete role " + role_id + " again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "delete role " + role_id + " failed.", error);
                return false;
            }
        }
    }
    return result;
}

export async function delete_loop(loop_id, ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let result = false;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "delete loop " + loop_id + "...");
            let parameter = {
                type: ov_Websocket.REQUEST_SCOPE.LOOP,
                id: loop_id
            };
            result = await ws.send_event(EVENT.DELETE, parameter);
            console.log(log_prefix(ws) + "deleted loop " + loop_id);
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to delete loop " + loop_id + " again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "delete loop " + loop_id + " failed.", error);
                return false;
            }
        }
    }
    return result;
}

export async function erase(type, id, ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let result = false;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "delete " + type + " " + id + "...");
            let parameter = {
                type: type,
                id: id
            };
            result = await ws.send_event(EVENT.DELETE, parameter);
            console.log(log_prefix(ws) + "deleted " + type + " " + id);
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to delete " + type + " " + id + " again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "delete " + type + " " + id + " failed.", error);
                return false;
            }
        }
    }
    return result;
}

export async function user_ldap_import(host, base, domain, user, passwd, ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let result = false;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "import users from ldap " + host + " - " + base + "...");
            let parameter = {
                host: host,
                base: base,
                domain: domain,
                user: user,
                password: passwd
            };
            result = await ws.send_event(EVENT.LDAP_IMPORT, parameter);
            console.log(log_prefix(ws) + "imported users from ldap " + host + " - " + base);
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to import users from ldap " + host + " - " + base + " again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "importing users from ldap " +  host + " - " + base + " failed.", error);
                return false;
            }
        }
    }
    return result;
}

export async function set_keyset_layout(id, domain, layout, ws){
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let result = false;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "save layout " + id + " in domain " + domain + "...");
            let parameter = {
                name: id,
                layout: layout,
                domain: domain
            };
            result = await ws.send_event(EVENT.SET_KEYSET_LAYOUT, parameter);
            console.log(log_prefix(ws) + "saved layout " + id + " in domain " + domain);
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to save layout " + id + " in domain " + domain + " again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "saving layout " +  id + " in domain " + domain + " failed.", error);
                return false;
            }
        }
    }
    return result;
}

export async function persist(ws){
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let result = false;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "persist saved data...");
            result = await ws.send_event(EVENT.PERSIST);
            console.log(log_prefix(ws) + "persisted saved data");
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to persist saved data again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) + "persisted saved data", error);
                return false;
            }
        }
    }
    return result;
}

//-----------------------------------------------------------------------------
// logging
//-----------------------------------------------------------------------------
function log_prefix(ws) {
    return "(" + ws.server_name + "/" + ws.port + " admin) ";
}