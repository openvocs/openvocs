/*
 * 	ov_recorder.js
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

var ov_recorder = {

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

            console.error("ov_admin is a protocol for ov_websocket " +
                "some ov_websocket MUST be present!");

        } else {

            console.log("ov_admin.incoming set as ov_websocket.incoming.")
            ov_websocket.incoming = ov_recorder.incoming;

            ov_websocket.debug(this.debug);

        }

    },

    incoming: function(msg){

        /* Expect some JSON message input. */

        if (true == this.debug)
            console.log("<-- RECV "+ JSON.stringify(msg, null, 2));


        switch (msg.event) {

            case "get_recordings":
                document.getElementById('state').innerHTML =
                JSON.stringify(msg.response, null, 2);
                break;

            case "load_recordings":
                document.getElementById('state').innerHTML =
                JSON.stringify(msg.response, null, 2);
                break;

            case "save_recordings":
                document.getElementById('state').innerHTML =
                JSON.stringify(msg.response, null, 2);
                break;

            case "add_recording":
                document.getElementById('state').innerHTML =
                JSON.stringify(msg.response, null, 2);
                break;

            case "del_recording":
                document.getElementById('state').innerHTML =
                JSON.stringify(msg.response, null, 2);
                break;


        }

    },

    login: function(user, pass){

        let request = {
            uuid : this.create_uuid(),
            event: "login",
            parameter: {}
        }

        request.parameter.user = user;
        request.parameter.password = pass;

        ov_websocket.send(JSON.stringify(request));

    },

    get_recordings: function(domain){

        let request = {
            uuid : this.create_uuid(),
            event: "get_recordings",
            parameter: {
            }
        }

        ov_websocket.send(JSON.stringify(request));

    },

    save: function(domain){

        let request = {
            uuid : this.create_uuid(),
            event: "save_recordings",
            parameter: {
            }
        }

        ov_websocket.send(JSON.stringify(request));

    },

    load: function(domain){

        let request = {
            uuid : this.create_uuid(),
            event: "load_recordings",
            parameter: {
            }
        }

        ov_websocket.send(JSON.stringify(request));

    },

    add: function(loopname){

        let request = {
            uuid : this.create_uuid(),
            event: "add_recording",
            parameter: {
            }
        }

        request.parameter.loop = loopname

        ov_websocket.send(JSON.stringify(request));

    },

    del: function(loopname){

        let request = {
            uuid : this.create_uuid(),
            event: "del_recording",
            parameter: {
            }
        }

        request.parameter.loop = loopname

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

ov_recorder.init();
