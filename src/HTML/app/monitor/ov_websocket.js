/*
 * 	ov_websocket.js
 * 	Author: Markus Töpfer
 *
 *      ------------------------------------------------------------------------
 *
 *      #GLOBAL Variables
 *
 *      ------------------------------------------------------------------------
 */

 var ov_websocket = {

    socket: null,
    debug:false,

    /*
     *      Websocket functionality
     */

    incoming: function(msg){

        /*
         *  SHOULD be overriden by protocol instance.
         */
        console.log(msg);

    },

    connect: function (address){

        if (this.socket) {
            console.log("Websocket already connected.");
            return;
        }

        console.log('Protocol: try to connect to ' + address);

        this.socket = new WebSocket(address);

        this.socket.onopen = function () {
            console.log('Protocol: connected');
        };

        this.socket.onclose = function (closeEvent) {

            console.log('Protocol: disconnected with close');
            this.socket = null;

        };

        this.socket.onerror = function (error) {
            console.error(error);
        };

        this.socket.onmessage = function (payload) {

            ov_websocket.incoming(JSON.parse(payload.data));

        };
    },

    debug: function( value ){

        this.debug = value
    },

    disconnect: function () {

        if (this.socket && this.socket.readyState == 1)
            this.socket.close();

        this.socket = null
    },

    send: function (message) {

        if (null == this.socket){
            console.error("No socket connection available. Cannot send. You need to connect the websocket!");
            return;
        }

        if (true == this.debug)
            console.log("--> SEND "+ message);

        this.socket.send(message);

    }

};