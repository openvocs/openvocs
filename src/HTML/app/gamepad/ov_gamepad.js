/*
 * 	ov_gamepad.js
 * 	Author: Markus Töpfer
 *
 *      ------------------------------------------------------------------------
 *
 *      #GLOBAL Variables
 *
 *      ------------------------------------------------------------------------
 */

var ptt = false;

window.addEventListener("gamepadconnected", (e) => {
	console.log("Gamepad connected");
  	const gp = navigator.getGamepads()[e.gamepad.index];
  console.log(
    "Gamepad connected at index %d: %s. %d buttons, %d axes.",
    gp.index,
    gp.id,
    gp.buttons.length,
    gp.axes.length,
  );

  setInterval(function(){
    isPressed = gp.buttons[0].pressed;
    document.getElementById("PTT").innerHTML = isPressed;

    if (isPressed != ptt){
    	console.log("PTT change to " + isPressed);
    	ptt = isPressed;
    }

  }, 100)

});

window.addEventListener("gamepaddisconnected", (e) => {
  console.log("Gamepad disconnected." + e);
});



 var ov_gamepad = {

 	init: function(){

        console.log("ov_gamepad loaded.");
        console.log("navigator supports gamepads " + !!(navigator.getGamepads));
    }

 };


 ov_gamepad.init();