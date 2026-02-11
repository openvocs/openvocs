/*
 * 	api.js
 * 	Author: Markus Töpfer
 */

var api = {

	websocket: null,

	init: function(){

 		console.log("api init");
 		this.websocket = websocket;
 		this.websocket.connect("https://192.168.178.25/api");
 	} 

};

api.init();



