<!--
SPDX-FileCopyrightText: 2025 German Aerospace Center (DLR)
SPDX-License-Identifier: Apache 2.0
-->

# **O P E N V O C S ®** 
##### README v.01.3 - Last Update 2025-07-01 - Status: ***DRAFT***


---
## Table of Content
<!--* -->
* [1. Introduction to OPENVOCS®](#Introduction)
* [2. Description and general overview](#Description)
* [3. Installation](#Installation)
    * [3.1 Installation - Linux](#Installation_linux)
    * [3.2 Installation - Base system](#Installation_baselinux)
* [4. HowTo OPENVOCS services](#OV_service)
* [5. The OPENVOCS API](#OV_api)
    * [5.1 Client API](#client_api)
* [6. Citation](#Citation)
* [7. Contributors](#Contributors)
* [8. Contact](#Contact)
* [9. Licenses](#Licenses)

---
## [1. Introduction to OPENVOCS®][toc]
OPENVOCS® is a pure software-based voice communication system developed within the department 'Space Operations and Astronaut Training'. It was developed especially for space mission operations. Therefore, the voice communication is organized via voice loops (multiple audio conferences held in parallel). The underlying system concept is a simple server-client structure. The voice interface can operate from within a standard internet browser (**Note** Firefox is tested and recommended, other browser also work, may lead to unexpected behavior).

In the following sections provide a detailed guide to build, install and use OPENVOCS®.

## [2. Description and general overview][toc]

Within the development some types of implementations are done under src
1. **lib**          core libraries
2. **service**      services using the libraries
3. **tools**        supporting development tools
4. **HTML**         html clients used for OPENVOCS®
5. **samples**      some samples to learn coding with OPENVOCS®

#### lib
The libraries provides the core functions to build services for an OPENVOCS® system implementation. Modules required to build an OPENVOCS® system are for example the openvocs_event_protocol, a websocket_protocol for transmission of the openvocs_event_protocol, a json_protocol for packaging of the openvocs_event_protocol, a server to wait for protocol messages and so on.
**lib** contains all abstract interfaces for OPENVOCS® services and the definition and implementation of actual required protocols and functionalities.

#### Service
A service on top of the library may be a complete OPENVOCS® instance, as client backend, or a part of a system backend. e.g. an authentication service, a media switching server, a signaling service and so on. An implemented service may provide the whole capability needed for a voice communication backend, or only a part of the functionality needed.

Each service have an independent documentation describing the service.

#### Tools
Tools are executable helper programs which may enhance the working experience of the specific intendent use case of the tool.

#### System requirements

Generally, the more resources provided, the more users will be able to speak in parallel without service degradation.
The following resources should suffice for at least 4 users that speak in parallel:

* Intel Core i7/6600U CPU @ 2.60GHz with 4 Cores (or comparable)
* 8 GB RAM
* 30 GB of Harddisk

These are the ***minimal** requirements - more users should be able to use OPENVOCS® with these resources, but at some point the service will degrade, e.g. the audio will be corrupted and&or users might not be able to login.

***BEWARE*** The maximum number of users is determined by the number of mixer processes started.

## [3. Installation][toc]

Building OPENVOCS® is straight forward and requires 3 steps:

1) install required dependencies \
2) source env.h to setup the build environment \
3) run make

### [3.1. Installation - Linux][toc]
#### Installing required libraries

To build OPENVOCS® some libraries must be present.
Libraries are preconfigured for Debian 12. For other distros you may install the required libraries manually.
To show required libraries you may use:

```
source env.sh
scripts/show_packages.sh debian
```

To install all required libraries use:

```
for item in `./scripts/show_packages.sh debian`; do sudo apt install -y $item; done
```



You will have to take into consideration that OPENVOCS® creates abundantly lot of logging, which might causes problems if you do not restrict the size of the logs retained.
You will have to ensure you got access to the systemd journal if you want to inspect the logs.

#### Buildung OPENVOCS®

The first step on building OPENVOCS® is to source the environment variables. This should be done using:

```
source env.sh
```

Once the environment is ready OPENVOCS® may be build using:

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

### [3.2 Installation - Base system][toc]
To install the base system you should build a package for your distribution. Supported distributions is Debian 12.

#### Build a package for Debian:

```
make deb
```
Once the build is completed the package will be available under the ./build folder.
The debian based packages a file like openvocs_1.2.2-1505_amd64.deb will be build.
You may install the package using dpkg

```
dpkg -i build/openvocs_<version>-<build_id>_amd64.deb
```

OPENVOCS® is now installed within your system. Important folder are configuration, which is installed to /etc/openvocs and HTML sources, which are installed under /srv/openvocs/HTML.

#### Finish the installation

Now move to /etc/openvocs/ You must now use the configuration script to configure openvocs for your IP environment.

```
sudo ./ov_config.sh <your ip address>
```

This will preconfigure all of the configuration files:

- /etc/openvocs/ov_mc_ice_proxy/config.json
- /etc/openvocs/ov_mc_vocs/config.json
- /etc/openvocs/ov_mc_mixer/config.json
- /etc/openvocs/ov_mc_vad/config.json

In addition it will generate a certificate, which may be used for OPENVOCS® operations.
**NOTE**: feel encouraged to use your own certificates for OPENVOCS®, if necessary

To finish the installation you should restart the systemctl daemon

```
sudo systemctl daemon-reload
```

#### Start the system
To start the OPENVOCS® system use:

```
sudo systemctl start ov_mc.target
```

This is the final target for OPENVOCS® within your systemd configuration. ov_mc.target will start all subtargets and subservices.

#### Buildung OPENVOCS® library

The first step on building OPENVOCS® is to source the environment variables. This should be done using:

```
source env.sh
```

Once the environment is ready OPENVOCS® may be build using:

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

## [4. HowTo OPENVOCS® services][toc]
The OPENVOCS® service tree will look as follows:
```
ov_mc.target
|__ ov_mc_vocs.service
|__ ov_mc_vad.service
|__ ov_mc_ice_proxy.service
|__ ov_mc_mixer@.service
|__ ov_mc_mixer.target
```
it may contain other sub services e.g. recorder, SIP gateway or ALSA gateway, dependent on the installed system base.

#### ov_mc.target
Main target for OPENVOCS®. Should be used to start, stop, restart the whole system.

#### ov_mc_vocs.service
Main service of OPENVOCS®. ov_mc_vocs contains the whole business logic of OPENVOCS® including permission and access management for connections, as well as the core webserver for client delivery and client interaction.

#### ov_mc_vad.service
This service is the voice activity indication service and used exclusively for this purpose. If voice activity indication is not working as expected you may reset this service.

#### ov_mc_ice_proxy.service
This is one instance of the ice proxy, the WebRTC Endpoint within OPENVOCS®. For simplicity only one ICE service is startet by default, but you may start several ICE services in parallel e.g. for load balancing.

The ICE service is responsible for audio distribution within the system. It will route incomming audio from WebRTC connection to mutlicast media streams and outgoing streams of a users media mixer to the WebRTC connection of that user.

#### ov_mc_mixer@.service
One instance of a ov_mixer microservice. The more mixers the system starts, the more users are supported. A mixer may be started on different hardware or cloud based systems.

#### ov_mc_mixer.target
The mixer definition of the current machine. This target defines how many mixer instances shall be started.

## [5. The OPENVOCS® API][toc]
The OPENVOCS® API is the client interface for OPENVOCS® communication. Using the API allows to build dedicated clients. Implementing the API allows to build dedicated servers. The API is the minimum information exchange definition required to run OPENVOCS® services.

### [5.1 Client API][toc]
#### Logout
Logout is used to close a user session.

```
{
	"event" : "logout"
}
```

#### Login
Login is used to start a user session.

The response to this event should contain a sessionid to be used for login updates.

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
**Note**: Depending on the version, this may not available.

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
**Note**: Depending on the version, Telephony may not available.

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
#### Switch loop volume
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
#### Call from a loop
Call from a loop to some SIP number. Works only with SIP gateway connected.

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
Hangup a call. Works only with SIP gateway connected.

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
Permit some callin to some loop. Works only with SIP gateway connected.

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
Revoke a previously set permission of some callin to some loop. Works only with SIP gateway connected.

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
List active calls. Works only with SIP gateway connected.

```
{
	"event" : "list_calls",
	"client" : <client uuid>,
	"parameter" :{}
}
```
#### List active call permissions
List active call permissions. Works only with SIP gateway connected.

```
{
	"event" : "list_permissions",
	"client" : <client uuid>,
	"parameter" :{}
}
```
#### List current SIP status
List SIP status. Works only with SIP gateway connected.

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


## [6. Citation][toc]
If you use this work in a publication, please cite using the citation:

***TODO*** ... here Bibtex or Zenodo Link

## [7. Contributors][toc]
Here you find the main contributors to the material:

- Markus Töpfer
- Michael Beer
- Anja Bertard
- Tobias Kolb
- Falk Schiffner

## [8. Contact][toc]
http://openvocs.org


## [9. License][toc]
Please see LICENSES for further information about how the content is licensed.




[toc]: #table-of-content "go to Table of Content"
