/*
 * 	ov_vocs_db.js
 * 	Author: Markus Töpfer
 *
 *
 *      This JS file expects some ov_websocket with a configure function,
 *      to set itself as protocol for the websocket to consume incoming messages.
 *
 *      ------------------------------------------------------------------------
 *
 *      #GLOBAL Variables
 *
 *      ------------------------------------------------------------------------
 */

var ov_vocs_db = {

    debug: true,
    session: null,
    client: null,

    create_uuid: function() {
        function s4() {
            return Math.floor((1 + Math.random()) * 0x10000).toString(16).substring(1);
        }
        return s4() + s4() + "-" + s4() + "-" + s4() + "-" + s4() + "-" + s4() + s4() + s4();
    },

    init: function(){

        if (null == ov_websocket){

            console.error("ov_ice_proxy is a protocol for ov_websocket " +
                "some ov_websocket MUST be present!");

        } else {

            console.log("ov_vocs_db.incoming set as ov_websocket.incoming.")
            ov_websocket.incoming = ov_vocs_db.incoming;

            ov_websocket.debug(this.debug);

        }

        this.client = this.create_uuid();

    },

    incoming: function(msg){

        /* Expect some JSON message input. */

        if (true == this.debug)
            console.log("<-- RECV "+ JSON.stringify(msg, null, 2));

        switch (msg.event) {

            case "login":
                console.log("Received login success response.");
                break;
    

            case "create": 
                document.getElementById("create_response").innerHTML =
                            JSON.stringify(msg, null, 2);
                break;

            case "update": 
                document.getElementById("update_response").innerHTML =
                            JSON.stringify(msg, null, 2);
                break;

            case "delete": 
                document.getElementById("delete_response").innerHTML =
                            JSON.stringify(msg, null, 2);
                break;

            case "get": 
                document.getElementById("get_response").innerHTML =
                            JSON.stringify(msg, null, 2);
                break;

            case "get_key": 
                document.getElementById("get_key_response").innerHTML =
                            JSON.stringify(msg, null, 2);
                break;

            case "update_key": 
                document.getElementById("update_key_response").innerHTML =
                            JSON.stringify(msg, null, 2);
                break;

            case "delete_key": 
                document.getElementById("delete_key_response").innerHTML =
                            JSON.stringify(msg, null, 2);
                break;

            case "save": 
                document.getElementById("save_response").innerHTML =
                            JSON.stringify(msg, null, 2);
                break;

            case "load": 
                document.getElementById("load_response").innerHTML =
                            JSON.stringify(msg, null, 2);
                break;

            case "admin_domains": 
                document.getElementById("admin_domains_response").innerHTML =
                            JSON.stringify(msg, null, 2);
                break;

            case "admin_projects": 
                document.getElementById("admin_projects_response").innerHTML =
                            JSON.stringify(msg, null, 2);
                break;

            case "ldap_import": 
                document.getElementById("ldap_import_response").innerHTML =
                            JSON.stringify(msg, null, 2);
                break;

        }

    },

    login: function(user, pass){

        let request = {
            client: this.client,
            uuid: ov_vocs_db.create_uuid(),
            event: "login",
            parameter: {}
        }

        request.parameter.user = user;
        request.parameter.password = pass;

        ov_websocket.send(JSON.stringify(request));

    },

    logout: function(){

        let request = {
            client: this.client,
            uuid: ov_vocs_db.create_uuid(),
            event: "logout",
            parameter: {}
        }

        ov_websocket.send(JSON.stringify(request));

    },

    create: function () {

        let request = JSON.parse(
            document.getElementById("create_input").value);

        request.uuid = ov_vocs_db.create_uuid();
        request.client = this.client;

        ov_websocket.send(JSON.stringify(request));
    },

    update: function () {

        let request = JSON.parse(
            document.getElementById("update_input").value);

        request.uuid = ov_vocs_db.create_uuid();

        ov_websocket.send(JSON.stringify(request));
    },

    delete: function () {

        let request = JSON.parse(
            document.getElementById("delete_input").value);

        request.uuid = ov_vocs_db.create_uuid();
        request.client = this.client;

        ov_websocket.send(JSON.stringify(request));
    },

    get: function () {

        let request = JSON.parse(
            document.getElementById("get_input").value);

        request.uuid = ov_vocs_db.create_uuid();
        request.client = this.client;

        ov_websocket.send(JSON.stringify(request));
    },

    admin_projects: function () {

        let request = JSON.parse(
            document.getElementById("admin_projects_input").value);

        request.uuid = ov_vocs_db.create_uuid();
        request.client = this.client;

        ov_websocket.send(JSON.stringify(request));
    },

    admin_domains: function () {

        let request = JSON.parse(
            document.getElementById("admin_domains_input").value);

        request.uuid = ov_vocs_db.create_uuid();
        request.client = this.client;

        ov_websocket.send(JSON.stringify(request));
    },

    load: function () {

        let request = JSON.parse(
            document.getElementById("load_input").value);

        request.uuid = ov_vocs_db.create_uuid();
        request.client = this.client;

        ov_websocket.send(JSON.stringify(request));
    },

    save: function () {

        let request = JSON.parse(
            document.getElementById("save_input").value);

        request.uuid = ov_vocs_db.create_uuid();
        request.client = this.client;

        ov_websocket.send(JSON.stringify(request));
    },

    delete_key: function () {

        let request = JSON.parse(
            document.getElementById("delete_key_input").value);

        request.uuid = ov_vocs_db.create_uuid();
        request.client = this.client;

        ov_websocket.send(JSON.stringify(request));
    },

    update_key: function () {

        let request = JSON.parse(
            document.getElementById("update_key_input").value);

        request.uuid = ov_vocs_db.create_uuid();
        request.client = this.client;

        ov_websocket.send(JSON.stringify(request));
    },

    get_key: function () {

        let request = JSON.parse(
            document.getElementById("get_key_input").value);

        request.uuid = ov_vocs_db.create_uuid();
        request.client = this.client;

        ov_websocket.send(JSON.stringify(request));
    },

    ldap_import: function () {

        let request = JSON.parse(
            document.getElementById("ldap_import_input").value);

        request.uuid = ov_vocs_db.create_uuid();
        request.client = this.client;

        ov_websocket.send(JSON.stringify(request));
    }

};

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC INIT IN TEMPLATE
 *
 *      This JS file expects some ov_websocket with a configure function,
 *      to set itself as protocl for the websocket to consume incoming messages.
 *
 *      ------------------------------------------------------------------------
 */

ov_vocs_db.init();