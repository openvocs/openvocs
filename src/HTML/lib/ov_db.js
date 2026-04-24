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

import ov_Websocket from "./ov_websocket.js";
import * as ov_Websockets from "./ov_websocket_list.js";

var RETRIES_ON_TEMP_ERROR = 5;

export async function check_ldap(ws) {
    ws = ws ? ws : ov_Websockets.current_lead_websocket;
    let result = false;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "check if auth against ldap...");
            result = await ws.send_event(ov_Websocket.EVENT.LDAP_CHECK);
            console.log(log_prefix(ws) + "auth against ldap: ", result.response);
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to check if auth against ldap again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) + "check if auth against ldap failed", error);
                return false;
            }
        }
    }
    return result.response;
}

export async function check_sip(websocket) {
    ws = ws ? ws : ov_Websockets.current_lead_websocket;
    let result;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(websocket) + "requesting sip status...");
            result = await websocket.send_event(ov_Websocket.EVENT.SIP);
            console.log(log_prefix(websocket) + "sip server connected: " + result.response);
            break;
        } catch (error) {
            if (websocket.is_connecting && error.temp_error) {
                console.log(log_prefix(websocket) + "temp error - try to request sip status again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, websocket);
            } else {
                console.warn(log_prefix(websocket) + "requesting sip status failed.", error.error);
                disconnect(websocket);
                return false;
            }
        }
    }
    return result.response;
}

export async function check_id(id, scope, ws) {
    let result = false;
    ws = ws ? ws : ov_Websockets.current_lead_websocket;
    scope = scope ? scope : "all";
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "checking id " + id + " of scope " + scope + "...");
            let parameter = {
                id: id
            };
            if (scope !== "all")
                parameter.scope = scope;
            result = await ws.send_event(ov_Websocket.EVENT.CHECK_ID, parameter);
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
    return result.response;
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
            result = await ws.send_event(ov_Websocket.EVENT.VERIFY, parameter);
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
            await ws.send_event(ov_Websocket.EVENT.CREATE, parameter);
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
            result = await ws.send_event(ov_Websocket.EVENT.LDAP_IMPORT, parameter);
            console.log(log_prefix(ws) + "imported users from ldap " + host + " - " + base);
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to import users from ldap " + host + " - " + base + " again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "importing users from ldap " + host + " - " + base + " failed.", error);
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
            result = await ws.send_event(ov_Websocket.EVENT.UPDATE, parameter);
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

// update_key

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
            result = await ws.send_event(ov_Websocket.EVENT.UPDATE_PASSWORD, parameter);
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

export async function remove(type, id, ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let result = false;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "delete " + type + " " + id + "...");
            let parameter = {
                type: type,
                id: id
            };
            result = await ws.send_event(ov_Websocket.EVENT.DELETE, parameter);
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

// delete_key

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

    return result.data;
}

export async function collect_roles(ws) {
    ws = ws ? ws : ov_Websockets.current_lead_websocket;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "collecting user roles...");
            let roles = await ws.send_event(ov_Websocket.EVENT.USER_ROLES);
            console.log(log_prefix(ws) + "received " + roles.length + " roles: " + roles.toString());
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) + " temp error - try to collect user roles again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) + " collect user roles failed.", error);
                return false;
            }
        }
    }
    return true;
}

export async function get_all_loops(ws) {
    let result;
    ws = ws ? ws : ov_Websockets.current_lead_websocket;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "get all loops...");
            result = await ws.send_event(ov_Websocket.EVENT.LOOPS);
            console.log(log_prefix(ws) + "get all loops successful");
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to get all loops again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "get all loops failed.", error);
                return false;
            }
        }
    }
    return result.loops;
}

export async function collect_loops(ws) {
    ws = ws ? ws : ov_Websockets.current_lead_websocket;
    let result;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "collecting role loops...");
            result = await ws.send_event(ov_Websocket.EVENT.ROLE_LOOPS);
            console.log(log_prefix(ws) + "received loops");
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) + "temp error - try to collect role loops again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) + "collect role loops failed.", error);
                disconnect(ws);
                return false;
            }
        }
    }
    return result.loops;
}

// retrieve admin domains of single server - specified or lead
export async function domains(ws) {
    ws = ws ? ws : ov_Websockets.current_lead_websocket;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "collecting domains with admin rights...");
            await ws.send_event(ov_Websocket.EVENT.ADMIN_DOMAINS, { user: ws.user.id });
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
    ws = ws ? ws : ov_Websockets.current_lead_websocket;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "collecting projects with admin rights...");
            await ws.send_event(ov_Websocket.EVENT.ADMIN_PROJECTS, { user: ws.user.id });
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


export async function get_highest_multicast_port(ws) {
    ws = ws ? ws : ov_Websockets.current_lead_websocket;
    let result = false;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "get highest multicast port...");
            result = await ws.send_event(ov_Websocket.EVENT.HIGHEST_MULTICAST_PORT);
            console.log(log_prefix(ws) + "highest multicast used port:", result.port);
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to get hightest multicast port again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) + "getting multicast port failed", error);
                return false;
            }
        }
    }
    return parseInt(result.port);
}

export async function set_keyset_layout(id, domain, layout, ws) {
    ws = ws ? ws : ov_Websockets.current_lead_websocket;
    let result = false;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "save layout " + id + " in domain " + domain + "...");
            let parameter = {
                name: id,
                layout: layout,
                domain: domain
            };
            result = await ws.send_event(ov_Websocket.EVENT.SET_KEYSET_LAYOUT, parameter);
            console.log(log_prefix(ws) + "saved layout " + id + " in domain " + domain);
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) +
                    "temp error - try to save layout " + id + " in domain " + domain + " again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) +
                    "saving layout " + id + " in domain " + domain + " failed.", error);
                return false;
            }
        }
    }
    return result;
}

export async function collect_keyset_layout(layout_id, ws) {
    ws = ws ? ws : ov_Websockets.current_lead_websocket;
    let result;
    if (!layout_id)
        layout_id = "default";
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "collecting keyset layout...");
            result = await ws.send_event(ov_Websocket.EVENT.KEYSET_LAYOUT, { domain: ws.user.domain, layout: layout_id });
            console.log(log_prefix(ws) + "received settings for layout" + layout_id);
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) + "temp error - try to collect keyset layout again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) + "collect keyset layout failed.", error);
                disconnect(ws);
                return false;
            }
        }
    }
    return result.layout;
}

export async function update_user_settings(parameter, ws) {
    ws = ws ? ws : ov_Websockets.current_lead_websocket;
    let result;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "update user settings...");
            result = await ws.send_event(ov_Websocket.EVENT.UPDATE_USER_SETTINGS, parameter);
            console.log(log_prefix(ws) + "updated user settings");
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) + "temp error - try to update user settings again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) + "update user settings failed.", error);
                disconnect(ws);
                return false;
            }
        }
    }
    return result;
}

export async function collect_user_settings(ws) {
    ws = ws ? ws : ov_Websockets.current_lead_websocket;
    let result;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "collecting user settings...");
            result = await ws.send_event(ov_Websocket.EVENT.USER_SETTINGS);
            console.log(log_prefix(ws) + "received user settings");
            break;
        } catch (error) {
            if (ws.is_connecting && error.temp_error) {
                console.log(log_prefix(ws) + "temp error - try to collect user settings again after timeout");
                await ov_Websockets.sleep(TEMP_ERROR_TIMEOUT, ws);
            } else {
                console.warn(log_prefix(ws) + "collect user settings failed.", error);
                disconnect(ws);
                return false;
            }
        }
    }
    return result.data;
}

export async function persist(ws) {
    ws = ws ? ws : ov_Websockets.prime_websocket;
    let result = false;
    for (let count = 0; count <= RETRIES_ON_TEMP_ERROR; count++) {
        try {
            console.log(log_prefix(ws) + "persist saved data...");
            result = await ws.send_event(ov_Websocket.EVENT.PERSIST);
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
    return "(" + ws.server_name + " admin) ";
}