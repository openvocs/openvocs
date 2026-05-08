/*
 * 	site.js
 * 	Author: Markus Töpfer
 */

var site = {

	init: function(){

 		console.log("site init");
 	},

 	overview: function(){

 		document.getElementById("main").innerHTML = null;
 		api.get_clients();


 	},


};

site.init();
