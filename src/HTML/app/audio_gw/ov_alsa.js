/*
 * 	ov_alsa.js
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

var ov_alsa = {

    debug: true,
    session: null,
    soundcards: null,

    create_uuid: function() {
        function s4() {
            return Math.floor((1 + Math.random()) * 0x10000).toString(16).substring(1);
        }
        return s4() + s4() + "-" + s4() + "-" + s4() + "-" + s4() + "-" + s4() + s4() + s4();
    },

    init: function(){

        if (null == ov_websocket){

            console.error("ov_alsa is a protocol for ov_websocket " +
                "some ov_websocket MUST be present!");

        } else {

            console.log("ov_alsa.incoming set as ov_websocket.incoming.")
            ov_websocket.incoming = ov_alsa.incoming;

            ov_websocket.debug(this.debug);

        }

    },

    incoming: function(msg){

        /* Expect some JSON message input. */

        if (true == this.debug)
            console.log("<-- RECV "+ JSON.stringify(msg, null, 2));


        switch (msg.event) {

            case "get soundcards":
                /*
                document.getElementById('state').innerHTML = 
                JSON.stringify(msg.response, null, 2);
                */
                this.soundcards = msg.response.soundcards;
                ov_soundcard.draw('state', this.soundcards);
                break;

            case "get configuration":

                const dir = msg.response.config.direction;
                const ip = msg.response.config.host;
                const name = msg.response.name;

                document.getElementById("input" + name).value = ip;
                const select = document.getElementById("select" + name);

                switch (dir) {

                    case "input":
                       select.value = "Input";
                       break;

                    case "output":
                        select.value = "Output";

                    default:
                        select.value = "none";
                }
                break;

        }

    },

    get_configuration: function(name){

        let request = {
            uuid : this.create_uuid(),
            event: "get configuration",
            parameter: {}
        }

        request.parameter.name = name;

        ov_websocket.send(JSON.stringify(request));

    },

    get_soundcards: function(){

        let request = {
            uuid : this.create_uuid(),
            event: "get soundcards",
            parameter: {}
        }

        ov_websocket.send(JSON.stringify(request));

    },

     get_hw_params: function(name){

        let request = {
            uuid : this.create_uuid(),
            event: "get hw params",
            parameter: {}
        }

        request.parameter.name = name;

        ov_websocket.send(JSON.stringify(request));

    },

    play_tone: function(name){

        let request = {
            uuid : this.create_uuid(),
            event: "play tone",
            parameter: {}
        }

        request.parameter.name = name;

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

ov_alsa.init();