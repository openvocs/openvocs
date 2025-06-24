<!--
SPDX-FileCopyrightText: 2025 German Aerospace Center (DLR)
SPDX-License-Identifier: Apache 2.0
-->
<div style="text-align: center;">
<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/ov_black.png" alt="OV_LOGI" title="Total Time Human in Space" />

# **O P E N V O C S ®**
## Manual v.01.1 - Last Update 2025-05-21 - Status: ***DRAFT***
</div>

---
## Table of Content
<!--* -->
* [Introduction](#Introduction)
* [Description and general overview](#Description)
* [OPENVOCS](#OPENVOCS)
* [Installation](#Installation)
    * [Installation - Linux](#Installation_linux)
    * [Installation - Base system](#Installation_baselinux)
    * [Installation - Windows](#Installation_win)
    * [Installation - MacOS](#Installation_macos)
* [HowTo OPENVOCS services](#OV_service)
* [The OPENVOCS API](#OV_api)
* [Usage](#usage)
    * [Voice Client](#usageVoice)
    * [Admin Client](#usageAdmin)
* [Citation](#Citation)
* [Contributors](#Contributors)
* [Contact](#Contact)
* [Licenses](#Licenses)

---
## 1. Introduction
Voice communication is a crucial part to successfully conduct a space missions.
To have an voice system, that is able to provide all necessary functionality is highly demanded. That is the reason the German Aerospace Center (Deutsches Zentrum für Luft und Raumfahrt - DLR)
have started the project "OPENVOCS®" to develop a new dynamic voice conferencing
system.

This document presents basic guidelines for using and administration the OPENVOCS® software.

The OPENVOCS® system is a pure software-based voice communication system developed by DLR e.V.  It was developed especially for space mission operations. Therefore, the voice communication is organized via voice loops (multiple audio conferences held in parallel). Further, the system provides a role-based access control, where depended on specific user roles, the access to voice loops is restricted. Moreover the allowance to only monitor or talk in a voice loop is differentiated.

---
## 2. OPENVOCS®
OPENVOCS is ...


In the following sections provide a detailed guide to build, install and use OPENVOCS.

## Description and general overview

Within the development some types of implementations are done under src
1. **lib**          core libraries
2. **service**      services using the libraries
3. **tools**        supporting development tools
4. **HTML**         html clients used for openvocs
5. **samples**      some samples to learn coding with openvocs

#### lib
The libraries provides the core functions to build services for an openvocs system implementation. Modules required to build an openvocs system are for example the openvocs_event_protocol, a websocket_protocol for transmission of the openvocs_event_protocol, a json_protocol for packaging of the openvocs_event_protocol, a server to wait for protocol messages and so on.
**lib** contains all abtract interfaces for openvocs services and the definition and implementation of actual required protocols and functionalities.

#### Service
A service on top of the library may be a complete openvocs instance, as client backend, or a part of a system backend. e.g. an authentication service, a media switching server, a signaling service and so on. An implemented service may provide the whole capability needed for a voice communication backend, or only a part of the functionality needed.

Each service have an independent documentation describing the service.

#### Tools
Tools are executable helper programs which may enhance the working experience of the specific intendent use case of the tool.

## Installation

Building openvocs is straight forward and requires 3 steps:

1) install required dependencies \
2) source env.h to setup the build environment \
3) run make

### Installation - Linux
#### Installing required libraries

To build openvocs some libraries must be present.
To show required libraries you may use:

```
scripts/show_packages.sh debian
```

To install all required libraries use:

```
for item in `./scripts/show_packages.sh ubuntu`; do sudo apt install -y $item; done
```

Libraries are preconfigured for Ubuntu, Debian and Suse variants. For other distros you may install the required libraries manually.

#### Buildung OPENVOCS

The first step on building openvocs is to source the environment variables. This should be done using:

```
source env.sh
```

Once the environment is ready openvocs may be build using:

```
make
```

This will build the whole project.

#### cleaning up a build

To cleanup a build use:

```
make clean
```

---

#### How to build openvocs library

The openvocs library may be installed to the local system for development against openvocs.

#### Installing required libraries

To build openvocs some libraries must be present. To show required libraries you may use:

```
scripts/show_packages.sh debian
```

To install all required libraries use:

```
for item in `./scripts/show_packages.sh ubuntu`; do sudo apt install -y $item; done
```

Libraries are preconfigured for Ubuntu, Debian and Suse variants. For other distros you may install the required libraries manually.

#### buildung openvocs

The first step on building openvocs is to source the environment variables. This should be done using:

```
source env.sh
```

Once the environment is ready openvocs may be build using:

```
make && make target_install_lib
```

```
This will build the whole project and install the library packages at:

- /usr/include/openvocs/
- /usr/lib/openvocs/
- /usr/lib/pkgconfig/
```
---
### Installation - Base system
To install the base system you should build a package for your distribution. Supported distributions are Debian, Suse and Ubuntu.

#### Build a package for Debian or Ubuntu:

```
make deb
```

#### Build a package for Suse

```
make rpm
```

Once the build is completed the package will be avaialable under the ./build folder.
e.g. for Debian based packages a file like openvocs_1.2.2-1505_amd64.deb will be build.
You may install the package using dpkg

```
dpkg -i build/openvocs_<version>-<build_id>_amd64.deb
```

Openvocs is now installed within your system. Important folder are configuration, which is installed to /etc/openvocs and HTML sources, which are installed under /srv/openvocs/HTML.

#### Finish the installation

Now move to /etc/openvocs/ov_mc_vocs. You must now use the configuration script to configure openvocs for your IP environment.

```
sudo ./ov_config.sh <your ip address>
```

This will preconfigure all of the configuration files:

- /etc/openvocs/ov_mc_ice_proxy/config.json
- /etc/openvocs/ov_mc_vocs/config.json
- /etc/openvocs/ov_mc_mixer/config.json
- /etc/openvocs/ov_mc_vad/config.json

In addition is will generate a certificate, which may be used for openvocs operations.
NOTE feel encouraged to use your own certificates for openvocs.

To finish the installation you should restart the systemctl daemon

```
sudo systemctl daemon-reload
```

#### Start the system
To start the openvocs system use:

```
sudo systemctl start ov_mc.target
```

This is the final target for openvocs within your systemd configuration. ov_mc.target will start all subtargets and subservices.


---


### Installation - Windows
To be able to run OPENVOCS in a Window machine, you need to install the Windows-Subsystem-for-Linux (WSL):

1. [Install WSL](https://www.roberts999.com/posts/2018/11/wsl-coding-windows-ubuntu#201811-GetWsl)
   1. Enable the windows subsystem for linux in windows features
      1. Open “turn Windows features on or off” by searching under the start menu
      2. Check the “windows subsystem for linux” option
      3. You will need to restart your pc for this to take affect
   2. Install ubuntu (or another linux distro)
      * ATTENTION: Ubuntu-20.04 does currently not work with WSL 1
      * from windows store: Search “windows store” in the start menu, then search for “ubuntu”
      * manual: see [microsoft docs](https://docs.microsoft.com/en-us/windows/wsl/install-manual)
   3. The first time running wsl, it will prompt you to setup a user account; just follow the prompts
2. Make sure the project is checked out with unix file formatting. Git may have converted the files to dos formatting on checkout.

```
// configure git project to checkout unix file formatting on windows
git config core.eol lf
git config core.autocrlf input

// fetch project content again (with the correct file formatting) and reset local version
git fetch origin master
git reset --hard FETCH_HEAD
git clean -df
```

3. In WSL navigate to openvocs project. If the project is located in the Windows part of your system, you can find it under `/mnt/c/...`.
4. You should use an editor that works directly in WSL (e.g. VSCode/VSCodium with the [WSL plugin](https://code.visualstudio.com/docs/remote/wsl)* to edit the project.
5. Continue with installation guide for [linux](#Linux)

\* With VSCode with WSL you may run into problems with Git (see [VSCode issue](https://github.com/microsoft/vscode-remote-release/issues/903)).
[This comment](https://github.com/microsoft/vscode-remote-release/issues/903#issuecomment-521304085) fixed it for me:
```
"remote.WSL.fileWatcher.polling": true,
"files.watcherExclude": {
   "**/.git/**": true
}
```

### Installation - MacOS
To build openvocs on macos a few tools should be installed

#### pkgconfig

pkg-config is used during linking within the openvocs makefile,
so it should be installed

```
sudo port -d selfupdate
sudo port install pkgconfig
```

You may want to make a link under /usr/local/bin to avoid PATH related find
problems within xcode.

```
ln -s /opt/local/bin/pkg-config /usr/local/bin/pkg-config
```

#### required libraries

```
sudo port install openssl libopus libsrtp
```


#### Testing

To run some tests under MacOS the dynamic library path MUST be hand over to
the testrun e.g.

```
DYLD_LIBRARY_PATH=$OPENVOCS_LIBS ./build/test/ov_base/ov_config_test
```

To run all tests of some module, you may used
```
./testrunnner.sh ov_base
```

---

## HowTo openvocs services
The openvocs service tree will look as follows:
```
ov_mc.target
|__ ov_mc_vocs.service
|__ ov_mc_vad.service
|__ ov_mc_ice_proxy.service
|__ ov_mc_mixer@.service
|__ ov_mc_mixer.target
```
it may contain other sub services e.g. recorder, sip gateway or alsa gateway, dependent on the installed system base.

#### ov_mc.target
Main target for openvocs. Should be used to start, stop, restart the whole system.

#### ov_mc_vocs.service
Main service of openvocs. ov_mc_vocs contains the whole business logic of openvocs including permission and access management for connections, as well as the core webserver for client delivery and client interaction.

#### ov_mc_vad.service
This service is the voice activity indication service and used exclusively for this purpose. If voice activity indication is not working as expected you may reset this service.

#### ov_mc_ice_proxy.service
This is one instance of the ice proxy, the WebRTC Endpoint within openvocs. For simplicity only one ICE service is startet by default, but you may start several ICE services in parallel e.g. for load balancing.

The ICE service is responsible for audio distribution within the system. It will route incomming audio from WebRTC connection to mutlicast media streams and outgoing streams of a users media mixer to the WebRTC connection of that user.

#### ov_mc_mixer@.service
One instance of a ov_mixer microservice. The more mixers the system starts, the more users are supported. A mixer may be started on different hardware or cloud based systems.

#### ov_mc_mixer.target
The mixer definition of the current machine. This target defines how many mixer instances shall be started.

## The OPENVOCS API
The openvocs API is the client interface for openvocs communication. Using the API allows to build dedicated clients. Implementing the API allows to build dedicated servers. The API is the minimum information exchange definition required to run openvocs services.

### Client API
#### Logout
Logout is used to close a user session.

```
{
	"event" : "logout"
}
```

#### Login
Login is used to start a user session.

The Response to this event should contain a sessionid to be used for login updates.

```
{
	"event" : "login",
	"client" : <client uuid>,
	"parameter" :
	{
		"user" : <username>,
		"password" : <user password or session id if session was established>
	}
}
```

#### Update Login
Update Login is used to update a session and to prevent the session closure.

A session is a temporary login stored within the browser to prevent storage of user passwords. Instead of some password some session id will be used.
Once the client lost a connection to the server, the session can be restored automatically without a new login.

```
{
	"event" : "update_login",
	"client" : <client uuid>,
	"parameter" :
	{
		"user" : <username>,
		"session" : <session id>
	}
}
```

#### Media
Media is used to transfer media parameter between client and server.

```
{
	"event" : "media",
	"client" : <client uuid>,
	"parameter" :
	{
		"sdp" : <session description>,
		"type" : <"request" | "offer" | "answer">
	}
}
```

#### Candidate
Candidate is used to transfer ICE candidates between client and server.

```
{
	"event" : "candidate",
	"client" : <client uuid>,
	"parameter" :
	{
		"candidate": <candidate_string>,
		"SDPMlineIndex" : <sdp media description index>,
		"SDPMid" : <sdp media id>,
		"ufrag" : <user fragment>,
	}
}
```

#### End of Candidates
End of Candidates is used to transfer ICE candidates end between client and server.

```
{
	"event" : "end_of_candidates",
	"client" : <client uuid>,
	"parameter" : {}
}
```

#### Client authorize
Authorize a session. (Select role)

```
{
	"event" : "update_login",
	"client" : <client uuid>,
	"parameter" :
	{
		"role" : <rolename>
	}
}
```
### Client Get
Get some information from server. Get user must be implemented to get the users data.

```
{
	"event" : "get",
	"client" : <client uuid>,
	"parameter" :
	{
		"type" : "user"
	}
}
```
#### User Roles
Get user roles information from server. Returns a list of user roles.

```
{
	"event" : "user_roles",
	"client" : <client uuid>,
	"parameter" :{}
}
```
#### Role Loops
Get role loops information from server.
Returns a list of user loops.

```
{
	"event" : "role_loops",
	"client" : <client uuid>,
	"parameter" :{}
}
```
#### Get Recordings
Get a list of recordings from server.
Returns a list of recordings.

```
{
	"event" : "get_recordings",
	"client" : <client uuid>,
	"parameter" :
	{
		"loop" : <looname>
		"user" : <optional username>
		"from" : <from epoch>
		"to"   : <to epoch>
	}
}
```
### SIP Status

Get the SIP status from server.
If a SIP gateway is connected the result will contain a SIP:true.

```
{
	"event" : "role_loops",
	"client" : <client uuid>,
	"parameter" :{}
}
```
#### Switch loop state
Switch a loopstate.

```
{
	"event" : "switch_state",
	"client" : <client uuid>,
	"parameter" :
	{
		"loop" : <loopname>,
		"state": <"recv" | "send" | "none">
	}
}
```
### Switch loop volume
Switch loop volume.

```
{
	"event" : "switch_volume",
	"client" : <client uuid>,
	"parameter" :
	{
		"loop" : <loopname>,
		"volume": <0 .. 100>
	}
}
```
#### Switch Push To Talk
Switch Push to Talk state.

```
{
	"event" : "switch_ptt",
	"client" : <client uuid>,
	"parameter" :
	{
		"loop" : <loopname>,
		"state": <true, false>
	}
}
```
### Call from a loop
Call from a loop to some SIP number.
Works only with sip gateway connected.

```
{
	"event" : "call",
	"client" : <client uuid>,
	"parameter" :
	{
		"loop" : <loopname>,
		"dest" : <destination number>,
		"from" : <optional from number>
	}
}
```
#### Hangup a call from a loop
Hangup a call.
Works only with sip gateway connected.

```
{
	"event" : "hangup",
	"client" : <client uuid>,
	"parameter" :
	{
		"call" : <callid>,
		"loop" : <loopname>
	}
}
```
#### Permit a call to a loop
Permit some callin to some loop.
Works only with sip gateway connected.

```
{
	"event" : "permit",
	"client" : <client uuid>,
	"parameter" :
	{
		"caller" : <callerid>,
		"callee" : <calleeid>,
		"loop"   : <loopname>,
		"from"   : <from epoch>
		"to"     : <to epoch>
 	}
}
```
#### Revoke a call to a loop
Revoke a previously set permission of some callin to some loop.
Works only with sip gateway connected.

```
{
	"event" : "revoke",
	"client" : <client uuid>,
	"parameter" :
	{
		"caller" : <callerid>,
		"callee" : <calleeid>,
		"loop"   : <loopname>,
		"from"   : <from epoch>
		"to"     : <to epoch>
 	}
}
```
### List active calls
List active calls.
Works only with sip gateway connected.

```
{
	"event" : "list_calls",
	"client" : <client uuid>,
	"parameter" :{}
}
```
#### List active call permissions
List active call permissions.
Works only with sip gateway connected.

```
{
	"event" : "list_permissions",
	"client" : <client uuid>,
	"parameter" :{}
}
```
#### List current SIP status
List SIP status.
Works only with sip gateway connected.

```
{
	"event" : "list_sip",
	"client" : <client uuid>,
	"parameter" :{}
}
```
#### Set Keyset layout
Set a keyset layout of some client.

```
{
	"event" : "set_keyset_layout",
	"client" : <client uuid>,
	"parameter" :
	{
		"domain" : "<domainname>",
        "name"   : "<name>",
        "layout"   : {}
	}
}
```
#### Get Keyset layout
Get a keyset layout for some client.

```
{
	"event" : "get_keyset_layout",
	"client" : <client uuid>,
	"parameter" :
	{
		"domain" : "<domainname>",
        "layout"   : "layoutname"
	}
}
```
#### Set user data
Set the users data.

```
{
	"event" : "set_user_data",
	"client" : <client uuid>,
	"parameter" :
	{
		...
	}
}
```
#### Get user data
Get the users data.

```
{
	"event" : "get_user_data",
	"client" : <client uuid>,
	"parameter" : {}
}
```
#### Register a client
Register a client to receive systembroadcasts.

```
{
	"event" : "register",
	"client" : <client uuid>,
	"parameter" : {}
}
```
---
### Usage
In the following the usage of OPENVOCS is shown. This section starts with explaining the voice client and is followed by explaining the administration interface.

### Voice Client

### Login and role selection
- Login website => enter username and password to authenticate, click LOGIN-button to continue.
- By clicking the circle arrow symbol, one can reload the login-page.

<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/01_Login.png" alt="login" width="40%"/>

- Role selection => select the role or project by clicking on it (only the roles and projects are shown that are allowed for the user).
- The voice loops associated with the role/project and the corresponding layout is loaded automatically.

<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/02_Role_Selection.png" alt="RolesProjects" width="40%"/>

- After the role is selceted the voice client will start automatically.
- Note: The role can be switched later in directly in the side menue, shown later in this guide.

### User interface - voice client
- Start of voice client GUI (switched off - all tiles are in grey color).
- If all tiles are grey, even if there is voice traffic, no audio is audible.
- It can be possible that there are more then one page with voice loop tiles.
- The page selection is placed on the lower-right corner and numbered.


<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/03_Loop_off.png" alt="GUI_VL_OFF" width="70%"/>

---
#### ***Voice client - tile elements***
<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/03_GUI_OFF_Kachel_new.png" alt="login" width="40%"/>

- The name or abbreviation of the voice loop is placed in the middle of the tile.
- Headphones and microphone symbols indicates allowance in a loop.
- Headphones only => only monitoring of the loop is allowed.
- Headphones and microphone => talking is also allowed in this loop.
- Icon with number => shows the number of active users in the loop.
- These areas are used to switch mode of the tile (see next section).
- The "OFF" button is dedicated to switch off this specific voice loop, and ends the audio transmission.
- Speaker icon opens a slider => change the volume of the loop.
- Note: Telephony Panel – is deactivated in the opensource version of OPENVOCS.


##### Change the mode from "monitoring" to "talk"
- 1st click on the tile (in grey OFF-state) will activate the monitoring mode first, indicated via blue color.
- 2nd click on the tile will activate the talking mode, indicated via green color.
- 3rd click on the tile will activate the monitoring mode again (blue color).
- To switch the loop OFF (back to grey color), click on the OFF-Button in the right corner.
- It is possible to select several voice loops for monitoring mode, but only one for talking mode.
- If a new voice loop is selected for talking mode, the fromer voice loop will switch back to monitoring automatically.
- Note: If a tile only have a headphones symbol, the talking mode is not accessible.

---
#### ***Voice client (monitoring: blue / talking: green -- switched ON)***
<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/04_Loop_Talk.png" alt="login" width="70%"/>

#### ***Voice client (talking: green -- PTT-bar switched ON)***
- To speak in a voice loop, the voice loop tile needs to be in talking mode (green color).
- In addition the PTT bar needs to be pressed.
- Three types of conformation about an open microphone are given:
  - 1. The PTT-bar will change to green color when pressed to indicate that talking is activated.
  - 2. A small audio waveform will be shown according to the microphone signal.
  - 3. The name/role of the user, that has an open microphone will appear under the tile name.
  - 4. The white frame indicates voice traffic on a voice loop.

<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/05_active-waveform.png" alt="login" width="70%"/>

- Note: Depending on the configuration of OPENVOCS®, a screen PTT is also available, or only the PTT on the headset can be used.


#### ***Voice client (volume slider -- 0-100 => 50 default)***
- Volume control panel, after touching the speaker symbol, the volume slider shows up and the bar can be moved up (louder) and down (quieter), the number on the side indicated the volume silder value:

    - 0 – no sound at all
    - 100 – maximal loudness

-To close the volume slider, one have to touch the speaker symbol again.

<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/06_GUI_Vol_Slider_new.png" alt="login" width="40%"/>

#### ***Voice client (menue opened via clicking on the OPENVOCS® logo)***
- On top the user name is depicted.
- Below that the role/project that is currently chosen is shown.
- In the server panel, all available OPENVOCS® server are shown. The server marked blue, is the server currently connected. The green dot, indicates the availability of the server. Is this indicator red, the server is not available.
- Further, a page reload button (circle arrows symbol) and additional options can be found (gear symbol).
- At the bottom is a red logout-button located, clicking on it, will disconnect and close the voice client GUI, leading back to the login page.

<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/07_menu-open.png" alt="login" width="70%"/>

<!--
#### ***Voice client (settings opened via gearbox icon => 1. selftest and design)***
- Here, two options are loaceted.
  - 1. Audio selftest (check for hearing audio by clicking on the 'play' button and microphone check => shown by moving waveform accordingly to the microphone signal)
  - 2. User interface options (here, the user can chose between a dark and light mode for the GUI and if the user wants to shown the user name or the role when pressing the PTT bar)

<img src="Images/08_GUI_Menue_Settings_1.png" alt="login" width="50%"/>

#### ***Voice client (settings opened via gearbox icon => 2. PTT options)***
- In the advanced setting submenue there is the possibility to adjust the scaling of GUI via entering a scaling factor
- Further, several different options to adjust the behavior of the opening of the microphone are given (e.g. change the PTT (press and release) to TTT (switch on/off) behavior).

<img src="Images/09_GUI_Menue_Settings_2.png" alt="login" width="50%"/>

-->




### Admin Client
#### Setup and configure a project/missions
- After login the administration interface starts with a selection of a project (e.g. missions).
- The list of existing project will be shown.
- To administrate a project simply choose the project be clicking on it.
- For creating a new project, the "+" icon is selected.
- The gear wheel in the top-right corner is for additional options.

<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/01_project_select.png" alt="placeholder" width="70%"/>

- After selecting a project, the administration interface with all available options opens.
- On this page one can also delete the project, by opening the "delete" menue.
- Changes can be saved via the "Save" button place at the bottom.

<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/02_project_open.png" alt="placeholder" width="70%"/>


#### Setup: Role - Voice Loop - User
- To configure, add, delete or change anything according to roles, voice loops and user one open via access management, on the panel at the bottom.
- Note: Depending on the version of OPENVOCS, a SIP and RECORDER setup option are available.

<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/03_role_vl_user.png" alt="placeholder" width="70%"/>

- Here all users, roles and voice loops for that project are shown.
- Moreover the connection between users, their roles and available voice loops are graphically indicated.
- For a better overview, one can hide unused users, roles, and voice loop, for that project, via the options hidden under the hamburger menue on the top-left corner.
- One can add new users, roles and voice loops by clicking on each "+" symbol

<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/03_role_options.png" alt="placeholder" width="70%"/>

- For changes on a user, role, or voice loop, click on the gear symbol, that appears when hoovering over it.
- In addition a linking / delinking symbol will appear.
- By clicking on it, one can create a link between as user and role, and link a role to a set of voice loops.
- The color will change to yellow, when selected.
- To exit the selection mode, one clicks on the door-exit symbol on the selected panel.

<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/03_role_vl_user_link_01.png" alt="placeholder" width="70%"/>

<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/03_role_vl_user_link_02.png" alt="placeholder" width="70%"/>

<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/03_role_vl_user_link_03.png" alt="placeholder" width="70%"/>


#### Setup voice loop layout
- After selecting layout from the bottom panel, the layout setup  starts.
- Under burger menue on upper-left, general layout options are placed.
        - Number of packages
        - Scale
        - Layout tpy (grid or list)
        - Columns and rows
        - Name and font scaling

<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/06_layout_grid_option.png" alt="placeholder" width="70%"/>

---
- In the drop-down menue at the top, the layout for specific roles in the project can be selected.
- After selecting on role the current layout for that role is loaded.
- Note: When starting a new project, the grid will be empty.

<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/04_layout_01.png" alt="placeholder" width="70%"/>

<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/05_layout_grid.png" alt="placeholder" width="70%"/>

- To add a new voice loop to the grid or change an existing voice loop position, one select on tile.
- The tile will change to a green frame, to indicate the selection.
- In addition a drop-down menue will appear.
- From that menue, a voice loop can be selected.
- Note: Only the voice loops that are linked before for that role will be available.
- The selection of the tile will end automatically when a new voice loop was selected or when clicking on the surrounding areas around the drop-down menue.


#### Import and Export Configurations
- When clicking on the OPENVOCS logo in the lower-left corner one ca find the options to import or export saved configurations.
- Via import, are previously saved configuration file (json) can be loaded-
- Via export, a json-file is directly download to the download folder on the system.
- The back button brings one back to the project selection page.
- Further, the logout button in located there.

<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/07_admin_imexport.png" alt="placeholder" width="70%"/>

#### LDAP import
- With this option one can import data from LDAP.
<img src="https://github.com/openvocs/openvocs/blob/dev/Handbook_DRAFT/images/08_LDAP_import.png" alt="placeholder" width="70%"/>



---

## Citation
If you use this work in a publication, please cite using the citation:

***TODO*** ... here Bibtex or Zenodo Link

## Contributors
Here you find the main contributors to the material:

- Markus Töpfer
- Michael Beer
- Anja Bertard
- Tobias Kolb
- Falk Schiffner

## Contact
http://openvocs.org


## License]
Please see LICENSES for further information about how the content is licensed.

---


