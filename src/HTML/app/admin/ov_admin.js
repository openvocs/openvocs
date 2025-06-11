/*
 * 	ov_admin.js
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

var ov_admin = {

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

            console.error("ov_admin is a protocol for ov_websocket " +
                "some ov_websocket MUST be present!");

        } else {

            console.log("ov_admin.incoming set as ov_websocket.incoming.")
            ov_websocket.incoming = ov_admin.incoming;

            ov_websocket.debug(this.debug);

        }

        this.client = this.create_uuid();

    },

    incoming: function(msg){

        /* Expect some JSON message input. */

        if (true == this.debug)
            console.log("<-- RECV "+ JSON.stringify(msg, null, 2));


        switch (msg.event) {

            case "state_mixer":
                document.getElementById('state').innerHTML =
                JSON.stringify(msg.response, null, 2);
                break;

            case "state_connections":
                document.getElementById('state').innerHTML =
                JSON.stringify(msg.response, null, 2);
                break;

            case "state_session":
                document.getElementById('state').innerHTML =
                JSON.stringify(msg.response, null, 2);
                break;

            default:
                document.getElementById('state').innerHTML =
                JSON.stringify(msg, null, 2);
                break;

        }

    },

    login: function(user, pass){

        let request = {
            uuid : this.create_uuid(),
            client: this.client,
            event: "login",
            parameter: {}
        }

        request.parameter.user = user;
        request.parameter.password = pass;

        ov_websocket.send(JSON.stringify(request));

    },

    authorize: function(domain){

        let request = {
            uuid : this.create_uuid(),
            client: this.client,
            event: "authorize",
            parameter: {
                role : "admin"
            }
        }

        request.parameter.domain = domain;
        ov_websocket.send(JSON.stringify(request));

    },

    mixer_state: function(){

        let request = {
            uuid : this.create_uuid(),
            client: this.client,
            event: "state_mixer",
            parameter: {}
        }

        ov_websocket.send(JSON.stringify(request));

    },

    connection_state: function(){

        let request = {
            uuid : this.create_uuid(),
            client: this.client,
            event: "state_connections",
            parameter: {}
        }

        ov_websocket.send(JSON.stringify(request));

    },

    session_state: function(session_id){

        let request = {
            uuid : this.create_uuid(),
            client: this.client,
            event: "state_session",
            parameter: {}
        }

        request.parameter.session = session_id;
        ov_websocket.send(JSON.stringify(request));

    },

    input: function(input){

        ov_websocket.send(input);

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

ov_admin.init();