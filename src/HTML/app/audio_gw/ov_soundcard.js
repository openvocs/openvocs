/*
 * 	ov_soundcard.js
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

var ov_soundcard = {

    getconfiguration: function(name) {

        ov_alsa.get_configuration(name);

    },

    deviceclick: function(name) {

        console.log("Click at "+name);
        ov_alsa.get_hw_params(name);
    },

    devicetone: function(name) {

        console.log("Click at tone "+name);
        ov_alsa.play_tone(name);
    },

    deviceset: function(name) {

        
        let multicast_ip = document.getElementById("input"+name).value;
        let direction = document.getElementById("select"+name);
        let dir = direction.options[direction.selectedIndex].text;

        console.log("Click at set "+name + " " + multicast_ip+ " "+ dir);

    },

    draw: function(name, soundcards){

        const newDiv = document.createElement("div");

        Object.entries(soundcards).forEach(([key, value]) => {

            const hw = document.createElement("div");
            newDiv.appendChild(hw);

            const name = document.createTextNode(key);
            hw.appendChild(name);

            Object.entries(value).forEach(([key, value]) => {

                const desc = document.createElement("div");
                hw.appendChild(desc);

                const name1 = document.createElement("div");
                desc.appendChild(name1);
                name1.style.float = "left";

                const name1text = document.createTextNode(key);
                name1.appendChild(name1text);

                const name1colon = document.createTextNode(":");
                name1.appendChild(name1colon);

                const item = document.createElement("div");
                desc.appendChild(item);

                if (key == "devices") {

                } else {

                    const itemname = document.createTextNode(value);
                    item.appendChild(itemname);
                }

            });

            Object.entries(value).forEach(([key, value]) => {

                const desc = document.createElement("div");
                hw.appendChild(desc);

                const item = document.createElement("div");
                desc.appendChild(item);

                if (key == "devices") {

                    const ruler = document.createElement("hr");
                    item.appendChild(ruler);

                    var nametag = "name";

                    Object.entries(value).forEach(([key, value]) => {

                        Object.entries(value).forEach(([key, value]) => {

                            const device = document.createElement("div");
                            item.appendChild(device);

                            if (key == "name"){
                                nametag = value;
                            }

                            const devitem = document.createTextNode(key);
                            device.appendChild(devitem);

                            const devspace = document.createTextNode(" ");
                            device.appendChild(devspace);

                            const devitemvalue = document.createTextNode(value);
                            device.appendChild(devitemvalue);
                        
                        });

                        const divid = document.createElement("div");
                        divid.setAttribute("id", nametag);
                        divid.style.background.color = "red";
                        divid.setAttribute("style","display:block;");
                        divid.style.display = "block";

                        item.appendChild(divid);

                        const selectdir = document.createElement("select");
                        selectdir.setAttribute("id", "select" + nametag);
                        divid.appendChild(selectdir);

                        var option = document.createElement("option");
                        option.text = "none";
                        selectdir.add(option);

                        option = document.createElement("option");
                        option.text = "Input";
                        selectdir.add(option); 

                        option = document.createElement("option");
                        option.text = "Output";
                        selectdir.add(option); 

                        var text = document.createTextNode("Multicast IP");
                        divid.appendChild(text);

                        var textinput = document.createElement("input");
                        textinput.setAttribute("id", "input" + nametag);
                        divid.appendChild(textinput);

                        const button0 = document.createElement("button");
                        divid.appendChild(button0);
                        button0.setAttribute("name", nametag);
                        button0.setAttribute("onclick", 'ov_soundcard.deviceset(this.name)');
                        button0.textContent = "Set";

                        const button1 = document.createElement("button");
                        item.appendChild(button1);
                        button1.setAttribute("name", nametag);
                        button1.setAttribute("onclick", 'ov_soundcard.deviceclick(this.name)');
                        button1.textContent = "HW Params";
                        button1.style.paddingRight = "10px";

                        const button2 = document.createElement("button");
                        item.appendChild(button2);
                        button2.setAttribute("name", nametag);
                        button2.setAttribute("onclick", 'ov_soundcard.devicetone(this.name)');
                        button2.textContent = "Play tone";
                        button2.style.paddingRight = "10px";

                        this.getconfiguration(nametag);

                        const ruler2 = document.createElement("hr");
                        item.appendChild(ruler2);

                                
                    });


                } 

            });

            const ruler1 = document.createElement("hr");
            hw.appendChild(ruler1);

        });

        var originalDiv = document.getElementById(name);
        originalDiv.parentNode.insertBefore(newDiv, originalDiv); 

    }

};