var api = {

    init: function() {

        api = document.createElement('script');
        api.src = "js/api.js";
        document.head.appendChild(api);
        
    }
};

var websocket = {

    init: function() {

        websocket = document.createElement('script');
        websocket.src = "js/websocket.js";
        document.head.appendChild(websocket);
        
    }
};


var site = {

    init: function() {

        site = document.createElement('script');
        site.src = "js/site.js";
        document.head.appendChild(site);
        
    }
};

var openvocs = {

    init: function(){

        console.log("openvocs init");
        websocket.init();
        api.init();
        site.init();

    } 
};

openvocs.init();


