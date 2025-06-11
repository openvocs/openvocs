# LOGIN

## ui - success

* [ ] login button is disabled
* [ ] typ in correct username and password 
* [ ] login button is enabled
* [ ] press on button triggers login
* [ ] enter triggers login
* [ ] after login list of roles is displayed
* [ ] click on role end the authorization process

## ui - authentication error

* [ ] typ in wrong credentials 
* [ ] the password field gets cleared 
* [ ] an login failed message is displayed
* [ ] login with correct credentials
* [ ] login succeeds

## server sync 

* [ ] same as "ui - success", validate the process for both servers in console

## server sync - authentication error

* [ ] same as "ui - authentication error"

## server sync - prime succeeds, backup doesn't

* [ ] have different auth config for both serves
* [ ] use authentication for login on prime server
* [ ] login succeeded
* [ ] verify disconnect notice in browser console
* [ ] backup gateway does not try to reconnect

## server sync - backup succeeds, prime doesn't

* [ ] have different auth config for both serves
* [ ] use authentication for login on the backup server
* [ ] error is shown
* [ ] login to backup server is never triggered

## server sync - prime gateway disconnects before login

* [ ] change prime websocket url to something that does not work
* [ ] load page and log in
* [ ] connection error should be displayed
* [ ] login to backup server is never triggered

## server sync - prime gateway disconnects after login

* [ ] login in
* [ ] shut down prime gateway
* [ ] disconnect notice should be displayed
* [ ] client doesn't try to reconnect

# LOGIN - VOCS

## success - single server

* [ ] configure only one server
* [ ] login
* [ ] choose role
* [ ] vocs client gets loaded 

## server sync success 

* [ ] configure at least two server
* [ ] login
* [ ] choose role
* [ ] vocs client gets loaded 
* [ ] verify that all server are connected (server list in gui or console)

## only prime succeeded, backend doesn`t

* [ ] have different auth config for both serves
* [ ] use authentication for login on prime server
* [ ] login succeeded
* [ ] verify disconnect notice in browser console
* [ ] backup gateway does not try to reconnect
* [ ] chose role
* [ ] vocs client gets loaded
* [ ] backup server tries to reconnect

## backup server disconnected

* [ ] login
* [ ] disconnect backup gateway
* [ ] choose role
* [ ] verify in console that the backup client tries to reconnect
* [ ] restart backup gateway
* [ ] verify over the server list in the gui or the console that backup server reconnect was successful

# VOCS

## ui

* [ ] loops display correct number of participants
* [ ] enter loop (recv) -> blue, participants +1
* [ ] switch loop again (send) -> green
* [ ] leave loop -> gray, participants -1
* [ ] enter two loops, one in send
* [ ] switch other to send -> only the second one should be in send. the other switches to recv
* [ ] toggle mute/talk button -> name is displayed in send loop
* [ ] switch other to send -> name is displayed only in new send loop
* [ ] leave loop -> name is no longer displayed (mute/talk button is still active)
* [ ] enter loop in send -> name is displayed again

## verify audio and volume

* [ ] login with two devices on the same server
* [ ] join the same loop
* [ ] speak with first device and listen on the second
* [ ] change volume meter to be high and/or low
* [ ] observe corresponding changes in the audio 

## verify persistence

* [ ] join loops
* [ ] change volume level in a joined loop
* [ ] change volume level in a not joined loop
* [ ] logout
* [ ] re-login
* [ ] the same loops are joined as before logout
* [ ] the loops have the same volume level
* [ ] listen to audio to verify the audio level is indeed low

## role change

* [ ] open menu slider
* [ ] select new role
* [ ] user gets logged in in selected role

## client sync - same user-role

* [ ] open two clients with same user-role login
* [ ] switch loop in one client
* [ ] other client should also switch
* [ ] switch volume in active loop
* [ ] other client should also switch
* [ ] switch volume in inactive loop
* [ ] other client should also switch
* [ ] activate PTT
* [ ] other client should NOT activate PTT

## client sync - same user, different role

* [ ] open two clients with same user but different roles
* [ ] switch loop in one client. loop should exists in both roles
* [ ] no loop should be switched in second client
* [ ] change volume level in active loop. loop should exists in both roles
* [ ] volume change should not change in second client
* [ ] change volume level in inactive loop. loop should exists in both roles
* [ ] volume change should not change in second client

## server sync

* [ ] connected with prime and back up server
* [ ] switch loop in prime
* [ ] control switch in backup (switch server to verify)
* [ ] change volume in a loop in backup
* [ ] control change of volume in prime

## server reconnect PRIME

* [ ] connected with prime and back up server
* [ ] kill prime server
* [ ] prime server tries to reconnect
* [ ] load screen is displayed
* [ ] restart prime ice-server
* [ ] prime server reconnects
* [ ] load screen disappears

## server reconnect - BACKUP

* [ ] connected with prime and back up server
* [ ] kill backup server
* [ ] backup server tries to reconnect
* [ ] switch loop in prime server
* [ ] restart backup ice-server
* [ ] backup server reconnects
* [ ] has same configuration as prime server (switch to backup server to verify)

todo: stop single instance? (only ice? or gateway or mixer?) 

## server reconnect - audio

* [ ] login with two devices on the same server
* [ ] join the same loop
* [ ] speak with first device and listen on the second
* [ ] restart prime server
* [ ] speak with first device and listen on the second

## server switch

* [ ] connected with prime and back up server
* [ ] login with second devices on prime server
* [ ] join the same loop
* [ ] speak with first device and listen on the second
* [ ] switch second devices to back server
* [ ] speak with second device and listen on the first
* [ ] on first device switch server to backup while still speaking
* [ ] kill prime server
* [ ] button on load screen opens menu slider
* [ ] prime server is displayed as disconnected + back up as connected
* [ ] switch server 
* [ ] loading screen disappears + loops are redrawn and can be switched
* [ ] switch back
* [ ] loading screen appears
* [ ] restart prime server
* [ ] prime server button is displayed as connected
* [ ] loading screen disappears
* [ ] prime server configuration has also changed according to what we changed on the backup server, no prime working again hint is shown
* [ ] kill prime server again
* [ ] switch server 
* [ ] restart prime server
* [ ] hint and start is shown, that prime is working again
* [ ] click on page, hides hint
* [ ] click on menu button hides star

## server prime-backup configuration

* [ ] do not have any server flagged as prime in vars.js
* [ ] prime server should be the request server
* [ ] flag one of the additional servers as prime
* [ ] prime server should be this server

## single server

* [ ] remove server backup list
* [ ] only one server button is displayed
* [ ] test ui
* [ ] verify audio
* [ ] test reconnect see server reconnect - PRIME

## reload page

* [ ] reload page
* [ ] user should be automatically logged in and role selected

## logout 

* [ ] logout
* [ ] login page should be displayed