/*
 * 	ov_monitor.js
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

var ov_monitor = {

    debug: true,
    session: null,

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

            console.log("ov_monitor.incoming set as ov_websocket.incoming.")
            ov_websocket.incoming = ov_monitor.incoming;

            ov_websocket.debug(this.debug);

        }

    },

    incoming: function(msg){

        /* Expect some JSON message input. */

        if (true == this.debug)
            console.log("<-- RECV "+ JSON.stringify(msg, null, 2));


        switch (msg.event) {

            case "state":
                document.getElementById('state').innerHTML =
                JSON.stringify(msg.response, null, 2);
                break;

            case "list_users":
                document.getElementById('state').innerHTML =
                JSON.stringify(msg.response, null, 2);
                break;
    
            case "list_loops":
                document.getElementById('state').innerHTML =
                JSON.stringify(msg.response, null, 2);
                break;

            case "get_user_state":
                document.getElementById('state').innerHTML =
                JSON.stringify(msg.response, null, 2);
                break;
    
            case "get_loop_state":
                document.getElementById('state').innerHTML =
                JSON.stringify(msg.response, null, 2);
                break;

            case "get_user_resources":
                document.getElementById('state').innerHTML =
                JSON.stringify(msg.response, null, 2);
                break;

            case "get_loop_resources":
                document.getElementById('state').innerHTML =
                JSON.stringify(msg.response, null, 2);
                break;

        }

    },

    login: function(user, pass){

        let request = {
            event: "login",
            parameter: {}
        }

        request.parameter.user = user;
        request.parameter.password = pass;

        ov_websocket.send(JSON.stringify(request));

    },

    state: function(){

        let request = {
            uuid : this.create_uuid(),
            event: "state",
            parameter: {}
        }

        ov_websocket.send(JSON.stringify(request));

    },

    state_users: function(){

        let request = {
            uuid : this.create_uuid(),
            event: "list_users",
            parameter: {}
        }

        ov_websocket.send(JSON.stringify(request));

    },
    state_loops: function(){

        let request = {
            uuid : this.create_uuid(),
            event: "list_loops",
            parameter: {}
        }

        ov_websocket.send(JSON.stringify(request));

    },
    state_user: function(name){

        let request = {
            uuid : this.create_uuid(),
            event: "get_user_state",
            parameter: {
                name : name
            }
        }

        ov_websocket.send(JSON.stringify(request));

    },
    state_loop: function(name){

        let request = {
            uuid : this.create_uuid(),
            event: "get_loop_state",
            parameter: {
                name : name
            }
        }

        ov_websocket.send(JSON.stringify(request));

    },
    state_user_resources: function(name){

        let request = {
            uuid : this.create_uuid(),
            event: "get_user_resources",
            parameter: {
                name : name
            }
        }

        ov_websocket.send(JSON.stringify(request));

    },
    state_loop_resources: function(name){

        let request = {
            uuid : this.create_uuid(),
            event: "get_loop_resources",
            parameter: {
                name : name
            }
        }

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

ov_monitor.init();