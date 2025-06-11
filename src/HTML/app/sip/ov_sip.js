/*
 * 	ov_sip.js
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

var ov_sip = {

    debug: true,
    session: null,
    client_id : null,

    create_uuid: function() {
        function s4() {
            return Math.floor((1 + Math.random()) * 0x10000).toString(16).substring(1);
        }
        return s4() + s4() + "-" + s4() + "-" + s4() + "-" + s4() + "-" + s4() + s4() + s4();
    },

    init: function(){

        if (null == ov_websocket){

            console.error("ov_sip is a protocol for ov_websocket " +
                "some ov_websocket MUST be present!");

        } else {

            console.log("ov_sip.incoming set as ov_websocket.incoming.")
            ov_websocket.incoming = ov_sip.incoming;

            ov_websocket.debug(this.debug);

        }

    },

    incoming: function(msg){

        /* Expect some JSON message input. */

        if (true == this.debug)
            console.log("<-- RECV "+ JSON.stringify(msg, null, 2));


        switch (msg.event) {

            case "list_calls":
                document.getElementById('state').innerHTML =
                JSON.stringify(msg.response, null, 2);
                break;

            case "list_call_permissions":
                document.getElementById('state').innerHTML =
                JSON.stringify(msg.response, null, 2);
                break;

            case "list_sip_status":
                document.getElementById('state').innerHTML =
                JSON.stringify(msg.response, null, 2);
                break;

        }

    },

    login: function(user, pass){

        this.client_id = this.create_uuid();

        let request = {
            uuid : this.create_uuid(),
            event: "login",
            client : this.client_id,
            parameter: {}
        }

        request.parameter.user = user;
        request.parameter.password = pass;

        ov_websocket.send(JSON.stringify(request));

        document.getElementById("pfromepoch").value = Date.now();
        document.getElementById("puntilepoch").value = (Date.now() + 360000);
        document.getElementById("rfromepoch").value = Date.now();
        document.getElementById("runtilepoch").value = (Date.now() + 360000);

    },

    authorize: function(domain){

        let request = {
            uuid : this.create_uuid(),
            event: "authorize",
            client : this.client_id,
            parameter: {
                role : "admin"
            }
        }

        request.parameter.domain = domain;
        ov_websocket.send(JSON.stringify(request));

    },

    call: function(loop, from, to){

        let request = {
            uuid : this.create_uuid(),
            client : this.client_id,
            event: "call",
            parameter: {}
        }

        request.parameter.loop = loop;
        request.parameter.destination = to;
        request.parameter.from = from;

        ov_websocket.send(JSON.stringify(request));

    },

    hangup: function(callid){

        let request = {
            uuid : this.create_uuid(),
            client : this.client_id,
            event: "hangup",
            parameter: {}
        }

        request.parameter.call = callid;

        ov_websocket.send(JSON.stringify(request));

    },

    permit: function(loop, dest, from, until){

        let request = {
            uuid : this.create_uuid(),
            client : this.client_id,
            event: "permit_call",
            parameter: {}
        }

        request.parameter.caller = dest;
        request.parameter.loop = loop;
        request.parameter.valid_from = from;
        request.parameter.valid_until = until;

        ov_websocket.send(JSON.stringify(request));

    },

    revoke: function(loop, dest, from, until){

        let request = {
            uuid : this.create_uuid(),
            client : this.client_id,
            event: "revoke_call",
            parameter: {}
        }

        request.parameter.caller = dest;
        request.parameter.loop = loop;
        request.parameter.valid_from = from;
        request.parameter.valid_until = until;

        ov_websocket.send(JSON.stringify(request));

    },

    list_calls: function(){

        let request = {
            uuid : this.create_uuid(),
            client : this.client_id,
            event: "list_calls",
            parameter: {}
        }

        ov_websocket.send(JSON.stringify(request));

    },

    list_permissions: function(){

        let request = {
            uuid : this.create_uuid(),
            client : this.client_id,
            event: "list_call_permissions",
            parameter: {}
        }

        ov_websocket.send(JSON.stringify(request));

    },

    list_status: function(){

        let request = {
            uuid : this.create_uuid(),
            client : this.client_id,
            event: "list_sip_status",
            parameter: {}
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

ov_sip.init();