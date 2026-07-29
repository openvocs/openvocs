# Changelog

## Version 3.0.1 - 2026-07-DD

### ADDED
- Implemented mixer spawning ?
- Added autostart for mixers in interconnect ? 
- Added option to import roles from LDAP. Configuration inside ov_mc_vocs/config.json.
- Added explanation on how SIP whitelisting works to SIP admin interface.
#### GUI
- Added fluent UI system icons as font for symbols.
- Users, roles and loops can now be moved between domain and projects scope.
- Added logout button on initial (empty) page to clean session, in case session reconnection fails
- Added logout button on role selection page in vocs ui

### CHANGED
- SSL/ov_io ?
- New Webserver ?
- Renamed API events.
#### GUI
- Changed disconnect, reconnect and autologin handling in webclient.
- Merge domain settings and project settings views in admin interface. 
- Changed Admin interface to now display and a edit the whole domain including its projects without having to return to the landing page.
- Change storage and handling of the domain data in admin interface.
- Changed saving routine of domain config in admin interface. Only save changes not everything all the time. 
- Only allow access to SIP admin interface, if SIP server is online.
- Auth for LDAP import is now configured inside ov_mc_vocs/config.json. Removed input mask in vocs admin interface. Reload domain from server after manual ldap import.
- Added graphical feedback to manual LDAP import.
- Remove unused users if users and roles are managed over LDAP.
- Changed layout of GUI screen keyboard. Added "Space" to keyboard. Removed "Enter". Added an upper shift option.
- Only display roles in the GUI authorization list that have a defined loop layout. Added scrolling to this list.
- Volume icon in loops change depending on volume.
- Toggle button icon for audio test in vocs settings slider.
- Prevent fttp display to show up on page load for a brief moment.
- Re-add setting to have a locked PTT screen button.
- Change position of "server" and "roles" labels.
- Reorganize settings slider.
- Change defaults to not log events send to and from the server.
- Use default grid layout (5x6), if non was provided by server. 

### FIXED
- Fixed issues with interconnect.
#### GUI
- Fixed font type and size inconsistencies in GUIs.
- Fixed unintentional deleting of users imported from LDAP during project save.
- It is no longer possible to authenticate with LDAP if the user is not imported into openvocs.
- When opening login page the caret is now displayed inside the user field.
- Fixed name display.

### REMOVED
#### GUI
- Remove landing page in admin interface.
- Remove grouping (and resorting) of nodes in rbac view

## Version 2.6.2 - 2026-05-08

- Recorder fixes added. 
- Recordings will be stored permanently on save in user interface.