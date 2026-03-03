/*
 * 	api.js
 * 	Author: Markus Töpfer
 */
var api = {

	client: null,

    create_uuid: function() {
        function s4() {
            return Math.floor((1 + Math.random()) * 0x10000).toString(16).substring(1);
        }
        return s4() + s4() + "-" + s4() + "-" + s4() + "-" + s4() + "-" + s4() + s4() + s4();
    },

	init: function(){

 		console.log("api init");
 		this.client = this.create_uuid();

 		websocket.connect("https://192.168.178.25/api");
 		websocket.debug(true);
 		websocket.incoming = this.incoming;
 	},

 	incoming : function(msg){

 		console.log("<-- RECV " + JSON.stringify(msg));

 	},

 	get_clients : function(){

 		 let request = {
            uuid : this.create_uuid(),
            client: this.client,
            event: "get_clients",
            parameter: {}
        }

        websocket.send(JSON.stringify(request));
 	}

};

api.init();



